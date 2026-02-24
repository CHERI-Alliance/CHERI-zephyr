#!/bin/bash
echo "Running codasip twister tests: BASIC ..."
#------------------------------------
#run with the codasip toolchain
#------------------------------------
#llvm-cheri codasip
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH=/opt/cheri/llvm-cheri-codasip/build
export QEMU_BIN_PATH=/opt/cheri/qemu-codasip/build

#64bit
echo "Running 64 bit tests"
./zephyr/scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv64cheri -p qemu_riscv64cheri_smp -p qemu_riscv64cheri_zcheripurecap -p qemu_riscv64cheri_smp_zcheripurecap \
-T zephyr/samples/hello_world -T zephyr/samples/synchronization -T zephyr/samples/philosophers -T zephyr/samples/basic/sys_heap \
-T zephyr/samples/cheri \
-v --short-build-path -O/tmp/twister-out

#32bit
echo "Running 32 bit tests"
./zephyr/scripts/twister --runtime-artifact-cleanup --force-color --inline-logs \
-p qemu_riscv32cheri -p qemu_riscv32cheri_smp -p qemu_riscv32cheri_zcheripurecap -p qemu_riscv32cheri_smp_zcheripurecap \
-T zephyr/samples/hello_world -T zephyr/samples/synchronization -T zephyr/samples/philosophers -T zephyr/samples/basic/sys_heap \
-T zephyr/samples/cheri \
-v --short-build-path -O/tmp/twister-out
echo "Done."
