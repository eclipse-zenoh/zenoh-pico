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

#include <string.h>

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

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
#define _ZP_RC_OP_INIT_CNT(cnt) _z_atomic_store_explicit(&(cnt), (unsigned int)1, _z_memory_order_relaxed);
#define _ZP_RC_OP_INCR_CNT(cnt) _z_atomic_fetch_add_explicit(&(cnt), (unsigned int)1, _z_memory_order_relaxed);
#define _ZP_RC_OP_INCR_AND_CMP_CNT(cnt, x) \
    _z_atomic_fetch_add_explicit(&(cnt), (unsigned int)1, _z_memory_order_relaxed) >= x
#define _ZP_RC_OP_DECR_AND_CMP_CNT(cnt, x) \
    _z_atomic_fetch_sub_explicit(&(cnt), (unsigned int)1, _z_memory_order_release) > (unsigned int)x
#define _ZP_RC_OP_SYNC atomic_thread_fence(_z_memory_order_acquire);
#define _ZP_RC_ATOMIC_LOAD_RELAXED(cnt) _z_atomic_load_explicit(&(cnt), _z_memory_order_relaxed)
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                                                                    \
    z_result_t _upgrade(_z_inner_rc_t* cnt) {                                                                         \
        if (cnt == NULL) {                                                                                            \
            _Z_ERROR_RETURN(_Z_ERR_INVALID);                                                                          \
        }                                                                                                             \
        unsigned int prev = _z_atomic_load_explicit(&cnt->_strong_cnt, _z_memory_order_relaxed);                      \
        while ((prev != 0) && (prev < _Z_RC_MAX_COUNT)) {                                                             \
            if (_z_atomic_compare_exchange_weak_explicit(&cnt->_strong_cnt, &prev, prev + 1, _z_memory_order_acquire, \
                                                         _z_memory_order_relaxed)) {                                  \
                return _Z_RES_OK;                                                                                     \
            }                                                                                                         \
        }                                                                                                             \
        _Z_ERROR_RETURN(_Z_ERR_INVALID);                                                                              \
    }

#else  // ZENOH_C_STANDARD == 99
#ifdef ZENOH_COMPILER_GCC

// c99 gcc sync builtin variant
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT(cnt)                    \
    __sync_fetch_and_and(&(cnt), (unsigned int)0); \
    __sync_fetch_and_add(&(cnt), (unsigned int)1);
#define _ZP_RC_OP_INCR_CNT(cnt) __sync_fetch_and_add(&(cnt), (unsigned int)1);
#define _ZP_RC_OP_INCR_AND_CMP_CNT(cnt, x) __sync_fetch_and_add(&(cnt), (unsigned int)1) >= x
#define _ZP_RC_OP_DECR_AND_CMP_CNT(cnt, x) __sync_fetch_and_sub(&(cnt), (unsigned int)1) > (unsigned int)x
#define _ZP_RC_OP_SYNC __sync_synchronize();
#define _ZP_RC_ATOMIC_LOAD_RELAXED(cnt) __atomic_load_n(&cnt, __ATOMIC_RELAXED)
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                                    \
    z_result_t _upgrade(_z_inner_rc_t* cnt) {                                         \
        if (cnt == NULL) {                                                            \
            _Z_ERROR_RETURN(_Z_ERR_INVALID);                                          \
        }                                                                             \
        unsigned int prev = __sync_fetch_and_add(&cnt->_strong_cnt, (unsigned int)0); \
        while ((prev != 0) && (prev < _Z_RC_MAX_COUNT)) {                             \
            if (__sync_bool_compare_and_swap(&cnt->_strong_cnt, prev, prev + 1)) {    \
                return _Z_RES_OK;                                                     \
            } else {                                                                  \
                prev = __sync_fetch_and_add(&cnt->_strong_cnt, (unsigned int)0);      \
            }                                                                         \
        }                                                                             \
        _Z_ERROR_RETURN(_Z_ERR_INVALID);                                              \
    }

