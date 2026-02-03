Title: Memory slab APIs with CHERI memory alignment

Description:

This test verifies that the CHERI-modified kernel memory slab APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    Ensure the llvm-cheri toolchain is selected and set up, then

    west build -p -b qemu_riscv32cheri_purecap tests/cheri/kernel/mem_slab/mslab_slab_bounds -T cheri.kernel.memory_slabs_bounds
    west build -t run

--------------------------------------------------------------------------------

Sample Output:

Running TESTSUITE test_mem_slab_cheri
===================================================================
START - test_a_scan_cheri_alignment_thresholds
scanning 32 bit alignment thresholds
#define CHERI_ALIGN_TABLE(X) \
  X(0, 1) /* initial */ \
  X(64, 8) \
  X(121, 16) \
  X(241, 32) \
  X(481, 64) \
  X(961, 128) \
  X(1921, 256) \
  X(3841, 512) \
  X(7681, 1024) \
  X(15361, 2048) \
  X(30721, 4096) \
  X(61441, 8192) \
  X(122881, 16384) \
  X(245761, 32768) \
  X(491521, 65536) \
  X(983041, 131072) \
  X(1966081, 262144) \
  X(3932161, 524288) \
  X(7864321, 1048576) \
  X(15728641, 2097152) \

 PASS - test_a_scan_cheri_alignment_thresholds in 10.746 seconds
===================================================================
START - test_b_cheri_macros
testing boundary 1
testing boundary 2
testing boundary 3
testing boundary 4
testing boundary 5
 PASS - test_b_cheri_macros in 0.180 seconds
===================================================================
START - test_c_init_slab_bounds
Checking k_mem_slab_init slab buffers are 32 bit CHERI aligned and tightly bound........
Slab init_slab_struct1, test 256 - 512 bytes........
  Requested block alignment: 4, after WB_UP: 8, CHERI builtins: 8
  Requested block size: 68, after WB_UP: 72, after CHERI builtins rounding during init: 72
  Requested slab alignment: 4, after WB_UP: 8, expected CHERI builtins: 32, needed from (block_len_rep *num_blks): 32
  Requested slab size: 340, after WB_UP: 360, block_len_rep*num_blks: 360, expected CHERI builtins: 384, needed rounded(block_len_rep*num_blks): 384
    Results in buffer: 0x80db3c40 with bounds 0x80db3c40 to 0x80db3dc0, and size 384 bytes, perms: 0x47d, tag: 0x1
Slab init_slab_struct2, test 512 bytes - 1 KiB........
  Requested block alignment: 4, after WB_UP: 8, CHERI builtins: 16
  Requested block size: 228, after WB_UP: 232, after CHERI builtins rounding during init: 240
  Requested slab alignment: 4, after WB_UP: 8, expected CHERI builtins: 64, needed from (block_len_rep *num_blks): 64
  Requested slab size: 912, after WB_UP: 928, block_len_rep*num_blks: 960, expected CHERI builtins: 960, needed rounded(block_len_rep*num_blks): 960
    Results in buffer: 0x80db3880 with bounds 0x80db3880 to 0x80db3c40, and size 960 bytes, perms: 0x47d, tag: 0x1
Slab init_slab_struct3, test 1 KiB - 2kiB........
  Requested block alignment: 4, after WB_UP: 8, CHERI builtins: 32
  Requested block size: 468, after WB_UP: 472, after CHERI builtins rounding during init: 480
  Requested slab alignment: 4, after WB_UP: 8, expected CHERI builtins: 128, needed from (block_len_rep *num_blks): 128
  Requested slab size: 1872, after WB_UP: 1888, block_len_rep*num_blks: 1920, expected CHERI builtins: 1920, needed rounded(block_len_rep*num_blks): 1920
    Results in buffer: 0x80db3100 with bounds 0x80db3100 to 0x80db3880, and size 1920 bytes, perms: 0x47d, tag: 0x1
Slab init_slab_struct4, test 2 KiB - 4 KiB........
  Requested block alignment: 4, after WB_UP: 8, CHERI builtins: 64
  Requested block size: 908, after WB_UP: 912, after CHERI builtins rounding during init: 960
  Requested slab alignment: 4, after WB_UP: 8, expected CHERI builtins: 256, needed from (block_len_rep *num_blks): 256
  Requested slab size: 3632, after WB_UP: 3648, block_len_rep*num_blks: 3840, expected CHERI builtins: 3840, needed rounded(block_len_rep*num_blks): 3840
    Results in buffer: 0x80db2200 with bounds 0x80db2200 to 0x80db3100, and size 3840 bytes, perms: 0x47d, tag: 0x1
Slab init_slab_struct5, test 4 KiB - 8 KiB........
  Requested block alignment: 8, after WB_UP: 8, CHERI builtins: 128
  Requested block size: 1068, after WB_UP: 1072, after CHERI builtins rounding during init: 1152
  Requested slab alignment: 8, after WB_UP: 8, expected CHERI builtins: 512, needed from (block_len_rep *num_blks): 512
  Requested slab size: 4272, after WB_UP: 4288, block_len_rep*num_blks: 4608, expected CHERI builtins: 4608, needed rounded(block_len_rep*num_blks): 4608
    Results in buffer: 0x80db1000 with bounds 0x80db1000 to 0x80db2200, and size 4608 bytes, perms: 0x47d, tag: 0x1
