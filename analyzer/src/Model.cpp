/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Model.h"
#include <cctype>

std::string IPCObject::typeString() const {
    return objectTypeToString(type);
}

ObjectType IPCObject::parseObjectType(const std::string& typeStr) {
    if (typeStr == "k_mutex" || typeStr == "struct k_mutex") return ObjectType::Mutex;
    if (typeStr == "k_sem" || typeStr == "struct k_sem") return ObjectType::Semaphore;
    if (typeStr == "k_fifo" || typeStr == "struct k_fifo") return ObjectType::Fifo;
    if (typeStr == "k_lifo" || typeStr == "struct k_lifo") return ObjectType::Lifo;
    if (typeStr == "k_queue" || typeStr == "struct k_queue") return ObjectType::Queue;
    if (typeStr == "k_msgq" || typeStr == "struct k_msgq") return ObjectType::Msgq;
    if (typeStr == "k_mbox" || typeStr == "struct k_mbox") return ObjectType::Mbox;
    if (typeStr == "k_pipe" || typeStr == "struct k_pipe") return ObjectType::Pipe;
    if (typeStr == "k_stack" || typeStr == "struct k_stack") return ObjectType::Stack;
    if (typeStr == "k_event" || typeStr == "struct k_event") return ObjectType::Event;
    if (typeStr == "k_condvar" || typeStr == "struct k_condvar") return ObjectType::Condvar;
    if (typeStr == "k_timer" || typeStr == "struct k_timer") return ObjectType::Timer;
    if (typeStr == "k_heap" || typeStr == "struct k_heap") return ObjectType::Heap;
    if (typeStr == "k_mem_slab" || typeStr == "struct k_mem_slab") return ObjectType::MemSlab;
    if (typeStr == "k_poll_signal" || typeStr == "struct k_poll_signal") return ObjectType::PollSignal;
    if (typeStr == "k_work" || typeStr == "struct k_work") return ObjectType::Work;
    return ObjectType::Unknown;
}

std::string IPCObject::objectTypeToString(ObjectType t) {
    switch (t) {
        case ObjectType::Mutex: return "mutex";
        case ObjectType::Semaphore: return "sem";
        case ObjectType::Fifo: return "fifo";
        case ObjectType::Lifo: return "lifo";
        case ObjectType::Queue: return "queue";
        case ObjectType::Msgq: return "msgq";
        case ObjectType::Mbox: return "mbox";
        case ObjectType::Pipe: return "pipe";
        case ObjectType::Stack: return "stack";
        case ObjectType::Event: return "event";
        case ObjectType::Condvar: return "condvar";
        case ObjectType::Timer: return "timer";
        case ObjectType::Heap: return "heap";
        case ObjectType::MemSlab: return "mem_slab";
        case ObjectType::PollSignal: return "poll_signal";
        case ObjectType::Work: return "work";
        case ObjectType::GlobalVar: return "global_var";
        default: return "unknown";
    }
}

std::string Interaction::actionString() const {
    switch (action) {
        case ActionType::Lock: return "lock";
        case ActionType::Unlock: return "unlock";
        case ActionType::Take: return "take";
        case ActionType::Give: return "give";
        case ActionType::Put: return "put";
        case ActionType::Get: return "get";
        case ActionType::Push: return "push";
        case ActionType::Pop: return "pop";
        case ActionType::Send: return "send";
        case ActionType::Receive: return "receive";
        case ActionType::Post: return "post";
        case ActionType::Wait: return "wait";
        case ActionType::Signal: return "signal";
        case ActionType::Broadcast: return "broadcast";
        case ActionType::Alloc: return "alloc";
        case ActionType::Free: return "free";
        case ActionType::Start: return "start";
        case ActionType::Stop: return "stop";
        case ActionType::Raise: return "raise";
        case ActionType::Reset: return "reset";
        case ActionType::Read: return "read";
        case ActionType::Write: return "write";
        case ActionType::Join: return "join";
        case ActionType::Abort: return "abort";
        case ActionType::Wakeup: return "wakeup";
        case ActionType::Submit: return "submit";
        case ActionType::Cancel: return "cancel";
        default: return "unknown";
    }
}

