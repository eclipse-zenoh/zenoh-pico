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

#include "zenoh-pico/protocol/definitions/message.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/codec/ext.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/core.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

/*=============================*/
/*           Fields            */
/*=============================*/
/*------------------ Payload field ------------------*/
int8_t _z_payload_encode(_z_wbuf_t *wbf, const _z_bytes_t *pld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _PAYLOAD");
    ret |= _z_bytes_encode(wbf, pld);

    return ret;
}

int8_t _z_payload_decode_na(_z_bytes_t *pld, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _PAYLOAD");
    return _z_bytes_decode(pld, zbf);
}

int8_t _z_payload_decode(_z_bytes_t *pld, _z_zbuf_t *zbf) { return _z_payload_decode_na(pld, zbf); }

int8_t _z_id_encode_as_zbytes(_z_wbuf_t *wbf, const _z_id_t *id) {
    int8_t ret = _Z_RES_OK;
    uint8_t len = _z_id_len(*id);

    if (len != 0) {
        printf("ZIDLEN: %d\n", len);
        _z_bytes_t buf = _z_bytes_wrap(id->id, len);
        ret = _z_bytes_encode(wbf, &buf);
    } else {
        _Z_DEBUG("Attempted to encode invalid ID 0");
        ret = _Z_ERR_MESSAGE_ZENOH_UNKNOWN;
    }
    return ret;
}

/// Decodes a `zid` from the zbf, returning a negative value in case of error.
///
/// Note that while `_z_id_t` has an error state (full 0s), this function doesn't
/// guarantee that this state will be set in case of errors.
int8_t _z_id_decode_as_zbytes(_z_id_t *id, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    uint8_t len = _z_zbuf_read(zbf);
    _z_zbuf_read_bytes(zbf, id->id, 0, len);
    memset(id->id + len, 0, 16 - len);
    return ret;
}

/*------------------ Timestamp Field ------------------*/
int8_t _z_timestamp_encode(_z_wbuf_t *wbf, const _z_timestamp_t *ts) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _TIMESTAMP");

    _Z_RETURN_IF_ERR(_z_uint64_encode(wbf, ts->time))
    ret |= _z_id_encode_as_zbytes(wbf, &ts->id);

    return ret;
}
int8_t _z_timestamp_encode_ext(_z_wbuf_t *wbf, const _z_timestamp_t *ts) {
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, _z_zint_len(ts->time) + 1 + _z_id_len(ts->id)));
    return _z_timestamp_encode(wbf, ts);
}

int8_t _z_timestamp_decode(_z_timestamp_t *ts, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _TIMESTAMP");
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint64_decode(&ts->time, zbf);
    ret |= _z_id_decode_as_zbytes(&ts->id, zbf);

    return ret;
}

/*------------------ ResKey Field ------------------*/
int8_t _z_keyexpr_encode(_z_wbuf_t *wbf, _Bool has_suffix, const _z_keyexpr_t *fld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _RESKEY");

    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, fld->_id))
    if (has_suffix == true) {
        _Z_RETURN_IF_ERR(_z_str_encode(wbf, fld->_suffix))
    }

    return ret;
}

int8_t _z_keyexpr_decode(_z_keyexpr_t *ke, _z_zbuf_t *zbf, _Bool has_suffix) {
    _Z_DEBUG("Decoding _RESKEY");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint16_decode(&ke->_id, zbf);
    if (has_suffix == true) {
        char *str = NULL;
        ret |= _z_str_decode(&str, zbf);
        if (ret == _Z_RES_OK) {
            ke->_suffix = str;
            ke->_mapping = _z_keyexpr_mapping(0, true);
        } else {
            ke->_suffix = NULL;
            ke->_mapping = _z_keyexpr_mapping(0, false);
        }
    } else {
        ke->_suffix = NULL;
    }

    return ret;
}

/*------------------ Locators Field ------------------*/
int8_t _z_locators_encode(_z_wbuf_t *wbf, const _z_locator_array_t *la) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _LOCATORS");
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, la->_len))
    for (size_t i = 0; i < la->_len; i++) {
        char *s = _z_locator_to_str(&la->_val[i]);
        _Z_RETURN_IF_ERR(_z_str_encode(wbf, s))
        zp_free(s);
    }

    return ret;
}

