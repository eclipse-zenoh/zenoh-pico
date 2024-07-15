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

#include "zenoh-pico/protocol/definitions/transport.h"

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/codec/ext.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/definitions/core.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

uint8_t _z_whatami_to_uint8(z_whatami_t whatami) {
    return (whatami >> 1) & 0x03;  // get set bit index; only first 3 bits can be set
}

z_whatami_t _z_whatami_from_uint8(uint8_t b) {
    return 1 << (b & 0x03);  // convert set bit idx into bitmask
}

/*------------------ Join Message ------------------*/
int8_t _z_join_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_join_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_T_JOIN");

    _Z_RETURN_IF_ERR(_z_wbuf_write(wbf, msg->_version));

    uint8_t cbyte = 0;
    cbyte |= _z_whatami_to_uint8(msg->_whatami);
    uint8_t zidlen = _z_id_len(msg->_zid);
    cbyte |= (uint8_t)(((zidlen - 1) & 0x0F) << 4);
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, cbyte));
    _Z_RETURN_IF_ERR(_z_wbuf_write_bytes(wbf, msg->_zid.id, 0, zidlen));

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_JOIN_S)) {
        cbyte = 0;
        cbyte |= (msg->_seq_num_res & 0x03);
        cbyte |= ((msg->_req_id_res & 0x03) << 2);
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, cbyte));
        _Z_RETURN_IF_ERR(_z_uint16_encode(wbf, msg->_batch_size));
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_JOIN_T) == true) {
        _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_lease / 1000));
    } else {
        _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_lease));
    }
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_next_sn._val._plain._reliable));
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_next_sn._val._plain._best_effort));
    if (msg->_next_sn._is_qos) {
        if (_Z_HAS_FLAG(header, _Z_FLAG_T_Z)) {
            _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZBUF | _Z_MSG_EXT_FLAG_M | 1));
            size_t len = 0;
            for (uint8_t i = 0; (i < Z_PRIORITIES_NUM) && (ret == _Z_RES_OK); i++) {
                len += _z_zint_len(msg->_next_sn._val._qos[i]._reliable) +
                       _z_zint_len(msg->_next_sn._val._qos[i]._best_effort);
            }
            _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, len));
            for (uint8_t i = 0; (i < Z_PRIORITIES_NUM) && (ret == _Z_RES_OK); i++) {
                _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_next_sn._val._qos[i]._reliable));
                _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_next_sn._val._qos[i]._best_effort));
            }
        } else {
            _Z_DEBUG("Attempted to serialize QoS-SN extension, but the header extension flag was unset");
            ret |= _Z_ERR_MESSAGE_SERIALIZATION_FAILED;
        }
    }

    return ret;
}

int8_t _z_join_decode_ext(_z_msg_ext_t *extension, void *ctx) {
    int8_t ret = _Z_RES_OK;
    _z_t_msg_join_t *msg = (_z_t_msg_join_t *)ctx;
    if (_Z_EXT_FULL_ID(extension->_header) ==
        (_Z_MSG_EXT_ENC_ZBUF | _Z_MSG_EXT_FLAG_M | 1)) {  // QOS: (enc=zbuf)(mandatory=true)(id=1)
        msg->_next_sn._is_qos = true;
        _z_zbuf_t zbf = _z_slice_as_zbuf(extension->_body._zbuf._val);
        for (int i = 0; (ret == _Z_RES_OK) && (i < Z_PRIORITIES_NUM); ++i) {
            ret |= _z_zsize_decode(&msg->_next_sn._val._qos[i]._reliable, &zbf);
            ret |= _z_zsize_decode(&msg->_next_sn._val._qos[i]._best_effort, &zbf);
        }
    } else if (_Z_MSG_EXT_IS_MANDATORY(extension->_header)) {
        ret = _Z_ERR_MESSAGE_EXTENSION_MANDATORY_AND_UNKNOWN;
    }
    return ret;
}

