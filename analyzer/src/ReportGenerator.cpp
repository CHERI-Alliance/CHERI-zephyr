/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ReportGenerator.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

ReportGenerator::ReportGenerator(const AnalysisResult& result, const std::vector<ThreadEdge>& edges,
                                   const std::string& scope)
    : result_(result), edges_(edges), scope_(scope) {
    if (scope_ == "user") {
        for (const auto& t : result_.threads) {
            if (t.isSystem) systemThreadNames_.insert(t.name);
        }
        for (const auto& o : result_.objects) {
            if (o.isSystem) systemObjectNames_.insert(o.name);
        }
    }
}

bool ReportGenerator::shouldShowThread(const std::string& name) const {
    if (scope_ != "user") return true;
    return systemThreadNames_.find(name) == systemThreadNames_.end();
}

bool ReportGenerator::shouldShowObject(const std::string& name) const {
    if (scope_ != "user") return true;
    return systemObjectNames_.find(name) == systemObjectNames_.end();
}

std::string ReportGenerator::generateJson(const CrashImpact* impact, const std::vector<CrashImpact>* allImpacts) const {
    std::ostringstream out;

    std::vector<const Thread*> visibleThreads;
    for (const auto& t : result_.threads) {
        if (shouldShowThread(t.name)) visibleThreads.push_back(&t);
    }
    std::vector<const IPCObject*> visibleObjects;
    for (const auto& o : result_.objects) {
        if (shouldShowObject(o.name)) visibleObjects.push_back(&o);
    }

    out << "{\n";
    out << "  \"threads\": [\n";
    for (size_t i = 0; i < visibleThreads.size(); ++i) {
        const auto& t = *visibleThreads[i];
        out << "    {\n";
        out << "      \"name\": \"" << escapeJson(t.name) << "\",\n";
        out << "      \"entry_fn\": \"" << escapeJson(t.entryFn) << "\",\n";
        out << "      \"source\": \"" << escapeJson(t.location.toString()) << "\",\n";
        out << "      \"priority\": " << t.priority << ",\n";
        out << "      \"is_workqueue\": " << (t.isWorkqueue ? "true" : "false") << ",\n";
        out << "      \"is_timer_cb\": " << (t.isTimerCb ? "true" : "false") << ",\n";
        out << "      \"source_file\": \"" << escapeJson(t.sourceFile) << "\",\n";
        out << "      \"is_system\": " << (t.isSystem ? "true" : "false") << ",\n";
        out << "      \"restartable\": " << (t.isRestartable ? "true" : "false") << "\n";
        out << "    }" << (i < visibleThreads.size() - 1 ? "," : "") << "\n";
    }
    out << "  ],\n";

    out << "  \"objects\": [\n";
    for (size_t i = 0; i < visibleObjects.size(); ++i) {
        const auto& o = *visibleObjects[i];
        out << "    {\n";
        out << "      \"name\": \"" << escapeJson(o.name) << "\",\n";
        out << "      \"type\": \"" << escapeJson(o.typeString()) << "\",\n";
        out << "      \"scope\": \"" << escapeJson(o.scope) << "\",\n";
        out << "      \"source\": \"" << escapeJson(o.location.toString()) << "\",\n";
        out << "      \"source_file\": \"" << escapeJson(o.sourceFile) << "\",\n";
        out << "      \"is_system\": " << (o.isSystem ? "true" : "false") << "\n";
        out << "    }" << (i < visibleObjects.size() - 1 ? "," : "") << "\n";
    }
    out << "  ],\n";

    out << "  \"interactions\": [\n";
    {
        bool first = true;
        for (const auto& inter : result_.interactions) {
            if (!shouldShowThread(inter.threadName) || !shouldShowObject(inter.objectName)) continue;
            if (!first) out << ",\n";
            first = false;
            out << "    {\n";
            out << "      \"thread\": \"" << escapeJson(inter.threadName) << "\",\n";
            out << "      \"object\": \"" << escapeJson(inter.objectName) << "\",\n";
            out << "      \"action\": \"" << escapeJson(inter.actionString()) << "\",\n";
            out << "      \"can_block\": " << (inter.canBlock ? "true" : "false") << ",\n";
            out << "      \"source\": \"" << escapeJson(inter.location.toString()) << "\",\n";
            out << "      \"resolved\": " << (inter.resolved ? "true" : "false") << "\n";
            out << "    }";
        }
        if (!first) out << "\n";
    }
    out << "  ],\n";

    out << "  \"contention_groups\": [\n";
    {
        bool first = true;
        for (const auto& group : result_.contentionGroups) {
            if (!shouldShowObject(group.objectName)) continue;
            std::vector<std::string> visibleGroupThreads;
            for (const auto& th : group.threads) {
                if (shouldShowThread(th)) visibleGroupThreads.push_back(th);
            }
            if (visibleGroupThreads.empty()) continue;
            if (!first) out << ",\n";
            first = false;
            out << "    {\n";
            out << "      \"object\": \"" << escapeJson(group.objectName) << "\",\n";
            out << "      \"object_type\": \"" << escapeJson(IPCObject::objectTypeToString(group.objectType)) << "\",\n";
            out << "      \"group_type\": \"" << escapeJson(edgeTypeToString(group.groupType)) << "\",\n";
            out << "      \"severity\": \"" << escapeJson(severityToString(group.severity)) << "\",\n";
            out << "      \"highly_connected\": " << (group.highlyConnected ? "true" : "false") << ",\n";
            out << "      \"threads\": [\n";
            for (size_t j = 0; j < visibleGroupThreads.size(); ++j) {
                out << "        \"" << escapeJson(visibleGroupThreads[j]) << "\"";
                if (j < visibleGroupThreads.size() - 1) out << ",";
                out << "\n";
            }
            out << "      ]\n";
            out << "    }";
        }
        if (!first) out << "\n";
    }
    out << "  ],\n";

    out << "  \"edges\": [\n";
    {
        bool first = true;
        for (const auto& e : edges_) {
            if (!shouldShowThread(e.fromThread) || !shouldShowThread(e.toThread)) continue;
            if (!first) out << ",\n";
            first = false;
            out << "    {\n";
            out << "      \"from\": \"" << escapeJson(e.fromThread) << "\",\n";
            out << "      \"to\": \"" << escapeJson(e.toThread) << "\",\n";
            out << "      \"object\": \"" << escapeJson(e.objectName) << "\",\n";
            out << "      \"edge_type\": \"" << escapeJson(edgeTypeToString(e.edgeType)) << "\",\n";
            out << "      \"severity\": \"" << escapeJson(severityToString(e.severity)) << "\",\n";
            out << "      \"highly_connected\": " << (e.highlyConnected ? "true" : "false") << "\n";
            out << "    }";
        }
        if (!first) out << "\n";
    }
    out << "  ]\n";

    if (impact) {
        out << ",\n  \"crash_impact\": {\n";
        out << "    \"crashed_thread\": \"" << escapeJson(impact->crashedThread) << "\",\n";
        out << "    \"affected_threads\": {\n";
        size_t count = 0;
        for (const auto& pair : impact->affectedThreads) {
            out << "      \"" << escapeJson(pair.first) << "\": {\n";
            out << "        \"reason\": \"" << escapeJson(pair.second.reason) << "\",\n";
            out << "        \"severity\": \"" << escapeJson(severityToString(pair.second.severity)) << "\",\n";
            out << "        \"path\": [";
            for (size_t j = 0; j < pair.second.path.size(); ++j) {
                out << "\"" << escapeJson(pair.second.path[j]) << "\"";
                if (j < pair.second.path.size() - 1) out << ", ";
            }
            out << "]\n";
            out << "      }" << (++count < impact->affectedThreads.size() ? "," : "") << "\n";
        }
        out << "    }\n";
        out << "    ,\n";
        emitRestartSet(out, *impact, /*indent=*/2);
        out << "    ,\n";
        emitFilterSkipped(out, *impact, /*indent=*/2);
        out << "  }\n";
    }

    if (allImpacts && !allImpacts->empty()) {
        out << ",\n  \"impact_analysis\": [\n";
        for (size_t i = 0; i < allImpacts->size(); ++i) {
            const auto& ci = (*allImpacts)[i];
            out << "    {\n";
            out << "      \"crashed_thread\": \"" << escapeJson(ci.crashedThread) << "\",\n";
            out << "      \"affected_threads\": {\n";
            size_t visibleCount = 0;
            size_t totalVisible = 0;
            for (const auto& pair : ci.affectedThreads) {
                if (shouldShowThread(pair.first)) totalVisible++;
            }
            for (const auto& pair : ci.affectedThreads) {
                if (!shouldShowThread(pair.first)) continue;
                out << "        \"" << escapeJson(pair.first) << "\": {\n";
                out << "          \"reason\": \"" << escapeJson(pair.second.reason) << "\",\n";
                out << "          \"severity\": \"" << escapeJson(severityToString(pair.second.severity)) << "\",\n";
                out << "          \"path\": [";
                for (size_t j = 0; j < pair.second.path.size(); ++j) {
                    out << "\"" << escapeJson(pair.second.path[j]) << "\"";
                    if (j < pair.second.path.size() - 1) out << ", ";
                }
                out << "]\n";
                out << "        }" << (++visibleCount < totalVisible ? "," : "") << "\n";
            }
            out << "      }\n";
            out << "      ,\n";
            emitRestartSet(out, ci, /*indent=*/3);
            out << "      ,\n";
            emitFilterSkipped(out, ci, /*indent=*/3);
            out << "    }" << (i < allImpacts->size() - 1 ? "," : "") << "\n";
        }
        out << "  ]\n";
    }

    out << "}\n";

    return out.str();
}

