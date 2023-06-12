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

#include "zenoh-pico/protocol/msgcodec.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/extcodec.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

/*=============================*/
/*           Fields            */
/*=============================*/
/*------------------ Payload field ------------------*/
int8_t _z_payload_encode(_z_wbuf_t *wbf, const _z_payload_t *pld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _PAYLOAD\n");
    ret |= _z_bytes_encode(wbf, pld);

    return ret;
}

int8_t _z_payload_decode_na(_z_payload_t *pld, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _PAYLOAD\n");
    return _z_bytes_decode(pld, zbf);
}

int8_t _z_payload_decode(_z_payload_t *pld, _z_zbuf_t *zbf) { return _z_payload_decode_na(pld, zbf); }

int8_t _z_id_encode_as_zbytes(_z_wbuf_t *wbf, const _z_id_t *id) {
    int8_t ret = _Z_RES_OK;
    uint8_t len = _z_id_len(*id);

    if (id->id[len] != 0) {
        ret |= _z_wbuf_write(wbf, len);
        ret |= _z_wbuf_write_bytes(wbf, id->id, 0, len);
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
    _Z_DEBUG("Encoding _TIMESTAMP\n");

    _Z_EC(_z_uint64_encode(wbf, ts->time))
    ret |= _z_id_encode_as_zbytes(wbf, &ts->id);

    return ret;
}

int8_t _z_timestamp_decode_na(_z_timestamp_t *ts, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _TIMESTAMP\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint64_decode(&ts->time, zbf);
    ret |= _z_id_decode_as_zbytes(&ts->id, zbf);

    return ret;
}

int8_t _z_timestamp_decode(_z_timestamp_t *ts, _z_zbuf_t *zbf) { return _z_timestamp_decode_na(ts, zbf); }

/*------------------ ResKey Field ------------------*/
int8_t _z_keyexpr_encode(_z_wbuf_t *wbf, _Bool has_suffix, const _z_keyexpr_t *fld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _RESKEY\n");

    _Z_EC(_z_zint_encode(wbf, fld->_id))
    if (has_suffix == true) {
        _Z_EC(_z_str_encode(wbf, fld->_suffix))
    }

    return ret;
}

int8_t _z_keyexpr_decode(_z_keyexpr_t *ke, _z_zbuf_t *zbf, _Bool has_suffix) {
    _Z_DEBUG("Decoding _RESKEY\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&ke->_id, zbf);
    if (has_suffix == true) {
        char *str = NULL;
        ret |= _z_str_decode(&str, zbf);
        if (ret == _Z_RES_OK) {
            ke->_suffix = str;
        } else {
            ke->_suffix = NULL;
        }
    } else {
        ke->_suffix = NULL;
    }

    return ret;
}

/*------------------ Locators Field ------------------*/
int8_t _z_locators_encode(_z_wbuf_t *wbf, const _z_locator_array_t *la) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _LOCATORS\n");
    _Z_EC(_z_zint_encode(wbf, la->_len))
    for (size_t i = 0; i < la->_len; i++) {
        char *s = _z_locator_to_str(&la->_val[i]);
        _Z_EC(_z_str_encode(wbf, s))
        z_free(s);
    }

    return ret;
}

int8_t _z_locators_decode_na(_z_locator_array_t *a_loc, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _LOCATORS\n");
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
                z_free(str);
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

/*------------------ Data Info Field ------------------*/
int8_t _z_data_info_encode(_z_wbuf_t *wbf, const _z_data_info_t *fld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DATA_INFO\n");

    // Encode the flags
    // WARNING: we do not support sliced content in zenoh-pico.
    //          Disable the SLICED flag to be on the safe side.
    _Z_EC(_z_zint_encode(wbf, fld->_flags & ~_Z_DATA_INFO_SLICED))

    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_KIND) == true) {
        _Z_EC(_z_uint8_encode(wbf, fld->_kind))
    }
    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_ENC) == true) {
        _Z_EC(_z_encoding_prefix_encode(wbf, fld->_encoding.prefix))
        _Z_EC(_z_bytes_encode(wbf, &fld->_encoding.suffix))
    }
    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_TSTAMP) == true) {
        _Z_EC(_z_timestamp_encode(wbf, &fld->_tstamp))
    }
    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_SRC_ID) == true) {
        _Z_EC(_z_bytes_encode(wbf, &fld->_source_id))
    }
    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_SRC_SN) == true) {
        _Z_EC(_z_zint_encode(wbf, fld->_source_sn))
    }

    return ret;
}