int8_t _z_locators_decode_na(_z_locator_array_t *a_loc, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _LOCATORS");
    int8_t ret = _Z_RES_OK;

    _z_zint_t len = 0;  // Number of elements in the array
    ret |= _z_zint_decode(&len, zbf);
    if (ret == _Z_RES_OK) {
        *a_loc = _z_locator_array_make(len);

        // Decode the elements
        for (size_t i = 0; i < len; i++) {
            char *str = NULL;
            ret |= _z_str_decode(&str, zbf);
            if (ret == _Z_RES_OK) {
                _z_locator_init(&a_loc->_val[i]);
                ret |= _z_locator_from_str(&a_loc->_val[i], str);
                zp_free(str);
            } else {
                a_loc->_len = i;
            }
        }
    } else {
        *a_loc = _z_locator_array_make(0);
    }

    return ret;
}

int8_t _z_locators_decode(_z_locator_array_t *a_loc, _z_zbuf_t *zbf) { return _z_locators_decode_na(a_loc, zbf); }

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/

int8_t _z_source_info_decode(_z_source_info_t *info, _z_zbuf_t *zbf) {
    uint8_t zidlen = 0;
    _z_zint_t intbuf;
    int8_t ret = _z_uint8_decode(&zidlen, zbf);
    if (ret == _Z_RES_OK) {
        zidlen >>= 4;
        if (_z_zbuf_len(zbf) >= zidlen) {
            _z_zbuf_read_bytes(zbf, info->_id.id, 0, zidlen);
        } else {
            ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    }
    if (ret == _Z_RES_OK) {
        ret = _z_zint_decode(&intbuf, zbf);
        if (intbuf <= UINT32_MAX) {
            info->_entity_id = (uint32_t)intbuf;
        } else {
            ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    }
    if (ret == _Z_RES_OK) {
        ret = _z_zint_decode(&intbuf, zbf);
        if (intbuf <= UINT32_MAX) {
            info->_source_sn = (uint32_t)intbuf;
        } else {
            ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    }
    return ret;
}
int8_t _z_source_info_encode(_z_wbuf_t *wbf, const _z_source_info_t *info) {
    int8_t ret = 0;
    uint8_t zidlen = _z_id_len(info->_id);
    ret |= _z_uint8_encode(wbf, zidlen << 4);
    _z_bytes_t zid = _z_bytes_wrap(info->_id.id, zidlen);
    ret |= _z_bytes_val_encode(wbf, &zid);
    ret |= _z_zint_encode(wbf, info->_entity_id);
    ret |= _z_zint_encode(wbf, info->_source_sn);
    return ret;
}
int8_t _z_source_info_encode_ext(_z_wbuf_t *wbf, const _z_source_info_t *info) {
    int8_t ret = 0;
    uint8_t zidlen = _z_id_len(info->_id);
    uint16_t len = 1 + zidlen + _z_zint_len(info->_entity_id) + _z_zint_len(info->_source_sn);
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, len));
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, zidlen << 4));
    _z_bytes_t zid = _z_bytes_wrap(info->_id.id, zidlen);
    _Z_RETURN_IF_ERR(_z_bytes_val_encode(wbf, &zid));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, info->_entity_id));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, info->_source_sn));
    return ret;
}
#if Z_FEATURE_ATTACHMENT == 1
int8_t _z_attachment_encode_ext_kv(_z_bytes_t key, _z_bytes_t value, void *ctx) {
    _z_wbuf_t *wbf = (_z_wbuf_t *)ctx;
    _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &key));
    _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &value));
    return 0;
}
int8_t _z_attachment_encode_ext(_z_wbuf_t *wbf, z_attachment_t att) {
    size_t len = _z_attachment_estimate_length(att);
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, len));
    _Z_RETURN_IF_ERR(z_attachment_iterate(att, _z_attachment_encode_ext_kv, wbf));
    return 0;
}
#endif
/*------------------ Push Body Field ------------------*/
int8_t _z_push_body_encode(_z_wbuf_t *wbf, const _z_push_body_t *pshb) {
    (void)(wbf);
    (void)(pshb);
    uint8_t header = pshb->_is_put ? _Z_MID_Z_PUT : _Z_MID_Z_DEL;
    _Bool has_source_info = _z_id_check(pshb->_body._put._commons._source_info._id) ||
                            pshb->_body._put._commons._source_info._source_sn != 0 ||
                            pshb->_body._put._commons._source_info._entity_id != 0;
#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t att = _z_encoded_as_attachment(&pshb->_body._put._attachment);
    _Bool has_attachment = pshb->_is_put && z_attachment_check(&att);
#else
    _Bool has_attachment = false;
#endif
    _Bool has_timestamp = _z_timestamp_check(&pshb->_body._put._commons._timestamp);
    _Bool has_encoding = false;
    if (has_source_info || has_attachment) {
        header |= _Z_FLAG_Z_Z;
    }
    if (pshb->_is_put) {
        if (has_timestamp) {
            header |= _Z_FLAG_Z_P_T;
        }
        has_encoding = pshb->_body._put._encoding.prefix != Z_ENCODING_PREFIX_EMPTY ||
                       !_z_bytes_is_empty(&pshb->_body._put._encoding.suffix);
        if (has_encoding) {
            header |= _Z_FLAG_Z_P_E;
        }
    } else {
        if (has_timestamp) {
            header |= _Z_FLAG_Z_D_T;
        }
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    if (has_timestamp) {
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &pshb->_body._put._commons._timestamp));
    }

    if (has_encoding) {
        _Z_RETURN_IF_ERR(_z_encoding_prefix_encode(wbf, pshb->_body._put._encoding.prefix));
        _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &pshb->_body._put._encoding.suffix));
    }

    if (has_source_info) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | 0x01 | (has_attachment ? _Z_FLAG_Z_Z : 0)));
        _Z_RETURN_IF_ERR(_z_source_info_encode_ext(wbf, &pshb->_body._put._commons._source_info));
    }
