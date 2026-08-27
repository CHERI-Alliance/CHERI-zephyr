#!/usr/bin/env python3
# Copyright (c) 2026 Matthew Bowden
#
# SPDX-License-Identifier: Apache-2.0
"""
Summarize Zephyr crash analysis JSON output in plain text.
"""

import argparse
import json
import sys
from pathlib import Path


def summarize(json_path: str) -> str:
    with open(json_path) as f:
        data = json.load(f)

    lines = []
    lines.append("=" * 60)
    lines.append("  ZEPHYR THREAD CRASH ANALYSIS SUMMARY")
    lines.append("=" * 60)
    lines.append("")

    # Threads
    threads = data.get("threads", [])
    lines.append(f"THREADS ({len(threads)})")
    lines.append("-" * 40)
    for t in threads:
        source = t.get("source", "unknown")
        priority = t.get("priority", 0)
        lines.append(f"  {t['name']}")
        lines.append(f"    Entry: {t['entry_fn']}")
        lines.append(f"    Location: {source}")
        lines.append(f"    Priority: {priority}")
    lines.append("")

    # Objects
    objects = data.get("objects", [])
    lines.append(f"IPC OBJECTS ({len(objects)})")
    lines.append("-" * 40)
    for o in objects:
        source = o.get("source", "unknown")
        obj_type = o.get("type", "unknown")
        scope = o.get("scope", "global")
        lines.append(f"  {o['name']} ({obj_type})")
        lines.append(f"    Scope: {scope}")
        lines.append(f"    Location: {source}")
    lines.append("")

    # Interactions
    interactions = data.get("interactions", [])
    lines.append(f"INTERACTIONS ({len(interactions)})")
    lines.append("-" * 40)

    by_thread = {}
    for i in interactions:
        thread = i["thread"]
        if thread not in by_thread:
            by_thread[thread] = []
        by_thread[thread].append(i)

    for thread, calls in by_thread.items():
        lines.append(f"  {thread}:")
        for i in calls:
            obj = i["object"]
            action = i["action"]
            block = "BLOCKING" if i.get("can_block", False) else "non-blocking"
            source = i.get("source", "?:?")
            lines.append(f"    {action:12} {obj:15} ({block}) at {source}")
    lines.append("")

    # Contention groups
    groups = data.get("contention_groups", [])
    lines.append(f"CONTENTION GROUPS ({len(groups)})")
    lines.append("-" * 40)
    for g in groups:
        threads_str = ", ".join(g["threads"])
        group_type = g.get("group_type", g.get("object_type", "unknown"))
        lines.append(f"  {g['object']} ({group_type}, {g['severity']}): {threads_str}")
    lines.append("")

    # Edges
    edges = data.get("edges", [])
    lines.append(f"THREAD-TO-THREAD EDGES ({len(edges)})")
    lines.append("-" * 40)

    edge_counts = {"CRITICAL": 0, "HIGH": 0, "MEDIUM": 0, "LOW": 0}
    for e in edges:
        sev = e.get("severity", "UNKNOWN")
        if sev in edge_counts:
            edge_counts[sev] += 1
    for g in groups:
        sev = g.get("severity", "HIGH")
        if sev in edge_counts:
            edge_counts[sev] += 1

    for sev, count in edge_counts.items():
        if count > 0:
            lines.append(f"  {sev}: {count}")

    lines.append("")
    lines.append("EDGE DETAILS:")
    lines.append("-" * 40)

    seen_pairs = set()
    for e in edges:
        from_t = e["from"]
        to_t = e["to"]
        obj = e["object"]
        edge_type = e["edge_type"]
        sev = e.get("severity", "UNKNOWN")
        pair_key = (from_t, to_t, obj)

        if pair_key not in seen_pairs:
            seen_pairs.add(pair_key)
            evidence = e.get("evidence", [])
            lines.append(f"  {from_t} -> {to_t} via {obj}")
            lines.append(f"    Type: {edge_type}, Severity: {sev}")
            if evidence:
                lines.append(f"    Evidence ({len(evidence)} interaction(s)):")
                for ev in evidence[:3]:
                    ev_src = ev.get("source", "?:?")
                    ev_action = ev.get("action", "?")
                    lines.append(f"      - {ev_action} at {ev_src}")
                if len(evidence) > 3:
                    lines.append(f"      ... and {len(evidence) - 3} more")
    lines.append("")

    # Crash impact if present
    if "crash_impact" in data:
        lines.append("CRASH IMPACT ANALYSIS")
        lines.append("-" * 40)
        impact = data["crash_impact"]
        crashed = impact.get("crashed_thread", "unknown")
        affected = impact.get("affected_threads", [])

        lines.append(f"Crashed thread: {crashed}")
        lines.append(f"Affected threads: {len(affected)}")
        lines.append("")

        for a in affected:
            thread = a.get("thread", "?")
            severity = a.get("severity", "?")
            reason = a.get("reason", "?")
            path = a.get("path", [])
            lines.append(f"  {thread} ({severity})")
            lines.append(f"    Reason: {reason}")
            if path:
                lines.append(f"    Path: {' -> '.join(path)}")
        lines.append("")

    lines.append("=" * 60)
    lines.append(f"Generated from: {json_path}")
    lines.append("=" * 60)

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Summarize Zephyr crash analysis JSON output"
    )
    parser.add_argument(
        "input",
        nargs="?",
        default="analysis_output.json",
        help="Input JSON file (default: analysis_output.json)"
    )
    parser.add_argument(
        "-o", "--output",
        help="Output file (default: stdout)"
    )
    args = parser.parse_args()

    summary = summarize(args.input)

    if args.output:
        Path(args.output).write_text(summary)
        print(f"Summary written to {args.output}")
    else:
        print(summary)


if __name__ == "__main__":
    main()
