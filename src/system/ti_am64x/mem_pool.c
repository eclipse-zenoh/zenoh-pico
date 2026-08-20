// Copyright (c) 2026 T3 Foundation (www.t3gemstone.org)
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Static pool allocator for TI AM64x Cortex-R5F.
// Provides z_malloc / z_free / z_realloc without heap fragmentation.
//
// Design: 4 size classes with fixed-count free-lists.
// Each block carries a 4-byte header storing its class index so that
// z_free() can locate the correct pool without any external metadata.
//
// Pool layout (DDR .bss, adjustable via Z_POOL_*_COUNT defines):
//   Class 0 —   64 B/block × 256 blocks =  16 KB  (small strings, key-exprs)
//   Class 1 —  256 B/block ×  64 blocks =  16 KB  (endpoint, addrinfo)
//   Class 2 — 1024 B/block ×  32 blocks =  32 KB  (transport state, session)
//   Class 3 — 4096 B/block ×   8 blocks =  32 KB  (I/O buffers, Z_BATCH_*_SIZE)
//   Total: 96 KB (tunable)
//
// Thread safety: FreeRTOS taskENTER_CRITICAL / taskEXIT_CRITICAL.
// Falls back to pvPortMalloc for allocations that exceed 4092 B (rare, logged).

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/system/platform/ti_am64x/mem_pool.h"
#include "zenoh-pico/utils/logging.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*------------------------------------------------------------------
 * Tuneable parameters
 *------------------------------------------------------------------*/
#ifndef Z_POOL_C0_COUNT
#define Z_POOL_C0_COUNT 256  /* 64 B blocks */
#endif
#ifndef Z_POOL_C1_COUNT
#define Z_POOL_C1_COUNT 64   /* 256 B blocks */
#endif
#ifndef Z_POOL_C2_COUNT
#define Z_POOL_C2_COUNT 32   /* 1024 B blocks */
#endif
#ifndef Z_POOL_C3_COUNT
#define Z_POOL_C3_COUNT 8    /* 4096 B blocks */
#endif

#define Z_POOL_CLASS_COUNT 4U

/*------------------------------------------------------------------
 * Block header (4 bytes, always at the start of each block).
 * When the block is on the free list, the bytes after the header
 * hold the next-free pointer.
 *------------------------------------------------------------------*/
typedef struct {
    uint8_t class_idx;
    uint8_t _pad[3];
} _z_pool_hdr_t;

#define Z_POOL_HDR_SIZE ((size_t)sizeof(_z_pool_hdr_t))  /* 4 */

/*------------------------------------------------------------------
 * Block sizes (total, including header)
 *------------------------------------------------------------------*/
static const size_t _z_pool_block_size[Z_POOL_CLASS_COUNT] = {64, 256, 1024, 4096};
static const size_t _z_pool_data_size[Z_POOL_CLASS_COUNT] = {
    64 - Z_POOL_HDR_SIZE,    /* 60 */
    256 - Z_POOL_HDR_SIZE,   /* 252 */
    1024 - Z_POOL_HDR_SIZE,  /* 1020 */
    4096 - Z_POOL_HDR_SIZE,  /* 4092 */
};

/*------------------------------------------------------------------
 * Backing store — static arrays in DDR .bss
 *------------------------------------------------------------------*/
static uint8_t _z_pool_mem0[Z_POOL_C0_COUNT][64]   __attribute__((aligned(8)));
static uint8_t _z_pool_mem1[Z_POOL_C1_COUNT][256]  __attribute__((aligned(8)));
static uint8_t _z_pool_mem2[Z_POOL_C2_COUNT][1024] __attribute__((aligned(8)));
static uint8_t _z_pool_mem3[Z_POOL_C3_COUNT][4096] __attribute__((aligned(8)));

static uint8_t *const _z_pool_base[Z_POOL_CLASS_COUNT] = {
    &_z_pool_mem0[0][0],
    &_z_pool_mem1[0][0],
    &_z_pool_mem2[0][0],
    &_z_pool_mem3[0][0],
};
static const size_t _z_pool_count[Z_POOL_CLASS_COUNT] = {
    Z_POOL_C0_COUNT, Z_POOL_C1_COUNT, Z_POOL_C2_COUNT, Z_POOL_C3_COUNT,
};

/*------------------------------------------------------------------
 * Free-list heads and diagnostics
 *------------------------------------------------------------------*/
static void *_z_pool_free_head[Z_POOL_CLASS_COUNT] = {NULL, NULL, NULL, NULL};
static bool  _z_pool_initialised = false;

static size_t _z_pool_used[Z_POOL_CLASS_COUNT]   = {0, 0, 0, 0};
static size_t _z_pool_peak[Z_POOL_CLASS_COUNT]   = {0, 0, 0, 0};
static size_t _z_pool_fallback_count             = 0;

/*------------------------------------------------------------------
 * Internal: build the free-list for one class
 *------------------------------------------------------------------*/
static void _z_pool_init_class(uint8_t cidx) {
    size_t  bsz   = _z_pool_block_size[cidx];
    size_t  count = _z_pool_count[cidx];
    uint8_t *base = _z_pool_base[cidx];

    _z_pool_free_head[cidx] = base;
    for (size_t i = 0; i < count; i++) {
        uint8_t *blk = base + i * bsz;
        ((_z_pool_hdr_t *)blk)->class_idx = cidx;
        void **next_slot = (void **)(blk + Z_POOL_HDR_SIZE);
        *next_slot = (i + 1 < count) ? (base + (i + 1) * bsz) : NULL;
    }
}

