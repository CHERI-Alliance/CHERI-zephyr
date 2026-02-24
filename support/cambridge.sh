#!/bin/bash
echo "Setting environment for Cambridge toolchain..."
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri
export LLVM_CHERI_TOOLCHAIN_PATH=/opt/cheri/cheri/output/sdk
export QEMU_BIN_PATH=/opt/cheri/cheri/output/sdk/bin
echo "Done."
