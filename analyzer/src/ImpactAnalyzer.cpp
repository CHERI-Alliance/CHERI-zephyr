/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ImpactAnalyzer.h"
#include <algorithm>
#include <map>
#include <unordered_map>

ImpactAnalyzer::ImpactAnalyzer(const AnalysisResult& result,
                               const std::vector<ThreadEdge>& edges,
                               const ImpactConfig& config)
    : result_(result), edges_(edges), config_(config) {
    for (const auto& edge : edges_) {
        edgesByFromThread_[edge.fromThread].push_back(&edge);
    }
    for (const auto& t : result_.threads) {
        if (!t.isRestartable) nonRestartableThreads_.insert(t.name);
    }
}

bool ImpactAnalyzer::shouldConsiderGroup(const ContentionGroup& group) const {
    // D2: advisory init-table content groups never propagate.
    if (group.groupType == EdgeType::InitTable) return false;
    // B: respect --impact-omnipresent flag for highly-connected objects.
    if (!config_.includeOmnipresent && group.highlyConnected) return false;
    // --allow-leaks: heap content groups are advisory-only. Defensive:
    // GraphBuilder only emits groups for Mutex objects today, so a heap
    // group cannot currently exist, but the guard keeps the flag's
    // semantics airtight against future group-emitting paths.
    if (config_.allowLeaks && group.objectName == "[system_heap]") return false;
    return true;
}

bool ImpactAnalyzer::shouldConsiderEdge(const ThreadEdge* edge) const {
    if (edge->edgeType == EdgeType::InitTable) return false;
    if (!config_.includeOmnipresent && edge->highlyConnected) return false;
    // --allow-leaks: leak-tolerant heap recovery. The crashed thread may
    // leak its own allocations but the heap stays consistent, so the
    // shared heap is not a crash-coupling channel: heap edges are
    // excluded from propagation and land in filter_skipped_objects. The
    // default (flag off) keeps the CRITICAL all-or-nothing clique.
    if (config_.allowLeaks && edge->objectName == "[system_heap]") return false;
    return true;
}

std::map<std::string, std::set<std::string>> ImpactAnalyzer::buildFilteredAdjacency(
    std::set<std::string>* skippedObjects, bool severityGated) {
    std::map<std::string, std::set<std::string>> adj;

    // D4 severity gate: when an advisory threshold is configured, edges at
    // or below it are advisory — including for SCC formation. A thread
    // fused into the crashed thread's SCC only through such weak edges
    // (writer-writer cycles over downgraded primitive-scalar counters,
    // atomic bookkeeping globals) is not blocking-coupled: a weak cycle
    // cannot deadlock, so it must not force joint restart. Only edges
    // strictly stronger than the threshold build the gated ("strong")
    // adjacency used by mandatoryByScc. Gate inactive without a threshold
    // (historical semantics: the full graph forms the SCC).
    bool gateActive = severityGated && config_.advisorySeverityThreshold != Severity::None;
    auto gateExcludes = [&](Severity sev) {
        return gateActive &&
               severityRank(sev) <= severityRank(config_.advisorySeverityThreshold);
    };

    // Index all thread names.
    for (const auto& t : result_.threads) adj[t.name];

    // Edges (directed). For mutex/bidirectional groups we add a symmetric
    // counterpart via the content-group loop below.
    for (const auto& edge : edges_) {
        if (!shouldConsiderEdge(&edge)) {
            if (skippedObjects) skippedObjects->insert(edge.objectName);
            continue;
        }
        if (gateExcludes(edge.severity)) continue;
        if (!shouldPropagate(&edge)) continue;
        if (edge.fromThread == edge.toThread) continue;
        adj[edge.fromThread].insert(edge.toThread);
    }

    // Content groups: contribute a clique (every pair of group threads is
    // mutually adjacent in both directions for propagating-edge categories,
    // matching GraphBuilder's pre-existing impact behaviour). Mutex groups
    // (mutual_exclusion), bidirectional mbox/pipe, and shared_memory groups
    // are inherently undirected for adjacency purposes.
    //
    // Item 2 defensive guard: active SharedMemory content groups are no
    // longer emitted by GraphBuilder::addGlobalVarEdges as of the item 1
    // rework (producer/consumer globals now use directed edges only, since
    // a reader crash cannot corrupt a global that the reader only read).
    // SharedMemory groups that survive into `contentionGroups` are
    // either InitTable (excluded by shouldConsiderGroup) or stale from a
    // future regression. Skip expanding them as a clique here so any such
    // stale group cannot silently collapse the SCC again; propagation
    // flows through the directed edges (`edges_`) instead.
    for (const auto& group : result_.contentionGroups) {
        if (!shouldConsiderGroup(group)) {
            if (skippedObjects) skippedObjects->insert(group.objectName);
            continue;
        }
        if (gateExcludes(group.severity)) continue;
        if (group.groupType == EdgeType::SharedMemory) continue;
        for (const auto& a : group.threads) {
            for (const auto& b : group.threads) {
                if (a != b) adj[a].insert(b);
            }
        }
    }

    return adj;
}