std::string ReportGenerator::generateText(const CrashImpact* impact) const {
    std::ostringstream out;

    size_t visibleThreads = std::count_if(result_.threads.begin(), result_.threads.end(),
        [this](const Thread& t) { return shouldShowThread(t.name); });
    size_t visibleObjects = std::count_if(result_.objects.begin(), result_.objects.end(),
        [this](const IPCObject& o) { return shouldShowObject(o.name); });
    size_t visibleInteractions = std::count_if(result_.interactions.begin(), result_.interactions.end(),
        [this](const Interaction& i) { return shouldShowThread(i.threadName) && shouldShowObject(i.objectName); });

    out << "=============================================================\n";
    out << "  ZEPHYR THREAD CRASH IMPACT ANALYSIS (Static)\n";
    out << "=============================================================\n\n";

    out << "Threads discovered: " << visibleThreads;
    if (scope_ == "user") out << " (user only; " << result_.threads.size() << " total)";
    out << "\n";
    out << "IPC objects discovered: " << visibleObjects;
    if (scope_ == "user") out << " (user only; " << result_.objects.size() << " total)";
    out << "\n";
    out << "Interactions discovered: " << visibleInteractions;
    long unresolved = std::count_if(result_.interactions.begin(), result_.interactions.end(),
        [this](const auto& i) { return !i.resolved && shouldShowThread(i.threadName) && shouldShowObject(i.objectName); });
    out << " (" << (visibleInteractions - unresolved) << " resolved, "
        << unresolved << " unresolved)\n";

    if (impact) {
        out << "\n-------------------------------------------------------------\n";
        out << "  CRASH SCENARIO: " << impact->crashedThread << " crashes\n";
        out << "-------------------------------------------------------------\n\n";

        for (const auto& pair : impact->affectedThreads) {
            const auto& entry = pair.second;
            out << severityToString(entry.severity) << " - " << pair.first << "\n";
            out << "  Reason: " << entry.reason << "\n";
            out << "  Path: ";
            for (size_t i = 0; i < entry.path.size(); ++i) {
                if (i > 0) out << " -> ";
                out << entry.path[i];
            }
            out << "\n\n";
        }
    }

    return out.str();
}