ActionType Interaction::parseAction(const std::string& actionStr) {
    if (actionStr == "lock") return ActionType::Lock;
    if (actionStr == "unlock") return ActionType::Unlock;
    if (actionStr == "take") return ActionType::Take;
    if (actionStr == "give") return ActionType::Give;
    if (actionStr == "put") return ActionType::Put;
    if (actionStr == "get") return ActionType::Get;
    if (actionStr == "push") return ActionType::Push;
    if (actionStr == "pop") return ActionType::Pop;
    if (actionStr == "send") return ActionType::Send;
    if (actionStr == "receive") return ActionType::Receive;
    if (actionStr == "post") return ActionType::Post;
    if (actionStr == "wait") return ActionType::Wait;
    if (actionStr == "signal") return ActionType::Signal;
    if (actionStr == "broadcast") return ActionType::Broadcast;
    if (actionStr == "alloc") return ActionType::Alloc;
    if (actionStr == "free") return ActionType::Free;
    if (actionStr == "start") return ActionType::Start;
    if (actionStr == "stop") return ActionType::Stop;
    if (actionStr == "raise") return ActionType::Raise;
    if (actionStr == "reset") return ActionType::Reset;
    if (actionStr == "read") return ActionType::Read;
    if (actionStr == "write") return ActionType::Write;
    if (actionStr == "join") return ActionType::Join;
    if (actionStr == "abort") return ActionType::Abort;
    if (actionStr == "wakeup") return ActionType::Wakeup;
    return ActionType::Unknown;
}

