/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Globals.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/MacroArgs.h"

#include "llvm/Support/raw_ostream.h"

class ZephyrMacroCallback : public clang::PPCallbacks {
public:
    explicit ZephyrMacroCallback(clang::Preprocessor& PP) : PP_(PP) {}

    void MacroExpands(const clang::Token& MacroNameTok,
                      const clang::MacroDefinition&,
                      clang::SourceRange Range,
                      const clang::MacroArgs* Args) override {
        if (!Args) return;

        std::string macroName = MacroNameTok.getIdentifierInfo()->getName().str();
        clang::Preprocessor& PP = PP_;

        if (macroName == "K_THREAD_DEFINE") {
            handleKThreadDefine(Args, Range, PP);
        } else if (macroName == "K_TIMER_DEFINE") {
            handleCallbackDefine(macroName, Args, Range, PP, 1);
        } else if (macroName == "K_WORK_DEFINE") {
            handleCallbackDefine(macroName, Args, Range, PP, 1);
        } else if (macroName == "K_WORK_DELAYABLE_DEFINE" || macroName == "K_DELAYED_WORK_DEFINE") {
            handleCallbackDefine(macroName, Args, Range, PP, 1);
        } else if (macroName == "SYS_INIT" || macroName == "SYS_INIT_NAMED") {
            handleSysInit(Args, Range, PP);
        } else if (macroName == "DEVICE_DT_DEFINE" || macroName == "DEVICE_DT_INST_DEFINE" || macroName == "DEVICE_DEFINE") {
            handleDeviceInit(macroName, Args, Range, PP);
        } else if (macroName == "INPUT_CALLBACK_DEFINE" || macroName == "INPUT_CALLBACK_DEFINE_NAMED") {
            handleInputCallback(Args, Range, PP);
        } else if (macroName == "IRQ_CONNECT" || macroName == "IRQ_CONNECT_DIRECT" ||
                   macroName == "IRQ_DIRECT_CONNECT" ||
                   macroName == "ARCH_IRQ_CONNECT" || macroName == "ARCH_IRQ_DIRECT_CONNECT" ||
                   macroName == "PCIE_IRQ_CONNECT") {
            handleIrqConnect(macroName, Args, Range, PP);
        } else if (macroName == "SHELL_CMD" || macroName == "SHELL_CMD_ARG" ||
                   macroName == "SHELL_CMD_REGISTER") {
            handleShellCommand(Args, Range, PP);
        } else {
            handleIpcDefine(macroName, Args, Range, PP);
        }
    }

private:
    void handleKThreadDefine(const clang::MacroArgs* Args, clang::SourceRange Range, clang::Preprocessor& PP) {
        if (Args->getNumMacroArguments() < 9) return;

        const clang::Token* nameTok = Args->getUnexpArgument(0);
        if (!nameTok || !nameTok->isAnyIdentifier()) return;
        std::string threadName = PP.getSpelling(*nameTok);

        const clang::Token* entryTok = Args->getUnexpArgument(2);
        if (!entryTok || !entryTok->isAnyIdentifier()) return;
        std::string entryFn = PP.getSpelling(*entryTok);

        int priority = 0;
        const clang::Token* prioTok = Args->getUnexpArgument(6);
        if (prioTok) {
            if (prioTok->is(clang::tok::numeric_constant)) {
                std::string prioStr = PP.getSpelling(*prioTok);
                try { priority = std::stoi(prioStr); } catch (...) {}
            }
        }

        g_threadEntryPoints.insert(entryFn);
        g_tidToThread[threadName] = entryFn;

        ThreadInfo info;
        info.name = threadName;
        info.entryFn = entryFn;
        info.priority = priority;

        clang::SourceLocation loc = Range.getBegin();
        if (loc.isValid()) {
            clang::PresumedLoc presumedLoc = PP.getSourceManager().getPresumedLoc(loc);
            if (presumedLoc.isValid()) {
                info.location = SourceLocation(presumedLoc.getFilename(), presumedLoc.getLine(), presumedLoc.getColumn());
                info.sourceFile = presumedLoc.getFilename();
            }
        }

        g_threadInfos[entryFn] = info;

        llvm::errs() << "  K_THREAD_DEFINE: name=" << threadName
                     << " entry=" << entryFn << " prio=" << priority << "\n";
    }

    void handleIpcDefine(const std::string& macroName, const clang::MacroArgs* Args, clang::SourceRange Range, clang::Preprocessor& PP) {
        ObjectType type = objectTypeFromMacro(macroName);
        if (type == ObjectType::Unknown) return;

        if (Args->getNumMacroArguments() < 1) return;

        const clang::Token* nameTok = Args->getUnexpArgument(0);
        if (!nameTok || !nameTok->isAnyIdentifier()) return;
        std::string objName = PP.getSpelling(*nameTok);

        std::string typeStr = IPCObject::objectTypeToString(type);
        g_objectsFromMacros.insert(objName);

        SourceLocation loc;
        std::string sourceFile;
        clang::SourceLocation clangLoc = Range.getBegin();
        if (clangLoc.isValid()) {
            clang::PresumedLoc presumedLoc = PP.getSourceManager().getPresumedLoc(clangLoc);
            if (presumedLoc.isValid()) {
                loc = SourceLocation(presumedLoc.getFilename(), presumedLoc.getLine(), presumedLoc.getColumn());
                sourceFile = presumedLoc.getFilename();
            }
        }

        ObjectInfo oi;
        oi.name = objName;
        oi.typeStr = "struct k_" + typeStr;
        oi.location = loc;
        oi.sourceFile = sourceFile;
        oi.isSystem = false;
        g_objects.push_back(oi);

        llvm::errs() << "  " << macroName << ": name=" << objName << " type=" << typeStr << "\n";
    }

