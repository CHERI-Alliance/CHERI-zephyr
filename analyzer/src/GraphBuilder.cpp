/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GraphBuilder.h"
#include "Globals.h"
#include <algorithm>
#include <set>
#include "llvm/Support/raw_ostream.h"

GraphBuilder::GraphBuilder(AnalysisResult& result, const KernelSymbolFilter& kernelFilter,
                           bool allowLeaks)
    : result_(result), kernelFilter_(kernelFilter), allowLeaks_(allowLeaks) {}

void GraphBuilder::buildGraph() {
    std::map<std::string, std::map<std::string, std::vector<Interaction>>> objToInteractions;

    for (const auto& interaction : result_.interactions) {
        if (interaction.threadName.empty() || interaction.objectName.empty() ||
            interaction.objectName == "?") continue;
        objToInteractions[interaction.objectName][interaction.threadName].push_back(interaction);
    }

    for (const auto& objPair : objToInteractions) {
        const std::string& objectName = objPair.first;
        const auto& threadInteractions = objPair.second;

        if (threadInteractions.size() < 2) continue;

        auto* obj = result_.findObject(objectName);
        if (!obj) continue;

        // App-safety: the prefix blocklist exists to suppress subsystem
        // primitives (logging, networking, timers, HAL). Never let it drop
        // a symbol that originates from the application under analysis —
        // those are genuine data dependencies, exactly what must survive.
        // Origin is determined by the object's source path (SDK marker),
        // NOT by the is_system flag: several corpus projects set
        // --app-source-prefix to the repo root, which misclassifies the
        // entire SDK as "app". isSdkSource is the reliable discriminator.
        if (KernelSymbolFilter::isSdkSource(obj->sourceFile) &&
            kernelFilter_.isExcluded(objectName)) continue;

        if (obj->type == ObjectType::Mutex) {
            ContentionGroup group;
            group.objectName = objectName;
            group.objectType = obj->type;
            group.groupType = EdgeType::MutualExclusion;
            group.severity = Severity::Critical;

            for (const auto& tPair : threadInteractions) {
                group.threads.push_back(tPair.first);
                for (const auto& i : tPair.second) {
                    group.evidence.push_back(i);
                }
            }

            contentionGroups_.push_back(group);
        } else {
            for (auto it1 = threadInteractions.begin(); it1 != threadInteractions.end(); ++it1) {
                for (auto it2 = std::next(it1); it2 != threadInteractions.end(); ++it2) {
                    const std::string& thread1 = it1->first;
                    const std::string& thread2 = it2->first;

                    std::vector<Interaction> evidence1;
                    for (const auto& i : it1->second) {
                        evidence1.push_back(i);
                    }
                    std::vector<Interaction> evidence2;
                    for (const auto& i : it2->second) {
                        evidence2.push_back(i);
                    }

                    EdgeType type1to2 = ThreadEdge::inferDirectionalEdgeType(*obj, it1->second);
                    EdgeType type2to1 = ThreadEdge::inferDirectionalEdgeType(*obj, it2->second);

                    addEdge(thread1, thread2, objectName, type1to2, evidence1);
                    addEdge(thread2, thread1, objectName, type2to1, evidence2);
                }
            }
        }
    }
}