Slab init_slab_struct6, test 32 KiB - 64 KiB........
  Requested block alignment: 16, after WB_UP: 16, CHERI builtins: 1024
  Requested block size: 9168, after WB_UP: 9168, after CHERI builtins rounding during init: 9216
  Requested slab alignment: 16, after WB_UP: 16, expected CHERI builtins: 4096, needed from (block_len_rep *num_blks): 4096
  Requested slab size: 36672, after WB_UP: 36672, block_len_rep*num_blks: 36864, expected CHERI builtins: 36864, needed rounded(block_len_rep*num_blks): 36864
    Results in buffer: 0x80da8000 with bounds 0x80da8000 to 0x80db1000, and size 36864 bytes, perms: 0x47d, tag: 0x1
Slab init_slab_struct7, test 128 KiB - 256 KiB........
  Requested block alignment: 16, after WB_UP: 16, CHERI builtins: 4096
  Requested block size: 38240, after WB_UP: 38240, after CHERI builtins rounding during init: 40960
  Requested slab alignment: 16, after WB_UP: 16, expected CHERI builtins: 16384, needed from (block_len_rep *num_blks): 16384
  Requested slab size: 152960, after WB_UP: 152960, block_len_rep*num_blks: 163840, expected CHERI builtins: 163840, needed rounded(block_len_rep*num_blks): 163840
    Results in buffer: 0x80d80000 with bounds 0x80d80000 to 0x80da8000, and size 163840 bytes, perms: 0x47d, tag: 0x1
Slab init_slab_struct8, test 512 KiB - 1 MiB........
  Requested block alignment: 16, after WB_UP: 16, CHERI builtins: 16384
  Requested block size: 131072, after WB_UP: 131072, after CHERI builtins rounding during init: 131072
  Requested slab alignment: 16, after WB_UP: 16, expected CHERI builtins: 65536, needed from (block_len_rep *num_blks): 65536
  Requested slab size: 524288, after WB_UP: 524288, block_len_rep*num_blks: 524288, expected CHERI builtins: 524288, needed rounded(block_len_rep*num_blks): 524288
    Results in buffer: 0x80d00000 with bounds 0x80d00000 to 0x80d80000, and size 524288 bytes, perms: 0x47d, tag: 0x1
Slab init_slab_struct9, test 8 MiB  - 16 MiB........
  Requested block alignment: 16, after WB_UP: 16, CHERI builtins: 262144
  Requested block size: 2830240, after WB_UP: 2830240, after CHERI builtins rounding during init: 2883584
  Requested slab alignment: 16, after WB_UP: 16, expected CHERI builtins: 1048576, needed from (block_len_rep *num_blks): 1048576
  Requested slab size: 11320960, after WB_UP: 11320960, block_len_rep*num_blks: 11534336, expected CHERI builtins: 11534336, needed rounded(block_len_rep*num_blks): 11534336
    Results in buffer: 0x80200000 with bounds 0x80200000 to 0x80d00000, and size 11534336 bytes, perms: 0x47d, tag: 0x1
checking init_slab_struct1.buffer.....
actual_slab_len: 384, expected_slab_rep_len: 384, needed_slab_rep_len: 384
checking init_slab_struct2.buffer.....
actual_slab_len: 960, expected_slab_rep_len: 960, needed_slab_rep_len: 960
checking init_slab_struct3.buffer.....
actual_slab_len: 1920, expected_slab_rep_len: 1920, needed_slab_rep_len: 1920
checking init_slab_struct4.buffer.....
actual_slab_len: 3840, expected_slab_rep_len: 3840, needed_slab_rep_len: 3840
checking init_slab_struct5.buffer.....
actual_slab_len: 4608, expected_slab_rep_len: 4608, needed_slab_rep_len: 4608
checking init_slab_struct6.buffer.....
actual_slab_len: 36864, expected_slab_rep_len: 36864, needed_slab_rep_len: 36864
checking init_slab_struct7.buffer.....
actual_slab_len: 163840, expected_slab_rep_len: 163840, needed_slab_rep_len: 163840
checking init_slab_struct8.buffer.....
actual_slab_len: 524288, expected_slab_rep_len: 524288, needed_slab_rep_len: 524288
checking init_slab_struct9.buffer.....
actual_slab_len: 11534336, expected_slab_rep_len: 11534336, needed_slab_rep_len: 11534336
 PASS - test_c_init_slab_bounds in 0.074 seconds
