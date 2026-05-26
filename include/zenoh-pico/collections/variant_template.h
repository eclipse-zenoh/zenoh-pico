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

// Tagged-union variant holding one of two to four user-supplied types at a
// time, plus an empty/uninitialised state.  (Similar to std::variant<A,B[,C[,D]]>.)
//
// Design summary
// ──────────────
// • States: NONE (default / moved-from), A, B, and optionally C and/or D.
//   C requires A and B; D requires C (and therefore A and B).
// • Storage is an anonymous union; the active member is tracked by a tag field.
// • Move semantics: constructors take ownership of the pointed-to value via
//   user-supplied (or default copy+zero) move callbacks.
// • No heap allocation.
//
// User must define the following macros before including this file:
//
// Required:
//   _ZP_VARIANT_TEMPLATE_NAME
//       base name for all generated symbols (e.g. "my_result")
//   _ZP_VARIANT_TEMPLATE_1_TYPE
//       type of the first alternative
//   _ZP_VARIANT_TEMPLATE_2_TYPE
//       type of the second alternative
//
// Optional extra alternatives (C requires A+B; D requires C):
//   _ZP_VARIANT_TEMPLATE_3_TYPE   — enables a third alternative
//   _ZP_VARIANT_TEMPLATE_4_TYPE   — enables a fourth alternative (requires C)
//
// Optional naming (default: 1, 2, 3, 4):
//   _ZP_VARIANT_TEMPLATE_1_NAME   identifier suffix for alternative 1 symbols
//   _ZP_VARIANT_TEMPLATE_2_NAME   identifier suffix for alternative 2 symbols
//   _ZP_VARIANT_TEMPLATE_3_NAME   identifier suffix for alternative 3 symbols
//   _ZP_VARIANT_TEMPLATE_4_NAME   identifier suffix for alternative 4 symbols
//       E.g. 1_NAME=ok, 2_NAME=err produces NAME_from_ok / NAME_is_ok /
//       NAME_get_ok / NAME_take_ok and NAME_TAG_ok.
//
// Optional callbacks (default: no-op destroy, shallow-copy move):
//   _ZP_VARIANT_TEMPLATE_1_DESTROY_FN_NAME(ptr)
//   _ZP_VARIANT_TEMPLATE_2_DESTROY_FN_NAME(ptr)
//   _ZP_VARIANT_TEMPLATE_3_DESTROY_FN_NAME(ptr)
//   _ZP_VARIANT_TEMPLATE_4_DESTROY_FN_NAME(ptr)
//   _ZP_VARIANT_TEMPLATE_1_MOVE_FN_NAME(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_2_MOVE_FN_NAME(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_3_MOVE_FN_NAME(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_4_MOVE_FN_NAME(dst_ptr, src_ptr)
//
// Generated API  (NAME=_ZP_VARIANT_TEMPLATE_NAME, XN=X alternative name)
// ────────────────────────────────────────────────────────────────────────
// Types:
//   NAME_t        — the variant struct
//   NAME_tag_t    — tag enum { NAME_TAG_NONE=0, NAME_TAG_AN, NAME_TAG_BN[, ...] }
//
// Construction / destruction:
//   NAME_t  NAME_none(void)
//   NAME_t  NAME_from_AN(A_TYPE *val)   — moves *val into new variant
//   NAME_t  NAME_from_BN(B_TYPE *val)
//   NAME_t  NAME_from_CN(C_TYPE *val)   — only if C_TYPE defined
//   NAME_t  NAME_from_DN(D_TYPE *val)   — only if D_TYPE defined
//   void    NAME_destroy(NAME_t *v)
//
// Move (variant-to-variant):
//   void    NAME_move(NAME_t *dst, NAME_t *src)
//
// Inspection:
//   bool       NAME_is_none(const NAME_t *v)
//   bool       NAME_is_AN  (const NAME_t *v)
//   bool       NAME_is_BN  (const NAME_t *v)
//   bool       NAME_is_CN  (const NAME_t *v)   — only if C_TYPE defined
//   bool       NAME_is_DN  (const NAME_t *v)   — only if D_TYPE defined
//   NAME_tag_t NAME_tag    (const NAME_t *v)
//
// Non-consuming access (UB if wrong alternative):
//   A_TYPE *NAME_get_AN(NAME_t *v)
//   B_TYPE *NAME_get_BN(NAME_t *v)
//   C_TYPE *NAME_get_CN(NAME_t *v)   — only if C_TYPE defined
//   D_TYPE *NAME_get_DN(NAME_t *v)   — only if D_TYPE defined
//
// Consuming access (moves value out, returns false if wrong alternative):
//   bool    NAME_take_AN(NAME_t *v, A_TYPE *out)
//   bool    NAME_take_BN(NAME_t *v, B_TYPE *out)
//   bool    NAME_take_CN(NAME_t *v, C_TYPE *out)   — only if C_TYPE defined
//   bool    NAME_take_DN(NAME_t *v, D_TYPE *out)   — only if D_TYPE defined
//

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