int8_t _z_join_decode(_z_t_msg_join_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_T_JOIN");
    int8_t ret = _Z_RES_OK;
    *msg = (_z_t_msg_join_t){0};

    ret |= _z_uint8_decode(&msg->_version, zbf);

    uint8_t cbyte = 0;
    ret |= _z_uint8_decode(&cbyte, zbf);
    msg->_whatami = _z_whatami_from_uint8(cbyte);

    uint8_t zidlen = ((cbyte & 0xF0) >> 4) + (uint8_t)1;
    msg->_zid = _z_id_empty();
    if (ret == _Z_RES_OK) {
        if (_z_zbuf_len(zbf) >= zidlen) {
            _z_zbuf_read_bytes(zbf, msg->_zid.id, 0, zidlen);
        } else {
            ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    }
    if (ret == _Z_RES_OK) {
        if (_Z_HAS_FLAG(header, _Z_FLAG_T_JOIN_S) == true) {
            cbyte = 0;
            ret |= _z_uint8_decode(&cbyte, zbf);
            msg->_seq_num_res = (cbyte & 0x03);
            msg->_req_id_res = ((cbyte >> 2) & 0x03);
            ret |= _z_uint16_decode(&msg->_batch_size, zbf);
        } else {
            msg->_seq_num_res = _Z_DEFAULT_RESOLUTION_SIZE;
            msg->_req_id_res = _Z_DEFAULT_RESOLUTION_SIZE;
            msg->_batch_size = _Z_DEFAULT_MULTICAST_BATCH_SIZE;
        }
    }
    if (ret == _Z_RES_OK) {
        ret |= _z_zsize_decode(&msg->_lease, zbf);
        if (_Z_HAS_FLAG(header, _Z_FLAG_T_JOIN_T) == true) {
            msg->_lease = msg->_lease * 1000;
        }
    }
    if (ret == _Z_RES_OK) {
        msg->_next_sn._is_qos = false;
        ret |= _z_zsize_decode(&msg->_next_sn._val._plain._reliable, zbf);
        ret |= _z_zsize_decode(&msg->_next_sn._val._plain._best_effort, zbf);
    }
    if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(header, _Z_FLAG_T_Z)) {
        ret |= _z_msg_ext_decode_iter(zbf, _z_join_decode_ext, msg);
    }

    return ret;
}

/*------------------ Init Message ------------------*/
int8_t _z_init_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_init_t *msg) {
    _Z_DEBUG("Encoding _Z_MID_T_INIT");
    int8_t ret = _Z_RES_OK;

    _Z_RETURN_IF_ERR(_z_wbuf_write(wbf, msg->_version))

    uint8_t cbyte = 0;
    cbyte |= _z_whatami_to_uint8(msg->_whatami);
    uint8_t zidlen = _z_id_len(msg->_zid);
    cbyte |= (uint8_t)(((zidlen - 1) & 0x0F) << 4);  // TODO[protocol]: check if ZID > 0 && <= 16
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, cbyte))
    _Z_RETURN_IF_ERR(_z_wbuf_write_bytes(wbf, msg->_zid.id, 0, zidlen))

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_INIT_S) == true) {
        cbyte = 0;
        cbyte |= (msg->_seq_num_res & 0x03);
        cbyte |= ((msg->_req_id_res & 0x03) << 2);
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, cbyte))
        _Z_RETURN_IF_ERR(_z_uint16_encode(wbf, msg->_batch_size))
    }

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_INIT_A) == true) {
        _Z_RETURN_IF_ERR(_z_slice_encode(wbf, &msg->_cookie))
    }

    return ret;
}

