/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Globals.h"

#include "clang/AST/Expr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include "llvm/Support/Casting.h"

namespace {

// Walk a MemberExpr chain to find the rightmost (deepest) FieldDecl whose type
// is a known kernel IPC object. Used to resolve patterns like:
//   k_mutex_lock(&ctx->lock, ...)            // single hop
//   k_queue_append(&fifo._queue, ...)        // single hop, underlying FIFO var
//   k_work_submit(&ctx->work._work, ...)     // nested: outer struct + Work
// Returns:
//   - the synthetic embedded object name (e.g. "app_ctx::lock" or "poll_work::work")
//   - or the file-scope variable name (e.g. "fifo" when the chain is `&fifo._queue`
//     and `_queue` is a kernel object field of the FIFO expansion struct)
// and the resolved object type in `outType` when known (always set if a name
// is returned through the embedded path; left unchanged for the file-scope path
// since the caller already has that information from elsewhere).
//
// Covers three base shapes:
//   (a) &(declRefExpr->field ...)  — base is a value-typed global like struct `fifo`
//                                    (the Zephyr K_FIFO_DEFINE expansion where
//                                    `fifo` itself is the file-scope global)
//   (b) &((declRefExpr)->field)    — base is a pointer-typed global like `ctx`
//   (c) &((this)->field)           — base is `this` in a C++ method
//   (d) &((*ptr)->field)           — base is unary-deref of a pointer
// Nested chains (base is itself a MemberExpr) recurse.
std::string resolveEmbeddedObjectName(const clang::MemberExpr* me,
                                      bool baseIsAddrOfWrapped);

// Given the base sub-expression of a MemberExpr, return either:
//   - "var:<varname>"   if the base refers to a file-scope global variable
//                       (the FIFO/queue macro-expansion case where the variable
//                       is itself the IPC object container)
//   - "struct:<type>"   if the base refers to a record-typed expression whose
//                       record type is in g_structFieldObjects (the
//                       ctx->lock embedded-field case)
//   - ""                if not resolvable
std::string classifyMemberBase(const clang::Expr* base, bool baseIsAddrOfWrapped) {
    if (!base) return "";
    base = base->IgnoreParenImpCasts();
    if (!base) return "";

    // `(*ptr)->field` — peel the unary-deref to get the pointer-typed base.
    if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(base)) {
        if (unaryOp->getOpcode() == clang::UO_Deref) {
            return classifyMemberBase(unaryOp->getSubExpr(), false);
        }
    }

    // `this->field` (C++). Resolve to the enclosing CXXRecord from the
    // expression's type (Clang versions vary; older ones don't expose
    // CXXThisExpr::getRecord directly).
    if (llvm::isa<clang::CXXThisExpr>(base)) {
        clang::QualType thisType = base->getType();
        const clang::RecordDecl* record = baseTypeAsRecordDecl(thisType);
        if (!record) return "";
        std::string name = record->getNameAsString();
        if (name.empty()) return "";
        return "struct:" + name;
    }

    // Plain DeclRefExpr base — either `fifo` (use the var name) or `ctx` (use
    // the struct type). We distinguish using g_structFieldObjects: if the
    // referenced variable's record type contains a discovered IPC field, treat
    // the var as an embedded-field container; otherwise treat the var itself
    // as the IPC object (the FIFO macro-expansion case).
    if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(base)) {
        if (auto var = llvm::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
            const clang::RecordDecl* rd = baseTypeAsRecordDecl(var->getType());
            if (rd && rd->getIdentifier()) {
                std::string tname = rd->getNameAsString();
                if (g_structsWithIpcFields.count(tname) > 0) {
                    return "struct:" + tname;
                }
            }
            // `var` is itself the IPC container (e.g. `&fifo._queue`)
            return "var:" + var->getNameAsString();
        }
        if (auto fn = llvm::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
            return "";  // base is a function name — not resolvable here
        }
    }

    // Nested MemberExpr: `outer.inner.field`.
    if (auto innerMe = llvm::dyn_cast<clang::MemberExpr>(base)) {
        return classifyMemberBase(innerMe->getBase(), false);
    }

    return "";
}