    void handleCallbackDefine(const std::string& macroName, const clang::MacroArgs* Args, [[maybe_unused]] clang::SourceRange Range, clang::Preprocessor& PP, int callbackArgIdx) {
        if (Args->getNumMacroArguments() < (unsigned)(callbackArgIdx + 1)) return;

        const clang::Token* nameTok = Args->getUnexpArgument(0);
        if (!nameTok || !nameTok->isAnyIdentifier()) return;
        std::string objName = PP.getSpelling(*nameTok);

        const clang::Token* cbTok = Args->getUnexpArgument(callbackArgIdx);
        if (!cbTok || !cbTok->isAnyIdentifier()) return;
        std::string cbName = PP.getSpelling(*cbTok);

        g_callbackTargets[objName] = cbName;

        ObjectType type = objectTypeFromMacro(macroName);
        if (type != ObjectType::Unknown) {
            g_objectsFromMacros.insert(objName);
            std::string sourceFileCb;
            clang::SourceLocation clangLocCb = Range.getBegin();
            if (clangLocCb.isValid()) {
                clang::PresumedLoc plCb = PP.getSourceManager().getPresumedLoc(clangLocCb);
                if (plCb.isValid()) sourceFileCb = plCb.getFilename();
            }
            ObjectInfo oi;
            oi.name = objName;
            oi.typeStr = "struct k_" + IPCObject::objectTypeToString(type);
            oi.sourceFile = sourceFileCb;
            oi.isSystem = false;
            g_objects.push_back(oi);
        }

        llvm::errs() << "  " << macroName << ": name=" << objName << " callback=" << cbName << "\n";
    }

    void handleSysInit(const clang::MacroArgs* Args, clang::SourceRange, clang::Preprocessor& PP) {
        if (Args->getNumMacroArguments() < 1) return;

        const clang::Token* fnTok = Args->getUnexpArgument(0);
        if (!fnTok || !fnTok->isAnyIdentifier()) return;

        std::string fnName = PP.getSpelling(*fnTok);
        g_sysInitFunctions.insert(fnName);

        llvm::errs() << "  SYS_INIT: fn=" << fnName << "\n";
    }

    void handleDeviceInit(const std::string& macroName, const clang::MacroArgs* Args, clang::SourceRange, clang::Preprocessor& PP) {
        // DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level, prio, api, ...)
        // DEVICE_DT_INST_DEFINE(inst, init_fn, pm, data, config, level, prio, api, ...)
        // DEVICE_DEFINE(dev_id, name, init_fn, pm, data, config, level, prio, api)
        int initFnArgIdx = (macroName == "DEVICE_DEFINE") ? 2 : 1;

        if (Args->getNumMacroArguments() < (unsigned)(initFnArgIdx + 1)) return;

        const clang::Token* fnTok = Args->getUnexpArgument(initFnArgIdx);
        if (!fnTok) return;

        if (fnTok->is(clang::tok::amp)) {
            ++fnTok;
        }

        if (fnTok->isAnyIdentifier()) {
            std::string fnName = PP.getSpelling(*fnTok);
            g_sysInitFunctions.insert(fnName);
            llvm::errs() << "  " << macroName << ": init_fn=" << fnName << "\n";
        }
    }

    void handleInputCallback(const clang::MacroArgs* Args, clang::SourceRange, clang::Preprocessor& PP) {
        // INPUT_CALLBACK_DEFINE(_dev, _callback, _user_data)
        // INPUT_CALLBACK_DEFINE_NAMED(_dev, _callback, _user_data, _name)
        if (Args->getNumMacroArguments() < 2) return;

        const clang::Token* cbTok = Args->getUnexpArgument(1);
        if (!cbTok || !cbTok->isAnyIdentifier()) return;

        std::string cbName = PP.getSpelling(*cbTok);
        g_inputCallbacks.insert(cbName);

        llvm::errs() << "  INPUT_CALLBACK_DEFINE: callback=" << cbName << "\n";
    }

    void handleIrqConnect(const std::string& macroName, const clang::MacroArgs* Args, clang::SourceRange, clang::Preprocessor& PP) {
        // IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)
        // IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)
        // ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)
        // ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)
        // PCIE_IRQ_CONNECT(bdf_p, irq_p, priority_p, isr_p, isr_param_p, flags_p)
        int isrArgIdx = (macroName == "PCIE_IRQ_CONNECT") ? 3 : 2;

        if (Args->getNumMacroArguments() < (unsigned)(isrArgIdx + 1)) return;

        const clang::Token* isrTok = Args->getUnexpArgument(isrArgIdx);
        if (!isrTok || !isrTok->isAnyIdentifier()) return;

        std::string isrName = PP.getSpelling(*isrTok);
        g_isrFunctions.insert(isrName);

        llvm::errs() << "  " << macroName << ": isr=" << isrName << "\n";
    }

    void handleShellCommand(const clang::MacroArgs* Args, clang::SourceRange, clang::Preprocessor& PP) {
        if (Args->getNumMacroArguments() < 4) return;

        const clang::Token* handlerTok = Args->getUnexpArgument(3);
        if (!handlerTok || !handlerTok->isAnyIdentifier()) return;

        std::string handlerName = PP.getSpelling(*handlerTok);
        if (handlerName == "NULL" || handlerName == "0") return;

        g_shellCommands.insert(handlerName);
        llvm::errs() << "  SHELL_CMD: handler=" << handlerName << "\n";
    }

