// Copyright (c) 2026 T3 Foundation (www.t3gemstone.org)
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Pool allocator public API for TI AM67A Cortex-R5F.
// Include this header from application code to monitor pool usage.

#ifndef ZENOH_PICO_SYSTEM_TI_AM67A_MEM_POOL_H
#define ZENOH_PICO_SYSTEM_TI_AM67A_MEM_POOL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define Z_MEM_POOL_CLASS_COUNT 4U

typedef struct {
    size_t block_data_size;
    size_t total_blocks;
    size_t used_blocks;
    size_t peak_blocks;
} z_mem_pool_class_stats_t;

typedef struct {
    z_mem_pool_class_stats_t classes[Z_MEM_POOL_CLASS_COUNT];
    size_t fallback_allocs;
} z_mem_pool_stats_t;

void z_mem_pool_stats_get(z_mem_pool_stats_t *out);
void z_mem_pool_print_stats(void);

#ifdef __cplusplus
}
#endif

#endif  /* ZENOH_PICO_SYSTEM_TI_AM67A_MEM_POOL_H */