int8_t _z_data_info_decode_na(_z_data_info_t *di, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _Z_DATA_INFO\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&di->_flags, zbf);
    // WARNING: we do not support sliced content in zenoh-pico.
    //          Return error in case the payload is sliced.
    if ((ret == _Z_RES_OK) && _Z_HAS_FLAG(di->_flags, _Z_DATA_INFO_SLICED) == false) {
        if (_Z_HAS_FLAG(di->_flags, _Z_DATA_INFO_KIND) == true) {
            ret |= _z_uint8_decode(&di->_kind, zbf);
        } else {
            di->_kind = Z_SAMPLE_KIND_PUT;
        }

        if (_Z_HAS_FLAG(di->_flags, _Z_DATA_INFO_ENC) == true) {
            ret |= _z_encoding_prefix_decode(&di->_encoding.prefix, zbf);
            ret |= _z_bytes_decode(&di->_encoding.suffix, zbf);
        } else {
            di->_encoding.prefix = Z_ENCODING_PREFIX_EMPTY;
            di->_encoding.suffix = _z_bytes_empty();
        }

        if (_Z_HAS_FLAG(di->_flags, _Z_DATA_INFO_TSTAMP) == true) {
            ret |= _z_timestamp_decode(&di->_tstamp, zbf);
        } else {
            _z_timestamp_reset(&di->_tstamp);
        }

        if (_Z_HAS_FLAG(di->_flags, _Z_DATA_INFO_SRC_ID) == true) {
            ret |= _z_bytes_decode(&di->_source_id, zbf);
        } else {
            di->_source_id = _z_bytes_empty();
        }

        if (_Z_HAS_FLAG(di->_flags, _Z_DATA_INFO_SRC_SN) == true) {
            ret |= _z_zint_decode(&di->_source_sn, zbf);
        } else {
            di->_source_sn = 0;
        }
    }

    return ret;
}

int8_t _z_data_info_decode(_z_data_info_t *di, _z_zbuf_t *zbf) { return _z_data_info_decode_na(di, zbf); }

/*------------------ Data Message ------------------*/
int8_t _z_data_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_data_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_Z_DATA\n");

    _Z_EC(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &msg->_key))

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_I) == true) {
        _Z_EC(_z_data_info_encode(wbf, &msg->_info))
    }
    _Z_EC(_z_payload_encode(wbf, &msg->_payload))

    return ret;
}

int8_t _z_data_decode_na(_z_msg_data_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_Z_DATA\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_I) == true) {
        ret |= _z_data_info_decode(&msg->_info, zbf);
    } else {
        msg->_info._flags = 0;
        msg->_info._kind = Z_SAMPLE_KIND_PUT;
        msg->_info._encoding.prefix = Z_ENCODING_PREFIX_EMPTY;
        msg->_info._encoding.suffix = _z_bytes_empty();
        msg->_info._source_id = _z_bytes_empty();
        msg->_info._source_sn = 0;
        _z_timestamp_reset(&msg->_info._tstamp);
    }
    ret |= _z_payload_decode(&msg->_payload, zbf);

    return ret;
}

int8_t _z_data_decode(_z_msg_data_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_data_decode_na(msg, zbf, header);
}

/*------------------ Pull Message ------------------*/
int8_t _z_pull_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_pull_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_Z_PULL\n");

    _Z_EC(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &msg->_key))

    _Z_EC(_z_zint_encode(wbf, msg->_pull_id))

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_N) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_max_samples))
    }

    return ret;
}

int8_t _z_pull_decode_na(_z_msg_pull_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_Z_PULL\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));
    ret |= _z_zint_decode(&msg->_pull_id, zbf);
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_N) == true) {
        ret |= _z_zint_decode(&msg->_max_samples, zbf);
    } else {
        msg->_max_samples = 1;  // FIXME: confirm default value
    }

    return ret;
}

int8_t _z_pull_decode(_z_msg_pull_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_pull_decode_na(msg, zbf, header);
}

