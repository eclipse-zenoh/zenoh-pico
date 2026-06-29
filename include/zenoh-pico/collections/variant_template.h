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

// Tagged-union variant holding one of up to eight user-supplied types at a
// time, plus an empty/uninitialised state.  (Similar to std::variant<A[,B..H]>.)
//
// Design summary
// ──────────────
// • States: NONE (default / moved-from) plus any subset of alternatives 1..8.
//   Alternatives are independent and may be sparse: any subset may be defined
//   (e.g. alternative 4 without alternative 3), as long as at least one is.
// • Storage is an anonymous union; the active member is tracked by a tag field.
// • Move semantics: constructors take ownership of the pointed-to value via
//   user-supplied (defaults to shallow copy without altering source) move callbacks.
// • No heap allocation.
//
// User must define the following macros before including this file:
//
// Required:
//   _ZP_VARIANT_TEMPLATE_NAME
//       base name for all generated symbols (e.g. "my_result")
//   At least one of _ZP_VARIANT_TEMPLATE_1_TYPE .. _ZP_VARIANT_TEMPLATE_8_TYPE
//       type of the corresponding alternative
//
// Optional alternatives (any subset, in any combination):
//   _ZP_VARIANT_TEMPLATE_1_TYPE   — enables the first alternative
//   _ZP_VARIANT_TEMPLATE_2_TYPE   — enables the second alternative
//   _ZP_VARIANT_TEMPLATE_3_TYPE   — enables a third alternative
//   _ZP_VARIANT_TEMPLATE_4_TYPE   — enables a fourth alternative
//   _ZP_VARIANT_TEMPLATE_5_TYPE   — enables a fifth alternative
//   _ZP_VARIANT_TEMPLATE_6_TYPE   — enables a sixth alternative
//   _ZP_VARIANT_TEMPLATE_7_TYPE   — enables a seventh alternative
//   _ZP_VARIANT_TEMPLATE_8_TYPE   — enables an eighth alternative
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
// Optional suppression:
//   _ZP_VARIANT_TEMPLATE_NO_MOVE_FN
//       When defined, the NAME_move(dst, src) function is NOT generated.
//       Useful when the variant is only ever constructed/destroyed in-place and
//       the move helper would create an unused-function warning.
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
//   NAME_none_t   — alias for void (the empty/uninitialised state holds no value)
//   NAME_1N_t     — alias for 1_TYPE   — only if 1_TYPE defined
//   NAME_2N_t     — alias for 2_TYPE   — only if 2_TYPE defined
//   NAME_3N_t     — alias for 3_TYPE   — only if 3_TYPE defined
//   NAME_4N_t     — alias for 4_TYPE   — only if 4_TYPE defined
//   NAME_5N_t     — alias for 5_TYPE   — only if 5_TYPE defined
//   NAME_6N_t     — alias for 6_TYPE   — only if 6_TYPE defined
//   NAME_7N_t     — alias for 7_TYPE   — only if 7_TYPE defined
//   NAME_8N_t     — alias for 8_TYPE   — only if 8_TYPE defined
//
// Construction / destruction:
//   NAME_t  NAME_none(void)
//   NAME_t  NAME_from_1N(1_TYPE *val)   — only if 1_TYPE defined; moves *val in
//   NAME_t  NAME_from_2N(2_TYPE *val)   — only if 2_TYPE defined
//   NAME_t  NAME_from_3N(3_TYPE *val)   — only if 3_TYPE defined
//   NAME_t  NAME_from_4N(4_TYPE *val)   — only if 4_TYPE defined
//   NAME_t  NAME_from_5N(5_TYPE *val)   — only if 5_TYPE defined
//   NAME_t  NAME_from_6N(6_TYPE *val)   — only if 6_TYPE defined
//   NAME_t  NAME_from_7N(7_TYPE *val)   — only if 7_TYPE defined
//   NAME_t  NAME_from_8N(8_TYPE *val)   — only if 8_TYPE defined
//   void    NAME_destroy(NAME_t *v)
//
// Move (variant-to-variant)  — omitted when _ZP_VARIANT_TEMPLATE_NO_MOVE_FN is defined:
//   void    NAME_move(NAME_t *dst, NAME_t *src)  — dst must be uninitialized or safe to overwrite
//
// Inspection:
//   bool       NAME_is_none(const NAME_t *v)
//   bool       NAME_is_1N  (const NAME_t *v)   — only if 1_TYPE defined
//   bool       NAME_is_2N  (const NAME_t *v)   — only if 2_TYPE defined
//   bool       NAME_is_3N  (const NAME_t *v)   — only if 3_TYPE defined
//   bool       NAME_is_4N  (const NAME_t *v)   — only if 4_TYPE defined
//   bool       NAME_is_5N  (const NAME_t *v)   — only if 5_TYPE defined
//   bool       NAME_is_6N  (const NAME_t *v)   — only if 6_TYPE defined
//   bool       NAME_is_7N  (const NAME_t *v)   — only if 7_TYPE defined
//   bool       NAME_is_8N  (const NAME_t *v)   — only if 8_TYPE defined
//   NAME_tag_t NAME_tag    (const NAME_t *v)
//
// Non-consuming access (UB if wrong alternative):
//   1_TYPE *NAME_get_1N(NAME_t *v)   — only if 1_TYPE defined
//   2_TYPE *NAME_get_2N(NAME_t *v)   — only if 2_TYPE defined
//   3_TYPE *NAME_get_3N(NAME_t *v)   — only if 3_TYPE defined
//   4_TYPE *NAME_get_4N(NAME_t *v)   — only if 4_TYPE defined
//   5_TYPE *NAME_get_5N(NAME_t *v)   — only if 5_TYPE defined
//   6_TYPE *NAME_get_6N(NAME_t *v)   — only if 6_TYPE defined
//   7_TYPE *NAME_get_7N(NAME_t *v)   — only if 7_TYPE defined
//   8_TYPE *NAME_get_8N(NAME_t *v)   — only if 8_TYPE defined
//
// Non-consuming read-only access (UB if wrong alternative):
//   const 1_TYPE *NAME_const_get_1N(const NAME_t *v)   — only if 1_TYPE defined
//   const 2_TYPE *NAME_const_get_2N(const NAME_t *v)   — only if 2_TYPE defined
//   const 3_TYPE *NAME_const_get_3N(const NAME_t *v)   — only if 3_TYPE defined
//   const 4_TYPE *NAME_const_get_4N(const NAME_t *v)   — only if 4_TYPE defined
//   const 5_TYPE *NAME_const_get_5N(const NAME_t *v)   — only if 5_TYPE defined
//   const 6_TYPE *NAME_const_get_6N(const NAME_t *v)   — only if 6_TYPE defined
//   const 7_TYPE *NAME_const_get_7N(const NAME_t *v)   — only if 7_TYPE defined
//   const 8_TYPE *NAME_const_get_8N(const NAME_t *v)   — only if 8_TYPE defined
//
// Consuming access (moves value out, returns false if wrong alternative):
//   bool    NAME_take_1N(NAME_t *v, 1_TYPE *out)   — only if 1_TYPE defined
//   bool    NAME_take_2N(NAME_t *v, 2_TYPE *out)   — only if 2_TYPE defined
//   bool    NAME_take_3N(NAME_t *v, 3_TYPE *out)   — only if 3_TYPE defined
//   bool    NAME_take_4N(NAME_t *v, 4_TYPE *out)   — only if 4_TYPE defined
//   bool    NAME_take_5N(NAME_t *v, 5_TYPE *out)   — only if 5_TYPE defined
//   bool    NAME_take_6N(NAME_t *v, 6_TYPE *out)   — only if 6_TYPE defined
//   bool    NAME_take_7N(NAME_t *v, 7_TYPE *out)   — only if 7_TYPE defined
//   bool    NAME_take_8N(NAME_t *v, 8_TYPE *out)   — only if 8_TYPE defined
//

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/cat.h"