// Try to resolve a MemberExpr chain rooted in an IPC field. Returns the
// synthetic embedded object name on success, "" on failure (in which case
// the caller falls back to the existing variable-based resolution).
std::string resolveEmbeddedMemberName(const clang::MemberExpr* me) {
    if (!me || !me->getMemberDecl()) return "";
    auto* field = llvm::dyn_cast<clang::FieldDecl>(me->getMemberDecl());
    if (!field) return "";
    std::string fieldName = field->getNameAsString();
    if (fieldName.empty()) return "";

    // Walk the base. classifyMemberBase returns either "struct:Type" or
    // "var:varname"; the latter means the MemberExpr is a field of a
    // file-scope struct that is itself treated as an IPC object container
    // (FIFO/LIFO expansion), which is handled by the existing branch above.
    std::string baseClass = classifyMemberBase(me->getBase(),
                                               me->isArrow());
    if (baseClass.empty()) return "";
    if (baseClass.compare(0, 7, "struct:") == 0) {
        std::string structType = baseClass.substr(7);
        auto it = g_structFieldObjects.find(structType);
        if (it != g_structFieldObjects.end() &&
            it->second.count(fieldName) > 0) {
            return structType + "::" + fieldName;
        }
    }
    return "";
}

} // namespace

bool canIpcCallBlock(const std::string& funcName, clang::CallExpr* call) {
    if (funcName == "k_mutex_lock" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_sem_take" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_fifo_get" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_lifo_get" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_queue_get" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if ((funcName == "k_msgq_put" || funcName == "k_msgq_get") && call->getNumArgs() >= 3) {
        clang::Expr* timeoutArg = call->getArg(2)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_mbox_get" && call->getNumArgs() >= 3) {
        clang::Expr* timeoutArg = call->getArg(2)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_mbox_put" && call->getNumArgs() >= 3) {
        clang::Expr* timeoutArg = call->getArg(2)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_pipe_put" && call->getNumArgs() >= 4) {
        clang::Expr* timeoutArg = call->getArg(3)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_pipe_get" && call->getNumArgs() >= 4) {
        clang::Expr* timeoutArg = call->getArg(3)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_event_wait" && call->getNumArgs() >= 4) {
        clang::Expr* timeoutArg = call->getArg(3)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_condvar_wait" && call->getNumArgs() >= 3) {
        clang::Expr* timeoutArg = call->getArg(2)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_stack_pop" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_mem_slab_alloc" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "k_heap_alloc" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    if (funcName == "malloc" || funcName == "calloc" || funcName == "realloc" ||
        funcName == "free" || funcName == "k_malloc" || funcName == "k_free") {
        return false;
    }
    if (funcName == "k_thread_join" && call->getNumArgs() >= 2) {
        clang::Expr* timeoutArg = call->getArg(1)->IgnoreParenImpCasts();
        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timeoutArg)) {
            std::string name = declRef->getNameInfo().getAsString();
            if (name == "K_FOREVER") return true;
            if (name == "K_NO_WAIT") return false;
        }
        return true;
    }
    return false;
}

