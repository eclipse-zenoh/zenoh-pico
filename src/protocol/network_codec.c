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

#include <stdint.h>

#include "zenoh-pico/protocol/extcodec.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

/*------------------ SubMode Field ------------------*/
int8_t _z_subinfo_encode(_z_wbuf_t *wbf, const _z_subinfo_t *fld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _SUB_MODE\n");

    uint8_t header = fld->mode;
    if ((fld->period.origin != 0) || (fld->period.period != 0) || (fld->period.duration != 0)) {
        _Z_SET_FLAG(header, _Z_FLAG_Z_P);
        _Z_EC(_z_wbuf_write(wbf, header))
        ret |= _z_period_encode(wbf, &fld->period);
    } else {
        _Z_EC(_z_wbuf_write(wbf, header))
    }

    return ret;
}

int8_t _z_subinfo_decode_na(_z_subinfo_t *si, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _SUB_MODE\n");
    int8_t ret = _Z_RES_OK;

    // Decode the header
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_R) == true) {
        si->reliability = Z_RELIABILITY_RELIABLE;
    } else {
        si->reliability = Z_RELIABILITY_BEST_EFFORT;
    }

    uint8_t h_subifo;
    ret |= _z_uint8_decode(&h_subifo, zbf);
    si->mode = _Z_MID(h_subifo);
    if (_Z_HAS_FLAG(h_subifo, _Z_FLAG_Z_P) == true) {
        ret |= _z_period_decode(&si->period, zbf);
    } else {
        si->period = (_z_period_t){.origin = 0, .period = 0, .duration = 0};
    }

    return ret;
}

int8_t _z_subinfo_decode(_z_subinfo_t *si, _z_zbuf_t *zbf, uint8_t header) {
    return _z_subinfo_decode_na(si, zbf, header);
}

/*------------------ Resource Declaration ------------------*/
int8_t _z_res_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_res_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_RESOURCE\n");

    _Z_EC(_z_zint_encode(wbf, dcl->_id))
    ret |= _z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &dcl->_key);

    return ret;
}

int8_t _z_res_decl_decode_na(_z_res_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_DECL_RESOURCE\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&dcl->_id, zbf);
    ret |= _z_keyexpr_decode(&dcl->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));

    return ret;
}

int8_t _z_res_decl_decode(_z_res_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    return _z_res_decl_decode_na(dcl, zbf, header);
}

/*------------------ Publisher Declaration ------------------*/
int8_t _z_pub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_pub_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_PUBLISHER\n");

    ret |= _z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &dcl->_key);

    return ret;
}

int8_t _z_pub_decl_decode_na(_z_pub_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_DECL_PUBLISHER\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&dcl->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));

    return ret;
}

int8_t _z_pub_decl_decode(_z_pub_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    return _z_pub_decl_decode_na(dcl, zbf, header);
}

/*------------------ Subscriber Declaration ------------------*/
int8_t _z_sub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_sub_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_SUBSCRIBER\n");

    _Z_EC(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &dcl->_key))
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_S) == true) {
        ret |= _z_subinfo_encode(wbf, &dcl->_subinfo);
    }

    return ret;
}

int8_t _z_sub_decl_decode_na(_z_sub_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_DECL_SUBSCRIBER\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&dcl->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_S) == true) {
        ret |= _z_subinfo_decode(&dcl->_subinfo, zbf, header);
    } else {
        dcl->_subinfo.mode = Z_SUBMODE_PUSH;  // Default subscription mode is non-periodic PUSH
        dcl->_subinfo.period = (_z_period_t){.origin = 0, .period = 0, .duration = 0};
        if (_Z_HAS_FLAG(header, _Z_FLAG_Z_R) == true) {
            dcl->_subinfo.reliability = Z_RELIABILITY_RELIABLE;
        } else {
            dcl->_subinfo.reliability = Z_RELIABILITY_BEST_EFFORT;
        }
    }

    return ret;
}