#if Z_FEATURE_ATTACHMENT == 1
    if (has_attachment) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | 0x03));
        _Z_RETURN_IF_ERR(_z_attachment_encode_ext(wbf, att));
    }
#endif
    if (pshb->_is_put) {
        _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &pshb->_body._put._payload));
    }

    return 0;
}
int8_t _z_push_body_decode_extensions(_z_msg_ext_t *extension, void *ctx) {
    _z_push_body_t *pshb = (_z_push_body_t *)ctx;
    int8_t ret = _Z_RES_OK;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZBUF | 0x01: {
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_source_info_decode(&pshb->_body._put._commons._source_info, &zbf);
            break;
        }
#if Z_FEATURE_ATTACHMENT == 1
        case _Z_MSG_EXT_ENC_ZBUF | 0x03: {
            pshb->_body._put._attachment.is_encoded = true;
            pshb->_body._put._attachment.body.encoded = extension->_body._zbuf._val._is_alloc
                                                            ? _z_bytes_steal(&extension->_body._zbuf._val)
                                                            : _z_bytes_duplicate(&extension->_body._zbuf._val);
            break;
        }
#endif
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                ret = _z_msg_ext_unknown_error(extension, 0x08);
            }
    }
    return ret;
}

int8_t _z_push_body_decode(_z_push_body_t *pshb, _z_zbuf_t *zbf, uint8_t header) {
    int8_t ret = _Z_RES_OK;
    if (ret == _Z_RES_OK) {
        switch (_Z_MID(header)) {
            case _Z_MID_Z_PUT: {
                pshb->_is_put = true;
                pshb->_body._put = (_z_msg_put_t){0};
                if (_Z_HAS_FLAG(header, _Z_FLAG_Z_P_T)) {
                    _Z_RETURN_IF_ERR(_z_timestamp_decode(&pshb->_body._put._commons._timestamp, zbf));
                }
                if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(header, _Z_FLAG_Z_P_E)) {
                    _Z_RETURN_IF_ERR(_z_encoding_prefix_decode(&pshb->_body._put._encoding.prefix, zbf));
                    _Z_RETURN_IF_ERR(_z_bytes_decode(&pshb->_body._put._encoding.suffix, zbf));
                }
                if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
                    _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_push_body_decode_extensions, pshb));
                }
                if (ret == _Z_RES_OK) {
                    _Z_RETURN_IF_ERR(_z_bytes_decode(&pshb->_body._put._payload, zbf));
                }
                break;
            }
            case _Z_MID_Z_DEL: {
                pshb->_is_put = false;
                pshb->_body._del = (_z_msg_del_t){0};
                if (_Z_HAS_FLAG(header, _Z_FLAG_Z_D_T)) {
                    _Z_RETURN_IF_ERR(_z_timestamp_decode(&pshb->_body._put._commons._timestamp, zbf));
                }
                if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
                    _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_push_body_decode_extensions, pshb));
                }
                break;
            }
            default: {
                ret = _Z_ERR_MESSAGE_ZENOH_UNKNOWN;
            }
        }
    }

    return ret;
}

int8_t _z_put_encode(_z_wbuf_t *wbf, const _z_msg_put_t *put) {
    _z_push_body_t body = {._is_put = true, ._body = {._put = *put}};
    return _z_push_body_encode(wbf, &body);
}
int8_t _z_put_decode(_z_msg_put_t *put, _z_zbuf_t *zbf, uint8_t header) {
    assert(_Z_MID(header) == _Z_MID_Z_PUT);
    _z_push_body_t body = {._is_put = true, ._body = {._put = *put}};
    int8_t ret = _z_push_body_decode(&body, zbf, header);
    *put = body._body._put;
    return ret;
}