===================================================================
START - test_d_init_block_bounds
Checking block allocations for 32 bit CHERI are aligned and tightly bound........
Block allocation from init_slab_struct1, test 256 - 512 bytes.....
  Requested length of each block: = 68 bytes, after WB_UP: 72 bytes
  Expected length of each block after CHERI builtins rounding during init: = 72 bytes
   block: ptr[0] = 0x80db3c40 with bounds 0x80db3c40 to 0x80db3c88 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80db3c88 with bounds 0x80db3c88 to 0x80db3cd0 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80db3cd0 with bounds 0x80db3cd0 to 0x80db3d18 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80db3d18 with bounds 0x80db3d18 to 0x80db3d60 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x80db3d60 with bounds 0x80db3d60 to 0x80db3da8 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from init_slab_struct2, test 512 bytes - 1 KiB.....
  Requested length of each block: = 228 bytes, after WB_UP: 232 bytes
  Expected length of each block after CHERI builtins rounding during init: = 240 bytes
   block: ptr[0] = 0x80db3880 with bounds 0x80db3880 to 0x80db3970 and size 240 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80db3970 with bounds 0x80db3970 to 0x80db3a60 and size 240 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80db3a60 with bounds 0x80db3a60 to 0x80db3b50 and size 240 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80db3b50 with bounds 0x80db3b50 to 0x80db3c40 and size 240 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from init_slab_struct3, test 1 KiB - 2kiB.....
  Requested length of each block: = 468 bytes, after WB_UP: 472 bytes
  Expected length of each block after CHERI builtins rounding during init: = 480 bytes
   block: ptr[0] = 0x80db3100 with bounds 0x80db3100 to 0x80db32e0 and size 480 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80db32e0 with bounds 0x80db32e0 to 0x80db34c0 and size 480 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80db34c0 with bounds 0x80db34c0 to 0x80db36a0 and size 480 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80db36a0 with bounds 0x80db36a0 to 0x80db3880 and size 480 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from init_slab_struct4, test 2 KiB - 4 KiB.....
  Requested length of each block: = 908 bytes, after WB_UP: 912 bytes
  Expected length of each block after CHERI builtins rounding during init: = 960 bytes
   block: ptr[0] = 0x80db2200 with bounds 0x80db2200 to 0x80db25c0 and size 960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80db25c0 with bounds 0x80db25c0 to 0x80db2980 and size 960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80db2980 with bounds 0x80db2980 to 0x80db2d40 and size 960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80db2d40 with bounds 0x80db2d40 to 0x80db3100 and size 960 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from init_slab_struct5, test 4 KiB - 8 KiB.....
  Requested length of each block: = 1068 bytes, after WB_UP: 1072 bytes
  Expected length of each block after CHERI builtins rounding during init: = 1152 bytes
   block: ptr[0] = 0x80db1000 with bounds 0x80db1000 to 0x80db1480 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80db1480 with bounds 0x80db1480 to 0x80db1900 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80db1900 with bounds 0x80db1900 to 0x80db1d80 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80db1d80 with bounds 0x80db1d80 to 0x80db2200 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from init_slab_struct6, test 32 KiB - 64 KiB.....
  Requested length of each block: = 9168 bytes, after WB_UP: 9168 bytes
  Expected length of each block after CHERI builtins rounding during init: = 9216 bytes
   block: ptr[0] = 0x80da8000 with bounds 0x80da8000 to 0x80daa400 and size 9216 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80daa400 with bounds 0x80daa400 to 0x80dac800 and size 9216 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80dac800 with bounds 0x80dac800 to 0x80daec00 and size 9216 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80daec00 with bounds 0x80daec00 to 0x80db1000 and size 9216 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from init_slab_struct7, test 128 KiB - 256 KiB.....
  Requested length of each block: = 38240 bytes, after WB_UP: 38240 bytes
  Expected length of each block after CHERI builtins rounding during init: = 40960 bytes
   block: ptr[0] = 0x80d80000 with bounds 0x80d80000 to 0x80d8a000 and size 40960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80d8a000 with bounds 0x80d8a000 to 0x80d94000 and size 40960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80d94000 with bounds 0x80d94000 to 0x80d9e000 and size 40960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80d9e000 with bounds 0x80d9e000 to 0x80da8000 and size 40960 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from init_slab_struct8, test 512 KiB - 1 MiB.....
  Requested length of each block: = 131072 bytes, after WB_UP: 131072 bytes
  Expected length of each block after CHERI builtins rounding during init: = 131072 bytes
   block: ptr[0] = 0x80d00000 with bounds 0x80d00000 to 0x80d20000 and size 131072 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80d20000 with bounds 0x80d20000 to 0x80d40000 and size 131072 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80d40000 with bounds 0x80d40000 to 0x80d60000 and size 131072 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80d60000 with bounds 0x80d60000 to 0x80d80000 and size 131072 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from init_slab_struct9, test 8 MiB  - 16 MiB.....
  Requested length of each block: = 2830240 bytes, after WB_UP: 2830240 bytes
  Expected length of each block after CHERI builtins rounding during init: = 2883584 bytes
   block: ptr[0] = 0x80200000 with bounds 0x80200000 to 0x804c0000 and size 2883584 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x804c0000 with bounds 0x804c0000 to 0x80780000 and size 2883584 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80780000 with bounds 0x80780000 to 0x80a40000 and size 2883584 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80a40000 with bounds 0x80a40000 to 0x80d00000 and size 2883584 bytes, tag: 0x1, checking....alignment: good, block length: good
Freeing blocks to init_slab_struct1, test 256 bytes - 512 bytes.....
  freed block: ptr[0] = 0x80db3c40 with bounds 0x80db3c40 to 0x80db3c88 and size 72 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80db3c88 with bounds 0x80db3c88 to 0x80db3cd0 and size 72 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80db3cd0 with bounds 0x80db3cd0 to 0x80db3d18 and size 72 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80db3d18 with bounds 0x80db3d18 to 0x80db3d60 and size 72 bytes, tag: 0x1,
  freed block: ptr[4] = 0x80db3d60 with bounds 0x80db3d60 to 0x80db3da8 and size 72 bytes, tag: 0x1,
Freeing blocks to init_slab_struct2, test 512 bytes - 1 KiB.....
  freed block: ptr[0] = 0x80db3880 with bounds 0x80db3880 to 0x80db3970 and size 240 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80db3970 with bounds 0x80db3970 to 0x80db3a60 and size 240 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80db3a60 with bounds 0x80db3a60 to 0x80db3b50 and size 240 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80db3b50 with bounds 0x80db3b50 to 0x80db3c40 and size 240 bytes, tag: 0x1,
Freeing blocks to init_slab_struct3, test 1 KiB - 2kiB.....
  freed block: ptr[0] = 0x80db3100 with bounds 0x80db3100 to 0x80db32e0 and size 480 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80db32e0 with bounds 0x80db32e0 to 0x80db34c0 and size 480 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80db34c0 with bounds 0x80db34c0 to 0x80db36a0 and size 480 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80db36a0 with bounds 0x80db36a0 to 0x80db3880 and size 480 bytes, tag: 0x1,