EdgeType ThreadEdge::inferEdgeType(const IPCObject& obj, const std::vector<Interaction>& interactions) {
    [[maybe_unused]] bool hasLock = false, hasUnlock = false;
    [[maybe_unused]] bool hasTake = false, hasGive = false;
    [[maybe_unused]] bool hasPut = false, hasGet = false;
    [[maybe_unused]] bool hasPost = false, hasWait = false;
    [[maybe_unused]] bool hasSignal = false, hasBroadcast = false;
    [[maybe_unused]] bool hasPush = false, hasPop = false;
    [[maybe_unused]] bool hasStart = false, hasStop = false;
    [[maybe_unused]] bool hasRaise = false, hasReset = false;
    [[maybe_unused]] bool hasAlloc = false, hasFree = false;
    [[maybe_unused]] bool hasSend = false, hasReceive = false;
    [[maybe_unused]] bool hasSubmit = false;
    [[maybe_unused]] bool hasCancel = false;

    for (const auto& i : interactions) {
        switch (i.action) {
            case ActionType::Lock: hasLock = true; break;
            case ActionType::Unlock: hasUnlock = true; break;
            case ActionType::Take: hasTake = true; break;
            case ActionType::Give: hasGive = true; break;
            case ActionType::Put: hasPut = true; break;
            case ActionType::Get: hasGet = true; break;
            case ActionType::Post: hasPost = true; break;
            case ActionType::Wait: hasWait = true; break;
            case ActionType::Signal: hasSignal = true; break;
            case ActionType::Broadcast: hasBroadcast = true; break;
            case ActionType::Push: hasPush = true; break;
            case ActionType::Pop: hasPop = true; break;
            case ActionType::Start: hasStart = true; break;
            case ActionType::Stop: hasStop = true; break;
            case ActionType::Raise: hasRaise = true; break;
            case ActionType::Reset: hasReset = true; break;
            case ActionType::Alloc: hasAlloc = true; break;
            case ActionType::Free: hasFree = true; break;
            case ActionType::Send: hasSend = true; break;
            case ActionType::Receive: hasReceive = true; break;
            case ActionType::Submit: hasSubmit = true; break;
            case ActionType::Cancel: hasCancel = true; break;
            default: break;
        }
    }

    switch (obj.type) {
        case ObjectType::Mutex:
            return EdgeType::MutualExclusion;
        case ObjectType::Semaphore:
            if (hasGive && hasTake) return EdgeType::Bidirectional;
            if (hasGive) return EdgeType::Signals;
            return EdgeType::ConsumesFrom;
        case ObjectType::Fifo:
        case ObjectType::Lifo:
        case ObjectType::Queue:
        case ObjectType::Msgq:
            if (hasPut) return EdgeType::ProducesFor;
            return EdgeType::ConsumesFrom;
        case ObjectType::Mbox:
            if (hasSend) return EdgeType::ProducesFor;
            if (hasReceive) return EdgeType::ConsumesFrom;
            return EdgeType::Bidirectional;
        case ObjectType::Pipe:
            if (hasPut) return EdgeType::ProducesFor;
            if (hasGet) return EdgeType::ConsumesFrom;
            return EdgeType::Bidirectional;
        case ObjectType::Event:
            if (hasPost) return EdgeType::Signals;
            return EdgeType::ConsumesFrom;
        case ObjectType::Condvar:
            if (hasSignal || hasBroadcast) return EdgeType::Signals;
            return EdgeType::ConsumesFrom;
        case ObjectType::Stack:
            if (hasPush) return EdgeType::ProducesFor;
            return EdgeType::ConsumesFrom;
        case ObjectType::Timer:
            if (hasStart) return EdgeType::Signals;
            return EdgeType::Unknown;
        case ObjectType::Work:
            if (hasSubmit) return EdgeType::Signals;
            return EdgeType::Unknown;
        case ObjectType::Heap:
            // Heap objects model a shared mutable resource, not a
            // producer/consumer data dependency. A thread crashing
            // mid-allocation/free can corrupt heap metadata, and safe
            // recovery requires reinitialising the heap, which
            // invalidates every live allocation pointer held by any
            // thread that has ever touched the heap — even threads that
            // only allocated once at boot must restart so they re-derive
            // their pointers from the fresh heap. So heap-using threads
            // are all-or-nothing coupling (a clique), not directional.
            // Returning SharedMemory here preserves the alloc/free
            // action in the evidence interactions while preventing
            // Misleading "produces_for"/"consumes_from" edge labels.
            return EdgeType::SharedMemory;
        case ObjectType::MemSlab:
            if (hasFree) return EdgeType::ProducesFor;
            if (hasAlloc) return EdgeType::ConsumesFrom;
            return EdgeType::Unknown;
        case ObjectType::PollSignal:
            if (hasRaise) return EdgeType::Signals;
            return EdgeType::Unknown;
        default:
            return EdgeType::Unknown;
    }
}

