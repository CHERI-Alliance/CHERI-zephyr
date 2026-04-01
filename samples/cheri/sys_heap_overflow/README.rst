.. zephyr:code-sample:: sys_heap_overflow
   :name: sys_heap_overflow

   CHERI sys_heap overflow

Overview
********

A sample that demonstrates CHERI with an overflow
on an allocated block from sys_heap
It prints the hardware exception to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cheri/sys_heap_overflow
   :host-os: unix
   :board: qemu_riscv32cheri_purecap
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build 1ad963cb67d4 ***

Running CHERI sys_heap overflow sample on qemu_riscv32cheri_smp_zcheripurecap/qemu_virt_riscv32
sys_heap allocation address: 0x800123b0
CHERI addr:   0x800123b0
CHERI base:   0x800123b0
CHERI top:   0x80012448
CHERI length: 152
CHERI tag: 1

max index: 165
Requested memory length: 150

Attempting to overflow sys_heap allocation...
written contents[0]: 0xab
....
written contents[148]: 0xab
written contents[149]: 0xab
written contents[150]: 0xab
written contents[151]: 0xab
E:
E:  mcause: 28, CHERI exception
E:  mtval2type: 65540, CHERI instruction fetch fault
E:  mtval2cause: 65540, CHERI bounds violation
E:      ca0: 80012448    ct0: 04444444
E:      ca1: 00000001    ct1: 22222220
E:      ca2: 00000000    ct2: 7ffffff8
E:      ca3: 00000004    ct3: 0000001c
E:      ca4: ffffffff    ct4: b0000000
E:      ca5: 80002f68    ct5: 00000000
E:      ca6: 0000003f    ct6: 00000000
E:      ca7: ffffffff
E:      csp: 80015260
E:      cra: 800008c8
E:    mepcc: 8000089c
E: mstatus: 00001880
E:
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x800127c0 (unknown)
>>> k_sys_fatal_error_handler: reason=0 esf=0x800151c0
CPU CHERI hardware exception!
Sample finished

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
