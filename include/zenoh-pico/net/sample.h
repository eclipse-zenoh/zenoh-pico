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

#ifndef ZENOH_PICO_SAMPLE_NETAPI_H
#define ZENOH_PICO_SAMPLE_NETAPI_H

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/ring.h"
#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A zenoh-net data sample.
 *
 * A sample is the value associated to a given resource at a given point in time.
 *
 * Members:
 *   _z_keyexpr_t key: The resource key of this data sample.
 *   _z_slice_t value: The value of this data sample.
 *   _z_encoding_t encoding: The encoding for the value of this data sample.
 *   _z_source_info_t source_info: The source info for this data sample (unstable).
 */
typedef struct _z_sample_owned_t {
    // FIXME: This should be a plain _z_keyexpr_t, but we need type compatibility with user's declared key expression,
    // via z_declare_keyexpr. So this keyexpr is always undeclared.
    _z_declared_keyexpr_t keyexpr;
    _z_bytes_t payload;
    _z_timestamp_t timestamp;
    _z_encoding_t encoding;
    z_sample_kind_t kind;
    _z_qos_t qos;
    _z_bytes_t attachment;
    z_reliability_t reliability;
    _z_source_info_t source_info;
} _z_sample_owned_t;

void _z_sample_owned_clear(_z_sample_owned_t *sample);
void _z_sample_owned_move(_z_sample_owned_t *dst, _z_sample_owned_t *src);
static inline _z_sample_owned_t _z_sample_owned_null(void) { return (_z_sample_owned_t){0}; }
z_result_t _z_sample_owned_copy(_z_sample_owned_t *dst, const _z_sample_owned_t *src);

// a non-owning view of fields of sample
typedef struct _z_sample_view_t {
    _z_sample_owned_t _inner;
} _z_sample_view_t;

static inline const _z_sample_owned_t *_z_sample_view_deref(const _z_sample_view_t *view) { return &view->_inner; }

#define _ZP_VARIANT_TEMPLATE_NAME _z_sample
#define _ZP_VARIANT_TEMPLATE_1_TYPE _z_sample_owned_t
#define _ZP_VARIANT_TEMPLATE_1_NAME owned
#define _ZP_VARIANT_TEMPLATE_1_DESTROY_FN _z_sample_owned_clear
#define _ZP_VARIANT_TEMPLATE_1_MOVE_FN _z_sample_owned_move
#define _ZP_VARIANT_TEMPLATE_2_TYPE _z_sample_view_t
#define _ZP_VARIANT_TEMPLATE_2_NAME view
#define _ZP_VARIANT_TEMPLATE_NO_MOVE_FN
#include "zenoh-pico/collections/variant_template.h"

// move if src is owned, copy if view
z_result_t _z_sample_move_or_copy(_z_sample_t *dst, _z_sample_t *src);

void _z_sample_create_view_from_data(_z_sample_t *dst, const _z_keyexpr_t *keyexpr, const _z_bytes_t *opt_payload,
                                     const _z_timestamp_t *opt_timestamp, const _z_encoding_t *opt_encoding,
                                     z_sample_kind_t kind, _z_qos_t qos, const _z_bytes_t *opt_attachment,
                                     const _z_source_info_t *opt_source_info, z_reliability_t reliability);

z_result_t _z_sample_copy(_z_sample_t *dst, const _z_sample_t *src);
static inline void _z_sample_clear(_z_sample_t *s) { _z_sample_destroy(s); }

static inline const _z_sample_owned_t *_z_sample_get_ref(const _z_sample_t *s) {
    _ZP_VARIANT_CONST_VISIT(_z_sample, s,
        (owned, return _),
        (view, return _z_sample_view_deref(_)),
        (none, return NULL)
    );
}

static inline _z_sample_t _z_sample_null(void) { return _z_sample_none(); }

static inline bool _z_sample_check(const _z_sample_t *s) { return !_z_sample_is_none(s); }

static inline size_t _z_sample_size(const _z_sample_t *s) {
    (void)(s);
    return sizeof(_z_sample_t);
}

_Z_ELEM_DEFINE(_z_sample, _z_sample_t, _z_sample_size, _z_sample_clear, _z_sample_copy, _z_sample_move_or_copy,
               _z_noop_eq, _z_noop_cmp, _z_noop_hash)
_Z_RING_DEFINE(_z_sample, _z_sample_t)

#ifdef __cplusplus
}
#endif
#endif /* ZENOH_PICO_SAMPLE_NETAPI_H */
