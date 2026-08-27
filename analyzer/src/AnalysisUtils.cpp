/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Globals.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"

const clang::Expr* stripCasts(const clang::Expr* e) {
    if (!e) return nullptr;
    while (true) {
        e = e->IgnoreParens();
        if (!e) return nullptr;
        auto* cast = llvm::dyn_cast<clang::CastExpr>(e);
        if (!cast) break;
        e = cast->getSubExpr();
    }
    return e;
}

const clang::RecordDecl* baseTypeAsRecordDecl(const clang::QualType& qt) {
    if (qt.isNull()) return nullptr;
    const clang::Type* t = qt.getTypePtrOrNull();
    if (!t) return nullptr;
    // Peel array types (rare for the base of a MemberExpr, but cheap to handle).
    t = t->getArrayElementTypeNoTypeQual();
    if (!t) t = qt.getTypePtrOrNull();
    // Peel pointers and references.
    if (auto* pt = llvm::dyn_cast<clang::PointerType>(t)) {
        t = pt->getPointeeType().getTypePtrOrNull();
        if (!t) return nullptr;
    } else if (auto* rt = llvm::dyn_cast<clang::ReferenceType>(t)) {
        t = rt->getPointeeType().getTypePtrOrNull();
        if (!t) return nullptr;
    }
    // Strip ElaboratedType (e.g. `struct foo` written explicitly) and TypedefType.
    t = t->getUnqualifiedDesugaredType();
    if (auto* recTy = llvm::dyn_cast<clang::RecordType>(t)) {
        return recTy->getDecl();
    }
    return nullptr;
}

std::string makeEmbeddedObjectName(const clang::Type* baseType, const std::string& fieldName) {
    if (!baseType) return "";
    const clang::Type* t = baseType->getUnqualifiedDesugaredType();
    if (!t) return "";
    if (auto* pt = llvm::dyn_cast<clang::PointerType>(t)) {
        t = pt->getPointeeType()->getUnqualifiedDesugaredType();
        if (!t) return "";
    } else if (auto* rt = llvm::dyn_cast<clang::ReferenceType>(t)) {
        t = rt->getPointeeType()->getUnqualifiedDesugaredType();
        if (!t) return "";
    }
    if (auto* recTy = llvm::dyn_cast<clang::RecordType>(t)) {
        const clang::RecordDecl* decl = recTy->getDecl();
        if (!decl) return "";
        std::string typeName = decl->getNameAsString();
        // Anonymous structs/unions produce empty names; skip them since their fields
        // are by definition accessible only via the parent record.
        if (typeName.empty()) return "";
        return typeName + "::" + fieldName;
    }
    return "";
}

std::set<std::string> g_threadEntryPoints;
std::map<std::string, ThreadInfo> g_threadInfos;
std::string g_currentFunction;
const clang::SourceManager* g_sourceManager = nullptr;
clang::ASTContext* g_astContext = nullptr;
std::vector<IpcCall> g_ipcCalls;
std::vector<ThreadToThreadCall> g_threadToThreadCalls;
std::vector<ObjectInfo> g_objects;
std::set<std::string> g_objectsFromMacros;
std::map<std::string, std::map<std::string, ObjectType>> g_structFieldObjects;
std::set<std::string> g_structsWithIpcFields;
std::map<std::string, std::map<std::string, GlobalVarAccess>> g_globalVarAccesses;
std::set<std::string> g_globalVarNames;
std::map<std::string, std::string> g_tidToThread;
std::map<std::string, std::set<std::string>> g_callGraph;
std::set<std::string> g_functionsWithDefinitions;
std::map<std::string, std::set<std::string>> g_helperToThreads;
std::set<std::string> g_reachableFromThreads;

std::map<std::string, std::set<std::string>> g_unresolvedCalls;

std::map<std::string, std::string> g_callbackTargets;

std::set<std::string> g_sysInitFunctions;

std::map<std::string, std::string> g_workQueueThreads;
std::map<std::string, std::set<std::string>> g_workQueueCallbacks;

std::set<std::string> g_isrFunctions;
std::set<std::string> g_inputCallbacks;
std::set<std::string> g_callbackFunctions;
std::map<std::string, std::set<std::string>> g_callbackRegistrations;

