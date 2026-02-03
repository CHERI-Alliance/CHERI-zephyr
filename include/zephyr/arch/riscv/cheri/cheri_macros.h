/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHERI_MACROS_H_
#define CHERI_MACROS_H_

#ifndef __ASSEMBLY__
/*--------------------------------------------------------------------------------*/
/* CHERI MEMORY ALIGNMENT */
/*--------------------------------------------------------------------------------*/
/*
 * CHERI memory allocation alignment:
 * See CHERI C++ programming guide.
 * Due to bounds compression, not all memory allocation requests can be exactly
 * bounded to the required alignment and length.
 * To obtain exact bounds requires increasing alignment and rounding up the length.
 *
 * For COMPILE-TIME alignment and length rounding use the macros here.
 * This can be used for example for backing buffers e.g mem_slabs.
 *  Three types of macros have been created:
 *  1) BIT-SHIFT table (with extra alignment step) based on Cambridge
 * CHERI C++ programming guide equations.
 *  2) LOOK-UP table of threshold values to match builtins.
 *  3) BIT-SHIFT table derived from threshold values.
 * A default macro is selected.
 *
 * There is currently a limitation placed on the macros for alignment and rounding of
 * memory lengths up to 16 MiB because of the impact on compile/build time.
 * Future development may improve the macros.
 *
 * For RUN-TIME alignment and length rounding use the CHERI‑recommended
 * compiler builtins which remain portable across architectures:
 *  __builtin_cheri_representable_alignment_mask(x)
 *  __builtin_cheri_round_representable_length(x)
 * The mask whose complement+1 is the required base alignment for length
 * (so required_alignment = ~mask + 1)
 * or using cheriintrin.h:
 *  size_t cheri_representable_alignment_mask(size_t len)
 *  size_t cheri_representable_length(size_t len)
 */

#ifdef __CHERI_PURE_CAPABILITY__

/* place limitation on memory lengths for macros to lengths of 16 MiB (2^24) */
#define CHERI_MACRO_MEM_MAX_LEN (1ULL << 24)

/* General CHERI round up to alignment */
#define CHERI_ROUND_UP(x, align)                                                                   \
	((((uintptr_t)(x) + ((size_t)(align) - 1)) / (size_t)(align)) * (size_t)(align))

#ifdef CONFIG_64BIT

/* Alignment for 64 bit RISCV64 */

/* For 64 bit, exact representability is guaranteed up to 4 KiB */
#define CHERI64_MIN_EXACT_BYTES 4096ULL

/*
 * 1) BIT-SHIFT table (with extra alignment step) based on Cambridge CHERI C++ programming guide
 * equations.
 *
 * Note this macro method is not limited to CHERI_MACRO_MEM_MAX_LEN
 * This is not the default macro due to compile/build time
 *
 * For 64 bit riscv:
 *  For arbitrary alignment, exact representability is guaranteed up to 4 KiB.
 *  After that, alignment goes up in powers of two:
 *  Requested length l	Required alignment (bytes)
 *         < 4 KiB	none (no extra CHERI alignment) 16 min for storing cap
 * 4 KiB   - 8  KiB	8
 * 8 KiB   - 16  KiB	16
 * 16 KiB  - 32 KiB	32
 * 32 KiB  - 64  KiB	64
 * 64 KiB  - 128  KiB	128
 * 128 KiB - 256  KiB	256
 * 256 KiB - 512 KiB	512
 * 512 KiB - 1 MiB	1024
 * 1 MiB   - 2 MiB	2048
 * 2 MiB   - 4 MiB	4096
 * and so on		doubles each power of two length step
 *
 * To find the representable length and alignment:
 * 1. Firstly you calculate the alignment based on the
 * non-rounded length:
 * Alignment = 2^(p-12+3), p=bit position of highest set bit
 * 2. Then use the alignment to round up the length
 * Representable length = roundUp(length/alignment) * length
 * 3. Then re-calculate the alignment based on
 * the rounded length. Most alignments stay the same
 * apart from those near the boundary which switch
 * to the next alignment threshold when rounded up.
 *
 * CHERI_ALIGN64_BIT_SHIFT_TABLE
 * 1. If len <= 4 KiB: return alignment of 1
 * 2. Otherwise return alignment based on length
 *   a.find highest set bit starting from bit 63 down to bit 12,
 *   this gives us the leading zeros.
 *   b.compute bitwidth E = (position - 12)
 *   c:then alignment = 2^(E+3),
 */
#define CHERI64_SHIFT  12
#define CHERI64_OFFSET 3