    static ObjectType objectTypeFromMacro(const std::string& macroName) {
        if (macroName == "K_MUTEX_DEFINE") return ObjectType::Mutex;
        if (macroName == "K_SEM_DEFINE") return ObjectType::Semaphore;
        if (macroName == "K_FIFO_DEFINE") return ObjectType::Fifo;
        if (macroName == "K_LIFO_DEFINE") return ObjectType::Lifo;
        if (macroName == "K_QUEUE_DEFINE") return ObjectType::Queue;
        if (macroName == "K_MSGQ_DEFINE") return ObjectType::Msgq;
        if (macroName == "K_MBOX_DEFINE") return ObjectType::Mbox;
        if (macroName == "K_PIPE_DEFINE") return ObjectType::Pipe;
        if (macroName == "K_STACK_DEFINE") return ObjectType::Stack;
        if (macroName == "K_EVENT_DEFINE") return ObjectType::Event;
        if (macroName == "K_CONDVAR_DEFINE") return ObjectType::Condvar;
        if (macroName == "K_TIMER_DEFINE") return ObjectType::Timer;
        if (macroName == "K_WORK_DEFINE") return ObjectType::Work;
        if (macroName == "K_WORK_DELAYABLE_DEFINE" || macroName == "K_DELAYED_WORK_DEFINE") return ObjectType::Work;
        if (macroName == "K_HEAP_DEFINE") return ObjectType::Heap;
        if (macroName == "K_MEM_SLAB_DEFINE") return ObjectType::MemSlab;
        return ObjectType::Unknown;
    }

    clang::Preprocessor& PP_;
};

// Extract a stable string key identifying a `k_work` argument expression.
// Supports the four forms that appear in real Zephyr code:
//   work                       -> "work"        (rare; passed by value)
//   &work                      -> "work"        (file-static or -global k_work)
//   &inst.queue.work           -> "inst.queue.work"      (struct.field path)
//   &mes->queue.work           -> "mes->queue.work"      (struct->field path)
// The same key is produced by `k_work_init` (callback registration) and
// `k_work_submit_to_queue` (callback dispatch), so the lookup in
// g_callbackTargets matches between the two. Nested struct/array bases
// (e.g. `mes->queue.work`) are walked recursively to produce a dotted
// path. The key is intentionally expression-textual — it need only be
// stable between paired init/submit sites in the same translation unit.
static std::string extractWorkItemName(const clang::Expr* e) {
    if (!e) return {};
    e = e->IgnoreParenImpCasts();
    // Strip leading address-of: &X -> X
    if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(e)) {
        if (unaryOp->getOpcode() == clang::UO_AddrOf) {
            e = unaryOp->getSubExpr()->IgnoreParenImpCasts();
        }
    }
    // Walk MemberExpr chains: outer->inner->leaf, building dotted path.
    std::string path;
    while (auto me = llvm::dyn_cast<clang::MemberExpr>(e)) {
        std::string memberName = me->getMemberDecl()->getNameAsString();
        path = path.empty() ? memberName : (memberName + (me->isArrow() ? "->" : ".") + path);
        e = me->getBase()->IgnoreParenImpCasts();
    }
    // Base case: simple DeclRefExpr.
    if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(e)) {
        std::string base = declRef->getNameInfo().getAsString();
        path = path.empty() ? base : (base + "." + path);
    }
    return path;
}

class Pass1Visitor : public clang::RecursiveASTVisitor<Pass1Visitor> {
public:
    bool VisitVarDecl(clang::VarDecl* decl) {
        if (!decl || !decl->hasInit()) return true;

        clang::Expr* init = decl->getInit()->IgnoreParenImpCasts();
        if (!init) return true;

        clang::CallExpr* call = llvm::dyn_cast<clang::CallExpr>(init);
        if (!call) return true;

        clang::FunctionDecl* callee = call->getDirectCallee();
        if (!callee) return true;

        std::string funcName = callee->getNameAsString();
        if (funcName == "k_thread_create" || funcName == "z_impl_k_thread_create") {
            std::string tidName = decl->getNameAsString();
            if (call->getNumArgs() >= 4) {
                const clang::Expr* entryArg = stripCasts(call->getArg(3));
                if (entryArg) entryArg = entryArg->IgnoreParenImpCasts();
                if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(entryArg)) {
                    if (auto fn = llvm::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
                        std::string entryName = fn->getNameAsString();
                        g_tidToThread[tidName] = entryName;
                    }
                }
            }
        }

        return true;
    }

    bool VisitCallExpr(clang::CallExpr* call) {
        clang::FunctionDecl* callee = call->getDirectCallee();
        if (!callee) return true;

        std::string funcName = callee->getNameAsString();

        if (funcName == "k_thread_create" || funcName == "z_impl_k_thread_create" ||
            funcName == "z_setup_new_thread" || funcName == "z_impl_z_setup_new_thread") {
            if (call->getNumArgs() >= 8) {
                const clang::Expr* arg = stripCasts(call->getArg(3));
                if (arg) arg = arg->IgnoreParenImpCasts();
                if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(arg)) {
                    if (auto fn = llvm::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
                        std::string entryName = fn->getNameAsString();
                        g_threadEntryPoints.insert(entryName);

                        ThreadInfo info;
                        info.name = entryName;
                        info.entryFn = entryName;
                        info.location = convertSourceLocation(call->getBeginLoc());

                        clang::PresumedLoc pLoc = call->getBeginLoc().isValid()
                            ? g_sourceManager->getPresumedLoc(call->getBeginLoc())
                            : clang::PresumedLoc();
                        if (pLoc.isValid())
                            info.sourceFile = pLoc.getFilename();

                        const clang::Expr* priorityArg = stripCasts(call->getArg(7));
                        if (priorityArg) priorityArg = priorityArg->IgnoreParenImpCasts();
                        if (auto intLit = llvm::dyn_cast<clang::IntegerLiteral>(priorityArg)) {
                            info.priority = intLit->getValue().getSExtValue();
                        }

                        g_threadInfos[entryName] = info;
                    }
                }
            }
        }

        if (funcName == "k_work_queue_start" || funcName == "z_impl_k_work_queue_start") {
            if (call->getNumArgs() >= 4) {
                clang::Expr* queueArg = call->getArg(0)->IgnoreParenImpCasts();
                std::string queueName;
                if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(queueArg)) {
                    if (unaryOp->getOpcode() == clang::UO_AddrOf) {
                        clang::Expr* subExpr = unaryOp->getSubExpr()->IgnoreParenImpCasts();
                        if (auto subDeclRef = llvm::dyn_cast<clang::DeclRefExpr>(subExpr)) {
                            queueName = subDeclRef->getNameInfo().getAsString();
                        }
                    }
                } else if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(queueArg)) {
                    queueName = declRef->getNameInfo().getAsString();
                }

                if (!queueName.empty()) {
                    std::string threadEntryFn = queueName + "_thread";
                    g_threadEntryPoints.insert(threadEntryFn);
                    g_workQueueThreads[queueName] = threadEntryFn;

                    ThreadInfo info;
                    info.name = queueName;
                    info.entryFn = threadEntryFn;
                    info.isWorkqueue = true;
                    info.location = convertSourceLocation(call->getBeginLoc());

                    clang::PresumedLoc pLoc = call->getBeginLoc().isValid()
                        ? g_sourceManager->getPresumedLoc(call->getBeginLoc())
                        : clang::PresumedLoc();
                    if (pLoc.isValid())
                        info.sourceFile = pLoc.getFilename();

                    clang::Expr* prioArg = call->getArg(3)->IgnoreParenImpCasts();
                    if (auto intLit = llvm::dyn_cast<clang::IntegerLiteral>(prioArg)) {
                        info.priority = intLit->getValue().getSExtValue();
                    }

                    g_threadInfos[threadEntryFn] = info;

                    llvm::errs() << "  k_work_queue_start: queue=" << queueName
                                 << " thread=" << threadEntryFn << "\n";
                }
            }
        }

        return true;
    }
};