// Tarjan's strongly-connected-components (iterative — large graphs would
// overflow the recursive form, and Zephyr projects routinely have 20+ threads).
// Implementation reads the prebuilt adjacency list; output is one sccId per
// thread name. Component IDs are contiguous from 0.
std::map<std::string, int> ImpactAnalyzer::computeSCCs() {
    return computeSCCsOver(buildFilteredAdjacency(/*skippedObjects=*/nullptr,
                                                   /*severityGated=*/false));
}

std::map<std::string, int> ImpactAnalyzer::computeSCCsOver(
    const std::map<std::string, std::set<std::string>>& adj) {
    std::map<std::string, int> index, lowlink, sccId;
    std::map<std::string, bool> onStack;
    std::vector<std::string> stack;
    int idx = 0;
    int sccCount = 0;

    // Iterative Tarjan. Each Frame owns its children list so that the
    // iterator survives push_back()s into the call stack and doesn't
    // dangle once the originating vector goes out of scope.
    struct Frame {
        std::string v;
        std::vector<std::string> children;
        size_t idx;
    };

    for (const auto& [v, _] : adj) {
        if (index.count(v)) continue;

        std::vector<Frame> callStack;
        std::vector<std::string> v_children(adj.at(v).begin(), adj.at(v).end());
        Frame f; f.v = v; f.children = std::move(v_children); f.idx = 0;
        callStack.push_back(std::move(f));
        index[v] = lowlink[v] = idx++;
        stack.push_back(v);
        onStack[v] = true;

        while (!callStack.empty()) {
            // Re-fetch a fresh reference every iteration: push_back may
            // reallocate the call stack, invalidating any prior reference.
            Frame& top = callStack.back();
            if (top.idx >= top.children.size()) {
                if (lowlink[top.v] == index[top.v]) {
                    std::string w;
                    do {
                        w = stack.back(); stack.pop_back();
                        onStack[w] = false;
                        sccId[w] = sccCount;
                    } while (w != top.v);
                    sccCount++;
                }
                // Capture state from the frame we are about to pop because
                // callStack.pop_back may invalidate `top` once the parent
                // is reached.
                std::string v_name = top.v;
                int lowlink_v = lowlink[top.v];
                (void)v_name;
                callStack.pop_back();
                if (!callStack.empty()) {
                    Frame& parent = callStack.back();
                    lowlink[parent.v] = std::min(lowlink[parent.v], lowlink_v);
                }
            } else {
                std::string w = top.children[top.idx];
                ++top.idx;
                if (index.count(w) == 0) {
                    std::vector<std::string> w_children;
                    auto wIt = adj.find(w);
                    if (wIt != adj.end()) {
                        w_children.assign(wIt->second.begin(), wIt->second.end());
                    }
                    Frame nf; nf.v = w; nf.children = std::move(w_children); nf.idx = 0;
                    callStack.push_back(std::move(nf));
                    index[w] = lowlink[w] = idx++;
                    stack.push_back(w);
                    onStack[w] = true;
                } else if (onStack[w]) {
                    lowlink[top.v] = std::min(lowlink[top.v], index[w]);
                }
            }
        }
    }

    return sccId;
}

