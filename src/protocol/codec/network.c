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

#include "zenoh-pico/protocol/definitions/network.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/codec/declarations.h"
#include "zenoh-pico/protocol/codec/ext.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/core.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Push Message ------------------*/

int8_t _z_push_encode(_z_wbuf_t *wbf, const _z_n_msg_push_t *msg) {
    uint8_t header = _Z_MID_N_PUSH | (msg->_key._uses_remote_mapping ? _Z_FLAG_N_REQUEST_M : 0);
    _Bool has_suffix = _z_keyexpr_has_suffix(msg->_key);
    _Bool has_timestamp_ext = _z_timestamp_check(&msg->_timestamp);
    if (has_suffix) {
        header |= _Z_FLAG_N_REQUEST_N;
    }
    if (has_timestamp_ext) {
        header |= _Z_FLAG_N_Z;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_keyexpr_encode(wbf, has_suffix, &msg->_key));

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
    int8_t ret = _Z_RES_OK;
    ret |= _z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_N_PUSH_N));
    msg->_key._uses_remote_mapping = !_Z_HAS_FLAG(header, _Z_FLAG_N_PUSH_M);
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

/*------------------ Request Message ------------------*/
int8_t _z_request_encode(_z_wbuf_t *wbf, const _z_n_msg_request_t *msg) {
    int8_t ret = _Z_RES_OK;
    uint8_t header = _Z_MID_N_REQUEST | (msg->_key._uses_remote_mapping ? _Z_FLAG_N_REQUEST_M : 0);
    _Bool has_suffix = _z_keyexpr_has_suffix(msg->_key);
    if (has_suffix) {
        header |= _Z_FLAG_N_REQUEST_N;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_rid));
    _Z_RETURN_IF_ERR(_z_keyexpr_encode(wbf, has_suffix, &msg->_key));

    _z_n_msg_request_exts_t exts = _z_n_msg_request_needed_exts(msg);
    if (exts.ext_qos) {
        exts.n -= 1;
        uint8_t extheader = 0x01 | _Z_MSG_EXT_ENC_ZINT | (exts.n ? _Z_FLAG_Z_Z : 0);
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->ext_qos._val));
    }
    if (exts.ext_tstamp) {
        exts.n -= 1;
        uint8_t extheader = 0x02 | _Z_MSG_EXT_ENC_ZBUF | (exts.n ? _Z_FLAG_Z_Z : 0);
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &msg->ext_tstamp));
    }
    if (exts.ext_target) {
        exts.n -= 1;
        uint8_t extheader = 0x04 | _Z_MSG_EXT_ENC_ZINT | (exts.n ? _Z_FLAG_Z_Z : 0) | _Z_MSG_EXT_FLAG_M;
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->ext_target));
    }
    if (exts.ext_budget) {
        exts.n -= 1;
        uint8_t extheader = 0x05 | _Z_MSG_EXT_ENC_ZINT | (exts.n ? _Z_FLAG_Z_Z : 0);
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->ext_budget));
    }
    if (exts.ext_timeout_ms) {
        exts.n -= 1;
        uint8_t extheader = 0x06 | _Z_MSG_EXT_ENC_ZINT | (exts.n ? _Z_FLAG_Z_Z : 0);
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->ext_timeout_ms));
    }

    switch (msg->_tag) {
        case _Z_REQUEST_QUERY: {
            _Z_RETURN_IF_ERR(_z_query_encode(wbf, &msg->_body._query));
        } break;
        case _Z_REQUEST_PUT: {
            _Z_RETURN_IF_ERR(_z_put_encode(wbf, &msg->_body.put));
        } break;
        case _Z_REQUEST_DEL: {
            _Z_RETURN_IF_ERR(_z_del_encode(wbf, &msg->_body.del));
        } break;
        case _Z_REQUEST_PULL: {
            _Z_RETURN_IF_ERR(_z_pull_encode(wbf, &msg->_body.pull));
        } break;
    }
    return ret;
}
int8_t _z_request_decode_extensions(_z_msg_ext_t *extension, void *ctx) {
    _z_n_msg_request_t *msg = (_z_n_msg_request_t *)ctx;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case 0x01 | _Z_MSG_EXT_ENC_ZINT: {  // QOS ext
            msg->ext_qos = (_z_n_qos_t){._val = extension->_body._zint._val};
            break;
        }
        case 0x02 | _Z_MSG_EXT_ENC_ZBUF: {  // Timestamp ext
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            _Z_RETURN_IF_ERR(_z_timestamp_decode(&msg->ext_tstamp, &zbf));
            break;
        }
        case 0x04 | _Z_MSG_EXT_ENC_ZINT: {
            msg->ext_target = extension->_body._zint._val;
            if (msg->ext_target > 2) {
                return _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
            }
        } break;
        case 0x05 | _Z_MSG_EXT_ENC_ZINT: {
            msg->ext_budget = extension->_body._zint._val;
        } break;
        case 0x06 | _Z_MSG_EXT_ENC_ZINT: {
            msg->ext_timeout_ms = extension->_body._zint._val;
        } break;
        default:
            if ((extension->_header & _Z_MSG_EXT_FLAG_M) != 0) {
                return _z_msg_ext_unknown_error(extension, 0x16);
            }
    }
    return _Z_RES_OK;
}
int8_t _z_request_decode(_z_n_msg_request_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_RETURN_IF_ERR(_z_zint_decode(&msg->_rid, zbf));
    _Z_RETURN_IF_ERR(_z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_N_REQUEST_N)));
    msg->_key._uses_remote_mapping = !_Z_HAS_FLAG(header, _Z_FLAG_N_REQUEST_M);
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_request_decode_extensions, msg));
    }
    switch (_Z_MID(header)) {
        case _Z_MID_Z_QUERY: {
            _Z_RETURN_IF_ERR(_z_query_decode(&msg->_body._query, zbf, header));
        } break;
        case _Z_MID_Z_PUT: {
            _Z_RETURN_IF_ERR(_z_put_decode(&msg->_body.put, zbf, header));
        } break;
        case _Z_MID_Z_DEL: {
            _Z_RETURN_IF_ERR(_z_del_decode(&msg->_body.del, zbf, header));
        } break;
        case _Z_MID_Z_PULL: {
            _Z_RETURN_IF_ERR(_z_pull_decode(&msg->_body.pull, zbf, header));
        } break;
        default:
            return _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
    return _Z_RES_OK;
}

