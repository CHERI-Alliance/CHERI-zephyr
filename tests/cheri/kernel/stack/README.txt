Title: Zephyr stacks with CHERI memory alignment

Description:

This test verifies that the CHERI-modified kernel stack macros and thread
creation operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    Ensure the llvm-cheri toolchain is selected and set up, then

    west build -p -b qemu_riscv32cheri_zcheripurecap tests/cheri/kernel/stack -T cheri.kernel.stack
    west build -t run

--------------------------------------------------------------------------------

Sample Output:

Running TESTSUITE cheri_stack
===================================================================
START - test_a_stack_bounds_cheri
Testing CHERI modified stack macros in thread_stack.h
Checking alignment and array sizes
=== user_stack ===
Raw base pointer: 0x8001ac20
CHERI addr:   0x8001ac20
CHERI base:   0x8001ac20
CHERI top:   0x8001b030
CHERI length: 1040
CHERI tag: 1
Stack allocated size (bytes): 1040
=== kernel_stack ===
Raw base pointer: 0x8001b0a0
CHERI addr:   0x8001b0a0
CHERI base:   0x8001b0a0
CHERI top:   0x8001b4b0
CHERI length: 1040
CHERI tag: 1
Stack allocated size (bytes): 1040
=== user_stack_array[0] ===
Raw base pointer: 0x80017820
CHERI addr:   0x80017820
CHERI base:   0x80017820
CHERI top:   0x80018460
CHERI length: 3136
CHERI tag: 1
Stack allocated size (bytes): 1040
=== user_stack_array[1] ===
Raw base pointer: 0x80017c30
CHERI addr:   0x80017c30
CHERI base:   0x80017820
CHERI top:   0x80018460
CHERI length: 3136
CHERI tag: 1
Stack allocated size (bytes): 1040
=== user_stack_array[2] ===
Raw base pointer: 0x80018040
CHERI addr:   0x80018040
CHERI base:   0x80017820
CHERI top:   0x80018460
CHERI length: 3136
CHERI tag: 1
Stack allocated size (bytes): 1040
=== kernel_stack_array[0] ===
Raw base pointer: 0x80018520
CHERI addr:   0x80018520
CHERI base:   0x80018520
CHERI top:   0x80019160
CHERI length: 3136
CHERI tag: 1
Stack allocated size (bytes): 1040
=== kernel_stack_array[1] ===
Raw base pointer: 0x80018930
CHERI addr:   0x80018930
CHERI base:   0x80018520
CHERI top:   0x80019160
CHERI length: 3136
CHERI tag: 1
Stack allocated size (bytes): 1040
=== kernel_stack_array[2] ===
Raw base pointer: 0x80018d40
CHERI addr:   0x80018d40
CHERI base:   0x80018520
CHERI top:   0x80019160
CHERI length: 3136
CHERI tag: 1
Stack allocated size (bytes): 1040
 PASS - test_a_stack_bounds_cheri in 0.026 seconds
===================================================================
START - test_b_stack_threads_bounds_cheri
Testing tight bounds of individual array stacks in riscv/core/thread.c
=== Creating threads on user_stack_array ===
stack array [0]
Inside thread inspecting running stack capability
Thread capability stack pointer (CSP):
CHERI addr:   0x80017be0
CHERI base:   0x80017820
CHERI top:   0x80017c30
CHERI length: 1040
CHERI tag: 1
stack array [1]
Inside thread inspecting running stack capability
Thread capability stack pointer (CSP):
CHERI addr:   0x80017ff0
CHERI base:   0x80017c30
CHERI top:   0x80018040
CHERI length: 1040
CHERI tag: 1
stack array [2]
Inside thread inspecting running stack capability
Thread capability stack pointer (CSP):
CHERI addr:   0x80018400
CHERI base:   0x80018040
CHERI top:   0x80018450
CHERI length: 1040
CHERI tag: 1
=== Creating threads on kernel_stack_array ===
stack array [0]
Inside thread inspecting running stack capability
Thread capability stack pointer (CSP):
CHERI addr:   0x800188e0
CHERI base:   0x80018520
CHERI top:   0x80018930
CHERI length: 1040
CHERI tag: 1
stack array [1]
Inside thread inspecting running stack capability
Thread capability stack pointer (CSP):
CHERI addr:   0x80018cf0
CHERI base:   0x80018930
CHERI top:   0x80018d40
CHERI length: 1040
CHERI tag: 1
stack array [2]
Inside thread inspecting running stack capability
Thread capability stack pointer (CSP):
CHERI addr:   0x80019100
CHERI base:   0x80018d40
CHERI top:   0x80019150
CHERI length: 1040
CHERI tag: 1
 PASS - test_b_stack_threads_bounds_cheri in 0.022 seconds
===================================================================
START - test_c_idle_stack_bounds_cheri
Inspecting the idle stacks
Checking alignment and array sizes
idle stack
CHERI addr:   0x8001b520
CHERI base:   0x8001b520
CHERI top:   0x8001b720
CHERI length: 512
CHERI tag: 1
logical stack size 512
 PASS - test_c_idle_stack_bounds_cheri in 0.004 seconds
===================================================================
START - test_d_idle_csp_bounds_cheri
Inspecting the idle stacks stack pointer in thread
 PASS - test_d_idle_csp_bounds_cheri in 0.020 seconds
===================================================================
TESTSUITE cheri_stack succeeded

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [cheri_stack]: pass = 4, fail = 0, skip = 0, total = 4 duration = 0.072 seconds
 - PASS - [cheri_stack.test_a_stack_bounds_cheri] duration = 0.026 seconds
 - PASS - [cheri_stack.test_b_stack_threads_bounds_cheri] duration = 0.022 seconds
 - PASS - [cheri_stack.test_c_idle_stack_bounds_cheri] duration = 0.004 seconds
 - PASS - [cheri_stack.test_d_idle_csp_bounds_cheri] duration = 0.020 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION SUCCESSFUL
