/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 University of Birmingham, Modified to support CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>

#ifdef __CHERI_PURE_CAPABILITY__
/* for CHERI purecap, memory slabs need larger block sizes and alignment.
 * The static allocation K_MEM_SLAB_DEFINE macro does not include any size
 * or alignment checks even though it specifies the following:
 *
 * "The memory slab's buffer contains @a slab_num_blocks memory blocks
 * that are @a slab_block_size bytes long. The buffer is aligned to a
 * @a slab_align -byte boundary. To ensure that each memory block is similarly
 * aligned to this boundary, @a slab_block_size must also be a multiple of
 * @a slab_align."
 *
 * For the dynamic allocation k_mem_slab_init the buffer alignment and size
 * also needs specifying.
 *
 * The tests here specify the size and alignment so this needs to be adjusted
 * correctly for CHERI.
 *
 */

#define CHERI_ALIGN 16 /* work for 64/32 bit */

#define LOOP 10
/* stack alignment is always 16 bytes for RISCV,
 * include/zephyr/arch/riscv/arch.h:#define ARCH_STACK_PTR_ALIGN 16
 */
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_NUM 4
#define SLAB_NUM 2
#define TIMEOUT K_MSEC(200)
#define BLK_NUM 3
#define BLK_ALIGN CHERI_ALIGN /* multiple of global alignment */
#define BLK_SIZE1 BLK_ALIGN * 2 /* multiple of slab alignment (BLK_ALIGN) */
#define BLK_SIZE2 BLK_ALIGN /* multiple of slab alignment (BLK_ALIGN) */

#else
#define LOOP 10
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_NUM 4
#define SLAB_NUM 2
#define TIMEOUT K_MSEC(200)
#define BLK_NUM 3
#define BLK_ALIGN 8
#define BLK_SIZE1 16
#define BLK_SIZE2 8
#endif

/* Blocks per slab.  Note this number carefully, because if it is
 * smaller than this the test can deadlock.  There are 4 threads
 * allocating 3 blocks each, so it's possible in exhaustion situations
 * for all four threads to have already allocated 2 blocks, with three
 * of them having gone to sleep waiting on a fourth.  There must
 * remain at least one free block for that final allocation, otherwise
 * all four threads will end up sleeping forever (or until the
 * allocation timeout hits and the test fails).
 */
#define SLAB_BLOCKS (THREAD_NUM * (BLK_NUM - 1) + 1)

K_MEM_SLAB_DEFINE(mslab1, BLK_SIZE1, SLAB_BLOCKS, BLK_ALIGN);
static struct k_mem_slab mslab2, *slabs[SLAB_NUM] = { &mslab1, &mslab2 };
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREAD_NUM, STACK_SIZE);
static struct k_thread tdata[THREAD_NUM];
static char __aligned(BLK_ALIGN) tslab[BLK_SIZE2 * SLAB_BLOCKS];
static atomic_t slab_id;
static volatile bool success[THREAD_NUM];

/* thread entry simply invoke the APIs*/
static void tmslab_api(void *p1, void *p2, void *p3)
{
	void *block[BLK_NUM];
	int id = atomic_inc(&slab_id);
	struct k_mem_slab *slab = slabs[id % SLAB_NUM];
	int j = LOOP, ret;

	while (j--) {
		(void)memset(block, 0, sizeof(block));

		for (int i = 0; i < BLK_NUM; i++) {
			ret = k_mem_slab_alloc(slab, &block[i], TIMEOUT);
			zassert_false(ret, "memory is not allocated");
		}
		for (int i = 0; i < BLK_NUM; i++) {
			if (block[i]) {
				k_mem_slab_free(slab, block[i]);
				block[i] = NULL;
			}
		}
	}
	success[id] = true;
}

/* test cases*/
/**
 * @brief Verify alloc and free from multiple equal priority threads
 *
 * @details Test creates 4 preemptive threads of equal priority. Then
 * validates the synchronization of threads by allocating and
 * freeing up the memory blocks in memory slab.
 *
 * @ingroup kernel_memory_slab_tests
 */
ZTEST(mslab_threadsafe, test_mslab_threadsafe)
{
	k_tid_t tid[THREAD_NUM];

	k_mem_slab_init(&mslab2, tslab, BLK_SIZE2, SLAB_BLOCKS);

	/* create multiple threads to invoke same memory slab APIs*/
	for (int i = 0; i < THREAD_NUM; i++) {
		tid[i] = k_thread_create(&tdata[i], tstack[i], STACK_SIZE,
					 tmslab_api, NULL, NULL, NULL,
					 K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}

	/* wait for completion */
	for (int i = 0; i < THREAD_NUM; i++) {
		int ret = k_thread_join(tid[i], K_FOREVER);

		zassert_false(ret, "k_thread_join() failed");
		zassert_true(success[i], "thread %d failed", i);
	}
}
