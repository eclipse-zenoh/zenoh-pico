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

// Tagged-union variant holding one of two to eight user-supplied types at a
// time, plus an empty/uninitialised state.  (Similar to std::variant<A,B[,C..H]>.)
//
// Design summary
// ──────────────
// • States: NONE (default / moved-from), A, B, and optionally C–H.
//   Each extra type requires all previous ones (C requires A+B; D requires C; etc.).
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
// Optional extra alternatives (each requires all previous ones):
//   _ZP_VARIANT_TEMPLATE_3_TYPE   — enables a third alternative
//   _ZP_VARIANT_TEMPLATE_4_TYPE   — enables a fourth alternative (requires 3)
//   _ZP_VARIANT_TEMPLATE_5_TYPE   — enables a fifth alternative (requires 4)
//   _ZP_VARIANT_TEMPLATE_6_TYPE   — enables a sixth alternative (requires 5)
//   _ZP_VARIANT_TEMPLATE_7_TYPE   — enables a seventh alternative (requires 6)
//   _ZP_VARIANT_TEMPLATE_8_TYPE   — enables an eighth alternative (requires 7)
//
// Optional naming (default: 1, 2, 3, 4, 5, 6, 7, 8):
//   _ZP_VARIANT_TEMPLATE_1_NAME   identifier suffix for alternative 1 symbols
//   _ZP_VARIANT_TEMPLATE_2_NAME   identifier suffix for alternative 2 symbols
//   _ZP_VARIANT_TEMPLATE_3_NAME   identifier suffix for alternative 3 symbols
//   _ZP_VARIANT_TEMPLATE_4_NAME   identifier suffix for alternative 4 symbols
//   _ZP_VARIANT_TEMPLATE_5_NAME   identifier suffix for alternative 5 symbols
//   _ZP_VARIANT_TEMPLATE_6_NAME   identifier suffix for alternative 6 symbols
//   _ZP_VARIANT_TEMPLATE_7_NAME   identifier suffix for alternative 7 symbols
//   _ZP_VARIANT_TEMPLATE_8_NAME   identifier suffix for alternative 8 symbols
//       E.g. 1_NAME=ok, 2_NAME=err produces NAME_from_ok / NAME_is_ok /
//       NAME_get_ok / NAME_take_ok and NAME_TAG_ok.
//
// Optional callbacks (default: no-op destroy, shallow-copy move):
//   _ZP_VARIANT_TEMPLATE_1_DESTROY_FN(ptr)
//   _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(ptr)
//   _ZP_VARIANT_TEMPLATE_3_DESTROY_FN(ptr)
//   _ZP_VARIANT_TEMPLATE_4_DESTROY_FN(ptr)
//   _ZP_VARIANT_TEMPLATE_5_DESTROY_FN(ptr)
//   _ZP_VARIANT_TEMPLATE_6_DESTROY_FN(ptr)
//   _ZP_VARIANT_TEMPLATE_7_DESTROY_FN(ptr)
//   _ZP_VARIANT_TEMPLATE_8_DESTROY_FN(ptr)
//   _ZP_VARIANT_TEMPLATE_1_MOVE_FN(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_2_MOVE_FN(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_3_MOVE_FN(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_4_MOVE_FN(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_5_MOVE_FN(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_6_MOVE_FN(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_7_MOVE_FN(dst_ptr, src_ptr)
//   _ZP_VARIANT_TEMPLATE_8_MOVE_FN(dst_ptr, src_ptr)
//
// Generated API  (NAME=_ZP_VARIANT_TEMPLATE_NAME, XN=X alternative name)
// ────────────────────────────────────────────────────────────────────────
// Types:
//   NAME_t        — the variant struct
//   NAME_tag_t    — tag enum { NAME_TAG_NONE=0, NAME_TAG_1N, NAME_TAG_2N[, ...] }
//
// Construction / destruction:
//   NAME_t  NAME_none(void)
//   NAME_t  NAME_from_1N(1_TYPE *val)   — moves *val into new variant
//   NAME_t  NAME_from_2N(2_TYPE *val)
//   NAME_t  NAME_from_3N(3_TYPE *val)   — only if 3_TYPE defined
//   NAME_t  NAME_from_4N(4_TYPE *val)   — only if 4_TYPE defined
//   NAME_t  NAME_from_5N(5_TYPE *val)   — only if 5_TYPE defined
//   NAME_t  NAME_from_6N(6_TYPE *val)   — only if 6_TYPE defined
//   NAME_t  NAME_from_7N(7_TYPE *val)   — only if 7_TYPE defined
//   NAME_t  NAME_from_8N(8_TYPE *val)   — only if 8_TYPE defined
//   void    NAME_destroy(NAME_t *v)
//
// Move (variant-to-variant):
//   void    NAME_move(NAME_t *dst, NAME_t *src)
//
// Inspection:
//   bool       NAME_is_none(const NAME_t *v)
//   bool       NAME_is_1N  (const NAME_t *v)
//   bool       NAME_is_2N  (const NAME_t *v)
//   bool       NAME_is_3N  (const NAME_t *v)   — only if 3_TYPE defined
//   bool       NAME_is_4N  (const NAME_t *v)   — only if 4_TYPE defined
//   bool       NAME_is_5N  (const NAME_t *v)   — only if 5_TYPE defined
//   bool       NAME_is_6N  (const NAME_t *v)   — only if 6_TYPE defined
//   bool       NAME_is_7N  (const NAME_t *v)   — only if 7_TYPE defined
//   bool       NAME_is_8N  (const NAME_t *v)   — only if 8_TYPE defined
//   NAME_tag_t NAME_tag    (const NAME_t *v)
//
// Non-consuming access (UB if wrong alternative):
//   1_TYPE *NAME_get_1N(NAME_t *v)
//   2_TYPE *NAME_get_2N(NAME_t *v)
//   3_TYPE *NAME_get_3N(NAME_t *v)   — only if 3_TYPE defined
//   4_TYPE *NAME_get_4N(NAME_t *v)   — only if 4_TYPE defined
//   5_TYPE *NAME_get_5N(NAME_t *v)   — only if 5_TYPE defined
//   6_TYPE *NAME_get_6N(NAME_t *v)   — only if 6_TYPE defined
//   7_TYPE *NAME_get_7N(NAME_t *v)   — only if 7_TYPE defined
//   8_TYPE *NAME_get_8N(NAME_t *v)   — only if 8_TYPE defined
//
// Consuming access (moves value out, returns false if wrong alternative):
//   bool    NAME_take_1N(NAME_t *v, 1_TYPE *out)
//   bool    NAME_take_2N(NAME_t *v, 2_TYPE *out)
//   bool    NAME_take_3N(NAME_t *v, 3_TYPE *out)   — only if 3_TYPE defined
//   bool    NAME_take_4N(NAME_t *v, 4_TYPE *out)   — only if 4_TYPE defined
//   bool    NAME_take_5N(NAME_t *v, 5_TYPE *out)   — only if 5_TYPE defined
//   bool    NAME_take_6N(NAME_t *v, 6_TYPE *out)   — only if 6_TYPE defined
//   bool    NAME_take_7N(NAME_t *v, 7_TYPE *out)   — only if 7_TYPE defined
//   bool    NAME_take_8N(NAME_t *v, 8_TYPE *out)   — only if 8_TYPE defined
//

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

