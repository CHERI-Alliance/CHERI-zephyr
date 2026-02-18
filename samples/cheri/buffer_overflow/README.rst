.. zephyr:code-sample:: buffer_overflow
   :name: buffer_overflow

   CHERI buffer overflow

Overview
********

A sample that demonstrates CHERI with a buffer overflow.
It prints the hardware exception to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cheri/buffer_overflow
   :host-os: unix
   :board: qemu_riscv32cheri_purecap
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build 78691d9c313c ***

Running CHERI buffer overflow sample on qemu_riscv32cheri_purecap/qemu_virt_riscv32

buffer address: 0x8000fe7c
buffer size: 4
CHERI addr:   0x8000fe7c
CHERI base:   0x8000fe7c
CHERI top:   0x8000fe80
CHERI length: 4
CHERI tag: 1

max index: 8

Attempting to overflow buffer...
written contents[0]: 0xab
written contents[1]: 0xab
written contents[2]: 0xab
written contents[3]: 0xab
E:
E:  mcause: 28, CHERI exception
E:  mtval: 321, CHERI length violation
E:      ca0: 8000fe80    ct0: 80001aa8
E:      ca1: 00000028    ct1: 55555555
E:      ca2: 00000000    ct2: 33333333
E:      ca3: 00000004    ct3: 0000000b
E:      ca4: ffffffff    ct4: 00000000
E:      ca5: ffffffff    ct5: 00000000
E:      ca6: 00000000    ct6: 00000000
E:      ca7: 0000003b
E:      csp: 8000fe70
E:      cra: 8000072e
E:    mepcc: 8000071a
E: mstatus: 00001880
E:
E: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
E: Current thread: 0x8000ebe0 (unknown)
>>> k_sys_fatal_error_handler: reason=0 esf=0x8000fdd0
CPU CHERI hardware exception!
Sample finished

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
