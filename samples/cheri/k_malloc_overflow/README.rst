.. zephyr:code-sample:: k_malloc_overflow
   :name: k_malloc_overflow

   CHERI k_malloc overflow

Overview
********

A sample that demonstrates CHERI with an overflow
on an allocated block from the global MEM_POOL
It prints the hardware exception to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cheri/k_malloc_overflow
   :host-os: unix
   :board: qemu_riscv32cheri_purecap
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build 89280cddd284 ***

Running CHERI k_malloc overflow sample on qemu_riscv32cheri_zcheripurecap/qemu_virt_riscv32
k_malloc allocation address: 0x80011da0
CHERI addr:   0x80011da0
CHERI base:   0x80011d98
CHERI top:   0x80011e20
CHERI length: 136
CHERI tag: 1

max index: 143
Requested memory length: 128

Attempting to overflow k_malloc allocation...
written contents[0]: 0xab
....
written contents[126]: 0xab
written contents[127]: 0xab
E:
E:  mcause: 28, CHERI exception
E:  mtval2type: 65540, CHERI instruction fetch fault
E:  mtval2cause: 65540, CHERI bounds violation
E:      ca0: 80011e20    ct0: 04444444
E:      ca1: 00000028    ct1: 22222220
E:      ca2: 00000000    ct2: 7ffffff8
E:      ca3: 00000004    ct3: 0000001c
E:      ca4: ffffffff    ct4: b0000000
E:      ca5: 80002d9c    ct5: 00000000
E:      ca6: 0000003f    ct6: 00000000
E:      ca7: ffffffff
E:      csp: 80011ac0
E:      cra: 80000868
E:    mepcc: 8000083c
E: mstatus: 00001880
E:
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x80010900 (unknown)
>>> k_sys_fatal_error_handler: reason=0 esf=0x80011a20
CPU CHERI hardware exception!
Sample finished

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