int8_t _z_sub_decl_decode(_z_sub_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    return _z_sub_decl_decode_na(dcl, zbf, header);
}

/*------------------ Queryable Declaration ------------------*/
int8_t _z_qle_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_qle_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_QUERYABLE\n");

    _Z_EC(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &dcl->_key));

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Q) == true) {
        _Z_EC(_z_zint_encode(wbf, dcl->_complete));
        _Z_EC(_z_zint_encode(wbf, dcl->_distance));
    }

    return ret;
}

int8_t _z_qle_decl_decode_na(_z_qle_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_DECL_QUERYABLE\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&dcl->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Q) == true) {
        ret |= _z_zint_decode(&dcl->_complete, zbf);
        ret |= _z_zint_decode(&dcl->_distance, zbf);
    } else {
        dcl->_complete = 0;
        dcl->_distance = 0;
    }

    return ret;
}

int8_t _z_qle_decl_decode(_z_qle_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    return _z_qle_decl_decode_na(dcl, zbf, header);
}

/*------------------ Forget Resource Declaration ------------------*/
int8_t _z_forget_res_decl_encode(_z_wbuf_t *wbf, const _z_forget_res_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_FORGET_RESOURCE\n");

    ret |= _z_zint_encode(wbf, dcl->_rid);

    return ret;
}

int8_t _z_forget_res_decl_decode_na(_z_forget_res_decl_t *dcl, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _Z_DECL_FORGET_RESOURCE\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&dcl->_rid, zbf);

    return ret;
}

int8_t _z_forget_res_decl_decode(_z_forget_res_decl_t *dcl, _z_zbuf_t *zbf) {
    return _z_forget_res_decl_decode_na(dcl, zbf);
}

/*------------------ Forget Publisher Declaration ------------------*/
int8_t _z_forget_pub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_forget_pub_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_FORGET_PUBLISHER\n");

    ret |= _z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &dcl->_key);

    return ret;
}

int8_t _z_forget_pub_decl_decode_na(_z_forget_pub_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_DECL_FORGET_PUBLISHER\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&dcl->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));

    return ret;
}

int8_t _z_forget_pub_decl_decode(_z_forget_pub_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    return _z_forget_pub_decl_decode_na(dcl, zbf, header);
}

/*------------------ Forget Subscriber Declaration ------------------*/
int8_t _z_forget_sub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_forget_sub_decl_t *dcl) {
    _Z_DEBUG("Encoding _Z_DECL_FORGET_PUBLISHER\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &dcl->_key);

    return ret;
}

int8_t _z_forget_sub_decl_decode_na(_z_forget_sub_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_DECL_FORGET_PUBLISHER\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&dcl->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));

    return ret;
}

int8_t _z_forget_sub_decl_decode(_z_forget_sub_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    return _z_forget_sub_decl_decode_na(dcl, zbf, header);
}

/*------------------ Forget Queryable Declaration ------------------*/
int8_t _z_forget_qle_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_forget_qle_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_FORGET_QUERYABLE\n");

    ret |= _z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &dcl->_key);

    return ret;
}

int8_t _z_forget_qle_decl_decode_na(_z_forget_qle_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_DECL_FORGET_QUERYABLE\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&dcl->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));

    return ret;
}

int8_t _z_forget_qle_decl_decode(_z_forget_qle_decl_t *dcl, _z_zbuf_t *zbf, uint8_t header) {
    return _z_forget_qle_decl_decode_na(dcl, zbf, header);
}