#define CHERI_ALIGN64_BIT_SHIFT_TABLE(len)                                                         \
	(((uint64_t)(len) < CHERI64_MIN_EXACT_BYTES)                                               \
		 ? 1ULL                                                                            \
		 : (1ULL << (((uint64_t)(len) >> 63)   ? (63 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 62) ? (62 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 61) ? (61 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 60) ? (60 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 59) ? (59 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 58) ? (58 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 57) ? (57 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 56) ? (56 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 55) ? (55 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 54) ? (54 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 53) ? (53 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 52) ? (52 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 51) ? (51 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 50) ? (50 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 49) ? (49 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 48) ? (48 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 47) ? (47 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 46) ? (46 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 45) ? (45 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 44) ? (44 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 43) ? (43 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 42) ? (42 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 41) ? (41 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 40) ? (40 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 39) ? (39 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 38) ? (38 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 37) ? (37 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 36) ? (36 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 35) ? (35 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 34) ? (34 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 33) ? (33 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 32) ? (32 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 31) ? (31 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 30) ? (30 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 29) ? (29 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 28) ? (28 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 27) ? (27 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 26) ? (26 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 25) ? (25 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 24) ? (24 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 23) ? (23 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 22) ? (22 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 21) ? (21 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 20) ? (20 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 19) ? (19 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 18) ? (18 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 17) ? (17 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 16) ? (16 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 15) ? (15 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 14) ? (14 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 13) ? (13 - CHERI64_SHIFT + CHERI64_OFFSET)     \
			     : ((uint64_t)(len) >> 12) ? (12 - CHERI64_SHIFT + CHERI64_OFFSET)     \
						       : 0)))

/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN64_BIT_SHIFT(len, min_align)                                                    \
	(((uint64_t)(len) < (uint64_t)CHERI64_MIN_EXACT_BYTES)                                     \
		 ? (uint64_t)(min_align)                                                           \
		 : (((uint64_t)(min_align) > CHERI_ALIGN64_BIT_SHIFT_TABLE((uint64_t)(len)))       \
			    ? (uint64_t)(min_align)                                                \
			    : CHERI_ALIGN64_BIT_SHIFT_TABLE((uint64_t)(len))))

/*
 * 2) LOOK-UP table of threshold values to match builtins
 *
 * Note this macro method is limited to CHERI_MACRO_MEM_MAX_LEN
 * This is not the default macro
 *
 * This is a look up table of threshold values and match the
 * actual builtins thresholds for alignment.
 * It is the same for codasip and cambridge architecture
 * No extra alignment steps are required.
 *
 * The actual input length threshold values are less
 * than a power-of-2. This is because after you round up the
 * length to the current alignment it can push it into the next
 * alignment threshold.
 * e.g (8185-8192)rounds up to 8192 with 8 byte align
 * hence the 8185 threshold for 16 bytes.
 *
 * (4096, 8)
 * (8185, 16) - (8185-8192)rounds up to 8192 with 8 byte align
 * (16369, 32) - (16369-16384)rounds up to 16384 with 16 byte align
 * (32737, 64)
 * (65473, 128)
 * (130945, 256)
 * (261889, 512)
 * (523777, 1024)
 * (1047553, 2048)
 * (2095105, 4096)
 * (4190209, 8192)
 * (8380417, 16384)
 * (16760833, 32768)
 */
#define CHERI_ALIGN64_FROM_TABLE(len)                                                              \
	(((uint64_t)(len) >= (uint64_t)16760833)  ? (uint64_t)32768                                \
	 : ((uint64_t)(len) >= (uint64_t)8380417) ? (uint64_t)16384                                \
	 : ((uint64_t)(len) >= (uint64_t)4190209) ? (uint64_t)8192                                 \
	 : ((uint64_t)(len) >= (uint64_t)2095105) ? (uint64_t)4096                                 \
	 : ((uint64_t)(len) >= (uint64_t)1047553) ? (uint64_t)2048                                 \
	 : ((uint64_t)(len) >= (uint64_t)523777)  ? (uint64_t)1024                                 \
	 : ((uint64_t)(len) >= (uint64_t)261889)  ? (uint64_t)512                                  \
	 : ((uint64_t)(len) >= (uint64_t)130945)  ? (uint64_t)256                                  \
	 : ((uint64_t)(len) >= (uint64_t)65473)   ? (uint64_t)128                                  \
	 : ((uint64_t)(len) >= (uint64_t)32737)   ? (uint64_t)64                                   \
	 : ((uint64_t)(len) >= (uint64_t)16369)   ? (uint64_t)32                                   \
	 : ((uint64_t)(len) >= (uint64_t)8185)    ? (uint64_t)16                                   \
	 : ((uint64_t)(len) >= (uint64_t)4096)    ? (uint64_t)8                                    \
						  : (uint64_t)1)

