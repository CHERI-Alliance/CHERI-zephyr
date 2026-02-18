.. zephyr:code-sample:: mem_blocks_overflow
   :name: mem_blocks_overflow

   CHERI mem_blocks overflow

Overview
********

A sample that demonstrates CHERI with an overflow
on an allocated block from mem_blocks
It prints the hardware exception to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cheri/mem_blocks_overflow
   :host-os: unix
   :board: qemu_riscv32cheri_purecap
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build 78691d9c313c ***

Running CHERI mem_blocks block overflow sample on qemu_riscv32cheri_purecap/qemu_virt_riscv32

mem_block buffer address: 0x80010a30

block size: 16
block address: 0x80010800
CHERI addr:   0x80010800
CHERI base:   0x80010800
CHERI top:   0x80010810
CHERI length: 16
CHERI tag: 1

max index: 20

Attempting to overflow memory block...
written contents[0]: 0xab
written contents[1]: 0xab
written contents[2]: 0xab
written contents[3]: 0xab
written contents[4]: 0xab
written contents[5]: 0xab
written contents[6]: 0xab
written contents[7]: 0xab
written contents[8]: 0xab
written contents[9]: 0xab
written contents[10]: 0xab
written contents[11]: 0xab
written contents[12]: 0xab
written contents[13]: 0xab
written contents[14]: 0xab
written contents[15]: 0xab
E:
E:  mcause: 28, CHERI exception
E:  mtval: 321, CHERI length violation
E:      ca0: 80010810    ct0: 80002054
E:      ca1: 00000028    ct1: 55555555
E:      ca2: 00000000    ct2: 33333333
E:      ca3: 00000004    ct3: 0000000b
E:      ca4: ffffffff    ct4: 00000000
E:      ca5: ffffffff    ct5: 00000000
E:      ca6: 00000000    ct6: 00000000
E:      ca7: 0000003b
E:      csp: 80010570
E:      cra: 80000774
E:    mepcc: 80000760
E: mstatus: 00001880
E:
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x8000f2c0 (unknown)
>>> k_sys_fatal_error_handler: reason=0 esf=0x800104d0
CPU CHERI hardware exception!
Sample finished

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
