/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

enum class ObjectType {
    Mutex,
    Semaphore,
    Fifo,
    Lifo,
    Queue,
    Msgq,
    Mbox,
    Pipe,
    Stack,
    Event,
    Condvar,
    Timer,
    Heap,
    MemSlab,
    PollSignal,
    Work,
    GlobalVar,
    Unknown
};

enum class ActionType {
    Lock,
    Unlock,
    Take,
    Give,
    Put,
    Get,
    Push,
    Pop,
    Send,
    Receive,
    Post,
    Wait,
    Signal,
    Broadcast,
    Alloc,
    Free,
    Start,
    Stop,
    Raise,
    Reset,
    Read,
    Write,
    Join,
    Abort,
    Wakeup,
    Submit,
    Cancel,
    Unknown
};

enum class EdgeType {
    MutualExclusion,
    Signals,
    ProducesFor,
    ConsumesFrom,
    Bidirectional,
    SharedMemory,
    // Advisory-only edge for compile-time / SYS_INIT-populated registration
    // arrays and callback tables (e.g. SHELL_CMD_REGISTER, MODULE_DECLARE,
    // bt_hci driver cb). Excluded from ImpactAnalyzer::shouldPropagate so a
    // "thread T writes the registration table at boot" pattern does not pull
    // every reader of the table into T's crash-impact set.
    InitTable,
    Join,
    Abort,
    Wakeup,
    Unknown
};

enum class Severity {
    Critical,
    High,
    Medium,
    Low,
    None
};

struct SourceLocation {
    std::string file;
    int line;
    int column;

    SourceLocation() : line(0), column(0) {}
    SourceLocation(const std::string& f, int l, int c = 0) : file(f), line(l), column(c) {}

    std::string toString() const {
        if (line == 0) return file;
        return file + ":" + std::to_string(line) +
               (column > 0 ? ":" + std::to_string(column) : "");
    }
};

struct Thread {
    std::string name;
    std::string entryFn;
    SourceLocation location;
    int priority;
    bool isWorkqueue;
    bool isTimerCb;
    bool isMainThread;
    std::string sourceFile;
    bool isSystem;
    // Whether the thread can be individually restarted as a recovery
    // action (see isNonRestartableThread). Non-restartable threads
    // (ISR pseudo-thread, idle, bg_thread_main, native_sim host
    // threads) are kept as propagation nodes in the coupling graph but
    // excluded from restart_set output.
    bool isRestartable;

    Thread() : priority(0), isWorkqueue(false), isTimerCb(false), isMainThread(false),
               isSystem(false), isRestartable(true) {}

    std::string toString() const {
        return name + " (entry: " + entryFn + ")";
    }
};

struct IPCObject {
    std::string name;
    ObjectType type;
    std::string scope;
    SourceLocation location;
    std::string embeddedIn;
    std::string addressExpr;
    std::string sourceFile;
    bool isSystem;

    IPCObject() : type(ObjectType::Unknown), embeddedIn(""), isSystem(false) {}

    std::string typeString() const;
    static ObjectType parseObjectType(const std::string& typeStr);
    static std::string objectTypeToString(ObjectType t);
};

struct Interaction {
    std::string threadName;
    std::string objectName;
    ActionType action;
    bool canBlock;
    SourceLocation location;
    bool resolved;

    Interaction() : action(ActionType::Unknown), canBlock(false), resolved(false) {}

    std::string actionString() const;
    static ActionType parseAction(const std::string& actionStr);
};

struct ThreadEdge {
    std::string fromThread;
    std::string toThread;
    std::string objectName;
    EdgeType edgeType;
    Severity severity;
    std::vector<Interaction> evidence;
    bool highlyConnected;

    ThreadEdge() : edgeType(EdgeType::Unknown), severity(Severity::None), highlyConnected(false) {}

    static EdgeType inferEdgeType(const IPCObject& obj, const std::vector<Interaction>& interactions);
    static EdgeType inferDirectionalEdgeType(const IPCObject& obj, const std::vector<Interaction>& fromThreadInteractions);
    static Severity inferSeverity(EdgeType edgeType);
};