int8_t _z_init_decode(_z_t_msg_init_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_T_INIT");
    *msg = (_z_t_msg_init_t){0};
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint8_decode(&msg->_version, zbf);

    uint8_t cbyte = 0;
    ret |= _z_uint8_decode(&cbyte, zbf);
    msg->_zid = _z_id_empty();

    if (ret == _Z_RES_OK) {
        msg->_whatami = _z_whatami_from_uint8(cbyte);
        uint8_t zidlen = ((cbyte & 0xF0) >> 4) + (uint8_t)1;
        if (_z_zbuf_len(zbf) >= zidlen) {
            _z_zbuf_read_bytes(zbf, msg->_zid.id, 0, zidlen);
        } else {
            ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    }

    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_INIT_S) == true)) {
        cbyte = 0;
        ret |= _z_uint8_decode(&cbyte, zbf);
        msg->_seq_num_res = (cbyte & 0x03);
        msg->_req_id_res = ((cbyte >> 2) & 0x03);
        ret |= _z_uint16_decode(&msg->_batch_size, zbf);
    } else {
        msg->_seq_num_res = _Z_DEFAULT_RESOLUTION_SIZE;
        msg->_req_id_res = _Z_DEFAULT_RESOLUTION_SIZE;
        msg->_batch_size = _Z_DEFAULT_UNICAST_BATCH_SIZE;
    }

    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_INIT_A) == true)) {
        ret |= _z_slice_decode(&msg->_cookie, zbf);
    } else {
        msg->_cookie = _z_slice_empty();
    }

    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_Z) == true)) {
        ret |= _z_msg_ext_skip_non_mandatories(zbf, 0x01);
    }

    return ret;
}

/*------------------ Open Message ------------------*/
int8_t _z_open_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_open_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_T_OPEN");

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_OPEN_T) == true) {
        _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_lease / 1000))
    } else {
        _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_lease))
    }

    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_initial_sn))

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_OPEN_A) == false) {
        _Z_RETURN_IF_ERR(_z_slice_encode(wbf, &msg->_cookie))
    }

    return ret;
}

int8_t _z_open_decode(_z_t_msg_open_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_T_OPEN");
    int8_t ret = _Z_RES_OK;
    *msg = (_z_t_msg_open_t){0};

    ret |= _z_zsize_decode(&msg->_lease, zbf);
    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_OPEN_T) == true)) {
        msg->_lease = msg->_lease * 1000;
    }

    ret |= _z_zsize_decode(&msg->_initial_sn, zbf);

    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_OPEN_A) == false)) {
        ret |= _z_slice_decode(&msg->_cookie, zbf);
        if (ret != _Z_RES_OK) {
            msg->_cookie = _z_slice_empty();
        }
    } else {
        msg->_cookie = _z_slice_empty();
    }
    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_Z) == true)) {
        ret |= _z_msg_ext_skip_non_mandatories(zbf, 0x02);
    }

    return ret;
}

/*------------------ Close Message ------------------*/
int8_t _z_close_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_close_t *msg) {
    (void)(header);
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_T_CLOSE");

    ret |= _z_wbuf_write(wbf, msg->_reason);

    return ret;
}

int8_t _z_close_decode(_z_t_msg_close_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    (void)(header);
    int8_t ret = _Z_RES_OK;
    *msg = (_z_t_msg_close_t){0};
    _Z_DEBUG("Decoding _Z_MID_T_CLOSE");

    ret |= _z_uint8_decode(&msg->_reason, zbf);

    return ret;
}

/*------------------ Keep Alive Message ------------------*/
int8_t _z_keep_alive_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_keep_alive_t *msg) {
    (void)(wbf);
    (void)(header);
    (void)(msg);

    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_T_KEEP_ALIVE");

    return ret;
}

int8_t _z_keep_alive_decode(_z_t_msg_keep_alive_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    (void)(msg);
    (void)(zbf);
    (void)(header);
    *msg = (_z_t_msg_keep_alive_t){0};

    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Decoding _Z_MID_T_KEEP_ALIVE");

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        ret |= _z_msg_ext_skip_non_mandatories(zbf, 0x03);
    }

    return ret;
}

/*------------------ Frame Message ------------------*/

