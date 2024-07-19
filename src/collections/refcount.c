//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/collections/refcount.h"

#include "zenoh-pico/utils/logging.h"

#define _Z_RC_MAX_COUNT INT32_MAX  // Based on Rust lazy overflow check

#if Z_FEATURE_MULTI_THREAD == 1
#if ZENOH_C_STANDARD != 99

#ifndef __cplusplus
#include <stdatomic.h>
#define _z_atomic(X) _Atomic X
#define _z_atomic_store_explicit atomic_store_explicit
#define _z_atomic_fetch_add_explicit atomic_fetch_add_explicit
#define _z_atomic_fetch_sub_explicit atomic_fetch_sub_explicit
#define _z_atomic_load_explicit atomic_load_explicit
#define _z_atomic_compare_exchange_strong atomic_compare_exchange_strong
#define _z_atomic_compare_exchange_weak_explicit atomic_compare_exchange_weak_explicit
#define _z_memory_order_acquire memory_order_acquire
#define _z_memory_order_release memory_order_release
#define _z_memory_order_relaxed memory_order_relaxed
#else
#include <atomic>
#define _z_atomic(X) std::atomic<X>
#define _z_atomic_store_explicit std::atomic_store_explicit
#define _z_atomic_fetch_add_explicit std::atomic_fetch_add_explicit
#define _z_atomic_fetch_sub_explicit std::atomic_fetch_sub_explicit
#define _z_atomic_load_explicit std::atomic_load_explicit
#define _z_atomic_compare_exchange_strong std::atomic_compare_exchange_strong
#define _z_atomic_compare_exchange_weak_explicit std::atomic_compare_exchange_weak_explicit
#define _z_memory_order_acquire std::memory_order_acquire
#define _z_memory_order_release std::memory_order_release
#define _z_memory_order_relaxed std::memory_order_relaxed
#endif  // __cplusplus

// c11 atomic variant
#define _ZP_RC_CNT_TYPE _z_atomic(unsigned int)
#define _ZP_RC_OP_INIT_CNT(p)                                                              \
    _z_atomic_store_explicit(&(p)->_strong_cnt, (unsigned int)1, _z_memory_order_relaxed); \
    _z_atomic_store_explicit(&(p)->_weak_cnt, (unsigned int)1, _z_memory_order_relaxed);
#define _ZP_RC_OP_INCR_STRONG_CNT(p) \
    _z_atomic_fetch_add_explicit(&(p)->_strong_cnt, (unsigned int)1, _z_memory_order_relaxed);
#define _ZP_RC_OP_INCR_AND_CMP_WEAK(p, x) \
    _z_atomic_fetch_add_explicit(&(p)->_weak_cnt, (unsigned int)1, _z_memory_order_relaxed) >= x
#define _ZP_RC_OP_DECR_AND_CMP_STRONG(p, x) \
    _z_atomic_fetch_sub_explicit(&(p)->_strong_cnt, (unsigned int)1, _z_memory_order_release) > (unsigned int)x
#define _ZP_RC_OP_DECR_AND_CMP_WEAK(p, x) \
    _z_atomic_fetch_sub_explicit(&(p)->_weak_cnt, (unsigned int)1, _z_memory_order_release) > (unsigned int)x
#define _ZP_RC_OP_CHECK_STRONG_CNT(p, x) _z_atomic_compare_exchange_strong(&(p)->_strong_cnt, &x, x)
#define _ZP_RC_OP_SYNC atomic_thread_fence(_z_memory_order_acquire);
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                                                                    \
    int8_t _upgrade(_z_inner_rc_t* cnt) {                                                                             \
        unsigned int prev = _z_atomic_load_explicit(&cnt->_strong_cnt, _z_memory_order_relaxed);                      \
        while ((prev != 0) && (prev < _Z_RC_MAX_COUNT)) {                                                             \
            if (_z_atomic_compare_exchange_weak_explicit(&cnt->_strong_cnt, &prev, prev + 1, _z_memory_order_acquire, \
                                                         _z_memory_order_relaxed)) {                                  \
                if (_ZP_RC_OP_INCR_AND_CMP_WEAK(cnt, _Z_RC_MAX_COUNT)) {                                              \
                    _Z_ERROR("Rc weak count overflow");                                                               \
                    return _Z_ERR_OVERFLOW;                                                                           \
                }                                                                                                     \
                return _Z_RES_OK;                                                                                     \
            }                                                                                                         \
        }                                                                                                             \
        return _Z_ERR_INVALID;                                                                                        \
    }