int8_t _z_del_encode(_z_wbuf_t *wbf, const _z_msg_del_t *del) {
    _z_push_body_t body = {._is_put = false, ._body = {._del = *del}};
    return _z_push_body_encode(wbf, &body);
}
int8_t _z_del_decode(_z_msg_del_t *del, _z_zbuf_t *zbf, uint8_t header) {
    assert(_Z_MID(header) == _Z_MID_Z_DEL);
    _z_push_body_t body = {._is_put = false, ._body = {._del = *del}};
    int8_t ret = _z_push_body_decode(&body, zbf, header);
    *del = body._body._del;
    return ret;
}

/*------------------ Query Message ------------------*/
int8_t _z_query_encode(_z_wbuf_t *wbf, const _z_msg_query_t *msg) {
    int8_t ret = _Z_RES_OK;
    uint8_t header = _Z_MID_Z_QUERY;

    _Bool has_params = z_bytes_check(&msg->_parameters);
    if (has_params) {
        _Z_SET_FLAG(header, _Z_FLAG_Z_Q_P);
    }
    _Bool has_consolidation = (msg->_consolidation != Z_CONSOLIDATION_MODE_DEFAULT);
    if (has_consolidation) {
        _Z_SET_FLAG(header, _Z_FLAG_Z_Q_C);
    }
    _z_msg_query_reqexts_t required_exts = _z_msg_query_required_extensions(msg);
    if (required_exts.body || required_exts.info || required_exts.attachment) {
        header |= _Z_FLAG_Z_Z;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    if (has_consolidation) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, msg->_consolidation));
    }
    if (has_params) {
        _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &msg->_parameters));
    }
    if (required_exts.body) {
        uint8_t extheader = _Z_MSG_EXT_ENC_ZBUF | 0x03;
        if (required_exts.info || required_exts.attachment) {
            extheader |= _Z_FLAG_Z_Z;
        }
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, _z_zint_len(msg->_ext_value.encoding.prefix) +
                                                 _z_bytes_encode_len(&msg->_ext_value.encoding.suffix) +
                                                 msg->_ext_value.payload.len));
        _Z_RETURN_IF_ERR(_z_encoding_prefix_encode(wbf, msg->_ext_value.encoding.prefix));
        _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &msg->_ext_value.encoding.suffix));
        _Z_RETURN_IF_ERR(_z_bytes_val_encode(wbf, &msg->_ext_value.payload));
    }
    if (required_exts.info) {
        uint8_t extheader = _Z_MSG_EXT_ENC_ZBUF | 0x01;
        if (required_exts.attachment) {
            extheader |= _Z_FLAG_Z_Z;
        }
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_source_info_encode_ext(wbf, &msg->_ext_info));
    }
#if Z_FEATURE_ATTACHMENT == 1
    if (required_exts.attachment) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | 0x05));
        z_attachment_t att = _z_encoded_as_attachment(&msg->_ext_attachment);
        _Z_RETURN_IF_ERR(_z_attachment_encode_ext(wbf, att));
    }
#endif

    return ret;
}

int8_t _z_query_decode_extensions(_z_msg_ext_t *extension, void *ctx) {
    _z_msg_query_t *msg = (_z_msg_query_t *)ctx;
    int8_t ret = _Z_RES_OK;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZBUF | 0x01: {  // Source Info
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_source_info_decode(&msg->_ext_info, &zbf);
            break;
        }
        case _Z_MSG_EXT_ENC_ZBUF | 0x03: {  // Payload
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_encoding_prefix_decode(&msg->_ext_value.encoding.prefix, &zbf);
            ret |= _z_bytes_decode(&msg->_ext_value.encoding.suffix, &zbf);
            _z_bytes_t bytes = _z_bytes_wrap((uint8_t *)_z_zbuf_start(&zbf), _z_zbuf_len(&zbf));
            _z_bytes_copy(&msg->_ext_value.payload, &bytes);
            break;
        }
#if Z_FEATURE_ATTACHMENT == 1
        case _Z_MSG_EXT_ENC_ZBUF | 0x05: {
            msg->_ext_attachment.is_encoded = true;
            msg->_ext_attachment.body.encoded = extension->_body._zbuf._val._is_alloc
                                                    ? _z_bytes_steal(&extension->_body._zbuf._val)
                                                    : _z_bytes_duplicate(&extension->_body._zbuf._val);
            break;
        }
#endif
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                ret = _z_msg_ext_unknown_error(extension, 0x09);
            }
    }
    return ret;
}