#else  // !ZENOH_COMPILER_GCC

// None variant
#error "Multi-thread refcount in C99 only exists for GCC, use GCC or C11 or deactivate multi-thread"
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT(cnt)
#define _ZP_RC_OP_INCR_CNT(cnt)
#define _ZP_RC_OP_INCR_AND_CMP_CNT(cnt, x) (x == 0)
#define _ZP_RC_OP_DECR_AND_CMP_CNT(cnt, x) (x == 0)
#define _ZP_RC_OP_SYNC
#define _ZP_RC_ATOMIC_LOAD_RELAXED(cnt) (cnt)
#define _ZP_RC_OP_UPGRADE_CAS_LOOP            \
    z_result_t _upgrade(_z_inner_rc_t* cnt) { \
        (void)cnt;                            \
        _Z_ERROR_RETURN(_Z_ERR_INVALID);      \
    }

#endif  // ZENOH_COMPILER_GCC
#endif  // ZENOH_C_STANDARD != 99
#else   // Z_FEATURE_MULTI_THREAD == 0

// Single thread variant
#define _ZP_RC_CNT_TYPE unsigned int
#define _ZP_RC_OP_INIT_CNT(cnt) cnt = (unsigned int)1;
#define _ZP_RC_OP_INCR_CNT(cnt) (cnt)++;
#define _ZP_RC_OP_INCR_AND_CMP_CNT(cnt, x) cnt++ >= x
#define _ZP_RC_OP_DECR_AND_CMP_CNT(cnt, x) (cnt)-- > (unsigned int)x
#define _ZP_RC_OP_SYNC
#define _ZP_RC_ATOMIC_LOAD_RELAXED(cnt) (cnt)
#define _ZP_RC_OP_UPGRADE_CAS_LOOP                                             \
    z_result_t _upgrade(_z_inner_rc_t* cnt) {                                  \
        if ((cnt->_strong_cnt != 0) && (cnt->_strong_cnt < _Z_RC_MAX_COUNT)) { \
            _ZP_RC_OP_INCR_CNT(cnt->_strong_cnt)                               \
            return _Z_RES_OK;                                                  \
        }                                                                      \
        _Z_ERROR_RETURN(_Z_ERR_OVERFLOW);                                      \
    }

#endif  // Z_FEATURE_MULTI_THREAD == 1

typedef struct {
    _ZP_RC_CNT_TYPE _strong_cnt;
    _ZP_RC_CNT_TYPE _weak_cnt;
} _z_inner_rc_t;

static inline _z_inner_rc_t* _z_rc_inner(void* rc) { return (_z_inner_rc_t*)rc; }

