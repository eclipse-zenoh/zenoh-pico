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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/codec/ext.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Push Message ------------------*/

int8_t _z_push_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_push_t *msg) {
    _Z_DEBUG("Encoding _Z_MID_N_PUSH\n");

    _Z_RETURN_IF_ERR(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_N_PUSH_N), &msg->_key));

    _Bool has_timestamp_ext = _z_timestamp_check(&msg->_timestamp);

    if (msg->_qos._val != _Z_N_QOS_DEFAULT._val) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZINT | 0x01 | (has_timestamp_ext << 7)));
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, msg->_qos._val));
    }

    if (has_timestamp_ext) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | 0x02));
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &msg->_timestamp));
    }

    _Z_RETURN_IF_ERR(_z_push_body_encode(wbf, &msg->_body));

    return _Z_RES_OK;
}

int8_t _z_push_decode_ext_cb(_z_msg_ext_t *extension, void *ctx) {
    int8_t ret = _Z_RES_OK;
    _z_n_msg_push_t *msg = (_z_n_msg_push_t *)ctx;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZINT | 0x01: {  // QOS ext
            msg->_qos = (_z_n_qos_t){._val = extension->_body._zint._val};
            break;
        }
        case _Z_MSG_EXT_ENC_ZBUF | 0x02: {  // Timestamp ext
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_timestamp_decode(&msg->_timestamp, &zbf);
            break;
        }
        default:
            if ((extension->_header & _Z_MSG_EXT_FLAG_M) != 0) {
                ret = _z_msg_ext_unknown_error(extension, 0x07);
            }
    }
    return ret;
}

int8_t _z_push_decode(_z_n_msg_push_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_N_PUSH\n");
    int8_t ret = _Z_RES_OK;
    _z_timestamp_reset(&msg->_timestamp);

    ret |= _z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_N_PUSH_N));
    if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(header, _Z_FLAG_N_Z)) {
        ret = _z_msg_ext_decode_iter(zbf, _z_push_decode_ext_cb, msg);
    }
    if (ret == _Z_RES_OK) {
        uint8_t msgheader;
        _Z_RETURN_IF_ERR(_z_uint8_decode(&msgheader, zbf));
        _Z_RETURN_IF_ERR(_z_push_body_decode(&msg->_body, zbf, msgheader));
    }

    return ret;
}

/*------------------ Request Body Field ------------------*/
int8_t _z_request_body_encode(_z_wbuf_t *wbf, const _z_request_body_t *reqb) {
    (void)(wbf);
    (void)(reqb);
    int8_t ret = _Z_RES_OK;

    return ret;
}

int8_t _z_request_body_decode_na(_z_request_body_t *reqb, _z_zbuf_t *zbf) {
    (void)(zbf);
    (void)(reqb);
    int8_t ret = _Z_RES_OK;

    return ret;
}

int8_t _z_request_body_decode(_z_request_body_t *reqb, _z_zbuf_t *zbf) { return _z_request_body_decode_na(reqb, zbf); }

/*------------------ Request Message ------------------*/
int8_t _z_request_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_request_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_N_REQUEST\n");

    _Z_EC(_z_zint_encode(wbf, msg->_rid));
    _Z_EC(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_N_REQUEST_N), &msg->_key));
    _Z_EC(_z_request_body_encode(wbf, &msg->_body));

    return ret;
}

int8_t _z_request_decode_na(_z_n_msg_request_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_N_REQUEST\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&msg->_rid, zbf);
    ret |= _z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_N_REQUEST_N));
    ret |= _z_request_body_decode(&msg->_body, zbf);

    return ret;
}

int8_t _z_request_decode(_z_n_msg_request_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_request_decode_na(msg, zbf, header);
}