/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN64_PRECISE(len, min_align)                                                      \
	(((uint64_t)(len) < (uint64_t)CHERI64_MIN_EXACT_BYTES)                                     \
		 ? (uint64_t)(min_align)                                                           \
		 : (((uint64_t)(min_align) > CHERI_ALIGN64_FROM_TABLE((uint64_t)(len)))            \
			    ? (uint64_t)(min_align)                                                \
			    : CHERI_ALIGN64_FROM_TABLE((uint64_t)(len))))

/*
 *  3) BIT-SHIFT table derived from threshold values
 *
 * Note this macro method is limited to CHERI_MACRO_MEM_MAX_LEN
 * To extend, increase bit position checks beyond 24
 * This is currently set as the default macro and is suitable
 * for memory allocations up to 16MiB
 *
 * This is a bit shift table equivalent to the threshold values
 * to return the alignment in a single step and match the
 * builtins thresholds
 *
 * As per previous LOOK-UP TABLE, input length threshold values are less
 * than a power-of-2.
 */
#define CHERI_LEN_THRESHOLD(k) (((1ULL << (k)) - (1ULL << ((k) - 10))) + 1ULL)
#define CHERI_ALIGNMENT(k)     ((1ULL << ((k) - 9)))

#define CHERI_ALIGN64_BIT_SHIFT_SINGLE(len)                                                        \
	(((uint64_t)(len) >= CHERI_LEN_THRESHOLD(24))   ? CHERI_ALIGNMENT(24)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(23)) ? CHERI_ALIGNMENT(23)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(22)) ? CHERI_ALIGNMENT(22)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(21)) ? CHERI_ALIGNMENT(21)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(20)) ? CHERI_ALIGNMENT(20)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(19)) ? CHERI_ALIGNMENT(19)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(18)) ? CHERI_ALIGNMENT(18)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(17)) ? CHERI_ALIGNMENT(17)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(16)) ? CHERI_ALIGNMENT(16)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(15)) ? CHERI_ALIGNMENT(15)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(14)) ? CHERI_ALIGNMENT(14)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(13)) ? CHERI_ALIGNMENT(13)                      \
	 : ((uint64_t)(len) >= CHERI_LEN_THRESHOLD(12)) ? CHERI_ALIGNMENT(12)                      \
							: CHERI_ALIGNMENT(12))

/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN64_SINGLE(len, min_align)                                                       \
	(((uint64_t)(len) < (uint64_t)CHERI64_MIN_EXACT_BYTES)                                     \
		 ? (uint64_t)(min_align)                                                           \
		 : (((uint64_t)(min_align) > CHERI_ALIGN64_BIT_SHIFT_SINGLE((uint64_t)(len)))      \
			    ? (uint64_t)(min_align)                                                \
			    : CHERI_ALIGN64_BIT_SHIFT_SINGLE((uint64_t)(len))))

#else

/* Alignment for 32 bit RISCV32 */

#ifdef CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI
/* codasip architecture */

/* For 32 bit codasip, exact representability is guaranteed up to 512 bytes */
#define CHERI32_MIN_EXACT_BYTES 512ULL

/*
 * 1) BIT-SHIFT table (with extra alignment step)
 *
 * Note this macro method is not limited to CHERI_MACRO_MEM_MAX_LEN
 * This is not the default macro due to compile/build time
 *
 * For 32 bit riscv codasip:
 *  Requested length l	Required alignment (bytes)
 *        < 512
 *  512 - 1 KiB		8 byte min for storing cap
 *  1 KiB - 2 KiB	16
 *  2 KiB - 4 KiB	32
 *  4 KiB - 8  KiB	64
 *  8 KiB - 16  KiB	128
 *  16 KiB - 32 KiB	256
 *  32 KiB - 64  KiB	512
 *  64 KiB - 128  KiB	1024
 *  128 KiB - 256  KiB	2048
 *  256 KiB - 512 KiB	4096
 *  512 KiB - 1 MiB	8192
 *  and so on		doubles each power of two length step
 *
 * To find the representable length and alignment:
 * 1. Firstly you calculate the alignment based on the
 * non-rounded length:
 * Alignment = 2^(p-9+3), p=bit position of highest set bit
 * 2. Then use the alignment to round up the length
 * Representable length = roundUp(length/alignment) * length
 * 3. Then re-calculate the alignment based on
 * the rounded length. Most alignments stay the same
 * apart from those near the boundary which switch
 * to the next alignment threshold when rounded up.
 */
