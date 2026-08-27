#!/usr/bin/env python3
# Copyright (c) 2026 Matthew Bowden
#
# SPDX-License-Identifier: Apache-2.0
"""Summarise the restartable-set analysis output for each project and across the corpus.

Reads test-results/*-analysis.json and produces a concise per-project and
corpus-wide summary of the B min-cut restart-set partition:

  - Per project: thread count, app/system split, content-group composition
    (IPC vs shared_memory vs init_table), and for each crash scenario the
    mandatory / advisory / filter_skipped partition sizes.
  - Corpus-wide: aggregate mandatory-set sizes, filter-skipped object
    frequency, and the projects with the largest/smallest mandatory sets.

Usage:
    scripts/restart_summary.py                        # all projects
    scripts/restart_summary.py zswatch ecfw           # specific projects
    scripts/restart_summary.py --dir test-results     # custom results dir
    scripts/restart_summary.py --json                 # machine-readable JSON output
    scripts/restart_summary.py --verbose              # per-crash detail
"""

import argparse
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

SEVERITY_ORDER = {"NONE": 0, "LOW": 1, "MEDIUM": 2, "HIGH": 3, "CRITICAL": 4}
RESTART_CLASS_ORDER = {"mandatory": 0, "advisory": 1, "filter_skipped": 2, "non_restartable": 3}


def load_project(path):
    with open(path) as f:
        return json.load(f)


def classify_thread(t):
    """Return 'app' or 'sys' based on is_system flag."""
    return "sys" if t.get("is_system") else "app"


def ci_for_crashed(crash_summaries, impact_analysis, crashed_thread):
    """Return the restart_set list for a given crashed thread."""
    for ci in impact_analysis:
        if ci.get("crashed_thread") == crashed_thread:
            return ci.get("restart_set", [])
    return []


