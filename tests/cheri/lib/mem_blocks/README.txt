Title: Memory block APIs with CHERI memory alignment

Description:

This test verifies that the CHERI-modified memory blocks APIs operate as expected.

--------------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on QEMU as follows:

    Ensure the llvm-cheri toolchain is selected and set up, then

    west build -p -b qemu_riscv32cheri_purecap tests/cheri/lib/mem_blocks -T cheri.libraries.mem_blocks
    west build -t run

--------------------------------------------------------------------------------

Sample Output:

Running TESTSUITE test_mem_block_cheri
===================================================================
START - test_a_mem_block_bounds
Checking compile-time backing buffers are 32 bit CHERI aligned and tightly bound........
buffer test 256 bytes - 512 bytes........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 64, after WB_UP: 64 CHERI macro rounding: 64, CHERI builtins rounding: 64
  Requested buffer size: 320, after WB_UP: 320 CHERI macro rounding: 320, CHERI builtins rounding: 320
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
    Results in buffer: 0x8136b800 with bounds 0x8136b800 to 0x8136b940, and size 320 bytes, perms: 0x400ff, tag: 0x1
buffer test 512 bytes - 1 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 128, after WB_UP: 128 CHERI macro rounding: 128, CHERI builtins rounding: 128
  Requested buffer size: 512, after WB_UP: 512 CHERI macro rounding: 512, CHERI builtins rounding: 512
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
    Results in buffer: 0x8136b940 with bounds 0x8136b940 to 0x8136bb40, and size 512 bytes, perms: 0x400ff, tag: 0x1
buffer test 1 KiB - 2 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 256, after WB_UP: 256 CHERI macro rounding: 256, CHERI builtins rounding: 256
  Requested buffer size: 1024, after WB_UP: 1024 CHERI macro rounding: 1024, CHERI builtins rounding: 1024
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 16
    Results in buffer: 0x81369800 with bounds 0x81369800 to 0x81369c00, and size 1024 bytes, perms: 0x400ff, tag: 0x1
buffer test 2 KiB - 4 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 512, after WB_UP: 512 CHERI macro rounding: 512, CHERI builtins rounding: 512
  Requested buffer size: 2048, after WB_UP: 2048 CHERI macro rounding: 2048, CHERI builtins rounding: 2048
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 32
    Results in buffer: 0x81369000 with bounds 0x81369000 to 0x81369800, and size 2048 bytes, perms: 0x400ff, tag: 0x1
buffer test 4 KiB - 8 KiB........
  Requested block alignment: 8, after WB_UP: 8, and after CHERI aligned: 16
  Requested block size: 1024, after WB_UP: 1024 CHERI macro rounding: 1024, CHERI builtins rounding: 1024
  Requested buffer size: 4096, after WB_UP: 4096 CHERI macro rounding: 4096, CHERI builtins rounding: 4096
  Requested buffer alignment: 8, after WB_UP: 8, and after CHERI aligned: 64
    Results in buffer: 0x81368000 with bounds 0x81368000 to 0x81369000, and size 4096 bytes, perms: 0x400ff, tag: 0x1
buffer test 32 KiB - 64 KiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 128
  Requested block size: 8192, after WB_UP: 8192 CHERI macro rounding: 8192, CHERI builtins rounding: 8192
  Requested buffer size: 32768, after WB_UP: 32768 CHERI macro rounding: 32768, CHERI builtins rounding: 32768
  Requested buffer alignment: 16, after WB_UP: 16, and after CHERI aligned: 512
    Results in buffer: 0x81360000 with bounds 0x81360000 to 0x81368000, and size 32768 bytes, perms: 0x400ff, tag: 0x1
buffer test 128 KiB - 256 KiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 512
  Requested block size: 32768, after WB_UP: 32768 CHERI macro rounding: 32768, CHERI builtins rounding: 32768
  Requested buffer size: 131072, after WB_UP: 131072 CHERI macro rounding: 131072, CHERI builtins rounding: 131072
  Requested buffer alignment: 16, after WB_UP: 16, and after CHERI aligned: 2048
    Results in buffer: 0x81340000 with bounds 0x81340000 to 0x81360000, and size 131072 bytes, perms: 0x400ff, tag: 0x1