#define CHERI32_SHIFT           9
#define CHERI32_OFFSET          3
#define CHERI_ALIGN32_BIT_SHIFT_TABLE(len)                                                         \
	(((uint32_t)(len) < (CHERI32_MIN_EXACT_BYTES))                                             \
		 ? (1ULL)                                                                          \
		 : (1U << (((uint32_t)(len) >> 31)   ? (31 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 30) ? (30 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 29) ? (29 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 28) ? (28 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 27) ? (27 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 26) ? (26 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 25) ? (25 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 24) ? (24 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 23) ? (23 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 22) ? (22 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 21) ? (21 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 20) ? (20 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 19) ? (19 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 18) ? (18 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 17) ? (17 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 16) ? (16 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 15) ? (15 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 14) ? (14 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 13) ? (13 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 12) ? (12 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 11) ? (11 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 10) ? (10 - CHERI32_SHIFT + CHERI32_OFFSET)       \
			   : ((uint32_t)(len) >> 9)  ? (9 - CHERI32_SHIFT + CHERI32_OFFSET)        \
						     : 0)))

/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN32_BIT_SHIFT(len, min_align)                                                    \
	(((uint32_t)(len) < (uint32_t)CHERI32_MIN_EXACT_BYTES)                                     \
		 ? (uint32_t)(min_align)                                                           \
		 : (((uint32_t)(min_align) > CHERI_ALIGN32_BIT_SHIFT_TABLE((uint32_t)(len)))       \
			    ? (uint32_t)(min_align)                                                \
			    : CHERI_ALIGN32_BIT_SHIFT_TABLE((uint32_t)(len))))

/*
 * 2) LOOK-UP table of threshold values to match builtins
 *
 * Note this macro method is limited to CHERI_MACRO_MEM_MAX_LEN
 * This is not the default macro
 *
 * This is a look up table of threshold values and match the
 * actual builtins thresholds for alignment.
 * It is the same for codasip and cambridge architecture
 * No extra alignment steps are required.
 *
 * The actual input length threshold values are less
 * than a power-of-2. This is because after you round up the
 * length to the current alignment it can push it into the next
 * alignment threshold.
 * e.g (1017-1024)rounds up to 1024 with 8 byte align
 * hence the 1017 threshold for 16 bytes.
 *
 * (0, 1)
 * (512, 8)
 * (1017, 16) - (1017-1024)rounds up to 1024 with 8 byte align
 * (2033, 32)
 * (4065, 64)
 * (8129, 128)
 * (16257, 256)
 * (32513, 512)
 * (65025, 1024)
 * (130049, 2048)
 * (260097, 4096)
 * (520193, 8192)
 * (1040385, 16384)
 * (2080769, 32768)
 * (4161537, 65536)
 * (8323073, 131072)
 * (16646145, 262144)
 */
#define CHERI_ALIGN32_FROM_TABLE(len)                                                              \
	(((uint32_t)(len) >= (uint32_t)16646145)  ? (uint32_t)262144                               \
	 : ((uint32_t)(len) >= (uint32_t)8323073) ? (uint32_t)131072                               \
	 : ((uint32_t)(len) >= (uint32_t)4161537) ? (uint32_t)65536                                \
	 : ((uint32_t)(len) >= (uint32_t)2080769) ? (uint32_t)32768                                \
	 : ((uint32_t)(len) >= (uint32_t)1040385) ? (uint32_t)16384                                \
	 : ((uint32_t)(len) >= (uint32_t)520193)  ? (uint32_t)8192                                 \
	 : ((uint32_t)(len) >= (uint32_t)260097)  ? (uint32_t)4096                                 \
	 : ((uint32_t)(len) >= (uint32_t)130049)  ? (uint32_t)2048                                 \
	 : ((uint32_t)(len) >= (uint32_t)65025)   ? (uint32_t)1024                                 \
	 : ((uint32_t)(len) >= (uint32_t)32513)   ? (uint32_t)512                                  \
	 : ((uint32_t)(len) >= (uint32_t)16257)   ? (uint32_t)256                                  \
	 : ((uint32_t)(len) >= (uint32_t)8129)    ? (uint32_t)128                                  \
	 : ((uint32_t)(len) >= (uint32_t)4065)    ? (uint32_t)64                                   \
	 : ((uint32_t)(len) >= (uint32_t)2033)    ? (uint32_t)32                                   \
	 : ((uint32_t)(len) >= (uint32_t)1017)    ? (uint32_t)16                                   \
	 : ((uint32_t)(len) >= (uint32_t)512)     ? (uint32_t)8                                    \
						  : (uint32_t)1)