def summarise_project(name, data, verbose=False):
    threads = data.get("threads", [])
    app_threads = [t for t in threads if not t.get("is_system")]
    sys_threads = [t for t in threads if t.get("is_system")]
    objects = data.get("objects", [])
    interactions = data.get("interactions", [])
    contention_groups = data.get("contention_groups", [])
    edges = data.get("edges", [])
    impact_analysis = data.get("impact_analysis", [])

    # Content group composition
    cg_by_type = Counter(g["group_type"] for g in contention_groups)
    cg_ipc = sum(v for k, v in cg_by_type.items() if k not in ("shared_memory", "init_table"))
    cg_shm = cg_by_type.get("shared_memory", 0)
    cg_init = cg_by_type.get("init_table", 0)

    # Edge composition
    edge_by_type = Counter(e["edge_type"] for e in edges)
    edge_ipc = sum(v for k, v in edge_by_type.items()
                   if k not in ("shared_memory", "init_table", "unknown"))
    edge_shm = edge_by_type.get("shared_memory", 0)
    edge_init = edge_by_type.get("init_table", 0)

    # Per-crash restart-set summary
    crash_summaries = []
    mandatory_sizes = []
    advisory_sizes = []
    filter_skipped_sizes = []
    filter_skipped_objects = set()

    for ci in impact_analysis:
        rs = ci.get("restart_set", [])
        fs_objs = ci.get("filter_skipped_objects", [])

        mand = [r for r in rs if r["restart_class"] == "mandatory"]
        adv = [r for r in rs if r["restart_class"] == "advisory"]
        flt = [r for r in rs if r["restart_class"] == "filter_skipped"]
        nrs = [r for r in rs if r["restart_class"] == "non_restartable"]

        mandatory_sizes.append(len(mand))
        advisory_sizes.append(len(adv))
        filter_skipped_sizes.append(len(flt))
        filter_skipped_objects.update(fs_objs)

        crash_summaries.append({
            "crashed_thread": ci["crashed_thread"],
            "affected": len(ci.get("affected_threads", {})),
            "mandatory": len(mand),
            "advisory": len(adv),
            "filter_skipped": len(flt),
            "non_restartable": len(nrs),
            "filter_skipped_objects": fs_objs,
        })

    n_threads = len(threads)
    n_crashes = len(impact_analysis)

    # --- Crash group analysis ---
    # Method 1: Symmetric SCC partition. Build an undirected graph where
    # threads A and B are connected if A appears in B's mandatory restart
    # set OR B appears in A's. Connected components = crash groups.
    # This captures transitive coupling: if A mandates B and B mandates C,
    # then {A,B,C} is one group even if A doesn't directly mandate C.
    mandatory_sets = {}
    for cs in crash_summaries:
        crashed = cs["crashed_thread"]
        rs = ci_for_crashed(crash_summaries, impact_analysis, crashed)
        mand_threads = {r["thread"] for r in rs if r["restart_class"] == "mandatory"}
        mandatory_sets[crashed] = mand_threads

    # Build undirected adjacency: A-B connected if B in mandatory_sets[A]
    # or A in mandatory_sets[B].
    adj = defaultdict(set)
    for crashed, mand_set in mandatory_sets.items():
        for other in mand_set:
            if other != crashed:
                adj[crashed].add(other)
                adj[other].add(crashed)

    # Connected components (BFS/DFS)
    visited = set()
    scc_groups = []
    for thread in mandatory_sets:
        if thread in visited:
            continue
        component = set()
        queue = [thread]
        while queue:
            t = queue.pop()
            if t in visited:
                continue
            visited.add(t)
            component.add(t)
            queue.extend(adj[t] - visited)
        scc_groups.append(sorted(component))
    scc_groups.sort(key=lambda g: -len(g))

    # Method 2: Exact-set equivalence. Group threads whose mandatory restart
    # sets are identical. Threads with the same mandatory set form one group;
    # threads with different sets form separate groups even if they overlap.
    set_to_threads = defaultdict(list)
    for crashed, mand_set in mandatory_sets.items():
        key = frozenset(mand_set)
        set_to_threads[key].append(crashed)
    exact_groups = list(set_to_threads.values())
    exact_groups.sort(key=lambda g: -len(g))

    # Worst-case crash (largest mandatory set)
    worst = max(crash_summaries, key=lambda c: c["mandatory"]) if crash_summaries else None

    # Best-case crash (smallest mandatory set, excluding self-only)
    non_trivial = [c for c in crash_summaries if c["mandatory"] > 1]
    best = min(non_trivial, key=lambda c: c["mandatory"]) if non_trivial else None

    summary = {
        "project": name,
        "threads": n_threads,
        "app_threads": len(app_threads),
        "sys_threads": len(sys_threads),
        "objects": len(objects),
        "interactions": len(interactions),
        "contention_groups": len(contention_groups),
        "cg_ipc": cg_ipc,
        "cg_shm": cg_shm,
        "cg_init": cg_init,
        "edges": len(edges),
        "edge_ipc": edge_ipc,
        "edge_shm": edge_shm,
        "edge_init": edge_init,
        "crash_scenarios": n_crashes,
        "mandatory_max": max(mandatory_sizes) if mandatory_sizes else 0,
        "mandatory_avg": sum(mandatory_sizes) / n_crashes if n_crashes else 0,
        "mandatory_min": min(mandatory_sizes) if mandatory_sizes else 0,
        "advisory_max": max(advisory_sizes) if advisory_sizes else 0,
        "advisory_avg": sum(advisory_sizes) / n_crashes if n_crashes else 0,
        "filter_skipped_max": max(filter_skipped_sizes) if filter_skipped_sizes else 0,
        "filter_skipped_objects_count": len(filter_skipped_objects),
        "filter_skipped_objects": sorted(filter_skipped_objects),
        "worst_crash": worst,
        "best_crash": best,
        # Crash group analysis:
        "scc_group_count": len(scc_groups),
        "scc_group_non_singleton": sum(1 for g in scc_groups if len(g) > 1),
        "scc_groups": scc_groups,
        "exact_group_count": len(exact_groups),
        "exact_group_non_singleton": sum(1 for g in exact_groups if len(g) > 1),
        "exact_groups": [sorted(g) for g in exact_groups],
    }

    if verbose:
        summary["crash_details"] = crash_summaries

    return summary


