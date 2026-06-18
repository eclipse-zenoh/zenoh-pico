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

#include "zenoh-pico/net/sample.h"

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

void _z_sample_create_view_from_data(_z_sample_t *dst, const _z_keyexpr_t *keyexpr, const _z_bytes_t *payload,
                                     const _z_timestamp_t *timestamp, const _z_encoding_t *encoding,
                                     z_sample_kind_t kind, _z_qos_t qos, const _z_bytes_t *attachment,
                                     const _z_source_info_t *source_info, z_reliability_t reliability) {
    dst->_view._inner.keyexpr._declaration = _z_keyexpr_wire_declaration_rc_null();
    dst->_view._inner.keyexpr._inner = *keyexpr;
    dst->_view._inner.payload = payload != NULL ? *payload : _z_bytes_null();
    dst->_view._inner.attachment = attachment != NULL ? *attachment : _z_bytes_null();
    dst->_view._inner.encoding = encoding != NULL ? *encoding : _z_encoding_null();
    dst->_view._inner.timestamp = timestamp != NULL ? *timestamp : _z_timestamp_null();
    dst->_view._inner.kind = kind;
    dst->_view._inner.qos = qos;
    dst->_view._inner.reliability = reliability;
    dst->_view._inner.source_info = source_info != NULL ? *source_info : _z_source_info_null();
    dst->_tag = _z_sample_tag_view;
}

void _z_sample_owned_move(_z_sample_owned_t *dst, _z_sample_owned_t *src) {
    *dst = *src;
    *src = _z_sample_owned_null();
}

void _z_sample_owned_clear(_z_sample_owned_t *sample) {
    _z_declared_keyexpr_clear(&sample->keyexpr);
    _z_encoding_clear(&sample->encoding);
    _z_bytes_clear(&sample->payload);
    _z_bytes_clear(&sample->attachment);
}

z_result_t _z_sample_owned_copy(_z_sample_owned_t *dst, const _z_sample_owned_t *src) {
    *dst = _z_sample_owned_null();
    _Z_RETURN_IF_ERR(_z_declared_keyexpr_copy(&dst->keyexpr, &src->keyexpr));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->payload, &src->payload), _z_sample_owned_clear(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_encoding_copy(&dst->encoding, &src->encoding), _z_sample_owned_clear(dst));
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_copy(&dst->attachment, &src->attachment), _z_sample_owned_clear(dst));
    dst->kind = src->kind;
    dst->timestamp = src->timestamp;
    dst->source_info = src->source_info;
    dst->qos = src->qos;
    dst->reliability = src->reliability;
    return _Z_RES_OK;
}

z_result_t _z_sample_move_or_copy(_z_sample_t *dst, _z_sample_t *src) {
    _ZP_VARIANT_VISIT(_z_sample, src,
        (owned, *dst = _z_sample_from_owned(_)),
        (view, {
            _z_sample_owned_t s = _z_sample_owned_null();
            z_result_t ret = _z_sample_owned_copy(&s, _z_sample_view_deref(_));
            if (ret != _Z_RES_OK) {
                *dst = _z_sample_none();
                return ret;
            }
            *dst = _z_sample_from_owned(&s);
        }),
        (none, *dst = _z_sample_none())
    );
    return _Z_RES_OK;
}

z_result_t _z_sample_copy(_z_sample_t *dst, const _z_sample_t *src) {
    const _z_sample_owned_t *ref = _z_sample_get_ref(src);
    if (ref != NULL) {
        _z_sample_owned_t s = _z_sample_owned_null();
        z_result_t ret = _z_sample_owned_copy(&s, ref);
        if (ret != _Z_RES_OK) {
            *dst = _z_sample_none();
            return ret;
        }
        *dst = _z_sample_from_owned(&s);
    } else {
        *dst = _z_sample_none();
    }
    return _Z_RES_OK;
}
