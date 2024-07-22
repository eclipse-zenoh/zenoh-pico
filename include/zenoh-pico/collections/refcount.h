//
// Copyright (c) 2022 ZettaScale Technology
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
#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/utils/result.h"

#ifdef __cplusplus
extern "C" {
#endif

int8_t _z_rc_init(void **cnt);
int8_t _z_rc_increase_strong(void *cnt);
int8_t _z_rc_increase_weak(void *cnt);
_Bool _z_rc_decrease_strong(void **cnt);
_Bool _z_rc_decrease_weak(void **cnt);
int8_t _z_rc_weak_upgrade(void *cnt);

size_t _z_rc_weak_count(void *cnt);
size_t _z_rc_strong_count(void *cnt);

/*------------------ Internal Array Macros ------------------*/
#define _Z_REFCOUNT_DEFINE(name, type)                                                                               \
    typedef struct name##_rc_t {                                                                                     \
        type##_t *_val;                                                                                              \
        void *_cnt;                                                                                                  \
    } name##_rc_t;                                                                                                   \
                                                                                                                     \
    typedef struct name##_weak_t {                                                                                   \
        type##_t *_val;                                                                                              \
        void *_cnt;                                                                                                  \
    } name##_weak_t;                                                                                                 \
                                                                                                                     \
    static inline name##_rc_t name##_rc_null(void) {                                                                 \
        name##_rc_t p;                                                                                               \
        p._val = NULL;                                                                                               \
        p._cnt = NULL;                                                                                               \
        return p;                                                                                                    \
    }                                                                                                                \
    static inline name##_weak_t name##_weak_null(void) {                                                             \
        name##_weak_t p;                                                                                             \
        p._val = NULL;                                                                                               \
        p._cnt = NULL;                                                                                               \
        return p;                                                                                                    \
    }                                                                                                                \
                                                                                                                     \
    static inline name##_rc_t name##_rc_new(type##_t *val) {                                                         \
        name##_rc_t p = name##_rc_null();                                                                            \
        if (_z_rc_init(&p._cnt) == _Z_RES_OK) {                                                                      \
            p._val = val;                                                                                            \
        }                                                                                                            \
        return p;                                                                                                    \
    }                                                                                                                \
    static inline name##_rc_t name##_rc_new_from_val(const type##_t *val) {                                          \
        type##_t *v = (type##_t *)z_malloc(sizeof(type##_t));                                                        \
        if (v == NULL) {                                                                                             \
            return name##_rc_null();                                                                                 \
        }                                                                                                            \
        *v = *val;                                                                                                   \
        name##_rc_t p = name##_rc_new(v);                                                                            \
        if (p._cnt == NULL) {                                                                                        \
            z_free(v);                                                                                               \
            return name##_rc_null();                                                                                 \
        }                                                                                                            \
        return p;                                                                                                    \
    }                                                                                                                \
    static inline name##_rc_t name##_rc_clone(const name##_rc_t *p) {                                                \
        name##_rc_t c = name##_rc_null();                                                                            \
        if (_z_rc_increase_strong(p->_cnt) == _Z_RES_OK) {                                                           \
            c = *p;                                                                                                  \
        }                                                                                                            \
        return c;                                                                                                    \
    }                                                                                                                \
    static inline name##_rc_t *name##_rc_clone_as_ptr(const name##_rc_t *p) {                                        \
        name##_rc_t *c = (name##_rc_t *)z_malloc(sizeof(name##_rc_t));                                               \
        if (c != NULL) {                                                                                             \
            *c = name##_rc_clone(p);                                                                                 \
            if (c->_cnt == NULL) {                                                                                   \
                z_free(c);                                                                                           \
            }                                                                                                        \
        }                                                                                                            \
        return c;                                                                                                    \
    }                                                                                                                \
    static inline name##_weak_t name##_rc_clone_as_weak(const name##_rc_t *p) {                                      \
        name##_weak_t c = name##_weak_null();                                                                        \
        if (_z_rc_increase_weak(p->_cnt) == _Z_RES_OK) {                                                             \
            c._val = p->_val;                                                                                        \
            c._cnt = p->_cnt;                                                                                        \
        }                                                                                                            \
        return c;                                                                                                    \
    }                                                                                                                \
    static inline name##_weak_t *name##_rc_clone_as_weak_ptr(const name##_rc_t *p) {                                 \
        name##_weak_t *c = (name##_weak_t *)z_malloc(sizeof(name##_weak_t));                                         \
        if (c != NULL) {                                                                                             \
            *c = name##_rc_clone_as_weak(p);                                                                         \
            if (c->_cnt == NULL) {                                                                                   \
                z_free(c);                                                                                           \
            }                                                                                                        \
        }                                                                                                            \
        return c;                                                                                                    \
    }                                                                                                                \
    static inline void name##_rc_copy(name##_rc_t *dst, const name##_rc_t *p) { *dst = name##_rc_clone(p); }         \
    static inline _Bool name##_rc_eq(const name##_rc_t *left, const name##_rc_t *right) {                            \
        return (left->_val == right->_val);                                                                          \
    }                                                                                                                \
    static inline _Bool name##_rc_decr(name##_rc_t *p) {                                                             \
        if ((p == NULL) || (p->_cnt == NULL)) {                                                                      \
            return false;                                                                                            \
        }                                                                                                            \
        if (_z_rc_decrease_strong(&p->_cnt)) {                                                                       \
            return true;                                                                                             \
        }                                                                                                            \
        return false;                                                                                                \
    }                                                                                                                \
    static inline _Bool name##_rc_drop(name##_rc_t *p) {                                                             \
        if (name##_rc_decr(p) && p->_val != NULL) {                                                                  \
            type##_clear(p->_val);                                                                                   \
            z_free(p->_val);                                                                                         \
            return true;                                                                                             \
        }                                                                                                            \
        return false;                                                                                                \
    }                                                                                                                \
    static inline name##_weak_t name##_weak_clone(const name##_weak_t *p) {                                          \
        name##_weak_t c = name##_weak_null();                                                                        \
        if (_z_rc_increase_weak(p->_cnt) == _Z_RES_OK) {                                                             \
            c = *p;                                                                                                  \
        }                                                                                                            \
        return c;                                                                                                    \
    }                                                                                                                \
    static inline void name##_weak_copy(name##_weak_t *dst, const name##_weak_t *p) { *dst = name##_weak_clone(p); } \
    static inline name##_rc_t name##_weak_upgrade(name##_weak_t *p) {                                                \
        name##_rc_t c = name##_rc_null();                                                                            \
        if (_z_rc_weak_upgrade(p->_cnt) == _Z_RES_OK) {                                                              \
            c._val = p->_val;                                                                                        \
            c._cnt = p->_cnt;                                                                                        \
        }                                                                                                            \
        return c;                                                                                                    \
    }                                                                                                                \
    static inline _Bool name##_weak_eq(const name##_weak_t *left, const name##_weak_t *right) {                      \
        return (left->_val == right->_val);                                                                          \
    }                                                                                                                \
    static inline _Bool name##_weak_drop(name##_weak_t *p) {                                                         \
        if ((p == NULL) || (p->_cnt == NULL)) {                                                                      \
            return false;                                                                                            \
        }                                                                                                            \
        if (_z_rc_decrease_weak(&p->_cnt)) {                                                                         \
            return true;                                                                                             \
        }                                                                                                            \
        return false;                                                                                                \
    }                                                                                                                \
    static inline size_t name##_rc_size(name##_rc_t *p) {                                                            \
        _ZP_UNUSED(p);                                                                                               \
        return sizeof(name##_rc_t);                                                                                  \
    }

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_COLLECTIONS_REFCOUNT_H */