Freeing blocks to init_slab_struct4, test 2 KiB - 4 KiB.....
  freed block: ptr[0] = 0x80db2200 with bounds 0x80db2200 to 0x80db25c0 and size 960 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80db25c0 with bounds 0x80db25c0 to 0x80db2980 and size 960 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80db2980 with bounds 0x80db2980 to 0x80db2d40 and size 960 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80db2d40 with bounds 0x80db2d40 to 0x80db3100 and size 960 bytes, tag: 0x1,
Freeing blocks to init_slab_struct5, test 4 KiB - 8 KiB.....
  freed block: ptr[0] = 0x80db1000 with bounds 0x80db1000 to 0x80db1480 and size 1152 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80db1480 with bounds 0x80db1480 to 0x80db1900 and size 1152 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80db1900 with bounds 0x80db1900 to 0x80db1d80 and size 1152 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80db1d80 with bounds 0x80db1d80 to 0x80db2200 and size 1152 bytes, tag: 0x1,
Freeing blocks to init_slab_struct6, test 32 KiB - 64 KiB.....
  freed block: ptr[0] = 0x80da8000 with bounds 0x80da8000 to 0x80daa400 and size 9216 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80daa400 with bounds 0x80daa400 to 0x80dac800 and size 9216 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80dac800 with bounds 0x80dac800 to 0x80daec00 and size 9216 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80daec00 with bounds 0x80daec00 to 0x80db1000 and size 9216 bytes, tag: 0x1,
Freeing blocks to init_slab_struct7, test 128 KiB - 256 KiB.....
  freed block: ptr[0] = 0x80d80000 with bounds 0x80d80000 to 0x80d8a000 and size 40960 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80d8a000 with bounds 0x80d8a000 to 0x80d94000 and size 40960 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80d94000 with bounds 0x80d94000 to 0x80d9e000 and size 40960 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80d9e000 with bounds 0x80d9e000 to 0x80da8000 and size 40960 bytes, tag: 0x1,
Freeing blocks to init_slab_struct8, test 512 KiB - 1 MiB.....
  freed block: ptr[0] = 0x80d00000 with bounds 0x80d00000 to 0x80d20000 and size 131072 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80d20000 with bounds 0x80d20000 to 0x80d40000 and size 131072 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80d40000 with bounds 0x80d40000 to 0x80d60000 and size 131072 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80d60000 with bounds 0x80d60000 to 0x80d80000 and size 131072 bytes, tag: 0x1,
Freeing blocks to init_slab_struct9, test 8 MiB - 16 MiB.....
  freed block: ptr[0] = 0x80200000 with bounds 0x80200000 to 0x804c0000 and size 2883584 bytes, tag: 0x1,
  freed block: ptr[1] = 0x804c0000 with bounds 0x804c0000 to 0x80780000 and size 2883584 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80780000 with bounds 0x80780000 to 0x80a40000 and size 2883584 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80a40000 with bounds 0x80a40000 to 0x80d00000 and size 2883584 bytes, tag: 0x1,
Repeat Block allocation from init_slab_struct5, test 4 KiB - 8 KiB.....
  Requested length of each block: = 1068 bytes, after WB_UP: 1072 bytes
  Expected length of each block after CHERI builtins rounding during init: = 1152 bytes
   block: ptr[0] = 0x80db1d80 with bounds 0x80db1d80 to 0x80db2200 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80db1900 with bounds 0x80db1900 to 0x80db1d80 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80db1480 with bounds 0x80db1480 to 0x80db1900 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80db1000 with bounds 0x80db1000 to 0x80db1480 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
Repeat Freeing blocks to init_slab_struct5, test 4 KiB - 8 KiB.....
  freed block: ptr[0] = 0x80db1d80 with bounds 0x80db1d80 to 0x80db2200 and size 1152 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80db1900 with bounds 0x80db1900 to 0x80db1d80 and size 1152 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80db1480 with bounds 0x80db1480 to 0x80db1900 and size 1152 bytes, tag: 0x1,
  freed block: ptr[3] = 0x80db1000 with bounds 0x80db1000 to 0x80db1480 and size 1152 bytes, tag: 0x1,
 PASS - test_d_init_block_bounds in 0.185 seconds
===================================================================
START - test_e_get_slab_length_edge_cases
x = cheri_round( cheri_round(len) * num_blks )
y = cheri_round( len * num_blks )
Extracting 32 bit CHERI representable buffer lengths that differ........
len: 129, WB_UP(len): 136, num_blks: 3, x: 448, y: 416
len: 130, WB_UP(len): 136, num_blks: 3, x: 448, y: 416
len: 131, WB_UP(len): 136, num_blks: 3, x: 448, y: 416
len: 132, WB_UP(len): 136, num_blks: 3, x: 448, y: 416
len: 133, WB_UP(len): 136, num_blks: 3, x: 448, y: 416
len: 134, WB_UP(len): 136, num_blks: 3, x: 448, y: 416
len: 135, WB_UP(len): 136, num_blks: 3, x: 448, y: 416
len: 136, WB_UP(len): 136, num_blks: 3, x: 448, y: 416
len: 161, WB_UP(len): 168, num_blks: 3, x: 576, y: 512
len: 162, WB_UP(len): 168, num_blks: 3, x: 576, y: 512
len: 163, WB_UP(len): 168, num_blks: 3, x: 576, y: 512
 PASS - test_e_get_slab_length_edge_cases in 0.016 seconds
===================================================================
START - test_f_init_slab_edge_cases_exact_fail
Checking k_mem_slab_init slab buffers are 32 bit CHERI aligned and tightly bound........
CHERI block size DOES NOT fit in the slab, returned buffer: (nil)
 PASS - test_f_init_slab_edge_cases_exact_fail in 0.002 seconds
===================================================================
START - test_g_init_slab_edge_cases_exact_macro_pass
Checking k_mem_slab_init slab buffers are 32 bit CHERI aligned and tightly bound........
CHERI block size DOES fit in the slab, returned buffer: 0x80db3f60
 PASS - test_g_init_slab_edge_cases_exact_macro_pass in 0.002 seconds