/*------------------ Declaration Field ------------------*/
int8_t _z_declaration_encode(_z_wbuf_t *wbf, const _z_declaration_t *dcl) {
    int8_t ret = _Z_RES_OK;

    _Z_EC(_z_wbuf_write(wbf, dcl->_header))
    uint8_t did = _Z_MID(dcl->_header);
    switch (did) {
        case _Z_DECL_RESOURCE: {
            ret |= _z_res_decl_encode(wbf, dcl->_header, &dcl->_body._res);
        } break;

        case _Z_DECL_PUBLISHER: {
            ret |= _z_pub_decl_encode(wbf, dcl->_header, &dcl->_body._pub);
        } break;

        case _Z_DECL_SUBSCRIBER: {
            ret |= _z_sub_decl_encode(wbf, dcl->_header, &dcl->_body._sub);
        } break;

        case _Z_DECL_QUERYABLE: {
            ret |= _z_qle_decl_encode(wbf, dcl->_header, &dcl->_body._qle);
        } break;

        case _Z_DECL_FORGET_RESOURCE: {
            ret |= _z_forget_res_decl_encode(wbf, &dcl->_body._forget_res);
        } break;

        case _Z_DECL_FORGET_PUBLISHER: {
            ret |= _z_forget_pub_decl_encode(wbf, dcl->_header, &dcl->_body._forget_pub);
        } break;

        case _Z_DECL_FORGET_SUBSCRIBER: {
            ret |= _z_forget_sub_decl_encode(wbf, dcl->_header, &dcl->_body._forget_sub);
        } break;

        case _Z_DECL_FORGET_QUERYABLE: {
            ret |= _z_forget_qle_decl_encode(wbf, dcl->_header, &dcl->_body._forget_qle);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to encode declaration with unknown ID(%d)\n", did);
            ret |= _Z_ERR_MESSAGE_SERIALIZATION_FAILED;
        } break;
    }

    return ret;
}

int8_t _z_declaration_decode_na(_z_declaration_t *decl, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint8_decode(&decl->_header, zbf);
    if (ret == _Z_RES_OK) {
        uint8_t mid = _Z_MID(decl->_header);

        switch (mid) {
            case _Z_DECL_RESOURCE: {
                ret |= _z_res_decl_decode(&decl->_body._res, zbf, decl->_header);
            } break;

            case _Z_DECL_PUBLISHER: {
                ret |= _z_pub_decl_decode(&decl->_body._pub, zbf, decl->_header);
            } break;

            case _Z_DECL_SUBSCRIBER: {
                ret |= _z_sub_decl_decode(&decl->_body._sub, zbf, decl->_header);
            } break;

            case _Z_DECL_QUERYABLE: {
                ret |= _z_qle_decl_decode(&decl->_body._qle, zbf, decl->_header);
            } break;

            case _Z_DECL_FORGET_RESOURCE: {
                ret |= _z_forget_res_decl_decode(&decl->_body._forget_res, zbf);
            } break;

            case _Z_DECL_FORGET_PUBLISHER: {
                ret |= _z_forget_pub_decl_decode(&decl->_body._forget_pub, zbf, decl->_header);
            } break;

            case _Z_DECL_FORGET_SUBSCRIBER: {
                ret |= _z_forget_sub_decl_decode(&decl->_body._forget_sub, zbf, decl->_header);
            } break;

            case _Z_DECL_FORGET_QUERYABLE: {
                ret |= _z_forget_qle_decl_decode(&decl->_body._forget_qle, zbf, decl->_header);
            } break;

            default: {
                _Z_DEBUG("WARNING: Trying to decode declaration with unknown ID(%d)\n", mid);
                ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
            } break;
        }
    }

    return ret;
}

int8_t _z_declaration_decode(_z_declaration_t *decl, _z_zbuf_t *zbf) { return _z_declaration_decode_na(decl, zbf); }

/*------------------ Declaration Message ------------------*/
int8_t _z_declare_encode(_z_wbuf_t *wbf, const _z_n_msg_declare_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_N_DECLARE\n");

    _Z_EC(_z_declaration_encode(wbf, &msg->_declaration));

    return ret;
}

