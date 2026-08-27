#!/usr/bin/env python3
# Copyright (c) 2026 Matthew Bowden
#
# SPDX-License-Identifier: Apache-2.0
"""Examine crash impact analysis results and report crash clusters per project.

A "crash cluster" is a connected component of threads linked by crash
impact edges.  An edge from A to B means: if A crashes, B is affected
through some shared object.  The script builds an undirected graph from
the impact analysis and reports connected components as clusters.

With --threshold, edges at or below the given severity are excluded,
which can split a single cluster into smaller, more meaningful sub-clusters.
For example --threshold MEDIUM keeps only HIGH and CRITICAL edges, so
two sub-systems joined only by a MEDIUM-severity bridge become separate
clusters.

Usage:
    scripts/thread_groups.py                         # all projects, no threshold
    scripts/thread_groups.py zswatch zmk             # specific projects
    scripts/thread_groups.py --threshold MEDIUM      # split on MEDIUM-or-below bridges
    scripts/thread_groups.py --threshold HIGH        # keep only CRITICAL edges
    scripts/thread_groups.py --dir other-dir         # custom results directory
"""

import argparse
import json
from collections import Counter, defaultdict, deque
from pathlib import Path

SEVERITY_ORDER = {"LOW": 0, "MEDIUM": 1, "HIGH": 2, "CRITICAL": 3}
SEVERITY_ABOVE = {v: k for k, v in SEVERITY_ORDER.items()}


def severity_rank(name):
    return SEVERITY_ORDER.get(name, -1)


def load_project(path):
    with open(path) as f:
        return json.load(f)


def path_hops(path):
    """Number of hops in an impact path.

    path = [crashed, via_obj, affected]                    -> 1 hop
    path = [crashed, via1, mid_thread, via2, affected]    -> 2 hops
    General: hops = (len(path) - 1) // 2
    """
    return (len(path) - 1) // 2


def build_impact_graph(data, threshold_rank):
    """Build an undirected graph from the impact analysis.

    Returns:
        adjacency: dict[thread] -> set[thread]
        edges: list of (src, dst, severity, via_object, hops)
        crash_sources: set of threads that have at least one outgoing edge
    """
    adjacency = defaultdict(set)
    edges = []
    crash_sources = set()

    for entry in data.get("impact_analysis", []):
        crashed = entry.get("crashed_thread", "")
        affected = entry.get("affected_threads", {})
        for at, info in affected.items():
            sev = info.get("severity", "?")
            if severity_rank(sev) <= threshold_rank:
                continue
            via = info.get("path", ["", "?"])
            via_obj = via[1] if len(via) >= 2 else "?"
            hops = path_hops(info.get("path", []))
            adjacency[crashed].add(at)
            adjacency[at].add(crashed)
            edges.append((crashed, at, sev, via_obj, hops))
            crash_sources.add(crashed)

    return adjacency, edges, crash_sources


def connected_components(adjacency, all_nodes):
    """Find connected components via BFS. Returns list of frozensets."""
    visited = set()
    components = []
    for node in all_nodes:
        if node in visited:
            continue
        component = set()
        queue = deque([node])
        while queue:
            n = queue.popleft()
            if n in visited:
                continue
            visited.add(n)
            component.add(n)
            for neighbor in adjacency.get(n, set()):
                if neighbor not in visited:
                    queue.append(neighbor)
        components.append(frozenset(component))
    return components


