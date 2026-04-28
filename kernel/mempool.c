/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

typedef void * (sys_heap_allocator_t)(struct sys_heap *heap, size_t align, size_t bytes);

static void *z_alloc_helper(struct k_heap *heap, size_t align, size_t size,
			    sys_heap_allocator_t sys_heap_allocator)
{
	void *mem;
	struct k_heap **heap_ref;
	size_t __align;
	k_spinlock_key_t key;

	/* A power of 2 as well as 0 is OK */
	__ASSERT((align & (align - 1)) == 0,
		"align must be a power of 2");

#ifdef __CHERI_PURE_CAPABILITY__
	/*
	 * For non-CHERI sys_heap_allocator alignment can be arbitrary allowing
	 * enough space for the header (of size pointer) to be stored before the
	 * start of the user pointer for data. | ptr header | user data |
	 *
	 * For CHERI however the sys_heap_allocator guarantees at least pointer
	 * alignment, so we need to add extra padding before the header to allow
	 * the correct user pointer alignment. |padding |ptr header | user data |
	 */
	size_t req_size;

	/* we ignore alignment requests below pointer size because already met */
	 __align = MAX(align, sizeof(void *));

	/* space for user data, ptr header, and worst‑case padding */
	/* hdr+data + __align-hdr in two steps */
	if (size_add_overflow(size, sizeof(heap_ref), &req_size) ||
		size_add_overflow(req_size,
		__align - sizeof(heap_ref), &req_size)) {
		return NULL;
	}

	key = k_spin_lock(&heap->lock);
	/* Request size with at-least pointer alignment which it does anyway */
	/* with CHERI the actual alignment maybe higher dep. on req_size */
	mem = sys_heap_allocator(&heap->heap, sizeof(void *), req_size);
	k_spin_unlock(&heap->lock, key);

	if (mem == NULL) {
		return NULL;
	}

	/* align user ptr */
	uintptr_t raw = (uintptr_t)mem;
	uintptr_t user_addr = ROUND_UP(raw + sizeof(heap_ref), __align);

	/* place header immediately before returned user ptr */
	heap_ref = (struct k_heap **)(user_addr - sizeof(heap_ref));
	*heap_ref = heap;
	mem = (void *)user_addr;

	__ASSERT(((uintptr_t)user_addr & (__align - 1)) == 0,
	"misaligned memory at %p (__align = %zu)",
	(void *)user_addr, __align);

	return mem;
#else
	/*
	 * Adjust the size to make room for our heap reference.
	 * Merge a rewind bit with align value (see sys_heap_aligned_alloc()).
	 * This allows for storing the heap pointer right below the aligned
	 * boundary without wasting any memory.
	 */
	if (size_add_overflow(size, sizeof(heap_ref), &size)) {
		return NULL;
	}
	__align = align | sizeof(heap_ref);

	/*
	 * No point calling k_heap_malloc/k_heap_aligned_alloc with K_NO_WAIT.
	 * Better bypass them and go directly to sys_heap_*() instead.
	 */
	key = k_spin_lock(&heap->lock);
	mem = sys_heap_allocator(&heap->heap, __align, size);
	k_spin_unlock(&heap->lock, key);

	if (mem == NULL) {
		return NULL;
	}

	heap_ref = mem;
	*heap_ref = heap;
	mem = ++heap_ref;
	__ASSERT(align == 0 || ((uintptr_t)mem & (align - 1)) == 0,
		 "misaligned memory at %p (align = %zu)", mem, align);

	return mem;
#endif
}

void k_free(void *ptr)
{
	struct k_heap **heap_ref;

	if (ptr != NULL) {
		heap_ref = ptr;
		--heap_ref;
#ifdef __CHERI_PURE_CAPABILITY__
	/* In cheri we need to take into account alignment padding
	 * when the alignment allocator is used since the
	 * underlying cap is always at least a pointer size aligned.
	 * |padding | header | user data |
	 */
	uintptr_t cap_addr = __builtin_cheri_address_get(ptr);
	uintptr_t cap_base = __builtin_cheri_base_get(ptr);
	size_t offset = cap_addr-cap_base;

	ptr = ptr-offset;
#else
		ptr = heap_ref;
#endif
		SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap_sys, k_free, *heap_ref, heap_ref);

		k_heap_free(*heap_ref, ptr);

		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap_sys, k_free, *heap_ref, heap_ref);
	}
}