// ── Validate ordering requirements ───────────────────────────────────────────

#if defined(_ZP_VARIANT_TEMPLATE_4_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_3_TYPE)
#error "variant_template.h: _ZP_VARIANT_TEMPLATE_4_TYPE requires _ZP_VARIANT_TEMPLATE_3_TYPE"
#endif
#if defined(_ZP_VARIANT_TEMPLATE_5_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_4_TYPE)
#error "variant_template.h: _ZP_VARIANT_TEMPLATE_5_TYPE requires _ZP_VARIANT_TEMPLATE_4_TYPE"
#endif
#if defined(_ZP_VARIANT_TEMPLATE_6_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_5_TYPE)
#error "variant_template.h: _ZP_VARIANT_TEMPLATE_6_TYPE requires _ZP_VARIANT_TEMPLATE_5_TYPE"
#endif
#if defined(_ZP_VARIANT_TEMPLATE_7_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_6_TYPE)
#error "variant_template.h: _ZP_VARIANT_TEMPLATE_7_TYPE requires _ZP_VARIANT_TEMPLATE_6_TYPE"
#endif
#if defined(_ZP_VARIANT_TEMPLATE_8_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_7_TYPE)
#error "variant_template.h: _ZP_VARIANT_TEMPLATE_8_TYPE requires _ZP_VARIANT_TEMPLATE_7_TYPE"
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
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_5_NAME
#define _ZP_VARIANT_TEMPLATE_5_NAME 5
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_6_NAME
#define _ZP_VARIANT_TEMPLATE_6_NAME 6
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_7_NAME
#define _ZP_VARIANT_TEMPLATE_7_NAME 7
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_8_NAME
#define _ZP_VARIANT_TEMPLATE_8_NAME 8
#endif
#endif

