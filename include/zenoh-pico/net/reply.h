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

#ifndef ZENOH_PICO_REPLY_NETAPI_H
#define ZENOH_PICO_REPLY_NETAPI_H

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/refcount.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _z_reply_err_owned_t {
    _z_bytes_t payload;
    _z_encoding_t encoding;
} _z_reply_err_owned_t;

void _z_reply_err_owned_clear(_z_reply_err_owned_t *reply);
void _z_reply_err_owned_move(_z_reply_err_owned_t *dst, _z_reply_err_owned_t *src);
static inline _z_reply_err_owned_t _z_reply_err_owned_null(void) { return (_z_reply_err_owned_t){0}; }
z_result_t _z_reply_err_owned_copy(_z_reply_err_owned_t *dst, const _z_reply_err_owned_t *src);

// a non-owning view of fields of reply_err
typedef struct _z_reply_err_view_t {
    _z_reply_err_owned_t _inner;
} _z_reply_err_view_t;

static inline const _z_reply_err_owned_t *_z_reply_err_view_deref(const _z_reply_err_view_t *view) {
    return &view->_inner;
}

#define _ZP_VARIANT_TEMPLATE_NAME _z_reply_err
#define _ZP_VARIANT_TEMPLATE_1_TYPE _z_reply_err_owned_t
#define _ZP_VARIANT_TEMPLATE_1_NAME owned
#define _ZP_VARIANT_TEMPLATE_1_DESTROY_FN _z_reply_err_owned_clear
#define _ZP_VARIANT_TEMPLATE_1_MOVE_FN _z_reply_err_owned_move
#define _ZP_VARIANT_TEMPLATE_2_TYPE _z_reply_err_view_t
#define _ZP_VARIANT_TEMPLATE_2_NAME view
#define _ZP_VARIANT_TEMPLATE_NO_MOVE_FN
#include "zenoh-pico/collections/variant_template.h"

// move if src is owned, copy if view
z_result_t _z_reply_err_move_or_copy(_z_reply_err_t *dst, _z_reply_err_t *src);
void _z_reply_err_create_view_from_data(_z_reply_err_t *dst, const _z_bytes_t *opt_payload,
                                        const _z_encoding_t *opt_encoding);
z_result_t _z_reply_err_copy(_z_reply_err_t *dst, const _z_reply_err_t *src);
static inline void _z_reply_err_clear(_z_reply_err_t *s) { _z_reply_err_destroy(s); }

static inline const _z_reply_err_owned_t *_z_reply_err_get_ref(const _z_reply_err_t *s) {
    _ZP_VARIANT_CONST_VISIT(_z_reply_err, s,
        (owned, return _),
        (view, return _z_reply_err_view_deref(_)),
        (none, return NULL)
    );
    return NULL;  // Unreachable, but avoids compiler warning.
}

static inline _z_reply_err_t _z_reply_err_null(void) { return _z_reply_err_none(); }
static inline bool _z_reply_err_check(const _z_reply_err_t *s) { return !_z_reply_err_is_none(s); }

#define _ZP_VARIANT_TEMPLATE_NAME _z_reply_data
#define _ZP_VARIANT_TEMPLATE_1_TYPE _z_sample_t
#define _ZP_VARIANT_TEMPLATE_1_NAME ok
#define _ZP_VARIANT_TEMPLATE_1_DESTROY_FN _z_sample_clear
#define _ZP_VARIANT_TEMPLATE_1_MOVE_FN(dst, src) (*dst = *src, *src = _z_sample_none())
#define _ZP_VARIANT_TEMPLATE_2_TYPE _z_reply_err_t
#define _ZP_VARIANT_TEMPLATE_2_NAME err
#define _ZP_VARIANT_TEMPLATE_2_DESTROY_FN _z_reply_err_clear
#define _ZP_VARIANT_TEMPLATE_2_MOVE_FN(dst, src) (*dst = *src, *src = _z_reply_err_none())
#define _ZP_VARIANT_TEMPLATE_NO_MOVE_FN
#include "zenoh-pico/collections/variant_template.h"

typedef struct _z_reply_t {
    _z_reply_data_t _result;
    _z_entity_global_id_t replier_id;
} _z_reply_t;

static inline _z_reply_t _z_reply_null(void) {
    _z_reply_t r;
    r._result = _z_reply_data_none();
    r.replier_id = _z_entity_global_id_null();
    return r;
}

z_result_t _z_reply_move_or_copy(_z_reply_t *dst, _z_reply_t *src);
void _z_reply_clear(_z_reply_t *src);
z_result_t _z_reply_copy(_z_reply_t *dst, const _z_reply_t *src);

static inline void _z_reply_create_ok_view_from_data(_z_reply_t *dst, const _z_keyexpr_t *keyexpr,
                                                     const _z_bytes_t *payload, const _z_timestamp_t *timestamp,
                                                     const _z_encoding_t *encoding, z_sample_kind_t kind,
                                                     const _z_bytes_t *attachment, const _z_source_info_t *source_info,
                                                     const _z_entity_global_id_t *replier_id) {
    dst->replier_id = replier_id == NULL ? _z_entity_global_id_null() : *replier_id;
    dst->_result._tag = _z_reply_data_tag_ok;
    _z_sample_create_view_from_data(&dst->_result._ok, keyexpr, payload, timestamp, encoding, kind, _Z_N_QOS_DEFAULT,
                                    attachment, source_info, Z_RELIABILITY_DEFAULT);
}

static inline void _z_reply_create_err_view_from_data(_z_reply_t *dst, const _z_bytes_t *payload,
                                                      const _z_encoding_t *encoding,
                                                      const _z_entity_global_id_t *replier_id) {
    dst->replier_id = replier_id == NULL ? _z_entity_global_id_null() : *replier_id;
    dst->_result._tag = _z_reply_data_tag_err;
    _z_reply_err_create_view_from_data(&dst->_result._err, payload, encoding);
}

// used to generate a stub reply for monotonic consolidation
z_result_t _z_reply_create_ok_owned_with_keyexpr(_z_reply_t *dst, const _z_keyexpr_t *keyexpr);
typedef struct _z_pending_reply_t {
    _z_reply_t _reply;
    _z_timestamp_t _tstamp;
} _z_pending_reply_t;

bool _z_pending_reply_eq(const _z_pending_reply_t *one, const _z_pending_reply_t *two);
void _z_pending_reply_clear(_z_pending_reply_t *res);

_Z_ELEM_DEFINE(_z_pending_reply, _z_pending_reply_t, _z_noop_size, _z_pending_reply_clear, _z_noop_copy, _z_noop_move,
               _z_pending_reply_eq, _z_noop_cmp, _z_noop_hash)
_Z_SLIST_DEFINE(_z_pending_reply, _z_pending_reply_t, false)

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_REPLY_NETAPI_H */