/*------------------ Response Message ------------------*/
int8_t _z_response_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_response_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_N_RESPONSE\n");
    if (msg->_uses_sender_mapping) {
        assert(_Z_HAS_FLAG(header, _Z_FLAG_N_RESPONSE_M));
    }
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_request_id));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_key._id));
    if (msg->_key._suffix != NULL) {
        assert(_Z_HAS_FLAG(header, _Z_FLAG_N_RESPONSE_N));
        _Z_RETURN_IF_ERR(_z_str_encode(wbf, msg->_key._suffix))
    }
    _Bool has_qos_ext = msg->_ext_qos._val != _Z_N_QOS_DEFAULT._val;
    _Bool has_ts_ext = _z_timestamp_check(&msg->_ext_timestamp);
    if (has_qos_ext) {
        assert(_Z_HAS_FLAG(header, _Z_FLAG_Z_Z));
        uint8_t extheader = _Z_MSG_EXT_ENC_ZINT | 0x01;
        if (has_ts_ext) {
            extheader |= _Z_FLAG_Z_Z;
        }
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_ext_qos._val));
    }
    if (has_ts_ext) {
        assert(_Z_HAS_FLAG(header, _Z_FLAG_Z_Z));
        uint8_t extheader = _Z_MSG_EXT_ENC_ZBUF | 0x02;
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &msg->_ext_timestamp));
    }

    switch (msg->_body_tag) {
        case _Z_RESPONSE_BODY_REPLY: {
            _Z_RETURN_IF_ERR(_z_reply_encode(wbf, &msg->_body._reply));
            break;
        }
        case _Z_RESPONSE_BODY_ERR: {
            _Z_RETURN_IF_ERR(_z_err_encode(wbf, &msg->_body._err));
            break;
        }
        case _Z_RESPONSE_BODY_ACK: {
            _Z_RETURN_IF_ERR(_z_ack_encode(wbf, &msg->_body._ack));
            break;
        }
        case _Z_RESPONSE_BODY_PUT: {
            _Z_RETURN_IF_ERR(_z_put_encode(wbf, &msg->_body._put));
            break;
        }
        case _Z_RESPONSE_BODY_DEL: {
            _Z_RETURN_IF_ERR(_z_del_encode(wbf, &msg->_body._del));
            break;
        }
    }

    return ret;
}
int8_t _z_response_decode_extension(_z_msg_ext_t *extension, void *ctx) {
    int8_t ret = _Z_RES_OK;
    _z_n_msg_response_t *msg = (_z_n_msg_response_t *)ctx;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZINT | 0x01: {
            msg->_ext_qos._val = extension->_body._zint._val;
            break;
        }
        case _Z_MSG_EXT_ENC_ZBUF | 0x02: {
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            ret = _z_timestamp_decode(&msg->_ext_timestamp, &zbf);
            break;
        }
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                ret = _z_msg_ext_unknown_error(extension, 0x0d);
            }
    }
    return ret;
}

int8_t _z_response_decode(_z_n_msg_response_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_N_RESPONSE\n");
    int8_t ret = _Z_RES_OK;
    msg->_uses_sender_mapping = _Z_HAS_FLAG(header, _Z_FLAG_N_RESPONSE_M);
    _Z_EC(_z_zint_decode(&msg->_request_id, zbf));
    _Z_EC(_z_zint_decode(&msg->_key._id, zbf));
    if (_Z_HAS_FLAG(header, _Z_FLAG_N_RESPONSE_N)) {
        _Z_EC(_z_str_decode((char **)&msg->_key._suffix, zbf))
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_EC(_z_msg_ext_decode_iter(zbf, _z_response_decode_extension, msg));
    }

    _Z_EC(_z_uint8_decode(&header, zbf));

    switch (_Z_MID(header)) {
        case _Z_MID_Z_REPLY: {
            _z_reply_decode(&msg->_body._reply, zbf, header);
            break;
        }
        case _Z_MID_Z_ERR: {
            _z_err_decode(&msg->_body._err, zbf, header);
            break;
        }
        case _Z_MID_Z_ACK: {
            _z_ack_decode(&msg->_body._ack, zbf, header);
            break;
        }
        case _Z_MID_Z_PUT: {
            _z_put_decode(&msg->_body._put, zbf, header);
            break;
        }
        case _Z_MID_Z_DEL: {
            _z_del_decode(&msg->_body._del, zbf, header);
            break;
        }
        default: {
            ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    }
    return ret;
}

/*------------------ Response Final Message ------------------*/
int8_t _z_response_final_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_response_final_t *msg) {
    (void)(header);
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_N_RESPONSE\n");

    _Z_EC(_z_zint_encode(wbf, msg->_request_id));

    return ret;
}

int8_t _z_response_final_decode_na(_z_n_msg_response_final_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    (void)(header);
    _Z_DEBUG("Decoding _Z_MID_N_RESPONSE\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&msg->_request_id, zbf);

    return ret;
}

int8_t _z_response_final_decode(_z_n_msg_response_final_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_response_final_decode_na(msg, zbf, header);
}

/*------------------ Network Message ------------------*/
int8_t _z_network_message_encode(_z_wbuf_t *wbf, const _z_network_message_t *msg) {
    int8_t ret = _Z_RES_OK;

    uint8_t header = msg->_header;
    if (_z_msg_ext_vec_is_empty(&msg->_extensions) == false) {
        header |= _Z_FLAG_N_Z;
    }

    _Z_EC(_z_wbuf_write(wbf, header))
    switch (_Z_MID(msg->_header)) {
        case _Z_MID_N_DECLARE: {
            ret |= _z_declare_encode(wbf, &msg->_body._declare);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to encode network message with unknown ID(%d)\n", _Z_MID(msg->_header));
            ret |= _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
        } break;
    }

    return ret;
}

int8_t _z_network_message_decode_na(_z_network_message_t *msg, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint8_decode(&msg->_header, zbf);  // Decode the header
    if (ret == _Z_RES_OK) {
        uint8_t mid = _Z_MID(msg->_header);
        switch (mid) {
            case _Z_MID_N_DECLARE: {
                ret |= _z_declare_decode(&msg->_body._declare, zbf);
            } break;

            default: {
                _Z_DEBUG("WARNING: Trying to decode session message with unknown ID(%d)\n", mid);
                ret |= _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
            } break;
        }
    }

    return ret;
}

int8_t _z_network_message_decode(_z_network_message_t *n_msg, _z_zbuf_t *zbf) {
    return _z_network_message_decode_na(n_msg, zbf);
}