// ── Optional callbacks with defaults ─────────────────────────────────────────

#ifndef _ZP_VARIANT_TEMPLATE_1_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_1_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_2_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_1_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_1_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#ifndef _ZP_VARIANT_TEMPLATE_2_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_3_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_3_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_3_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_3_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_4_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_4_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_4_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_4_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_5_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_5_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_5_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_5_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_6_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_6_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_6_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_6_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_7_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_7_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_7_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_7_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_8_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_8_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_8_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_8_MOVE_FN(dst, src) *(dst) = *(src);
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
_ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(5)
_ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(6)
_ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(7)
_ZP_VARIANT_TEMPLATE_DEFINE_VISIT_FN_CHECKER_TYPE(8)

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
#define _ZP_VARIANT_VISIT_5(v, f_1, f_2, f_3, f_4, f_5) \
    __zp_variant_check_visit_fn_5((v)->_0);             \
    switch ((int)(v)->_tag) {                           \
        case 1: {                                       \
            f_1((&(v)->_1));                            \
        } break;                                        \
        case 2: {                                       \
            f_2((&(v)->_2));                            \
        } break;                                        \
        case 3: {                                       \
            f_3((&(v)->_3));                            \
        } break;                                        \
        case 4: {                                       \
            f_4((&(v)->_4));                            \
        } break;                                        \
        case 5: {                                       \
            f_5((&(v)->_5));                            \
        } break;                                        \
    }
#define _ZP_VARIANT_VISIT_6(v, f_1, f_2, f_3, f_4, f_5, f_6) \
    __zp_variant_check_visit_fn_6((v)->_0);                  \
    switch ((int)(v)->_tag) {                                \
        case 1: {                                            \
            f_1((&(v)->_1));                                 \
        } break;                                             \
        case 2: {                                            \
            f_2((&(v)->_2));                                 \
        } break;                                             \
        case 3: {                                            \
            f_3((&(v)->_3));                                 \
        } break;                                             \
        case 4: {                                            \
            f_4((&(v)->_4));                                 \
        } break;                                             \
        case 5: {                                            \
            f_5((&(v)->_5));                                 \
        } break;                                             \
        case 6: {                                            \
            f_6((&(v)->_6));                                 \
        } break;                                             \
    }
#define _ZP_VARIANT_VISIT_7(v, f_1, f_2, f_3, f_4, f_5, f_6, f_7) \
    __zp_variant_check_visit_fn_7((v)->_0);                       \
    switch ((int)(v)->_tag) {                                     \
        case 1: {                                                 \
            f_1((&(v)->_1));                                      \
        } break;                                                  \
        case 2: {                                                 \
            f_2((&(v)->_2));                                      \
        } break;                                                  \
        case 3: {                                                 \
            f_3((&(v)->_3));                                      \
        } break;                                                  \
        case 4: {                                                 \
            f_4((&(v)->_4));                                      \
        } break;                                                  \
        case 5: {                                                 \
            f_5((&(v)->_5));                                      \
        } break;                                                  \
        case 6: {                                                 \
            f_6((&(v)->_6));                                      \
        } break;                                                  \
        case 7: {                                                 \
            f_7((&(v)->_7));                                      \
        } break;                                                  \
    }