int8_t _z_query_decode(_z_msg_query_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_Z_QUERY");
    *msg = (_z_msg_query_t){0};
    int8_t ret = _Z_RES_OK;

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Q_C)) {
        _Z_RETURN_IF_ERR(_z_uint8_decode((uint8_t *)&msg->_consolidation, zbf));
    } else {
        msg->_consolidation = Z_CONSOLIDATION_MODE_DEFAULT;
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Q_P)) {
        _Z_RETURN_IF_ERR(_z_bytes_decode(&msg->_parameters, zbf));
    } else {
        _z_bytes_clear(&msg->_parameters);
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_query_decode_extensions, msg));
    }
    return ret;
}

int8_t _z_reply_encode(_z_wbuf_t *wbf, const _z_msg_reply_t *reply) {
    uint8_t header = _Z_MID_Z_REPLY;
    if (_z_timestamp_check(&reply->_timestamp)) {
        header |= _Z_FLAG_Z_R_T;
    }
    if (reply->_value.encoding.prefix != Z_ENCODING_PREFIX_EMPTY ||
        !_z_bytes_is_empty(&reply->_value.encoding.suffix)) {
        header |= _Z_FLAG_Z_R_E;
    }
#if Z_FEATURE_ATTACHMENT == 1
    z_attachment_t att = _z_encoded_as_attachment(&reply->_ext_attachment);
    _Bool has_attachment = z_attachment_check(&att);
#else
    _Bool has_attachment = false;
#endif
    _Bool has_sourceinfo = _z_id_check(reply->_ext_source_info._id) || reply->_ext_source_info._source_sn != 0 ||
                           reply->_ext_source_info._entity_id != 0;
    _Bool has_consolidation_ext = reply->_ext_consolidation != Z_CONSOLIDATION_MODE_AUTO;
    if (has_consolidation_ext || has_sourceinfo || has_attachment) {
        header |= _Z_FLAG_Z_Z;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    int8_t ret = _Z_RES_OK;
    if (_z_timestamp_check(&reply->_timestamp)) {
        assert(_Z_HAS_FLAG(header, _Z_FLAG_Z_R_T));
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &reply->_timestamp));
    }
    if (((reply->_value.encoding.prefix != 0) || !_z_bytes_is_empty(&reply->_value.encoding.suffix))) {
        assert(_Z_HAS_FLAG(header, _Z_FLAG_Z_R_E));
        _Z_RETURN_IF_ERR(_z_encoding_prefix_encode(wbf, reply->_value.encoding.prefix));
        _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &reply->_value.encoding.suffix));
    }
    if (has_sourceinfo) {
        uint8_t extheader = _Z_MSG_EXT_ENC_ZBUF | 0x01;
        if (has_consolidation_ext || has_attachment) {
            extheader |= _Z_MSG_EXT_FLAG_Z;
        }
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_source_info_encode_ext(wbf, &reply->_ext_source_info));
    }
    if (has_consolidation_ext) {
        uint8_t extheader = _Z_MSG_EXT_ENC_ZINT | _Z_MSG_EXT_FLAG_M | 0x02;
        if (has_attachment) {
            extheader |= _Z_MSG_EXT_FLAG_Z;
        }
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, reply->_ext_consolidation));
    }
#if Z_FEATURE_ATTACHMENT == 1
    if (has_attachment) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | 0x04));
        _Z_RETURN_IF_ERR(_z_attachment_encode_ext(wbf, att));
    }
#endif
    _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &reply->_value.payload));
    return ret;
}
int8_t _z_reply_decode_extension(_z_msg_ext_t *extension, void *ctx) {
    int8_t ret = _Z_RES_OK;
    _z_msg_reply_t *reply = (_z_msg_reply_t *)ctx;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZBUF | 0x01: {
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_source_info_decode(&reply->_ext_source_info, &zbf);
            break;
        }
        case _Z_MSG_EXT_ENC_ZINT | _Z_MSG_EXT_FLAG_M | 0x02: {
            reply->_ext_consolidation = extension->_body._zint._val;
            break;
        }
#if Z_FEATURE_ATTACHMENT == 1
        case _Z_MSG_EXT_ENC_ZBUF | 0x04: {
            reply->_ext_attachment.is_encoded = true;
            reply->_ext_attachment.body.encoded = extension->_body._zbuf._val._is_alloc
                                                      ? _z_bytes_steal(&extension->_body._zbuf._val)
                                                      : _z_bytes_duplicate(&extension->_body._zbuf._val);
            break;
        }
#endif
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                ret = _z_msg_ext_unknown_error(extension, 0x0a);
            }
    }
    return ret;
}
int8_t _z_reply_decode(_z_msg_reply_t *reply, _z_zbuf_t *zbf, uint8_t header) {
    int8_t ret = _Z_RES_OK;
    *reply = (_z_msg_reply_t){0};
    reply->_ext_consolidation = Z_CONSOLIDATION_MODE_AUTO;
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_R_T)) {
        _Z_RETURN_IF_ERR(_z_timestamp_decode(&reply->_timestamp, zbf));
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_R_E)) {
        _Z_RETURN_IF_ERR(_z_encoding_prefix_decode(&reply->_value.encoding.prefix, zbf));
        _Z_RETURN_IF_ERR(_z_bytes_decode(&reply->_value.encoding.suffix, zbf));
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_reply_decode_extension, reply));
    }
    _Z_RETURN_IF_ERR(_z_bytes_decode(&reply->_value.payload, zbf));

    return ret;
}

