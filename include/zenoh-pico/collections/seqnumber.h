//
// Copyright (c) 2025 ZettaScale Technology
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

#ifndef ZENOH_PICO_COLLECTIONS_SEQNUMBER_H
#define ZENOH_PICO_COLLECTIONS_SEQNUMBER_H

#include <stdint.h>

#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_MULTI_THREAD == 1 && ZENOH_C_STANDARD != 99
#ifndef __cplusplus
#include <stdatomic.h>
#define _Z_SEQNUMBER_TYPE _Atomic uint32_t
#else
#include <atomic>
#define _Z_SEQNUMBER_TYPE std::atomic<uint32_t>
#endif
#else
#define _Z_SEQNUMBER_TYPE uint32_t
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    _Z_SEQNUMBER_TYPE _seq;
#if Z_FEATURE_MULTI_THREAD == 1 && ZENOH_C_STANDARD == 99 && !defined(ZENOH_COMPILER_GCC)
    _z_mutex_t _mutex;
#endif
} _z_seqnumber_t;

z_result_t _z_seqnumber_init(_z_seqnumber_t *seq);
z_result_t _z_seqnumber_fetch(_z_seqnumber_t *seq, uint32_t *value);
z_result_t _z_seqnumber_fetch_and_increment(_z_seqnumber_t *seq, uint32_t *value);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_COLLECTIONS_SEQNUMBER_H */