// ── Validate requirements ────────────────────────────────────────────────────
//
// Alternatives are independent: any subset of 1..8 may be defined (including
// sparse combinations such as 4 without 3).  At least one must be present.

#if !defined(_ZP_VARIANT_TEMPLATE_1_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_2_TYPE) && \
    !defined(_ZP_VARIANT_TEMPLATE_3_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_4_TYPE) && \
    !defined(_ZP_VARIANT_TEMPLATE_5_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_6_TYPE) && \
    !defined(_ZP_VARIANT_TEMPLATE_7_TYPE) && !defined(_ZP_VARIANT_TEMPLATE_8_TYPE)
#error "variant_template.h: at least one of _ZP_VARIANT_TEMPLATE_1_TYPE.._ZP_VARIANT_TEMPLATE_8_TYPE must be defined"
#define _ZP_VARIANT_TEMPLATE_1_TYPE int
#endif

// ── Required macros ──────────────────────────────────────────────────────────

#ifndef _ZP_VARIANT_TEMPLATE_NAME
#error "_ZP_VARIANT_TEMPLATE_NAME must be defined before including variant_template.h"
#define _ZP_VARIANT_TEMPLATE_NAME _zp_variant
#endif

// ── Optional names with defaults ─────────────────────────────────────────────

#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_1_NAME
#define _ZP_VARIANT_TEMPLATE_1_NAME 1
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_2_NAME
#define _ZP_VARIANT_TEMPLATE_2_NAME 2
#endif
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

#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_1_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_1_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_1_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_1_MOVE_FN(dst, src) *(dst) = *(src);
#endif
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
#ifndef _ZP_VARIANT_TEMPLATE_2_DESTROY_FN
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(ptr) (void)(ptr)
#endif
#ifndef _ZP_VARIANT_TEMPLATE_2_MOVE_FN
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(dst, src) *(dst) = *(src);
#endif
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

// ── Internal name helpers ─────────────────────────────────────────────────────
//
// All public symbols are derived from NAME and the user-supplied alternative
// names so that multiple instantiations in the same translation unit never
// collide.  Three-part names use two chained _ZP_CAT calls:
//   _ZP_CAT(NAME, _ZP_CAT(TAG, AN))  →  NAME_TAG_AN

#define _ZP_VARIANT_TEMPLATE_TYPE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, t)
#define _ZP_VARIANT_TEMPLATE_TAG_TYPE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, tag_t)