EdgeType ThreadEdge::inferDirectionalEdgeType(const IPCObject& obj, const std::vector<Interaction>& fromThreadInteractions) {
    bool hasTake = false, hasGive = false;
    bool hasPut = false, hasGet = false;
    bool hasPost = false, hasWait = false;
    bool hasSignal = false, hasBroadcast = false;
    bool hasPush = false, hasPop = false;
    bool hasStart = false;
    bool hasRaise = false;
    bool hasAlloc = false, hasFree = false;
    bool hasSend = false, hasReceive = false;
bool hasSubmit = false;
    [[maybe_unused]] bool hasCancel = false;
    [[maybe_unused]] bool hasLock = false;
    [[maybe_unused]] bool hasUnlock = false;
    [[maybe_unused]] bool hasStop = false;
    [[maybe_unused]] bool hasReset = false;

    for (const auto& i : fromThreadInteractions) {
        switch (i.action) {
            case ActionType::Lock: hasLock = true; break;
            case ActionType::Unlock: hasUnlock = true; break;
            case ActionType::Take: hasTake = true; break;
            case ActionType::Give: hasGive = true; break;
            case ActionType::Put: hasPut = true; break;
            case ActionType::Get: hasGet = true; break;
            case ActionType::Post: hasPost = true; break;
            case ActionType::Wait: hasWait = true; break;
            case ActionType::Signal: hasSignal = true; break;
            case ActionType::Broadcast: hasBroadcast = true; break;
            case ActionType::Push: hasPush = true; break;
            case ActionType::Pop: hasPop = true; break;
            case ActionType::Start: hasStart = true; break;
            case ActionType::Stop: hasStop = true; break;
            case ActionType::Raise: hasRaise = true; break;
            case ActionType::Reset: hasReset = true; break;
            case ActionType::Alloc: hasAlloc = true; break;
            case ActionType::Free: hasFree = true; break;
            case ActionType::Send: hasSend = true; break;
            case ActionType::Receive: hasReceive = true; break;
            case ActionType::Submit: hasSubmit = true; break;
            case ActionType::Cancel: hasCancel = true; break;
            default: break;
        }
    }

    switch (obj.type) {
        case ObjectType::Mutex:
            return EdgeType::MutualExclusion;
        case ObjectType::Semaphore:
            if (hasGive) return EdgeType::Signals;
            if (hasTake) return EdgeType::ConsumesFrom;
            if (hasLock) return EdgeType::MutualExclusion;
            return EdgeType::Unknown;
        case ObjectType::Fifo:
        case ObjectType::Lifo:
        case ObjectType::Queue:
        case ObjectType::Msgq:
            if (hasPut) return EdgeType::ProducesFor;
            if (hasGet) return EdgeType::ConsumesFrom;
            return EdgeType::Unknown;
        case ObjectType::Mbox:
            if (hasSend) return EdgeType::ProducesFor;
            if (hasReceive) return EdgeType::ConsumesFrom;
            return EdgeType::Bidirectional;
        case ObjectType::Pipe:
            if (hasPut) return EdgeType::ProducesFor;
            if (hasGet) return EdgeType::ConsumesFrom;
            return EdgeType::Bidirectional;
        case ObjectType::Event:
            if (hasPost) return EdgeType::Signals;
            if (hasWait) return EdgeType::ConsumesFrom;
            return EdgeType::Unknown;
        case ObjectType::Condvar:
            if (hasSignal || hasBroadcast) return EdgeType::Signals;
            if (hasWait) return EdgeType::ConsumesFrom;
            return EdgeType::Unknown;
        case ObjectType::Stack:
            if (hasPush) return EdgeType::ProducesFor;
            if (hasPop) return EdgeType::ConsumesFrom;
            return EdgeType::Unknown;
        case ObjectType::Timer:
            if (hasStart) return EdgeType::Signals;
            return EdgeType::Unknown;
        case ObjectType::Work:
            if (hasSubmit) return EdgeType::Signals;
            return EdgeType::Unknown;
        case ObjectType::Heap:
            // See inferEdgeType() for rationale: heap is a clique coupled
            // by shared-resource integrity, not producer/consumer.
            return EdgeType::SharedMemory;
        case ObjectType::MemSlab:
            if (hasFree) return EdgeType::ProducesFor;
            if (hasAlloc) return EdgeType::ConsumesFrom;
            return EdgeType::Unknown;
        case ObjectType::PollSignal:
            if (hasRaise) return EdgeType::Signals;
            return EdgeType::Unknown;
        default:
            return EdgeType::Unknown;
    }
}

Severity ThreadEdge::inferSeverity(EdgeType edgeType) {
    switch (edgeType) {
        case EdgeType::MutualExclusion: return Severity::Critical;
        case EdgeType::Signals: return Severity::High;
        case EdgeType::ProducesFor: return Severity::High;
        case EdgeType::ConsumesFrom: return Severity::Medium;
        case EdgeType::Bidirectional: return Severity::High;
        case EdgeType::SharedMemory: return Severity::High;
        case EdgeType::Join: return Severity::Low;
    case EdgeType::Abort: return Severity::High;
    case EdgeType::Wakeup: return Severity::Low;
        default: return Severity::Medium;
    }
}

