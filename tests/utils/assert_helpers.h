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

#ifndef ZP_ASSERT_HELPERS_H
#define ZP_ASSERT_HELPERS_H

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "zenoh-pico/utils/result.h"

#ifdef NDEBUG
#include <stdlib.h>
#endif

// Fallbacks for older libcs
#ifndef PRIu32
#define PRIu32 "u"
#endif
#ifndef PRIu64
#define PRIu64 "llu"
#endif
#ifndef PRIi32
#define PRIi32 "d"
#endif

// Trap that fails in both debug and release builds
#ifndef NDEBUG
#define _ZP_TEST_TRAP() (assert(0 && "test assertion failed"))
#else
#define _ZP_TEST_TRAP() (abort())
#endif

// ---------- Assertions ----------

#define ASSERT_TRUE(expr)                                                                         \
    do {                                                                                          \
        bool _zp_v_ = (expr);                                                                     \
        if (!_zp_v_) {                                                                            \
            fprintf(stderr, "[FAIL] %s:%d: expected %s to be true\n", __FILE__, __LINE__, #expr); \
            fflush(stderr);                                                                       \
            _ZP_TEST_TRAP();                                                                      \
        }                                                                                         \
    } while (0)

#define ASSERT_FALSE(expr)                                                                         \
    do {                                                                                           \
        bool _zp_v_ = (expr);                                                                      \
        if (_zp_v_) {                                                                              \
            fprintf(stderr, "[FAIL] %s:%d: expected %s to be false\n", __FILE__, __LINE__, #expr); \
            fflush(stderr);                                                                        \
            _ZP_TEST_TRAP();                                                                       \
        }                                                                                          \
    } while (0)

#define ASSERT_EQ_U32(a, b)                                                                                         \
    do {                                                                                                            \
        uint32_t _zp_a_ = (uint32_t)(a);                                                                            \
        uint32_t _zp_b_ = (uint32_t)(b);                                                                            \
        if (_zp_a_ != _zp_b_) {                                                                                     \
            fprintf(stderr, "[FAIL] %s:%d: %s (%" PRIu32 ") != %s (%" PRIu32 ")\n", __FILE__, __LINE__, #a, _zp_a_, \
                    #b, _zp_b_);                                                                                    \
            fflush(stderr);                                                                                         \
            _ZP_TEST_TRAP();                                                                                        \
        }                                                                                                           \
    } while (0)

#define ASSERT_EQ_U64(a, b)                                                                                         \
    do {                                                                                                            \
        uint64_t _zp_a_ = (uint64_t)(a);                                                                            \
        uint64_t _zp_b_ = (uint64_t)(b);                                                                            \
        if (_zp_a_ != _zp_b_) {                                                                                     \
            fprintf(stderr, "[FAIL] %s:%d: %s (%" PRIu64 ") != %s (%" PRIu64 ")\n", __FILE__, __LINE__, #a, _zp_a_, \
                    #b, _zp_b_);                                                                                    \
            fflush(stderr);                                                                                         \
            _ZP_TEST_TRAP();                                                                                        \
        }                                                                                                           \
    } while (0)

#define ASSERT_EQ_I32(a, b)                                                                                         \
    do {                                                                                                            \
        int32_t _zp_a_ = (int32_t)(a);                                                                              \
        int32_t _zp_b_ = (int32_t)(b);                                                                              \
        if (_zp_a_ != _zp_b_) {                                                                                     \
            fprintf(stderr, "[FAIL] %s:%d: %s (%" PRIi32 ") != %s (%" PRIi32 ")\n", __FILE__, __LINE__, #a, _zp_a_, \
                    #b, _zp_b_);                                                                                    \
            fflush(stderr);                                                                                         \
            _ZP_TEST_TRAP();                                                                                        \
        }                                                                                                           \
    } while (0)

#define ASSERT_EQ_PTR(a, b)                                                                                    \
    do {                                                                                                       \
        const void *_zp_a_ = (const void *)(a);                                                                \
        const void *_zp_b_ = (const void *)(b);                                                                \
        if (_zp_a_ != _zp_b_) {                                                                                \
            fprintf(stderr, "[FAIL] %s:%d: %s (%p) != %s (%p)\n", __FILE__, __LINE__, #a, _zp_a_, #b, _zp_b_); \
            fflush(stderr);                                                                                    \
            _ZP_TEST_TRAP();                                                                                   \
        }                                                                                                      \
    } while (0)

#define ASSERT_OK(expr)                                                                                  \
    do {                                                                                                 \
        z_result_t _zp_res_ = (expr);                                                                    \
        if (_zp_res_ != _Z_RES_OK) {                                                                     \
            fprintf(stderr, "[FAIL] %s:%d: %s returned %d\n", __FILE__, __LINE__, #expr, (int)_zp_res_); \
            fflush(stderr);                                                                              \
            _ZP_TEST_TRAP();                                                                             \
        }                                                                                                \
    } while (0)

// Expect any error (i.e., not _Z_RES_OK)
#define ASSERT_NOT_OK(expr)                                                                                \
    do {                                                                                                   \
        z_result_t _zp_res_ = (expr);                                                                      \
        if (_zp_res_ == _Z_RES_OK) {                                                                       \
            fprintf(stderr, "[FAIL] %s:%d: %s returned OK (expected error)\n", __FILE__, __LINE__, #expr); \
            fflush(stderr);                                                                                \
            _ZP_TEST_TRAP();                                                                               \
        }                                                                                                  \
    } while (0)

// Expect a specific error/result code
#define ASSERT_ERR(expr, expected)                                                                                    \
    do {                                                                                                              \
        z_result_t _zp_res_ = (expr);                                                                                 \
        z_result_t _zp_exp_ = (expected);                                                                             \
        if (_zp_res_ != _zp_exp_) {                                                                                   \
            fprintf(stderr, "[FAIL] %s:%d: %s returned %d (expected %d)\n", __FILE__, __LINE__, #expr, (int)_zp_res_, \
                    (int)_zp_exp_);                                                                                   \
            fflush(stderr);                                                                                           \
            _ZP_TEST_TRAP();                                                                                          \
        }                                                                                                             \
    } while (0)

#endif  // ZP_ASSERT_HELPERS_H