std::string ReportGenerator::generateDot(const CrashImpact* impact) const {
    std::ostringstream out;

    out << "digraph crash_impact";
    if (impact) {
        out << "_" << impact->crashedThread;
    }
    out << " {\n";
    out << "    rankdir=LR;\n";
    out << "    node [shape=box, style=filled];\n\n";

    for (const auto& t : result_.threads) {
        if (!shouldShowThread(t.name)) continue;
        std::string fillcolor = t.isSystem ? "lightblue" : "lightgray";
        std::string fontcolor = "black";
        std::string label = t.name;

        if (impact) {
            if (t.name == impact->crashedThread) {
                fillcolor = "red";
                fontcolor = "white";
                label += "\\n(CRASHED)";
            } else if (impact->affectedThreads.count(t.name)) {
                auto& entry = impact->affectedThreads.at(t.name);
                switch (entry.severity) {
                    case Severity::Critical: fillcolor = "darkred"; fontcolor = "white"; break;
                    case Severity::High: fillcolor = "orange"; break;
                    case Severity::Medium: fillcolor = "yellow"; break;
                    case Severity::Low: fillcolor = "lightgreen"; break;
                    default: break;
                }
                label += "\\n(" + severityToString(entry.severity) + ")";
            }
        }

        out << "    \"" << t.name << "\" [fillcolor=" << fillcolor
            << ", fontcolor=" << fontcolor << ", label=\"" << label << "\""
            << ", color=" << (t.isSystem ? "dodgerblue" : "black")
            << ", penwidth=" << (t.isSystem ? 2 : 1) << "];\n";
    }

    out << "\n";

    for (const auto& e : edges_) {
        if (!shouldShowThread(e.fromThread) || !shouldShowThread(e.toThread)) continue;
        std::string color = "gray";
        int penwidth = 1;

        if (impact && impact->crashedThread == e.fromThread) {
            color = "red";
            penwidth = 2;
        }

        out << "    \"" << e.fromThread << "\" -> \"" << e.toThread
            << "\" [label=\"" << e.objectName << " (" << edgeTypeToString(e.edgeType)
            << ")\", color=" << color << ", penwidth=" << penwidth << "];\n";
    }

    for (const auto& group : result_.contentionGroups) {
        if (!shouldShowObject(group.objectName)) continue;
        std::vector<std::string> visibleGroupThreads;
        for (const auto& th : group.threads) {
            if (shouldShowThread(th)) visibleGroupThreads.push_back(th);
        }
        if (visibleGroupThreads.empty()) continue;

        std::string groupName = group.objectName + "_group";
        std::string groupLabel = group.objectName + "\\n(" + std::to_string(visibleGroupThreads.size()) + " threads)";

        std::string shape = "house";
        std::string fillcolor = "lightcoral";
        std::string edgeGroupType = edgeTypeToString(group.groupType);

        if (edgeGroupType == "shared_memory") {
            shape = "diamond";
            fillcolor = "lightyellow";
        } else if (edgeGroupType == "init_table") {
            // Advisory-only: write-once boot registration table.
            // Visually downplayed to indicate non-propagating.
            shape = "note";
            fillcolor = "lightgrey";
        } else if (edgeGroupType == "bidirectional") {
            shape = "hexagon";
            fillcolor = "lightblue";
        }

        out << "    \"" << groupName << "\" [shape=" << shape << ", style=filled, fillcolor=" << fillcolor << ", label=\""
            << groupLabel << "\"];\n";

        for (const auto& thread : visibleGroupThreads) {
            std::string color = "gray";
            int penwidth = 1;
            if (impact && impact->crashedThread == thread) {
                color = "red";
                penwidth = 2;
            }
            out << "    \"" << thread << "\" -> \"" << groupName
                << "\" [color=" << color << ", penwidth=" << penwidth << "];\n";
        }
    }

    out << "}\n";

    return out.str();
}