// Alternative type aliases (NAME_XN_t) and the empty-state alias (NAME_none_t)
#define _ZP_VARIANT_ALIAS_NONE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, none_t)
typedef void _ZP_VARIANT_ALIAS_NONE;
#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
#define _ZP_VARIANT_ALIAS_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(_ZP_VARIANT_TEMPLATE_1_NAME, t))
typedef _ZP_VARIANT_TEMPLATE_1_TYPE _ZP_VARIANT_ALIAS_1;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
#define _ZP_VARIANT_ALIAS_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(_ZP_VARIANT_TEMPLATE_2_NAME, t))
typedef _ZP_VARIANT_TEMPLATE_2_TYPE _ZP_VARIANT_ALIAS_2;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#define _ZP_VARIANT_ALIAS_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(_ZP_VARIANT_TEMPLATE_3_NAME, t))
typedef _ZP_VARIANT_TEMPLATE_3_TYPE _ZP_VARIANT_ALIAS_3;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#define _ZP_VARIANT_ALIAS_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(_ZP_VARIANT_TEMPLATE_4_NAME, t))
typedef _ZP_VARIANT_TEMPLATE_4_TYPE _ZP_VARIANT_ALIAS_4;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
#define _ZP_VARIANT_ALIAS_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(_ZP_VARIANT_TEMPLATE_5_NAME, t))
typedef _ZP_VARIANT_TEMPLATE_5_TYPE _ZP_VARIANT_ALIAS_5;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
#define _ZP_VARIANT_ALIAS_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(_ZP_VARIANT_TEMPLATE_6_NAME, t))
typedef _ZP_VARIANT_TEMPLATE_6_TYPE _ZP_VARIANT_ALIAS_6;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
#define _ZP_VARIANT_ALIAS_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(_ZP_VARIANT_TEMPLATE_7_NAME, t))
typedef _ZP_VARIANT_TEMPLATE_7_TYPE _ZP_VARIANT_ALIAS_7;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#define _ZP_VARIANT_ALIAS_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(_ZP_VARIANT_TEMPLATE_8_NAME, t))
typedef _ZP_VARIANT_TEMPLATE_8_TYPE _ZP_VARIANT_ALIAS_8;
#endif

// Tag enum values
#define _ZP_VARIANT_TAG_NONE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, tag_none)
#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
#define _ZP_VARIANT_TAG_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_1_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
#define _ZP_VARIANT_TAG_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(tag, _ZP_VARIANT_TEMPLATE_2_NAME))
#endif
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
#define _ZP_VARIANT_FN_GET_NONE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, get_none)
#define _ZP_VARIANT_FN_CONST_GET_NONE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, const_get_none)

#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
#define _ZP_VARIANT_FN_FROM_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_FN_IS_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_FN_GET_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_FN_CONST_GET_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(const_get, _ZP_VARIANT_TEMPLATE_1_NAME))
#define _ZP_VARIANT_FN_TAKE_1 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_1_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
#define _ZP_VARIANT_FN_FROM_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_2_NAME))
#define _ZP_VARIANT_FN_IS_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_2_NAME))
#define _ZP_VARIANT_FN_GET_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_2_NAME))
#define _ZP_VARIANT_FN_CONST_GET_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(const_get, _ZP_VARIANT_TEMPLATE_2_NAME))
#define _ZP_VARIANT_FN_TAKE_2 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_2_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#define _ZP_VARIANT_FN_FROM_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_3_NAME))
#define _ZP_VARIANT_FN_IS_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_3_NAME))
#define _ZP_VARIANT_FN_GET_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_3_NAME))
#define _ZP_VARIANT_FN_CONST_GET_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(const_get, _ZP_VARIANT_TEMPLATE_3_NAME))
#define _ZP_VARIANT_FN_TAKE_3 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_3_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#define _ZP_VARIANT_FN_FROM_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_4_NAME))
#define _ZP_VARIANT_FN_IS_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_4_NAME))
#define _ZP_VARIANT_FN_GET_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_4_NAME))
#define _ZP_VARIANT_FN_CONST_GET_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(const_get, _ZP_VARIANT_TEMPLATE_4_NAME))
#define _ZP_VARIANT_FN_TAKE_4 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_4_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
#define _ZP_VARIANT_FN_FROM_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_5_NAME))
#define _ZP_VARIANT_FN_IS_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_5_NAME))
#define _ZP_VARIANT_FN_GET_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_5_NAME))
#define _ZP_VARIANT_FN_CONST_GET_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(const_get, _ZP_VARIANT_TEMPLATE_5_NAME))
#define _ZP_VARIANT_FN_TAKE_5 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_5_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
#define _ZP_VARIANT_FN_FROM_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_6_NAME))
#define _ZP_VARIANT_FN_IS_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_6_NAME))
#define _ZP_VARIANT_FN_GET_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_6_NAME))
#define _ZP_VARIANT_FN_CONST_GET_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(const_get, _ZP_VARIANT_TEMPLATE_6_NAME))
#define _ZP_VARIANT_FN_TAKE_6 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_6_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
#define _ZP_VARIANT_FN_FROM_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_7_NAME))
#define _ZP_VARIANT_FN_IS_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_7_NAME))
#define _ZP_VARIANT_FN_GET_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_7_NAME))
#define _ZP_VARIANT_FN_CONST_GET_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(const_get, _ZP_VARIANT_TEMPLATE_7_NAME))
#define _ZP_VARIANT_FN_TAKE_7 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_7_NAME))
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#define _ZP_VARIANT_FN_FROM_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(from, _ZP_VARIANT_TEMPLATE_8_NAME))
#define _ZP_VARIANT_FN_IS_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(is, _ZP_VARIANT_TEMPLATE_8_NAME))
#define _ZP_VARIANT_FN_GET_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(get, _ZP_VARIANT_TEMPLATE_8_NAME))
#define _ZP_VARIANT_FN_CONST_GET_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(const_get, _ZP_VARIANT_TEMPLATE_8_NAME))
#define _ZP_VARIANT_FN_TAKE_8 _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, _ZP_CAT(take, _ZP_VARIANT_TEMPLATE_8_NAME))
#endif

