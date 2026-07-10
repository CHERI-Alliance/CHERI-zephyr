.. zephyr:board:: hobgoblin_riscv32cheri

Overview
********

The RISCV32CHERI hobgoblin board configuration is used for the RISCV32 and RISCV32CHERI Codasip FPGA board.

Get the Codasip Toolchain and hobgoblin FPGA hardware
**************************
You need to use the Codasip llvm-cheri toolchain.
Set the following to build with west:
``export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri``
``export LLVM_CHERI_TOOLCHAIN_PATH=/path/llvm-cheri-codasip/build``
If you have the Codasip prime-kit available
you can run the elf on the supplied qemu
or hardware platform


Programming and Debugging
*************************

Applications for the ``hobgoblin_riscv32cheri`` board configuration can be built
using west and run using the Codasip prime-kit sdk qemu or converted to flash.bin
for the hardware.

Flashing
========

Flashing is conducted by creating a flash.bin file and then copying to the
on-board SD card. Reboot to run the program. See the Prime-kit user guide.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
