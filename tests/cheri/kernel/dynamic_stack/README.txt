Title: Zephyr dynamic stacks with CHERI memory alignment

Description:

This test verifies that the CHERI-modified dynamic stack api and thread
creation operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    Ensure the llvm-cheri toolchain is selected and set up, then

    west build -p -b qemu_riscv32cheri_zcheripurecap tests/cheri/kernel/dynamic_stack -T cheri.kernel.dynamic.stack.pool
    west build -t run

--------------------------------------------------------------------------------

Sample Output:

Running TESTSUITE cheri_dynamic_stack
===================================================================
START - test_dynamic_pool_stack_bounds_cheri
CONFIG_DYNAMIC_THREAD_PREFER_POOL=y
CONFIG_DYNAMIC_THREAD_STACK_SIZE:2048
Stack: 0 from pool
CHERI addr:   0x8001f000
CHERI base:   0x8001f000
CHERI top:   0x8001f800
CHERI length: 2048
CHERI tag: 1
Stack: 1 from pool
CHERI addr:   0x8001f800
CHERI base:   0x8001f800
CHERI top:   0x80020000
CHERI length: 2048
CHERI tag: 1
........spawn threads........
Inside thread inspecting running stack capability
requested stack size: 2048
expected bounds size: 2048
Thread capability stack pointer (CSP):
CHERI addr:   0x8001f7b0
CHERI base:   0x8001f000
CHERI top:   0x8001f800
CHERI length: 2048
CHERI tag: 1
Inside thread inspecting running stack capability
requested stack size: 2048
expected bounds size: 2048
Thread capability stack pointer (CSP):
CHERI addr:   0x8001ffb0
CHERI base:   0x8001f800
CHERI top:   0x80020000
CHERI length: 2048
CHERI tag: 1
 PASS - test_dynamic_pool_stack_bounds_cheri in 0.015 seconds
===================================================================
START - test_dynamic_sysheap_stack_bounds_cheri
 SKIP - test_dynamic_sysheap_stack_bounds_cheri in 0.002 seconds
===================================================================
TESTSUITE cheri_dynamic_stack succeeded

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [cheri_dynamic_stack]: pass = 1, fail = 0, skip = 1, total = 2 duration = 0.017 seconds
 - PASS - [cheri_dynamic_stack.test_dynamic_pool_stack_bounds_cheri] duration = 0.015 seconds
 - SKIP - [cheri_dynamic_stack.test_dynamic_sysheap_stack_bounds_cheri] duration = 0.002 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION SUCCESSFUL