def print_per_project(summary):
    s = summary
    print(f"\n{'='*80}")
    print(f"  {s['project']}")
    print(f"{'='*80}")
    print(f"  Threads:          {s['threads']}  (app={s['app_threads']}, sys={s['sys_threads']})")
    print(f"  Objects:          {s['objects']}")
    print(f"  Interactions:     {s['interactions']}")
    print(f"  Content groups:   {s['contention_groups']}  "
          f"(IPC={s['cg_ipc']}, SHM={s['cg_shm']}, init_table={s['cg_init']})")
    print(f"  Edges:            {s['edges']}  "
          f"(IPC={s['edge_ipc']}, SHM={s['edge_shm']}, init_table={s['edge_init']})")
    print(f"  Crash scenarios:  {s['crash_scenarios']}")
    print(f"  Mandatory set:    max={s['mandatory_max']}  "
          f"avg={s['mandatory_avg']:.1f}  min={s['mandatory_min']}")
    print(f"  Advisory set:     max={s['advisory_max']}  avg={s['advisory_avg']:.1f}")
    print(f"  Filter-skipped:   max={s['filter_skipped_max']}  "
          f"distinct_objects={s['filter_skipped_objects_count']}")

    if s["worst_crash"]:
        w = s["worst_crash"]
        print(f"  Worst crash:      {w['crashed_thread']} "
              f"-> {w['mandatory']} mandatory, {w['advisory']} advisory, "
              f"{w['filter_skipped']} filter_skipped")
    if s["best_crash"]:
        b = s["best_crash"]
        print(f"  Best crash:       {b['crashed_thread']} "
              f"-> {b['mandatory']} mandatory, {b['advisory']} advisory, "
              f"{b['filter_skipped']} filter_skipped")

    if s["filter_skipped_objects"]:
        objs = s["filter_skipped_objects"][:8]
        suffix = f" ... (+{len(s['filter_skipped_objects'])-8} more)" if len(s["filter_skipped_objects"]) > 8 else ""
        print(f"  Filter-skipped objs: {', '.join(objs)}{suffix}")

    # Crash group analysis
    print(f"\n  Crash groups (symmetric SCC):  {s['scc_group_count']} group(s)")
    for i, grp in enumerate(s["scc_groups"]):
        members = ", ".join(grp[:6])
        if len(grp) > 6:
            members += f" ... (+{len(grp)-6})"
        print(f"    Group {i+1} ({len(grp)} threads): {members}")

    print(f"\n  Crash groups (exact-set):      {s['exact_group_count']} group(s)")
    for i, grp in enumerate(s["exact_groups"][:5]):
        members = ", ".join(grp[:4])
        if len(grp) > 4:
            members += f" ... (+{len(grp)-4})"
        print(f"    Group {i+1} ({len(grp)} threads): {members}")
    if len(s["exact_groups"]) > 5:
        print(f"    ... (+{len(s['exact_groups'])-5} more groups)")

    if "crash_details" in s:
        print(f"\n  {'crashed_thread':<25s} {'aff':>3s} {'mand':>4s} {'adv':>4s} {'skip':>4s} {'nrst':>4s}  filter_skipped_objects")
        print(f"  {'-'*80}")
        for c in s["crash_details"]:
            fs = ", ".join(c["filter_skipped_objects"][:3])
            if len(c["filter_skipped_objects"]) > 3:
                fs += f" (+{len(c['filter_skipped_objects'])-3})"
            print(f"  {c['crashed_thread']:<25s} {c['affected']:>3d} {c['mandatory']:>4d} "
                  f"{c['advisory']:>4d} {c['filter_skipped']:>4d} {c['non_restartable']:>4d}  {fs}")


