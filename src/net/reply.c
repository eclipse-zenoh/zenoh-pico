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

#include "zenoh-pico/net/reply.h"

#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_QUERY == 1

void _z_reply_err_owned_clear(_z_reply_err_owned_t *reply) {
    _z_bytes_clear(&reply->payload);
    _z_encoding_clear(&reply->encoding);
}

void _z_reply_err_owned_move(_z_reply_err_owned_t *dst, _z_reply_err_owned_t *src) {
    *dst = *src;
    *src = _z_reply_err_owned_null();
}

z_result_t _z_reply_err_owned_copy(_z_reply_err_owned_t *dst, const _z_reply_err_owned_t *src) {
    *dst = _z_reply_err_owned_null();
    _Z_RETURN_IF_ERR(_z_bytes_copy(&dst->payload, &src->payload));
    _Z_CLEAN_RETURN_IF_ERR(_z_encoding_copy(&dst->encoding, &src->encoding), _z_bytes_clear(&dst->payload));
    return _Z_RES_OK;
}

void _z_reply_err_create_view_from_data(_z_reply_err_t *dst, const _z_bytes_t *payload, const _z_encoding_t *encoding) {
    dst->_view._inner.payload = payload != NULL ? *payload : _z_bytes_null();
    dst->_view._inner.encoding = encoding != NULL ? *encoding : _z_encoding_null();
    dst->_tag = _z_reply_err_tag_view;
}

z_result_t _z_reply_err_move_or_copy(_z_reply_err_t *dst, _z_reply_err_t *src) {
    _ZP_VARIANT_VISIT(_z_reply_err, src,
        (owned, *dst = _z_reply_err_from_owned(_)),
        (view, {
            _z_reply_err_owned_t r = _z_reply_err_owned_null();
            z_result_t ret = _z_reply_err_owned_copy(&r, _z_reply_err_view_deref(_));
            if (ret != _Z_RES_OK) {
                *dst = _z_reply_err_none();
                return ret;
            }
            *dst = _z_reply_err_from_owned(&r);
        }),
        (none, *dst = _z_reply_err_none())
    );
    return _Z_RES_OK;
}

z_result_t _z_reply_err_copy(_z_reply_err_t *dst, const _z_reply_err_t *src) {
    const _z_reply_err_owned_t *ref = _z_reply_err_get_ref(src);
    if (ref != NULL) {
        _z_reply_err_owned_t s = _z_reply_err_owned_null();
        z_result_t ret = _z_reply_err_owned_copy(&s, ref);
        if (ret != _Z_RES_OK) {
            *dst = _z_reply_err_none();
            return ret;
        }
        *dst = _z_reply_err_from_owned(&s);
    } else {
        *dst = _z_reply_err_none();
    }
    return _Z_RES_OK;
}

z_result_t _z_reply_move_or_copy(_z_reply_t *dst, _z_reply_t *src) {
    dst->replier_id = src->replier_id;
    _ZP_VARIANT_VISIT(_z_reply_data, &src->_result,
        (ok, {
            _z_sample_t s = _z_sample_none();
            z_result_t ret = _z_sample_move_or_copy(&s, _);
            if (ret != _Z_RES_OK) {
                dst->_result = _z_reply_data_none();
                return ret;
            }
            dst->_result = _z_reply_data_from_ok(&s);
        }),
        (err, {
            _z_reply_err_t r = _z_reply_err_none();
            z_result_t ret = _z_reply_err_move_or_copy(&r, _);
            if (ret != _Z_RES_OK) {
                dst->_result = _z_reply_data_none();
                return ret;
            }
            dst->_result = _z_reply_data_from_err(&r);
        }),
        (none, dst->_result = _z_reply_data_none())
    );
    return _Z_RES_OK;
}

void _z_reply_clear(_z_reply_t *reply) {
    _z_reply_data_destroy(&reply->_result);
    reply->replier_id = _z_entity_global_id_null();
}

z_result_t _z_reply_copy(_z_reply_t *dst, const _z_reply_t *src) {
    dst->replier_id = src->replier_id;
    _ZP_VARIANT_CONST_VISIT(_z_reply_data, &src->_result,
        (ok, {
            _z_sample_t s = _z_sample_none();
            z_result_t ret = _z_sample_copy(&s, _);
            if (ret != _Z_RES_OK) {
                *dst = _z_reply_null();
                return ret;
            }
            dst->_result = _z_reply_data_from_ok(&s);
        }),
        (err, {
            _z_reply_err_t r = _z_reply_err_none();
            z_result_t ret = _z_reply_err_copy(&r, _);
            if (ret != _Z_RES_OK) {
                *dst = _z_reply_null();
                return ret;
            }
            dst->_result = _z_reply_data_from_err(&r);
        }),
        (none, *dst = _z_reply_null())
    );
    return _Z_RES_OK;
}

z_result_t _z_reply_create_ok_owned_with_keyexpr(_z_reply_t *dst, const _z_keyexpr_t *keyexpr) {
    *dst = _z_reply_null();
    _z_sample_owned_t s = _z_sample_owned_null();
    _Z_RETURN_IF_ERR(_z_keyexpr_copy(&s.keyexpr._inner, keyexpr));
    dst->_result._ok = _z_sample_from_owned(&s);
    dst->_result._tag = _z_reply_data_tag_ok;
    dst->replier_id = _z_entity_global_id_null();
    return _Z_RES_OK;
}

bool _z_pending_reply_eq(const _z_pending_reply_t *one, const _z_pending_reply_t *two) {
    return one->_tstamp.time == two->_tstamp.time;
}

void _z_pending_reply_clear(_z_pending_reply_t *pr) {
    // Free reply
    _z_reply_clear(&pr->_reply);

    // Free the timestamp
    _z_timestamp_clear(&pr->_tstamp);
}

#endif  // Z_FEATURE_QUERY == 1