/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN32_PRECISE(len, min_align)                                                      \
	(((uint32_t)(len) < (uint32_t)CHERI32_MIN_EXACT_BYTES)                                     \
		 ? (uint32_t)(min_align)                                                           \
		 : (((uint32_t)(min_align) > CHERI_ALIGN32_FROM_TABLE((uint32_t)(len)))            \
			    ? (uint32_t)(min_align)                                                \
			    : CHERI_ALIGN32_FROM_TABLE((uint32_t)(len))))

/*
 *  3) BIT-SHIFT table derived from threshold values
 *
 * Note this macro method is limited to CHERI_MACRO_MEM_MAX_LEN
 * To extend, increase bit position checks beyond 24
 * This is currently set as the default macro and is suitable
 * for memory allocations up to 16MiB
 *
 * This is a bit shift table equivalent to the threshold values
 * to return the alignment in a single step and match the
 * builtins thresholds
 *
 * As per previous LOOK-UP TABLE, input length threshold values are less
 * than a power-of-2.
 */
#define CHERI32_LEN_THRESHOLD(k) (((1ULL << (k)) - (1ULL << ((k) - 7))) + 1ULL)
#define CHERI32_ALIGNMENT(k)     ((1ULL << ((k) - 6)))

#define CHERI_ALIGN32_BIT_SHIFT_SINGLE(len)                                                        \
	(((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(24))   ? CHERI32_ALIGNMENT(24)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(23)) ? CHERI32_ALIGNMENT(23)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(22)) ? CHERI32_ALIGNMENT(22)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(21)) ? CHERI32_ALIGNMENT(21)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(20)) ? CHERI32_ALIGNMENT(20)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(19)) ? CHERI32_ALIGNMENT(19)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(18)) ? CHERI32_ALIGNMENT(18)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(17)) ? CHERI32_ALIGNMENT(17)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(16)) ? CHERI32_ALIGNMENT(16)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(15)) ? CHERI32_ALIGNMENT(15)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(14)) ? CHERI32_ALIGNMENT(14)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(13)) ? CHERI32_ALIGNMENT(13)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(12)) ? CHERI32_ALIGNMENT(12)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(11)) ? CHERI32_ALIGNMENT(11)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(10)) ? CHERI32_ALIGNMENT(10)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(9))  ? CHERI32_ALIGNMENT(9)                   \
							  : CHERI32_ALIGNMENT(9))

/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN32_SINGLE(len, min_align)                                                       \
	(((uint32_t)(len) < (uint32_t)CHERI32_MIN_EXACT_BYTES)                                     \
		 ? (uint32_t)(min_align)                                                           \
		 : (((uint32_t)(min_align) > CHERI_ALIGN32_BIT_SHIFT_SINGLE((uint32_t)(len)))      \
			    ? (uint32_t)(min_align)                                                \
			    : CHERI_ALIGN32_BIT_SHIFT_SINGLE((uint32_t)(len))))

#else

/* cambridge architecture */

/* For 32 bit codasip, exact representability is guaranteed up to 64 bytes */
#define CHERI32_MIN_EXACT_BYTES 64ULL

/*
 * 1) BIT-SHIFT table (with extra alignment step).
 *
 * Note this macro method is not limited to CHERI_MACRO_MEM_MAX_LEN
 * This is not the default macro due to compile/build time
 *
 * For 32 bit riscv cambridge:
 *  Requested length l	Required alignment (bytes)
 *       < 64
 *  64 - 128		8 byte min for storing cap
 *  128 - 256		16
 *  256 - 512		32
 *  512 - 1 KiB		64
 *  1 KiB - 2 KiB	128
 *  2 KiB - 4 KiB	256
 *  4 KiB - 8  KiB	512
 *  8 KiB - 16  KiB	1024
 *  16 KiB - 32 KiB	2048
 *  32 KiB - 64  KiB	4096
 *  64 KiB - 128  KiB	8192
 *  128 KiB - 256  KiB	16384
 *  256 KiB - 512 KiB	32768
 *  512 KiB - 1 MiB	65536
 *  and so on		doubles each power of two length step
 *
 * To find the representable length and alignment:
 * 1. Firstly you calculate the alignment based on the
 * non-rounded length:
 * Alignment = 2^(p-6+3), p=bit position of highest set bit
 * 2. Then use the alignment to round up the length
 * Representable length = roundUp(length/alignment) * length
 * 3. Then re-calculate the alignment based on
 * the rounded length. Most alignments stay the same
 * apart from those near the boundary which switch
 * to the next alignment threshold when rounded up.
 */