// Compute the (forward-compat historical) full-transitive crash-impact set,
// and additionally populate CrashImpact.restartSet with the B
// mandatory/advisory/filter_skipped partition.
CrashImpact ImpactAnalyzer::computeCrashImpact(const std::string& crashedThread) {
    CrashImpact impact;
    impact.crashedThread = crashedThread;

    // Build a list of filter-skipped objects (collected only when the
    // omnipresent filter is active). The set is the same regardless of which
    // thread crashed, so we compute it once per call (small relative to the
    // BFS anyway).
    std::set<std::string> skippedObjects;
    buildFilteredAdjacency(&skippedObjects);
    impact.filterSkippedObjects.assign(skippedObjects.begin(), skippedObjects.end());

    struct WorkItem {
        std::string thread;
        std::string reason;
        Severity severity;
        std::vector<std::string> path;
        int hop;
    };

    std::vector<WorkItem> worklist;
    std::map<std::string, Severity> bestSeverity;

    worklist.push_back({crashedThread, "crashed", Severity::Critical, {crashedThread}, 0});
    bestSeverity[crashedThread] = Severity::Critical;

    while (!worklist.empty()) {
        WorkItem current = worklist.back();
        worklist.pop_back();

        Severity previousBest = bestSeverity.count(current.thread) ? bestSeverity[current.thread] : Severity::None;
        if (severityRank(current.severity) < severityRank(previousBest)) continue;

        if (current.thread != crashedThread) {
            impact.affectedThreads[current.thread] = {
                current.reason,
                current.severity,
                current.path
            };
        }

        for (const auto& group : result_.contentionGroups) {
            if (std::find(group.threads.begin(), group.threads.end(), current.thread) == group.threads.end()) continue;
            if (!shouldConsiderGroup(group)) continue;

            Severity pathSeverity = severityFromRank(
                std::min(severityRank(current.severity), severityRank(group.severity)));

            for (const auto& otherThread : group.threads) {
                if (otherThread == current.thread) continue;

                Severity propagatedSeverity = propagateSeverity(pathSeverity, current.hop + 1);

                Severity otherBest = bestSeverity.count(otherThread) ? bestSeverity[otherThread] : Severity::None;
                if (severityRank(propagatedSeverity) <= severityRank(otherBest)) continue;

                std::string reason = current.reason + " -> " + otherThread +
                                     " via " + group.objectName + " (" + edgeTypeToString(group.groupType) + ")";

                std::vector<std::string> newPath = current.path;
                newPath.push_back(group.objectName);
                newPath.push_back(otherThread);

                bestSeverity[otherThread] = propagatedSeverity;
                worklist.push_back({
                    otherThread,
                    reason,
                    propagatedSeverity,
                    newPath,
                    current.hop + 1
                });
            }
        }

        auto it = edgesByFromThread_.find(current.thread);
        if (it == edgesByFromThread_.end()) continue;

        for (const auto* edge : it->second) {
            if (!shouldConsiderEdge(edge)) continue;
            if (!shouldPropagate(edge)) continue;

            Severity edgePathSeverity = severityFromRank(
                std::min(severityRank(current.severity), severityRank(edge->severity)));
            Severity propagatedSeverity = propagateSeverity(edgePathSeverity, current.hop + 1);

            Severity otherBest = bestSeverity.count(edge->toThread) ? bestSeverity[edge->toThread] : Severity::None;
            if (severityRank(propagatedSeverity) <= severityRank(otherBest)) continue;

            std::string reason = current.reason + " -> " + edge->toThread +
                                 " via " + edge->objectName + " (" +
                                 edgeTypeToString(edge->edgeType) + ")";

            std::vector<std::string> newPath = current.path;
            newPath.push_back(edge->objectName);
            newPath.push_back(edge->toThread);

            bestSeverity[edge->toThread] = propagatedSeverity;
            worklist.push_back({
                edge->toThread,
                reason,
                propagatedSeverity,
                newPath,
                current.hop + 1
            });
        }
    }

    // B: build restart-set partition from the BFS result + SCCs.
    //
    // The SCC of the crashed thread forms the mandatory set (every thread in
    // the same SCC must restart jointly because every thread — including the
    // crashed one — deadlocks otherwise). Additionally, every thread reached
    // via a first-order (direct) CRITICAL/HIGH blocking-edge or content group
    // of the SCC is mandatory. Threads reached only via deeper hops, via
    // MEDIUM/LOW shared memory, or via init tables are advisory (when they
    // appear in affected_threads at all).
    //
    // D4 (recommendation 4): the SCC above is computed over the
    // severity-gated adjacency — cycles formed only by edges at or below the
    // advisory threshold (downgraded primitive-scalar debug counters,
    // atomic bookkeeping globals: csp_dbg_*, soc_cpus_active-style writer
    // cliques) do NOT force joint restart, because a weak edge cannot block
    // and therefore a weak cycle cannot deadlock. Such threads fall through
    // to the ordinary hop+severity classification below (typically
    // advisory). Mutex groups (Critical) and HIGH producer/consumer cycles
    // still fuse. Without a configured advisory threshold the gate is off
    // (historical full-graph SCC).
    //
    // mandatoryHopDepth == -1 (default): all threads that appear in
    //   affected_threads are mandatory (preserves historical full-coverage
    //   semantics when the B flags are unset).
    // mandatoryHopDepth == 0: only the crashed thread's SCC is mandatory.
    // mandatoryHopDepth == 1: SCC + direct CRITICAL/HIGH neighbours (the
    //   design-intent "minimum restartable set").
    //
    // To support mandatoryHopDepth == 1 we need to track reachability hop per
    // affected thread. The BFS above already tracks hop; rebuild hop info per
    // successor thread using a secondary single-source BFS over the
    // filter-respecting graph.

    auto sccMap = computeSCCsOver(buildFilteredAdjacency(/*skippedObjects=*/nullptr,
                                                          /*severityGated=*/true));
    int crashedSccId = -1;
    auto it = sccMap.find(crashedThread);
    if (it != sccMap.end()) crashedSccId = it->second;
    std::set<std::string> crashedScc;
    if (crashedSccId >= 0) {
        for (const auto& [t, id] : sccMap) {
            if (id == crashedSccId) crashedScc.insert(t);
        }
    }

    // Hop-bounded reachability from the crashed thread over the filtered
    // adjacency. We always explore to unbounded depth so that every
    // reachable affected thread receives a `hopFromCrashed` entry and can
    // be classified correctly. The `mandatoryHopDepth` cap is consulted
    // only by the `mandatoryByHop` decision below (line ~335); classifying
    // deeper threads as advisory requires that they actually appear in
    // `hopFromCrashed`. Capping the BFS at `mandatoryHopDepth` would
    // leave deeper threads absent from the map, misclassifying them as
    // `FilterSkipped` (or, when `includeOmnipresent` is true, forcing them
    // into advisory via the fall-through rather than via hop-depth logic).
    auto adj = buildFilteredAdjacency(/*skippedObjects=*/nullptr);
    std::map<std::string, int> hopFromCrashed;
    std::vector<std::string> bfs{crashedThread};
    hopFromCrashed[crashedThread] = 0;
    while (!bfs.empty()) {
        std::string u = bfs.back(); bfs.pop_back();
        int uh = hopFromCrashed[u];
        for (const auto& w : adj[u]) {
            if (hopFromCrashed.count(w)) continue;
            hopFromCrashed[w] = uh + 1;
            bfs.push_back(w);
        }
    }

    // Classify each thread that appears in the historical affected_threads
    // block. Threads not reached in the BFS (omnipresent-filter exit) are
    // recorded as filter_skipped.
    //
    // Non-restartable threads (isr / idle / bg_thread_main / native_sim
    // host threads) are cut from the restart set: they remain in
    // affected_threads (factual impact) and keep propagating through the
    // graph (they are full members of `adj` / `crashedScc` above), but
    // "restart the ISR" is not an actionable recovery step.
    std::set<std::string> banAdvisory;
    for (const auto& [t, _] : impact.affectedThreads) banAdvisory.insert(t);

    for (const auto& [t, _] : impact.affectedThreads) {
        if (nonRestartableThreads_.count(t) > 0) continue;

        RestartEntry r;
        r.thread = t;
        r.reason = impact.affectedThreads[t].reason;
        r.severity = impact.affectedThreads[t].severity;
        r.path = impact.affectedThreads[t].path;

        // Filter-skipped: this is a thread that the BFS did reach (since it
        // appears in affected_threads — the historical propagation may have
        // used a skipped object via the bestSeverity race). Practically we
        // never reach this case as long as the group-loop above is enforced
        // by shouldConsiderGroup; but a defensive classification handles
        // future variants.
        auto hopIt = hopFromCrashed.find(t);
        bool inFilteredAdjacency = (hopIt != hopFromCrashed.end());

        // mandatoryByScc: membership in the crashed thread's SCC over the
        // severity-GATED adjacency (see the D4 note above) — only cycles of
        // edges stronger than the advisory threshold force joint restart.
        bool mandatoryByScc = crashedScc.count(t) > 0;
        // mandatoryByHopDepth: reached within the mandated hop depth via the
        // filtered adjacency (which already strips highlyConnected when
        // includeOmnipresent is false).
        bool mandatoryByHop = inFilteredAdjacency &&
                              (config_.mandatoryHopDepth < 0 ||
                               hopIt->second <= config_.mandatoryHopDepth);
        // Severity-gated mandatory: true if the affected thread's recorded
        // severity is strictly greater than the advisory threshold.
        bool mandatoryBySeverity = (config_.advisorySeverityThreshold == Severity::None) ||
                                   (severityRank(impact.affectedThreads[t].severity) >
                                    severityRank(config_.advisorySeverityThreshold));

        if (!inFilteredAdjacency && !config_.includeOmnipresent) {
            // The thread was reached only because the historical full-transitive
            // BFS used an edge/group we now filter. Classify as filter_skipped
            // so downstream consumers know to discount it.
            r.restartClass = RestartClass::FilterSkipped;
        } else if (mandatoryByScc ||
                   (config_.mandatoryHopDepth < 0) ||
                   (mandatoryByHop && mandatoryBySeverity)) {
            r.restartClass = RestartClass::Mandatory;
        } else {
            r.restartClass = RestartClass::Advisory;
        }

        impact.restartSet.push_back(r);
    }

    // Always include the crashed thread itself in the restart set. When
    // the crashed thread is itself non-restartable (isr / idle /
    // bg_thread_main / native_sim host thread) the entry is emitted with
    // RestartClass::NonRestartable: recovery of the crashed entity needs
    // a system-level action, while the remaining restart_set members are
    // what must restart regardless.
    RestartEntry self;
    self.thread = crashedThread;
    if (nonRestartableThreads_.count(crashedThread) > 0) {
        self.restartClass = RestartClass::NonRestartable;
        self.reason = "crashed (non-restartable thread; recovery requires system-level restart)";
    } else {
        self.restartClass = RestartClass::Mandatory;
        self.reason = "crashed";
    }
    self.severity = Severity::Critical;
    self.path = {crashedThread};
    impact.restartSet.push_back(self);

    return impact;
}

