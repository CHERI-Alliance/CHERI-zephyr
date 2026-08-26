/*
 * Copyright (c) 2026 University of Birmingham, support for CHERI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

[[gnu::section(".compartment2.entry"), gnu::aligned(512)]]
void attack_entry(void *p1, void *p2, void *p3);