static void _z_pool_init(void) {
    for (uint8_t c = 0; c < Z_POOL_CLASS_COUNT; c++) {
        _z_pool_init_class(c);
    }
    _z_pool_initialised = true;
}

/*------------------------------------------------------------------
 * z_malloc
 *------------------------------------------------------------------*/
void *z_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    taskENTER_CRITICAL();
    if (!_z_pool_initialised) {
        _z_pool_init();
    }
    taskEXIT_CRITICAL();

    /* Find smallest fitting class */
    uint8_t cidx = Z_POOL_CLASS_COUNT;
    for (uint8_t c = 0; c < Z_POOL_CLASS_COUNT; c++) {
        if (size <= _z_pool_data_size[c]) {
            cidx = c;
            break;
        }
    }

    if (cidx < Z_POOL_CLASS_COUNT) {
        taskENTER_CRITICAL();
        void *blk = _z_pool_free_head[cidx];
        if (blk != NULL) {
            _z_pool_free_head[cidx] = *(void **)(((uint8_t *)blk) + Z_POOL_HDR_SIZE);
            _z_pool_used[cidx]++;
            if (_z_pool_used[cidx] > _z_pool_peak[cidx]) {
                _z_pool_peak[cidx] = _z_pool_used[cidx];
            }
        }
        taskEXIT_CRITICAL();

        if (blk != NULL) {
            return ((uint8_t *)blk) + Z_POOL_HDR_SIZE;
        }
        _Z_ERROR("zenoh-pico pool class %u exhausted, falling back to heap", (unsigned)cidx);
    }

    /* Oversized or pool-exhausted fallback */
    taskENTER_CRITICAL();
    _z_pool_fallback_count++;
    taskEXIT_CRITICAL();

    void *ptr = pvPortMalloc(size + Z_POOL_HDR_SIZE);
    if (ptr != NULL) {
        ((_z_pool_hdr_t *)ptr)->class_idx = 0xFF;
        return ((uint8_t *)ptr) + Z_POOL_HDR_SIZE;
    }
    return NULL;
}

/*------------------------------------------------------------------
 * z_free
 *------------------------------------------------------------------*/
void z_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    uint8_t *blk  = (uint8_t *)ptr - Z_POOL_HDR_SIZE;
    uint8_t  cidx = ((_z_pool_hdr_t *)blk)->class_idx;

    if (cidx == 0xFF) {
        vPortFree(blk);
        return;
    }

    if (cidx >= Z_POOL_CLASS_COUNT) {
        _Z_ERROR("zenoh-pico z_free: invalid class index %u", (unsigned)cidx);
        return;
    }

    taskENTER_CRITICAL();
    void **next_slot = (void **)(blk + Z_POOL_HDR_SIZE);
    *next_slot = _z_pool_free_head[cidx];
    _z_pool_free_head[cidx] = blk;
    if (_z_pool_used[cidx] > 0) {
        _z_pool_used[cidx]--;
    }
    taskEXIT_CRITICAL();
}

/*------------------------------------------------------------------
 * z_realloc
 *------------------------------------------------------------------*/
void *z_realloc(void *ptr, size_t new_size) {
    if (ptr == NULL) {
        return z_malloc(new_size);
    }
    if (new_size == 0) {
        z_free(ptr);
        return NULL;
    }

    uint8_t *blk      = (uint8_t *)ptr - Z_POOL_HDR_SIZE;
    uint8_t  cidx     = ((_z_pool_hdr_t *)blk)->class_idx;
    size_t   old_data = (cidx < Z_POOL_CLASS_COUNT) ? _z_pool_data_size[cidx] : new_size;

    void *new_ptr = z_malloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }
    size_t copy_size = (old_data < new_size) ? old_data : new_size;
    memcpy(new_ptr, ptr, copy_size);
    z_free(ptr);
    return new_ptr;
}

/*------------------------------------------------------------------
 * Pool statistics
 *------------------------------------------------------------------*/
void z_mem_pool_stats_get(z_mem_pool_stats_t *out) {
    if (out == NULL) {
        return;
    }
    taskENTER_CRITICAL();
    for (uint8_t c = 0; c < Z_POOL_CLASS_COUNT; c++) {
        out->classes[c].block_data_size = _z_pool_data_size[c];
        out->classes[c].total_blocks    = _z_pool_count[c];
        out->classes[c].used_blocks     = _z_pool_used[c];
        out->classes[c].peak_blocks     = _z_pool_peak[c];
    }
    out->fallback_allocs = _z_pool_fallback_count;
    taskEXIT_CRITICAL();
}

void z_mem_pool_print_stats(void) {
    z_mem_pool_stats_t s;
    z_mem_pool_stats_get(&s);
    _Z_INFO("=== zenoh-pico pool stats ===");
    for (uint8_t c = 0; c < Z_POOL_CLASS_COUNT; c++) {
        _Z_INFO("  class[%u] %4u B: used=%u peak=%u total=%u",
                (unsigned)c,
                (unsigned)s.classes[c].block_data_size,
                (unsigned)s.classes[c].used_blocks,
                (unsigned)s.classes[c].peak_blocks,
                (unsigned)s.classes[c].total_blocks);
    }
    _Z_INFO("  heap fallbacks: %u", (unsigned)s.fallback_allocs);
    _Z_INFO("=============================");
}
