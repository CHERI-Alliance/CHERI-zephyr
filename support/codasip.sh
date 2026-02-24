#!/bin/bash
echo "Setting environment for Codasip toolchain..."
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH=/opt/cheri/llvm-cheri-codasip/build
export QEMU_BIN_PATH=/opt/cheri/qemu-codasip/build
echo "Done."