===================================================================
START - test_h_init_block_edge_cases_exact_macro_pass
Checking k_mem_slab_init allocated blocks are 32 bit CHERI aligned and tightly bound........
Block allocation from init_slab_struct_l2.....
  Requested length of each block: = 136 bytes, after WB_UP: 136 bytes
  Expected length of each block after CHERI builtins rounding during init: = 144 bytes
   block: ptr[0] = 0x80db3f60 with bounds 0x80db3f60 to 0x80db3ff0 and size 144 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80db3ff0 with bounds 0x80db3ff0 to 0x80db4080 and size 144 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80db4080 with bounds 0x80db4080 to 0x80db4110 and size 144 bytes, tag: 0x1, checking....alignment: good, block length: good
Freeing blocks to init_slab_struct_l2, .....
  freed block: ptr[0] = 0x80db3f60 with bounds 0x80db3f60 to 0x80db3ff0 and size 144 bytes, tag: 0x1,
  freed block: ptr[1] = 0x80db3ff0 with bounds 0x80db3ff0 to 0x80db4080 and size 144 bytes, tag: 0x1,
  freed block: ptr[2] = 0x80db4080 with bounds 0x80db4080 to 0x80db4110 and size 144 bytes, tag: 0x1,
 PASS - test_h_init_block_edge_cases_exact_macro_pass in 0.015 seconds
===================================================================
START - test_i_macro_slab_bounds
Checking compile-time backing buffers are 32 bit CHERI aligned and tightly bound........
Slab macro_slab1 test 256 bytes - 512 bytes........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 68, after WB_UP: 72 after macro rounding to CHERI representable length: 72,after CHERI builtins rounding during init: 72
  Requested slab alignment: 4, after WB_UP: 8, and after CHERI aligned: 32
  Requested slab size: 340, after WB_UP: 360 and after round up to CHERI representable length: 384
    Results in buffer: 0x819b3c40 with bounds 0x819b3c40 to 0x819b3dc0, and size 384 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab2 test 512 bytes - 1 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 16
  Requested block size: 228, after WB_UP: 232 after macro rounding to CHERI representable length: 240,after CHERI builtins rounding during init: 240
  Requested slab alignment: 4, after WB_UP: 8, and after CHERI aligned: 64
  Requested slab size: 912, after WB_UP: 928 and after round up to CHERI representable length: 960
    Results in buffer: 0x819b3880 with bounds 0x819b3880 to 0x819b3c40, and size 960 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab3 test 1 KiB - 2 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 32
  Requested block size: 468, after WB_UP: 472 after macro rounding to CHERI representable length: 480,after CHERI builtins rounding during init: 480
  Requested slab alignment: 4, after WB_UP: 8, and after CHERI aligned: 128
  Requested slab size: 1872, after WB_UP: 1888 and after round up to CHERI representable length: 1920
    Results in buffer: 0x819b3100 with bounds 0x819b3100 to 0x819b3880, and size 1920 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab4 test 2 KiB - 4 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 64
  Requested block size: 908, after WB_UP: 912 after macro rounding to CHERI representable length: 960,after CHERI builtins rounding during init: 960
  Requested slab alignment: 4, after WB_UP: 8, and after CHERI aligned: 256
  Requested slab size: 3632, after WB_UP: 3648 and after round up to CHERI representable length: 3840
    Results in buffer: 0x819b2200 with bounds 0x819b2200 to 0x819b3100, and size 3840 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab5 test 4 KiB - 8 KiB........
  Requested block alignment: 8, after WB_UP: 8, and after CHERI aligned: 128
  Requested block size: 1068, after WB_UP: 1072 after macro rounding to CHERI representable length: 1152,after CHERI builtins rounding during init: 1152
  Requested slab alignment: 8, after WB_UP: 8, and after CHERI aligned: 512
  Requested slab size: 4272, after WB_UP: 4288 and after round up to CHERI representable length: 4608
    Results in buffer: 0x819b1000 with bounds 0x819b1000 to 0x819b2200, and size 4608 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab6 test 32 KiB - 64 KiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 1024
  Requested block size: 9168, after WB_UP: 9168 after macro rounding to CHERI representable length: 9216,after CHERI builtins rounding during init: 9216
  Requested slab alignment: 16, after WB_UP: 16, and after CHERI aligned: 4096
  Requested slab size: 36672, after WB_UP: 36672 and after round up to CHERI representable length: 36864
    Results in buffer: 0x819a8000 with bounds 0x819a8000 to 0x819b1000, and size 36864 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab7 test 128 KiB - 256 KiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 4096
  Requested block size: 38240, after WB_UP: 38240 after macro rounding to CHERI representable length: 40960,after CHERI builtins rounding during init: 40960
  Requested slab alignment: 16, after WB_UP: 16, and after CHERI aligned: 16384
  Requested slab size: 152960, after WB_UP: 152960 and after round up to CHERI representable length: 163840
    Results in buffer: 0x81980000 with bounds 0x81980000 to 0x819a8000, and size 163840 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab8 test 512 KiB - 1 MiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 16384
  Requested block size: 131072, after WB_UP: 131072 after macro rounding to CHERI representable length: 131072,after CHERI builtins rounding during init: 131072
  Requested slab alignment: 16, after WB_UP: 16, and after CHERI aligned: 65536
  Requested slab size: 524288, after WB_UP: 524288 and after round up to CHERI representable length: 524288
    Results in buffer: 0x81900000 with bounds 0x81900000 to 0x81980000, and size 524288 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab9 test 8 MiB - 16 MiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 262144
  Requested block size: 2830240, after WB_UP: 2830240 after macro rounding to CHERI representable length: 2883584,after CHERI builtins rounding during init: 2883584
  Requested slab alignment: 16, after WB_UP: 16, and after CHERI aligned: 1048576
  Requested slab size: 11320960, after WB_UP: 11320960 and after round up to CHERI representable length: 11534336
    Results in buffer: 0x80e00000 with bounds 0x80e00000 to 0x81900000, and size 11534336 bytes, perms: 0x47d, tag: 0x1