std::set<std::string> g_isrRegistrationFunctions = {
    "k_timer_init",
    "irq_connect_dynamic",
    "rtc_alarm_set_callback",
    "kscan_config",
    "sensor_trigger_set",
    "gpio_init_callback",
    "uart_callback_set",
    "dma_callback_set",
    "adc_callback_set",
    "i2s_trigger_set",
    "spi_release",
    "entropy_callback_set",
    "can_add_rx_filter",
    "ptp_clock_set",
    "net_pkt_cb_register",
    "usbd_endpoint_set_cb",
    "udc_ep_enqueue",
    "nrfx_saadc_init",
    "nrfx_rtc_init",
    "nrfx_gpiote_input_connect",
    "nrfx_timer_init",
    "nrfx_comp_init",
    "nrfx_lpcomp_init",
    "nrfx_qdec_init",
    "nrfx_twim_init",
    "nrfx_twis_init",
    "nrfx_uarte_init",
    "nrfx_spim_init",
    "nrfx_spis_init",
    "nrfx_clock_init",
    "nrfx_power_init",
    "nrfx_usbuninit",
};
std::set<std::string> g_nonIsrInlineFunctions = {
    "bt_gatt_foreach_attr",
    "bt_gatt_foreach_attr_type",
    "cbvprintf",
    "adc_raw_to_x_dt_chan",
    "bt_hci_open",
    "sys_slist_find_and_remove",
    "sys_slist_find",
};
std::set<std::string> g_shellCommands;

std::set<std::string> g_initThreads;

bool isInitThread(const std::string& threadName) {
    return g_initThreads.count(threadName) > 0;
}

bool isLinkerSectionSymbol(const std::string& varName) {
    // Zephyr linker iterables produce boundary symbols named
    // _<section>_list_start / _<section>_list_end (the common form, e.g.
    // _shell_root_cmds_list_start) and occasionally _<section>_start /
    // _<section>_end (e.g. __tdata_start). Match the _list_ variants and
    // the bare _start/_end variants of linker-emitted sections. We avoid
    // matching arbitrary `_foo_start` app globals by requiring either the
    // `_list_` infix or a leading double underscore (linker/runtime
    // convention for toolchain-emitted symbols).
    auto endsWith = [](const std::string& s, const std::string& suf) {
        return s.size() >= suf.size() &&
               s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
    };
    bool isListSym = endsWith(varName, "_list_start") ||
                     endsWith(varName, "_list_end");
    bool isBareLinkerSym = (endsWith(varName, "_start") || endsWith(varName, "_end")) &&
                           varName.size() > 6 &&
                           varName.rfind("__", 0) == 0;
    return isListSym || isBareLinkerSym;
}

// Populate g_initThreads. We consider any thread whose *sole* attributed
// function graph is the SYS_INIT chain to be an "init thread" (writes from
// such threads are presumed to occur only at boot). Today the SYS_INIT
// plumbing funnels everything through bg_thread_main, so the practical
// result is {"bg_thread_main", "main"}; the loop here also picks up any
// thread whose entry function name itself is registered as a SYS_INIT
// function (rare but legitimate).
void computeInitThreads() {
    g_initThreads.clear();
    g_initThreads.insert("bg_thread_main");
    g_initThreads.insert("main");
    g_initThreads.insert("init");
    // Work-queue-style init threads that hold the system work queue are
    // sometimes the attributed owner of SYS_INIT-callback-driven helpers.
    g_initThreads.insert("k_sys_work_q");
    g_initThreads.insert("work_queue_main");

    // Any thread whose entry function is registered as a SYS_INIT function
    // is by definition an init-context thread.
    for (const auto& fn : g_sysInitFunctions) {
        for (const auto& [tname, info] : g_threadInfos) {
            if (info.entryFn == fn || info.name == fn) {
                g_initThreads.insert(info.name);
            }
        }
    }
}

