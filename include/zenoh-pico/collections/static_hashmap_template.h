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
//

// Fixed-size hash map with separate chaining using a compile-time node pool.
//
// No heap allocation — all nodes live in an embedded array whose capacity is
// chosen at compile time.
//
// Design highlights:
//   * Nodes are allocated from a flat pool of _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
//     elements.
//   * Each bucket is the head of an intrusive singly-linked list whose links
//     are pool indices (not pointers).
//   * Iterators are pool indices and remain STABLE across insertions and
//     removals of other keys.
//   * The index type is automatically selected based on capacity (uint8_t,
//     uint16_t, or uint32_t).
//
// User must define the following macros before including this file:
//
// Required:
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE
//       type of the key
//   _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE
//       type of the value
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(key_ptr) -> size_t
//       hash function for the key
//
// Optional:
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(key_a_ptr, key_b_ptr) -> bool
//       equality function for keys (default: *key_a_ptr == *key_b_ptr)
//   _ZP_STATIC_HASHMAP_TEMPLATE_NAME
//       base name for all generated symbols
//       (default: _ZP_CAT(key_type, _ZP_CAT(val_type, hmap)))
//   _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY
//       maximum total number of entries that can be stored;
//       this is also used as the number of hash buckets
//       (default: 16)
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN(key_ptr)
//       destroy a key (default: no-op)
//   _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN(val_ptr)
//       destroy a value (default: no-op)
//   _ZP_STATIC_HASHMAP_TEMPLATE_KEY_MOVE_FN(dst_ptr, src_ptr)
//       move a key (default: copy without destroying src)
//   _ZP_STATIC_HASHMAP_TEMPLATE_VAL_MOVE_FN(dst_ptr, src_ptr)
//       move a value (default: copy without destroying src)

// ── Required macros ──────────────────────────────────────────────────────────

#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE
#error "_ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE must be defined before including static_hashmap_template.h"
#endif

#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN
#error "_ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN must be defined before including static_hashmap_template.h"
#endif

#ifndef _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE
#error "_ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE must be defined before including static_hashmap_template.h"
#endif

// Delegate to the shared internal template engine.
#include "zenoh-pico/collections/internal/static_hashmap_template_internal.h"