class CallGraphVisitor : public clang::RecursiveASTVisitor<CallGraphVisitor> {
public:
    bool TraverseFunctionDecl(clang::FunctionDecl* decl) {
        if (!decl) return true;
        if (decl->isThisDeclarationADefinition() && decl->hasBody()) {
            g_functionsWithDefinitions.insert(decl->getNameAsString());
            std::string prevFunction = g_currentFunction;
            g_currentFunction = decl->getNameAsString();
            clang::RecursiveASTVisitor<CallGraphVisitor>::TraverseStmt(decl->getBody());
            g_currentFunction = prevFunction;
            return true;
        }
        return clang::RecursiveASTVisitor<CallGraphVisitor>::TraverseFunctionDecl(decl);
    }

    bool VisitCallExpr(clang::CallExpr* call) {
        if (g_currentFunction.empty()) return true;
        clang::FunctionDecl* callee = call->getDirectCallee();
        if (callee) {
            std::string calleeName = callee->getNameAsString();
            g_callGraph[g_currentFunction].insert(calleeName);

            if ((calleeName == "k_timer_start" || calleeName == "z_impl_k_timer_start") && call->getNumArgs() >= 1) {
                clang::Expr* timerArg = call->getArg(0)->IgnoreParenImpCasts();
                std::string timerName;
                if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(timerArg)) {
                    timerName = declRef->getNameInfo().getAsString();
                } else if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(timerArg)) {
                    if (unaryOp->getOpcode() == clang::UO_AddrOf) {
                        clang::Expr* subExpr = unaryOp->getSubExpr()->IgnoreParenImpCasts();
                        if (auto subDeclRef = llvm::dyn_cast<clang::DeclRefExpr>(subExpr)) {
                            timerName = subDeclRef->getNameInfo().getAsString();
                        }
                    }
                }
                if (!timerName.empty()) {
                    auto it = g_callbackTargets.find(timerName);
                    if (it != g_callbackTargets.end()) {
                        g_callGraph[g_currentFunction].insert(it->second);
                    }
                }
            }
            if ((calleeName == "k_work_init" || calleeName == "z_impl_k_work_init") &&
                call->getNumArgs() >= 2) {
                // k_work_init(work, handler) registers the callback function
                // that will later run on the work queue when the work item is
                // submitted. Record work-item-name -> callback-function-name so
                // the eventual k_work_submit[_to_queue] visitor can resolve the
                // handler and attribute it to the queue thread.
                std::string workName = extractWorkItemName(call->getArg(0));
                clang::Expr* handlerArg = call->getArg(1)->IgnoreParenImpCasts();
                std::string handlerName;
                if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(handlerArg)) {
                    if (auto fn = llvm::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
                        handlerName = fn->getNameAsString();
                    }
                } else if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(handlerArg)) {
                    // &handler — same form used by some macro expansions.
                    if (unaryOp->getOpcode() == clang::UO_AddrOf) {
                        clang::Expr* subExpr = unaryOp->getSubExpr()->IgnoreParenImpCasts();
                        if (auto subDeclRef = llvm::dyn_cast<clang::DeclRefExpr>(subExpr)) {
                            if (auto fn = llvm::dyn_cast<clang::FunctionDecl>(subDeclRef->getDecl())) {
                                handlerName = fn->getNameAsString();
                            }
                        }
                    }
                }
                if (!workName.empty() && !handlerName.empty()) {
                    g_callbackTargets[workName] = handlerName;
                    llvm::errs() << "  k_work_init: work=" << workName
                                 << " handler=" << handlerName << "\n";
                }
            }
            if ((calleeName == "k_work_submit" || calleeName == "z_impl_k_work_submit" ||
                 calleeName == "k_work_schedule" || calleeName == "z_impl_k_work_schedule" ||
                 calleeName == "k_work_reschedule" || calleeName == "z_impl_k_work_reschedule") && call->getNumArgs() >= 1) {
                std::string workName = extractWorkItemName(call->getArg(0));
                if (!workName.empty()) {
                    auto it = g_callbackTargets.find(workName);
                    if (it != g_callbackTargets.end()) {
                        g_callGraph[g_currentFunction].insert(it->second);
                        // k_work_submit goes to the system work queue
                        g_workQueueCallbacks["_system_work_q"].insert(it->second);
                    }
                }
            }
            if ((calleeName == "k_work_submit_to_queue" || calleeName == "z_impl_k_work_submit_to_queue" ||
                 calleeName == "k_work_schedule_for_queue" || calleeName == "z_impl_k_work_schedule_for_queue" ||
                 calleeName == "k_work_reschedule_for_queue" || calleeName == "z_impl_k_work_reschedule_for_queue") && call->getNumArgs() >= 2) {
                // Extract queue name from arg 0
                clang::Expr* queueArg = call->getArg(0)->IgnoreParenImpCasts();
                std::string queueName;
                if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(queueArg)) {
                    if (unaryOp->getOpcode() == clang::UO_AddrOf) {
                        clang::Expr* subExpr = unaryOp->getSubExpr()->IgnoreParenImpCasts();
                        if (auto subDeclRef = llvm::dyn_cast<clang::DeclRefExpr>(subExpr)) {
                            queueName = subDeclRef->getNameInfo().getAsString();
                        }
                    }
                } else if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(queueArg)) {
                    queueName = declRef->getNameInfo().getAsString();
                }

                // Extract work item name from arg 1 using the shared helper
                // (supports &struct->field.work member chains).
                std::string workName = extractWorkItemName(call->getArg(1));
                if (!workName.empty()) {
                    auto it = g_callbackTargets.find(workName);
                    if (it != g_callbackTargets.end()) {
                        g_callGraph[g_currentFunction].insert(it->second);
                        if (!queueName.empty()) {
                            g_workQueueCallbacks[queueName].insert(it->second);
                        }
                    }
                }
            }
        } else {
            clang::Expr* calleeExpr = call->getCallee()->IgnoreParenImpCasts();
            if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(calleeExpr)) {
                if (auto fn = llvm::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
                    std::string calleeName = fn->getNameAsString();
                    g_callGraph[g_currentFunction].insert(calleeName);
                }
            } else if (auto memberExpr = llvm::dyn_cast<clang::MemberExpr>(calleeExpr)) {
                std::string memberName = memberExpr->getMemberDecl()->getNameAsString();
                if (g_ipcFunctions.count(stripZImpl(memberName)) > 0) {
                    g_callGraph[g_currentFunction].insert(memberName);
                } else {
                    g_unresolvedCalls[g_currentFunction].insert(memberName);
                }
            } else {
                std::string exprStr;
                llvm::raw_string_ostream os(exprStr);
                call->getCallee()->printPretty(os, nullptr, clang::PrintingPolicy(clang::LangOptions()));
                g_unresolvedCalls[g_currentFunction].insert(os.str());
            }
        }

        for (unsigned i = 0; i < call->getNumArgs(); ++i) {
            clang::Expr* arg = call->getArg(i)->IgnoreParenImpCasts();
            if (auto unaryOp = llvm::dyn_cast<clang::UnaryOperator>(arg)) {
                if (unaryOp->getOpcode() == clang::UO_AddrOf) {
                    arg = unaryOp->getSubExpr()->IgnoreParenImpCasts();
                }
            }
            if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(arg)) {
                if (auto fn = llvm::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
                    std::string fnName = fn->getNameAsString();
                    if (g_threadEntryPoints.count(fnName) == 0 &&
                        g_ipcFunctions.count(stripZImpl(fnName)) == 0 &&
                        !isZephyrKernelInternal(fnName)) {
                        g_callbackFunctions.insert(fnName);

                        bool isIsrRegistration = false;
                        if (callee) {
                            std::string calleeName = stripZImpl(callee->getNameAsString());
                            if (g_isrRegistrationFunctions.count(calleeName) > 0) {
                                isIsrRegistration = true;
                            } else if (callee->isInlineSpecified() &&
                                       callee->getStorageClass() == clang::SC_Static &&
                                       g_nonIsrInlineFunctions.count(calleeName) == 0) {
                                isIsrRegistration = true;
                            }
                        } else {
                            isIsrRegistration = true;
                        }

                        if (isIsrRegistration) {
                            g_callbackRegistrations[fnName].insert(g_currentFunction);
                        }
                    }
                }
            }
        }

        return true;
    }
};

