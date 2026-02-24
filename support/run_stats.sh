#!/bin/bash
# Copyright (c) 2026 University of Birmingham, Added to support CHERI spec
#
# SPDX-License-Identifier: Apache-2.0
#
#-------------------------------------
# This is a cheri specific STATS test
# covering:
#  tests/kernel
#  tests/arch
#  tests/cheri
#-------------------------------------

parser="/home/user/zephyrproject/parse_twister_stats.py"
base_dir="/home/user/zephyrproject/STATS"

# Create STATS directory if it does not already exist
mkdir -p "$base_dir"

files=(
  "OUTPUT_STATS_CODASIP_32_BIT.log"
  "OUTPUT_STATS_CODASIP_64_BIT.log"
  "OUTPUT_STATS_CHERIBUILD_32_BIT.log"
  "OUTPUT_STATS_CHERIBUILD_64_BIT.log"
)

#WITH CHERI only

#run codasip tests
##llvm-cheri codasip
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH=/opt/cheri/llvm-cheri-codasip/build
export QEMU_BIN_PATH=/opt/cheri/qemu-codasip/build

#32bit
./zephyr/scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv32cheri_zcheripurecap -p qemu_riscv32cheri_smp_zcheripurecap \
-T zephyr/tests/kernel \
-T zephyr/tests/arch \
-T zephyr/tests/cheri \
-v --short-build-path -O/tmp/twister-out \
2>&1 | tee ${base_dir}/${files[0]}

#64bit
./zephyr/scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv64cheri_zcheripurecap -p qemu_riscv64cheri_smp_zcheripurecap \
-T zephyr/tests/kernel \
-T zephyr/tests/arch \
-T zephyr/tests/cheri \
-v --short-build-path -O/tmp/twister-out \
2>&1 | tee ${base_dir}/${files[1]}

#run cheribuild tests
##llvm-cheri cambridge
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH=/opt/cheri/cheri/output/sdk
export QEMU_BIN_PATH=/opt/cheri/cheri/output/sdk/bin

#32bit
./zephyr/scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv32cheri_purecap -p qemu_riscv32cheri_smp_purecap \
-T zephyr/tests/kernel \
-T zephyr/tests/arch \
-T zephyr/tests/cheri \
-v --short-build-path -O/tmp/twister-out \
2>&1 | tee ${base_dir}/${files[2]}

#64bit
./zephyr/scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv64cheri_purecap -p qemu_riscv64cheri_smp_purecap \
-T zephyr/tests/kernel \
-T zephyr/tests/arch \
-T zephyr/tests/cheri \
-v --short-build-path -O/tmp/twister-out \
2>&1 | tee ${base_dir}/${files[3]}

# parse output into a table
for f in "${files[@]}"; do
  path="${base_dir}/${f}"
  # Make a clean prefix like CODASIP_32_BIT, 
  prefix="${f#OUTPUT_STATS_}"
  prefix="${prefix%.log}"

  # Read KEY=VALUE lines from the Python helper and assign with a prefix
  while IFS='=' read -r key val; do
    varname="${prefix}_${key}"   # e.g., CODASIP_32_BIT_CONFIGS_PASSED
    printf -v "$varname" '%s' "$val"
  done < <(python3 "$parser" "$path")
done

# print a concise summary
echo "====================================="
echo " CHERI‑RISC‑V Twister Test Results"
echo " arch + kernel + cheri"
echo "====================================="
for p in CODASIP_32_BIT CODASIP_64_BIT CHERIBUILD_32_BIT CHERIBUILD_64_BIT; do
  echo "[$p]"
  eval "echo \"  Test Configurations: \${${p}_CONFIGS_PASSED}/\${${p}_CONFIGS_TOTAL} (\${${p}_CONFIGS_PCT}%)\""
  eval "echo \"  Test Cases:   \${${p}_TESTS_PASSED}/\${${p}_TESTS_TOTAL} (\${${p}_TESTS_PCT}%)\""
done

