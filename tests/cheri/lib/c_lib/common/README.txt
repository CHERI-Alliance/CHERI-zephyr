Title: Minimal libc APIs with CHERI

Description:

This test verifies minimal libc functions that have been modified
for CHERI. It is a variation of some of the tests in tests/lib/c_lib/common.
Additional CHERI tests have been added.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    Ensure the llvm-cheri toolchain is selected and set up, then

    west build -p -b qemu_riscv32cheri_purecap tests/cheri/lib/c_lib/common
    west build -t run

--------------------------------------------------------------------------------

Sample Output:
riscv32cheri_purecap
[0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: riscv32cheri
*** Booting Zephyr OS build 75bb3a0fd12f ***
Running TESTSUITE libc_common_cheri
===================================================================
START - cheri_test_memmove_overlap_backward
 PASS - cheri_test_memmove_overlap_backward in 0.001 seconds
===================================================================
START - cheri_test_memmove_same_offset_alignment_backward
src
CHERI addr:   0x80019040
CHERI base:   0x80019000
CHERI top:   0x80019200
CHERI length: 512
CHERI tag: 1
dst
CHERI addr:   0x80019048
CHERI base:   0x80019000
CHERI top:   0x80019200
CHERI length: 512
CHERI tag: 1
src_end=0x80019060
overlap=1
src_copy
CHERI addr:   0x8001903f
CHERI base:   0x80019000
CHERI top:   0x80019200
CHERI length: 512
CHERI tag: 1
dst_copy
CHERI addr:   0x80019047
CHERI base:   0x80019000
CHERI top:   0x80019200
CHERI length: 512
CHERI tag: 1
 PASS - cheri_test_memmove_same_offset_alignment_backward in 0.009 seconds
===================================================================
START - test_memcpy
 PASS - test_memcpy in 0.001 seconds
===================================================================
START - test_memcpy_cheri_basic
src
CHERI addr:   0x80019254
CHERI base:   0x80019254
CHERI top:   0x80019258
CHERI length: 4
CHERI tag: 1
dst
CHERI addr:   0x80019254
CHERI base:   0x80019254
CHERI top:   0x80019258
CHERI length: 4
CHERI tag: 1
 PASS - test_memcpy_cheri_basic in 0.004 seconds
===================================================================
START - test_memcpy_cheri_partial_capability_copy
src.value1: 123
src.ptr
CHERI addr:   0x800186a0
CHERI base:   0x800186a0
CHERI top:   0x800186a4
CHERI length: 4
CHERI tag: 1
dst.value1: 123
dst.ptr
CHERI addr:   0x0
CHERI base:   0x0
CHERI top:   0xffffffff
CHERI length: 4294967295
CHERI tag: 0
 PASS - test_memcpy_cheri_partial_capability_copy in 0.004 seconds
===================================================================
START - test_memcpy_cheri_same_offset_alignment
src_tail[3] : 36
src->ptr1
CHERI addr:   0x800186a0
CHERI base:   0x800186a0
CHERI top:   0x800186a4
CHERI length: 4
CHERI tag: 1
src->ptr2
CHERI addr:   0x800186a4
CHERI base:   0x800186a4
CHERI top:   0x800186a8
CHERI length: 4
CHERI tag: 1
dst_tail[3] : 36
dst->ptr1
CHERI addr:   0x800186a0
CHERI base:   0x800186a0
CHERI top:   0x800186a4
CHERI length: 4
CHERI tag: 1
dst->ptr2
CHERI addr:   0x800186a4
CHERI base:   0x800186a4
CHERI top:   0x800186a8
CHERI length: 4
CHERI tag: 1
 PASS - test_memcpy_cheri_same_offset_alignment in 0.008 seconds
===================================================================
START - test_memcpy_cheri_struct
src[0].ptr1
CHERI addr:   0x800186a0
CHERI base:   0x800186a0
CHERI top:   0x800186a4
CHERI length: 4
CHERI tag: 1
dst[0].ptr1
CHERI addr:   0x800186a0
CHERI base:   0x800186a0
CHERI top:   0x800186a4
CHERI length: 4
CHERI tag: 1
 PASS - test_memcpy_cheri_struct in 0.005 seconds
===================================================================
START - test_memmove
 PASS - test_memmove in 0.001 seconds
===================================================================
TESTSUITE libc_common_cheri succeeded

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [libc_common_cheri]: pass = 8, fail = 0, skip = 0, total = 8 duration = 0.033 seconds
 - PASS - [libc_common_cheri.cheri_test_memmove_overlap_backward] duration = 0.001 seconds
 - PASS - [libc_common_cheri.cheri_test_memmove_same_offset_alignment_backward] duration = 0.009 seconds
 - PASS - [libc_common_cheri.test_memcpy] duration = 0.001 seconds
 - PASS - [libc_common_cheri.test_memcpy_cheri_basic] duration = 0.004 seconds
 - PASS - [libc_common_cheri.test_memcpy_cheri_partial_capability_copy] duration = 0.004 seconds
 - PASS - [libc_common_cheri.test_memcpy_cheri_same_offset_alignment] duration = 0.008 seconds
 - PASS - [libc_common_cheri.test_memcpy_cheri_struct] duration = 0.005 seconds
 - PASS - [libc_common_cheri.test_memmove] duration = 0.001 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION SUCCESSFUL