struct GlobalVarAccess {
    std::string varName;
    std::string threadName;
    std::string accessType;
    bool isAtomic;
    bool isVolatile;
    bool isConst;
    // Whether the variable's underlying type (after typedef/elaboration
    // peeling) is a primitive scalar: bool/char/short/int/long/float/
    // double + their unsigned variants + size_t/uintptr_t/intptr_t/
    // fixed-width aliases (uint8_t..uint64_t..int64_t). Set by
    // Visitors.cpp when recording the access; consulted by
    // GraphBuilder::addGlobalVarEdges to downgrade the cross-thread
    // edge severity for variables whose writes are atomic on every
    // supported Zephyr target (single bus transaction for the natural
    // word width, no tear window). This avoids spurious coupling
    // through monotonic counters, error-flags, and last-error codes
    // (csp_dbg_*, log counters, errno-style globals) that every
    // thread writes but no thread reads operationally. Downgrade
    // preserves propagation (so the edge is still walked and shows
    // up in `filter_skipped_objects` / advisory tier) but lowers the
    // severity so the edge can't pull the affected thread into the
    // mandatory restart set on its own.
    bool isPrimitiveScalar;
    SourceLocation location;

    GlobalVarAccess() : accessType("readwrite"), isAtomic(false), isVolatile(false), isConst(false), isPrimitiveScalar(false) {}
};

struct UnresolvedInteraction {
    std::string threadName;
    std::string call;
    SourceLocation location;
    std::string reason;
};

struct ContentionGroup {
    std::string objectName;
    ObjectType objectType;
    EdgeType groupType;
    Severity severity;
    std::vector<std::string> threads;
    std::vector<Interaction> evidence;
    bool highlyConnected;

    ContentionGroup() : objectType(ObjectType::Unknown), groupType(EdgeType::Unknown), severity(Severity::None), highlyConnected(false) {}
};

struct AnalysisResult {
    std::vector<Thread> threads;
    std::vector<IPCObject> objects;
    std::vector<Interaction> interactions;
    std::vector<ThreadEdge> edges;
    std::vector<ContentionGroup> contentionGroups;
    std::vector<GlobalVarAccess> globalVarAccesses;
    std::vector<UnresolvedInteraction> unresolved;
    int omnipresentFilteredCount;

    std::map<std::string, Thread> threadMap;
    std::map<std::string, IPCObject> objectMap;

    void buildMaps();
    Thread* findThread(const std::string& name);
    IPCObject* findObject(const std::string& name);

    AnalysisResult() : omnipresentFilteredCount(0) {}
};

struct ImpactEntry {
    std::string reason;
    Severity severity;
    std::vector<std::string> path;
};

// Restart-set classification for a single affected thread. `Mandatory`
// indicates the thread must be restarted alongside the crashed thread
// (it is in the crashed thread's SCC, or it is connected by a first-order
// CRITICAL/HIGH edge that can block). `Advisory` means the thread is
// impacted but restarting it is not strictly required to guarantee no
// data corruption will persist (e.g. 2nd-order shared-memory coupling).
// `FilterSkipped` indicates the thread was reachable only through
// omnipresent / init-table edges that the B min-cut explicitly excluded
// from impact propagation. `NonRestartable` is used only for the crashed
// thread itself when it cannot be individually restarted (ISR pseudo-
// thread, idle, bg_thread_main, native_sim host threads): recovery then
// requires a system-level action (reboot), and the remaining restart_set
// members list what must restart regardless. Non-restartable *other*
// threads are excluded from the restart set outright (they stay in
// affected_threads and keep propagating through the graph).
enum class RestartClass {
    Mandatory,
    Advisory,
    FilterSkipped,
    NonRestartable
};

struct RestartEntry {
    std::string thread;
    RestartClass restartClass;
    std::string reason;
    Severity severity;
    std::vector<std::string> path;
};

struct CrashImpact {
    std::string crashedThread;
    std::map<std::string, ImpactEntry> affectedThreads;
    // B: minimum restartable set classification. The mandatory set is
    // the crashed thread's SCC plus direct critical/high-blocking-edge
    // successors; the advisory set is everything else reachable through
    // the (optionally filtered) propagation graph.
    std::vector<RestartEntry> restartSet;
    // List of object names (edges / content groups) skipped during the
    // propagation because they were tagged highlyConnected (when the
    // user opted in to --impact-omnipresent=false).
    std::vector<std::string> filterSkippedObjects;
};

std::string severityToString(Severity s);
Severity stringToSeverity(const std::string& s);
int severityRank(Severity s);
Severity severityFromRank(int rank);
std::string restartClassToString(RestartClass rc);

