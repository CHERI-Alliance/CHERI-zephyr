/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "Model.h"

#include "clang/AST/Expr.h"
#include "clang/Basic/SourceManager.h"

namespace clang { class ASTContext; }

#include <string>
#include <vector>
#include <map>
#include <set>

struct ThreadInfo {
    std::string name;
    std::string entryFn;
    SourceLocation location;
    int priority;
    std::string sourceFile;
    bool isSystem;
    bool isWorkqueue;
    ThreadInfo() : priority(0), isSystem(false), isWorkqueue(false) {}
};

struct IpcCall {
    std::string threadName;
    std::string objectName;
    ActionType action;
    SourceLocation location;
    bool canBlock;
};

struct ThreadToThreadCall {
    std::string fromThread;
    std::string targetThread;
    EdgeType edgeType;
    SourceLocation location;
    bool canBlock;
};

struct ObjectInfo {
    std::string name;
    std::string typeStr;
    SourceLocation location;
    std::string sourceFile;
    bool isSystem;
    ObjectInfo() : isSystem(false) {}
};

extern std::set<std::string> g_threadEntryPoints;
extern std::map<std::string, ThreadInfo> g_threadInfos;
extern std::string g_currentFunction;
extern const clang::SourceManager* g_sourceManager;
extern clang::ASTContext* g_astContext;
extern std::vector<IpcCall> g_ipcCalls;
extern std::vector<ThreadToThreadCall> g_threadToThreadCalls;
extern std::vector<ObjectInfo> g_objects;
extern std::set<std::string> g_objectsFromMacros;
// Discovered struct-typed IPC fields, keyed by struct type name (e.g. "espi_dev",
// "app_ctx") -> field name -> object type. Populated by Pass2Visitor::VisitRecordDecl
// and consulted by checkIpcOperation() to resolve `&ctx->field` access patterns where
// `field` is itself a kernel object (k_mutex / k_sem / etc.). Lookups are by struct
// type rather than by instance, since the static analysis cannot distinguish between
// two runtime instances of the same struct type.
extern std::map<std::string, std::map<std::string, ObjectType>> g_structFieldObjects;
// Synthetic embedded-IPC object names registered from g_structFieldObjects. Used by
// the global-var collector to skip the parent struct variable when its only purpose
// is to host discovered embedded IPC fields (otherwise the struct itself would be
// mis-recorded as a HIGH shared-memory coupling across the threads that touch it).
extern std::set<std::string> g_structsWithIpcFields;
extern std::map<std::string, std::map<std::string, GlobalVarAccess>> g_globalVarAccesses;
extern std::set<std::string> g_globalVarNames;
extern std::map<std::string, std::string> g_tidToThread;
extern std::map<std::string, std::set<std::string>> g_callGraph;
extern std::set<std::string> g_functionsWithDefinitions;
extern std::map<std::string, std::set<std::string>> g_helperToThreads;
extern std::set<std::string> g_reachableFromThreads;
extern std::set<std::string> g_ipcFunctions;
extern std::map<std::string, std::set<std::string>> g_unresolvedCalls;
extern std::map<std::string, std::string> g_callbackTargets;
extern std::set<std::string> g_sysInitFunctions;
extern std::map<std::string, std::string> g_workQueueThreads;
extern std::map<std::string, std::set<std::string>> g_workQueueCallbacks;
extern std::set<std::string> g_isrFunctions;
extern std::set<std::string> g_inputCallbacks;
extern std::set<std::string> g_callbackFunctions;
extern std::map<std::string, std::set<std::string>> g_callbackRegistrations;
extern std::set<std::string> g_isrRegistrationFunctions;
extern std::set<std::string> g_nonIsrInlineFunctions;
extern std::set<std::string> g_shellCommands;

// Threads that exist purely to run SYS_INIT / boot-phase code. Today this is
// effectively just {"bg_thread_main", "main"} because the SYS_INIT plumbing
// in AnalysisUtils.cpp:165-212 attributes every SYS_INIT callback to
// bg_thread_main. Used by GraphBuilder::addGlobalVarEdges to detect
// write-once init-table patterns (e.g. SHELL_CMD_REGISTER, MODULE_DECLARE,
// bt_hci driver cb registration): when every writer thread of a global
// variable is in this set, the variable is treated as an advisory init-table
// edge rather than a propagating shared-memory coupling.
extern std::set<std::string> g_initThreads;
bool isInitThread(const std::string& threadName);
void computeInitThreads();

