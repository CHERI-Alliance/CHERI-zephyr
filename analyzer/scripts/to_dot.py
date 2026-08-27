#!/usr/bin/env python3
# Copyright (c) 2026 Matthew Bowden
#
# SPDX-License-Identifier: Apache-2.0
"""
Generate a Graphviz DOT graph from Zephyr crash analysis JSON output.
"""

import argparse
import json
import sys
from pathlib import Path


SEVERITY_COLORS = {
    "CRITICAL": "darkred",
    "HIGH": "orange",
    "MEDIUM": "gold",
    "LOW": "lightgreen",
    "NONE": "lightgray",
}

EDGE_TYPE_STYLES = {
    "mutual_exclusion": "solid",
    "signals": "dashed",
    "produces_for": "dotted",
    "consumes_from": "dotted",
    "bidirectional": "solid",
    "shared_memory": "solid",
    "join": "dashed",
    "unknown": "dashed",
}


def escape_label(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")


def generate_dot(json_path: str, *, impact_of: str = None) -> str:
    with open(json_path) as f:
        data = json.load(f)

    lines = []
    lines.append("digraph crash_impact {")
    lines.append("    rankdir=LR;")
    lines.append("    node [shape=box, style=filled, fontname=\"Helvetica\"];")
    lines.append("    edge [fontname=\"Helvetica\"];")
    lines.append("")

    threads = {t["name"]: t for t in data.get("threads", [])}
    edges = data.get("edges", [])

    crashed = None
    if impact_of:
        crashed = impact_of
    elif "crash_impact" in data:
        crashed = data["crash_impact"].get("crashed_thread")

    for thread in threads.values():
        name = thread["name"]
        if name == crashed:
            lines.append(f'    {escape_label(name)} [fillcolor=red, fontcolor=white, label="{escape_label(name)}\\n(CRASHED)"];')
        elif name == impact_of if impact_of else False:
            lines.append(f'    {escape_label(name)} [fillcolor=yellow, fontcolor=black, label="{escape_label(name)}\\n(crashed)"];')
        else:
            lines.append(f'    {escape_label(name)} [fillcolor=lightblue, fontcolor=black, label="{escape_label(name)}"];')

    lines.append("")

    seen = set()
    for edge in edges:
        from_t = escape_label(edge["from"])
        to_t = escape_label(edge["to"])
        obj = escape_label(edge["object"])
        edge_type = edge.get("edge_type", "unknown")
        severity = edge.get("severity", "NONE")
        style = EDGE_TYPE_STYLES.get(edge_type, "dashed")
        color = SEVERITY_COLORS.get(severity, "black")
        key = (from_t, to_t, obj)

        if key not in seen:
            seen.add(key)
            label = f"{obj}\\n({edge_type})"
            lines.append(f'    {from_t} -> {to_t} [label="{label}", color={color}, style={style}];')

    lines.append("}")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Generate Graphviz DOT from Zephyr crash analysis JSON"
    )
    parser.add_argument(
        "input",
        nargs="?",
        default="analysis_output.json",
        help="Input JSON file (default: analysis_output.json)"
    )
    parser.add_argument(
        "-o", "--output",
        help="Output DOT file (default: stdout)"
    )
    parser.add_argument(
        "--impact-of",
        help="Highlight crash impact for this thread"
    )
    args = parser.parse_args()

    dot = generate_dot(args.input, impact_of=args.impact_of)

    if args.output:
        Path(args.output).write_text(dot)
        print(f"DOT graph written to {args.output}")
        print(f"Visualize with: dot -Tsvg {args.output} -o graph.svg")
    else:
        print(dot)


if __name__ == "__main__":
    main()