// ── Validate D requires C ────────────────────────────────────────────────────

#if defined(_ZP_VARIANT_TEMPLATE_4_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_3_TYPE)
#error "variant_template.h: _ZP_VARIANT_TEMPLATE_4_TYPE requires _ZP_VARIANT_TEMPLATE_3_TYPE"
#endif

// ── Required macros ──────────────────────────────────────────────────────────

#ifndef _ZP_VARIANT_TEMPLATE_NAME
#error "_ZP_VARIANT_TEMPLATE_NAME must be defined before including variant_template.h"
#define _ZP_VARIANT_TEMPLATE_NAME _zp_variant
#endif
#ifndef _ZP_VARIANT_TEMPLATE_1_TYPE
#error "_ZP_VARIANT_TEMPLATE_1_TYPE must be defined before including variant_template.h"
#define _ZP_VARIANT_TEMPLATE_1_TYPE int
#endif
#ifndef _ZP_VARIANT_TEMPLATE_2_TYPE
#error "_ZP_VARIANT_TEMPLATE_2_TYPE must be defined before including variant_template.h"
#define _ZP_VARIANT_TEMPLATE_2_TYPE int
#endif

// ── Optional names with defaults ─────────────────────────────────────────────

#ifndef _ZP_VARIANT_TEMPLATE_1_NAME
#define _ZP_VARIANT_TEMPLATE_1_NAME 1
#endif
#ifndef _ZP_VARIANT_TEMPLATE_2_NAME
#define _ZP_VARIANT_TEMPLATE_2_NAME 2
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_3_NAME
#define _ZP_VARIANT_TEMPLATE_3_NAME 3
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_4_NAME
#define _ZP_VARIANT_TEMPLATE_4_NAME 4
#endif
#endif

// ── Optional callbacks with defaults ─────────────────────────────────────────

#ifndef _ZP_VARIANT_TEMPLATE_1_DESTROY_FN_NAME
#define _ZP_VARIANT_TEMPLATE_1_DESTROY_FN_NAME(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_2_DESTROY_FN_NAME
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN_NAME(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_1_MOVE_FN_NAME
#define _ZP_VARIANT_TEMPLATE_1_MOVE_FN_NAME(dst, src) \
    *(dst) = *(src);                                  \
    _ZP_VARIANT_TEMPLATE_1_DESTROY_FN_NAME(src);
#endif
#ifndef _ZP_VARIANT_TEMPLATE_2_MOVE_FN_NAME
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN_NAME(dst, src) \
    *(dst) = *(src);                                  \
    _ZP_VARIANT_TEMPLATE_2_DESTROY_FN_NAME(src);
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_3_DESTROY_FN_NAME
#define _ZP_VARIANT_TEMPLATE_3_DESTROY_FN_NAME(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_3_MOVE_FN_NAME
#define _ZP_VARIANT_TEMPLATE_3_MOVE_FN_NAME(dst, src) \
    *(dst) = *(src);                                  \
    _ZP_VARIANT_TEMPLATE_3_DESTROY_FN_NAME(src);
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_4_DESTROY_FN_NAME
#define _ZP_VARIANT_TEMPLATE_4_DESTROY_FN_NAME(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_4_MOVE_FN_NAME
#define _ZP_VARIANT_TEMPLATE_4_MOVE_FN_NAME(dst, src) \
    *(dst) = *(src);                                  \
    _ZP_VARIANT_TEMPLATE_4_DESTROY_FN_NAME(src);
#endif
#endif

#ifndef _ZP_VARIANT_VISIT

#define _ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(N) \
    typedef struct __zp_variant_sentinel_##N##_t {           \
        uint8_t _##N;                                        \
    } __zp_variant_sentinel_##N##_t;                         \
    static inline void __zp_variant_check_visit_fn_##N(__zp_variant_sentinel_##N##_t f) { (void)f; }

_ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(1)
_ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(2)
_ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(3)
_ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(4)

#define _ZP_VARIANT_VISIT_1(v, f_1)         \
    __zp_variant_check_visit_fn_1((v)->_0); \
    if ((int)(v)->_tag == 1) {              \
        f_1(&(v)->_1);                      \
    }
#define _ZP_VARIANT_VISIT_2(v, f_1, f_2)    \
    __zp_variant_check_visit_fn_2((v)->_0); \
    switch ((int)(v)->_tag) {               \
        case 1: {                           \
            f_1((&(v)->_1));                \
        } break;                            \
        case 2: {                           \
            f_2((&(v)->_2));                \
        } break;                            \
    }

#define _ZP_VARIANT_VISIT_3(v, f_1, f_2, f_3) \
    __zp_variant_check_visit_fn_3((v)->_0);   \
    switch ((int)(v)->_tag) {                 \
        case 1: {                             \
            f_1((&(v)->_1));                  \
        } break;                              \
        case 2: {                             \
            f_2((&(v)->_2));                  \
        } break;                              \
        case 3: {                             \
            f_3((&(v)->_3));                  \
        } break;                              \
    }
#define _ZP_VARIANT_VISIT_4(v, f_1, f_2, f_3, f_4) \
    __zp_variant_check_visit_fn_4((v)->_0);        \
    switch ((int)(v)->_tag) {                      \
        case 1: {                                  \
            f_1((&(v)->_1));                       \
        } break;                                   \
        case 2: {                                  \
            f_2((&(v)->_2));                       \
        } break;                                   \
        case 3: {                                  \
            f_3((&(v)->_3));                       \
        } break;                                   \
        case 4: {                                  \
            f_4((&(v)->_4));                       \
        } break;                                   \
    }

// _ZP_VARIANT_VISIT dispatches to the correct visitor function based on the tag.
#define _ZP_VARIANT_VISIT(v, ...) _ZP_CAT(_ZP_VARIANT_VISIT, _ZP_VA_NARGS(__VA_ARGS__))(v, __VA_ARGS__)
#undef _ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE
#endif
// ── Internal name helpers ─────────────────────────────────────────────────────
//
// All public symbols are derived from NAME and the user-supplied alternative
// names so that multiple instantiations in the same translation unit never
// collide.  Three-part names use two chained _ZP_CAT calls:
//   _ZP_CAT(NAME, _ZP_CAT(TAG, AN))  →  NAME_TAG_AN

#define _ZP_VARIANT_TEMPLATE_TYPE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, t)
#define _ZP_VARIANT_TEMPLATE_TAG_TYPE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, tag_t)

// Tag enum values
#define _ZP_VARIANT_TAG_NONE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, tag_none)
#define _ZP_VARIANT_TAG_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_TAG_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_2_NAME))
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#define _ZP_VARIANT_TAG_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_3_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#define _ZP_VARIANT_TAG_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_4_NAME))
#endif