std::string ReportGenerator::escapeJson(const std::string& s) const {
    std::ostringstream out;
    for (char c : s) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    return out.str();
}

std::string ReportGenerator::edgeTypeToString(EdgeType type) const {
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

void ReportGenerator::emitRestartSet(std::ostringstream& out, const CrashImpact& ci, int indent) const {
    std::string pad(static_cast<size_t>(indent) * 2, ' ');
    std::string padInner(static_cast<size_t>(indent + 1) * 2, ' ');

    bool first = true;
    out << pad << "\"restart_set\": [\n";
    for (const auto& r : ci.restartSet) {
        if (!shouldShowThread(r.thread)) continue;
        if (!first) out << ",\n";
        first = false;
        out << padInner << "{\n";
        out << padInner << "  \"thread\": \"" << escapeJson(r.thread) << "\",\n";
        out << padInner << "  \"restart_class\": \"" << restartClassToString(r.restartClass) << "\",\n";
        out << padInner << "  \"reason\": \"" << escapeJson(r.reason) << "\",\n";
        out << padInner << "  \"severity\": \"" << escapeJson(severityToString(r.severity)) << "\",\n";
        out << padInner << "  \"path\": [";
        for (size_t j = 0; j < r.path.size(); ++j) {
            out << "\"" << escapeJson(r.path[j]) << "\"";
            if (j < r.path.size() - 1) out << ", ";
        }
        out << "]\n";
        out << padInner << "}";
    }
    if (!first) out << "\n";
    out << pad << "]\n";
}

void ReportGenerator::emitFilterSkipped(std::ostringstream& out, const CrashImpact& ci, int indent) const {
    std::string pad(static_cast<size_t>(indent) * 2, ' ');
    std::string padInner(static_cast<size_t>(indent + 1) * 2, ' ');

    out << pad << "\"filter_skipped_objects\": [";
    for (size_t j = 0; j < ci.filterSkippedObjects.size(); ++j) {
        out << "\"" << escapeJson(ci.filterSkippedObjects[j]) << "\"";
        if (j < ci.filterSkippedObjects.size() - 1) out << ", ";
    }
    out << "]\n";
}