/*------------------ Query Message ------------------*/
int8_t _z_query_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_query_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_Z_QUERY\n");

    _Z_EC(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &msg->_key))

    _Z_EC(_z_str_encode(wbf, msg->_parameters))

    _Z_EC(_z_zint_encode(wbf, msg->_qid))

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_T) == true) {
        _Z_EC(_z_query_target_encode(wbf, msg->_target))
    }
    ret |= _z_consolidation_mode_encode(wbf, msg->_consolidation);

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_B) == true) {
        _Z_EC(_z_data_info_encode(wbf, &msg->_info))
        _Z_EC(_z_payload_encode(wbf, &msg->_payload))
    }

    return ret;
}

int8_t _z_query_decode_na(_z_msg_query_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_Z_QUERY\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));
    ret |= _z_str_decode(&msg->_parameters, zbf);
    ret |= _z_zint_decode(&msg->_qid, zbf);
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_T) == true) {
        ret |= _z_query_target_decode(&msg->_target, zbf);
    } else {
        msg->_target = Z_QUERY_TARGET_BEST_MATCHING;
    }
    ret |= _z_consolidation_mode_decode(&msg->_consolidation, zbf);
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_I) == true) {
        ret |= _z_data_info_decode(&msg->_info, zbf);
        ret |= _z_payload_decode(&msg->_payload, zbf);
    } else {
        msg->_info._flags = 0;
        msg->_info._kind = Z_SAMPLE_KIND_PUT;
        msg->_info._encoding.prefix = Z_ENCODING_PREFIX_EMPTY;
        msg->_info._encoding.suffix = _z_bytes_empty();
        msg->_info._source_id = _z_bytes_empty();
        msg->_info._source_sn = 0;
        _z_timestamp_reset(&msg->_info._tstamp);
        msg->_payload = _z_bytes_empty();
    }

    return ret;
}

int8_t _z_query_decode(_z_msg_query_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_query_decode_na(msg, zbf, header);
}

/*------------------ Zenoh Message ------------------*/
int8_t _z_zenoh_message_encode(_z_wbuf_t *wbf, const _z_zenoh_message_t *msg) {
    int8_t ret = _Z_RES_OK;

    _Z_EC(_z_wbuf_write(wbf, msg->_header))

    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_Z_DATA: {
            ret |= _z_data_encode(wbf, msg->_header, &msg->_body._data);
        } break;

        case _Z_MID_Z_QUERY: {
            ret |= _z_query_encode(wbf, msg->_header, &msg->_body._query);
        } break;

        case _Z_MID_Z_PULL: {
            ret |= _z_pull_encode(wbf, msg->_header, &msg->_body._pull);
        } break;

        case _Z_MID_Z_UNIT: {
            // Do nothing. Unit messages have no body
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
            ret |= _Z_ERR_MESSAGE_ZENOH_UNKNOWN;
        } break;
    }

    return ret;
}

int8_t _z_zenoh_message_decode_na(_z_zenoh_message_t *msg, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _Bool is_last = false;
    do {
        ret |= _z_uint8_decode(&msg->_header, zbf);
        if (ret == _Z_RES_OK) {
            uint8_t mid = _Z_MID(msg->_header);
            switch (mid) {
                case _Z_MID_Z_DATA: {
                    ret |= _z_data_decode(&msg->_body._data, zbf, msg->_header);
                    is_last = true;
                } break;

                case _Z_MID_Z_QUERY: {
                    ret |= _z_query_decode(&msg->_body._query, zbf, msg->_header);
                    is_last = true;
                } break;

                case _Z_MID_Z_PULL: {
                    ret |= _z_pull_decode(&msg->_body._pull, zbf, msg->_header);
                    is_last = true;
                } break;

                case _Z_MID_Z_UNIT: {
                    // Do nothing. Unit messages have no body.
                    is_last = true;
                } break;

                case _Z_MID_Z_LINK_STATE_LIST: {
                    _Z_DEBUG("WARNING: Link state not supported in zenoh-pico\n");
                    is_last = true;
                } break;

                default: {
                    _Z_DEBUG("WARNING: Trying to decode zenoh message with unknown ID(%d)\n", mid);
                    ret |= _Z_ERR_MESSAGE_ZENOH_UNKNOWN;
                } break;
            }
        } else {
            msg->_header = 0xFF;
        }
    } while ((ret == _Z_RES_OK) && (is_last == false));

    return ret;
}

