#!/usr/bin/env python3
# Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
#
# SPDX-License-Identifier: Apache-2.0
#

import re
import sys
from pathlib import Path

def strip_ansi(s: str) -> str:
    # Remove ANSI color codes
    return re.sub(r'\x1b\[[0-9;]*m', '', s)

def main():
    # Get path args or use default
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("/tmp/twister-out/OUTPUT_STATS.log")
    if not path.is_file():
        print(f"echo 'ERROR: file not found: {path}' 1>&2")
        sys.exit(1)

    text = strip_ansi(path.read_text(encoding="utf-8", errors="replace"))

    # Get results - test configs: "<passed> of <total> executed test configurations passed (<pct>%)"
    cfg_re = re.compile(
        r'(\d+)\s+of\s+(\d+)\s+executed\s+test\s+configurations\s+passed\s+\(([\d.]+)%\)',
        re.IGNORECASE
    )

    # Get results - test cases: "<passed> of <total> executed test cases passed (<pct>%)"
    case_re = re.compile(
        r'(\d+)\s+of\s+(\d+)\s+executed\s+test\s+cases\s+passed\s+\(([\d.]+)%\)',
        re.IGNORECASE
    )

    cfg_m = cfg_re.search(text)
    case_m = case_re.search(text)

    if not cfg_m:
        print("echo 'ERROR: could not find test configurations summary' 1>&2")
        sys.exit(2)
    if not case_m:
        print("echo 'ERROR: could not find test cases summary' 1>&2")
        sys.exit(3)

    cfg_passed, cfg_total, cfg_pct = cfg_m.groups()
    case_passed, case_total, case_pct = case_m.groups()

    # Print as shell-safe assignments
    print(f"CONFIGS_PASSED={cfg_passed}")
    print(f"CONFIGS_TOTAL={cfg_total}")
    print(f"CONFIGS_PCT={cfg_pct}")
    print(f"TESTS_PASSED={case_passed}")
    print(f"TESTS_TOTAL={case_total}")
    print(f"TESTS_PCT={case_pct}")

if __name__ == "__main__":
    main()