/*------------------ Response Message ------------------*/
int8_t _z_response_encode(_z_wbuf_t *wbf, const _z_n_msg_response_t *msg) {
    int8_t ret = _Z_RES_OK;
    uint8_t header = _Z_MID_N_RESPONSE;
    _Z_DEBUG("Encoding _Z_MID_N_RESPONSE\n");
    _Bool has_qos_ext = msg->_ext_qos._val != _Z_N_QOS_DEFAULT._val;
    _Bool has_ts_ext = _z_timestamp_check(&msg->_ext_timestamp);
    _Bool has_responder_ext = _z_id_check(msg->_ext_responder._zid);
    uint8_t n_ext = (has_qos_ext ? 1 : 0) + (has_ts_ext ? 1 : 0) + (has_responder_ext ? 1 : 0);
    if (msg->_key._uses_remote_mapping) {
        _Z_SET_FLAG(header, _Z_FLAG_N_RESPONSE_M);
    }
    if (msg->_key._suffix != NULL) {
        _Z_SET_FLAG(header, _Z_FLAG_N_RESPONSE_N);
    }
    if (n_ext != 0) {
        _Z_SET_FLAG(header, _Z_FLAG_Z_Z);
    }

    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_request_id));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_key._id));
    if (msg->_key._suffix != NULL) {
        _Z_RETURN_IF_ERR(_z_str_encode(wbf, msg->_key._suffix))
    }
    if (has_qos_ext) {
        n_ext -= 1;
        uint8_t extheader = _Z_MSG_EXT_ENC_ZINT | 0x01;
        if (n_ext != 0) {
            extheader |= _Z_FLAG_Z_Z;
        }
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_ext_qos._val));
    }
    if (has_ts_ext) {
        n_ext -= 1;
        uint8_t extheader = _Z_MSG_EXT_ENC_ZBUF | 0x02 | (n_ext != 0 ? _Z_FLAG_Z_Z : 0);
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &msg->_ext_timestamp));
    }
    if (has_responder_ext) {
        n_ext -= 1;
        uint8_t extheader = _Z_MSG_EXT_ENC_ZBUF | 0x02 | (n_ext != 0 ? _Z_FLAG_Z_Z : 0);
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        uint8_t zidlen = _z_id_len(msg->_ext_responder._zid);
        extheader = (zidlen - 1) << 4;
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, extheader));
        _Z_RETURN_IF_ERR(_z_wbuf_write_bytes(wbf, msg->_ext_responder._zid.id, 0, zidlen));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_ext_responder._eid));
    }

    switch (msg->_tag) {
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
        case _Z_MSG_EXT_ENC_ZBUF | 0x03: {
            _z_zbuf_t _zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            _z_zbuf_t *zbf = &_zbf;
            uint8_t header;
            _Z_RETURN_IF_ERR(_z_uint8_decode(&header, zbf));
            uint8_t zidlen = (header >> 4) + 1;
            _Z_RETURN_IF_ERR(_z_zbuf_read_exact(zbf, msg->_ext_responder._zid.id, zidlen));
            _Z_RETURN_IF_ERR(_z_zint32_decode(&msg->_ext_responder._eid, zbf));
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
    msg->_key._uses_remote_mapping = !_Z_HAS_FLAG(header, _Z_FLAG_N_RESPONSE_M);
    _Z_RETURN_IF_ERR(_z_zint_decode(&msg->_request_id, zbf));
    _Z_RETURN_IF_ERR(_z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_N_RESPONSE_N)));
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_response_decode_extension, msg));
    }

    uint8_t inner_header;
    _Z_RETURN_IF_ERR(_z_uint8_decode(&inner_header, zbf));

    switch (_Z_MID(inner_header)) {
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
int8_t _z_response_final_encode(_z_wbuf_t *wbf, const _z_n_msg_response_final_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_N_RESPONSE\n");
    uint8_t header = _Z_MID_N_RESPONSE_FINAL;
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, msg->_request_id));

    return ret;
}
int8_t _z_response_final_decode(_z_n_msg_response_final_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    (void)(header);
    int8_t ret = _Z_RES_OK;
    ret |= _z_zint_decode(&msg->_request_id, zbf);
    return ret;
}