void GraphBuilder::addGlobalVarEdges(const std::map<std::string, std::map<std::string, GlobalVarAccess>>& globalVarAccesses) {
    for (const auto& varPair : globalVarAccesses) {
        const std::string& varName = varPair.first;
        const auto& threadAccesses = varPair.second;

        // App-safety guard for the global-var path (see the object path above):
        // apply the prefix blocklist only to SDK-origin (or unknown-origin)
        // variables, never to application-origin globals.
        auto* varObj = result_.findObject(varName);
        bool sdkOrigin = varObj ? KernelSymbolFilter::isSdkSource(varObj->sourceFile) : true;
        if (sdkOrigin && kernelFilter_.isExcluded(varName)) continue;

        if (threadAccesses.size() < 2) continue;

        std::vector<std::string> writers;
        std::vector<std::string> readers;
        std::vector<Interaction> allEvidence;

        for (const auto& tPair : threadAccesses) {
            const GlobalVarAccess& access = tPair.second;
            const std::string& thread = tPair.first;

            Interaction evidence;
            evidence.threadName = thread;
            evidence.objectName = varName;
            evidence.action = (access.accessType == "write" || access.accessType == "readwrite")
                            ? ActionType::Write : ActionType::Read;
            evidence.location = access.location;
            evidence.resolved = true;
            allEvidence.push_back(evidence);

            if (access.accessType == "write" || access.accessType == "readwrite") {
                writers.push_back(thread);
            } else {
                readers.push_back(thread);
            }
        }

        Severity severity;
        if (writers.size() >= 2) {
            severity = Severity::High;
        } else if (writers.size() == 1 && !readers.empty()) {
            severity = Severity::Medium;
        } else {
            severity = Severity::Low;
        }

        bool anyAtomic = false;
        bool anyVolatile = false;
        bool anyConst = false;
        bool anyPrimitiveScalar = false;
        for (const auto& tPair : threadAccesses) {
            if (tPair.second.isAtomic) anyAtomic = true;
            if (tPair.second.isVolatile) anyVolatile = true;
            if (tPair.second.isConst) anyConst = true;
            if (tPair.second.isPrimitiveScalar) anyPrimitiveScalar = true;
        }

        if (anyAtomic) {
            if (severity == Severity::High) severity = Severity::Low;
            else if (severity == Severity::Medium) severity = Severity::Low;
        } else if (anyVolatile) {
            if (severity == Severity::High) severity = Severity::Medium;
            else if (severity == Severity::Medium) severity = Severity::Low;
        }

        // Primitive-scalar downgrade. A primitive scalar (bool/char/
        // int/long/float/double/enum and fixed-width aliases) fits in
        // a single naturally-aligned bus transaction on every Zephyr
        // target we support (Cortex-M, RISC-V, POSIX native_sim) and
        // therefore cannot tear under mid-write crash — the value left
        // in the global is either the old or the new word, never a
        // half-and-half mix. The only way a corrupt primitive can
        // propagate damage to another thread is via the *index/handle*
        // pattern (the thread crashes after writing the primitive but
        // before completing some related state update elsewhere, and
        // another thread uses the primitive's (consistent, valid)
        // value as an index/handle to mutate shared state). That
        // indirect propagation, when it occurs, is captured by an
        // independent `SharedMemory` edge on the indexed struct/array
        // (which is not a primitive scalar and so retains its full
        // severity). The primitive edge itself can therefore safely
        // cap at Medium: it remains mandatory for direct readers
        // (propagation is preserved via `produces_for`), but its
        // severity no longer inflates deeper hop-count transitive
        // reachability into HIGH. This pattern (monotonic debug
        // counters, error flags, last-error codes such as
        // `csp_dbg_buffer_out++`, `csp_dbg_errno`, log counters, and
        // errno-style globals) is exactly the residual spurious-
        // coupling class flagged in section 9.3 of the review report.
        if (anyPrimitiveScalar && severityRank(severity) > severityRank(Severity::Medium)) {
            severity = Severity::Medium;
        }

        // D2: write-once init-table demotion. A global whose only writers are
        // init-thread-attributed (bg_thread_main / SYS_INIT wrappers / work q
        // system init) is presumed populated at boot and only read at runtime.
        // The corresponding edge is demoted to EdgeType::InitTable, severity
        // Low, and excluded from ImpactAnalyzer propagation, so it no longer
        // drags every reader into the crash-impact set of the boot thread.
        bool allWritersAreInit = !writers.empty() && std::all_of(
            writers.begin(), writers.end(),
            [](const std::string& t){ return isInitThread(t); });
        bool isBootInitTable = (allWritersAreInit && !readers.empty()) ||
                               (anyConst && !writers.empty() && !readers.empty()) ||
                               isLinkerSectionSymbol(varName);

        EdgeType groupEdgeType = EdgeType::SharedMemory;
        if (isBootInitTable) {
            severity = Severity::Low;
            groupEdgeType = EdgeType::InitTable;
        }

        // Item 1 rework: globals are now modelled with directed
        // producer->consumer edges only. The historical symmetric
        // `SharedMemory` content group — expanded into a bidirectional
        // clique by ImpactAnalyzer::buildFilteredAdjacency — is what
        // collapsed the thread graph into one giant SCC and made
        // `mandatoryByScc` classify every reachable thread as mandatory.
        //
        // The clique is preserved only for the InitTable tier (kept for
        // JSON reporting; InitTable groups are excluded from impact
        // propagation by shouldConsiderGroup, so the clique is inert).
        // For the active SharedMemory tier we no longer emit a content
        // group at all — propagation is carried by the directed edges
        // produced below.
        if (groupEdgeType == EdgeType::SharedMemory) {
            // No content group: rely on directed edges.
        } else {
            ContentionGroup group;
            group.objectName = varName;
            group.objectType = ObjectType::GlobalVar;
            group.groupType = groupEdgeType;
            group.severity = severity;

            for (const auto& tPair : threadAccesses) {
                group.threads.push_back(tPair.first);
            }
            group.evidence = allEvidence;

            contentionGroups_.push_back(group);
        }

        // Directed edges.
        //   writer -> reader : ProducesFor  (writer corrupts global; reader must restart)
        //   reader -> writer : (dropped)    reads don't mutate; reader crash can't corrupt writer
        //   writer -> writer : ProducesFor  **both directions** — multiple writers form an
        //                                    SCC because each may have cached a stale read.
        if (!isBootInitTable) {
            // writer -> reader
            for (const auto& writer : writers) {
                for (const auto& reader : readers) {
                    std::vector<Interaction> writerEvidence;
                    std::vector<Interaction> readerEvidence;
                    for (const auto& e : allEvidence) {
                        if (e.threadName == writer) writerEvidence.push_back(e);
                        if (e.threadName == reader) readerEvidence.push_back(e);
                    }
                    addEdge(writer, reader, varName, EdgeType::ProducesFor, severity, writerEvidence);
                }
            }
            // writer <-> writer (mutual). Only meaningful for >=2 writers;
            // for exactly one writer the self-skip in addEdge handles
            // the degenerate case.
            for (size_t i = 0; i < writers.size(); ++i) {
                for (size_t j = 0; j < writers.size(); ++j) {
                    if (i == j) continue;
                    std::vector<Interaction> wi;
                    std::vector<Interaction> wj;
                    for (const auto& e : allEvidence) {
                        if (e.threadName == writers[i]) wi.push_back(e);
                        if (e.threadName == writers[j]) wj.push_back(e);
                    }
                    addEdge(writers[i], writers[j], varName, EdgeType::ProducesFor, severity, wi);
                }
            }
        }
    }
}