int8_t _z_declare_decode_na(_z_n_msg_declare_t *msg, _z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _Z_MID_N_DECLARE\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_declaration_decode_na(&msg->_declaration, zbf);
    if (ret != _Z_RES_OK) {
        // TODO[protocol]: Add empty declaration
    }

    return ret;
}

int8_t _z_declare_decode(_z_n_msg_declare_t *msg, _z_zbuf_t *zbf) { return _z_declare_decode_na(msg, zbf); }

/*------------------ Push Body Field ------------------*/
int8_t _z_push_body_encode(_z_wbuf_t *wbf, const _z_push_body_t *pshb) {
    (void)(wbf);
    (void)(pshb);
    int8_t ret = _Z_RES_OK;

    return ret;
}

int8_t _z_push_body_decode_na(_z_push_body_t *pshb, _z_zbuf_t *zbf) {
    (void)(zbf);
    (void)(pshb);
    int8_t ret = _Z_RES_OK;

    return ret;
}

int8_t _z_push_body_decode(_z_push_body_t *pshb, _z_zbuf_t *zbf) { return _z_push_body_decode_na(pshb, zbf); }

/*------------------ Push Message ------------------*/
int8_t _z_push_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_push_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_N_PUSH\n");

    _Z_EC(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_N_PUSH_N), &msg->_key));
    _Z_EC(_z_push_body_encode(wbf, &msg->_body));

    return ret;
}

int8_t _z_push_decode_na(_z_n_msg_push_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_N_PUSH\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_N_PUSH_N));
    ret |= _z_push_body_decode(&msg->_body, zbf);

    return ret;
}

int8_t _z_push_decode(_z_n_msg_push_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_push_decode_na(msg, zbf, header);
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

/*------------------ Response Body Field ------------------*/
int8_t _z_response_body_encode(_z_wbuf_t *wbf, const _z_response_body_t *rspb) {
    (void)(wbf);
    (void)(rspb);
    int8_t ret = _Z_RES_OK;

    return ret;
}

int8_t _z_response_body_decode_na(_z_response_body_t *rspb, _z_zbuf_t *zbf) {
    (void)(zbf);
    (void)(rspb);
    int8_t ret = _Z_RES_OK;

    return ret;
}

int8_t _z_response_body_decode(_z_response_body_t *rspb, _z_zbuf_t *zbf) {
    return _z_response_body_decode_na(rspb, zbf);
}

/*------------------ Response Message ------------------*/
int8_t _z_response_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_response_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_N_RESPONSE\n");

    _Z_EC(_z_zint_encode(wbf, msg->_rid));
    _Z_EC(_z_keyexpr_encode(wbf, _Z_HAS_FLAG(header, _Z_FLAG_N_RESPONSE_N), &msg->_key));
    _Z_EC(_z_response_body_encode(wbf, &msg->_body));

    return ret;
}

int8_t _z_response_decode_na(_z_n_msg_response_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    _Z_DEBUG("Decoding _Z_MID_N_RESPONSE\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&msg->_rid, zbf);
    ret |= _z_keyexpr_decode(&msg->_key, zbf, _Z_HAS_FLAG(header, _Z_FLAG_N_RESPONSE_N));
    ret |= _z_response_body_decode(&msg->_body, zbf);

    return ret;
}

int8_t _z_response_decode(_z_n_msg_response_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    return _z_response_decode_na(msg, zbf, header);
}

/*------------------ Response Final Message ------------------*/
int8_t _z_response_final_encode(_z_wbuf_t *wbf, uint8_t header, const _z_n_msg_response_final_t *msg) {
    (void)(header);
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_N_RESPONSE\n");

    _Z_EC(_z_zint_encode(wbf, msg->_rid));

    return ret;
}

int8_t _z_response_final_decode_na(_z_n_msg_response_final_t *msg, _z_zbuf_t *zbf, uint8_t header) {
    (void)(header);
    _Z_DEBUG("Decoding _Z_MID_N_RESPONSE\n");
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&msg->_rid, zbf);

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