#define CHERI32_SHIFT           6
#define CHERI32_OFFSET          3
#define CHERI_ALIGN32_BIT_SHIFT_TABLE(len)                                                         \
	(((uint32_t)(len) < (CHERI32_MIN_EXACT_BYTES))                                             \
		 ? (1ULL)                                                                          \
		 : (1ULL << (((uint32_t)(len) >> 31)   ? (31 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 30) ? (30 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 29) ? (29 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 28) ? (28 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 27) ? (27 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 26) ? (26 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 25) ? (25 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 24) ? (24 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 23) ? (23 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 22) ? (22 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 21) ? (21 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 20) ? (20 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 19) ? (19 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 18) ? (18 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 17) ? (17 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 16) ? (16 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 15) ? (15 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 14) ? (14 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 13) ? (13 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 12) ? (12 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 11) ? (11 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 10) ? (10 - CHERI32_SHIFT + CHERI32_OFFSET)     \
			     : ((uint32_t)(len) >> 9)  ? (9 - CHERI32_SHIFT + CHERI32_OFFSET)      \
			     : ((uint32_t)(len) >> 8)  ? (8 - CHERI32_SHIFT + CHERI32_OFFSET)      \
			     : ((uint32_t)(len) >> 7)  ? (7 - CHERI32_SHIFT + CHERI32_OFFSET)      \
			     : ((uint32_t)(len) >> 6)  ? (6 - CHERI32_SHIFT + CHERI32_OFFSET)      \
						       : 0)))

/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN32_BIT_SHIFT(len, min_align)                                                    \
	(((uint32_t)(len) < (uint32_t)CHERI32_MIN_EXACT_BYTES)                                     \
		 ? (uint32_t)(min_align)                                                           \
		 : (((uint32_t)(min_align) > CHERI_ALIGN32_BIT_SHIFT_TABLE((uint32_t)(len)))       \
			    ? (uint32_t)(min_align)                                                \
			    : CHERI_ALIGN32_BIT_SHIFT_TABLE((uint32_t)(len))))

/*
 * 2) LOOK-UP table of threshold values to match builtins
 *
 * Note this macro method is limited to CHERI_MACRO_MEM_MAX_LEN
 * This is not the default macro
 *
 * This is a look up table of threshold values and match the
 * actual builtins thresholds for alignment.
 * It is the same for codasip and cambridge architecture
 * No extra alignment steps are required.
 *
 * The actual input length threshold values are less
 * than a power-of-2. This is because after you round up the
 * length to the current alignment it can push it into the next
 * alignment threshold.
 * e.g (121-128)rounds up to 128 with 8 byte align
 * hence the 121 threshold for 16 bytes.
 *
 * (0, 1)
 * (64, 8)
 * (121, 16) - (121-128)rounds up to 128 with 8 byte align
 * (241, 32)
 * (481, 64)
 * (961, 128)
 * (1921, 256)
 * (3841, 512)
 * (7681, 1024)
 * (15361, 2048)
 * (30721, 4096)
 * (61441, 8192)
 * (122881, 16384)
 * (245761, 32768)
 * (491521, 65536)
 * (983041, 131072)
 * (1966081, 262144)
 * (3932161, 524288)
 * (7864321, 1048576)
 * (15728641, 2097152)
 */
#define CHERI_ALIGN32_FROM_TABLE(len)                                                              \
	(((uint32_t)(len) >= (uint32_t)15728641)  ? (uint32_t)2097152                              \
	 : ((uint32_t)(len) >= (uint32_t)7864321) ? (uint32_t)1048576                              \
	 : ((uint32_t)(len) >= (uint32_t)3932161) ? (uint32_t)524288                               \
	 : ((uint32_t)(len) >= (uint32_t)1966081) ? (uint32_t)262144                               \
	 : ((uint32_t)(len) >= (uint32_t)983041)  ? (uint32_t)131072                               \
	 : ((uint32_t)(len) >= (uint32_t)491521)  ? (uint32_t)65536                                \
	 : ((uint32_t)(len) >= (uint32_t)245761)  ? (uint32_t)32768                                \
	 : ((uint32_t)(len) >= (uint32_t)122881)  ? (uint32_t)16384                                \
	 : ((uint32_t)(len) >= (uint32_t)61441)   ? (uint32_t)8192                                 \
	 : ((uint32_t)(len) >= (uint32_t)30721)   ? (uint32_t)4096                                 \
	 : ((uint32_t)(len) >= (uint32_t)15361)   ? (uint32_t)2048                                 \
	 : ((uint32_t)(len) >= (uint32_t)7681)    ? (uint32_t)1024                                 \
	 : ((uint32_t)(len) >= (uint32_t)3841)    ? (uint32_t)512                                  \
	 : ((uint32_t)(len) >= (uint32_t)1921)    ? (uint32_t)256                                  \
	 : ((uint32_t)(len) >= (uint32_t)961)     ? (uint32_t)128                                  \
	 : ((uint32_t)(len) >= (uint32_t)481)     ? (uint32_t)64                                   \
	 : ((uint32_t)(len) >= (uint32_t)241)     ? (uint32_t)32                                   \
	 : ((uint32_t)(len) >= (uint32_t)121)     ? (uint32_t)16                                   \
	 : ((uint32_t)(len) >= (uint32_t)64)      ? (uint32_t)8                                    \
						  : (uint32_t)1)