int8_t _z_frame_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_frame_t *msg) {
    int8_t ret = _Z_RES_OK;

    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_sn))

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_Z)) {
        ret = _Z_ERR_MESSAGE_SERIALIZATION_FAILED;
    }
    if (ret == _Z_RES_OK) {
        size_t len = _z_network_message_vec_len(&msg->_messages);
        for (size_t i = 0; i < len; i++) {
            _Z_RETURN_IF_ERR(_z_network_message_encode(wbf, _z_network_message_vec_get(&msg->_messages, i)))
        }
    }

    return ret;
}

int8_t _z_frame_decode(_z_t_msg_frame_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    int8_t ret = _Z_RES_OK;
    *msg = (_z_t_msg_frame_t){0};

    ret |= _z_zsize_decode(&msg->_sn, zbf);
    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_Z) == true)) {
        ret |= _z_msg_ext_skip_non_mandatories(zbf, 0x04);
    }
    if (ret == _Z_RES_OK) {
        msg->_messages = _z_network_message_vec_make(_ZENOH_PICO_FRAME_MESSAGES_VEC_SIZE);
        while (_z_zbuf_len(zbf) > 0) {
            // Mark the reading position of the iobfer
            size_t r_pos = _z_zbuf_get_rpos(zbf);
            _z_network_message_t *nm = (_z_network_message_t *)z_malloc(sizeof(_z_network_message_t));
            memset(nm, 0, sizeof(_z_network_message_t));
            ret |= _z_network_message_decode(nm, zbf);
            if (ret == _Z_RES_OK) {
                _z_network_message_vec_append(&msg->_messages, nm);
            } else {
                _z_n_msg_free(&nm);

                _z_zbuf_set_rpos(zbf, r_pos);  // Restore the reading position of the iobfer

                // FIXME: Check for the return error, since not all of them means a decoding error
                //        in this particular case. As of now, we roll-back the reading position
                //        and return to the Zenoh transport-level decoder.
                //        https://github.com/eclipse-zenoh/zenoh-pico/pull/132#discussion_r1045593602
                if ((ret & _Z_ERR_MESSAGE_ZENOH_UNKNOWN) == _Z_ERR_MESSAGE_ZENOH_UNKNOWN) {
                    ret = _Z_RES_OK;
                }
                break;
            }
        }
    }
    return ret;
}

/*------------------ Fragment Message ------------------*/
int8_t _z_fragment_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_fragment_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_TRANSPORT_FRAGMENT");
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, msg->_sn))
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_Z)) {
        ret = _Z_ERR_MESSAGE_SERIALIZATION_FAILED;
    }
    if (ret == _Z_RES_OK && _z_slice_check(&msg->_payload)) {
        _Z_RETURN_IF_ERR(_z_wbuf_write_bytes(wbf, msg->_payload.start, 0, msg->_payload.len));
    }

    return ret;
}

int8_t _z_fragment_decode(_z_t_msg_fragment_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    int8_t ret = _Z_RES_OK;
    *msg = (_z_t_msg_fragment_t){0};

    _Z_DEBUG("Decoding _Z_TRANSPORT_FRAGMENT");
    ret |= _z_zsize_decode(&msg->_sn, zbf);

    if ((ret == _Z_RES_OK) && (_Z_HAS_FLAG(header, _Z_FLAG_T_Z) == true)) {
        ret |= _z_msg_ext_skip_non_mandatories(zbf, 0x05);
    }

    _z_slice_t slice = _z_slice_wrap((uint8_t *)_z_zbuf_start(zbf), _z_zbuf_len(zbf));
    _z_slice_copy(&msg->_payload, &slice);
    zbf->_ios._r_pos = zbf->_ios._w_pos;

    return ret;
}

/*------------------ Transport Extensions Message ------------------*/
int8_t _z_extensions_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_ext_vec_t *v_ext) {
    (void)(header);
    int8_t ret = _Z_RES_OK;

    _Z_DEBUG("Encoding _Z_TRANSPORT_EXTENSIONS");
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_Z) == true) {
        ret |= _z_msg_ext_vec_encode(wbf, v_ext);
    }

    return ret;
}