Severity ImpactAnalyzer::propagateSeverity(Severity severity, int hop) {
    if (hop <= 1) return severity;

    int rank = severityRank(severity);
    int newRank = rank - (hop - 1);

    if (newRank <= 0) return Severity::Low;
    if (newRank == 1) return Severity::Low;
    if (newRank == 2) return Severity::Medium;
    if (newRank == 3) return Severity::High;
    return Severity::Critical;
}

bool ImpactAnalyzer::shouldPropagate(const ThreadEdge* edge) {
    switch (edge->edgeType) {
        case EdgeType::MutualExclusion:
        case EdgeType::Signals:
        case EdgeType::ProducesFor:
        case EdgeType::ConsumesFrom:
        case EdgeType::Bidirectional:
        case EdgeType::SharedMemory:
        case EdgeType::Join:
        case EdgeType::Abort:
        case EdgeType::Wakeup:
            return true;
        // D2: init-table edges are advisory only — boot-time
        // registration tables (e.g. SHELL_CMD_REGISTER, MODULE_DECLARE)
        // are written once before the scheduler runs and only read at
        // runtime; their writers do not pull readers into the crash
        // impact set.
        case EdgeType::InitTable:
        default:
            return false;
    }
}

std::string ImpactAnalyzer::edgeTypeToString(EdgeType type) {
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