/** Recognise Zephyr linker-section iterable boundary symbols produced by
 *  TYPE_SECTION_ITERABLE / ITERABLE_SECTION_ROM / ITERABLE_SECTION_RAM
 *  (e.g. `_shell_root_cmds_list_start`, `_bt_conn_cb_list_end`,
 *  `_net_mgmt_event_static_handler_list_start`, `__event_subscriptions_end`).
 *  These symbols are emitted by the linker to bound a section populated at
 *  link time from SHELL_CMD_REGISTER / NET_MGMT_EVENT_HANDLER / BT_CONN_CB
 *  / etc. macro instances; no thread ever writes them at runtime. They
 *  couple every reader of the section to the (nonexistent) writer in the
 *  static graph, so GraphBuilder demotes them to EdgeType::InitTable.
 */
bool isLinkerSectionSymbol(const std::string& varName);

/** Strip parenImp casts AND explicit casts (CStyle/Static/Functional etc.) so
 *  entry-point extraction can see the underlying DeclRefExpr through patterns
 *  like `(k_thread_entry_t)my_thread` or `K_PRIO_PREEMPT(5)`.
 *  Returns the innermost non-cast expression (may be a DeclRefExpr / IntegerLiteral / etc.).
 */
const clang::Expr* stripCasts(const clang::Expr* e);

/** Build the synthetic IPC-object name used for embedded struct fields. The scheme
 *  is `<StructType>::<fieldName>` (e.g. `espi_dev::lock`). The struct type name is
 *  the bare record name (without `struct ` prefix, without template parameters)
 *  so that downstream dedup in main.cpp produces one object per struct-type/field
 *  pair. Returns an empty string if no record type can be derived (e.g. callers
 *  passing a void* base or a non-record-typed expression).
 */
std::string makeEmbeddedObjectName(const clang::Type* baseType, const std::string& fieldName);

/** Given a QualType, peel through pointers, references, and qualifiers to get
 *  the underlying `RecordDecl` (or nullptr if the type is not a record). Also
 *  handles the unary-deref case: callers should pass the *pointee* type (i.e.
 *  the type yielded by `memberExpr->getBase()->getType()` for `(*ptr)->field`
 *  will be `T*`, this returns the T record).
 */
const clang::RecordDecl* baseTypeAsRecordDecl(const clang::QualType& qt);

SourceLocation convertSourceLocation(const clang::SourceLocation& loc);
bool isZephyrKernelInternal(const std::string& fn);

/** Registration/resolution name for a kernel-object variable.
 *
 *  External-linkage variables keep their bare name (`data_mutex`); a bare
 *  name that was created by a K_*_DEFINE macro (K_MUTEX_DEFINE etc.) also
 *  keeps the bare name, matching the pass-1 macro registration (the macro
 *  callback has no linkage information). Internal-linkage (file-scope
 *  `static`) variables are qualified with the source-file basename
 *  (`mutex@fs.c`), mirroring the global-var access scheme in
 *  Visitors.cpp, so same-named statics in different translation units do
 *  not collapse into one fake-shared object. Note: a static defined in a
 *  shared header (one instance per including TU) still merges under this
 *  scheme — a known, benign limitation of basename qualification.
 */
std::string qualifiedObjectVarName(const clang::VarDecl* var);

/** True when loc spells inside a Zephyr generated syscall wrapper header
 *  (…/include/generated[/zephyr]/syscalls/*.h — layout varies by Zephyr
 *  version, and these directories are -I, not -isystem, so
 *  SourceManager::isInSystemHeader does not fire). The inline wrappers
 *  there call z_impl_*() with their own *parameters* as arguments;
 *  attributing those wrapper-internal calls as thread IPC operations
 *  smears every calling thread onto a same-named file-scope object
 *  (e.g. `z_impl_k_mutex_lock(mutex, …)` → object "mutex" colliding with
 *  `static struct k_mutex mutex` in zephyr/subsys/fs/fs.c). See
 *  analysis-report.md, Defect 1.
 */
bool isGeneratedSyscallHeaderLoc(const clang::SourceLocation& loc);
void computeReachableFunctions();
std::string threadDisplayName(const std::string& entryFn);
std::vector<std::string> getOwningThreads(const std::string& fnName);
std::string stripZImpl(const std::string& name);
ActionType inferActionType(const std::string& funcName);
bool actionCanBlock(ActionType action);
bool canIpcCallBlock(const std::string& funcName, clang::CallExpr* call);
void checkIpcOperation(clang::CallExpr* call, ActionType action, const std::string& funcName);
void processRecoveryExpr(clang::RecoveryExpr* recovery, ActionType action);
bool checkThreadToThreadCall(clang::CallExpr* call, const std::string& funcName, ActionType action);

#endif