std::set<std::string> g_ipcFunctions = {
    "k_mutex_lock", "k_mutex_unlock",
    "k_sem_take", "k_sem_give", "k_sem_reset",
    "k_fifo_put", "k_fifo_get", "k_fifo_alloc_put",
    "k_lifo_put", "k_lifo_alloc_put", "k_lifo_get",
    "k_msgq_put", "k_msgq_get", "k_msgq_purge",
    "k_event_post", "k_event_wait",
    "k_condvar_signal", "k_condvar_wait", "k_condvar_broadcast",
    "k_timer_start", "k_timer_stop",
    "k_pipe_put", "k_pipe_get",
    "k_stack_push", "k_stack_pop",
    "k_queue_append", "k_queue_insert", "k_queue_prepend", "k_queue_get", "k_queue_remove",
    "k_mbox_put", "k_mbox_get", "k_mbox_async_put",
    "k_heap_alloc", "k_heap_free",
    "k_mem_slab_alloc", "k_mem_slab_free",
    "malloc", "calloc", "realloc", "free",
    "k_malloc", "k_free",
    "k_poll_signal_raise", "k_poll_signal_reset",
    "k_work_submit", "k_work_submit_to_queue", "k_work_cancel",
    "k_work_schedule", "k_work_schedule_for_queue",
    "k_work_reschedule", "k_work_reschedule_for_queue",
    "k_work_queue_start",
    "k_thread_join", "k_thread_abort", "k_wakeup"
};

SourceLocation convertSourceLocation(const clang::SourceLocation& loc) {
    if (!loc.isValid() || !g_sourceManager) {
        return SourceLocation();
    }
    clang::PresumedLoc presumedLoc = g_sourceManager->getPresumedLoc(loc);
    if (presumedLoc.isValid()) {
        const char* filename = presumedLoc.getFilename();
        if (filename && strlen(filename) > 0) {
            return SourceLocation(filename, presumedLoc.getLine(), presumedLoc.getColumn());
        }
    }
    clang::SourceLocation spellingLoc = g_sourceManager->getSpellingLoc(loc);
    unsigned line = g_sourceManager->getSpellingLineNumber(spellingLoc);
    unsigned col = g_sourceManager->getSpellingColumnNumber(spellingLoc);
    std::string filename = g_sourceManager->getFilename(spellingLoc).str();
    return SourceLocation(filename, line, col);
}

std::string qualifiedObjectVarName(const clang::VarDecl* var) {
    if (!var) return "";
    std::string name = var->getNameAsString();
    if (name.empty()) return "";

    // Internal-linkage (file-scope `static`) variables — including
    // `static K_MUTEX_DEFINE(...)`-created ones — are private to their
    // translation unit: qualify with the source-file basename so
    // same-named statics in different TUs stay distinct (e.g. Zephyr's
    // `static struct k_mutex mutex` in subsys/fs/fs.c vs
    // `static K_MUTEX_DEFINE(mutex)` in subsys/disk/disk_access.c, or the
    // per-TU `static K_MUTEX_DEFINE(lock)` instances throughout the net
    // subsystem). The storage class is only set on the defining
    // declaration, hence the canonical-decl indirection. Note the test
    // must be SC_Static and not !hasExternalStorage(): a plain external
    // definition (`struct k_mutex m = {...}`, storage class SC_None) has
    // external linkage but no explicit `extern`, and
    // hasExternalStorage() is false for it.
    const clang::VarDecl* canonical = llvm::dyn_cast_or_null<clang::VarDecl>(var->getCanonicalDecl());
    if (!canonical) canonical = var;
    if (canonical->getDeclContext() && canonical->getDeclContext()->isFileContext() &&
        canonical->getStorageClass() == clang::SC_Static) {
        std::string sourceFile;
        if (canonical->getLocation().isValid() && g_sourceManager) {
            clang::PresumedLoc pLoc = g_sourceManager->getPresumedLoc(canonical->getLocation());
            if (pLoc.isValid()) sourceFile = pLoc.getFilename();
        }
        if (!sourceFile.empty()) {
            size_t slash = sourceFile.find_last_of("/\\");
            if (slash != std::string::npos) {
                sourceFile = sourceFile.substr(slash + 1);
            }
            return name + "@" + sourceFile;
        }
    }

    // External linkage (plain `K_MUTEX_DEFINE(...)` or a normal global):
    // the bare name is the project-wide symbol identity, matching the
    // pass-1 macro registration for K_*_DEFINE-created objects.
    return name;
}