buffer test 512 KiB - 1 MiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 1024
  Requested block size: 65536, after WB_UP: 65536 CHERI macro rounding: 65536, CHERI builtins rounding: 65536
  Requested buffer size: 524288, after WB_UP: 524288 CHERI macro rounding: 524288, CHERI builtins rounding: 524288
  Requested buffer alignment: 16, after WB_UP: 16, and after CHERI aligned: 8192
    Results in buffer: 0x812c0000 with bounds 0x812c0000 to 0x81340000, and size 524288 bytes, perms: 0x400ff, tag: 0x1
buffer test 8 MiB - 16 MiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 16384
  Requested block size: 1048576, after WB_UP: 1048576 CHERI macro rounding: 1048576, CHERI builtins rounding: 1048576
  Requested buffer size: 8388608, after WB_UP: 8388608 CHERI macro rounding: 8388608, CHERI builtins rounding: 8388608
  Requested buffer alignment: 16, after WB_UP: 16, and after CHERI aligned: 131072
    Results in buffer: 0x80ac0000 with bounds 0x80ac0000 to 0x812c0000, and size 8388608 bytes, perms: 0x400ff, tag: 0x1
checking macro_mem_block1.buffer.....
actual_buffer_len: 320, expected_buffer_len: 320
checking macro_mem_block2.buffer.....
actual_buffer_len: 512, expected_buffer_len: 512
checking macro_mem_block3.buffer.....
actual_buffer_len: 1024, expected_buffer_len: 1024
checking macro_mem_block4.buffer.....
actual_buffer_len: 2048, expected_buffer_len: 2048
checking macro_mem_block5.buffer.....
actual_buffer_len: 4096, expected_buffer_len: 4096
checking macro_mem_block6.buffer.....
actual_buffer_len: 32768, expected_buffer_len: 32768
checking macro_mem_block7.buffer.....
actual_buffer_len: 131072, expected_buffer_len: 131072
checking macro_mem_block8.buffer.....
actual_buffer_len: 524288, expected_buffer_len: 524288
checking macro_mem_block9.buffer.....
actual_buffer_len: 8388608, expected_buffer_len: 8388608
 PASS - test_a_mem_block_bounds in 0.066 seconds