void GraphBuilder::addEdge(const std::string& from, const std::string& to,
                          const std::string& objectName, EdgeType edgeType,
                          const std::vector<Interaction>& evidence) {
    if (from.empty() || to.empty() || from == to) return;
    Severity sev = ThreadEdge::inferSeverity(edgeType);
    // Heap is a shared mutable resource, not data dependency: a thread
    // crashing mid-allocation/free can corrupt heap metadata, and safe
    // recovery requires reinitialising the heap, which invalidates every
    // live allocation pointer across every thread that has ever touched
    // the heap. Coupling is therefore all-or-nothing with Critical
    // integrity requirements (matching mutual_exclusion semantics).
    if (objectName == "[system_heap]") {
        // --allow-leaks: leak-tolerant recovery — the edge is kept in the
        // graph (factual shared-heap usage) but downgraded to Low, and
        // ImpactAnalyzer excludes it from propagation. Default: Critical
        // all-or-nothing clique (see comment in the ImpactConfig).
        sev = allowLeaks_ ? Severity::Low : Severity::Critical;
    }
    addEdge(from, to, objectName, edgeType, sev, evidence);
}

void GraphBuilder::addEdge(const std::string& from, const std::string& to,
                          const std::string& objectName, EdgeType edgeType,
                          Severity severityOverride,
                          const std::vector<Interaction>& evidence) {
    if (from.empty() || to.empty() || from == to) return;

    ThreadEdge edge;
    edge.fromThread = from;
    edge.toThread = to;
    edge.objectName = objectName;
    edge.edgeType = edgeType;
    edge.severity = severityOverride;
    edge.evidence = evidence;

    edges_.push_back(edge);
}

std::string GraphBuilder::edgeTypeToString(EdgeType type) {
    switch (type) {
        case EdgeType::MutualExclusion: return "mutual_exclusion";
        case EdgeType::Signals: return "signals";
        case EdgeType::ProducesFor: return "produces_for";
        case EdgeType::ConsumesFrom: return "consumes_from";
        case EdgeType::Bidirectional: return "bidirectional";
        case EdgeType::SharedMemory: return "shared_memory";
        case EdgeType::InitTable: return "init_table";
case EdgeType::Join: return "join";
    case EdgeType::Abort: return "abort";
    case EdgeType::Wakeup: return "wakeup";
    default: return "unknown";
    }
}

void GraphBuilder::filterOmnipresentGroups(size_t totalThreadCount) {
    if (!kernelFilter_.excludeOmnipresent || totalThreadCount == 0) return;
    if ((int)totalThreadCount < kernelFilter_.omnipresentMinThreads) return;

    std::map<std::string, std::set<std::string>> objectThreadMap;
    for (const auto& g : contentionGroups_) {
        for (const auto& t : g.threads) {
            objectThreadMap[g.objectName].insert(t);
        }
    }
    for (const auto& e : edges_) {
        objectThreadMap[e.objectName].insert(e.fromThread);
        objectThreadMap[e.objectName].insert(e.toThread);
    }

    std::set<std::string> omnipresentObjects;
    for (const auto& [name, threads] : objectThreadMap) {
        if ((100.0 * threads.size()) / totalThreadCount >= kernelFilter_.omnipresentThreshold) {
            omnipresentObjects.insert(name);
        }
    }

    for (auto& g : contentionGroups_) {
        if (omnipresentObjects.count(g.objectName)) {
            g.highlyConnected = true;
        }
    }

    for (auto& e : edges_) {
        if (omnipresentObjects.count(e.objectName)) {
            e.highlyConnected = true;
        }
    }

    if (!omnipresentObjects.empty()) {
        llvm::errs() << "Detected " << omnipresentObjects.size() << " highly connected objects (>="
                     << kernelFilter_.omnipresentThreshold << "% of " << totalThreadCount << " threads):\n";
        for (const auto& name : omnipresentObjects) {
            llvm::errs() << "  " << name << " (" << objectThreadMap[name].size() << "/" << totalThreadCount << " threads)\n";
        }
    }

    result_.omnipresentFilteredCount = omnipresentObjects.size();
}