bool isGeneratedSyscallHeaderLoc(const clang::SourceLocation& loc) {
    if (!loc.isValid() || !g_sourceManager) return false;
    clang::PresumedLoc pLoc = g_sourceManager->getPresumedLoc(loc);
    if (!pLoc.isValid()) return false;
    const char* filename = pLoc.getFilename();
    if (!filename || filename[0] == '\0') return false;
    std::string f(filename);
    // Zephyr generates the inline syscall wrappers into
    //   <build>/zephyr/include/generated/syscalls/*.h          (≤3.5)
    //   <build>/zephyr/include/generated/zephyr/syscalls/*.h   (≥3.6)
    // Both always contain "generated" and "syscalls" path
    // components; requiring both keeps the check from firing on an
    // unrelated app directory that happens to be named "syscalls".
    return f.find("generated") != std::string::npos &&
           f.find("syscalls") != std::string::npos;
}

bool isZephyrKernelInternal(const std::string& fn) {
    if (fn.rfind("arch_", 0) == 0) return true;
    if (fn.rfind("z_", 0) == 0) return true;
    if (fn.rfind("impl_", 0) == 0) return true;
    if (fn.rfind("__", 0) == 0) return true;
    return false;
}

void computeReachableFunctions() {
    g_reachableFromThreads = g_threadEntryPoints;
    g_helperToThreads.clear();

    for (const auto& entryPoint : g_threadEntryPoints) {
        std::set<std::string> visited;
        std::vector<std::string> worklist;
        worklist.push_back(entryPoint);

        while (!worklist.empty()) {
            std::string fn = worklist.back();
            worklist.pop_back();

            if (visited.count(fn) > 0) continue;
            visited.insert(fn);

            auto it = g_callGraph.find(fn);
            if (it == g_callGraph.end()) continue;

            for (const auto& callee : it->second) {
                if (g_functionsWithDefinitions.count(callee) == 0) continue;
                if (g_threadEntryPoints.count(callee) > 0) continue;
                if (visited.count(callee) > 0) continue;

                g_reachableFromThreads.insert(callee);
                g_helperToThreads[callee].insert(entryPoint);
                worklist.push_back(callee);
            }
        }
    }

    // Add SYS_INIT functions as reachable from the init thread.
    // SYS_INIT callbacks run during kernel init (before main), in the
    // context of the init thread. Attribute their IPC calls to that thread.
    if (!g_sysInitFunctions.empty()) {
        std::string initThread;
        for (const auto& name : g_threadEntryPoints) {
            if (name == "bg_thread_main" || name == "main") {
                initThread = name;
                break;
            }
        }
        if (initThread.empty() && !g_threadEntryPoints.empty()) {
            initThread = *g_threadEntryPoints.begin();
        }

        std::string initDisplay = initThread.empty() ? "" : threadDisplayName(initThread);

        for (const auto& sysInitFn : g_sysInitFunctions) {
            g_reachableFromThreads.insert(sysInitFn);
            if (!initThread.empty()) {
                g_helperToThreads[sysInitFn].insert(initThread);
            }

            std::set<std::string> visited;
            std::vector<std::string> worklist;
            worklist.push_back(sysInitFn);

            while (!worklist.empty()) {
                std::string fn = worklist.back();
                worklist.pop_back();

                if (visited.count(fn) > 0) continue;
                visited.insert(fn);

                auto it = g_callGraph.find(fn);
                if (it == g_callGraph.end()) continue;

                for (const auto& callee : it->second) {
                    if (g_functionsWithDefinitions.count(callee) == 0) continue;
                    if (g_threadEntryPoints.count(callee) > 0) continue;
                    if (visited.count(callee) > 0) continue;

                    g_reachableFromThreads.insert(callee);
                    if (!initThread.empty()) {
                        g_helperToThreads[callee].insert(initThread);
                    }
                    worklist.push_back(callee);
                }
            }
        }
    }

    // Add work queue thread callbacks as reachable from their queue threads.
    // k_work_queue_start creates a thread; work items submitted to that queue
    // execute on that thread. Map callbacks to the queue's virtual thread.
    for (const auto& [queueName, threadEntryFn] : g_workQueueThreads) {
        g_threadEntryPoints.insert(threadEntryFn);
        g_reachableFromThreads.insert(threadEntryFn);

        auto cbIt = g_workQueueCallbacks.find(queueName);
        if (cbIt != g_workQueueCallbacks.end()) {
            for (const auto& callback : cbIt->second) {
                g_reachableFromThreads.insert(callback);
                g_helperToThreads[callback].insert(threadEntryFn);

                std::set<std::string> visited;
                std::vector<std::string> worklist;
                worklist.push_back(callback);

                while (!worklist.empty()) {
                    std::string fn = worklist.back();
                    worklist.pop_back();

                    if (visited.count(fn) > 0) continue;
                    visited.insert(fn);

                    auto cgIt = g_callGraph.find(fn);
                    if (cgIt == g_callGraph.end()) continue;

                    for (const auto& callee : cgIt->second) {
                        if (g_functionsWithDefinitions.count(callee) == 0) continue;
                        if (g_threadEntryPoints.count(callee) > 0) continue;
                        if (visited.count(callee) > 0) continue;

                        g_reachableFromThreads.insert(callee);
                        g_helperToThreads[callee].insert(threadEntryFn);
                        worklist.push_back(callee);
                    }
                }
            }
        }
    }

    // Add ISR functions as reachable from an ISR pseudo-thread.
    // IRQ_CONNECT registers an ISR handler that runs in interrupt context.
    // Model these as a single "isr" pseudo-thread for IPC attribution.
    if (!g_isrFunctions.empty()) {
        std::string isrThread = "isr";
        g_threadEntryPoints.insert(isrThread);
        g_reachableFromThreads.insert(isrThread);

        ThreadInfo isrInfo;
        isrInfo.name = "isr";
        isrInfo.entryFn = "isr";
        isrInfo.isSystem = true;
        g_threadInfos[isrThread] = isrInfo;

        for (const auto& isrFn : g_isrFunctions) {
            g_reachableFromThreads.insert(isrFn);
            g_helperToThreads[isrFn].insert(isrThread);

            std::set<std::string> visited;
            std::vector<std::string> worklist;
            worklist.push_back(isrFn);

            while (!worklist.empty()) {
                std::string fn = worklist.back();
                worklist.pop_back();

                if (visited.count(fn) > 0) continue;
                visited.insert(fn);

                auto cgIt = g_callGraph.find(fn);
                if (cgIt == g_callGraph.end()) continue;

                for (const auto& callee : cgIt->second) {
                    if (g_functionsWithDefinitions.count(callee) == 0) continue;
                    if (g_threadEntryPoints.count(callee) > 0) continue;
                    if (visited.count(callee) > 0) continue;

                    g_reachableFromThreads.insert(callee);
                    g_helperToThreads[callee].insert(isrThread);
                    worklist.push_back(callee);
                }
            }
        }
    }

    // Add INPUT_CALLBACK_DEFINE callbacks as reachable from init thread.
    // Input callbacks are registered during init via INPUT_CALLBACK_DEFINE,
    // then invoked in interrupt context when input events arrive.
    // Attribute their IPC calls to the init thread (same as SYS_INIT).
    if (!g_inputCallbacks.empty()) {
        std::string initThread;
        for (const auto& name : g_threadEntryPoints) {
            if (name == "bg_thread_main" || name == "main") {
                initThread = name;
                break;
            }
        }
        if (initThread.empty() && !g_threadEntryPoints.empty()) {
            initThread = *g_threadEntryPoints.begin();
        }

        for (const auto& cbFn : g_inputCallbacks) {
            g_reachableFromThreads.insert(cbFn);
            if (!initThread.empty()) {
                g_helperToThreads[cbFn].insert(initThread);
            }

            std::set<std::string> visited;
            std::vector<std::string> worklist;
            worklist.push_back(cbFn);

            while (!worklist.empty()) {
                std::string fn = worklist.back();
                worklist.pop_back();

                if (visited.count(fn) > 0) continue;
                visited.insert(fn);

                auto cgIt = g_callGraph.find(fn);
                if (cgIt == g_callGraph.end()) continue;

                for (const auto& callee : cgIt->second) {
                    if (g_functionsWithDefinitions.count(callee) == 0) continue;
                    if (g_threadEntryPoints.count(callee) > 0) continue;
                    if (visited.count(callee) > 0) continue;
                    if (g_reachableFromThreads.count(callee) > 0) continue;

                    g_reachableFromThreads.insert(callee);
                    if (!initThread.empty()) {
                        g_helperToThreads[callee].insert(initThread);
                    }
                    worklist.push_back(callee);
                }
            }
        }
    }

    // Add shell command handlers as reachable from the shell thread.
    // SHELL_CMD/SHELL_CMD_ARG/SHELL_CMD_REGISTER register command handlers
    // that are invoked by the shell thread when commands are entered.
    if (!g_shellCommands.empty()) {
        std::string shellThread;
        for (const auto& name : g_threadEntryPoints) {
            if (name == "shell_thread") {
                shellThread = name;
                break;
            }
        }

        if (!shellThread.empty()) {
            for (const auto& cmdFn : g_shellCommands) {
                g_reachableFromThreads.insert(cmdFn);
                g_helperToThreads[cmdFn].insert(shellThread);

                std::set<std::string> visited;
                std::vector<std::string> worklist;
                worklist.push_back(cmdFn);

                while (!worklist.empty()) {
                    std::string fn = worklist.back();
                    worklist.pop_back();

                    if (visited.count(fn) > 0) continue;
                    visited.insert(fn);

                    auto cgIt = g_callGraph.find(fn);
                    if (cgIt == g_callGraph.end()) continue;

                    for (const auto& callee : cgIt->second) {
                        if (g_functionsWithDefinitions.count(callee) == 0) continue;
                        if (g_threadEntryPoints.count(callee) > 0) continue;
                        if (visited.count(callee) > 0) continue;

                        g_reachableFromThreads.insert(callee);
                        g_helperToThreads[callee].insert(shellThread);
                        worklist.push_back(callee);
                    }
                }
            }
        }
    }

    // Add function-pointer callbacks as reachable from the ISR thread.
    // Functions passed as arguments to calls (e.g., currsmp_configure(dev,
    // regulate, NULL)) are potential callbacks that may be invoked from
    // interrupt context. Only add callbacks registered by functions that
    // are reachable from the init thread (SYS_INIT callbacks run during
    // kernel init and typically register ISR callbacks). Skip callbacks
    // already reachable from any thread to avoid false positives.
    if (!g_callbackRegistrations.empty() && g_threadEntryPoints.count("isr") > 0) {
        std::string isrThread = "isr";

        std::string initThread;
        for (const auto& name : g_threadEntryPoints) {
            if (name == "bg_thread_main" || name == "main") {
                initThread = name;
                break;
            }
        }

        for (const auto& [cbFn, registrants] : g_callbackRegistrations) {
            if (g_reachableFromThreads.count(cbFn) > 0) continue;

            bool registeredByInit = false;
            for (const auto& registrant : registrants) {
                auto it = g_helperToThreads.find(registrant);
                if (it != g_helperToThreads.end() && !initThread.empty() &&
                    it->second.count(initThread) > 0) {
                    registeredByInit = true;
                    break;
                }
            }
            if (!registeredByInit) continue;

            g_reachableFromThreads.insert(cbFn);
            g_helperToThreads[cbFn].insert(isrThread);

            std::set<std::string> visited;
            std::vector<std::string> worklist;
            worklist.push_back(cbFn);

            while (!worklist.empty()) {
                std::string fn = worklist.back();
                worklist.pop_back();

                if (visited.count(fn) > 0) continue;
                visited.insert(fn);

                auto cgIt = g_callGraph.find(fn);
                if (cgIt == g_callGraph.end()) continue;

                for (const auto& callee : cgIt->second) {
                    if (g_functionsWithDefinitions.count(callee) == 0) continue;
                    if (g_threadEntryPoints.count(callee) > 0) continue;
                    if (visited.count(callee) > 0) continue;

                    g_reachableFromThreads.insert(callee);
                    g_helperToThreads[callee].insert(isrThread);
                    worklist.push_back(callee);
                }
            }
        }
    }
}