===================================================================
START - test_b_mem_block_block_bounds
Checking block allocations for 32 bit CHERI are aligned and tightly bound........
Block allocation from macro_mem_block1, test 256 - 512 bytes.....
  Requested length of each block: = 64 bytes, after WB_UP: 64 bytes
  Expected length of each block after CHERI alignment and rounding: = 64 bytes
   block: ptr[0] = 0x8136b800 with bounds 0x8136b800 to 0x8136b840 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x8136b840 with bounds 0x8136b840 to 0x8136b880 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x8136b880 with bounds 0x8136b880 to 0x8136b8c0 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x8136b8c0 with bounds 0x8136b8c0 to 0x8136b900 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x8136b900 with bounds 0x8136b900 to 0x8136b940 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_mem_block2, test 512 bytes - 1 KiB.....
  Requested length of each block: = 128 bytes, after WB_UP: 128 bytes
  Expected length of each block after CHERI alignment and rounding: = 128 bytes
   block: ptr[0] = 0x8136b940 with bounds 0x8136b940 to 0x8136b9c0 and size 128 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x8136b9c0 with bounds 0x8136b9c0 to 0x8136ba40 and size 128 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x8136ba40 with bounds 0x8136ba40 to 0x8136bac0 and size 128 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x8136bac0 with bounds 0x8136bac0 to 0x8136bb40 and size 128 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_mem_block3, test 1 KiB - 2 KiB.....
  Requested length of each block: = 256 bytes, after WB_UP: 256 bytes
  Expected length of each block after CHERI alignment and rounding: = 256 bytes
   block: ptr[0] = 0x81369800 with bounds 0x81369800 to 0x81369900 and size 256 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x81369900 with bounds 0x81369900 to 0x81369a00 and size 256 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x81369a00 with bounds 0x81369a00 to 0x81369b00 and size 256 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x81369b00 with bounds 0x81369b00 to 0x81369c00 and size 256 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_mem_block4, test 2 KiB - 4 KiB.....
  Requested length of each block: = 512 bytes, after WB_UP: 512 bytes
  Expected length of each block after CHERI alignment and rounding: = 512 bytes
   block: ptr[0] = 0x81369000 with bounds 0x81369000 to 0x81369200 and size 512 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x81369200 with bounds 0x81369200 to 0x81369400 and size 512 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x81369400 with bounds 0x81369400 to 0x81369600 and size 512 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x81369600 with bounds 0x81369600 to 0x81369800 and size 512 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_mem_block5, test 4 KiB - 8 KiB.....
  Requested length of each block: = 1024 bytes, after WB_UP: 1024 bytes
  Expected length of each block after CHERI alignment and rounding: = 1024 bytes
   block: ptr[0] = 0x81368000 with bounds 0x81368000 to 0x81368400 and size 1024 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x81368400 with bounds 0x81368400 to 0x81368800 and size 1024 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x81368800 with bounds 0x81368800 to 0x81368c00 and size 1024 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x81368c00 with bounds 0x81368c00 to 0x81369000 and size 1024 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_mem_block6, test 32 KiB - 64 KiB.....
  Requested length of each block: = 8192 bytes, after WB_UP: 8192 bytes
  Expected length of each block after CHERI alignment and rounding: = 8192 bytes
   block: ptr[0] = 0x81360000 with bounds 0x81360000 to 0x81362000 and size 8192 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x81362000 with bounds 0x81362000 to 0x81364000 and size 8192 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x81364000 with bounds 0x81364000 to 0x81366000 and size 8192 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x81366000 with bounds 0x81366000 to 0x81368000 and size 8192 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_mem_block7, test 128 KiB - 256 KiB.....
  Requested length of each block: = 32768 bytes, after WB_UP: 32768 bytes
  Expected length of each block after CHERI alignment and rounding: = 32768 bytes
   block: ptr[0] = 0x81340000 with bounds 0x81340000 to 0x81348000 and size 32768 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x81348000 with bounds 0x81348000 to 0x81350000 and size 32768 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x81350000 with bounds 0x81350000 to 0x81358000 and size 32768 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x81358000 with bounds 0x81358000 to 0x81360000 and size 32768 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_mem_block8, test 512 KiB - 1 MiB.....
  Requested length of each block: = 65536 bytes, after WB_UP: 65536 bytes
  Expected length of each block after CHERI alignment and rounding: = 65536 bytes
   block: ptr[0] = 0x812c0000 with bounds 0x812c0000 to 0x812d0000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x812d0000 with bounds 0x812d0000 to 0x812e0000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x812e0000 with bounds 0x812e0000 to 0x812f0000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x812f0000 with bounds 0x812f0000 to 0x81300000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x81300000 with bounds 0x81300000 to 0x81310000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[5] = 0x81310000 with bounds 0x81310000 to 0x81320000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[6] = 0x81320000 with bounds 0x81320000 to 0x81330000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[7] = 0x81330000 with bounds 0x81330000 to 0x81340000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from macro_mem_block9, test 8 MiB - 16 MiB.....
  Requested length of each block: = 1048576 bytes, after WB_UP: 1048576 bytes
  Expected length of each block after CHERI alignment and rounding: = 1048576 bytes
   block: ptr[0] = 0x80ac0000 with bounds 0x80ac0000 to 0x80bc0000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80bc0000 with bounds 0x80bc0000 to 0x80cc0000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80cc0000 with bounds 0x80cc0000 to 0x80dc0000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80dc0000 with bounds 0x80dc0000 to 0x80ec0000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x80ec0000 with bounds 0x80ec0000 to 0x80fc0000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[5] = 0x80fc0000 with bounds 0x80fc0000 to 0x810c0000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[6] = 0x810c0000 with bounds 0x810c0000 to 0x811c0000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[7] = 0x811c0000 with bounds 0x811c0000 to 0x812c0000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
Freeing blocks to macro_mem_block1, test 256 bytes - 512 bytes.....
Freeing blocks to macro_mem_block2, test 512 bytes - 1 KiB.....
Freeing blocks to macro_mem_block3, test 1 KiB - 2 KiB.....
Freeing blocks to macro_mem_block4, test 2 KiB - 4 KiB.....
Freeing blocks to macro_mem_block5, test 4 KiB - 8 KiB.....
Freeing blocks to macro_mem_block6, test 32 KiB - 64 KiB.....
Freeing blocks to macro_mem_block7, test 128 KiB - 256 KiB.....
Freeing blocks to macro_mem_block8, test 512 KiB - 1 MiB.....
Freeing blocks to macro_mem_block9, test 8 MiB - 16 MiB.....
 PASS - test_b_mem_block_block_bounds in 0.124 seconds