/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN32_PRECISE(len, min_align)                                                      \
	(((uint32_t)(len) < (uint32_t)CHERI32_MIN_EXACT_BYTES)                                     \
		 ? (uint32_t)(min_align)                                                           \
		 : (((uint32_t)(min_align) > CHERI_ALIGN32_FROM_TABLE((uint32_t)(len)))            \
			    ? (uint32_t)(min_align)                                                \
			    : CHERI_ALIGN32_FROM_TABLE((uint32_t)(len))))

/*
 *  3) BIT-SHIFT table derived from threshold values
 *
 * Note this macro method is limited to CHERI_MACRO_MEM_MAX_LEN
 * To extend, increase bit position checks beyond 24
 * This is currently set as the default macro and is suitable
 * for memory allocations up to 16MiB
 *
 * This is a bit shift table equivalent to the threshold values
 * to return the alignment in a single step and match the
 * builtins thresholds
 *
 * As per previous LOOK-UP TABLE, input length threshold values are less
 * than a power-of-2.
 */
#define CHERI32_LEN_THRESHOLD(k) (((1ULL << (k)) - (1ULL << ((k) - 4))) + 1ULL)
#define CHERI32_ALIGNMENT(k)     ((1ULL << ((k) - 3)))

#define CHERI_ALIGN32_BIT_SHIFT_SINGLE(len)                                                        \
	(((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(24))   ? CHERI32_ALIGNMENT(24)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(23)) ? CHERI32_ALIGNMENT(23)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(22)) ? CHERI32_ALIGNMENT(22)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(21)) ? CHERI32_ALIGNMENT(21)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(20)) ? CHERI32_ALIGNMENT(20)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(19)) ? CHERI32_ALIGNMENT(19)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(18)) ? CHERI32_ALIGNMENT(18)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(17)) ? CHERI32_ALIGNMENT(17)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(16)) ? CHERI32_ALIGNMENT(16)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(15)) ? CHERI32_ALIGNMENT(15)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(14)) ? CHERI32_ALIGNMENT(14)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(13)) ? CHERI32_ALIGNMENT(13)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(12)) ? CHERI32_ALIGNMENT(12)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(11)) ? CHERI32_ALIGNMENT(11)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(10)) ? CHERI32_ALIGNMENT(10)                  \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(9))  ? CHERI32_ALIGNMENT(9)                   \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(8))  ? CHERI32_ALIGNMENT(8)                   \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(7))  ? CHERI32_ALIGNMENT(7)                   \
	 : ((uint32_t)(len) >= CHERI32_LEN_THRESHOLD(6))  ? CHERI32_ALIGNMENT(6)                   \
							  : CHERI32_ALIGNMENT(6))

/* Return alignment or minimum alignment, whichever is higher */
#define CHERI_ALIGN32_SINGLE(len, min_align)                                                       \
	(((uint32_t)(len) < (uint32_t)CHERI32_MIN_EXACT_BYTES)                                     \
		 ? (uint32_t)(min_align)                                                           \
		 : (((uint32_t)(min_align) > CHERI_ALIGN32_BIT_SHIFT_SINGLE((uint32_t)(len)))      \
			    ? (uint32_t)(min_align)                                                \
			    : CHERI_ALIGN32_BIT_SHIFT_SINGLE((uint32_t)(len))))

#endif /* CONFIG_RISCV_ISA_ZCHERIPURECAP_ABI */
#endif /* CONFIG_64BIT */

/* length rounding and final alignment calculations */
#if CONFIG_64BIT
/*64 bit*/

/*
 * 1) BIT-SHIFT table (with extra alignment step):
 * 1.round up the length based on the calculated alignment of the length,
 * or at least the minimum alignment (min_align)
 * 2. calculate the over-all alignment based on the rounded up length to
 * check for boundary condition jump to the next alignment section
 */