std::string threadDisplayName(const std::string& entryFn) {
    auto it = g_threadInfos.find(entryFn);
    if (it != g_threadInfos.end() && !it->second.name.empty()) {
        return it->second.name;
    }
    return entryFn;
}

std::vector<std::string> getOwningThreads(const std::string& fnName) {
    if (g_threadEntryPoints.count(fnName) > 0) {
        return {threadDisplayName(fnName)};
    }
    auto it = g_helperToThreads.find(fnName);
    if (it != g_helperToThreads.end()) {
        std::vector<std::string> result;
        for (const auto& entryFn : it->second) {
            result.push_back(threadDisplayName(entryFn));
        }
        return result;
    }
    return {};
}

std::string stripZImpl(const std::string& name) {
    const std::string prefix = "z_impl_";
    if (name.compare(0, prefix.size(), prefix) == 0) {
        return name.substr(prefix.size());
    }
    return name;
}

ActionType inferActionType(const std::string& funcName) {
    if (funcName == "k_mutex_lock") return ActionType::Lock;
    if (funcName == "k_mutex_unlock") return ActionType::Unlock;
    if (funcName == "k_sem_take") return ActionType::Take;
    if (funcName == "k_sem_give") return ActionType::Give;
    if (funcName == "k_sem_reset") return ActionType::Reset;
    if (funcName == "k_fifo_put" || funcName == "k_fifo_alloc_put" ||
        funcName == "k_lifo_put" || funcName == "k_lifo_alloc_put" ||
        funcName == "k_msgq_put" || funcName == "k_pipe_put" ||
        funcName == "k_queue_append" || funcName == "k_queue_insert" || funcName == "k_queue_prepend") return ActionType::Put;
    if (funcName == "k_fifo_get" || funcName == "k_lifo_get" ||
        funcName == "k_msgq_get" || funcName == "k_pipe_get" ||
        funcName == "k_queue_get") return ActionType::Get;
    if (funcName == "k_mbox_put" || funcName == "k_mbox_async_put") return ActionType::Send;
    if (funcName == "k_mbox_get") return ActionType::Receive;
    if (funcName == "k_event_post") return ActionType::Post;
    if (funcName == "k_event_wait") return ActionType::Wait;
    if (funcName == "k_condvar_signal") return ActionType::Signal;
    if (funcName == "k_condvar_wait") return ActionType::Wait;
    if (funcName == "k_condvar_broadcast") return ActionType::Broadcast;
    if (funcName == "k_stack_push") return ActionType::Push;
    if (funcName == "k_stack_pop") return ActionType::Pop;
    if (funcName == "k_timer_start") return ActionType::Start;
    if (funcName == "k_timer_stop") return ActionType::Stop;
    if (funcName == "k_heap_alloc" || funcName == "k_mem_slab_alloc" || funcName == "malloc" || funcName == "calloc" || funcName == "realloc" || funcName == "k_malloc") return ActionType::Alloc;
    if (funcName == "k_heap_free" || funcName == "k_mem_slab_free" || funcName == "free" || funcName == "k_free") return ActionType::Free;
    if (funcName == "k_poll_signal_raise") return ActionType::Raise;
    if (funcName == "k_poll_signal_reset") return ActionType::Reset;
    if (funcName == "k_thread_join") return ActionType::Join;
    if (funcName == "k_thread_abort") return ActionType::Abort;
    if (funcName == "k_wakeup") return ActionType::Wakeup;
    if (funcName == "k_work_submit" || funcName == "k_work_submit_to_queue" ||
        funcName == "k_work_schedule" || funcName == "k_work_schedule_for_queue" ||
        funcName == "k_work_reschedule" || funcName == "k_work_reschedule_for_queue") return ActionType::Submit;
    if (funcName == "k_work_cancel") return ActionType::Cancel;
    if (funcName == "k_queue_remove" || funcName == "k_msgq_purge") return ActionType::Unknown;
    return ActionType::Unknown;
}

bool actionCanBlock(ActionType action) {
    switch (action) {
        case ActionType::Lock:
        case ActionType::Take:
        case ActionType::Wait:
        case ActionType::Get:
        case ActionType::Pop:
        case ActionType::Receive:
        case ActionType::Alloc:
        case ActionType::Join:
            return true;
        default:
            return false;
    }
}