===================================================================
START - test_c_mem_block_ext_bounds
Checking compile-time backing buffers are 32 bit CHERI aligned and tightly bound........
buffer test 256 bytes - 512 bytes........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 64, after WB_UP: 64 CHERI macro rounding: 64, CHERI builtins rounding: 64
  Requested buffer size: 320, after WB_UP: 320 CHERI macro rounding: 320, CHERI builtins rounding: 320
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
    Results in buffer: 0x80aaa000 with bounds 0x80aaa000 to 0x80aaa140, and size 320 bytes, perms: 0x400ff, tag: 0x1
buffer test 512 bytes - 1 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 128, after WB_UP: 128 CHERI macro rounding: 128, CHERI builtins rounding: 128
  Requested buffer size: 512, after WB_UP: 512 CHERI macro rounding: 512, CHERI builtins rounding: 512
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
    Results in buffer: 0x80aa9c00 with bounds 0x80aa9c00 to 0x80aa9e00, and size 512 bytes, perms: 0x400ff, tag: 0x1
buffer test 1 KiB - 2 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 256, after WB_UP: 256 CHERI macro rounding: 256, CHERI builtins rounding: 256
  Requested buffer size: 1024, after WB_UP: 1024 CHERI macro rounding: 1024, CHERI builtins rounding: 1024
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 16
    Results in buffer: 0x80aa9800 with bounds 0x80aa9800 to 0x80aa9c00, and size 1024 bytes, perms: 0x400ff, tag: 0x1
buffer test 2 KiB - 4 KiB........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 512, after WB_UP: 512 CHERI macro rounding: 512, CHERI builtins rounding: 512
  Requested buffer size: 2048, after WB_UP: 2048 CHERI macro rounding: 2048, CHERI builtins rounding: 2048
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 32
    Results in buffer: 0x80aa9000 with bounds 0x80aa9000 to 0x80aa9800, and size 2048 bytes, perms: 0x400ff, tag: 0x1
buffer test 4 KiB - 8 KiB........
  Requested block alignment: 8, after WB_UP: 8, and after CHERI aligned: 16
  Requested block size: 1024, after WB_UP: 1024 CHERI macro rounding: 1024, CHERI builtins rounding: 1024
  Requested buffer size: 4096, after WB_UP: 4096 CHERI macro rounding: 4096, CHERI builtins rounding: 4096
  Requested buffer alignment: 8, after WB_UP: 8, and after CHERI aligned: 64
    Results in buffer: 0x80aa8000 with bounds 0x80aa8000 to 0x80aa9000, and size 4096 bytes, perms: 0x400ff, tag: 0x1
buffer test 32 KiB - 64 KiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 128
  Requested block size: 8192, after WB_UP: 8192 CHERI macro rounding: 8192, CHERI builtins rounding: 8192
  Requested buffer size: 32768, after WB_UP: 32768 CHERI macro rounding: 32768, CHERI builtins rounding: 32768
  Requested buffer alignment: 16, after WB_UP: 16, and after CHERI aligned: 512
    Results in buffer: 0x80aa0000 with bounds 0x80aa0000 to 0x80aa8000, and size 32768 bytes, perms: 0x400ff, tag: 0x1
buffer test 128 KiB - 256 KiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 512
  Requested block size: 32768, after WB_UP: 32768 CHERI macro rounding: 32768, CHERI builtins rounding: 32768
  Requested buffer size: 131072, after WB_UP: 131072 CHERI macro rounding: 131072, CHERI builtins rounding: 131072
  Requested buffer alignment: 16, after WB_UP: 16, and after CHERI aligned: 2048
    Results in buffer: 0x80a80000 with bounds 0x80a80000 to 0x80aa0000, and size 131072 bytes, perms: 0x400ff, tag: 0x1
buffer test 512 KiB - 1 MiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 1024
  Requested block size: 65536, after WB_UP: 65536 CHERI macro rounding: 65536, CHERI builtins rounding: 65536
  Requested buffer size: 524288, after WB_UP: 524288 CHERI macro rounding: 524288, CHERI builtins rounding: 524288
  Requested buffer alignment: 16, after WB_UP: 16, and after CHERI aligned: 8192
    Results in buffer: 0x80a00000 with bounds 0x80a00000 to 0x80a80000, and size 524288 bytes, perms: 0x400ff, tag: 0x1