/** Classify a discovered thread as individually restartable or not.
 *
 *  Non-restartable entities (analysis-report.md, recommendation 2):
 *    - `isr` — the synthetic ISR pseudo-thread. Not a schedulable thread
 *      at all; there is nothing to restart.
 *    - `idle` / `idle_NN` — the per-CPU kernel idle threads. Essential
 *      kernel threads, not restartable from application recovery code.
 *    - `bg_thread_main` — Zephyr's static background thread that runs
 *      main() once; "restarting" it is equivalent to a reboot. It is
 *      still kept as a propagation node so apps whose runtime loop
 *      lives in main() (grbl, spinner, quadcopter) keep their real
 *      coupling paths.
 *    - native_sim / native_posix host threads — threads defined by the
 *      emulator's host-side drivers (SDL display/events, native UART
 *      PTY, native TAP ethernet, DMIC emulator). They exist only in
 *      emulated builds and model host I/O, not a restartable unit of
 *      the analyzed system.
 *
 *  Classification is by thread name / entry-function name for the
 *  kernel entities (Zephyr naming convention) and by source-file path
 *  patterns for the native_sim host drivers. Note that a name collision
 *  with an application-defined entry function (e.g. an app entry
 *  function literally called `idle`) would also be classified
 *  non-restartable; the `--non-restartable` / restartable flag is
 *  emitted per thread in the JSON report so such misclassification is
 *  visible, and callers can extend the list via CLI.
 */
bool isNonRestartableThread(const std::string& name,
                            const std::string& entryFn,
                            const std::string& sourceFile);

enum class KernelSymbolAction {
    Include,
    Exclude,
    Downgrade
};

struct KernelSymbolFilter {
    std::map<std::string, KernelSymbolAction> rules;
    bool excludeOmnipresent;
    int omnipresentThreshold;
    int omnipresentMinThreads;
    std::vector<std::string> excludePrefixes;

    KernelSymbolFilter() : excludeOmnipresent(false), omnipresentThreshold(100), omnipresentMinThreads(6) {}

    KernelSymbolAction getAction(const std::string& symbolName) const {
        auto it = rules.find(symbolName);
        if (it != rules.end()) return it->second;
        return KernelSymbolAction::Include;
    }

    bool matchesPattern(const std::string& symbolName) const {
        for (const auto& prefix : excludePrefixes) {
            if (symbolName.compare(0, prefix.size(), prefix) == 0) {
                return true;
            }
        }
        return false;
    }

    bool isExcluded(const std::string& symbolName) const {
        if (getAction(symbolName) == KernelSymbolAction::Exclude) return true;
        return matchesPattern(symbolName);
    }

    // Whether a source path belongs to the Zephyr / NCS SDK rather than
    // the application under analysis. Used to make the prefix blocklist
    // "app-safe": the blocklist exists to suppress subsystem primitives,
    // so it must never drop a symbol that originates from application
    // code. Detection is by well-known SDK path components (zephyr,
    // zephyr_fork, modules, nrfxlib, packages, hal, libc) — matched as
    // whole path components, not substrings, so e.g. a "zephyr_fork/"
    // checkout is recognized while an app file merely containing the
    // string is not. The per-project --app-source-prefix cannot be
    // trusted here: several corpus projects set it to the repo root,
    // which misclassifies the entire SDK as "app", and ecfw keeps its
    // Zephyr fork (zephyr_fork/) inside the app prefix. If in doubt the
    // answer is "SDK", which keeps the historical blocklist behavior
    // (exclusion); only a positively app-origin symbol is spared.
    static bool isSdkSource(const std::string& sourceFile) {
        size_t start = 0;
        while (start <= sourceFile.size()) {
            size_t end = sourceFile.find('/', start);
            if (end == std::string::npos) end = sourceFile.size();
            if (end > start) {
                std::string comp = sourceFile.substr(start, end - start);
                if (comp.rfind("zephyr", 0) == 0) return true; // zephyr, zephyr_fork, ...
                if (comp == "modules" || comp == "nrfxlib" ||
                    comp == "packages" || comp == "hal" || comp == "libc") {
                    return true;
                }
            }
            start = end + 1;
        }
        return false;
    }

    bool isDowngraded(const std::string& symbolName) const {
        return getAction(symbolName) == KernelSymbolAction::Downgrade;
    }