#define _ZP_VARIANT_VISIT_8(v, f_1, f_2, f_3, f_4, f_5, f_6, f_7, f_8) \
    __zp_variant_check_visit_fn_8((v)->_0);                            \
    switch ((int)(v)->_tag) {                                          \
        case 1: {                                                      \
            f_1((&(v)->_1));                                           \
        } break;                                                       \
        case 2: {                                                      \
            f_2((&(v)->_2));                                           \
        } break;                                                       \
        case 3: {                                                      \
            f_3((&(v)->_3));                                           \
        } break;                                                       \
        case 4: {                                                      \
            f_4((&(v)->_4));                                           \
        } break;                                                       \
        case 5: {                                                      \
            f_5((&(v)->_5));                                           \
        } break;                                                       \
        case 6: {                                                      \
            f_6((&(v)->_6));                                           \
        } break;                                                       \
        case 7: {                                                      \
            f_7((&(v)->_7));                                           \
        } break;                                                       \
        case 8: {                                                      \
            f_8((&(v)->_8));                                           \
        } break;                                                       \
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
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
#define _ZP_VARIANT_TAG_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_5_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
#define _ZP_VARIANT_TAG_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_6_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
#define _ZP_VARIANT_TAG_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_7_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#define _ZP_VARIANT_TAG_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_8_NAME))
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
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
#define _ZP_VARIANT_FN_FROM_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_5_NAME))
#define _ZP_VARIANT_FN_IS_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_5_NAME))
#define _ZP_VARIANT_FN_GET_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_5_NAME))
#define _ZP_VARIANT_FN_TAKE_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_5_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
#define _ZP_VARIANT_FN_FROM_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_6_NAME))
#define _ZP_VARIANT_FN_IS_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_6_NAME))
#define _ZP_VARIANT_FN_GET_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_6_NAME))
#define _ZP_VARIANT_FN_TAKE_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_6_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
#define _ZP_VARIANT_FN_FROM_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_7_NAME))
#define _ZP_VARIANT_FN_IS_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_7_NAME))
#define _ZP_VARIANT_FN_GET_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_7_NAME))
#define _ZP_VARIANT_FN_TAKE_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_7_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#define _ZP_VARIANT_FN_FROM_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_8_NAME))
#define _ZP_VARIANT_FN_IS_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_8_NAME))
#define _ZP_VARIANT_FN_GET_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_8_NAME))
#define _ZP_VARIANT_FN_TAKE_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_8_NAME))
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
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
    _ZP_VARIANT_TAG_5 = 5,
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
    _ZP_VARIANT_TAG_6 = 6,
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
    _ZP_VARIANT_TAG_7 = 7,
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
    _ZP_VARIANT_TAG_8 = 8,
#endif
} _ZP_VARIANT_TEMPLATE_TAG_TYPE;

// ── Variant struct ────────────────────────────────────────────────────────────

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#define _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE __zp_variant_sentinel_8_t
#elif defined(_ZP_VARIANT_TEMPLATE_7_TYPE)
#define _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE __zp_variant_sentinel_7_t
#elif defined(_ZP_VARIANT_TEMPLATE_6_TYPE)
#define _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE __zp_variant_sentinel_6_t
#elif defined(_ZP_VARIANT_TEMPLATE_5_TYPE)
#define _ZP_VARIANT_TEMPLATE_SENTINEL_TYPE __zp_variant_sentinel_5_t
#elif defined(_ZP_VARIANT_TEMPLATE_4_TYPE)
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
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
        _ZP_VARIANT_TEMPLATE_5_TYPE _5;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
        _ZP_VARIANT_TEMPLATE_6_TYPE _6;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
        _ZP_VARIANT_TEMPLATE_7_TYPE _7;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
        _ZP_VARIANT_TEMPLATE_8_TYPE _8;
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
    _ZP_VARIANT_TEMPLATE_1_MOVE_FN(&v._1, val);
    return v;
}