void processRecoveryExpr(clang::RecoveryExpr* recovery, ActionType action) {
    if (!recovery || g_currentFunction.empty() ||
        g_reachableFromThreads.count(g_currentFunction) == 0) {
        return;
    }

    // Same wrapper guard as checkIpcOperation: never attribute calls that
    // spell inside the generated syscall headers.
    if (isGeneratedSyscallHeaderLoc(recovery->getBeginLoc())) return;

    std::string funcName;
    std::string objName;

    // Accept a variable reference only when it has global storage, and
    // record its linkage-qualified name (see checkIpcOperation).
    auto resolveGlobalVar = [](const clang::DeclRefExpr* declRef) -> std::string {
        if (!declRef) return "";
        auto* var = llvm::dyn_cast<clang::VarDecl>(declRef->getDecl());
        if (!var) return "";
        if (!var->hasGlobalStorage()) return "";
        return qualifiedObjectVarName(var);
    };

    for (auto* child : recovery->children()) {
        if (!child) continue;

        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(child)) {
            if (llvm::isa<clang::FunctionDecl>(declRef->getDecl())) {
                funcName = declRef->getNameInfo().getAsString();
            } else {
                std::string resolved = resolveGlobalVar(declRef);
                if (!resolved.empty()) objName = resolved;
            }
        } else if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(child)) {
            if (unaryOp->getOpcode() == clang::UO_AddrOf) {
                if (auto subExpr = llvm::dyn_cast<clang::DeclRefExpr>(unaryOp->getSubExpr())) {
                    std::string resolved = resolveGlobalVar(subExpr);
                    if (!resolved.empty()) objName = resolved;
                }
            }
        } else if (auto binaryOp = llvm::dyn_cast<clang::BinaryOperator>(child)) {
            if (binaryOp->getOpcode() == clang::BO_Comma) {
                for (auto* subchild : binaryOp->children()) {
                    if (!subchild) continue;
                    if (auto subDeclRef = llvm::dyn_cast<clang::DeclRefExpr>(subchild)) {
                        if (llvm::isa<clang::FunctionDecl>(subDeclRef->getDecl())) {
                            funcName = subDeclRef->getNameInfo().getAsString();
                        } else {
                            std::string resolved = resolveGlobalVar(subDeclRef);
                            if (!resolved.empty()) objName = resolved;
                        }
                    } else if (auto subUnaryOp = llvm::dyn_cast<clang::UnaryOperator>(subchild)) {
                        if (subUnaryOp->getOpcode() == clang::UO_AddrOf) {
                            if (auto subExpr = llvm::dyn_cast<clang::DeclRefExpr>(subUnaryOp->getSubExpr())) {
                                std::string resolved = resolveGlobalVar(subExpr);
                                if (!resolved.empty()) objName = resolved;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!funcName.empty() && !objName.empty()) {
        std::string canonicalName = stripZImpl(funcName);
        if (g_ipcFunctions.count(canonicalName) > 0) {
            SourceLocation loc = convertSourceLocation(recovery->getBeginLoc());
            bool canBlock = actionCanBlock(action);
            for (const auto& owningThread : getOwningThreads(g_currentFunction)) {
                g_ipcCalls.push_back({owningThread, objName, action, loc, canBlock});
            }
        }
    }
}

bool checkThreadToThreadCall(clang::CallExpr* call, const std::string& funcName, ActionType action) {
    if (funcName != "k_thread_join" && funcName != "k_thread_abort" &&
        funcName != "k_wakeup" && funcName != "z_impl_k_thread_join" &&
        funcName != "z_impl_k_thread_abort" && funcName != "z_impl_k_wakeup") {
        return false;
    }

    // Never attribute wrapper-internal z_impl_k_thread_*() calls from the
    // generated syscall headers: their argument is the wrapper's own
    // parameter, not a resolvable thread id.
    if (isGeneratedSyscallHeaderLoc(call->getBeginLoc())) return true;

    if (call->getNumArgs() < 1) return true;
    if (g_currentFunction.empty() || g_reachableFromThreads.count(g_currentFunction) == 0) return true;

    std::string targetThreadEntry;
    clang::Expr* arg = call->getArg(0)->IgnoreParenImpCasts();
    if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(arg)) {
        // Only global thread-id variables (the `const k_tid_t` globals
        // emitted by K_THREAD_DEFINE) can identify a thread statically;
        // parameters/locals named like a thread are coincidences.
        auto* var = llvm::dyn_cast<clang::VarDecl>(declRef->getDecl());
        if (!var || var->hasGlobalStorage()) {
            std::string tidName = declRef->getNameInfo().getAsString();
            auto it = g_tidToThread.find(tidName);
            if (it != g_tidToThread.end()) {
                targetThreadEntry = it->second;
            } else if (g_threadEntryPoints.count(tidName) > 0) {
                targetThreadEntry = tidName;
            }
        }
    }

    if (targetThreadEntry.empty()) return true;

    std::string targetDisplayName = threadDisplayName(targetThreadEntry);
    EdgeType edgeType = EdgeType::Join;
    if (funcName == "k_thread_abort" || funcName == "z_impl_k_thread_abort") {
        edgeType = EdgeType::Abort;
    } else if (funcName == "k_wakeup" || funcName == "z_impl_k_wakeup") {
        edgeType = EdgeType::Wakeup;
    }

    SourceLocation loc = convertSourceLocation(call->getBeginLoc());
    bool canBlock = (action == ActionType::Join) ? canIpcCallBlock(stripZImpl(funcName), call) : false;

    for (const auto& owningThread : getOwningThreads(g_currentFunction)) {
        g_threadToThreadCalls.push_back({owningThread, targetDisplayName, edgeType, loc, canBlock});
    }

    return true;
}

void checkIpcOperation(clang::CallExpr* call, ActionType action, const std::string& funcName) {
    if (!call) return;

    // Never attribute wrapper-internal z_impl_*() calls from the generated
    // syscall headers. Every such wrapper passes its own *parameter* as the
    // object argument (`z_impl_k_mutex_lock(mutex, …)`), and the wrapper is
    // reachable from every thread that calls the public API — so recording
    // these calls smears all callers onto a same-named file-scope object
    // (analysis-report.md, Defect 1). The real attribution happens at the
    // application-level call site, which this guard does not touch.
    if (isGeneratedSyscallHeaderLoc(call->getBeginLoc())) return;

    static const std::set<std::string> systemHeapFunctions = {
        "malloc", "calloc", "realloc", "free", "k_malloc", "k_free"
    };

    std::string objName;

    if (systemHeapFunctions.count(funcName) > 0) {
        objName = "[system_heap]";
    }

    if (objName.empty()) {
        if (call->getNumArgs() < 1) return;
        clang::Expr* arg = call->getArg(0)->IgnoreParenImpCasts();

        // Helper: accept a DeclRefExpr only when it refers to a variable
        // with global storage (file-scope or extern). Parameters and
        // locals are rejected: they are private to the current invocation
        // and can never identify a shared IPC object. This is the root
        // fix for the syscall-wrapper smearing — a ParmVarDecl is a
        // VarDecl, so the previous unguarded dyn_cast accepted it.
        auto resolveGlobalVar = [](const clang::DeclRefExpr* declRef) -> std::string {
            if (!declRef) return "";
            auto* var = llvm::dyn_cast<clang::VarDecl>(declRef->getDecl());
            if (!var) return "";
            if (!var->hasGlobalStorage()) return "";
            return qualifiedObjectVarName(var);
        };

        if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(arg)) {
            objName = resolveGlobalVar(declRef);
        } else if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(arg)) {
            if (unaryOp->getOpcode() == clang::UO_AddrOf) {
                clang::Expr* subExpr = unaryOp->getSubExpr()->IgnoreParenImpCasts();

                // Generic embedded-field resolution: `&ctx->lock`, `&state.lock`,
                // `&(this->lock)`, `&(*ptr)->lock`, `&outer.inner.lock`,
                // `&obj._queue` (FIFO/LIFO macro expansion), `&obj.sub._queue`.
                // resolveEmbeddedMemberName handles the non-FIFO cases; for
                // the FIFO/queue expansion forms (where the field type itself
                // is a k_queue and the outer variable owns it), the same
                // resolver returns the file-scope variable name.
                std::string embedded = resolveEmbeddedMemberName(
                    llvm::dyn_cast<clang::MemberExpr>(subExpr));
                if (!embedded.empty()) {
                    objName = embedded;
                } else if (auto memberExpr = llvm::dyn_cast<clang::MemberExpr>(subExpr)) {
                    // Fall back: field is `_queue` or another container field
                    // of a file-scope IPC container variable.
                    std::string memberName = memberExpr->getMemberDecl()->getNameAsString();
                    if (memberName == "_queue") {
                        clang::Expr* baseExpr = memberExpr->getBase()->IgnoreParenImpCasts();
                        if (auto baseUnaryOp = llvm::dyn_cast<clang::UnaryOperator>(baseExpr)) {
                            if (baseUnaryOp->getOpcode() == clang::UO_AddrOf) {
                                clang::Expr* inner = baseUnaryOp->getSubExpr()->IgnoreParenImpCasts();
                                if (auto baseDeclRef = llvm::dyn_cast<clang::DeclRefExpr>(inner)) {
                                    objName = resolveGlobalVar(baseDeclRef);
                                } else if (auto innerMember = llvm::dyn_cast<clang::MemberExpr>(inner)) {
                                    // k_fifo/k_lifo macro expansion over an embedded
                                    // struct field (analysis-report.md, Defect 2):
                                    // `k_fifo_put(&ctx->fifo, ...)` expands to
                                    // `k_queue_append(&(&ctx->fifo)->_queue, ...)`.
                                    // Resolve to the synthetic struct-type-keyed
                                    // object name (e.g. gs_usb_data::rx_fifo), the
                                    // same identity the embedded-field path uses for
                                    // `k_mutex_lock(&ctx->lock, ...)`.
                                    objName = resolveEmbeddedMemberName(innerMember);
                                }
                            }
                        } else if (auto baseDeclRef = llvm::dyn_cast<clang::DeclRefExpr>(baseExpr)) {
                            objName = resolveGlobalVar(baseDeclRef);
                        } else if (auto baseMember = llvm::dyn_cast<clang::MemberExpr>(baseExpr)) {
                            // Pointer-typed FIFO/LIFO field:
                            // `k_fifo_put(ctx->fifo_p, ...)` expands to
                            // `k_queue_append(&(ctx->fifo_p)->_queue, ...)`.
                            objName = resolveEmbeddedMemberName(baseMember);
                        }
                    }
                } else if (auto subDeclRef = llvm::dyn_cast<clang::DeclRefExpr>(subExpr)) {
                    objName = resolveGlobalVar(subDeclRef);
                }
            }
        }
    }

    if (!objName.empty() && !g_currentFunction.empty() &&
        g_reachableFromThreads.count(g_currentFunction) > 0) {
        SourceLocation loc = convertSourceLocation(call->getBeginLoc());
        bool canBlock = canIpcCallBlock(funcName, call);
        for (const auto& owningThread : getOwningThreads(g_currentFunction)) {
            g_ipcCalls.push_back({owningThread, objName, action, loc, canBlock});
        }
    }
}