int8_t _z_zenoh_message_decode(_z_zenoh_message_t *msg, _z_zbuf_t *zbf) { return _z_zenoh_message_decode_na(msg, zbf); }

/*=============================*/
/*       Scouting Messages     */
/*=============================*/
/*------------------ Scout Message ------------------*/
int8_t _z_scout_encode(_z_wbuf_t *wbf, uint8_t header, const _z_s_msg_scout_t *msg) {
    int8_t ret = _Z_RES_OK;
    (void)(header);
    _Z_DEBUG("Encoding _Z_MID_SCOUT\n");

    _Z_EC(_z_uint8_encode(wbf, msg->_version))

    uint8_t cbyte = 0;
    cbyte |= (msg->_what & 0x07);
    uint8_t zid_len = _z_id_len(msg->_zid);
    if (zid_len > 0) {
        _Z_SET_FLAG(cbyte, _Z_FLAG_T_SCOUT_I);
        cbyte |= ((zid_len - 1) & 0x0F) << 4;
    }
    _Z_EC(_z_uint8_encode(wbf, cbyte))

    ret |= _z_wbuf_write_bytes(wbf, msg->_zid.id, 0, zid_len);

    return ret;
}

int8_t _z_scout_decode_na(_z_s_msg_scout_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    int8_t ret = _Z_RES_OK;
    (void)(header);
    _Z_DEBUG("Decoding _Z_MID_SCOUT\n");

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

int8_t _z_scout_decode(_z_s_msg_scout_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_scout_decode_na(msg, zbf, header);
}

/*------------------ Hello Message ------------------*/
int8_t _z_hello_encode(_z_wbuf_t *wbf, uint8_t header, const _z_s_msg_hello_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_HELLO\n");

    _Z_EC(_z_uint8_encode(wbf, msg->_version))
    uint8_t zidlen = _z_id_len(msg->_zid);
    uint8_t cbyte = 0;
    cbyte |= (msg->_whatami & 0x03);
    cbyte |= ((zidlen - 1) & 0x0F) << 4;
    _Z_EC(_z_uint8_encode(wbf, cbyte))
    _Z_EC(_z_bytes_val_encode(wbf, &(_z_bytes_t){.start = msg->_zid.id, .len = zidlen, ._is_alloc = false}));

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_HELLO_L) == true) {
        _Z_EC(_z_locators_encode(wbf, &msg->_locators))
    }

    return ret;
}

int8_t _z_hello_decode_na(_z_s_msg_hello_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_HELLO\n");
    int8_t ret = _Z_RES_OK;

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

    // Encode the decorators if present

    uint8_t header = msg->_header;
    // size_t n_ext = _z_msg_ext_vec_len(&msg->_extensions);
    // if (n_ext > 0) {
    //     header |= _Z_FLAG_T_Z;
    // }

    _Z_EC(_z_wbuf_write(wbf, header))
    switch (_Z_MID(msg->_header)) {
        case _Z_MID_SCOUT: {
            ret |= _z_scout_encode(wbf, msg->_header, &msg->_body._scout);
        } break;

        case _Z_MID_HELLO: {
            ret |= _z_hello_encode(wbf, msg->_header, &msg->_body._hello);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to encode session message with unknown ID(%d)\n", _Z_MID(msg->_header));
            ret |= _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
        } break;
    }

    // ret |= _z_msg_ext_vec_encode(wbf, &msg->_extensions);

    return ret;
}
int8_t _z_scouting_message_decode_na(_z_scouting_message_t *msg, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

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
                    _Z_DEBUG("WARNING: Trying to decode session message with unknown ID(%d)\n", mid);
                    ret |= _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
                    is_last = true;
                } break;
            }
        } else {
            msg->_header = 0xFF;
        }
    } while ((ret == _Z_RES_OK) && (is_last == false));

    if ((ret == _Z_RES_OK) && (msg->_header & _Z_MSG_EXT_FLAG_Z) != 0) {
        ret |= _z_msg_ext_skip_non_mandatories(zbf);
    }

    return ret;
}

int8_t _z_scouting_message_decode(_z_scouting_message_t *s_msg, _z_zbuf_t *zbf) {
    return _z_scouting_message_decode_na(s_msg, zbf);
}