Slab macro_slab_l1 test edge case........
  Requested block alignment: 8, after WB_UP: 8, and after CHERI aligned: 16
  Requested block size: 136, after WB_UP: 136 after macro rounding to CHERI representable length: 144,after CHERI builtins rounding during init: 144
  Requested slab alignment: 8, after WB_UP: 8, and after CHERI aligned: 32
  Requested slab size: 408, after WB_UP: 408 and after round up to CHERI representable length: 448
    Results in buffer: 0x819b3dc0 with bounds 0x819b3dc0 to 0x819b3f80, and size 448 bytes, perms: 0x47d, tag: 0x1
checking macro_slab1.buffer.....
checking macro_slab2.buffer.....
checking macro_slab3.buffer.....
checking macro_slab4.buffer.....
checking macro_slab5.buffer.....
checking macro_slab6.buffer.....
checking macro_slab7.buffer.....
checking macro_slab8.buffer.....
checking macro_slab9.buffer.....
checking macro_slab_l1.buffer.....
 PASS - test_i_macro_slab_bounds in 0.067 seconds
===================================================================
START - test_j_macro_block_bounds
Checking block allocations for 32 bit CHERI are aligned and tightly bound........
Block allocation from macro_slab1, test 256 - 512 bytes.....
  Requested length of each block: = 68 bytes, after WB_UP: 72 bytes
  Expected length of each block after CHERI alignment and rounding: = 72 bytes
   block: ptr[0] = 0x819b3c40 with bounds 0x819b3c40 to 0x819b3c88 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x819b3c88 with bounds 0x819b3c88 to 0x819b3cd0 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x819b3cd0 with bounds 0x819b3cd0 to 0x819b3d18 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x819b3d18 with bounds 0x819b3d18 to 0x819b3d60 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x819b3d60 with bounds 0x819b3d60 to 0x819b3da8 and size 72 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab2, test 512 bytes - 1 KiB.....
  Requested length of each block: = 228 bytes, after WB_UP: 232 bytes
  Expected length of each block after CHERI alignment and rounding: = 240 bytes
   block: ptr[0] = 0x819b3880 with bounds 0x819b3880 to 0x819b3970 and size 240 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x819b3970 with bounds 0x819b3970 to 0x819b3a60 and size 240 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x819b3a60 with bounds 0x819b3a60 to 0x819b3b50 and size 240 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x819b3b50 with bounds 0x819b3b50 to 0x819b3c40 and size 240 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab3, test 1 KiB - 2kiB.....
  Requested length of each block: = 468 bytes, after WB_UP: 472 bytes
  Expected length of each block after CHERI alignment and rounding: = 480 bytes
   block: ptr[0] = 0x819b3100 with bounds 0x819b3100 to 0x819b32e0 and size 480 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x819b32e0 with bounds 0x819b32e0 to 0x819b34c0 and size 480 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x819b34c0 with bounds 0x819b34c0 to 0x819b36a0 and size 480 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x819b36a0 with bounds 0x819b36a0 to 0x819b3880 and size 480 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab4, test 2 KiB - 4 KiB.....
  Requested length of each block: = 908 bytes, after WB_UP: 912 bytes
  Expected length of each block after CHERI alignment and rounding: = 960 bytes
   block: ptr[0] = 0x819b2200 with bounds 0x819b2200 to 0x819b25c0 and size 960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x819b25c0 with bounds 0x819b25c0 to 0x819b2980 and size 960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x819b2980 with bounds 0x819b2980 to 0x819b2d40 and size 960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x819b2d40 with bounds 0x819b2d40 to 0x819b3100 and size 960 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab5, test 4 KiB - 8 KiB.....
  Requested length of each block: = 1068 bytes, after WB_UP: 1072 bytes
  Expected length of each block after CHERI alignment and rounding: = 1152 bytes
   block: ptr[0] = 0x819b1000 with bounds 0x819b1000 to 0x819b1480 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x819b1480 with bounds 0x819b1480 to 0x819b1900 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x819b1900 with bounds 0x819b1900 to 0x819b1d80 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x819b1d80 with bounds 0x819b1d80 to 0x819b2200 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab6, test 32 KiB - 64 KiB.....
  Requested length of each block: = 9168 bytes, after WB_UP: 9168 bytes
  Expected length of each block after CHERI alignment and rounding: = 9216 bytes
   block: ptr[0] = 0x819a8000 with bounds 0x819a8000 to 0x819aa400 and size 9216 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x819aa400 with bounds 0x819aa400 to 0x819ac800 and size 9216 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x819ac800 with bounds 0x819ac800 to 0x819aec00 and size 9216 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x819aec00 with bounds 0x819aec00 to 0x819b1000 and size 9216 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab7, test 128 KiB - 256 KiB.....
  Requested length of each block: = 38240 bytes, after WB_UP: 38240 bytes
  Expected length of each block after CHERI alignment and rounding: = 40960 bytes
   block: ptr[0] = 0x81980000 with bounds 0x81980000 to 0x8198a000 and size 40960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x8198a000 with bounds 0x8198a000 to 0x81994000 and size 40960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x81994000 with bounds 0x81994000 to 0x8199e000 and size 40960 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x8199e000 with bounds 0x8199e000 to 0x819a8000 and size 40960 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab8, test 512 KiB - 1 MiB.....
  Requested length of each block: = 131072 bytes, after WB_UP: 131072 bytes
  Expected length of each block after CHERI alignment and rounding: = 131072 bytes
   block: ptr[0] = 0x81900000 with bounds 0x81900000 to 0x81920000 and size 131072 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x81920000 with bounds 0x81920000 to 0x81940000 and size 131072 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x81940000 with bounds 0x81940000 to 0x81960000 and size 131072 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x81960000 with bounds 0x81960000 to 0x81980000 and size 131072 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab9, test 8 MiB  - 16 MiB.....
  Requested length of each block: = 2830240 bytes, after WB_UP: 2830240 bytes
  Expected length of each block after CHERI alignment and rounding: = 2883584 bytes
   block: ptr[0] = 0x80e00000 with bounds 0x80e00000 to 0x810c0000 and size 2883584 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x810c0000 with bounds 0x810c0000 to 0x81380000 and size 2883584 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x81380000 with bounds 0x81380000 to 0x81640000 and size 2883584 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x81640000 with bounds 0x81640000 to 0x81900000 and size 2883584 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_slab_l1, test edge case.....
  Requested length of each block: = 136 bytes, after WB_UP: 136 bytes
  Expected length of each block after CHERI alignment and rounding: = 144 bytes
   block: ptr[0] = 0x819b3dc0 with bounds 0x819b3dc0 to 0x819b3e50 and size 144 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x819b3e50 with bounds 0x819b3e50 to 0x819b3ee0 and size 144 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x819b3ee0 with bounds 0x819b3ee0 to 0x819b3f70 and size 144 bytes, tag: 0x1, checking....alignment: good, block length: good