std::string severityToString(Severity s) {
    switch (s) {
        case Severity::Critical: return "CRITICAL";
        case Severity::High: return "HIGH";
        case Severity::Medium: return "MEDIUM";
        case Severity::Low: return "LOW";
        default: return "NONE";
    }
}

Severity stringToSeverity(const std::string& s) {
    if (s == "CRITICAL") return Severity::Critical;
    if (s == "HIGH") return Severity::High;
    if (s == "MEDIUM") return Severity::Medium;
    if (s == "LOW") return Severity::Low;
    return Severity::None;
}

std::string restartClassToString(RestartClass rc) {
    switch (rc) {
        case RestartClass::Mandatory: return "mandatory";
        case RestartClass::Advisory: return "advisory";
        case RestartClass::FilterSkipped: return "filter_skipped";
        case RestartClass::NonRestartable: return "non_restartable";
        default: return "unknown";
    }
}

bool isNonRestartableThread(const std::string& name,
                            const std::string& entryFn,
                            const std::string& sourceFile) {
    // Kernel entities by Zephyr naming convention (checked on both the
    // thread name and the entry-function name; k_thread_create registers
    // the entry name as the thread name, K_THREAD_DEFINE registers the
    // tid variable, so either can carry the conventional name).
    auto isKernelNonRestartable = [](const std::string& n) {
        if (n == "isr") return true;                 // synthetic ISR pseudo-thread
        if (n == "idle") return true;                // per-CPU idle thread
        if (n.rfind("idle_", 0) == 0) {              // SMP idle threads: idle_00..
            size_t i = 5;
            while (i < n.size() && isdigit(static_cast<unsigned char>(n[i]))) i++;
            if (i == n.size() && i > 5) return true;
        }
        if (n == "bg_thread_main") return true;      // static boot thread (runs main())
        return false;
    };
    if (isKernelNonRestartable(name) || isKernelNonRestartable(entryFn)) return true;

    // native_sim / native_posix host-side driver threads: SDL display and
    // event loop, native UART PTY, native TAP ethernet, DMIC emulator, and
    // anything defined under the native_sim / native_posix / boards/native
    // trees. Matched on the translation-unit path so the rule is robust to
    // driver-specific thread names.
    static const char* kHostDriverPathPatterns[] = {
        "native_sim", "native_posix", "boards/native",
        "uart_native", "native_tap", "display_sdl", "sdl_events", "dmic_emul",
    };
    for (const char* pattern : kHostDriverPathPatterns) {
        if (sourceFile.find(pattern) != std::string::npos) return true;
    }

    return false;
}

int severityRank(Severity s) {
    switch (s) {
        case Severity::Critical: return 4;
        case Severity::High: return 3;
        case Severity::Medium: return 2;
        case Severity::Low: return 1;
        default: return 0;
    }
}

Severity severityFromRank(int rank) {
    switch (rank) {
        case 4: return Severity::Critical;
        case 3: return Severity::High;
        case 2: return Severity::Medium;
        case 1: return Severity::Low;
        default: return Severity::None;
    }
}

void AnalysisResult::buildMaps() {
    for (auto& t : threads) {
        threadMap[t.name] = t;
    }
    for (auto& o : objects) {
        objectMap[o.name] = o;
    }
}

Thread* AnalysisResult::findThread(const std::string& name) {
    auto it = threadMap.find(name);
    return it != threadMap.end() ? &it->second : nullptr;
}

IPCObject* AnalysisResult::findObject(const std::string& name) {
    auto it = objectMap.find(name);
    return it != objectMap.end() ? &it->second : nullptr;
}