int8_t _z_err_encode(_z_wbuf_t *wbf, const _z_msg_err_t *err) {
    int8_t ret = _Z_RES_OK;
    uint8_t header = _Z_MID_Z_ERR;
    _Bool has_timestamp = _z_timestamp_check(&err->_timestamp);
    if (has_timestamp) {
        header |= _Z_FLAG_Z_E_T;
    }
    if (err->_is_infrastructure) {
        header |= _Z_FLAG_Z_E_I;
    }
    _Bool has_payload_ext = err->_ext_value.payload.start != NULL || err->_ext_value.encoding.prefix != 0 ||
                            !_z_bytes_is_empty(&err->_ext_value.encoding.suffix);
    _Bool has_sinfo_ext = _z_id_check(err->_ext_source_info._id) || err->_ext_source_info._entity_id != 0 ||
                          err->_ext_source_info._source_sn != 0;
    if (has_sinfo_ext || has_payload_ext) {
        header |= _Z_FLAG_Z_Z;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, err->_code));
    if (has_timestamp) {
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &err->_timestamp));
    }
    if (has_sinfo_ext) {
        uint8_t extheader = _Z_MSG_EXT_ENC_ZBUF | 0x01;
        if (has_payload_ext) {
            extheader |= _Z_MSG_EXT_FLAG_Z;
        }
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_source_info_encode_ext(wbf, &err->_ext_source_info));
    }
    if (has_payload_ext) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | 0x02));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, _z_zint_len(err->_ext_value.encoding.prefix) +
                                                 _z_bytes_encode_len(&err->_ext_value.encoding.suffix) +
                                                 _z_bytes_encode_len(&err->_ext_value.payload)));
        _Z_RETURN_IF_ERR(_z_encoding_prefix_encode(wbf, err->_ext_value.encoding.prefix));
        _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &err->_ext_value.encoding.suffix));
        _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &err->_ext_value.payload));
    }
    return ret;
}
int8_t _z_err_decode_extension(_z_msg_ext_t *extension, void *ctx) {
    int8_t ret = _Z_RES_OK;
    _z_msg_err_t *reply = (_z_msg_err_t *)ctx;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZBUF | 0x01: {
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_source_info_decode(&reply->_ext_source_info, &zbf);
            break;
        }
        case _Z_MSG_EXT_ENC_ZBUF | 0x02: {
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_encoding_prefix_decode(&reply->_ext_value.encoding.prefix, &zbf);
            ret |= _z_bytes_decode(&reply->_ext_value.encoding.suffix, &zbf);
            ret |= _z_bytes_decode(&reply->_ext_value.payload, &zbf);
            break;
        }
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                ret = _z_msg_ext_unknown_error(extension, 0x0a);
            }
    }
    return ret;
}
int8_t _z_err_decode(_z_msg_err_t *err, _z_zbuf_t *zbf, uint8_t header) {
    *err = (_z_msg_err_t){0};
    int8_t ret = _Z_RES_OK;
    _z_zint_t code;
    ret = _z_zint_decode(&code, zbf);
    if (code <= UINT16_MAX) {
        err->_code = (uint16_t)code;
    } else {
        ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
    err->_is_infrastructure = _Z_HAS_FLAG(header, _Z_FLAG_Z_E_I);
    if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(header, _Z_FLAG_Z_E_T)) {
        ret = _z_timestamp_decode(&err->_timestamp, zbf);
    }
    if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        ret = _z_msg_ext_decode_iter(zbf, _z_err_decode_extension, err);
    }

    return ret;
}