z_result_t _z_rc_init(void** cnt) {
    *cnt = z_malloc(sizeof(_z_inner_rc_t));
    if ((*cnt) == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    _z_inner_rc_t* rc = *cnt;
    _ZP_RC_OP_INIT_CNT(rc->_strong_cnt);
    _ZP_RC_OP_INIT_CNT(
        rc->_weak_cnt);  // Note we increase weak count by 1 when creating a new rc, to take ownership of counter.
    return _Z_RES_OK;
}

z_result_t _z_rc_increase_strong(void* cnt) {
    _z_inner_rc_t* c = (_z_inner_rc_t*)cnt;
    if (_ZP_RC_OP_INCR_AND_CMP_CNT(c->_strong_cnt, _Z_RC_MAX_COUNT)) {
        _Z_ERROR("Rc strong count overflow");
        _Z_ERROR_RETURN(_Z_ERR_OVERFLOW);
    }
    return _Z_RES_OK;
}

z_result_t _z_rc_increase_weak(void* cnt) {
    _z_inner_rc_t* c = (_z_inner_rc_t*)cnt;
    if (c == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    if (_ZP_RC_OP_INCR_AND_CMP_CNT(c->_weak_cnt, _Z_RC_MAX_COUNT)) {
        _Z_ERROR("Rc weak count overflow");
        _Z_ERROR_RETURN(_Z_ERR_OVERFLOW);
    }
    return _Z_RES_OK;
}

bool _z_rc_decrease_strong(void** cnt) {
    _z_inner_rc_t* c = (_z_inner_rc_t*)*cnt;
    if (_ZP_RC_OP_DECR_AND_CMP_CNT(c->_strong_cnt, 1)) {
        return false;
    }
    // destroy fake weak that we created during strong init
    _z_rc_decrease_weak(cnt);
    return true;
}

bool _z_rc_decrease_weak(void** cnt) {
    _z_inner_rc_t* c = (_z_inner_rc_t*)*cnt;
    if (_ZP_RC_OP_DECR_AND_CMP_CNT(c->_weak_cnt, 1)) {
        return false;
    }
    _ZP_RC_OP_SYNC
    z_free(*cnt);
    *cnt = NULL;
    return true;
}

_ZP_RC_OP_UPGRADE_CAS_LOOP

z_result_t _z_rc_weak_upgrade(void* cnt) { return _upgrade((_z_inner_rc_t*)cnt); }

size_t _z_rc_weak_count(void* rc) {
    if (rc == NULL) {
        return 0;
    }
    size_t strong_count = _ZP_RC_ATOMIC_LOAD_RELAXED(_z_rc_inner(rc)->_strong_cnt);
    size_t weak_count = _ZP_RC_ATOMIC_LOAD_RELAXED(_z_rc_inner(rc)->_weak_cnt);
    if (weak_count == 0) {
        return 0;
    }
    return (strong_count > 0) ? weak_count - 1 : weak_count;  // substruct 1 weak ref that we added during strong init
}

size_t _z_rc_strong_count(void* rc) {
    return rc == NULL ? 0 : _ZP_RC_ATOMIC_LOAD_RELAXED(_z_rc_inner(rc)->_strong_cnt);
}

typedef struct {
    _ZP_RC_CNT_TYPE _strong_cnt;
} _z_inner_simple_rc_t;

#define RC_CNT_SIZE sizeof(_z_inner_simple_rc_t)

static inline _z_inner_simple_rc_t* _z_simple_rc_inner(void* rc) { return (_z_inner_simple_rc_t*)rc; }

void* _z_simple_rc_value(void* rc) { return (void*)_z_ptr_u8_offset((uint8_t*)rc, (ptrdiff_t)RC_CNT_SIZE); }

z_result_t _z_simple_rc_init(void** rc, const void* val, size_t val_size) {
    *rc = z_malloc(RC_CNT_SIZE + val_size);
    if ((*rc) == NULL) {
        _Z_ERROR("Failed to allocate rc");
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    _z_inner_simple_rc_t* inner = _z_simple_rc_inner(*rc);
    _ZP_RC_OP_INIT_CNT(inner->_strong_cnt);
    memcpy(_z_simple_rc_value(*rc), val, val_size);
    return _Z_RES_OK;
}

z_result_t _z_simple_rc_increase(void* rc) {
    _z_inner_simple_rc_t* c = _z_simple_rc_inner(rc);
    if (_ZP_RC_OP_INCR_AND_CMP_CNT(c->_strong_cnt, _Z_RC_MAX_COUNT)) {
        _Z_ERROR("Rc strong count overflow");
        _Z_ERROR_RETURN(_Z_ERR_OVERFLOW);
    }
    return _Z_RES_OK;
}

bool _z_simple_rc_decrease(void* rc) {
    _z_inner_simple_rc_t* c = _z_simple_rc_inner(rc);
    if (_ZP_RC_OP_DECR_AND_CMP_CNT(c->_strong_cnt, 1)) {
        return false;
    }
    return true;
}

size_t _z_simple_rc_strong_count(void* rc) {
    return rc == NULL ? 0 : _ZP_RC_ATOMIC_LOAD_RELAXED(_z_simple_rc_inner(rc)->_strong_cnt);
}