buffer test 8 MiB - 16 MiB........
  Requested block alignment: 16, after WB_UP: 16, and after CHERI aligned: 16384
  Requested block size: 1048576, after WB_UP: 1048576 CHERI macro rounding: 1048576, CHERI builtins rounding: 1048576
  Requested buffer size: 8388608, after WB_UP: 8388608 CHERI macro rounding: 8388608, CHERI builtins rounding: 8388608
  Requested buffer alignment: 16, after WB_UP: 16, and after CHERI aligned: 131072
    Results in buffer: 0x80200000 with bounds 0x80200000 to 0x80a00000, and size 8388608 bytes, perms: 0x400ff, tag: 0x1
checking mem_block_ext1.buffer.....
actual_buffer_len: 320, expected_buffer_len: 320
checking mem_block_ext2.buffer.....
actual_buffer_len: 512, expected_buffer_len: 512
checking mem_block_ext3.buffer.....
actual_buffer_len: 1024, expected_buffer_len: 1024
checking mem_block_ext4.buffer.....
actual_buffer_len: 2048, expected_buffer_len: 2048
checking mem_block_ext5.buffer.....
actual_buffer_len: 4096, expected_buffer_len: 4096
checking mem_block_ext6.buffer.....
actual_buffer_len: 32768, expected_buffer_len: 32768
checking mem_block_ext7.buffer.....
actual_buffer_len: 131072, expected_buffer_len: 131072
checking mem_block_ext8.buffer.....
actual_buffer_len: 524288, expected_buffer_len: 524288
checking mem_block_ext9.buffer.....
actual_buffer_len: 8388608, expected_buffer_len: 8388608
 PASS - test_c_mem_block_ext_bounds in 0.066 seconds