int8_t _z_ack_encode(_z_wbuf_t *wbf, const _z_msg_ack_t *ack) {
    int8_t ret = _Z_RES_OK;
    uint8_t header = _Z_MID_Z_ACK;
    _Bool has_ts = _z_timestamp_check(&ack->_timestamp);
    _Bool has_sinfo_ext = _z_id_check(ack->_ext_source_info._id) || ack->_ext_source_info._source_sn != 0 ||
                          ack->_ext_source_info._entity_id != 0;
    if (has_ts) {
        header |= _Z_FLAG_Z_A_T;
    }
    if (has_sinfo_ext) {
        header |= _Z_FLAG_Z_Z;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    if (has_ts) {
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &ack->_timestamp));
    }
    if (has_sinfo_ext) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | 0x01));
        _Z_RETURN_IF_ERR(_z_source_info_encode_ext(wbf, &ack->_ext_source_info));
    }
    return ret;
}
int8_t _z_ack_decode_extension(_z_msg_ext_t *extension, void *ctx) {
    int8_t ret = _Z_RES_OK;
    _z_msg_ack_t *ack = (_z_msg_ack_t *)ctx;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZBUF | 0x01: {
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_source_info_decode(&ack->_ext_source_info, &zbf);
            break;
        }
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                ret = _z_msg_ext_unknown_error(extension, 0x0b);
            }
    }
    return ret;
}
int8_t _z_ack_decode(_z_msg_ack_t *ack, _z_zbuf_t *zbf, uint8_t header) {
    int8_t ret = _Z_RES_OK;
    *ack = (_z_msg_ack_t){0};
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_A_T)) {
        _Z_RETURN_IF_ERR(_z_timestamp_decode(&ack->_timestamp, zbf));
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        ret = _z_msg_ext_decode_iter(zbf, _z_ack_decode_extension, ack);
    }
    return ret;
}

int8_t _z_pull_encode(_z_wbuf_t *wbf, const _z_msg_pull_t *pull) {
    int8_t ret = _Z_RES_OK;
    uint8_t header = _Z_MID_Z_PULL;
    _Bool has_info_ext = _z_id_check(pull->_ext_source_info._id) || pull->_ext_source_info._source_sn != 0 ||
                         pull->_ext_source_info._entity_id != 0;
    if (has_info_ext) {
        header |= _Z_FLAG_Z_Z;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    if (has_info_ext) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | 0x01));
        _Z_RETURN_IF_ERR(_z_source_info_encode_ext(wbf, &pull->_ext_source_info));
    }
    return ret;
}
int8_t _z_pull_decode_extension(_z_msg_ext_t *extension, void *ctx) {
    int8_t ret = _Z_RES_OK;
    _z_msg_pull_t *pull = (_z_msg_pull_t *)ctx;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZBUF | 0x01: {
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_source_info_decode(&pull->_ext_source_info, &zbf);
            break;
        }
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                ret = _z_msg_ext_unknown_error(extension, 0x0c);
            }
    }
    return ret;
}
int8_t _z_pull_decode(_z_msg_pull_t *pull, _z_zbuf_t *zbf, uint8_t header) {
    *pull = (_z_msg_pull_t){0};
    int8_t ret = _Z_RES_OK;
    if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        ret = _z_msg_ext_decode_iter(zbf, _z_pull_decode_extension, pull);
    }
    return ret;
}

/*=============================*/
/*       Scouting Messages     */
/*=============================*/
/*------------------ Scout Message ------------------*/
int8_t _z_scout_encode(_z_wbuf_t *wbf, uint8_t header, const _z_s_msg_scout_t *msg) {
    int8_t ret = _Z_RES_OK;
    (void)(header);
    _Z_DEBUG("Encoding _Z_MID_SCOUT");

    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, msg->_version))

    uint8_t cbyte = 0;
    cbyte |= (msg->_what & 0x07);
    uint8_t zid_len = _z_id_len(msg->_zid);
    if (zid_len > 0) {
        _Z_SET_FLAG(cbyte, _Z_FLAG_T_SCOUT_I);
        cbyte |= ((zid_len - 1) & 0x0F) << 4;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, cbyte))

    ret |= _z_wbuf_write_bytes(wbf, msg->_zid.id, 0, zid_len);

    return ret;
}