#if (K_HEAP_MEM_POOL_SIZE > 0)

K_HEAP_DEFINE(_system_heap, K_HEAP_MEM_POOL_SIZE);
#define _SYSTEM_HEAP (&_system_heap)

void *k_aligned_alloc(size_t align, size_t size)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap_sys, k_aligned_alloc, _SYSTEM_HEAP);

	void *ret = z_alloc_helper(_SYSTEM_HEAP, align, size, sys_heap_aligned_alloc);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap_sys, k_aligned_alloc, _SYSTEM_HEAP, ret);

	return ret;
}

void *k_malloc(size_t size)
{
	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap_sys, k_malloc, _SYSTEM_HEAP);

	void *ret = z_alloc_helper(_SYSTEM_HEAP, 0, size, sys_heap_noalign_alloc);

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap_sys, k_malloc, _SYSTEM_HEAP, ret);

	return ret;
}

void *k_calloc(size_t nmemb, size_t size)
{
	void *ret;
	size_t bounds;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap_sys, k_calloc, _SYSTEM_HEAP);

	if (size_mul_overflow(nmemb, size, &bounds)) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap_sys, k_calloc, _SYSTEM_HEAP, NULL);

		return NULL;
	}

	ret = k_malloc(bounds);
	if (ret != NULL) {
#ifdef __CHERI_PURE_CAPABILITY__
		/* CHERI modified k_malloc may return a memory allocation > bounds
		 * we need to zero everything including the padding
		 * even if the user doesn't use it, to be safe.
		 */

		/* get the full length from base */
		size_t cheri_len = __builtin_cheri_length_get(ret);
		/* subtract the header pointer */
		cheri_len = cheri_len - sizeof(void *);
		/* Amend bounds to include any CHERI padding */
		bounds = cheri_len;
#endif
		(void)memset(ret, 0, bounds);
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap_sys, k_calloc, _SYSTEM_HEAP, ret);

	return ret;
}

void *k_realloc(void *ptr, size_t size)
{
	struct k_heap *heap, **heap_ref;
	k_spinlock_key_t key;
	void *ret;

	if (size == 0) {
		k_free(ptr);
		return NULL;
	}
	if (ptr == NULL) {
		return k_malloc(size);
	}
	heap_ref = ptr;
	ptr = --heap_ref;
	heap = *heap_ref;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(k_heap_sys, k_realloc, heap, ptr);

	if (size_add_overflow(size, sizeof(heap_ref), &size)) {
		SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap_sys, k_realloc, heap, ptr, NULL);
		return NULL;
	}

	/*
	 * No point calling k_heap_realloc() with K_NO_WAIT here.
	 * Better bypass it and go directly to sys_heap_realloc() instead.
	 */
	key = k_spin_lock(&heap->lock);
	ret = sys_heap_realloc(&heap->heap, ptr, size);
	k_spin_unlock(&heap->lock, key);

	if (ret != NULL) {
		heap_ref = ret;
#ifdef __CHERI_PURE_CAPABILITY__
		/* There are no guarantees from sys_heap that capabilities
		 * are copied during realloc, we therefore need to
		 * restore the header (pointer to k_heap struct)
		 */
		*heap_ref = heap;
#endif
		ret = ++heap_ref;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(k_heap_sys, k_realloc, heap, ptr, ret);

	return ret;
}

void k_thread_system_pool_assign(struct k_thread *thread)
{
	thread->resource_pool = _SYSTEM_HEAP;
}
#else
#define _SYSTEM_HEAP	NULL
#endif /* K_HEAP_MEM_POOL_SIZE */

static void *z_thread_alloc_helper(size_t align, size_t size,
				   sys_heap_allocator_t sys_heap_allocator)
{
	void *ret;
	struct k_heap *heap;

	if (k_is_in_isr()) {
		heap = _SYSTEM_HEAP;
	} else {
		heap = _current->resource_pool;
	}

	if (heap != NULL) {
		ret = z_alloc_helper(heap, align, size, sys_heap_allocator);
	} else {
		ret = NULL;
	}

	return ret;
}

void *z_thread_aligned_alloc(size_t align, size_t size)
{
	return z_thread_alloc_helper(align, size, sys_heap_aligned_alloc);
}

void *z_thread_malloc(size_t size)
{
	return z_thread_alloc_helper(0, size, sys_heap_noalign_alloc);
}