int8_t _z_extensions_decode(_z_msg_ext_vec_t *v_ext, _z_zbuf_t *zbf, uint8_t header) {
    (void)(header);
    int8_t ret = _Z_RES_OK;

    _Z_DEBUG("Decoding _Z_TRANSPORT_EXTENSIONS");
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_Z) == true) {
        ret |= _z_msg_ext_vec_decode(v_ext, zbf);
    } else {
        _z_msg_ext_vec_reset(v_ext);
    }

    return ret;
}

/*------------------ Transport Message ------------------*/
int8_t _z_transport_message_encode(_z_wbuf_t *wbf, const _z_transport_message_t *msg) {
    int8_t ret = _Z_RES_OK;

    uint8_t header = msg->_header;

    _Z_RETURN_IF_ERR(_z_wbuf_write(wbf, header))
    switch (_Z_MID(msg->_header)) {
        case _Z_MID_T_FRAME: {
            ret |= _z_frame_encode(wbf, msg->_header, &msg->_body._frame);
        } break;
        case _Z_MID_T_FRAGMENT: {
            ret |= _z_fragment_encode(wbf, msg->_header, &msg->_body._fragment);
        } break;
        case _Z_MID_T_KEEP_ALIVE: {
            ret |= _z_keep_alive_encode(wbf, msg->_header, &msg->_body._keep_alive);
        } break;
        case _Z_MID_T_JOIN: {
            ret |= _z_join_encode(wbf, msg->_header, &msg->_body._join);
        } break;
        case _Z_MID_T_INIT: {
            ret |= _z_init_encode(wbf, msg->_header, &msg->_body._init);
        } break;
        case _Z_MID_T_OPEN: {
            ret |= _z_open_encode(wbf, msg->_header, &msg->_body._open);
        } break;
        case _Z_MID_T_CLOSE: {
            ret |= _z_close_encode(wbf, msg->_header, &msg->_body._close);
        } break;
        default: {
            _Z_DEBUG("WARNING: Trying to encode session message with unknown ID(%d)", _Z_MID(msg->_header));
            ret |= _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
        } break;
    }

    return ret;
}

int8_t _z_transport_message_decode(_z_transport_message_t *msg, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint8_decode(&msg->_header, zbf);  // Decode the header
    if (ret == _Z_RES_OK) {
        uint8_t mid = _Z_MID(msg->_header);
        switch (mid) {
            case _Z_MID_T_FRAME: {
                ret |= _z_frame_decode(&msg->_body._frame, zbf, msg->_header);
            } break;
            case _Z_MID_T_FRAGMENT: {
                ret |= _z_fragment_decode(&msg->_body._fragment, zbf, msg->_header);
            } break;
            case _Z_MID_T_KEEP_ALIVE: {
                ret |= _z_keep_alive_decode(&msg->_body._keep_alive, zbf, msg->_header);
            } break;
            case _Z_MID_T_JOIN: {
                ret |= _z_join_decode(&msg->_body._join, zbf, msg->_header);
            } break;
            case _Z_MID_T_INIT: {
                ret |= _z_init_decode(&msg->_body._init, zbf, msg->_header);
            } break;
            case _Z_MID_T_OPEN: {
                ret |= _z_open_decode(&msg->_body._open, zbf, msg->_header);
            } break;
            case _Z_MID_T_CLOSE: {
                ret |= _z_close_decode(&msg->_body._close, zbf, msg->_header);
            } break;
            default: {
                _Z_DEBUG("WARNING: Trying to decode session message with unknown ID(0x%x) (header=0x%x)", mid,
                         msg->_header);
                ret |= _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
            } break;
        }
    } else {
        msg->_header = 0xFF;
    }

    return ret;
}
