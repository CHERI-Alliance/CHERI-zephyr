/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Globals.h"
#include "Visitors.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"

#include "GraphBuilder.h"
#include "ImpactAnalyzer.h"
#include "ReportGenerator.h"

#include <iostream>
#include <fstream>
#include <algorithm>

namespace {
    llvm::cl::OptionCategory CrashAnalyzerCategory("Crash Analyzer Options");

    llvm::cl::opt<std::string> ZephyrBase(
        "zephyr-base",
        llvm::cl::desc("Path to Zephyr base directory"),
        llvm::cl::value_desc("path"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<std::string> OutputFormat(
        "output",
        llvm::cl::desc("Output format: json, text, or dot"),
        llvm::cl::value_desc("format"),
        llvm::cl::init("json"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<std::string> OutputFile(
        "output-file",
        llvm::cl::desc("Output file path"),
        llvm::cl::value_desc("path"),
        llvm::cl::init("-"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<std::string> ImpactOf(
        "impact-of",
        llvm::cl::desc("Compute crash impact for this thread"),
        llvm::cl::value_desc("thread_name"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<bool> ImpactAll(
        "impact-all",
        llvm::cl::desc("Compute crash impact for every thread and include in JSON output"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::list<std::string> ExtraSources(
        "extra-source",
        llvm::cl::desc("Additional source files to analyze (for cross-TU call graph)"),
        llvm::cl::value_desc("file"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<bool> UseCompdb(
        "use-compdb",
        llvm::cl::desc("Auto-discover source files from the compilation database"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<std::string> AppSourcePrefix(
        "app-source-prefix",
        llvm::cl::desc("Path prefix for user application source files (used to classify threads/objects as user vs system)"),
        llvm::cl::value_desc("path"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<std::string> Scope(
        "scope",
        llvm::cl::desc("Output scope: 'user' (only user threads/objects) or 'all' (including system)"),
        llvm::cl::value_desc("scope"),
        llvm::cl::init("user"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<bool> FilterKernelSymbols(
        "filter-kernel-symbols",
        llvm::cl::desc("Filter kernel symbols: exclude log metadata, downgrade _kernel to Medium severity"),
        llvm::cl::cat(CrashAnalyzerCategory));

    // B: SCC + first-order-critical-edge min-cut / restart-partition flags.
    // Defaults preserve historical full-transitive behaviour so existing
    // consumers see no change in `affected_threads` until the new
    // `restart_set` field is read.
    llvm::cl::opt<bool> ImpactOmnipresent(
        "impact-omnipresent",
        llvm::cl::desc("Include highlyConnected (omnipresent) edges/groups in impact "
                       "propagation. ON by default (preserves historical behaviour). "
                       "Pass --impact-omnipresent=false to filter subsystem primitives "
                       "(mpsc_pbuf_buffer::sem, system_heap) from the propagation graph."),
        llvm::cl::init(true),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<int> ImpactHopDepth(
        "impact-hop-depth",
        llvm::cl::desc("Maximum BFS hop for mandatory restart set membership. "
                       "-1 = unbounded (historical behaviour: all affected threads "
                       "are mandatory). 0 = SCC only. 1 = SCC + first-order "
                       "CRITICAL/HIGH blocking edges (the design-intent minimum "
                       "restartable set)."),
        llvm::cl::init(-1),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<std::string> ImpactAdvisorySeverity(
        "impact-advisory-severity",
        llvm::cl::desc("Edges at or below this severity are classified as advisory "
                       "(reported but not mandatory). One of NONE/LOW/MEDIUM/HIGH/"
                       "CRITICAL. NONE (default) = no advisory tier, all threads in "
                       "the impact set are mandatory."),
        llvm::cl::init("NONE"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::list<std::string> NonRestartableThreads(
        "non-restartable",
        llvm::cl::desc("Additional thread name(s) to treat as not individually "
                       "restartable: excluded from restart_set output (kept as "
                       "propagation nodes and in affected_threads). Complements "
                       "the built-in classification of isr/idle/bg_thread_main/"
                       "native_sim host threads. May be given multiple times."),
        llvm::cl::value_desc("thread_name"),
        llvm::cl::cat(CrashAnalyzerCategory));

    llvm::cl::opt<bool> AllowLeaks(
        "allow-leaks",
        llvm::cl::desc("Leak-tolerant heap recovery: assume a crashed thread leaks "
                       "its own allocations without corrupting heap metadata, so "
                       "other [system_heap] users need not restart. Heap edges are "
                       "downgraded to LOW and excluded from impact propagation "
                       "(reported in filter_skipped_objects). Default off: heap is "
                       "a CRITICAL all-or-nothing clique because a crash mid-heap-"
                       "operation can corrupt heap metadata, and safe recovery "
                       "requires reinitialising the heap."),
        llvm::cl::cat(CrashAnalyzerCategory));
}

int main(int argc, const char** argv) {
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

    auto expectedParser = clang::tooling::CommonOptionsParser::create(
        argc, argv, CrashAnalyzerCategory);
    if (!expectedParser) {
        llvm::errs() << expectedParser.takeError();
        return 1;
    }
    clang::tooling::CommonOptionsParser& parser = expectedParser.get();

    std::vector<std::string> sourceFiles = parser.getSourcePathList();

    for (const auto& extra : ExtraSources) {
        sourceFiles.push_back(extra);
    }

    if (UseCompdb) {
        auto allCommands = parser.getCompilations().getAllCompileCommands();
        std::set<std::string> existingFiles(sourceFiles.begin(), sourceFiles.end());
        int added = 0;
        int skipped = 0;
        for (const auto& cmd : allCommands) {
            std::string filename = cmd.Filename;
            if (existingFiles.count(filename) == 0) {
                if (filename.size() > 2 && filename.substr(filename.size() - 2) == ".S") {
                    skipped++;
                    continue;
                }
                sourceFiles.push_back(filename);
                existingFiles.insert(filename);
                added++;
            }
        }
        llvm::errs() << "Added " << added << " source file(s) from compilation database";
        if (skipped > 0) {
            llvm::errs() << " (" << skipped << " assembly file(s) skipped)";
        }
        llvm::errs() << "\n";
    }

    if (sourceFiles.empty()) {
        llvm::errs() << "Error: No source files specified\n";
        return 1;
    }

    llvm::errs() << "Analyzing " << sourceFiles.size() << " source file(s):\n";
    for (const auto& sf : sourceFiles) {
        llvm::errs() << "  " << sf << "\n";
    }

    g_threadEntryPoints.clear();
    g_objects.clear();
    g_objectsFromMacros.clear();
    g_structFieldObjects.clear();
    g_structsWithIpcFields.clear();
    g_ipcCalls.clear();
    g_threadToThreadCalls.clear();
    g_tidToThread.clear();
    g_callGraph.clear();
    g_functionsWithDefinitions.clear();
    g_helperToThreads.clear();
    g_reachableFromThreads.clear();
    g_globalVarAccesses.clear();
    g_globalVarNames.clear();
    g_threadInfos.clear();
    g_unresolvedCalls.clear();
    g_callbackTargets.clear();
    g_sysInitFunctions.clear();
    g_workQueueThreads.clear();
    g_workQueueCallbacks.clear();
    g_isrFunctions.clear();
    g_inputCallbacks.clear();
    g_callbackFunctions.clear();
    g_callbackRegistrations.clear();
    g_shellCommands.clear();
    g_initThreads.clear();
    g_astContext = nullptr;

    auto argAdjuster = [](const std::vector<std::string>& args, llvm::StringRef) {
        static const std::set<std::string> dropFlags = {
            "-mfp16-format=ieee",
            "-mno-fp-ret-in-rah",
            "-fno-reorder-functions",
            "-fno-defer-pop",
            "-fno-freestanding",
            "-fno-printf-return-value",
            "-fstrict-volatile-bitfields",
            "--param=min-pagesize=0",
            "-specs=picolibc.specs",
            "--specs=picolibc.specs",
            "-mlongcalls",
            "-Wlogical-op",
            "-Wformat-overflow=2",
            
        };
        std::vector<std::string> result;
        result.reserve(args.size());
        for (size_t i = 0; i < args.size(); ++i) {
            bool drop = false;
            for (const auto& flag : dropFlags) {
                if (args[i] == flag) { drop = true; break; }
            }
            if (!drop) result.push_back(args[i]);
        }
        return result;
    };

    clang::tooling::ClangTool tool(parser.getCompilations(), sourceFiles);
    tool.appendArgumentsAdjuster(argAdjuster);

    auto factory1 = createPass1ActionFactory();
    int result1 = tool.run(factory1.get());
    if (result1 != 0) {
        llvm::errs() << "Warning: thread discovery pass had errors (some files may have failed to parse)\n";
    }

    if (g_threadEntryPoints.empty()) {
        llvm::errs() << "No threads discovered\n";
        return 1;
    }

    llvm::errs() << "Discovered " << g_threadEntryPoints.size() << " thread(s)\n";
    for (const auto& name : g_threadEntryPoints) {
        llvm::errs() << "  " << name;
        if (g_threadInfos.count(name) > 0) {
            llvm::errs() << " (priority " << g_threadInfos[name].priority << ")";
        }
        llvm::errs() << "\n";
    }

    if (!g_sysInitFunctions.empty()) {
        llvm::errs() << "Discovered " << g_sysInitFunctions.size() << " SYS_INIT function(s)\n";
        for (const auto& fn : g_sysInitFunctions) {
            llvm::errs() << "  " << fn << "\n";
        }
    }

    // Add a system work queue thread if any k_work_submit (without queue) was seen
    if (g_workQueueCallbacks.count("_system_work_q") > 0) {
        std::string systemWqEntry = "work_queue_main";
        g_threadEntryPoints.insert(systemWqEntry);
        ThreadInfo wqInfo;
        wqInfo.name = "system_work_q";
        wqInfo.entryFn = systemWqEntry;
        wqInfo.isWorkqueue = true;
        g_threadInfos[systemWqEntry] = wqInfo;
        llvm::errs() << "Added system work queue thread: " << systemWqEntry << "\n";
    }

    if (!g_workQueueThreads.empty()) {
        llvm::errs() << "Discovered " << g_workQueueThreads.size() << " work queue thread(s)\n";
        for (const auto& [queueName, threadEntry] : g_workQueueThreads) {
            llvm::errs() << "  " << queueName << " -> " << threadEntry << "\n";
        }
    }

    if (!g_isrFunctions.empty()) {
        llvm::errs() << "Discovered " << g_isrFunctions.size() << " ISR function(s)\n";
    }

    if (!g_inputCallbacks.empty()) {
        llvm::errs() << "Discovered " << g_inputCallbacks.size() << " input callback(s)\n";
    }

    if (!g_shellCommands.empty()) {
        llvm::errs() << "Discovered " << g_shellCommands.size() << " shell command handler(s)\n";
    }

    // Classify threads and objects as user vs system
    if (!AppSourcePrefix.empty()) {
        int userThreads = 0, systemThreads = 0;
        for (auto& [name, info] : g_threadInfos) {
            info.isSystem = info.sourceFile.find(AppSourcePrefix) != 0;
            if (info.isSystem) systemThreads++; else userThreads++;
        }
        for (auto& obj : g_objects) {
            obj.isSystem = obj.sourceFile.find(AppSourcePrefix) != 0;
        }
        int systemObjects = std::count_if(g_objects.begin(), g_objects.end(),
            [](const ObjectInfo& o) { return o.isSystem; });
        llvm::errs() << "Classification: " << userThreads << " user thread(s), "
                     << systemThreads << " system thread(s), "
                     << g_objects.size() - systemObjects << " user object(s), "
                     << systemObjects << " system object(s)\n";
    }

    g_callGraph.clear();
    g_functionsWithDefinitions.clear();

    clang::tooling::ClangTool tool2(parser.getCompilations(), sourceFiles);
    tool2.appendArgumentsAdjuster(argAdjuster);
    auto factory2 = createPass2ActionFactory();
    int result2 = tool2.run(factory2.get());
    if (result2 != 0) {
        llvm::errs() << "Warning: call graph pass had errors (some files may have failed to parse)\n";
    }

    llvm::errs() << "Call graph: " << g_callGraph.size() << " functions with calls\n";
    llvm::errs() << "Function definitions: " << g_functionsWithDefinitions.size() << "\n";

    if (!g_callbackFunctions.empty()) {
        llvm::errs() << "Discovered " << g_callbackFunctions.size() << " callback function(s)\n";
        llvm::errs() << "ISR-registered callbacks: " << g_callbackRegistrations.size() << "\n";
    }

    computeReachableFunctions();
    computeInitThreads();

    int totalCount = g_reachableFromThreads.size() - g_threadEntryPoints.size();
    int filteredCount = 0;
    for (const auto& fn : g_reachableFromThreads) {
        if (g_threadEntryPoints.count(fn) == 0 && !isZephyrKernelInternal(fn)) {
            filteredCount++;
        }
    }
    int suppressedCount = totalCount - filteredCount;

    llvm::errs() << "Reachable helper functions: " << totalCount;
    if (suppressedCount > 0) {
        llvm::errs() << " (" << filteredCount << " shown, " << suppressedCount << " kernel internals suppressed)";
    }
    llvm::errs() << "\n";
    for (const auto& fn : g_reachableFromThreads) {
        if (g_threadEntryPoints.count(fn) == 0 && !isZephyrKernelInternal(fn)) {
            auto it = g_helperToThreads.find(fn);
            if (it != g_helperToThreads.end()) {
                llvm::errs() << "  " << fn << " <- ";
                for (const auto& t : it->second) {
                    llvm::errs() << t << " ";
                }
                llvm::errs() << "\n";
            }
        }
    }
llvm::errs() << "\n";

    if (!g_unresolvedCalls.empty()) {
        llvm::errs() << "Unresolved indirect calls:\n";
        for (const auto& [fn, calls] : g_unresolvedCalls) {
            llvm::errs() << "  " << fn << ": ";
            for (const auto& call : calls) {
                llvm::errs() << call << " ";
            }
            llvm::errs() << "\n";
        }
        llvm::errs() << "\n";
    }

    clang::tooling::ClangTool tool3(parser.getCompilations(), sourceFiles);
    tool3.appendArgumentsAdjuster(argAdjuster);
    auto factory3 = createPass3ActionFactory();
    int result3 = tool3.run(factory3.get());
    if (result3 != 0) {
        llvm::errs() << "Warning: interaction discovery pass had errors (some files may have failed to parse)\n";
    }

    llvm::errs() << "IPC calls: " << g_ipcCalls.size() << "\n";
    llvm::errs() << "Thread-to-thread calls: " << g_threadToThreadCalls.size() << "\n";
    llvm::errs() << "Global variable accesses: " << g_globalVarNames.size() << " variables across " << g_globalVarAccesses.size() << " variable-thread pairs\n";

    AnalysisResult analysisResult;

    std::set<std::string> extraNonRestartable(NonRestartableThreads.begin(),
                                              NonRestartableThreads.end());

    for (const auto& name : g_threadEntryPoints) {
        Thread t;
        if (g_threadInfos.count(name) > 0) {
            t.name = g_threadInfos[name].name.empty() ? name : g_threadInfos[name].name;
            t.entryFn = g_threadInfos[name].entryFn.empty() ? name : g_threadInfos[name].entryFn;
            t.location = g_threadInfos[name].location;
            t.priority = g_threadInfos[name].priority;
            t.sourceFile = g_threadInfos[name].sourceFile;
            t.isSystem = g_threadInfos[name].isSystem;
        } else {
            t.name = name;
            t.entryFn = name;
        }
        // Recommendation 2: classify individually-restartable vs not.
        // isr/idle/bg_thread_main and native_sim host threads are cut
        // from restart-set output but kept as propagation nodes.
        t.isRestartable = !isNonRestartableThread(t.name, t.entryFn, t.sourceFile) &&
                          extraNonRestartable.count(t.name) == 0;
        analysisResult.threads.push_back(t);
    }

    std::map<std::string, ObjectInfo> uniqueObjects;
    for (const auto& obj : g_objects) {
        if (uniqueObjects.count(obj.name) == 0) {
            uniqueObjects[obj.name] = obj;
        }
    }
    for (const auto& pair : uniqueObjects) {
        const std::string& name = pair.first;
        const std::string& typeStr = pair.second.typeStr;
        const SourceLocation& loc = pair.second.location;

        IPCObject obj;
        obj.name = name;
        obj.scope = "global";
        obj.location = loc;
        obj.sourceFile = pair.second.sourceFile;
        obj.isSystem = pair.second.isSystem;

        ObjectType type = IPCObject::parseObjectType(typeStr);
        if (type == ObjectType::Unknown) {
            if (name.find("mutex") != std::string::npos ||
                name.find("lock") != std::string::npos) {
                type = ObjectType::Mutex;
            } else if (name.find("sem") != std::string::npos) {
                type = ObjectType::Semaphore;
            } else if (name.find("fifo") != std::string::npos) {
                type = ObjectType::Fifo;
            } else if (name.find("msgq") != std::string::npos || name.find("msg_q") != std::string::npos) {
                type = ObjectType::Msgq;
            } else if (name.find("event") != std::string::npos) {
                type = ObjectType::Event;
            } else if (name.find("condvar") != std::string::npos) {
                type = ObjectType::Condvar;
            } else if (name.find("stack") != std::string::npos) {
                type = ObjectType::Stack;
            } else if (name.find("timer") != std::string::npos) {
                type = ObjectType::Timer;
            } else if (name.find("work") != std::string::npos) {
                type = ObjectType::Work;
            } else if (name.find("pipe") != std::string::npos) {
                type = ObjectType::Pipe;
            } else if (name.find("heap") != std::string::npos) {
                type = ObjectType::Heap;
            } else if (name.find("mem_slab") != std::string::npos) {
                type = ObjectType::MemSlab;
            } else if (name.find("lifo") != std::string::npos) {
                type = ObjectType::Lifo;
            } else if (name.find("queue") != std::string::npos) {
                type = ObjectType::Queue;
            } else if (name.find("mbox") != std::string::npos) {
                type = ObjectType::Mbox;
            } else if (name.find("poll_signal") != std::string::npos) {
                type = ObjectType::PollSignal;
            }
        }
        obj.type = type;
        analysisResult.objects.push_back(obj);
    }

    bool hasSystemHeap = false;
    for (const auto& call : g_ipcCalls) {
        if (call.objectName == "[system_heap]") {
            hasSystemHeap = true;
            break;
        }
    }
    if (hasSystemHeap) {
        IPCObject sysHeap;
        sysHeap.name = "[system_heap]";
        sysHeap.scope = "system";
        sysHeap.type = ObjectType::Heap;
        sysHeap.isSystem = true;
        analysisResult.objects.push_back(sysHeap);
    }

    analysisResult.buildMaps();

    for (const auto& call : g_ipcCalls) {
        if (!analysisResult.findObject(call.objectName)) continue;
        Interaction inter;
        inter.threadName = call.threadName;
        inter.objectName = call.objectName;
        inter.action = call.action;
        inter.resolved = true;
        inter.location = call.location;
        inter.canBlock = call.canBlock;
        analysisResult.interactions.push_back(inter);
    }

    for (const auto& ttc : g_threadToThreadCalls) {
        Interaction inter;
        inter.threadName = ttc.fromThread;
        inter.objectName = ttc.targetThread;
        inter.action = (ttc.edgeType == EdgeType::Join) ? ActionType::Join :
                       (ttc.edgeType == EdgeType::Abort) ? ActionType::Abort : ActionType::Wakeup;
        inter.resolved = true;
        inter.location = ttc.location;
        inter.canBlock = ttc.canBlock;
        analysisResult.interactions.push_back(inter);
    }

    KernelSymbolFilter kernelFilter;
    if (FilterKernelSymbols) {
        kernelFilter = KernelSymbolFilter::defaultFilter();
        llvm::errs() << "Kernel symbol filtering enabled (excluding kernel internals";
        if (kernelFilter.excludeOmnipresent) {
            llvm::errs() << ", excluding omnipresent objects (>=" << kernelFilter.omnipresentThreshold << "% threads, min " << kernelFilter.omnipresentMinThreads << " threads)";
        }
        llvm::errs() << ")\n";
    }

    GraphBuilder graphBuilder(analysisResult, kernelFilter, AllowLeaks);
    graphBuilder.buildGraph();
    graphBuilder.addGlobalVarEdges(g_globalVarAccesses);

    for (const auto& ttc : g_threadToThreadCalls) {
        Interaction evidence;
        evidence.threadName = ttc.fromThread;
        evidence.objectName = ttc.targetThread;
        evidence.action = (ttc.edgeType == EdgeType::Join) ? ActionType::Join :
                         (ttc.edgeType == EdgeType::Abort) ? ActionType::Abort : ActionType::Wakeup;
        evidence.location = ttc.location;
        evidence.canBlock = ttc.canBlock;
        evidence.resolved = true;
        graphBuilder.addEdge(ttc.fromThread, ttc.targetThread, ttc.targetThread,
                            ttc.edgeType, {evidence});
    }

    if (FilterKernelSymbols) {
        graphBuilder.filterOmnipresentGroups(analysisResult.threads.size());
        if (analysisResult.omnipresentFilteredCount > 0) {
            llvm::errs() << "Detected " << analysisResult.omnipresentFilteredCount
                         << " highly connected objects (flagged in output, not filtered)\n";
        }
    }

    const auto& edges = graphBuilder.getEdges();
    const auto& contentionGroups = graphBuilder.getContentionGroups();

    analysisResult.contentionGroups = contentionGroups;

    ReportGenerator reportGenerator(analysisResult, edges, Scope);

    std::string output;
    CrashImpact* impact = nullptr;
    CrashImpact computedImpact;
    std::vector<CrashImpact> allImpacts;

    // Build ImpactConfig from CLI flags.
    ImpactConfig impactConfig;
    impactConfig.includeOmnipresent = (bool)ImpactOmnipresent;
    impactConfig.mandatoryHopDepth = (int)ImpactHopDepth;
    impactConfig.advisorySeverityThreshold = stringToSeverity(ImpactAdvisorySeverity);
    impactConfig.allowLeaks = (bool)AllowLeaks;

    if (!ImpactOf.empty()) {
        ImpactAnalyzer impactAnalyzer(analysisResult, edges, impactConfig);
        computedImpact = impactAnalyzer.computeCrashImpact(ImpactOf);
        impact = &computedImpact;
    } else if (ImpactAll && OutputFormat == "json") {
        ImpactAnalyzer impactAnalyzer(analysisResult, edges, impactConfig);
        for (const auto& t : analysisResult.threads) {
            allImpacts.push_back(impactAnalyzer.computeCrashImpact(t.name));
        }
    }

    if (OutputFormat == "json") {
        if (!allImpacts.empty()) {
            output = reportGenerator.generateJson(nullptr, &allImpacts);
        } else {
            output = reportGenerator.generateJson(impact);
        }
    } else if (OutputFormat == "text") {
        output = reportGenerator.generateText(impact);
    } else if (OutputFormat == "dot") {
        output = reportGenerator.generateDot(impact);
    } else {
        llvm::errs() << "Error: Unknown output format '" << OutputFormat << "'\n";
        return 1;
    }

    if (OutputFile == "-") {
        llvm::outs() << output;
    } else {
        std::ofstream outFile(OutputFile);
        if (!outFile.is_open()) {
            llvm::errs() << "Error: Cannot open output file '" << OutputFile << "'\n";
            return 1;
        }
        outFile << output;
        outFile.close();
    }

    return 0;
}
