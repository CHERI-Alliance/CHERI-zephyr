/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified to support CHERI 2025, University of Birmingham
 */

#include <zephyr/linker/iterable_sections.h>

#if defined(CONFIG_CHERI)
	ITERABLE_SECTION_ROM(intc_table, Z_LINK_ITERABLE_SUBALIGN)
#else

	ITERABLE_SECTION_ROM(intc_table, 4)
#endif