static std::string classifyGlobalAccess(const clang::DeclRefExpr* declRef) {
    if (!g_astContext) return "readwrite";

    const clang::Stmt* current = declRef;
    bool seenAddrOf = false;

    while (true) {
        auto parents = g_astContext->getParents(*current);
        if (parents.begin() == parents.end()) break;

        const auto& parent = *parents.begin();

        if (parent.get<clang::ImplicitCastExpr>() ||
            parent.get<clang::ParenExpr>() ||
            parent.get<clang::FullExpr>() ||
            parent.get<clang::MaterializeTemporaryExpr>()) {
            const auto* stmt = parent.get<clang::Stmt>();
            if (!stmt) break;
            current = stmt;
            continue;
        }

        if (auto* me = parent.get<clang::MemberExpr>()) {
            if (me->getBase() == current) {
                if (me->isArrow()) {
                    return "read";
                }
                current = me;
                continue;
            }
            return "read";
        }

        if (auto* as = parent.get<clang::ArraySubscriptExpr>()) {
            if (as->getBase() == current) {
                current = as;
                continue;
            }
            return "read";
        }

        if (auto* bo = parent.get<clang::BinaryOperator>()) {
            if (bo->getLHS() == current) {
                if (bo->isAssignmentOp()) {
                    if (bo->isCompoundAssignmentOp()) {
                        return "readwrite";
                    }
                    return "write";
                }
                return "read";
            }
            return "read";
        }

        if (auto* uo = parent.get<clang::UnaryOperator>()) {
            auto op = uo->getOpcode();
            if (op == clang::UO_PostInc || op == clang::UO_PostDec ||
                op == clang::UO_PreInc || op == clang::UO_PreDec) {
                return "write";
            }
            if (op == clang::UO_AddrOf) {
                seenAddrOf = true;
                current = uo;
                continue;
            }
            return "read";
        }

        if (parent.get<clang::CallExpr>()) {
            if (seenAddrOf) {
                return "readwrite";
            }
            return "read";
        }

        if (parent.get<clang::ReturnStmt>() ||
            parent.get<clang::InitListExpr>() ||
            parent.get<clang::ConditionalOperator>()) {
            return "read";
        }

        if (seenAddrOf) return "readwrite";
        return "read";
    }

    if (seenAddrOf) return "readwrite";
    return "read";
}

