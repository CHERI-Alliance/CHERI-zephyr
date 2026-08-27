/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GRAPHBUILDER_H
#define GRAPHBUILDER_H

#include "Model.h"
#include <vector>
#include <map>
#include <set>

class GraphBuilder {
public:
    explicit GraphBuilder(AnalysisResult& result, const KernelSymbolFilter& kernelFilter = KernelSymbolFilter{},
                          bool allowLeaks = false);

    void buildGraph();
    const std::vector<ThreadEdge>& getEdges() const { return edges_; }
    const std::vector<ContentionGroup>& getContentionGroups() const { return contentionGroups_; }

    void addGlobalVarEdges(const std::map<std::string, std::map<std::string, GlobalVarAccess>>& globalVarAccesses);
    void filterOmnipresentGroups(size_t totalThreadCount);
    void addEdge(const std::string& from, const std::string& to,
                 const std::string& objectName, EdgeType edgeType,
                 const std::vector<Interaction>& evidence);
    // Severity-override overload: lets addGlobalVarEdges apply the
    // primitive-scalar downgrade (and any future global-specific
    // severity adjustments) to directed producer/consumer edges,
    // rather than letting inferSeverity(ProducesFor) always win.
    void addEdge(const std::string& from, const std::string& to,
                 const std::string& objectName, EdgeType edgeType,
                 Severity severityOverride,
                 const std::vector<Interaction>& evidence);

private:
    AnalysisResult& result_;
    std::vector<ThreadEdge> edges_;
    std::vector<ContentionGroup> contentionGroups_;
    KernelSymbolFilter kernelFilter_;
    // --allow-leaks: downgrades [system_heap] edges from Critical to Low
    // (leak-tolerant recovery; ImpactAnalyzer additionally excludes them
    // from propagation). Default false = Critical all-or-nothing clique.
    bool allowLeaks_;

    std::string edgeTypeToString(EdgeType type);
};

#endif