Freeing blocks to macro_slab1, test 256 bytes - 512 bytes.....
  freed block: ptr[0] = 0x819b3c40 with bounds 0x819b3c40 to 0x819b3c88 and size 72 bytes, tag: 0x1,
  freed block: ptr[1] = 0x819b3c88 with bounds 0x819b3c88 to 0x819b3cd0 and size 72 bytes, tag: 0x1,
  freed block: ptr[2] = 0x819b3cd0 with bounds 0x819b3cd0 to 0x819b3d18 and size 72 bytes, tag: 0x1,
  freed block: ptr[3] = 0x819b3d18 with bounds 0x819b3d18 to 0x819b3d60 and size 72 bytes, tag: 0x1,
  freed block: ptr[4] = 0x819b3d60 with bounds 0x819b3d60 to 0x819b3da8 and size 72 bytes, tag: 0x1,
Freeing blocks to macro_slab2, test 512 bytes - 1 KiB.....
  freed block: ptr[0] = 0x819b3880 with bounds 0x819b3880 to 0x819b3970 and size 240 bytes, tag: 0x1,
  freed block: ptr[1] = 0x819b3970 with bounds 0x819b3970 to 0x819b3a60 and size 240 bytes, tag: 0x1,
  freed block: ptr[2] = 0x819b3a60 with bounds 0x819b3a60 to 0x819b3b50 and size 240 bytes, tag: 0x1,
  freed block: ptr[3] = 0x819b3b50 with bounds 0x819b3b50 to 0x819b3c40 and size 240 bytes, tag: 0x1,
Freeing blocks to macro_slab3, test 1 KiB - 2kiB.....
  freed block: ptr[0] = 0x819b3100 with bounds 0x819b3100 to 0x819b32e0 and size 480 bytes, tag: 0x1,
  freed block: ptr[1] = 0x819b32e0 with bounds 0x819b32e0 to 0x819b34c0 and size 480 bytes, tag: 0x1,
  freed block: ptr[2] = 0x819b34c0 with bounds 0x819b34c0 to 0x819b36a0 and size 480 bytes, tag: 0x1,
  freed block: ptr[3] = 0x819b36a0 with bounds 0x819b36a0 to 0x819b3880 and size 480 bytes, tag: 0x1,
Freeing blocks to macro_slab4, test 2 KiB - 4 KiB.....
  freed block: ptr[0] = 0x819b2200 with bounds 0x819b2200 to 0x819b25c0 and size 960 bytes, tag: 0x1,
  freed block: ptr[1] = 0x819b25c0 with bounds 0x819b25c0 to 0x819b2980 and size 960 bytes, tag: 0x1,
  freed block: ptr[2] = 0x819b2980 with bounds 0x819b2980 to 0x819b2d40 and size 960 bytes, tag: 0x1,
  freed block: ptr[3] = 0x819b2d40 with bounds 0x819b2d40 to 0x819b3100 and size 960 bytes, tag: 0x1,
Freeing blocks to macro_slab5, test 4 KiB - 8 KiB.....
  freed block: ptr[0] = 0x819b1000 with bounds 0x819b1000 to 0x819b1480 and size 1152 bytes, tag: 0x1,
  freed block: ptr[1] = 0x819b1480 with bounds 0x819b1480 to 0x819b1900 and size 1152 bytes, tag: 0x1,
  freed block: ptr[2] = 0x819b1900 with bounds 0x819b1900 to 0x819b1d80 and size 1152 bytes, tag: 0x1,
  freed block: ptr[3] = 0x819b1d80 with bounds 0x819b1d80 to 0x819b2200 and size 1152 bytes, tag: 0x1,
