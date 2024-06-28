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
#ifndef INCLUDE_ZENOH_PICO_API_OLV_MACROS_H
#define INCLUDE_ZENOH_PICO_API_OLV_MACROS_H

// Owned/Loaned/View type macros
//
// !!! FOR INTERNAL USAGE ONLY !!!

// For pointer types
#define _Z_OWNED_TYPE_PTR(type, name) \
    typedef struct {                  \
        type *_val;                   \
    } z_owned_##name##_t;

// For refcounted types
#define _Z_OWNED_TYPE_RC(type, name) \
    typedef struct {                 \
        type _rc;                    \
    } z_owned_##name##_t;

#define _Z_LOANED_TYPE(type, name) typedef type z_loaned_##name##_t;

#define _Z_VIEW_TYPE(type, name) \
    typedef struct {             \
        type _val;               \
    } z_view_##name##_t;

#define _Z_OWNED_FUNCTIONS_DEF(loanedtype, ownedtype, name)         \
    _Bool z_##name##_check(const ownedtype *obj);                   \
    const loanedtype *z_##name##_loan(const ownedtype *obj);        \
    loanedtype *z_##name##_loan_mut(ownedtype *obj);                \
    ownedtype *z_##name##_move(ownedtype *obj);                     \
    int8_t z_##name##_clone(ownedtype *obj, const loanedtype *src); \
    void z_##name##_drop(ownedtype *obj);                           \
    void z_##name##_null(ownedtype *obj);

#define _Z_VIEW_FUNCTIONS_DEF(loanedtype, viewtype, name)         \
    const loanedtype *z_view_##name##_loan(const viewtype *name); \
    loanedtype *z_view_##name##_loan_mut(viewtype *name);         \
    void z_view_##name##_null(viewtype *name);

#define _Z_OWNED_FUNCTIONS_PTR_IMPL(type, name, f_copy, f_free)                                     \
    _Bool z_##name##_check(const z_owned_##name##_t *obj) { return obj->_val != NULL; }             \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj) { return obj->_val; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj) { return obj->_val; }         \
    void z_##name##_null(z_owned_##name##_t *obj) { obj->_val = NULL; }                             \
    z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *obj) { return obj; }                    \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {              \
        int8_t ret = _Z_RES_OK;                                                                     \
        obj->_val = (type *)z_malloc(sizeof(type));                                                 \
        if (obj->_val != NULL) {                                                                    \
            f_copy(obj->_val, src);                                                                 \
        } else {                                                                                    \
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                      \
        }                                                                                           \
        return ret;                                                                                 \
    }                                                                                               \
    void z_##name##_drop(z_owned_##name##_t *obj) {                                                 \
        if ((obj != NULL) && (obj->_val != NULL)) {                                                 \
            f_free(&obj->_val);                                                                     \
        }                                                                                           \
    }

#define _Z_OWNED_FUNCTIONS_PTR_TRIVIAL_IMPL(type, name)                                             \
    _Bool z_##name##_check(const z_owned_##name##_t *obj) { return obj->_val != NULL; }             \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj) { return obj->_val; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj) { return obj->_val; }         \
    void z_##name##_null(z_owned_##name##_t *obj) { obj->_val = NULL; }                             \
    z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *obj) { return obj; }                    \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {              \
        int8_t ret = _Z_RES_OK;                                                                     \
        obj->_val = (type *)z_malloc(sizeof(type));                                                 \
        if (obj->_val != NULL) {                                                                    \
            *obj->_val = *src;                                                                      \
        } else {                                                                                    \
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                      \
        }                                                                                           \
        return ret;                                                                                 \
    }                                                                                               \
    void z_##name##_drop(z_owned_##name##_t *obj) {                                                 \
        if ((obj != NULL) && (obj->_val != NULL)) {                                                 \
            z_free(obj->_val);                                                                      \
            obj->_val = NULL;                                                                       \
        }                                                                                           \
    }

#define _Z_OWNED_FUNCTIONS_RC_IMPL(name)                                                            \
    _Bool z_##name##_check(const z_owned_##name##_t *val) { return val->_rc.in != NULL; }           \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *val) { return &val->_rc; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *val) { return &val->_rc; }         \
    void z_##name##_null(z_owned_##name##_t *val) { val->_rc.in = NULL; }                           \
    z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *val) { return val; }                    \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {              \
        int8_t ret = _Z_RES_OK;                                                                     \
        obj->_rc = _z_##name##_rc_clone((z_loaned_##name##_t *)src);                                \
        if (obj->_rc.in == NULL) {                                                                  \
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                      \
        }                                                                                           \
        return ret;                                                                                 \
    }                                                                                               \
    void z_##name##_drop(z_owned_##name##_t *val) {                                                 \
        if (val->_rc.in != NULL) {                                                                  \
            if (_z_##name##_rc_drop(&val->_rc)) {                                                   \
                val->_rc.in = NULL;                                                                 \
            }                                                                                       \
        }                                                                                           \
    }

#define _Z_VIEW_FUNCTIONS_PTR_IMPL(type, name)                                                           \
    const z_loaned_##name##_t *z_view_##name##_loan(const z_view_##name##_t *obj) { return &obj->_val; } \
    z_loaned_##name##_t *z_view_##name##_loan_mut(z_view_##name##_t *obj) { return &obj->_val; }

#define _Z_OWNED_FUNCTIONS_CLOSURE_DEF(ownedtype, name) \
    _Bool z_##name##_check(const ownedtype *val);       \
    ownedtype *z_##name##_move(ownedtype *val);         \
    void z_##name##_drop(ownedtype *val);               \
    void z_##name##_null(ownedtype *name);

#define _Z_OWNED_FUNCTIONS_CLOSURE_IMPL(ownedtype, name, f_call, f_drop)           \
    _Bool z_##name##_check(const ownedtype *val) { return val->call != NULL; }     \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }                     \
    void z_##name##_drop(ownedtype *val) {                                         \
        if (val->drop != NULL) {                                                   \
            (val->drop)(val->context);                                             \
            val->drop = NULL;                                                      \
        }                                                                          \
        val->call = NULL;                                                          \
        val->context = NULL;                                                       \
    }                                                                              \
    void z_##name##_null(ownedtype *val) {                                         \
        val->call = NULL;                                                          \
        val->drop = NULL;                                                          \
        val->context = NULL;                                                       \
    }                                                                              \
    int8_t z_##name(ownedtype *closure, f_call call, f_drop drop, void *context) { \
        closure->call = call;                                                      \
        closure->drop = drop;                                                      \
        closure->context = context;                                                \
                                                                                   \
        return _Z_RES_OK;                                                          \
    }

// Gets internal value from refcounted type (e.g. z_loaned_session_t, z_query_t)
#define _Z_RC_IN_VAL(arg) ((arg)->in->val)

// Checks if refcounted type is initialized
#define _Z_RC_IS_NULL(arg) ((arg)->in == NULL)

// Gets internal value from refcounted owned type (e.g. z_owned_session_t, z_owned_query_t)
#define _Z_OWNED_RC_IN_VAL(arg) ((arg)->_rc.in->val)

#endif /* INCLUDE_ZENOH_PICO_API_OLV_MACROS_H */
