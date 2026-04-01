.. zephyr:code-sample:: k_heap_overflow
   :name: k_heap_overflow

   CHERI k_heap overflow

Overview
********

A sample that demonstrates CHERI with an overflow
on an allocated block from k_heap
It prints the hardware exception to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cheri/k_heap_overflow
   :host-os: unix
   :board: qemu_riscv32cheri_purecap
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build 1ad963cb67d4 ***

Running CHERI k_heap overflow sample on qemu_riscv32cheri_purecap/qemu_virt_riscv32
k_heap allocation address: 0x80010a30
CHERI addr:   0x80010a30
CHERI base:   0x80010a30
CHERI top:   0x80010ab0
CHERI length: 128
CHERI tag: 1

max index: 143
Requested memory length: 128

Attempting to overflow k_heap allocation...
written contents[0]: 0xab
....
written contents[126]: 0xab
written contents[127]: 0xab
E:
E:  mcause: 28, CHERI exception
E:  mtval: 321, CHERI length violation
E:      ca0: 80010ab0    ct0: 8000212a
E:      ca1: 00000028    ct1: 55555555
E:      ca2: 00000000    ct2: 33333333
E:      ca3: 00000004    ct3: 0000000b
E:      ca4: ffffffff    ct4: 00000000
E:      ca5: ffffffff    ct5: 00000000
E:      ca6: 00000000    ct6: 00000000
E:      ca7: 0000003b
E:      csp: 80010960
E:      cra: 8000077c
E:    mepcc: 80000756
E: mstatus: 00001880
E:
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x8000f7a0 (unknown)
>>> k_sys_fatal_error_handler: reason=0 esf=0x800108c0
CPU CHERI hardware exception!
Sample finished

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