// Classify whether a QualType is a primitive scalar: bool, char, short,
// int, long, float, double (and unsigned variants), plus the fixed-width
// stdint aliases (uint8_t/int8_t..uint64_t/int64_t, size_t, ssize_t,
// uintptr_t, intptr_t, ptrdiff_t) when only the typedef name is visible.
// Enumerations are included: an enum value is implemented as an int and
// atomically updated in a single store; crash-corruption of an enum
// variable can't tear. Pointer-typed globals are excluded (a corrupted
// pointer carries an address-of-shared-state hazard that goes beyond a
// simple primitive store).
static bool isPrimitiveScalarType(const clang::QualType& qt) {
    if (qt.isNull()) return false;
    const clang::Type* t = qt.getTypePtrOrNull();
    if (!t) return false;
    // Peel typedefs/elaborated/qualified wrappers.
    t = t->getUnqualifiedDesugaredType();
    if (!t) return false;

    // Builtin scalar types: bool/char/short/int/long/float/double and
    // their unsigned counterparts are Clang builtin types. This is the
    // primary catch for `unsigned int counter` / `uint8_t idx` / etc.
    if (t->isBuiltinType()) {
        // Exclude nullptr_t and complex; anything else builtin here is
        // one of the above scalars.
        if (t->isNullPtrType()) return false;
        if (t->isComplexType()) return false;
        return true;
    }
    // Enumerations: implemented as int (or smaller). Single-store atomic.
    if (t->isEnumeralType()) return true;
    // typedef names that survive into the type's spelling even after
    // desugaring? They don't (desugaring flattens to the underlying
    // type), so the BuiltinType branch above already covers uint8_t etc.
    // However, some platforms wrap fixed-width types in another typedef
    // that desugars to `unsigned int`/`unsigned long`, which isBuiltin
    // also handles. Pointer-typed globals are excluded here.
    return false;
}

class Pass2Visitor : public clang::RecursiveASTVisitor<Pass2Visitor> {
public:
    bool VisitVarDecl(clang::VarDecl* decl) {
        if (!decl->hasGlobalStorage()) return true;

        std::string name = decl->getNameAsString();

        if (g_objectsFromMacros.count(name) > 0) {
            // Macro-created object (K_MUTEX_DEFINE etc.). The pass-1 macro
            // callback already registered the bare name, which is the
            // correct identity for external linkage. For internal linkage
            // (`static K_MUTEX_DEFINE(...)`) the bare name would merge with
            // every other same-named macro static in the project (Zephyr's
            // net subsystem has one `static K_MUTEX_DEFINE(lock)` per TU),
            // so fall through and register the file-qualified name here.
            // SC_Static (not !hasExternalStorage()) because a plain
            // external definition has storage class SC_None.
            const clang::VarDecl* canonical =
                llvm::dyn_cast_or_null<clang::VarDecl>(decl->getCanonicalDecl());
            if (!canonical) canonical = decl;
            bool internalLinkage = canonical->getDeclContext() &&
                                   canonical->getDeclContext()->isFileContext() &&
                                   canonical->getStorageClass() == clang::SC_Static;
            if (!internalLinkage) return true;
        }

        std::string typeStr = decl->getType().getAsString();

        ObjectType type = IPCObject::parseObjectType(typeStr);
        if (type != ObjectType::Unknown) {
            SourceLocation loc = convertSourceLocation(decl->getLocation());
            std::string sourceFile;
            if (decl->getLocation().isValid() && g_sourceManager) {
                clang::PresumedLoc pLoc = g_sourceManager->getPresumedLoc(decl->getLocation());
                if (pLoc.isValid()) sourceFile = pLoc.getFilename();
            }
            ObjectInfo oi;
            // Linkage-qualified name: file-scope `static` kernel objects
            // register as `name@file.c` so same-named statics in different
            // TUs (e.g. `static struct k_mutex mutex` in zephyr fs.c and
            // disk_access.c) stay distinct. Must stay in sync with
            // checkIpcOperation's resolution (qualifiedObjectVarName).
            oi.name = qualifiedObjectVarName(decl);
            oi.typeStr = typeStr;
            oi.location = loc;
            oi.sourceFile = sourceFile;
            oi.isSystem = false;
            g_objects.push_back(oi);
        }

        return true;
    }

