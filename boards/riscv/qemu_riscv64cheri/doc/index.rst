.. _qemu_riscv64cheri:

RISCV64CHERI Emulation (QEMU)
########################

Overview
********

The RISCV64CHERI QEMU board configuration is used to emulate the RISCV64 and RISCV64CHERI architecture.

Get the Toolchain and QEMU
**************************

The minimum version of the `cheri SDK tools
<https://github.com/CTSRD-CHERI/cheribuild/tree/main>`_
with toolchain and QEMU support for the RISCV64 and RISCV64-purecap architecture
Please build CHERI for freestanding baremetal binaries:
`./cheribuild.py freestanding-cheri-sdk -d -v`
To install the run time libraries:
`./cheribuild.py compiler-rt-builtins-baremetal-riscv64`
`./cheribuild.py compiler-rt-builtins-baremetal-riscv64-purecap`
Set the following to build with west:
`export ZEPHYR_TOOLCHAIN_VARIANT=llvm-cheri`
`export LLVM_CHERI_TOOLCHAIN_PATH=/path/cheri/output/sdk`
`export QEMU_BIN_PATH=/path/cheri/output/sdk/bin`


Programming and Debugging
*************************

Applications for the ``qemu_riscv64cheri`` board configuration can be built and run in
the usual way for emulated boards (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

While this board is emulated and you can't "flash" it, you can use this
configuration to run basic Zephyr applications and kernel tests in the QEMU
emulated environment. For example, with the :zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_riscv64cheri
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

        ***** Booting Zephyr OS build zephyr-v3.5.0-1110-gec031029f4ca *****
        threadA: Hello World from cpu 0 on riscv64cheri!
        threadB: Hello World from cpu 0 on riscv64cheri!
        threadA: Hello World from cpu 0 on riscv64cheri!
        threadB: Hello World from cpu 0 on riscv64cheri!
        threadA: Hello World from cpu 0 on riscv64cheri!
        threadB: Hello World from cpu 0 on riscv64cheri!
        threadA: Hello World from cpu 0 on riscv64cheri!
        threadB: Hello World from cpu 0 on riscv64cheri!
        threadA: Hello World from cpu 0 on riscv64cheri!
        threadB: Hello World from cpu 0 on riscv64cheri!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