def print_separator(char="=", width=92):
    print(char * width)


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument(
        "projects", nargs="*", help="project names to include (default: all)"
    )
    ap.add_argument(
        "--dir", default="test-results", help="results directory (default: test-results)"
    )
    ap.add_argument(
        "--threshold",
        choices=["LOW", "MEDIUM", "HIGH", "CRITICAL"],
        default=None,
        help=(
            "exclude edges at or below this severity. "
            "LOW=keep MEDIUM+, MEDIUM=keep HIGH+, HIGH=keep CRITICAL only. "
            "Default: keep all edges (no threshold)."
        ),
    )
    args = ap.parse_args()

    # threshold_rank: edges with severity_rank <= threshold_rank are excluded
    if args.threshold:
        threshold_rank = severity_rank(args.threshold)
        threshold_label = f" (threshold: {args.threshold} — keeping {args.threshold}+ excluded, only edges above {args.threshold} count)"
        kept_label = f"edges above {args.threshold}"
    else:
        threshold_rank = -1
        threshold_label = ""
        kept_label = "all edges"

    results_dir = Path(args.dir)
    if not results_dir.is_dir():
        ap.error(f"directory not found: {results_dir}")

    all_files = sorted(results_dir.glob("*-analysis.json"))
    if args.projects:
        wanted = {p.lower() for p in args.projects}
        all_files = [
            f for f in all_files
            if f.stem.replace("-analysis", "").lower() in wanted
        ]
    if not all_files:
        print("No matching project files found.")
        return

    print_separator()
    title = "CRASH IMPACT CLUSTER SUMMARY"
    if args.threshold:
        title += f"  [threshold={args.threshold}, {kept_label}]"
    print(title)
    print_separator()

    header = (
        f"{'Project':<14} {'Thr':>4} {'Iso':>4} "
        f"{'Cls':>4} {'#2':>4} {'#3':>4} {'#4+':>4} {'Max':>4} {'Hop':>4}"
    )
    print(header)
    print("-" * len(header))

    for fpath in all_files:
        label = fpath.stem.replace("-analysis", "")
        data = load_project(fpath)
        thread_count = len(data.get("threads", []))
        all_thread_names = {t["name"] for t in data.get("threads", [])}

        adjacency, edges, crash_sources = build_impact_graph(data, threshold_rank)

        # all known threads are graph nodes; those with no edges become singletons
        graph_nodes = set(adjacency.keys()) | all_thread_names
        components = connected_components(adjacency, graph_nodes)

        non_trivial = [c for c in components if len(c) >= 2]
        isolated = [c for c in components if len(c) == 1]
        iso_count = len(isolated)

        size_hist = Counter(len(c) for c in non_trivial)
        max_size = max((len(c) for c in non_trivial), default=0)
        max_hop = max((e[4] for e in edges), default=0)

        print(
            f"{label:<14} {thread_count:>4} {iso_count:>4} "
            f"{len(non_trivial):>4} {size_hist.get(2, 0):>4} "
            f"{size_hist.get(3, 0):>4} "
            f"{sum(c for s, c in size_hist.items() if s >= 4):>4} "
            f"{max_size:>4} {max_hop:>4}"
        )

    print()
    print(
        "Iso = isolated threads, Cls = clusters (2+ threads), "
        "#2/#3 = clusters of exactly that size, #4+ = clusters of 4+"
    )
    print()

    # ---- Per-project detail ----
    for fpath in all_files:
        label = fpath.stem.replace("-analysis", "")
        data = load_project(fpath)
        all_thread_names = {t["name"] for t in data.get("threads", [])}

        adjacency, edges, crash_sources = build_impact_graph(data, threshold_rank)

        graph_nodes = set(adjacency.keys()) | all_thread_names
        components = connected_components(adjacency, graph_nodes)
        non_trivial = [c for c in components if len(c) >= 2]
        isolated = [c for c in components if len(c) == 1]

        if not non_trivial:
            print_separator("-")
            print(f"{label.upper()}  —  no crash clusters (all {len(isolated)} threads isolated)")
            print_separator("-")
            if isolated:
                names = sorted(next(iter(c)) for c in isolated)
                print(f"  Isolated: {', '.join(names)}")
            print()
            continue

        # sort clusters by size descending
        sorted_clusters = sorted(non_trivial, key=lambda c: -len(c))

        print_separator("-")
        cluster_word = "cluster" if len(non_trivial) == 1 else "clusters"
        print(
            f"{label.upper()}  —  {len(non_trivial)} {cluster_word}, "
            f"{len(isolated)} isolated threads"
        )
        print_separator("-")

        for ci, cluster in enumerate(sorted_clusters):
            # edges within this cluster
            cluster_edges = [
                e for e in edges if e[0] in cluster and e[1] in cluster
            ]
            sev_counter = Counter(e[2] for e in cluster_edges)
            max_hop = max((e[4] for e in cluster_edges), default=0)
            sources_in_cluster = crash_sources & cluster
            sev_str = "/".join(f"{s}:{c}" for s, c in sev_counter.most_common())

            tag = ""
            if len(sorted_clusters) > 1:
                tag = f"  cluster {ci + 1}/{len(sorted_clusters)}"

            print(f"  [{len(cluster)} threads, {len(sources_in_cluster)} crash sources, "
                  f"hop={max_hop}, {sev_str}]{tag}")
            print(f"    {', '.join(sorted(cluster))}")

            # per-source breakdown
            if len(sources_in_cluster) > 1:
                print()
                print("    crash sources:")
                for src in sorted(sources_in_cluster):
                    src_affected = [
                        e for e in cluster_edges if e[0] == src
                    ]
                    src_sev = Counter(e[2] for e in src_affected)
                    src_sev_str = "/".join(
                        f"{s}:{c}" for s, c in src_sev.most_common()
                    )
                    via_list = ", ".join(
                        f"{e[1]}:{e[3]}" for e in src_affected
                    )
                    print(
                        f"      {src:<30s} -> {len(src_affected)} threads  "
                        f"sev={src_sev_str}"
                    )
                    print(f"        via: {via_list}")

        # isolated threads
        iso_names = sorted(next(iter(c)) for c in isolated)
        if iso_names:
            print()
            print(f"  Isolated: {', '.join(iso_names)}")

        print()


if __name__ == "__main__":
    main()