    // Discover IPC objects embedded as fields of struct/union records.
    // For a record like:
    //   struct app_ctx { struct k_mutex lock; struct k_sem sem; ... };
    // we register each kernel-typed field. Later, checkIpcOperation() resolves
    // access patterns like `k_mutex_lock(&ctx->lock, ...)` against the synthetic
    // object name "<RecordType>::<fieldName>" (e.g. "app_ctx::lock").
    bool VisitRecordDecl(clang::RecordDecl* decl) {
        if (!decl) return true;
        if (!decl->isThisDeclarationADefinition()) return true;
        if (!decl->getIdentifier()) return true;
        if (g_sourceManager && g_sourceManager->isInSystemHeader(decl->getLocation())) return true;

        std::string structName = decl->getNameAsString();
        if (structName.empty()) return true;
        if (structName.rfind("k_", 0) == 0 || structName.rfind("z_", 0) == 0) return true;

        for (auto* field : decl->fields()) {
            if (!field) continue;
            std::string fieldStr = field->getNameAsString();
            if (fieldStr.empty()) continue;
            std::string typeStr = field->getType().getAsString();
            ObjectType ft = IPCObject::parseObjectType(typeStr);
            if (ft == ObjectType::Unknown) {
                // Handle pointers to kernel objects (e.g. `struct k_mutex *lock`).
                // parseObjectType expects the bare struct type, so desugar and peel.
                const clang::Type* t = field->getType()->getUnqualifiedDesugaredType();
                if (auto* pt = llvm::dyn_cast_or_null<clang::PointerType>(t)) {
                    clang::QualType pointee = pt->getPointeeType();
                    std::string ptStr = pointee.getAsString();
                    ft = IPCObject::parseObjectType(ptStr);
                }
            }
            if (ft == ObjectType::Unknown) continue;

            g_structFieldObjects[structName][fieldStr] = ft;
            g_structsWithIpcFields.insert(structName);

            // Register a synthetic ObjectInfo so main.cpp's
            // analysisResult.findObject(name) lookup succeeds later and keeps
            // the interaction in the output.
            std::string objName = structName + "::" + fieldStr;
            if (g_objectsFromMacros.count(objName) == 0) {
                SourceLocation loc = convertSourceLocation(field->getLocation());
                std::string sourceFile;
                if (field->getLocation().isValid() && g_sourceManager) {
                    clang::PresumedLoc pLoc = g_sourceManager->getPresumedLoc(field->getLocation());
                    if (pLoc.isValid()) sourceFile = pLoc.getFilename();
                }
                ObjectInfo oi;
                oi.name = objName;
                oi.typeStr = typeStr;
                oi.location = loc;
                oi.sourceFile = sourceFile;
                oi.isSystem = false;
                g_objects.push_back(oi);
            }
        }

        return true;
    }

    bool TraverseFunctionDecl(clang::FunctionDecl* decl) {
        if (!decl) return true;

        if (decl->isThisDeclarationADefinition() && decl->hasBody()) {
            std::string fnName = decl->getNameAsString();

            if (g_threadEntryPoints.count(fnName) > 0 ||
                g_reachableFromThreads.count(fnName) > 0) {
                std::string prevFunction = g_currentFunction;
                g_currentFunction = fnName;
                clang::RecursiveASTVisitor<Pass2Visitor>::TraverseStmt(decl->getBody());
                g_currentFunction = prevFunction;
                return true;
            }
        }

        return clang::RecursiveASTVisitor<Pass2Visitor>::TraverseFunctionDecl(decl);
    }

    bool VisitCallExpr(clang::CallExpr* call) {
        clang::FunctionDecl* callee = call->getDirectCallee();
        if (!callee) return true;

        std::string funcName = callee->getNameAsString();
        std::string canonicalName = stripZImpl(funcName);

        if (g_ipcFunctions.count(canonicalName) > 0) {
            ActionType action = inferActionType(canonicalName);
            if (checkThreadToThreadCall(call, canonicalName, action)) {
            } else {
                checkIpcOperation(call, action, canonicalName);
            }
        }

        return true;
    }

    bool VisitRecoveryExpr(clang::RecoveryExpr* recovery) {
        if (!recovery) return true;

        std::string canonicalName;
        for (auto* child : recovery->children()) {
            if (!child) continue;
            if (auto declRef = llvm::dyn_cast<clang::DeclRefExpr>(child)) {
                if (llvm::isa<clang::FunctionDecl>(declRef->getDecl())) {
                    canonicalName = stripZImpl(declRef->getNameInfo().getAsString());
                    break;
                }
            }
        }

        if (!canonicalName.empty() && g_ipcFunctions.count(canonicalName) > 0) {
            ActionType action = inferActionType(canonicalName);
            processRecoveryExpr(recovery, action);
        }

        return true;
    }

