
//
// Copyright (c) 2026 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>

#include "zenoh-pico/utils/hash.h"

#include <stddef.h>
#include <stdint.h>

// Define FNV1a parameters for 32 and 64-bit platforms
#if SIZE_MAX == UINT64_MAX
#define _Z_FNV_OFFSET_BASIS 14695981039346656037ULL
#define _Z_FNV_PRIME 1099511628211ULL
#elif SIZE_MAX == UINT32_MAX
#define _Z_FNV_OFFSET_BASIS 2166136261U
#define _Z_FNV_PRIME 16777619U
#else
#error "Unsupported size_t size"
#endif

size_t _z_hash_combine(size_t h1, size_t h2) {
    // Use a simple hash combining function (e.g., boost::hash_combine)
    h1 ^= (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    return h1;
}

size_t _z_fnv1_hash(const uint8_t *data, size_t len) {
    size_t hash = _Z_FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= _Z_FNV_PRIME;
    }
    return hash;
}
