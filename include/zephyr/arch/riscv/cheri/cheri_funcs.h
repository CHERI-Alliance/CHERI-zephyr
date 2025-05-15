/*
 * Added to support CHERI codasip xa730, v0.9.x version of the CHERI spec. 2025, University of Birmingham
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CHERI_FUNCS_H_
#define CHERI_FUNCS_H_

#ifndef __ASSEMBLY__
void *cheri_build_device_cap(size_t address, size_t size);

extern void init_cap_relocs(void *data_cap, void *code_cap);
extern void init_cap_roots(void *root);
#endif



#endif /* CHERI_FUNCS_H_ */