// Union member names
//
// Each active member is named "_" followed by the alternative's NAME, e.g.
// alternative 1 with NAME=path yields the member "_path".  With the default
// numeric names this reproduces the original "_1", "_2", ... members.
// Two levels of indirection are required so the NAME argument is fully expanded
// before being pasted onto the leading underscore.
#define _ZP_VARIANT_MEMBER_INNER(name) _##name
#define _ZP_VARIANT_MEMBER(name) _ZP_VARIANT_MEMBER_INNER(name)
#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
#define _ZP_VARIANT_MEMBER_1 _ZP_VARIANT_MEMBER(_ZP_VARIANT_TEMPLATE_1_NAME)
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
#define _ZP_VARIANT_MEMBER_2 _ZP_VARIANT_MEMBER(_ZP_VARIANT_TEMPLATE_2_NAME)
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
#define _ZP_VARIANT_MEMBER_3 _ZP_VARIANT_MEMBER(_ZP_VARIANT_TEMPLATE_3_NAME)
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
#define _ZP_VARIANT_MEMBER_4 _ZP_VARIANT_MEMBER(_ZP_VARIANT_TEMPLATE_4_NAME)
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
#define _ZP_VARIANT_MEMBER_5 _ZP_VARIANT_MEMBER(_ZP_VARIANT_TEMPLATE_5_NAME)
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
#define _ZP_VARIANT_MEMBER_6 _ZP_VARIANT_MEMBER(_ZP_VARIANT_TEMPLATE_6_NAME)
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
#define _ZP_VARIANT_MEMBER_7 _ZP_VARIANT_MEMBER(_ZP_VARIANT_TEMPLATE_7_NAME)
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
#define _ZP_VARIANT_MEMBER_8 _ZP_VARIANT_MEMBER(_ZP_VARIANT_TEMPLATE_8_NAME)
#endif

// ── Tag enum ──────────────────────────────────────────────────────────────────
//
// TAG_NONE (= 0) is the zero-initialised / moved-from / empty state.

