/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IMPACTANALYZER_H
#define IMPACTANALYZER_H

#include "Model.h"
#include <vector>
#include <map>
#include <string>
#include <set>

// Configuration for the B min-cut impact computation. Built from CLI flags in
// main.cpp. Defaults preserve historical full-transitive behaviour.
struct ImpactConfig {
    // Whether to include highlyConnected (omnipresent) content groups and edges
    // in the propagation graph. Default: true (historical behaviour).
    bool includeOmnipresent = true;
    // Maximum BFS hop to add threads to the mandatory restart set. -1 = unbounded
    // (historical behaviour). 0 = SCC only. 1 = SCC + first-order critical/high
    // blocking edges.
    int mandatoryHopDepth = -1;
    // Edges at or above this severity are mandatory; below it are advisory.
    Severity advisorySeverityThreshold = Severity::None;
    // Leak-tolerant heap recovery (--allow-leaks): assume a crashed thread
    // leaks its own allocations without corrupting heap metadata, so other
    // heap users need not restart. Excludes [system_heap] edges from impact
    // propagation (they are still reported in the edges list, downgraded to
    // LOW severity, and the object appears in filter_skipped_objects).
    // Default: false — heap is a CRITICAL all-or-nothing clique (a crash
    // mid-heap-operation can corrupt heap metadata; safe recovery requires
    // reinitialising the heap and every allocation pointer with it).
    bool allowLeaks = false;
};

class ImpactAnalyzer {
public:
    ImpactAnalyzer(const AnalysisResult& result, const std::vector<ThreadEdge>& edges,
                   const ImpactConfig& config = ImpactConfig());

    // Top-level API used by main.cpp. Always populates CrashImpact.affectedThreads
    // (the full transitive set, historical behaviour) AND CrashImpact.restartSet
    // (B min-cut partition: mandatory/advisory/filter_skipped).
    CrashImpact computeCrashImpact(const std::string& crashedThread);

    // Compute SCCs (Tarjan) over the filtered directed thread graph. Returns a
    // map of thread-name -> SCC-id (compact, contiguous from 0).
    std::map<std::string, int> computeSCCs();

private:
    // Tarjan SCC over an explicit adjacency map.
    std::map<std::string, int> computeSCCsOver(
        const std::map<std::string, std::set<std::string>>& adj);
    const AnalysisResult& result_;
    const std::vector<ThreadEdge>& edges_;
    ImpactConfig config_;

    std::map<std::string, std::vector<const ThreadEdge*>> edgesByFromThread_;
    // Threads classified as not individually restartable (ISR pseudo-
    // thread, idle, bg_thread_main, native_sim host threads). They stay
    // in the propagation graph (adjacency, SCC, BFS) but are excluded
    // from the restart set — except the crashed thread itself, which is
    // emitted with RestartClass::NonRestartable.
    std::set<std::string> nonRestartableThreads_;

    // Build a filtered adjacency map (edges + content groups) respecting the
    // ImpactConfig. Returns adjacency list and (out-param) the list of object
    // names that were skip-filtered.
    //
    // severityGated (D4 / recommendation 4): when true AND an advisory
    // severity threshold is configured, only edges / content groups
    // strictly STRONGER than the threshold participate. Used for the
    // SCC that feeds mandatoryByScc: threads fused into the crashed
    // thread's SCC only through weak (advisory-strength) edges —
    // downgraded primitive-scalar debug counters, atomic bookkeeping —
    // are not blocking-coupled and must not become mandatory merely by
    // cycling. With no threshold configured the gate is inactive
    // (historical full-graph SCC semantics).
    std::map<std::string, std::set<std::string>> buildFilteredAdjacency(
        std::set<std::string>* skippedObjects = nullptr,
        bool severityGated = false);

    Severity propagateSeverity(Severity severity, int hop);
    bool shouldPropagate(const ThreadEdge* edge);
    bool shouldConsiderEdge(const ThreadEdge* edge) const;
    bool shouldConsiderGroup(const ContentionGroup& group) const;
    std::string edgeTypeToString(EdgeType type);

    // Tarjan helpers.
    void tarjanSCC(const std::string& v,
                   std::map<std::string, int>& index,
                   std::map<std::string, int>& lowlink,
                   std::map<std::string, bool>& onStack,
                   std::vector<std::string>& stack,
                   int& idx,
                   std::map<std::string, int>& sccId,
                   const std::map<std::string, std::set<std::string>>& adj);
};

#endif