Freeing blocks to macro_slab6, test 32 KiB - 64 KiB.....
  freed block: ptr[0] = 0x819a8000 with bounds 0x819a8000 to 0x819aa400 and size 9216 bytes, tag: 0x1,
  freed block: ptr[1] = 0x819aa400 with bounds 0x819aa400 to 0x819ac800 and size 9216 bytes, tag: 0x1,
  freed block: ptr[2] = 0x819ac800 with bounds 0x819ac800 to 0x819aec00 and size 9216 bytes, tag: 0x1,
  freed block: ptr[3] = 0x819aec00 with bounds 0x819aec00 to 0x819b1000 and size 9216 bytes, tag: 0x1,
Freeing blocks to macro_slab7, test 128 KiB - 256 KiB.....
  freed block: ptr[0] = 0x81980000 with bounds 0x81980000 to 0x8198a000 and size 40960 bytes, tag: 0x1,
  freed block: ptr[1] = 0x8198a000 with bounds 0x8198a000 to 0x81994000 and size 40960 bytes, tag: 0x1,
  freed block: ptr[2] = 0x81994000 with bounds 0x81994000 to 0x8199e000 and size 40960 bytes, tag: 0x1,
  freed block: ptr[3] = 0x8199e000 with bounds 0x8199e000 to 0x819a8000 and size 40960 bytes, tag: 0x1,
Freeing blocks to macro_slab8, test 512 KiB - 1 MiB.....
  freed block: ptr[0] = 0x81900000 with bounds 0x81900000 to 0x81920000 and size 131072 bytes, tag: 0x1,
  freed block: ptr[1] = 0x81920000 with bounds 0x81920000 to 0x81940000 and size 131072 bytes, tag: 0x1,
  freed block: ptr[2] = 0x81940000 with bounds 0x81940000 to 0x81960000 and size 131072 bytes, tag: 0x1,
  freed block: ptr[3] = 0x81960000 with bounds 0x81960000 to 0x81980000 and size 131072 bytes, tag: 0x1,
Freeing blocks to macro_slab9, test 8 MiB - 16 MiB.....
  freed block: ptr[0] = 0x80e00000 with bounds 0x80e00000 to 0x810c0000 and size 2883584 bytes, tag: 0x1,
  freed block: ptr[1] = 0x810c0000 with bounds 0x810c0000 to 0x81380000 and size 2883584 bytes, tag: 0x1,
  freed block: ptr[2] = 0x81380000 with bounds 0x81380000 to 0x81640000 and size 2883584 bytes, tag: 0x1,
  freed block: ptr[3] = 0x81640000 with bounds 0x81640000 to 0x81900000 and size 2883584 bytes, tag: 0x1,
Freeing blocks to macro_slab_l1, test edge case.....
  freed block: ptr[0] = 0x819b3dc0 with bounds 0x819b3dc0 to 0x819b3e50 and size 144 bytes, tag: 0x1,
  freed block: ptr[1] = 0x819b3e50 with bounds 0x819b3e50 to 0x819b3ee0 and size 144 bytes, tag: 0x1,
  freed block: ptr[2] = 0x819b3ee0 with bounds 0x819b3ee0 to 0x819b3f70 and size 144 bytes, tag: 0x1,
Repeat Block allocation from macro_slab5, test 4 KiB - 8 KiB.....
  Requested length of each block: = 1068 bytes, after WB_UP: 1072 bytes
  Expected length of each block after CHERI alignment and rounding: = 1152 bytes
   block: ptr[0] = 0x819b1d80 with bounds 0x819b1d80 to 0x819b2200 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x819b1900 with bounds 0x819b1900 to 0x819b1d80 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x819b1480 with bounds 0x819b1480 to 0x819b1900 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x819b1000 with bounds 0x819b1000 to 0x819b1480 and size 1152 bytes, tag: 0x1, checking....alignment: good, block length: good
Repeat Freeing blocks to macro_slab5, test 4 KiB - 8 KiB.....
  freed block: ptr[0] = 0x819b1d80 with bounds 0x819b1d80 to 0x819b2200 and size 1152 bytes, tag: 0x1,
  freed block: ptr[1] = 0x819b1900 with bounds 0x819b1900 to 0x819b1d80 and size 1152 bytes, tag: 0x1,
  freed block: ptr[2] = 0x819b1480 with bounds 0x819b1480 to 0x819b1900 and size 1152 bytes, tag: 0x1,
  freed block: ptr[3] = 0x819b1000 with bounds 0x819b1000 to 0x819b1480 and size 1152 bytes, tag: 0x1,
 PASS - test_j_macro_block_bounds in 0.197 seconds
===================================================================
TESTSUITE test_mem_slab_cheri succeeded

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [test_mem_slab_cheri]: pass = 10, fail = 0, skip = 0, total = 10 duration = 11.484 seconds
 - PASS - [test_mem_slab_cheri.test_a_scan_cheri_alignment_thresholds] duration = 10.746 seconds
 - PASS - [test_mem_slab_cheri.test_b_cheri_macros] duration = 0.180 seconds
 - PASS - [test_mem_slab_cheri.test_c_init_slab_bounds] duration = 0.074 seconds
 - PASS - [test_mem_slab_cheri.test_d_init_block_bounds] duration = 0.185 seconds
 - PASS - [test_mem_slab_cheri.test_e_get_slab_length_edge_cases] duration = 0.016 seconds
 - PASS - [test_mem_slab_cheri.test_f_init_slab_edge_cases_exact_fail] duration = 0.002 seconds
 - PASS - [test_mem_slab_cheri.test_g_init_slab_edge_cases_exact_macro_pass] duration = 0.002 seconds
 - PASS - [test_mem_slab_cheri.test_h_init_block_edge_cases_exact_macro_pass] duration = 0.015 seconds
 - PASS - [test_mem_slab_cheri.test_i_macro_slab_bounds] duration = 0.067 seconds
 - PASS - [test_mem_slab_cheri.test_j_macro_block_bounds] duration = 0.197 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION SUCCESSFUL