def print_corpus(summaries):
    n_projects = len(summaries)
    total_threads = sum(s["threads"] for s in summaries)
    total_app = sum(s["app_threads"] for s in summaries)
    total_crashes = sum(s["crash_scenarios"] for s in summaries)

    corpus_mandatory_max = max(s["mandatory_max"] for s in summaries)
    corpus_mandatory_avg = sum(s["mandatory_avg"] * s["crash_scenarios"] for s in summaries) / total_crashes

    # Filter-skipped object frequency across corpus
    fs_freq = Counter()
    for s in summaries:
        for obj in s["filter_skipped_objects"]:
            fs_freq[obj] += 1

    # Projects with largest/smallest mandatory max
    by_mand_max = sorted(summaries, key=lambda s: s["mandatory_max"], reverse=True)
    by_mand_frac = sorted(summaries,
                          key=lambda s: s["mandatory_max"] / max(s["threads"], 1),
                          reverse=True)

    print(f"\n{'='*80}")
    print(f"  CORPUS SUMMARY ({n_projects} projects, {total_threads} threads, "
          f"{total_app} app, {total_crashes} crash scenarios)")
    print(f"{'='*80}")
    print(f"  Mandatory set:  corpus max={corpus_mandatory_max}  "
          f"weighted avg={corpus_mandatory_avg:.1f}")
    print(f"  Advisory set:   corpus max={max(s['advisory_max'] for s in summaries)}")
    print(f"  Filter-skipped:  corpus max={max(s['filter_skipped_max'] for s in summaries)}")

    print(f"\n  Per-project table:")
    print(f"  {'project':<16s} {'thr':>4s} {'app':>4s} {'cg_ipc':>6s} {'cg_shm':>6s} "
          f"{'cg_init':>7s} {'mand_max':>8s} {'adv_max':>7s} {'skip_max':>8s} "
          f"{'scc_grp':>7s} {'scc_ns':>6s} {'exact_grp':>9s} {'ex_ns':>5s}")
    print(f"  {'-'*95}")
    for s in summaries:
        print(f"  {s['project']:<16s} {s['threads']:>4d} {s['app_threads']:>4d} "
              f"{s['cg_ipc']:>6d} {s['cg_shm']:>6d} {s['cg_init']:>7d} "
              f"{s['mandatory_max']:>8d} {s['advisory_max']:>7d} "
              f"{s['filter_skipped_max']:>8d} "
              f"{s['scc_group_count']:>7d} {s['scc_group_non_singleton']:>6d} "
              f"{s['exact_group_count']:>9d} {s['exact_group_non_singleton']:>5d}")

    # Corpus-wide crash-group stats
    total_scc_groups = sum(s["scc_group_count"] for s in summaries)
    total_exact_groups = sum(s["exact_group_count"] for s in summaries)
    print(f"\n  Crash groups (corpus total):  "
          f"SCC groups={total_scc_groups}  exact-set groups={total_exact_groups}")

    # Projects with most/fewest SCC groups
    by_scc = sorted(summaries, key=lambda s: s["scc_group_count"], reverse=True)
    print(f"\n  Most crash groups (SCC):")
    for s in by_scc[:5]:
        print(f"    {s['project']:<16s} {s['scc_group_count']} group(s)  "
              f"(largest: {len(s['scc_groups'][0]) if s['scc_groups'] else 0} threads)")
    print(f"\n  Fewest crash groups (SCC):")
    for s in by_scc[-3:]:
        print(f"    {s['project']:<16s} {s['scc_group_count']} group(s)  "
              f"(largest: {len(s['scc_groups'][0]) if s['scc_groups'] else 0} threads)")

    print(f"\n  Largest mandatory sets (by fraction of threads):")
    for s in by_mand_frac[:5]:
        frac = s["mandatory_max"] / max(s["threads"], 1)
        w = s["worst_crash"]
        print(f"    {s['project']:<16s} {s['mandatory_max']}/{s['threads']} "
              f"({frac:.0%}) via {w['crashed_thread']}")

    print(f"\n  Smallest mandatory sets (by fraction of threads, >1 mandatory):")
    for s in reversed(by_mand_frac):
        if s["mandatory_max"] > 1:
            frac = s["mandatory_max"] / max(s["threads"], 1)
            print(f"    {s['project']:<16s} {s['mandatory_max']}/{s['threads']} ({frac:.0%})")
            break

    if fs_freq:
        print(f"\n  Most frequently filter-skipped objects (across {n_projects} projects):")
        for obj, count in fs_freq.most_common(10):
            print(f"    {obj:<40s} skipped in {count} project(s)")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("projects", nargs="*", help="project names to summarise (default: all)")
    parser.add_argument("--dir", default="test-results", help="results directory (default: test-results)")
    parser.add_argument("--json", action="store_true", help="output machine-readable JSON")
    parser.add_argument("--verbose", "-v", action="store_true", help="per-crash detail table")
    args = parser.parse_args()

    results_dir = Path(args.dir)
    if not results_dir.is_dir():
        print(f"Error: results directory '{results_dir}' not found", file=sys.stderr)
        return 1

    # Discover project files
    if args.projects:
        files = []
        for p in args.projects:
            f = results_dir / f"{p}-analysis.json"
            if not f.exists():
                print(f"Warning: {f} not found, skipping", file=sys.stderr)
                continue
            files.append((p, f))
    else:
        files = sorted((f.stem.replace("-analysis", ""), f)
                       for f in results_dir.glob("*-analysis.json"))

    if not files:
        print("No analysis files found", file=sys.stderr)
        return 1

    summaries = []
    for name, path in files:
        data = load_project(path)
        s = summarise_project(name, data, verbose=args.verbose)
        summaries.append(s)
        if not args.json:
            print_per_project(s)

    if not args.json:
        print_corpus(summaries)
    else:
        json.dump({"projects": summaries}, sys.stdout, indent=2)
        print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