    static KernelSymbolFilter defaultFilter() {
        KernelSymbolFilter f;
        f.excludeOmnipresent = true;
        // Re-calibrated per corpus-wide analysis: at 80%/min6 the
        // statistical check never fires (every candidate above 80% is
        // already removed by excludePrefixes first), leaving
        // highly_connected permanently false. Lowering to 60% and
        // requiring >=10 threads exempts small projects where a single
        // genuine app-global legitimately touches half the threads, and
        // catches the surviving corpus-wide primitives (TLS thread-local
        // storage, HAL SoC clock, callback-registry state) that couple
        // >60% of threads without representing app-level data
        // dependency. Conservative — flags 5 object/project pairs
        // across the 20-project corpus, all clearly subsystem primitives.
        f.omnipresentThreshold = 60;
        f.omnipresentMinThreads = 10;
        f.excludePrefixes = {
            "__log_",
            "__rodata_region_",
            "__device_dts_ord_",
            "_kernel",
            "_log_",
            "_sched_",
            "_thread_dummy",
            "_zbus_",
            "z_thread_monitor_",
            "_char_out",
            "nsi_",
            "_net_buf_pool_list_",
            "announce_remaining",
            "announced_cycles",
            "announcing_cpu",
            "anchor",
            "backend_attached",
            "buffered_cnt",
            "cached_icr",
            "cc_data",
            "curr_tick",
            "curr_log_buffer",
            "current",
            "currently_running_irq",
            "cycle_count",
            "dname_cache",
            "dname_cache_buffer",
            "dname_cache_config",
            "dropped_cnt",
            "force_isr_mask",
            "fs_mnt_list",
            "initialized",
            "int_mask",
            "irq_vector_table",
            "k_sys_work_q",
            "last_announcement",
            "last_count",
            "last_failure_report",
            "last_load",
            "last_tick_time",
            "lock",
            "log_buffer",
            "log_process_thread_sem",
            "log_process_thread_timer",
            "may_swap",
            "nce_st",
            "overflow_cyc",
            "overflow_cnt",
            "panic_mode",
            "pending_cancels",
            "pending_current",
            "prev_timestamp",
            "proc_tid",
            "proc_latency",
            "process_lock",
            "sched_spinlock",
            "slice_expired",
            "slice_max_prio",
            "slice_ticks",
            "slice_timeouts",
            "sname_cache",
            "sname_cache_buffer",
            "sname_cache_config",
            "sys_busy",
            "te_state",
            "tick_period",
            "timeout_list",
            "timeout_lock",
            "timestamp_freq",
            "timestamp_func",
            "timestamps",
            "total_cycles",
            "unordered_cnt",
            "usage_lock",
            "z_idle_threads",
            "z_sys_post_kernel",
            "conn_lock",
            "net_mgmt_callback_lock",
            "nbr_lock",
            "net_mgmt_event_lock",
            // Zephyr-internal omnipresent objects identified by corpus-wide
            // analysis of filter-skipped objects appearing across multiple
            // projects. These are subsystem primitives (logging, SoC init,
            // BLE, USB, memfault, settings tests, native_sim) that couple
            // every thread to every other thread without representing
            // genuine application-level data dependency.
            "SystemCoreClock",
            "global_event_mask",
            "conns",
            "settings_save_dst",
            "timer",
            "last_static_handle",
            "s_crash_reason",
            "s_mflt_reboot_info",
            "s_reboot_reason_data",
            "s_memfault_trace_event_ctx",
            "espi_dev",
            "plat_data",
            "platformskutype",
            "udc_work_q",
            "pint_irq_cfg",
            "hw_wdt_dev",
            "hw_wdt_channel",
            "g_native_symbols_list",
            "g_exit_cb",
            "g_initialized",
            "runtime_running_mode",
            "ll_adv",
            "le_event_mask",
            "event_mask",
            "event_mask_page_2",
            "memory_mode",
            "pool_allocator",
            "malloc_func",
            "free_func",
            "realloc_func",
            "fs_state",
            "externref_map",
            "g_app_stacks",
            // Zephyr-internal omnipresent objects with embedded-field names
            // (C-fix synthetic :: names that can't be matched by simple prefix
            // on the short name — the prefix matches the struct type part).
            "mpsc_pbuf_buffer",
            // NCS library omnipresent objects (Nordic SDK, not Zephyr kernel
            // and not application code). These couple every thread that uses
            // the NCS date_time or location subsystem but don't represent
            // application-level crash coupling.
            "app_evt_handler",
            "location_core_work_q",
            // Zephyr thread-local-storage pointer for the current thread
            // context. Touched by every thread that calls any TLS-using
            // primitive (logging, network crypto, mbed TLS) across 5/20
            // corpus projects at 50-67% thread coverage. A thread writing
            // its own TLS pointer does not corrupt any other thread's
            // TLS, so this is omnipresent coupling, not data dependency.
            "z_tls_current",
        };
        return f;
    }
};

#endif