===================================================================
START - test_d_mem_block_ext_block_bounds
Checking block allocations for 32 bit CHERI are aligned and tightly bound........
Block allocation from mem_block_ext1, test 256 - 512 bytes.....
  Requested length of each block: = 64 bytes, after WB_UP: 64 bytes
  Expected length of each block after CHERI alignment and rounding: = 64 bytes
   block: ptr[0] = 0x80aaa000 with bounds 0x80aaa000 to 0x80aaa040 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80aaa040 with bounds 0x80aaa040 to 0x80aaa080 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80aaa080 with bounds 0x80aaa080 to 0x80aaa0c0 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80aaa0c0 with bounds 0x80aaa0c0 to 0x80aaa100 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x80aaa100 with bounds 0x80aaa100 to 0x80aaa140 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_ext2, test 512 bytes - 1 KiB.....
  Requested length of each block: = 128 bytes, after WB_UP: 128 bytes
  Expected length of each block after CHERI alignment and rounding: = 128 bytes
   block: ptr[0] = 0x80aa9c00 with bounds 0x80aa9c00 to 0x80aa9c80 and size 128 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80aa9c80 with bounds 0x80aa9c80 to 0x80aa9d00 and size 128 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80aa9d00 with bounds 0x80aa9d00 to 0x80aa9d80 and size 128 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80aa9d80 with bounds 0x80aa9d80 to 0x80aa9e00 and size 128 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_ext3, test 1 KiB - 2 KiB.....
  Requested length of each block: = 256 bytes, after WB_UP: 256 bytes
  Expected length of each block after CHERI alignment and rounding: = 256 bytes
   block: ptr[0] = 0x80aa9800 with bounds 0x80aa9800 to 0x80aa9900 and size 256 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80aa9900 with bounds 0x80aa9900 to 0x80aa9a00 and size 256 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80aa9a00 with bounds 0x80aa9a00 to 0x80aa9b00 and size 256 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80aa9b00 with bounds 0x80aa9b00 to 0x80aa9c00 and size 256 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_ext4, test 2 KiB - 4 KiB.....
  Requested length of each block: = 512 bytes, after WB_UP: 512 bytes
  Expected length of each block after CHERI alignment and rounding: = 512 bytes
   block: ptr[0] = 0x80aa9000 with bounds 0x80aa9000 to 0x80aa9200 and size 512 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80aa9200 with bounds 0x80aa9200 to 0x80aa9400 and size 512 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80aa9400 with bounds 0x80aa9400 to 0x80aa9600 and size 512 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80aa9600 with bounds 0x80aa9600 to 0x80aa9800 and size 512 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_ext5, test 4 KiB - 8 KiB.....
  Requested length of each block: = 1024 bytes, after WB_UP: 1024 bytes
  Expected length of each block after CHERI alignment and rounding: = 1024 bytes
   block: ptr[0] = 0x80aa8000 with bounds 0x80aa8000 to 0x80aa8400 and size 1024 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80aa8400 with bounds 0x80aa8400 to 0x80aa8800 and size 1024 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80aa8800 with bounds 0x80aa8800 to 0x80aa8c00 and size 1024 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80aa8c00 with bounds 0x80aa8c00 to 0x80aa9000 and size 1024 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_ext6, test 32 KiB - 64 KiB.....
  Requested length of each block: = 8192 bytes, after WB_UP: 8192 bytes
  Expected length of each block after CHERI alignment and rounding: = 8192 bytes
   block: ptr[0] = 0x80aa0000 with bounds 0x80aa0000 to 0x80aa2000 and size 8192 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80aa2000 with bounds 0x80aa2000 to 0x80aa4000 and size 8192 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80aa4000 with bounds 0x80aa4000 to 0x80aa6000 and size 8192 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80aa6000 with bounds 0x80aa6000 to 0x80aa8000 and size 8192 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_ext7, test 128 KiB - 256 KiB.....
  Requested length of each block: = 32768 bytes, after WB_UP: 32768 bytes
  Expected length of each block after CHERI alignment and rounding: = 32768 bytes
   block: ptr[0] = 0x80a80000 with bounds 0x80a80000 to 0x80a88000 and size 32768 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80a88000 with bounds 0x80a88000 to 0x80a90000 and size 32768 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80a90000 with bounds 0x80a90000 to 0x80a98000 and size 32768 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80a98000 with bounds 0x80a98000 to 0x80aa0000 and size 32768 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_ext8, test 512 KiB - 1 MiB.....
  Requested length of each block: = 65536 bytes, after WB_UP: 65536 bytes
  Expected length of each block after CHERI alignment and rounding: = 65536 bytes
   block: ptr[0] = 0x80a00000 with bounds 0x80a00000 to 0x80a10000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80a10000 with bounds 0x80a10000 to 0x80a20000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80a20000 with bounds 0x80a20000 to 0x80a30000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80a30000 with bounds 0x80a30000 to 0x80a40000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x80a40000 with bounds 0x80a40000 to 0x80a50000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[5] = 0x80a50000 with bounds 0x80a50000 to 0x80a60000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[6] = 0x80a60000 with bounds 0x80a60000 to 0x80a70000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[7] = 0x80a70000 with bounds 0x80a70000 to 0x80a80000 and size 65536 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_ext9, test 8 MiB - 16 MiB.....
  Requested length of each block: = 1048576 bytes, after WB_UP: 1048576 bytes
  Expected length of each block after CHERI alignment and rounding: = 1048576 bytes
   block: ptr[0] = 0x80200000 with bounds 0x80200000 to 0x80300000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80300000 with bounds 0x80300000 to 0x80400000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80400000 with bounds 0x80400000 to 0x80500000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80500000 with bounds 0x80500000 to 0x80600000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x80600000 with bounds 0x80600000 to 0x80700000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[5] = 0x80700000 with bounds 0x80700000 to 0x80800000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[6] = 0x80800000 with bounds 0x80800000 to 0x80900000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[7] = 0x80900000 with bounds 0x80900000 to 0x80a00000 and size 1048576 bytes, tag: 0x1, checking....alignment: good, block length: good
Freeing blocks to mem_block_ext1, test 256 bytes - 512 bytes.....
Freeing blocks to mem_block_ext2, test 512 bytes - 1 KiB.....
Freeing blocks to mem_block_ext3, test 1 KiB - 2 KiB.....
Freeing blocks to mem_block_ext4, test 2 KiB - 4 KiB.....
Freeing blocks to mem_block_ext5, test 4 KiB - 8 KiB.....
Freeing blocks to mem_block_ext6, test 32 KiB - 64 KiB.....
Freeing blocks to mem_block_ext7, test 128 KiB - 256 KiB.....
Freeing blocks to mem_block_ext8, test 512 KiB - 1 MiB.....
Freeing blocks to mem_block_ext9, test 8 MiB - 16 MiB.....
 PASS - test_d_mem_block_ext_block_bounds in 0.124 seconds