#define CHERI_ROUND_UP_TO_REP_LEN_BIT_SHIFT(len, min_align)                                        \
	CHERI_ROUND_UP(len, CHERI_ALIGN64_BIT_SHIFT(len, min_align))
#define CHERI_ALIGN_FOR_LEN_BIT_SHIFT(len, min_align)                                              \
	CHERI_ALIGN64_BIT_SHIFT(CHERI_ROUND_UP_TO_REP_LEN_BIT_SHIFT(len, min_align), min_align)

/*
 * 2) LOOK-UP table of threshold values to match builtins:
 * 1.round up the length based on the calculated alignment of the length,
 * or at least the minimum alignment (min_align)
 * 2. calculate the alignment
 */
#define CHERI_ROUND_UP_TO_REP_LEN_PRECISE(len, min_align)                                          \
	CHERI_ROUND_UP(len, CHERI_ALIGN64_PRECISE(len, min_align))
#define CHERI_ALIGN_FOR_LEN_PRECISE(len, min_align) CHERI_ALIGN64_PRECISE(len, min_align)

/*
 * 3) BIT-SHIFT table derived from threshold values
 * 1.round up the length based on the calculated alignment of the length,
 * or at least the minimum alignment (min_align)
 * 2. calculate the alignment
 */
#define CHERI_ROUND_UP_TO_REP_LEN_SINGLE(len, min_align)                                           \
	CHERI_ROUND_UP(len, CHERI_ALIGN64_SINGLE(len, min_align))
#define CHERI_ALIGN_FOR_LEN_SINGLE(len, min_align) CHERI_ALIGN64_SINGLE(len, min_align)

/* define the exact representability guarantee */
#define CHERI_EXACT_SIZE_LIMIT CHERI64_MIN_EXACT_BYTES

#else
/* 32 bit*/

/*
 * 1) BIT-SHIFT table (with extra alignment step):
 * 1.round up the length based on the calculated alignment of the length,
 * or at least the minimum alignment (min_align)
 * 2. calculate the over-all alignment based on the rounded up length to
 * check for boundary condition jump to the next alignment section
 */
#define CHERI_ROUND_UP_TO_REP_LEN_BIT_SHIFT(len, min_align)                                        \
	CHERI_ROUND_UP(len, CHERI_ALIGN32_BIT_SHIFT(len, min_align))
#define CHERI_ALIGN_FOR_LEN_BIT_SHIFT(len, min_align)                                              \
	CHERI_ALIGN32_BIT_SHIFT(CHERI_ROUND_UP_TO_REP_LEN_BIT_SHIFT(len, min_align), min_align)

/*
 * 2) LOOK-UP table of threshold values to match builtins:
 * 1.round up the length based on the calculated alignment of the length,
 * or at least the minimum alignment (min_align)
 * 2. calculate the alignment
 */
#define CHERI_ROUND_UP_TO_REP_LEN_PRECISE(len, min_align)                                          \
	CHERI_ROUND_UP(len, CHERI_ALIGN32_PRECISE(len, min_align))
#define CHERI_ALIGN_FOR_LEN_PRECISE(len, min_align) CHERI_ALIGN32_PRECISE(len, min_align)

/*
 * 3) BIT-SHIFT table derived from threshold values
 * 1.round up the length based on the calculated alignment of the length,
 * or at least the minimum alignment (min_align)
 * 2. calculate the alignment
 */
#define CHERI_ROUND_UP_TO_REP_LEN_SINGLE(len, min_align)                                           \
	CHERI_ROUND_UP(len, CHERI_ALIGN32_SINGLE(len, min_align))
#define CHERI_ALIGN_FOR_LEN_SINGLE(len, min_align) CHERI_ALIGN32_SINGLE(len, min_align)

/* define the exact representability guarantee */
#define CHERI_EXACT_SIZE_LIMIT                     CHERI32_MIN_EXACT_BYTES

#endif /* CONFIG_64BIT */

/* set the default macros - limited to CHERI_MACRO_MEM_MAX_LEN */
#define CHERI_ROUND_UP_TO_REP_LEN(len, min_align) CHERI_ROUND_UP_TO_REP_LEN_SINGLE(len, min_align)
#define CHERI_ALIGN_FOR_LEN(len, min_align)       CHERI_ALIGN_FOR_LEN_SINGLE(len, min_align)

#endif /* __CHERI_PURE_CAPABILITY__ */

#endif /*ifndef __ASSEMBLY__*/

#endif /* CHERI_MACROS_H_ */