#else  // ZENOH_C_STANDARD == 99
#ifdef ZENOH_COMPILER_GCC

// c99 gcc sync builtin variant
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT(p)                                 \
    __sync_fetch_and_and(&(p)->_strong_cnt, (unsigned int)0); \
    __sync_fetch_and_add(&(p)->_strong_cnt, (unsigned int)1); \
    __sync_fetch_and_and(&(p)->_weak_cnt, (unsigned int)0);   \
    __sync_fetch_and_add(&(p)->_weak_cnt, (unsigned int)1);
#define _ZP_RC_OP_INCR_STRONG_CNT(p) __sync_fetch_and_add(&(p)->_strong_cnt, (unsigned int)1);
#define _ZP_RC_OP_INCR_AND_CMP_WEAK(p, x) __sync_fetch_and_add(&(p)->_weak_cnt, (unsigned int)1) >= x
#define _ZP_RC_OP_DECR_AND_CMP_STRONG(p, x) __sync_fetch_and_sub(&(p)->_strong_cnt, (unsigned int)1) > (unsigned int)x
#define _ZP_RC_OP_DECR_AND_CMP_WEAK(p, x) __sync_fetch_and_sub(&(p)->_weak_cnt, (unsigned int)1) > (unsigned int)x
#define _ZP_RC_OP_CHECK_STRONG_CNT(p, x) __sync_bool_compare_and_swap(&(p)->_strong_cnt, x, x)
#define _ZP_RC_OP_SYNC __sync_synchronize();
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                                    \
    int8_t _upgrade(_z_inner_rc_t* cnt) {                                             \
        unsigned int prev = __sync_fetch_and_add(&cnt->_strong_cnt, (unsigned int)0); \
        while ((prev != 0) && (prev < _Z_RC_MAX_COUNT)) {                             \
            if (__sync_bool_compare_and_swap(&cnt->_strong_cnt, prev, prev + 1)) {    \
                if (_ZP_RC_OP_INCR_AND_CMP_WEAK(cnt, _Z_RC_MAX_COUNT)) {              \
                    _Z_ERROR("Rc weak count overflow");                               \
                    return _Z_ERR_OVERFLOW;                                           \
                }                                                                     \
                                                                                      \
                return _Z_RES_OK;                                                     \
            } else {                                                                  \
                prev = __sync_fetch_and_add(&cnt->_strong_cnt, (unsigned int)0);      \
            }                                                                         \
        }                                                                             \
        return _Z_ERR_INVALID;                                                        \
    }

#else  // !ZENOH_COMPILER_GCC

// None variant
#error "Multi-thread refcount in C99 only exists for GCC, use GCC or C11 or deactivate multi-thread"
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT(p)
#define _ZP_RC_OP_INCR_STRONG_CNT(p)
#define _ZP_RC_OP_INCR_AND_CMP_WEAK(p, x) (x == 0)
#define _ZP_RC_OP_DECR_AND_CMP_STRONG(p, x) (x == 0)
#define _ZP_RC_OP_DECR_AND_CMP_WEAK(p, x) (x == 0)
#define _ZP_RC_OP_CHECK_STRONG_CNT(p, x) (x == 0) && (p != NULL)
#define _ZP_RC_OP_SYNC
#define _ZP_RC_OP_UPGRADE_CAS_LOOP        \
    int8_t _upgrade(_z_inner_rc_t* cnt) { \
        (void)cnt;                        \
        return _Z_ERR_INVALID;            \
    }

#endif  // ZENOH_COMPILER_GCC
#endif  // ZENOH_C_STANDARD != 99
#else   // Z_FEATURE_MULTI_THREAD == 0