    bool VisitDeclRefExpr(clang::DeclRefExpr* declRef) {
        if (g_currentFunction.empty() ||
            g_reachableFromThreads.count(g_currentFunction) == 0) {
            return true;
        }

        if (auto var = llvm::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
            if (!var->hasGlobalStorage()) return true;

            // Reject function-scope `static` locals: Clang's hasGlobalStorage() is
            // true for them, but each is private to its declaring function. Without
            // this filter, every `static int i;` inside a Zephyr kernel function
            // produces a spurious "shared memory" coupling across every thread that
            // happens to have a same-named static in any other file.
            if (!var->getDeclContext()->isFileContext() && !var->hasExternalStorage()) return true;

            if (g_sourceManager && g_sourceManager->isInSystemHeader(var->getLocation())) return true;

            if (var->getType().isConstQualified()) return true;

            std::string varName = var->getNameAsString();
            std::string typeStr = var->getType().getAsString();

            // Internal-linkage (file-scope `static`) globals are private to
            // their declaring translation unit by the C standard. Multiple
            // TUs in real Zephyr apps routinely declare disjoint globals
            // under the same short name (e.g. akiraos has 12 NRFS service
            // files each declaring `static nrfs_X_cb_t m_cb;`, and 8 files
            // declaring `static <something> state;`). Recording accesses
            // by bare name collapses all of these into one fake-shared
            // symbol and produces an artificial giant SCC of every thread
            // that uses any NRFS service / any of the `state` globals.
            //
            // Distinguish internal-linkage globals from external-linkage
            // globals by qualifying the recorded name with the source
            // file basename (`m_cb@nrfs_diag.c` vs `m_cb@nrfs_clock.c`).
            // External-linkage globals (the `extern` case) keep the bare
            // name (`csp_dbg_errno`) because by definition there is one
            // symbol across the project.
            //
            // Detection: `isFileContext()` is true (filter above), and the
            // canonical declaration's storage class is SC_Static. Note
            // SC_Static rather than !hasExternalStorage(): a plain external
            // definition (storage class SC_None, no explicit `extern`)
            // also fails hasExternalStorage() but is externally linked.
            if (var->getDeclContext()->isFileContext() &&
                var->getCanonicalDecl()->getStorageClass() == clang::SC_Static) {
                std::string sourceFile;
                if (var->getLocation().isValid() && g_sourceManager) {
                    clang::PresumedLoc pLoc = g_sourceManager->getPresumedLoc(var->getLocation());
                    if (pLoc.isValid()) {
                        sourceFile = pLoc.getFilename();
                    }
                }
                if (!sourceFile.empty()) {
                    // Reduce to basename for compactness and stable across
                    // full-path changes (e.g. when the project is checked
                    // out in a different location).
                    size_t slash = sourceFile.find_last_of("/\\");
                    if (slash != std::string::npos) {
                        sourceFile = sourceFile.substr(slash + 1);
                    }
                    varName = varName + "@" + sourceFile;
                }
            }

            auto* concreteVar = var->getCanonicalDecl();
            if (llvm::isa<clang::FunctionDecl>(concreteVar)) return true;
            if (llvm::isa<clang::EnumConstantDecl>(concreteVar)) return true;

            if (varName.empty()) return true;

            ObjectType varType = IPCObject::parseObjectType(typeStr);
            if (varType != ObjectType::Unknown) {
                return true;
            }

            // If the variable's record type hosts discovered embedded IPC
            // fields (e.g. `struct espi_dev { ...; struct k_mutex lock; }`),
            // access to the container variable is handled by checkIpcOperation
            // via the synthetic `Type::member` object, not as a shared-memory
            // edge. Otherwise every thread holding a `struct espi_dev *`
            // would be HIGH-coupled through the `espi_dev` global alone —
            // masking the actual per-field interactions.
            const clang::RecordDecl* rd = baseTypeAsRecordDecl(var->getType());
            if (rd && rd->getIdentifier()) {
                std::string typeName = rd->getNameAsString();
                if (g_structsWithIpcFields.count(typeName) > 0) {
                    return true;
                }
            }

            bool isVolatile = var->getType().isVolatileQualified();
            bool isAtomic = (typeStr.find("atomic_t") != std::string::npos ||
                             typeStr.find("atomic_val_t") != std::string::npos ||
                             typeStr.find("atomic_ptr_t") != std::string::npos ||
                             typeStr.find("atomic") != std::string::npos);
            bool isConst = var->getType().isConstQualified();
            bool isPrimitiveScalar = isPrimitiveScalarType(var->getType());

            std::string accessKindStr = classifyGlobalAccess(declRef);

            for (const auto& owningThread : getOwningThreads(g_currentFunction)) {
                auto& threadAccesses = g_globalVarAccesses[varName];
                auto it = threadAccesses.find(owningThread);
                if (it == threadAccesses.end()) {
                    GlobalVarAccess access;
                    access.varName = varName;
                    access.threadName = owningThread;
                    access.accessType = accessKindStr;
                    access.isAtomic = isAtomic;
                    access.isVolatile = isVolatile;
                    access.isConst = isConst;
                    access.isPrimitiveScalar = isPrimitiveScalar;
                    access.location = convertSourceLocation(declRef->getBeginLoc());
                    threadAccesses[owningThread] = access;
                } else {
                    if (it->second.accessType != accessKindStr) {
                        it->second.accessType = "readwrite";
                    }
                    if (isAtomic) it->second.isAtomic = true;
                    if (isVolatile) it->second.isVolatile = true;
                    if (isConst) it->second.isConst = true;
                    if (isPrimitiveScalar) it->second.isPrimitiveScalar = true;
                }
            }

            g_globalVarNames.insert(varName);
        }

        return true;
    }
};

class Pass1Consumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext& context) override {
        g_sourceManager = &context.getSourceManager();
        Pass1Visitor visitor;
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
};

class Pass2Consumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext& context) override {
        g_sourceManager = &context.getSourceManager();

        g_currentFunction.clear();

        CallGraphVisitor cgVisitor;
        cgVisitor.TraverseDecl(context.getTranslationUnitDecl());
    }
};

class Pass3Consumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext& context) override {
        g_sourceManager = &context.getSourceManager();
        g_astContext = &context;

        g_currentFunction.clear();
        Pass2Visitor visitor;
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
};

class Pass1Action : public clang::ASTFrontendAction {
public:
    bool BeginSourceFileAction(clang::CompilerInstance& CI) override {
        CI.getPreprocessor().addPPCallbacks(
            std::make_unique<ZephyrMacroCallback>(CI.getPreprocessor()));
        return true;
    }

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& compiler,
        llvm::StringRef inFile) override {
        (void)(compiler);
        (void)inFile;
        return std::make_unique<Pass1Consumer>();
    }
};

class Pass2Action : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& compiler,
        llvm::StringRef inFile) override {
        (void)compiler;
        (void)inFile;
        return std::make_unique<Pass2Consumer>();
    }
};

class Pass3Action : public clang::ASTFrontendAction {
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& compiler,
        llvm::StringRef inFile) override {
        (void)compiler;
        (void)inFile;
        return std::make_unique<Pass3Consumer>();
    }
};

#include "clang/Tooling/Tooling.h"

std::unique_ptr<clang::tooling::FrontendActionFactory> createPass1ActionFactory() {
    return clang::tooling::newFrontendActionFactory<Pass1Action>();
}

std::unique_ptr<clang::tooling::FrontendActionFactory> createPass2ActionFactory() {
    return clang::tooling::newFrontendActionFactory<Pass2Action>();
}

std::unique_ptr<clang::tooling::FrontendActionFactory> createPass3ActionFactory() {
    return clang::tooling::newFrontendActionFactory<Pass3Action>();
}