int8_t _z_scout_decode(_z_s_msg_scout_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    int8_t ret = _Z_RES_OK;
    (void)(header);
    _Z_DEBUG("Decoding _Z_MID_SCOUT");
    *msg = (_z_s_msg_scout_t){0};

    ret |= _z_uint8_decode(&msg->_version, zbf);

    uint8_t cbyte = 0;
    ret |= _z_uint8_decode(&cbyte, zbf);
    msg->_what = cbyte & 0x07;
    msg->_zid = _z_id_empty();
    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(cbyte, _Z_FLAG_T_SCOUT_I) == true)) {
        uint8_t zidlen = ((cbyte & 0xF0) >> 4) + 1;
        _z_zbuf_read_bytes(zbf, msg->_zid.id, 0, zidlen);
    }

    return ret;
}

/*------------------ Hello Message ------------------*/
int8_t _z_hello_encode(_z_wbuf_t *wbf, uint8_t header, const _z_s_msg_hello_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_HELLO");

    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, msg->_version))
    uint8_t zidlen = _z_id_len(msg->_zid);
    uint8_t cbyte = 0;
    cbyte |= (msg->_whatami & 0x03);
    cbyte |= ((zidlen - 1) & 0x0F) << 4;
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, cbyte))
    _Z_RETURN_IF_ERR(_z_bytes_val_encode(wbf, &(_z_bytes_t){.start = msg->_zid.id, .len = zidlen, ._is_alloc = false}));

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_HELLO_L) == true) {
        _Z_RETURN_IF_ERR(_z_locators_encode(wbf, &msg->_locators))
    }

    return ret;
}

int8_t _z_hello_decode_na(_z_s_msg_hello_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_HELLO");
    int8_t ret = _Z_RES_OK;
    *msg = (_z_s_msg_hello_t){0};

    ret |= _z_uint8_decode(&msg->_version, zbf);

    uint8_t cbyte = 0;
    ret |= _z_uint8_decode(&cbyte, zbf);
    msg->_whatami = cbyte & 0x03;
    uint8_t zidlen = ((cbyte & 0xF0) >> 4) + 1;

    if (ret == _Z_RES_OK) {
        msg->_zid = _z_id_empty();
        _z_zbuf_read_bytes(zbf, msg->_zid.id, 0, zidlen);
    } else {
        msg->_zid = _z_id_empty();
    }

    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_HELLO_L) == true)) {
        ret |= _z_locators_decode(&msg->_locators, zbf);
        if (ret != _Z_RES_OK) {
            msg->_locators = _z_locator_array_empty();
        }
    } else {
        msg->_locators = _z_locator_array_empty();
    }

    return ret;
}

int8_t _z_hello_decode(_z_s_msg_hello_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_hello_decode_na(msg, zbf, header);
}

int8_t _z_scouting_message_encode(_z_wbuf_t *wbf, const _z_scouting_message_t *msg) {
    int8_t ret = _Z_RES_OK;

    uint8_t header = msg->_header;

    _Z_RETURN_IF_ERR(_z_wbuf_write(wbf, header))
    switch (_Z_MID(msg->_header)) {
        case _Z_MID_SCOUT: {
            ret |= _z_scout_encode(wbf, msg->_header, &msg->_body._scout);
        } break;

        case _Z_MID_HELLO: {
            ret |= _z_hello_encode(wbf, msg->_header, &msg->_body._hello);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to encode session message with unknown ID(%d)", _Z_MID(msg->_header));
            ret |= _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
        } break;
    }

    return ret;
}
int8_t _z_scouting_message_decode_na(_z_scouting_message_t *msg, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    *msg = (_z_scouting_message_t){0};

    _Bool is_last = false;

    do {
        ret |= _z_uint8_decode(&msg->_header, zbf);  // Decode the header
        if (ret == _Z_RES_OK) {
            uint8_t mid = _Z_MID(msg->_header);
            switch (mid) {
                case _Z_MID_SCOUT: {
                    ret |= _z_scout_decode(&msg->_body._scout, zbf, msg->_header);
                    is_last = true;
                } break;

                case _Z_MID_HELLO: {
                    ret |= _z_hello_decode(&msg->_body._hello, zbf, msg->_header);
                    is_last = true;
                } break;

                default: {
                    _Z_DEBUG("WARNING: Trying to decode scouting message with unknown ID(0x%x)", mid);
                    ret |= _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
                    is_last = true;
                } break;
            }
        } else {
            msg->_header = 0xFF;
        }
    } while ((ret == _Z_RES_OK) && (is_last == false));

    if ((ret == _Z_RES_OK) && (msg->_header & _Z_MSG_EXT_FLAG_Z) != 0) {
        ret |= _z_msg_ext_skip_non_mandatories(zbf, 0x06);
    }

    return ret;
}

int8_t _z_scouting_message_decode(_z_scouting_message_t *s_msg, _z_zbuf_t *zbf) {
    return _z_scouting_message_decode_na(s_msg, zbf);
}