// Single thread variant
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT(p)           \
    (p)->_strong_cnt = (unsigned int)1; \
    (p)->_weak_cnt = (unsigned int)1;
#define _ZP_RC_OP_INCR_STRONG_CNT(p) p->_strong_cnt++;
#define _ZP_RC_OP_INCR_AND_CMP_WEAK(p, x) p->_weak_cnt++ >= x
#define _ZP_RC_OP_DECR_AND_CMP_STRONG(p, x) p->_strong_cnt-- > (unsigned int)x
#define _ZP_RC_OP_DECR_AND_CMP_WEAK(p, x) p->_weak_cnt-- > (unsigned int)x
#define _ZP_RC_OP_CHECK_STRONG_CNT(p, x) (p->_strong_cnt == x)
#define _ZP_RC_OP_SYNC
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                             \
    int8_t _upgrade(_z_inner_rc_t* cnt) {                                      \
        if ((cnt->_strong_cnt != 0) && (cnt->_strong_cnt < _Z_RC_MAX_COUNT)) { \
            if (_ZP_RC_OP_INCR_AND_CMP_WEAK(cnt, _Z_RC_MAX_COUNT)) {           \
                _Z_ERROR("Rc weak count overflow");                            \
                return _Z_ERR_OVERFLOW;                                        \
            }                                                                  \
            _ZP_RC_OP_INCR_STRONG_CNT(cnt)                                     \
            return _Z_RES_OK;                                                  \
        }                                                                      \
        return _Z_ERR_OVERFLOW;                                                \
    }

#endif  // Z_FEATURE_MULTI_THREAD == 1

typedef struct {
    _ZP_RC_CNT_TYPE _strong_cnt;
    _ZP_RC_CNT_TYPE _weak_cnt;
} _z_inner_rc_t;

int8_t _z_rc_init(void** cnt) {
    *cnt = z_malloc(sizeof(_z_inner_rc_t));
    if ((*cnt) == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _ZP_RC_OP_INIT_CNT((_z_inner_rc_t*)*cnt)
    return _Z_RES_OK;
}

int8_t _z_rc_increase_strong(void* cnt) {
    _z_inner_rc_t* c = (_z_inner_rc_t*)cnt;
    if (_ZP_RC_OP_INCR_AND_CMP_WEAK(c, _Z_RC_MAX_COUNT)) {
        _Z_ERROR("Rc weak count overflow");
        return _Z_ERR_OVERFLOW;
    }
    _ZP_RC_OP_INCR_STRONG_CNT(c);
    return _Z_RES_OK;
}

int8_t _z_rc_increase_weak(void* cnt) {
    _z_inner_rc_t* c = (_z_inner_rc_t*)cnt;
    if (_ZP_RC_OP_INCR_AND_CMP_WEAK(c, _Z_RC_MAX_COUNT)) {
        _Z_ERROR("Rc weak count overflow");
        return _Z_ERR_OVERFLOW;
    }
    return _Z_RES_OK;
}

_Bool _z_rc_decrease_strong(void** cnt) {
    _z_inner_rc_t* c = (_z_inner_rc_t*)*cnt;
    if (_ZP_RC_OP_DECR_AND_CMP_STRONG(c, 1)) {
        return _z_rc_decrease_weak(cnt);
    }
    return _z_rc_decrease_weak(cnt);
}

_Bool _z_rc_decrease_weak(void** cnt) {
    _z_inner_rc_t* c = (_z_inner_rc_t*)*cnt;
    if (_ZP_RC_OP_DECR_AND_CMP_WEAK(c, 1)) {
        return false;
    }
    _ZP_RC_OP_SYNC
    z_free(*cnt);
    *cnt = NULL;
    return true;
}

_ZP_RC_OP_UPGRADE_CAS_LOOP

int8_t _z_rc_weak_upgrade(void* cnt) { return _upgrade((_z_inner_rc_t*)cnt); }

size_t _z_rc_weak_count(void* cnt) { return ((_z_inner_rc_t*)cnt)->_weak_cnt; }

size_t _z_rc_strong_count(void* cnt) { return ((_z_inner_rc_t*)cnt)->_strong_cnt; }
