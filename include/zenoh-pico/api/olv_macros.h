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

// Gets internal value from refcounted type (e.g. z_loaned_session_t, z_query_t)
#define _Z_RC_IN_VAL(arg) ((arg)->_val)

// Checks if refcounted type is initialized
#define _Z_RC_IS_NULL(arg) ((arg)->_cnt == NULL)

// Gets internal value from refcounted owned type (e.g. z_owned_session_t, z_owned_query_t)
#define _Z_OWNED_RC_IN_VAL(arg) ((arg)->_rc._val)

// Owned/Loaned/View type macros
//
// !!! FOR INTERNAL USAGE ONLY !!!

#define _Z_MOVED_TYPE(name)       \
    typedef struct {              \
        z_owned_##name##_t _this; \
    } z_moved_##name##_t;

#define _Z_LOANED_TYPE(type, name) typedef type z_loaned_##name##_t;

// For value types
#define _Z_OWNED_TYPE_VALUE(type, name) \
    typedef struct {                    \
        type _val;                      \
    } z_owned_##name##_t;               \
    _Z_LOANED_TYPE(type, name)          \
    _Z_MOVED_TYPE(name)

// For refcounted types
#define _Z_OWNED_TYPE_RC(type, name) \
    typedef struct {                 \
        type _rc;                    \
    } z_owned_##name##_t;            \
    _Z_LOANED_TYPE(type, name)       \
    _Z_MOVED_TYPE(name)

#define _Z_VIEW_TYPE(type, name) \
    typedef struct {             \
        type _val;               \
    } z_view_##name##_t;

#define _Z_OWNED_FUNCTIONS_DEF(name)                                                  \
    void z_internal_##name##_null(z_owned_##name##_t *obj);                           \
    _Bool z_internal_##name##_check(const z_owned_##name##_t *obj);                   \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj);        \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj);                \
    z_moved_##name##_t *z_##name##_move(z_owned_##name##_t *obj);                     \
    void z_##name##_take(z_owned_##name##_t *obj, z_moved_##name##_t *src);           \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src); \
    void z_##name##_drop(z_moved_##name##_t *obj);

#define _Z_OWNED_FUNCTIONS_NO_COPY_DEF(name)                                   \
    void z_internal_##name##_null(z_owned_##name##_t *obj);                    \
    _Bool z_internal_##name##_check(const z_owned_##name##_t *obj);            \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj); \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj);         \
    z_moved_##name##_t *z_##name##_move(z_owned_##name##_t *obj);              \
    void z_##name##_take(z_owned_##name##_t *obj, z_moved_##name##_t *src);    \
    void z_##name##_drop(z_moved_##name##_t *obj);

#define _Z_OWNED_FUNCTIONS_SYSTEM_DEF(name)                                    \
    void z_internal_##name##_null(z_owned_##name##_t *obj);                    \
    void z_##name##_take(z_owned_##name##_t *obj, z_moved_##name##_t *src);    \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj); \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj);         \
    z_moved_##name##_t *z_##name##_move(z_owned_##name##_t *obj);

#define _Z_VIEW_FUNCTIONS_DEF(name)                                                 \
    _Bool z_view_##name##_is_empty(const z_view_##name##_t *obj);                   \
    const z_loaned_##name##_t *z_view_##name##_loan(const z_view_##name##_t *name); \
    z_loaned_##name##_t *z_view_##name##_loan_mut(z_view_##name##_t *name);         \
    void _z_view_##name##_empty(z_view_##name##_t *name);

#define _Z_OWNED_FUNCTIONS_IMPL_MOVE_TAKE(name)                                                          \
    z_moved_##name##_t *z_##name##_move(z_owned_##name##_t *obj) { return (z_moved_##name##_t *)(obj); } \
    void z_##name##_take(z_owned_##name##_t *obj, z_moved_##name##_t *src) {                             \
        *obj = src->_this;                                                                               \
        z_internal_##name##_null(&src->_this);                                                           \
    }

#define _Z_OWNED_FUNCTIONS_PTR_IMPL(type, name, f_copy, f_free)                                     \
    _Z_OWNED_FUNCTIONS_IMPL_MOVE_TAKE(name)                                                         \
    void z_internal_##name##_null(z_owned_##name##_t *obj) { obj->_val = NULL; }                    \
    _Bool z_internal_##name##_check(const z_owned_##name##_t *obj) { return obj->_val != NULL; }    \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj) { return obj->_val; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj) { return obj->_val; }         \
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
    void z_##name##_drop(z_moved_##name##_t *obj) {                                                 \
        if ((obj != NULL) && (obj->_this._val != NULL)) {                                           \
            f_free(obj->_this._val);                                                                \
        }                                                                                           \
    }

#define _Z_OWNED_FUNCTIONS_VALUE_IMPL(type, name, f_check, f_null, f_copy, f_drop)                   \
    _Z_OWNED_FUNCTIONS_IMPL_MOVE_TAKE(name)                                                          \
    void z_internal_##name##_null(z_owned_##name##_t *obj) { obj->_val = f_null(); }                 \
    _Bool z_internal_##name##_check(const z_owned_##name##_t *obj) { return f_check((&obj->_val)); } \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj) { return &obj->_val; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj) { return &obj->_val; }         \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {               \
        return f_copy((&obj->_val), src);                                                            \
    }                                                                                                \
    void z_##name##_drop(z_moved_##name##_t *obj) {                                                  \
        if (obj != NULL) f_drop((&obj->_this._val));                                                 \
    }

