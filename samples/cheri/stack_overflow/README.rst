.. zephyr:code-sample:: stack_overflow
   :name: stack_overflow

   CHERI stack overflow

Overview
********

A sample that demonstrates CHERI with a stack overflow.
The stack overflow is triggered during thread entry.
It prints the hardware exception to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/cheri/stack_overflow
   :host-os: unix
   :board: qemu_riscv32cheri_purecap
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

*** Booting Zephyr OS build 78691d9c313c ***

Running CHERI stack overflow sample on qemu_riscv32cheri_purecap/qemu_virt_riscv32
stack size= 2048 bytes, overflow bytes= 6144
Inside thread entry function
Thread capability stack pointer (CSP):
CHERI addr:   0x80011ab0
CHERI base:   0x80011300
CHERI top:   0x80011b00
CHERI length: 2048
CHERI tag: 1
bytes before end of stack: 80
reading stack[0] @ 0x80011ab0 -> 0x50
reading stack[1] @ 0x80011ab1 -> 0x00
reading stack[2] @ 0x80011ab2 -> 0x00
reading stack[3] @ 0x80011ab3 -> 0x00
reading stack[4] @ 0x80011ab4 -> 0x70
reading stack[5] @ 0x80011ab5 -> 0x59
reading stack[6] @ 0x80011ab6 -> 0x50
reading stack[7] @ 0x80011ab7 -> 0x41
reading stack[8] @ 0x80011ab8 -> 0xb7
reading stack[9] @ 0x80011ab9 -> 0x1a
reading stack[10] @ 0x80011aba -> 0x01
reading stack[11] @ 0x80011abb -> 0x80
reading stack[12] @ 0x80011abc -> 0x9f
reading stack[13] @ 0x80011abd -> 0x1b
reading stack[14] @ 0x80011abe -> 0xd0
reading stack[15] @ 0x80011abf -> 0x47
reading stack[16] @ 0x80011ac0 -> 0x47
reading stack[17] @ 0x80011ac1 -> 0x00
reading stack[18] @ 0x80011ac2 -> 0x00
reading stack[19] @ 0x80011ac3 -> 0x00
reading stack[20] @ 0x80011ac4 -> 0xaa
reading stack[21] @ 0x80011ac5 -> 0xaa
reading stack[22] @ 0x80011ac6 -> 0xaa
reading stack[23] @ 0x80011ac7 -> 0xaa
reading stack[24] @ 0x80011ac8 -> 0x00
reading stack[25] @ 0x80011ac9 -> 0x00
reading stack[26] @ 0x80011aca -> 0x00
reading stack[27] @ 0x80011acb -> 0x00
reading stack[28] @ 0x80011acc -> 0x00
reading stack[29] @ 0x80011acd -> 0x00
reading stack[30] @ 0x80011ace -> 0x00
reading stack[31] @ 0x80011acf -> 0x00
reading stack[32] @ 0x80011ad0 -> 0x00
reading stack[33] @ 0x80011ad1 -> 0x00
reading stack[34] @ 0x80011ad2 -> 0x00
reading stack[35] @ 0x80011ad3 -> 0x00
reading stack[36] @ 0x80011ad4 -> 0x00
reading stack[37] @ 0x80011ad5 -> 0x00
reading stack[38] @ 0x80011ad6 -> 0x00
reading stack[39] @ 0x80011ad7 -> 0x00
reading stack[40] @ 0x80011ad8 -> 0x00
reading stack[41] @ 0x80011ad9 -> 0x00
reading stack[42] @ 0x80011ada -> 0x00
reading stack[43] @ 0x80011adb -> 0x00
reading stack[44] @ 0x80011adc -> 0x00
reading stack[45] @ 0x80011add -> 0x00
reading stack[46] @ 0x80011ade -> 0x00
reading stack[47] @ 0x80011adf -> 0x00
reading stack[48] @ 0x80011ae0 -> 0xaa
reading stack[49] @ 0x80011ae1 -> 0xaa
reading stack[50] @ 0x80011ae2 -> 0xaa
reading stack[51] @ 0x80011ae3 -> 0xaa
reading stack[52] @ 0x80011ae4 -> 0xaa
reading stack[53] @ 0x80011ae5 -> 0xaa
reading stack[54] @ 0x80011ae6 -> 0xaa
reading stack[55] @ 0x80011ae7 -> 0xaa
reading stack[56] @ 0x80011ae8 -> 0xd6
reading stack[57] @ 0x80011ae9 -> 0x0d
reading stack[58] @ 0x80011aea -> 0x00
reading stack[59] @ 0x80011aeb -> 0x80
reading stack[60] @ 0x80011aec -> 0x00
reading stack[61] @ 0x80011aed -> 0x92
reading stack[62] @ 0x80011aee -> 0x78
reading stack[63] @ 0x80011aef -> 0x45
reading stack[64] @ 0x80011af0 -> 0xaa
reading stack[65] @ 0x80011af1 -> 0xaa
reading stack[66] @ 0x80011af2 -> 0xaa
reading stack[67] @ 0x80011af3 -> 0xaa
reading stack[68] @ 0x80011af4 -> 0xaa
reading stack[69] @ 0x80011af5 -> 0xaa
reading stack[70] @ 0x80011af6 -> 0xaa
reading stack[71] @ 0x80011af7 -> 0xaa
reading stack[72] @ 0x80011af8 -> 0xaa
reading stack[73] @ 0x80011af9 -> 0xaa
reading stack[74] @ 0x80011afa -> 0xaa
reading stack[75] @ 0x80011afb -> 0xaa
reading stack[76] @ 0x80011afc -> 0xaa
reading stack[77] @ 0x80011afd -> 0xaa
reading stack[78] @ 0x80011afe -> 0xaa
reading stack[79] @ 0x80011aff -> 0xaa
[00:00:00.070,000] <err> os:
[00:00:00.070,000] <err> os:  mcause: 28, CHERI exception
[00:00:00.070,000] <err> os:  mtval: 321, CHERI length violation
[00:00:00.070,000] <err> os:      ca0: 80011b00    ct0: 01010101
[00:00:00.070,000] <err> os:      ca1: 00000028    ct1: 55555555
[00:00:00.070,000] <err> os:      ca2: 00000000    ct2: 33333333
[00:00:00.070,000] <err> os:      ca3: 00000004    ct3: 0000000a
[00:00:00.070,000] <err> os:      ca4: ffffffff    ct4: 00000000
[00:00:00.070,000] <err> os:      ca5: ffffffff    ct5: 00000000
[00:00:00.070,000] <err> os:      ca6: 00000000    ct6: 00000001
[00:00:00.080,000] <err> os:      ca7: 0000003b
[00:00:00.080,000] <err> os:      csp: 80011ab0
[00:00:00.080,000] <err> os:      cra: 80000738
[00:00:00.080,000] <err> os:    mepcc: 80000722
[00:00:00.080,000] <err> os: mstatus: 00001880
[00:00:00.080,000] <err> os:
[00:00:00.080,000] <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
[00:00:00.080,000] <err> os: Current thread: 0x80010cc0 (unknown)
>>> k_sys_fatal_error_handler: reason=0 esf=0x80011a10
CPU CHERI hardware exception!
Sample finished

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