typedef enum _ZP_VARIANT_TEMPLATE_TAG_TYPE {
    _ZP_VARIANT_TAG_NONE = 0,
#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
    _ZP_VARIANT_TAG_1 = 1,
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
    _ZP_VARIANT_TAG_2 = 2,
#endif
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

typedef struct _ZP_VARIANT_TEMPLATE_TYPE {
    _ZP_VARIANT_TEMPLATE_TAG_TYPE _tag;
    union {
#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
        _ZP_VARIANT_TEMPLATE_1_TYPE _ZP_VARIANT_MEMBER_1;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
        _ZP_VARIANT_TEMPLATE_2_TYPE _ZP_VARIANT_MEMBER_2;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
        _ZP_VARIANT_TEMPLATE_3_TYPE _ZP_VARIANT_MEMBER_3;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
        _ZP_VARIANT_TEMPLATE_4_TYPE _ZP_VARIANT_MEMBER_4;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
        _ZP_VARIANT_TEMPLATE_5_TYPE _ZP_VARIANT_MEMBER_5;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
        _ZP_VARIANT_TEMPLATE_6_TYPE _ZP_VARIANT_MEMBER_6;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
        _ZP_VARIANT_TEMPLATE_7_TYPE _ZP_VARIANT_MEMBER_7;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
        _ZP_VARIANT_TEMPLATE_8_TYPE _ZP_VARIANT_MEMBER_8;
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

#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_1(_ZP_VARIANT_TEMPLATE_1_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_1;
    _ZP_VARIANT_TEMPLATE_1_MOVE_FN(&v._ZP_VARIANT_MEMBER_1, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_2(_ZP_VARIANT_TEMPLATE_2_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_2;
    _ZP_VARIANT_TEMPLATE_2_MOVE_FN(&v._ZP_VARIANT_MEMBER_2, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_3(_ZP_VARIANT_TEMPLATE_3_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_3;
    _ZP_VARIANT_TEMPLATE_3_MOVE_FN(&v._ZP_VARIANT_MEMBER_3, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_4(_ZP_VARIANT_TEMPLATE_4_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_4;
    _ZP_VARIANT_TEMPLATE_4_MOVE_FN(&v._ZP_VARIANT_MEMBER_4, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_5(_ZP_VARIANT_TEMPLATE_5_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_5;
    _ZP_VARIANT_TEMPLATE_5_MOVE_FN(&v._ZP_VARIANT_MEMBER_5, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_6(_ZP_VARIANT_TEMPLATE_6_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_6;
    _ZP_VARIANT_TEMPLATE_6_MOVE_FN(&v._ZP_VARIANT_MEMBER_6, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_7(_ZP_VARIANT_TEMPLATE_7_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_7;
    _ZP_VARIANT_TEMPLATE_7_MOVE_FN(&v._ZP_VARIANT_MEMBER_7, val);
    return v;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
static inline _ZP_VARIANT_TEMPLATE_TYPE _ZP_VARIANT_FN_FROM_8(_ZP_VARIANT_TEMPLATE_8_TYPE *val) {
    _ZP_VARIANT_TEMPLATE_TYPE v;
    memset(&v, 0, sizeof(v));
    v._tag = _ZP_VARIANT_TAG_8;
    _ZP_VARIANT_TEMPLATE_8_MOVE_FN(&v._ZP_VARIANT_MEMBER_8, val);
    return v;
}
#endif

// ── NAME_destroy ──────────────────────────────────────────────────────────────

static inline void _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, destroy)(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    switch (v->_tag) {
#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
        case _ZP_VARIANT_TAG_1:
            _ZP_VARIANT_TEMPLATE_1_DESTROY_FN(&v->_ZP_VARIANT_MEMBER_1);
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
        case _ZP_VARIANT_TAG_2:
            _ZP_VARIANT_TEMPLATE_2_DESTROY_FN(&v->_ZP_VARIANT_MEMBER_2);
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
        case _ZP_VARIANT_TAG_3:
            _ZP_VARIANT_TEMPLATE_3_DESTROY_FN(&v->_ZP_VARIANT_MEMBER_3);
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
        case _ZP_VARIANT_TAG_4:
            _ZP_VARIANT_TEMPLATE_4_DESTROY_FN(&v->_ZP_VARIANT_MEMBER_4);
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
        case _ZP_VARIANT_TAG_5:
            _ZP_VARIANT_TEMPLATE_5_DESTROY_FN(&v->_ZP_VARIANT_MEMBER_5);
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
        case _ZP_VARIANT_TAG_6:
            _ZP_VARIANT_TEMPLATE_6_DESTROY_FN(&v->_ZP_VARIANT_MEMBER_6);
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
        case _ZP_VARIANT_TAG_7:
            _ZP_VARIANT_TEMPLATE_7_DESTROY_FN(&v->_ZP_VARIANT_MEMBER_7);
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
        case _ZP_VARIANT_TAG_8:
            _ZP_VARIANT_TEMPLATE_8_DESTROY_FN(&v->_ZP_VARIANT_MEMBER_8);
            break;
#endif
        default:
            break;
    }
    v->_tag = _ZP_VARIANT_TAG_NONE;
}

// ── NAME_move ─────────────────────────────────────────────────────────────────
// Moves *src into *dst.  *dst must be uninitialized or otherwise safe to overwrite;
// previous contents of *dst are not destroyed.
// *src is left in NONE state.
// Generated only when _ZP_VARIANT_TEMPLATE_NO_MOVE_FN is NOT defined.

#ifndef _ZP_VARIANT_TEMPLATE_NO_MOVE_FN
static inline void _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, move)(_ZP_VARIANT_TEMPLATE_TYPE *dst,
                                                            _ZP_VARIANT_TEMPLATE_TYPE *src) {
    switch (src->_tag) {
#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
        case _ZP_VARIANT_TAG_1:
            _ZP_VARIANT_TEMPLATE_1_MOVE_FN(&dst->_ZP_VARIANT_MEMBER_1, &src->_ZP_VARIANT_MEMBER_1);
            dst->_tag = _ZP_VARIANT_TAG_1;
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
        case _ZP_VARIANT_TAG_2:
            _ZP_VARIANT_TEMPLATE_2_MOVE_FN(&dst->_ZP_VARIANT_MEMBER_2, &src->_ZP_VARIANT_MEMBER_2);
            dst->_tag = _ZP_VARIANT_TAG_2;
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
        case _ZP_VARIANT_TAG_3:
            _ZP_VARIANT_TEMPLATE_3_MOVE_FN(&dst->_ZP_VARIANT_MEMBER_3, &src->_ZP_VARIANT_MEMBER_3);
            dst->_tag = _ZP_VARIANT_TAG_3;
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
        case _ZP_VARIANT_TAG_4:
            _ZP_VARIANT_TEMPLATE_4_MOVE_FN(&dst->_ZP_VARIANT_MEMBER_4, &src->_ZP_VARIANT_MEMBER_4);
            dst->_tag = _ZP_VARIANT_TAG_4;
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
        case _ZP_VARIANT_TAG_5:
            _ZP_VARIANT_TEMPLATE_5_MOVE_FN(&dst->_ZP_VARIANT_MEMBER_5, &src->_ZP_VARIANT_MEMBER_5);
            dst->_tag = _ZP_VARIANT_TAG_5;
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
        case _ZP_VARIANT_TAG_6:
            _ZP_VARIANT_TEMPLATE_6_MOVE_FN(&dst->_ZP_VARIANT_MEMBER_6, &src->_ZP_VARIANT_MEMBER_6);
            dst->_tag = _ZP_VARIANT_TAG_6;
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
        case _ZP_VARIANT_TAG_7:
            _ZP_VARIANT_TEMPLATE_7_MOVE_FN(&dst->_ZP_VARIANT_MEMBER_7, &src->_ZP_VARIANT_MEMBER_7);
            dst->_tag = _ZP_VARIANT_TAG_7;
            break;
#endif
#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
        case _ZP_VARIANT_TAG_8:
            _ZP_VARIANT_TEMPLATE_8_MOVE_FN(&dst->_ZP_VARIANT_MEMBER_8, &src->_ZP_VARIANT_MEMBER_8);
            dst->_tag = _ZP_VARIANT_TAG_8;
            break;
#endif
        default:
            dst->_tag = _ZP_VARIANT_TAG_NONE;
            break;
    }
    src->_tag = _ZP_VARIANT_TAG_NONE;
}
#endif  // _ZP_VARIANT_TEMPLATE_NO_MOVE_FN

// ── NAME_tag / NAME_is_none / NAME_is_XN ─────────────────────────────────────

static inline _ZP_VARIANT_TEMPLATE_TAG_TYPE _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME,
                                                    tag)(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return v->_tag;
}

static inline bool _ZP_CAT(_ZP_VARIANT_TEMPLATE_NAME, is_none)(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return v->_tag == _ZP_VARIANT_TAG_NONE;
}

#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
static inline bool _ZP_VARIANT_FN_IS_1(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_1; }
#endif

#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
static inline bool _ZP_VARIANT_FN_IS_2(const _ZP_VARIANT_TEMPLATE_TYPE *v) { return v->_tag == _ZP_VARIANT_TAG_2; }
#endif

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

// For the NONE alternative, returns NULL instead of a pointer to a non-existent member.
static inline void *_ZP_VARIANT_FN_GET_NONE(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    (void)v;
    return NULL;
}

#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
static inline _ZP_VARIANT_TEMPLATE_1_TYPE *_ZP_VARIANT_FN_GET_1(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_1;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
static inline _ZP_VARIANT_TEMPLATE_2_TYPE *_ZP_VARIANT_FN_GET_2(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_2;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline _ZP_VARIANT_TEMPLATE_3_TYPE *_ZP_VARIANT_FN_GET_3(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_3;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline _ZP_VARIANT_TEMPLATE_4_TYPE *_ZP_VARIANT_FN_GET_4(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_4;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
static inline _ZP_VARIANT_TEMPLATE_5_TYPE *_ZP_VARIANT_FN_GET_5(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_5;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
static inline _ZP_VARIANT_TEMPLATE_6_TYPE *_ZP_VARIANT_FN_GET_6(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_6;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
static inline _ZP_VARIANT_TEMPLATE_7_TYPE *_ZP_VARIANT_FN_GET_7(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_7;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
static inline _ZP_VARIANT_TEMPLATE_8_TYPE *_ZP_VARIANT_FN_GET_8(_ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_8;
}
#endif

// ── NAME_const_get_XN ─────────────────────────────────────────────────────────
// Returns a const pointer to the active member.
// UB if the variant does not hold the requested alternative.

static inline const void *_ZP_VARIANT_FN_CONST_GET_NONE(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    (void)v;
    return NULL;
}

#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
static inline const _ZP_VARIANT_TEMPLATE_1_TYPE *_ZP_VARIANT_FN_CONST_GET_1(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_1;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
static inline const _ZP_VARIANT_TEMPLATE_2_TYPE *_ZP_VARIANT_FN_CONST_GET_2(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_2;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline const _ZP_VARIANT_TEMPLATE_3_TYPE *_ZP_VARIANT_FN_CONST_GET_3(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_3;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline const _ZP_VARIANT_TEMPLATE_4_TYPE *_ZP_VARIANT_FN_CONST_GET_4(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_4;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
static inline const _ZP_VARIANT_TEMPLATE_5_TYPE *_ZP_VARIANT_FN_CONST_GET_5(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_5;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
static inline const _ZP_VARIANT_TEMPLATE_6_TYPE *_ZP_VARIANT_FN_CONST_GET_6(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_6;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
static inline const _ZP_VARIANT_TEMPLATE_7_TYPE *_ZP_VARIANT_FN_CONST_GET_7(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_7;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
static inline const _ZP_VARIANT_TEMPLATE_8_TYPE *_ZP_VARIANT_FN_CONST_GET_8(const _ZP_VARIANT_TEMPLATE_TYPE *v) {
    return &v->_ZP_VARIANT_MEMBER_8;
}
#endif

// ── NAME_take_XN ─────────────────────────────────────────────────────────────
// Moves the held value out into *out and resets the variant to NONE.
// Returns true on success; returns false (leaving *v and *out untouched) if
// the variant does not currently hold the requested alternative.

#ifdef _ZP_VARIANT_TEMPLATE_1_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_1(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_1_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_1) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_1_MOVE_FN(out, &v->_ZP_VARIANT_MEMBER_1);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_2_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_2(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_2_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_2) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_2_MOVE_FN(out, &v->_ZP_VARIANT_MEMBER_2);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_3_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_3(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_3_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_3) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_3_MOVE_FN(out, &v->_ZP_VARIANT_MEMBER_3);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_4_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_4(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_4_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_4) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_4_MOVE_FN(out, &v->_ZP_VARIANT_MEMBER_4);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_5_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_5(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_5_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_5) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_5_MOVE_FN(out, &v->_ZP_VARIANT_MEMBER_5);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_6_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_6(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_6_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_6) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_6_MOVE_FN(out, &v->_ZP_VARIANT_MEMBER_6);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_7_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_7(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_7_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_7) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_7_MOVE_FN(out, &v->_ZP_VARIANT_MEMBER_7);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifdef _ZP_VARIANT_TEMPLATE_8_TYPE
static inline bool _ZP_VARIANT_FN_TAKE_8(_ZP_VARIANT_TEMPLATE_TYPE *v, _ZP_VARIANT_TEMPLATE_8_TYPE *out) {
    if (v->_tag != _ZP_VARIANT_TAG_8) {
        return false;
    }
    _ZP_VARIANT_TEMPLATE_8_MOVE_FN(out, &v->_ZP_VARIANT_MEMBER_8);
    v->_tag = _ZP_VARIANT_TAG_NONE;
    return true;
}
#endif

#ifndef _ZP_VARIANT_VISIT

#define _ZP_VARIANT_INNER_GET_TAG(tag, f) tag
#define _ZP_VARIANT_INNER_GET_F(tag, f) f

// One switch arm: calls the visitor with a pointer into the active union member.
#define _ZP_VARIANT_INNER_VISIT_CASE(variant_name, v, tag_f)                         \
    case _ZP_CAT(variant_name, _ZP_CAT(tag, _ZP_VARIANT_INNER_GET_TAG tag_f)): {     \
        _ZP_CAT(_ZP_CAT(variant_name, _ZP_VARIANT_INNER_GET_TAG tag_f), t) *_ =      \
            _ZP_CAT(variant_name, _ZP_CAT(get, _ZP_VARIANT_INNER_GET_TAG tag_f)(v)); \
        (void)_;                                                                     \
        _ZP_VARIANT_INNER_GET_F tag_f;                                               \
        break;                                                                       \
    }
// Const counterpart of _ZP_VARIANT_INNER_VISIT_CASE: binds `_` to a read-only
// pointer into the active union member (obtained via NAME_const_get_XN), so it
// can be used on a `const variant_t *`.
#define _ZP_VARIANT_INNER_CONST_VISIT_CASE(variant_name, v, tag_f)                         \
    case _ZP_CAT(variant_name, _ZP_CAT(tag, _ZP_VARIANT_INNER_GET_TAG tag_f)): {           \
        const _ZP_CAT(_ZP_CAT(variant_name, _ZP_VARIANT_INNER_GET_TAG tag_f), t) *_ =      \
            _ZP_CAT(variant_name, _ZP_CAT(const_get, _ZP_VARIANT_INNER_GET_TAG tag_f)(v)); \
        (void)_;                                                                           \
        _ZP_VARIANT_INNER_GET_F tag_f;                                                     \
        break;                                                                             \
    }

// Arity-specific switch builders, shared between the mutable and const visitors.
// `case_macro` (either _ZP_VARIANT_INNER_VISIT_CASE or its const counterpart) is
// the per-arm builder; it is invoked once per alternative, so the switch
// skeletons are written exactly once.
#define _ZP_VARIANT_VISIT_IMPL_1(case_macro, variant_name, v, tag_f1) \
    switch ((v)->_tag) { case_macro(variant_name, v, tag_f1) }
#define _ZP_VARIANT_VISIT_IMPL_2(case_macro, variant_name, v, tag_f1, tag_f2) \
    switch ((v)->_tag) { case_macro(variant_name, v, tag_f1) case_macro(variant_name, v, tag_f2) }
#define _ZP_VARIANT_VISIT_IMPL_3(case_macro, variant_name, v, tag_f1, tag_f2, tag_f3)                               \
    switch ((v)->_tag) {                                                                                            \
        case_macro(variant_name, v, tag_f1) case_macro(variant_name, v, tag_f2) case_macro(variant_name, v, tag_f3) \
    }
#define _ZP_VARIANT_VISIT_IMPL_4(case_macro, variant_name, v, tag_f1, tag_f2, tag_f3, tag_f4)                       \
    switch ((v)->_tag) {                                                                                            \
        case_macro(variant_name, v, tag_f1) case_macro(variant_name, v, tag_f2) case_macro(variant_name, v, tag_f3) \
            case_macro(variant_name, v, tag_f4)                                                                     \
    }
#define _ZP_VARIANT_VISIT_IMPL_5(case_macro, variant_name, v, tag_f1, tag_f2, tag_f3, tag_f4, tag_f5)               \
    switch ((v)->_tag) {                                                                                            \
        case_macro(variant_name, v, tag_f1) case_macro(variant_name, v, tag_f2) case_macro(variant_name, v, tag_f3) \
            case_macro(variant_name, v, tag_f4) case_macro(variant_name, v, tag_f5)                                 \
    }
#define _ZP_VARIANT_VISIT_IMPL_6(case_macro, variant_name, v, tag_f1, tag_f2, tag_f3, tag_f4, tag_f5, tag_f6)       \
    switch ((v)->_tag) {                                                                                            \
        case_macro(variant_name, v, tag_f1) case_macro(variant_name, v, tag_f2) case_macro(variant_name, v, tag_f3) \
            case_macro(variant_name, v, tag_f4) case_macro(variant_name, v, tag_f5)                                 \
                case_macro(variant_name, v, tag_f6)                                                                 \
    }
#define _ZP_VARIANT_VISIT_IMPL_7(case_macro, variant_name, v, tag_f1, tag_f2, tag_f3, tag_f4, tag_f5, tag_f6, tag_f7) \
    switch ((v)->_tag) {                                                                                              \
        case_macro(variant_name, v, tag_f1) case_macro(variant_name, v, tag_f2) case_macro(variant_name, v, tag_f3)   \
            case_macro(variant_name, v, tag_f4) case_macro(variant_name, v, tag_f5)                                   \
                case_macro(variant_name, v, tag_f6) case_macro(variant_name, v, tag_f7)                               \
    }
#define _ZP_VARIANT_VISIT_IMPL_8(case_macro, variant_name, v, tag_f1, tag_f2, tag_f3, tag_f4, tag_f5, tag_f6, tag_f7, \
                                 tag_f8)                                                                              \
    switch ((v)->_tag) {                                                                                              \
        case_macro(variant_name, v, tag_f1) case_macro(variant_name, v, tag_f2) case_macro(variant_name, v, tag_f3)   \
            case_macro(variant_name, v, tag_f4) case_macro(variant_name, v, tag_f5)                                   \
                case_macro(variant_name, v, tag_f6) case_macro(variant_name, v, tag_f7)                               \
                    case_macro(variant_name, v, tag_f8)                                                               \
    }
#define _ZP_VARIANT_VISIT_IMPL_9(case_macro, variant_name, v, tag_f1, tag_f2, tag_f3, tag_f4, tag_f5, tag_f6, tag_f7, \
                                 tag_f8, tag_f9)                                                                      \
    switch ((v)->_tag) {                                                                                              \
        case_macro(variant_name, v, tag_f1) case_macro(variant_name, v, tag_f2) case_macro(variant_name, v, tag_f3)   \
            case_macro(variant_name, v, tag_f4) case_macro(variant_name, v, tag_f5)                                   \
                case_macro(variant_name, v, tag_f6) case_macro(variant_name, v, tag_f7)                               \
                    case_macro(variant_name, v, tag_f8) case_macro(variant_name, v, tag_f9)                           \
    }

// Public entry points: pick the arity-specific builder, then plug in the
// mutable or read-only arm macro.  Both share the same switch skeletons above.
// The outer _ZP_EXPAND forces an extra rescan so the traditional MSVC
// preprocessor splits the forwarded __VA_ARGS__ into separate arguments instead
// of passing it as a single token to the selected _ZP_VARIANT_VISIT_IMPL_N.
#define _ZP_VARIANT_VISIT(variant_name, v, ...)                                                                       \
    _ZP_EXPAND(_ZP_CAT(_ZP_VARIANT_VISIT_IMPL, _ZP_VA_NARGS(__VA_ARGS__))(_ZP_VARIANT_INNER_VISIT_CASE, variant_name, \
                                                                          v, __VA_ARGS__))
#define _ZP_VARIANT_CONST_VISIT(variant_name, v, ...)                                                         \
    _ZP_EXPAND(_ZP_CAT(_ZP_VARIANT_VISIT_IMPL, _ZP_VA_NARGS(__VA_ARGS__))(_ZP_VARIANT_INNER_CONST_VISIT_CASE, \
                                                                          variant_name, v, __VA_ARGS__))
#endif

// ── Undef all macros ──────────────────────────────────────────────────────────
#undef _ZP_VARIANT_TEMPLATE_NAME
#undef _ZP_VARIANT_TEMPLATE_NO_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_1_TYPE
#undef _ZP_VARIANT_TEMPLATE_2_TYPE
#undef _ZP_VARIANT_TEMPLATE_3_TYPE
#undef _ZP_VARIANT_TEMPLATE_4_TYPE
#undef _ZP_VARIANT_TEMPLATE_5_TYPE
#undef _ZP_VARIANT_TEMPLATE_6_TYPE
#undef _ZP_VARIANT_TEMPLATE_7_TYPE
#undef _ZP_VARIANT_TEMPLATE_8_TYPE
#undef _ZP_VARIANT_TEMPLATE_1_NAME
#undef _ZP_VARIANT_TEMPLATE_2_NAME
#undef _ZP_VARIANT_TEMPLATE_3_NAME
#undef _ZP_VARIANT_TEMPLATE_4_NAME
#undef _ZP_VARIANT_TEMPLATE_5_NAME
#undef _ZP_VARIANT_TEMPLATE_6_NAME
#undef _ZP_VARIANT_TEMPLATE_7_NAME
#undef _ZP_VARIANT_TEMPLATE_8_NAME
#undef _ZP_VARIANT_TEMPLATE_1_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_2_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_3_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_4_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_5_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_6_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_7_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_8_DESTROY_FN
#undef _ZP_VARIANT_TEMPLATE_1_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_2_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_3_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_4_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_5_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_6_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_7_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_8_MOVE_FN
#undef _ZP_VARIANT_TEMPLATE_TYPE
#undef _ZP_VARIANT_TEMPLATE_TAG_TYPE
#undef _ZP_VARIANT_ALIAS_NONE
#undef _ZP_VARIANT_ALIAS_1
#undef _ZP_VARIANT_ALIAS_2
#undef _ZP_VARIANT_ALIAS_3
#undef _ZP_VARIANT_ALIAS_4
#undef _ZP_VARIANT_ALIAS_5
#undef _ZP_VARIANT_ALIAS_6
#undef _ZP_VARIANT_ALIAS_7
#undef _ZP_VARIANT_ALIAS_8
#undef _ZP_VARIANT_TAG_NONE
#undef _ZP_VARIANT_TAG_1
#undef _ZP_VARIANT_TAG_2
#undef _ZP_VARIANT_TAG_3
#undef _ZP_VARIANT_TAG_4
#undef _ZP_VARIANT_TAG_5
#undef _ZP_VARIANT_TAG_6
#undef _ZP_VARIANT_TAG_7
#undef _ZP_VARIANT_TAG_8
#undef _ZP_VARIANT_FN_FROM_1
#undef _ZP_VARIANT_FN_FROM_2
#undef _ZP_VARIANT_FN_FROM_3
#undef _ZP_VARIANT_FN_FROM_4
#undef _ZP_VARIANT_FN_FROM_5
#undef _ZP_VARIANT_FN_FROM_6
#undef _ZP_VARIANT_FN_FROM_7
#undef _ZP_VARIANT_FN_FROM_8
#undef _ZP_VARIANT_FN_IS_1
#undef _ZP_VARIANT_FN_IS_2
#undef _ZP_VARIANT_FN_IS_3
#undef _ZP_VARIANT_FN_IS_4
#undef _ZP_VARIANT_FN_IS_5
#undef _ZP_VARIANT_FN_IS_6
#undef _ZP_VARIANT_FN_IS_7
#undef _ZP_VARIANT_FN_IS_8
#undef _ZP_VARIANT_FN_GET_NONE
#undef _ZP_VARIANT_FN_GET_1
#undef _ZP_VARIANT_FN_GET_2
#undef _ZP_VARIANT_FN_GET_3
#undef _ZP_VARIANT_FN_GET_4
#undef _ZP_VARIANT_FN_GET_5
#undef _ZP_VARIANT_FN_GET_6
#undef _ZP_VARIANT_FN_GET_7
#undef _ZP_VARIANT_FN_GET_8
#undef _ZP_VARIANT_FN_CONST_GET_NONE
#undef _ZP_VARIANT_FN_CONST_GET_1
#undef _ZP_VARIANT_FN_CONST_GET_2
#undef _ZP_VARIANT_FN_CONST_GET_3
#undef _ZP_VARIANT_FN_CONST_GET_4
#undef _ZP_VARIANT_FN_CONST_GET_5
#undef _ZP_VARIANT_FN_CONST_GET_6
#undef _ZP_VARIANT_FN_CONST_GET_7
#undef _ZP_VARIANT_FN_CONST_GET_8
#undef _ZP_VARIANT_FN_TAKE_1
#undef _ZP_VARIANT_FN_TAKE_2
#undef _ZP_VARIANT_FN_TAKE_3
#undef _ZP_VARIANT_FN_TAKE_4
#undef _ZP_VARIANT_FN_TAKE_5
#undef _ZP_VARIANT_FN_TAKE_6
#undef _ZP_VARIANT_FN_TAKE_7
#undef _ZP_VARIANT_FN_TAKE_8
#undef _ZP_VARIANT_MEMBER_INNER
#undef _ZP_VARIANT_MEMBER
#undef _ZP_VARIANT_MEMBER_1
#undef _ZP_VARIANT_MEMBER_2
#undef _ZP_VARIANT_MEMBER_3
#undef _ZP_VARIANT_MEMBER_4
#undef _ZP_VARIANT_MEMBER_5
#undef _ZP_VARIANT_MEMBER_6
#undef _ZP_VARIANT_MEMBER_7
#undef _ZP_VARIANT_MEMBER_8