#define _Z_OWNED_FUNCTIONS_VALUE_NO_COPY_IMPL_INNER(type, name, f_check, f_null, f_drop, attribute)                \
    attribute void z_internal_##name##_null(z_owned_##name##_t *obj) { obj->_val = f_null(); }                     \
    attribute _Bool z_internal_##name##_check(const z_owned_##name##_t *obj) { return f_check((&obj->_val)); }     \
    attribute const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj) { return &obj->_val; }     \
    attribute z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj) { return &obj->_val; }             \
    attribute z_moved_##name##_t *z_##name##_move(z_owned_##name##_t *obj) { return (z_moved_##name##_t *)(obj); } \
    attribute void z_##name##_take(z_owned_##name##_t *obj, z_moved_##name##_t *src) {                             \
        *obj = src->_this;                                                                                         \
        z_internal_##name##_null(&src->_this);                                                                     \
    }                                                                                                              \
    attribute void z_##name##_drop(z_moved_##name##_t *obj) {                                                      \
        if (obj != NULL) f_drop((&obj->_this._val));                                                               \
    }

#define _ZP_NOTHING

#define _Z_OWNED_FUNCTIONS_VALUE_NO_COPY_IMPL(type, name, f_check, f_null, f_drop) \
    _Z_OWNED_FUNCTIONS_VALUE_NO_COPY_IMPL_INNER(type, name, f_check, f_null, f_drop, _ZP_NOTHING)

#define _Z_OWNED_FUNCTIONS_VALUE_NO_COPY_INLINE_IMPL(type, name, f_check, f_null, f_drop) \
    _Z_OWNED_FUNCTIONS_VALUE_NO_COPY_IMPL_INNER(type, name, f_check, f_null, f_drop, static inline)

#define _Z_OWNED_FUNCTIONS_RC_IMPL(name)                                                                 \
    _Z_OWNED_FUNCTIONS_IMPL_MOVE_TAKE(name)                                                              \
    void z_internal_##name##_null(z_owned_##name##_t *val) { val->_rc = _z_##name##_rc_null(); }         \
    _Bool z_internal_##name##_check(const z_owned_##name##_t *val) { return !_Z_RC_IS_NULL(&val->_rc); } \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *val) { return &val->_rc; }      \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *val) { return &val->_rc; }              \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {                   \
        int8_t ret = _Z_RES_OK;                                                                          \
        obj->_rc = _z_##name##_rc_clone((z_loaned_##name##_t *)src);                                     \
        if (_Z_RC_IS_NULL(&obj->_rc)) {                                                                  \
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                           \
        }                                                                                                \
        return ret;                                                                                      \
    }                                                                                                    \
    void z_##name##_drop(z_moved_##name##_t *obj) {                                                      \
        if (!_Z_RC_IS_NULL(&obj->_this._rc)) {                                                           \
            _z_##name##_rc_drop(&obj->_this._rc);                                                        \
        }                                                                                                \
    }

#define _Z_OWNED_FUNCTIONS_SYSTEM_IMPL(type, name)                                                   \
    _Z_OWNED_FUNCTIONS_IMPL_MOVE_TAKE(name)                                                          \
    void z_internal_##name##_null(z_owned_##name##_t *obj) { (void)obj; }                            \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj) { return &obj->_val; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj) { return &obj->_val; }

#define _Z_VIEW_FUNCTIONS_IMPL(type, name, f_check, f_null)                                              \
    _Bool z_view_##name##_is_empty(const z_view_##name##_t *obj) { return !f_check((&obj->_val)); }      \
    const z_loaned_##name##_t *z_view_##name##_loan(const z_view_##name##_t *obj) { return &obj->_val; } \
    z_loaned_##name##_t *z_view_##name##_loan_mut(z_view_##name##_t *obj) { return &obj->_val; }         \
    void z_view_##name##_empty(z_view_##name##_t *obj) { obj->_val = f_null(); }

#define _Z_OWNED_FUNCTIONS_CLOSURE_DEF(name)                                \
    void z_internal_##name##_null(z_owned_##name##_t *name);                \
    _Bool z_internal_##name##_check(const z_owned_##name##_t *val);         \
    z_moved_##name##_t *z_##name##_move(z_owned_##name##_t *val);           \
    void z_##name##_take(z_owned_##name##_t *obj, z_moved_##name##_t *src); \
    void z_##name##_drop(z_moved_##name##_t *obj);                          \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *val);

#define _Z_OWNED_FUNCTIONS_CLOSURE_IMPL(name, f_call, f_drop)                                         \
    _Z_OWNED_FUNCTIONS_IMPL_MOVE_TAKE(name)                                                           \
    void z_internal_##name##_null(z_owned_##name##_t *val) {                                          \
        val->_val.call = NULL;                                                                        \
        val->_val.drop = NULL;                                                                        \
        val->_val.context = NULL;                                                                     \
    }                                                                                                 \
    _Bool z_internal_##name##_check(const z_owned_##name##_t *val) { return val->_val.call != NULL; } \
    void z_##name##_drop(z_moved_##name##_t *obj) {                                                   \
        if (obj->_this._val.drop != NULL) {                                                           \
            (obj->_this._val.drop)(obj->_this._val.context);                                          \
            obj->_this._val.drop = NULL;                                                              \
        }                                                                                             \
        obj->_this._val.call = NULL;                                                                  \
        obj->_this._val.context = NULL;                                                               \
    }                                                                                                 \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *val) { return &val->_val; }  \
    int8_t z_##name(z_owned_##name##_t *closure, f_call call, f_drop drop, void *context) {           \
        closure->_val.call = call;                                                                    \
        closure->_val.drop = drop;                                                                    \
        closure->_val.context = context;                                                              \
                                                                                                      \
        return _Z_RES_OK;                                                                             \
    }

#endif /* INCLUDE_ZENOH_PICO_API_OLV_MACROS_H */
