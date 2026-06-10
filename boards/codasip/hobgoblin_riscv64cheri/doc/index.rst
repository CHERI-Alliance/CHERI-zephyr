.. zephyr:board:: hobgoblin_riscv64cheri

Overview
********

The RISCV64CHERI hobgoblin board configuration is used for the RISCV64 and RISCV64CHERI Codasip FPGA board.

Get the Codasip Toolchain and hobgoblin FPGA hardware
**************************
You need to use the Codasip llvm-cheri toolchain.
Set the following to build with west:
``export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri``
``export LLVM_CHERI_TOOLCHAIN_PATH=/path/llvm-cheri-codasip/build``
If you have the Codasip prime-kit available
``export QEMU_BIN_PATH=/path/codasip-embedded-sdk-1.3.0/bin``


Programming and Debugging
*************************

Applications for the ``hobgoblin_riscv64cheri`` board configuration can be built
using west and run using the Codasip prime-kit sdk qemu or converted to flash.bin
for the hardware.

Flashing
========

Flashing is conducted by creating a flash.bin file and then copying to the
on-board SD card. Reboot to run the program. See the Prime-kit user guide.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