// Function names
#define _ZP_VARIANT_FN_FROM_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_FN_FROM_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_2_NAME))
#define _ZP_VARIANT_FN_IS_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_FN_IS_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_2_NAME))
#define _ZP_VARIANT_FN_GET_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_FN_GET_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_2_NAME))
#define _ZP_VARIANT_FN_TAKE_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_FN_TAKE_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_2_NAME))
#define _ZP_VARIANT_FN_VISIT _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, visit)
#define _ZP_VARIANT_FN_VISIT_CTX _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, visit_ctx)
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#define _ZP_VARIANT_FN_FROM_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_3_NAME))
#define _ZP_VARIANT_FN_IS_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_3_NAME))
#define _ZP_VARIANT_FN_GET_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_3_NAME))
#define _ZP_VARIANT_FN_TAKE_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_3_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#define _ZP_VARIANT_FN_FROM_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_4_NAME))
#define _ZP_VARIANT_FN_IS_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_4_NAME))
#define _ZP_VARIANT_FN_GET_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_4_NAME))
#define _ZP_VARIANT_FN_TAKE_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_4_NAME))
#endif

// ── Tag enum ──────────────────────────────────────────────────────────────────
//
// TAG_NONE (= 0) is the zero-initialised / moved-from / empty state.

typedef enum _ZP_VARIANT_TEMPLATE_TAG_TYPE {
    _ZP_VARIANT_TAG_NONE = 0,
    _ZP_VARIANT_TAG_1 = 1,
    _ZP_VARIANT_TAG_2 = 2,
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
    _ZP_VARIANT_TAG_3 = 3,
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
    _ZP_VARIANT_TAG_4 = 4,
#endif
} _ZP_VARIANT_TEMPLATE_TAG_TYPE;

// ── Variant struct ────────────────────────────────────────────────────────────

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#define _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE __zp_variant_sentinel_4_t
#elif defined(_ZP_VARIANT_TEMPLATE_3_TYPE)
#define _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE __zp_variant_sentinel_3_t
#else
#define _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE __zp_variant_sentinel_2_t
#endif

typedef struct _ZP_VARIANT_TEMPLATE_TYPE {
    _ZP_VARIANT_TEMPLATE_TAG_TYPE _tag;
    union {
        _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE _0;
        _ZP_VARIANT_TEMPLATE_1_TYPE _1;
        _ZP_VARIANT_TEMPLATE_2_TYPE _2;
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
        _ZP_VARIANT_TEMPLATE_3_TYPE _3;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
        _ZP_VARIANT_TEMPLATE_4_TYPE _4;
#endif
    };
} _ZP_VARIANT_TEMPLATE_TYPE;

// ── NAME_none ─────────────────────────────────────────────────────────────────

static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, none)(void) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_NONE;
    return v;
}

// ── NAME_from_XN ─────────────────────────────────────────────────────────────

static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_1(_ZP_VARIANT_TEMPLATE_1_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_1;
    _ZP_VARIANT_TEMPLATE_1_MOVE_FN_NAME(&v._1, val);
    return v;
}

