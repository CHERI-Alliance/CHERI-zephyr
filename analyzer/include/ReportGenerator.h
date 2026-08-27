/*
 * Copyright (c) 2026 Matthew Bowden
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include "Model.h"
#include <string>
#include <vector>
#include <set>

class ReportGenerator {
public:
    ReportGenerator(const AnalysisResult& result, const std::vector<ThreadEdge>& edges,
                    const std::string& scope = "all");

    std::string generateJson(const CrashImpact* impact = nullptr, const std::vector<CrashImpact>* allImpacts = nullptr) const;
    std::string generateText(const CrashImpact* impact = nullptr) const;
    std::string generateDot(const CrashImpact* impact = nullptr) const;

private:
    const AnalysisResult& result_;
    const std::vector<ThreadEdge>& edges_;
    std::string scope_;
    std::set<std::string> systemThreadNames_;
    std::set<std::string> systemObjectNames_;

    bool shouldShowThread(const std::string& name) const;
    bool shouldShowObject(const std::string& name) const;
    std::string escapeJson(const std::string& s) const;
    std::string edgeTypeToString(EdgeType type) const;

    // B min-cut report helpers. emitRestartSet writes a "restart_set" array
    // for a single CrashImpact, classifying each reached thread as
    // mandatory / advisory / filter_skipped. emitFilterSkipped writes the
    // "filter_skipped_objects" array of objects the propagation graph
    // excluded under --impact-omnipresent=false.
    void emitRestartSet(std::ostringstream& out, const CrashImpact& ci, int indent) const;
    void emitFilterSkipped(std::ostringstream& out, const CrashImpact& ci, int indent) const;
};

#endif
