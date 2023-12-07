//
// Copyright (c) 2023 ZettaScale Technology
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

#ifndef ZENOH_PICO_COLLECTIONS_REFCOUNT_H
#define ZENOH_PICO_COLLECTIONS_REFCOUNT_H

#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/collections/pointer.h"

typedef struct ref {
    void (*free)(const struct ref *);
    int count;
} _z_rc_t;

#if Z_FEATURE_MULTI_THREAD == 1
#if ZENOH_C_STANDARD != 99
#if !defined(ZENOH_NO_STDATOMIC)

#else
#error "Implement Windows atomics"
#endif  // !defined(ZENOH_NO_STDATOMIC)

static inline void ref_inc(const struct ref *ref) {
    _z_atomic_fetch_add_explicit((int *)&ref->count, 1, _z_memory_order_relaxed);
}

static inline void ref_dec(const struct ref *ref) {
    if (_z_atomic_fetch_sub_explicit((int *)&ref->count, 1, _z_memory_order_relaxed) == 1) {
        ref->free(ref);
    }
}

#else
// FIXME Check for Windows
static inline void ref_inc(const struct ref *ref) { __sync_add_and_fetch((int *)&ref->count, 1); }

static inline void ref_dec(const struct ref *ref) {
    if (__sync_sub_and_fetch((int *)&ref->count, 1) == 0) {
        ref->free(ref);
    }
}
#endif  // ZENOH_C_STANDARD != 99

#else
static inline void ref_inc(const struct ref *ref) { ((struct ref *)ref)->count++; }

static inline void ref_dec(const struct ref *ref) {
    if (--((struct ref *)ref)->count == 0) {
        ref->free(ref);
    }
}
#endif  // Z_FEATURE_MULTI_THREAD == 1

#endif  // ZENOH_PICO_COLLECTIONS_REFCOUNT_H