static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_2(_ZP_VARIANT_TEMPLATE_2_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_2;
    _ZP_VARIANT_TEMPLATE_2_MOVE_FN_NAME(&v._2, val);
    return v;
}

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_3(_ZP_VARIANT_TEMPLATE_3_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_3;
    _ZP_VARIANT_TEMPLATE_3_MOVE_FN_NAME(&v._3, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_4(_ZP_VARIANT_TEMPLATE_4_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_4;
    _ZP_VARIANT_TEMPLATE_4_MOVE_FN_NAME(&v._4, val);
    return v;
}
#endif

// ── NAME_destroy ──────────────────────────────────────────────────────────────

static inline void _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, destroy)(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    if (v->_tag == _ZP_VARIANT_TAG_1) {
        _ZP_VARIANT_TEMPLATE_1_DESTROY_FN_NAME(&v->_1);
    } else if (v->_tag == _ZP_VARIANT_TAG_2) {
        _ZP_VARIANT_TEMPLATE_2_DESTROY_FN_NAME(&v->_2);
    }
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
    else if (v->_tag == _ZP_VARIANT_TAG_3) {
        _ZP_VARIANT_TEMPLATE_3_DESTROY_FN_NAME(&v->_3);
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
    else if (v->_tag == _ZP_VARIANT_TAG_4) {
        _ZP_VARIANT_TEMPLATE_4_DESTROY_FN_NAME(&v->_4);
    }
#endif
    v->_tag = _ZP_VARIANT_TAG_NONE;
}

// ── NAME_move ─────────────────────────────────────────────────────────────────
// Moves *src into *dst.  Previous contents of *dst are destroyed.
// *src is left in NONE state.

static inline void _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, move)(_ZP_VARIANT_TEMPLATE_TYPE *dst,
                                                            _ZP_VARIANT_TEMPLATE_TYPE *src) {
    _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, destroy)(dst);
    if (src->_tag == _ZP_VARIANT_TAG_1) {
        _ZP_VARIANT_TEMPLATE_1_MOVE_FN_NAME(&dst->_1, &src->_1);
        dst->_tag = _ZP_VARIANT_TAG_1;
    } else if (src->_tag == _ZP_VARIANT_TAG_2) {
        _ZP_VARIANT_TEMPLATE_2_MOVE_FN_NAME(&dst->_2, &src->_2);
        dst->_tag = _ZP_VARIANT_TAG_2;
    }
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
    else if (src->_tag == _ZP_VARIANT_TAG_3) {
        _ZP_VARIANT_TEMPLATE_3_MOVE_FN_NAME(&dst->_3, &src->_3);
        dst->_tag = _ZP_VARIANT_TAG_3;
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
    else if (src->_tag == _ZP_VARIANT_TAG_4) {
        _ZP_VARIANT_TEMPLATE_4_MOVE_FN_NAME(&dst->_4, &src->_4);
        dst->_tag = _ZP_VARIANT_TAG_4;
    }
#endif
    else {
        dst->_tag = _ZP_VARIANT_TAG_NONE;
    }
    src->_tag = _ZP_VARIANT_TAG_NONE;
}

// ── NAME_tag / NAME_is_none / NAME_is_XN ─────────────────────────────────────

static inline _ZP_VARIANT_TEMPLATE_TAG_TYPE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME,
                                                    tag)(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return v->_tag;
}

static inline bool _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, is_none)(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return v->_tag == _ZP_VARIANT_TAG_NONE;
}

static inline bool _ZP_VARIANT_FN_IS_1(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_1; }