int8_t _z_declare_encode(_z_wbuf_t *wbf, const _z_n_msg_declare_t *decl) {
    uint8_t header = _Z_MID_N_DECLARE;
    _Bool has_qos_ext = decl->_ext_qos._val != _Z_N_QOS_DEFAULT._val;
    _Bool has_timestamp_ext = _z_timestamp_check(&decl->_ext_timestamp);
    uint8_t n = (has_qos_ext ? 1 : 0) + (has_timestamp_ext ? 1 : 0);
    if (n != 0) {
        header |= _Z_FLAG_N_Z;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    if (has_qos_ext) {
        n -= 1;
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, 0x01 | _Z_MSG_EXT_ENC_ZINT | (n != 0 ? _Z_FLAG_Z_Z : 0)));
        _Z_RETURN_IF_ERR(_z_zint_encode(wbf, decl->_ext_qos._val));
    }
    if (has_timestamp_ext) {
        n -= 1;
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, 0x02 | _Z_MSG_EXT_ENC_ZBUF | (n != 0 ? _Z_FLAG_Z_Z : 0)));
        _Z_RETURN_IF_ERR(_z_timestamp_encode(wbf, &decl->_ext_timestamp));
    }
    return _z_declaration_encode(wbf, &decl->_decl);
}
int8_t _z_declare_decode_extensions(_z_msg_ext_t *extension, void *ctx) {
    _z_n_msg_declare_t *decl = (_z_n_msg_declare_t *)ctx;
    switch (_Z_EXT_FULL_ID(extension->_header)) {
        case _Z_MSG_EXT_ENC_ZINT | 0x01: {
            decl->_ext_qos._val = extension->_body._zint._val;
            break;
        }
        case _Z_MSG_EXT_ENC_ZBUF | 0x02: {
            _z_zbuf_t zbf = _z_zbytes_as_zbuf(extension->_body._zbuf._val);
            return _z_timestamp_decode(&decl->_ext_timestamp, &zbf);
        }
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                return _z_msg_ext_unknown_error(extension, 0x19);
            }
    }
    return _Z_RES_OK;
}
int8_t _z_declare_decode(_z_n_msg_declare_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    if (_Z_HAS_FLAG(header, _Z_FLAG_N_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_declare_decode_extensions, decl))
    }
    return _z_declaration_decode(&decl->_decl, zbf);
}

int8_t _z_network_message_encode(_z_wbuf_t *wbf, const _z_network_message_t *msg) {
    switch (msg->_tag) {
        case _Z_N_DECLARE: {
            return _z_declare_encode(wbf, &msg->_body._declare);
        } break;
        case _Z_N_PUSH: {
            return _z_push_encode(wbf, &msg->_body._push);
        } break;
        case _Z_N_REQUEST: {
            return _z_request_encode(wbf, &msg->_body._request);
        } break;
        case _Z_N_RESPONSE: {
            return _z_response_encode(wbf, &msg->_body._response);
        } break;
        case _Z_N_RESPONSE_FINAL: {
            return _z_response_final_encode(wbf, &msg->_body._response_final);
        } break;
    }
}
int8_t _z_network_message_decode(_z_network_message_t *msg, _z_zbuf_t *zbf) {
    uint8_t header;
    _Z_RETURN_IF_ERR(_z_uint8_decode(&header, zbf));
    switch (_Z_MID(header)) {
        case _Z_MID_N_DECLARE: {
            return _z_declare_decode(&msg->_body._declare, zbf, header);
        } break;
        case _Z_MID_N_PUSH: {
            return _z_push_decode(&msg->_body._push, zbf, header);
        } break;
        case _Z_MID_N_REQUEST: {
            return _z_request_decode(&msg->_body._request, zbf, header);
        } break;
        case _Z_MID_N_RESPONSE: {
            return _z_response_decode(&msg->_body._response, zbf, header);
        } break;
        case _Z_MID_N_RESPONSE_FINAL: {
            return _z_response_final_decode(&msg->_body._response_final, zbf, header);
        } break;
        default:
            return _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
}