===================================================================
START - test_e_mem_block_bounds_orig
Checking buffers from original mem_block test are 32 bit CHERI aligned and tightly bound........
buffer mem_block_01........
  Requested block alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 64, after WB_UP: 64 CHERI macro rounding: 64, CHERI builtins rounding: 64
  Requested buffer size: 512, after WB_UP: 512 CHERI macro rounding: 512, CHERI builtins rounding: 512
  Requested buffer alignment: 4, after WB_UP: 8, and after CHERI aligned: 8
    Results in buffer: 0x8136bb40 with bounds 0x8136bb40 to 0x8136bd40, and size 512 bytes, perms: 0x400ff, tag: 0x1
checking mem_block_01.buffer.....
actual_buffer_len: 512, expected_buffer_len: 512
buffer mem_block_02........
  Requested block alignment: 1, after WB_UP: 8, and after CHERI aligned: 8
  Requested block size: 64, after WB_UP: 64 CHERI macro rounding: 64, CHERI builtins rounding: 64
  Requested buffer size: 512, after WB_UP: 512 CHERI macro rounding: 512, CHERI builtins rounding: 512
  Requested buffer alignment: 1, after WB_UP: 8, and after CHERI aligned: 8
    Results in buffer: 0x80aa9e00 with bounds 0x80aa9e00 to 0x80aaa000, and size 512 bytes, perms: 0x400ff, tag: 0x1
checking mem_block_01.buffer.....
actual_buffer_len: 512, expected_buffer_len: 512
Block allocation from mem_block_01.....
  Requested length of each block: = 64 bytes, after WB_UP: 64 bytes
  Expected length of each block after CHERI alignment and rounding: = 64 bytes
   block: ptr[0] = 0x8136bb40 with bounds 0x8136bb40 to 0x8136bb80 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x8136bb80 with bounds 0x8136bb80 to 0x8136bbc0 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x8136bbc0 with bounds 0x8136bbc0 to 0x8136bc00 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x8136bc00 with bounds 0x8136bc00 to 0x8136bc40 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x8136bc40 with bounds 0x8136bc40 to 0x8136bc80 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[5] = 0x8136bc80 with bounds 0x8136bc80 to 0x8136bcc0 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[6] = 0x8136bcc0 with bounds 0x8136bcc0 to 0x8136bd00 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[7] = 0x8136bd00 with bounds 0x8136bd00 to 0x8136bd40 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
Block allocation from mem_block_02.....
  Requested length of each block: = 64 bytes, after WB_UP: 64 bytes
  Expected length of each block after CHERI alignment and rounding: = 64 bytes
   block: ptr[0] = 0x80aa9e00 with bounds 0x80aa9e00 to 0x80aa9e40 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[1] = 0x80aa9e40 with bounds 0x80aa9e40 to 0x80aa9e80 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[2] = 0x80aa9e80 with bounds 0x80aa9e80 to 0x80aa9ec0 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[3] = 0x80aa9ec0 with bounds 0x80aa9ec0 to 0x80aa9f00 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[4] = 0x80aa9f00 with bounds 0x80aa9f00 to 0x80aa9f40 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[5] = 0x80aa9f40 with bounds 0x80aa9f40 to 0x80aa9f80 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[6] = 0x80aa9f80 with bounds 0x80aa9f80 to 0x80aa9fc0 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
   block: ptr[7] = 0x80aa9fc0 with bounds 0x80aa9fc0 to 0x80aaa000 and size 64 bytes, tag: 0x1, checking....alignment: good, block length: good
Freeing blocks to mem_block_01.....
Freeing blocks to mem_block_02.....
 PASS - test_e_mem_block_bounds_orig in 0.052 seconds
===================================================================
TESTSUITE test_mem_block_cheri succeeded

------ TESTSUITE SUMMARY START ------

SUITE PASS - 100.00% [test_mem_block_cheri]: pass = 5, fail = 0, skip = 0, total = 5 duration = 0.432 seconds
 - PASS - [test_mem_block_cheri.test_a_mem_block_bounds] duration = 0.066 seconds
 - PASS - [test_mem_block_cheri.test_b_mem_block_block_bounds] duration = 0.124 seconds
 - PASS - [test_mem_block_cheri.test_c_mem_block_ext_bounds] duration = 0.066 seconds
 - PASS - [test_mem_block_cheri.test_d_mem_block_ext_block_bounds] duration = 0.124 seconds
 - PASS - [test_mem_block_cheri.test_e_mem_block_bounds_orig] duration = 0.052 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION SUCCESSFUL