static inline bool _ZP_VARIANT_FN_IS_2(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_2; }

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline bool _ZP_VARIANT_FN_IS_3(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_3; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline bool _ZP_VARIANT_FN_IS_4(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_4; }
#endif

// ── NAME_get_XN ──────────────────────────────────────────────────────────────
// Returns a pointer to the active member.
// UB if the variant does not hold the requested alternative.
// For read-only access cast the result to const at the call site.

static inline _ZP_VARIANT_TEMPLATE_1_TYPE *_ZP_VARIANT_FN_GET_1(_ZP_VARIANT_TEMPLATE_TYPE *v) { return &v->_1; }

static inline _ZP_VARIANT_TEMPLATE_2_TYPE *_ZP_VARIANT_FN_GET_2(_ZP_VARIANT_TEMPLATE_TYPE *v) { return &v->_2; }

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline _ZP_VARIANT_TEMPLATE_3_TYPE *_ZP_VARIANT_FN_GET_3(_ZP_VARIANT_TEMPLATE_TYPE *v) { return &v->_3; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline _ZP_VARIANT_TEMPLATE_4_TYPE *_ZP_VARIANT_FN_GET_4(_ZP_VARIANT_TEMPLATE_TYPE *v) { return &v->_4; }
#endif

// ── NAME_take_XN ─────────────────────────────────────────────────────────────
// Moves the held value out into *out and resets the variant to NONE.
// Returns true on success; returns false (leaving *v and *out untouched) if
// the variant does not currently hold the requested alternative.

static inline bool _ZP_VARIANT_FN_TAKE_1(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_1_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_1) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_1_MOVE_FN_NAME(out, &v->_1);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}

static inline bool _ZP_VARIANT_FN_TAKE_2(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_2_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_2) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_2_MOVE_FN_NAME(out, &v->_2);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_3(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_3_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_3) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_3_MOVE_FN_NAME(out, &v->_3);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_4(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_4_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_4) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_4_MOVE_FN_NAME(out, &v->_4);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif
// ── Undef all macros ──────────────────────────────────────────────────────────
#undef _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE
#undef _ZP_VARIANT_TEMPLATE_NAME
#undef _ZP_VARIANT_TEMPLATE_1_TYPE
#undef _ZP_VARIANT_TEMPLATE_2_TYPE
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#undef _ZP_VARIANT_TEMPLATE_3_TYPE
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#undef _ZP_VARIANT_TEMPLATE_4_TYPE
#endif
#undef _ZP_VARIANT_TEMPLATE_1_NAME
#undef _ZP_VARIANT_TEMPLATE_2_NAME
#ifdef _ZP_VARIANT_TEMPLATE_3_NAME
#undef _ZP_VARIANT_TEMPLATE_3_NAME
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_NAME
#undef _ZP_VARIANT_TEMPLATE_4_NAME
#endif
#undef _ZP_VARIANT_TEMPLATE_1_DESTROY_FN_NAME
#undef _ZP_VARIANT_TEMPLATE_2_DESTROY_FN_NAME
#ifdef _ZP_VARIANT_TEMPLATE_3_DESTROY_FN_NAME
#undef _ZP_VARIANT_TEMPLATE_3_DESTROY_FN_NAME
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_DESTROY_FN_NAME
#undef _ZP_VARIANT_TEMPLATE_4_DESTROY_FN_NAME
#endif
#undef _ZP_VARIANT_TEMPLATE_1_MOVE_FN_NAME
#undef _ZP_VARIANT_TEMPLATE_2_MOVE_FN_NAME
#ifdef _ZP_VARIANT_TEMPLATE_3_MOVE_FN_NAME
#undef _ZP_VARIANT_TEMPLATE_3_MOVE_FN_NAME
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_MOVE_FN_NAME
#undef _ZP_VARIANT_TEMPLATE_4_MOVE_FN_NAME
#endif
#undef _ZP_VARIANT_TEMPLATE_TYPE
#undef _ZP_VARIANT_TEMPLATE_TAG_TYPE
#undef _ZP_VARIANT_TAG_NONE
#undef _ZP_VARIANT_TAG_1
#undef _ZP_VARIANT_TAG_2
#ifdef _ZP_VARIANT_TAG_3
#undef _ZP_VARIANT_TAG_3
#endif
#ifdef _ZP_VARIANT_TAG_4
#undef _ZP_VARIANT_TAG_4
#endif
#undef _ZP_VARIANT_FN_FROM_1
#undef _ZP_VARIANT_FN_FROM_2
#ifdef _ZP_VARIANT_FN_FROM_3
#undef _ZP_VARIANT_FN_FROM_3
#endif
#ifdef _ZP_VARIANT_FN_FROM_4
#undef _ZP_VARIANT_FN_FROM_4
#endif
#undef _ZP_VARIANT_FN_IS_1
#undef _ZP_VARIANT_FN_IS_2
#ifdef _ZP_VARIANT_FN_IS_3
#undef _ZP_VARIANT_FN_IS_3
#endif
#ifdef _ZP_VARIANT_FN_IS_4
#undef _ZP_VARIANT_FN_IS_4
#endif
#undef _ZP_VARIANT_FN_GET_1
#undef _ZP_VARIANT_FN_GET_2
#ifdef _ZP_VARIANT_FN_GET_3
#undef _ZP_VARIANT_FN_GET_3
#endif
#ifdef _ZP_VARIANT_FN_GET_4
#undef _ZP_VARIANT_FN_GET_4
#endif
#undef _ZP_VARIANT_FN_TAKE_1
#undef _ZP_VARIANT_FN_TAKE_2
#ifdef _ZP_VARIANT_FN_TAKE_3
#undef _ZP_VARIANT_FN_TAKE_3
#endif
#ifdef _ZP_VARIANT_FN_TAKE_4
#undef _ZP_VARIANT_FN_TAKE_4
#endif