static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_2(_ZP_VARIANT_TEMPLATE_2_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_2;
    _ZP_VARIANT_TEMPLATE_2_MOVE_FN(&v._2, val);
    return v;
}

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_3(_ZP_VARIANT_TEMPLATE_3_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_3;
    _ZP_VARIANT_TEMPLATE_3_MOVE_FN(&v._3, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_4(_ZP_VARIANT_TEMPLATE_4_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_4;
    _ZP_VARIANT_TEMPLATE_4_MOVE_FN(&v._4, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_5(_ZP_VARIANT_TEMPLATE_5_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_5;
    _ZP_VARIANT_TEMPLATE_5_MOVE_FN(&v._5, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_6(_ZP_VARIANT_TEMPLATE_6_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_6;
    _ZP_VARIANT_TEMPLATE_6_MOVE_FN(&v._6, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_7(_ZP_VARIANT_TEMPLATE_7_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_7;
    _ZP_VARIANT_TEMPLATE_7_MOVE_FN(&v._7, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_8(_ZP_VARIANT_TEMPLATE_8_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_8;
    _ZP_VARIANT_TEMPLATE_8_MOVE_FN(&v._8, val);
    return v;
}
#endif

// ── NAME_destroy ──────────────────────────────────────────────────────────────

static inline void _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, destroy)(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    if (v->_tag == _ZP_VARIANT_TAG_1) {
        _ZP_VARIANT_TEMPLATE_1_DESTROY_FN(&v->_1);
    } else if (v->_tag == _ZP_VARIANT_TAG_2) {
        _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(&v->_2);
    }
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
    else if (v->_tag == _ZP_VARIANT_TAG_3) {
        _ZP_VARIANT_TEMPLATE_3_DESTROY_FN(&v->_3);
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
    else if (v->_tag == _ZP_VARIANT_TAG_4) {
        _ZP_VARIANT_TEMPLATE_4_DESTROY_FN(&v->_4);
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
    else if (v->_tag == _ZP_VARIANT_TAG_5) {
        _ZP_VARIANT_TEMPLATE_5_DESTROY_FN(&v->_5);
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
    else if (v->_tag == _ZP_VARIANT_TAG_6) {
        _ZP_VARIANT_TEMPLATE_6_DESTROY_FN(&v->_6);
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
    else if (v->_tag == _ZP_VARIANT_TAG_7) {
        _ZP_VARIANT_TEMPLATE_7_DESTROY_FN(&v->_7);
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
    else if (v->_tag == _ZP_VARIANT_TAG_8) {
        _ZP_VARIANT_TEMPLATE_8_DESTROY_FN(&v->_8);
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
        _ZP_VARIANT_TEMPLATE_1_MOVE_FN(&dst->_1, &src->_1);
        dst->_tag = _ZP_VARIANT_TAG_1;
    } else if (src->_tag == _ZP_VARIANT_TAG_2) {
        _ZP_VARIANT_TEMPLATE_2_MOVE_FN(&dst->_2, &src->_2);
        dst->_tag = _ZP_VARIANT_TAG_2;
    }
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
    else if (src->_tag == _ZP_VARIANT_TAG_3) {
        _ZP_VARIANT_TEMPLATE_3_MOVE_FN(&dst->_3, &src->_3);
        dst->_tag = _ZP_VARIANT_TAG_3;
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
    else if (src->_tag == _ZP_VARIANT_TAG_4) {
        _ZP_VARIANT_TEMPLATE_4_MOVE_FN(&dst->_4, &src->_4);
        dst->_tag = _ZP_VARIANT_TAG_4;
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
    else if (src->_tag == _ZP_VARIANT_TAG_5) {
        _ZP_VARIANT_TEMPLATE_5_MOVE_FN(&dst->_5, &src->_5);
        dst->_tag = _ZP_VARIANT_TAG_5;
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
    else if (src->_tag == _ZP_VARIANT_TAG_6) {
        _ZP_VARIANT_TEMPLATE_6_MOVE_FN(&dst->_6, &src->_6);
        dst->_tag = _ZP_VARIANT_TAG_6;
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
    else if (src->_tag == _ZP_VARIANT_TAG_7) {
        _ZP_VARIANT_TEMPLATE_7_MOVE_FN(&dst->_7, &src->_7);
        dst->_tag = _ZP_VARIANT_TAG_7;
    }
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
    else if (src->_tag == _ZP_VARIANT_TAG_8) {
        _ZP_VARIANT_TEMPLATE_8_MOVE_FN(&dst->_8, &src->_8);
        dst->_tag = _ZP_VARIANT_TAG_8;
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

#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
static inline bool _ZP_VARIANT_FN_IS_5(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_5; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
static inline bool _ZP_VARIANT_FN_IS_6(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_6; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
static inline bool _ZP_VARIANT_FN_IS_7(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_7; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
static inline bool _ZP_VARIANT_FN_IS_8(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_8; }
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

#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
static inline _ZP_VARIANT_TEMPLATE_5_TYPE *_ZP_VARIANT_FN_GET_5(_ZP_VARIANT_TEMPLATE_TYPE *v) { return &v->_5; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
static inline _ZP_VARIANT_TEMPLATE_6_TYPE *_ZP_VARIANT_FN_GET_6(_ZP_VARIANT_TEMPLATE_TYPE *v) { return &v->_6; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
static inline _ZP_VARIANT_TEMPLATE_7_TYPE *_ZP_VARIANT_FN_GET_7(_ZP_VARIANT_TEMPLATE_TYPE *v) { return &v->_7; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
static inline _ZP_VARIANT_TEMPLATE_8_TYPE *_ZP_VARIANT_FN_GET_8(_ZP_VARIANT_TEMPLATE_TYPE *v) { return &v->_8; }
#endif

// ── NAME_take_XN ─────────────────────────────────────────────────────────────
// Moves the held value out into *out and resets the variant to NONE.
// Returns true on success; returns false (leaving *v and *out untouched) if
// the variant does not currently hold the requested alternative.

static inline bool _ZP_VARIANT_FN_TAKE_1(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_1_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_1) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_1_MOVE_FN(out, &v->_1);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}

static inline bool _ZP_VARIANT_FN_TAKE_2(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_2_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_2) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_2_MOVE_FN(out, &v->_2);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_3(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_3_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_3) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_3_MOVE_FN(out, &v->_3);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_4(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_4_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_4) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_4_MOVE_FN(out, &v->_4);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_5(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_5_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_5) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_5_MOVE_FN(out, &v->_5);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_6(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_6_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_6) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_6_MOVE_FN(out, &v->_6);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_7(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_7_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_7) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_7_MOVE_FN(out, &v->_7);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_8(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_8_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_8) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_8_MOVE_FN(out, &v->_8);
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
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
#undef _ZP_VARIANT_TEMPLATE_5_TYPE
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
#undef _ZP_VARIANT_TEMPLATE_6_TYPE
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
#undef _ZP_VARIANT_TEMPLATE_7_TYPE
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#undef _ZP_VARIANT_TEMPLATE_8_TYPE
#endif
#undef _ZP_VARIANT_TEMPLATE_1_NAME
#undef _ZP_VARIANT_TEMPLATE_2_NAME
#ifdef _ZP_VARIANT_TEMPLATE_3_NAME
#undef _ZP_VARIANT_TEMPLATE_3_NAME
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_NAME
#undef _ZP_VARIANT_TEMPLATE_4_NAME
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_NAME
#undef _ZP_VARIANT_TEMPLATE_5_NAME
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_NAME
#undef _ZP_VARIANT_TEMPLATE_6_NAME
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_NAME
#undef _ZP_VARIANT_TEMPLATE_7_NAME
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_NAME
#undef _ZP_VARIANT_TEMPLATE_8_NAME
#endif
#undef _ZP_VARIANT_TEMPLATE_1_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_2_DESTROY_FN
#ifdef _ZP_VARIANT_TEMPLATE_3_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_3_DESTROY_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_4_DESTROY_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_5_DESTROY_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_6_DESTROY_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_7_DESTROY_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_8_DESTROY_FN
#endif
#undef _ZP_VARIANT_TEMPLATE_1_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_2_MOVE_FN
#ifdef _ZP_VARIANT_TEMPLATE_3_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_3_MOVE_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_4_MOVE_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_5_MOVE_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_6_MOVE_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_7_MOVE_FN
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_8_MOVE_FN
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
#ifdef _ZP_VARIANT_TAG_5
#undef _ZP_VARIANT_TAG_5
#endif
#ifdef _ZP_VARIANT_TAG_6
#undef _ZP_VARIANT_TAG_6
#endif
#ifdef _ZP_VARIANT_TAG_7
#undef _ZP_VARIANT_TAG_7
#endif
#ifdef _ZP_VARIANT_TAG_8
#undef _ZP_VARIANT_TAG_8
#endif
#undef _ZP_VARIANT_FN_FROM_1
#undef _ZP_VARIANT_FN_FROM_2
#ifdef _ZP_VARIANT_FN_FROM_3
#undef _ZP_VARIANT_FN_FROM_3
#endif
#ifdef _ZP_VARIANT_FN_FROM_4
#undef _ZP_VARIANT_FN_FROM_4
#endif
#ifdef _ZP_VARIANT_FN_FROM_5
#undef _ZP_VARIANT_FN_FROM_5
#endif
#ifdef _ZP_VARIANT_FN_FROM_6
#undef _ZP_VARIANT_FN_FROM_6
#endif
#ifdef _ZP_VARIANT_FN_FROM_7
#undef _ZP_VARIANT_FN_FROM_7
#endif
#ifdef _ZP_VARIANT_FN_FROM_8
#undef _ZP_VARIANT_FN_FROM_8
#endif
#undef _ZP_VARIANT_FN_IS_1
#undef _ZP_VARIANT_FN_IS_2
#ifdef _ZP_VARIANT_FN_IS_3
#undef _ZP_VARIANT_FN_IS_3
#endif
#ifdef _ZP_VARIANT_FN_IS_4
#undef _ZP_VARIANT_FN_IS_4
#endif
#ifdef _ZP_VARIANT_FN_IS_5
#undef _ZP_VARIANT_FN_IS_5
#endif
#ifdef _ZP_VARIANT_FN_IS_6
#undef _ZP_VARIANT_FN_IS_6
#endif
#ifdef _ZP_VARIANT_FN_IS_7
#undef _ZP_VARIANT_FN_IS_7
#endif
#ifdef _ZP_VARIANT_FN_IS_8
#undef _ZP_VARIANT_FN_IS_8
#endif
#undef _ZP_VARIANT_FN_GET_1
#undef _ZP_VARIANT_FN_GET_2
#ifdef _ZP_VARIANT_FN_GET_3
#undef _ZP_VARIANT_FN_GET_3
#endif
#ifdef _ZP_VARIANT_FN_GET_4
#undef _ZP_VARIANT_FN_GET_4
#endif
#ifdef _ZP_VARIANT_FN_GET_5
#undef _ZP_VARIANT_FN_GET_5
#endif
#ifdef _ZP_VARIANT_FN_GET_6
#undef _ZP_VARIANT_FN_GET_6
#endif
#ifdef _ZP_VARIANT_FN_GET_7
#undef _ZP_VARIANT_FN_GET_7
#endif
#ifdef _ZP_VARIANT_FN_GET_8
#undef _ZP_VARIANT_FN_GET_8
#endif
#undef _ZP_VARIANT_FN_TAKE_1
#undef _ZP_VARIANT_FN_TAKE_2
#ifdef _ZP_VARIANT_FN_TAKE_3
#undef _ZP_VARIANT_FN_TAKE_3
#endif
#ifdef _ZP_VARIANT_FN_TAKE_4
#undef _ZP_VARIANT_FN_TAKE_4
#endif
#ifdef _ZP_VARIANT_FN_TAKE_5
#undef _ZP_VARIANT_FN_TAKE_5
#endif
#ifdef _ZP_VARIANT_FN_TAKE_6
#undef _ZP_VARIANT_FN_TAKE_6
#endif
#ifdef _ZP_VARIANT_FN_TAKE_7
#undef _ZP_VARIANT_FN_TAKE_7
#endif
#ifdef _ZP_VARIANT_FN_TAKE_8
#undef _ZP_VARIANT_FN_TAKE_8
#endif
