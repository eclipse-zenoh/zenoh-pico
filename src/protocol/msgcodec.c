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

#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/utils/logging.h"

/*=============================*/
/*           Fields            */
/*=============================*/
/*------------------ Payload field ------------------*/
int8_t _z_payload_encode(_z_wbuf_t *wbf, const _z_payload_t *pld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _PAYLOAD\n");
    ret = _z_bytes_encode(wbf, pld);

    return ret;
}

void _z_payload_decode_na(_z_zbuf_t *zbf, _z_payload_result_t *r) {
    _Z_DEBUG("Decoding _PAYLOAD\n");
    r->_tag = _Z_RES_OK;

    _z_bytes_result_t r_arr = _z_bytes_decode(zbf);
    _ASSURE_P_RESULT(r_arr, r, _Z_ERR_PARSE_PAYLOAD);
    r->_value = r_arr._value;
}

_z_payload_result_t _z_payload_decode(_z_zbuf_t *zbf) {
    _z_payload_result_t r;
    _z_payload_decode_na(zbf, &r);
    return r;
}

/*------------------ Timestamp Field ------------------*/
int8_t _z_timestamp_encode(_z_wbuf_t *wbf, const _z_timestamp_t *ts) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _TIMESTAMP\n");

    // Encode the body
    _Z_EC(_z_zint_encode(wbf, ts->_time))
    ret = _z_bytes_encode(wbf, &ts->_id);

    return ret;
}

void _z_timestamp_decode_na(_z_zbuf_t *zbf, _z_timestamp_result_t *r) {
    _Z_DEBUG("Decoding _TIMESTAMP\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._time = r_zint._value;

    _z_bytes_result_t r_arr = _z_bytes_decode(zbf);
    _ASSURE_P_RESULT(r_arr, r, _Z_ERR_PARSE_BYTES);
    r->_value._id = r_arr._value;
}

_z_timestamp_result_t _z_timestamp_decode(_z_zbuf_t *zbf) {
    _z_timestamp_result_t r;
    _z_timestamp_decode_na(zbf, &r);
    return r;
}

/*------------------ SubMode Field ------------------*/
int8_t _z_subinfo_encode(_z_wbuf_t *wbf, const _z_subinfo_t *fld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _SUB_MODE\n");

    // FIXME: subinfo encode can be easily optimized

    // Encode the header
    uint8_t header = fld->mode;
    if ((fld->period.origin != 0) || (fld->period.period != 0) || (fld->period.duration != 0)) {
        _Z_SET_FLAG(header, _Z_FLAG_Z_P);
    }
    _Z_EC(_z_wbuf_write(wbf, header))

    // Encode the body
    if ((fld->period.origin != 0) || (fld->period.period != 0) || (fld->period.duration != 0)) {
        ret = _z_period_encode(wbf, &fld->period);
    }

    return ret;
}

void _z_subinfo_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_subinfo_result_t *r) {
    _Z_DEBUG("Decoding _SUB_MODE\n");
    r->_tag = _Z_RES_OK;

    // Decode the header
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_R) == true) {
        r->_value.reliability = Z_RELIABILITY_RELIABLE;
    } else {
        r->_value.reliability = Z_RELIABILITY_BEST_EFFORT;
    }

    _z_uint8_result_t r_uint8 = _z_uint8_decode(zbf);
    _ASSURE_P_RESULT(r_uint8, r, _Z_ERR_PARSE_PERIOD)

    uint8_t mode = r_uint8._value;
    r->_value.mode = _Z_MID(mode);
    if (_Z_HAS_FLAG(mode, _Z_FLAG_Z_P) == true) {
        _z_period_result_t r_tp = _z_period_decode(zbf);
        _ASSURE_P_RESULT(r_tp, r, _Z_ERR_PARSE_PERIOD)
        r->_value.period = r_tp._value;
    } else {
        r->_value.period = (_z_period_t){.origin = 0, .period = 0, .duration = 0};
    }
}

_z_subinfo_result_t _z_subinfo_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_subinfo_result_t r;
    _z_subinfo_decode_na(zbf, header, &r);
    return r;
}

/*------------------ ResKey Field ------------------*/
int8_t _z_keyexpr_encode(_z_wbuf_t *wbf, uint8_t header, const _z_keyexpr_t *fld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _RESKEY\n");

    // Encode the body
    _Z_EC(_z_zint_encode(wbf, fld->_id))
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_K) == true) {
        _Z_EC(_z_str_encode(wbf, fld->_suffix))
    }

    return ret;
}

void _z_keyexpr_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_keyexpr_result_t *r) {
    _Z_DEBUG("Decoding _RESKEY\n");
    r->_tag = _Z_RES_OK;

    // Decode the header
    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._id = r_zint._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_K) == true) {
        _z_str_result_t r_str = _z_str_decode(zbf);
        _ASSURE_P_RESULT(r_str, r, _Z_ERR_PARSE_STRING)
        r->_value._suffix = r_str._value;
    } else {
        r->_value._suffix = NULL;
    }
}

_z_keyexpr_result_t _z_keyexpr_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_keyexpr_result_t r;
    _z_keyexpr_decode_na(zbf, header, &r);
    return r;
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

void _z_locators_decode_na(_z_zbuf_t *zbf, _z_locator_array_result_t *r) {
    _Z_DEBUG("Decoding _LOCATORS\n");
    r->_tag = _Z_RES_OK;

    // Decode the number of elements
    _z_zint_result_t r_n = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_n, r, _Z_ERR_PARSE_ZINT)
    size_t len = (size_t)r_n._value;

    r->_value = _z_locator_array_make(len);

    // Decode the elements
    for (size_t i = 0; i < len; i++) {
        _z_str_result_t r_s = _z_str_decode(zbf);
        _ASSURE_P_RESULT(r_s, r, _Z_ERR_PARSE_STRING);

        // TODO: this can be optimized by avoid translation from locator to str and vice-versa
        _z_locator_result_t r_l = _z_locator_from_str(r_s._value);
        z_free(r_s._value);
        _ASSURE_P_RESULT(r_l, r, _Z_ERR_INVALID_LOCATOR);

        r->_value._val[i] = r_l._value;
    }
}

_z_locator_array_result_t _z_locators_decode(_z_zbuf_t *zbf) {
    _z_locator_array_result_t r;
    _z_locators_decode_na(zbf, &r);
    return r;
}

/*=============================*/
/*     Message decorators      */
/*=============================*/
/*------------------ Attachment Decorator ------------------*/
int8_t _z_attachment_encode(_z_wbuf_t *wbf, const _z_attachment_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_ATTACHMENT\n");

    // Encode the header
    // WARNING: we do not support sliced content in zenoh-pico.
    //          Disable the SLICED flag to be on the safe side.
    _Z_EC(_z_wbuf_write(wbf, msg->_header & ~_Z_FLAG_T_Z))

    // Encode the body
    ret = _z_payload_encode(wbf, &msg->_payload);

    return ret;
}

void _z_attachment_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_attachment_p_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_ATTACHMENT\n");
    r->_tag = _Z_RES_OK;

    // Store the header
    r->_value->_header = header;

    // WARNING: we do not support sliced content in zenoh-pico.
    //          Return error in case the payload is sliced.
    if (_Z_HAS_FLAG(r->_value->_header, _Z_FLAG_T_Z) == false) {
        _z_payload_result_t r_pld = _z_payload_decode(zbf);
        _ASSURE_FREE_P_RESULT(r_pld, r, _Z_ERR_PARSE_PAYLOAD, attachment)
        r->_value->_payload = r_pld._value;
    } else {
        r->_tag = _Z_ERR_PARSE_PAYLOAD;
    }
}

_z_attachment_p_result_t _z_attachment_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_attachment_p_result_t r;
    _z_attachment_p_result_init(&r);
    _z_attachment_decode_na(zbf, header, &r);
    return r;
}

/*------------------ ReplyContext Decorator ------------------*/
int8_t _z_reply_context_encode(_z_wbuf_t *wbf, const _z_reply_context_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_REPLY_CONTEXT\n");

    // Encode the header
    _Z_EC(_z_wbuf_write(wbf, msg->_header))

    // Encode the body
    _Z_EC(_z_zint_encode(wbf, msg->_qid))
    if (_Z_HAS_FLAG(msg->_header, _Z_FLAG_Z_F) == false) {
        _Z_EC(_z_bytes_encode(wbf, &msg->_replier_id))
    }

    return ret;
}

void _z_reply_context_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_reply_context_p_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_REPLY_CONTEXT\n");
    r->_tag = _Z_RES_OK;

    // Store the header
    r->_value->_header = header;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_FREE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT, reply_context);
    r->_value->_qid = r_zint._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_F) == false) {
        _z_bytes_result_t r_arr = _z_bytes_decode(zbf);
        _ASSURE_FREE_P_RESULT(r_arr, r, _Z_ERR_PARSE_BYTES, reply_context)
        r->_value->_replier_id = _z_bytes_duplicate(&r_arr._value);
    }
}

_z_reply_context_p_result_t _z_reply_context_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_reply_context_p_result_t r;
    _z_reply_context_p_result_init(&r);
    _z_reply_context_decode_na(zbf, header, &r);
    return r;
}

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/
/*------------------ Resource Declaration ------------------*/
int8_t _z_res_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_res_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_RESOURCE\n");

    // Encode the body
    _Z_EC(_z_zint_encode(wbf, dcl->_id))
    ret = _z_keyexpr_encode(wbf, header, &dcl->_key);

    return ret;
}

void _z_res_decl_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_res_decl_result_t *r) {
    _Z_DEBUG("Decoding _Z_DECL_RESOURCE\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._id = r_zint._value;

    _z_keyexpr_result_t r_res = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_res, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_res._value;
}

_z_res_decl_result_t _z_res_decl_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_res_decl_result_t r;
    _z_res_decl_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Publisher Declaration ------------------*/
int8_t _z_pub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_pub_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_PUBLISHER\n");

    // Encode the body
    ret = _z_keyexpr_encode(wbf, header, &dcl->_key);

    return ret;
}

void _z_pub_decl_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_pub_decl_result_t *r) {
    _Z_DEBUG("Decoding _Z_DECL_PUBLISHER\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_res = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_res, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_res._value;
}

_z_pub_decl_result_t _z_pub_decl_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_pub_decl_result_t r;
    _z_pub_decl_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Subscriber Declaration ------------------*/
int8_t _z_sub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_sub_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_SUBSCRIBER\n");

    // Encode the body
    _Z_EC(_z_keyexpr_encode(wbf, header, &dcl->_key))
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_S) == true) {
        ret = _z_subinfo_encode(wbf, &dcl->_subinfo);
    }

    return ret;
}

void _z_sub_decl_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_sub_decl_result_t *r) {
    _Z_DEBUG("Decoding _Z_DECL_SUBSCRIBER\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_res = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_res, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_res._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_S) == true) {
        _z_subinfo_result_t r_smd = _z_subinfo_decode(zbf, header);
        _ASSURE_P_RESULT(r_smd, r, _Z_ERR_PARSE_SUBMODE)
        r->_value._subinfo = r_smd._value;
    } else {
        // Default subscription mode is non-periodic PUSH
        r->_value._subinfo.mode = Z_SUBMODE_PUSH;
        r->_value._subinfo.period = (_z_period_t){.origin = 0, .period = 0, .duration = 0};
        if (_Z_HAS_FLAG(header, _Z_FLAG_Z_R) == true) {
            r->_value._subinfo.reliability = Z_RELIABILITY_RELIABLE;
        } else {
            r->_value._subinfo.reliability = Z_RELIABILITY_BEST_EFFORT;
        }
    }
}

_z_sub_decl_result_t _z_sub_decl_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_sub_decl_result_t r;
    _z_sub_decl_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Queryable Declaration ------------------*/
int8_t _z_qle_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_qle_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_QUERYABLE\n");

    // Encode the body
    _Z_EC(_z_keyexpr_encode(wbf, header, &dcl->_key));

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Q) == true) {
        _Z_EC(_z_zint_encode(wbf, dcl->_complete));
        _Z_EC(_z_zint_encode(wbf, dcl->_distance));
    }

    return ret;
}

void _z_qle_decl_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_qle_decl_result_t *r) {
    _Z_DEBUG("Decoding _Z_DECL_QUERYABLE\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_res = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_res, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_res._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Q) == true) {
        _z_zint_result_t r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._complete = r_zint._value;

        r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._distance = r_zint._value;
    }
}

_z_qle_decl_result_t _z_qle_decl_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_qle_decl_result_t r;
    _z_qle_decl_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Forget Resource Declaration ------------------*/
int8_t _z_forget_res_decl_encode(_z_wbuf_t *wbf, const _z_forget_res_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_FORGET_RESOURCE\n");

    // Encode the body
    ret = _z_zint_encode(wbf, dcl->_rid);

    return ret;
}

void _z_forget_res_decl_decode_na(_z_zbuf_t *zbf, _z_forget_res_decl_result_t *r) {
    _Z_DEBUG("Decoding _Z_DECL_FORGET_RESOURCE\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._rid = r_zint._value;
}

_z_forget_res_decl_result_t _z_forget_res_decl_decode(_z_zbuf_t *zbf) {
    _z_forget_res_decl_result_t r;
    _z_forget_res_decl_decode_na(zbf, &r);
    return r;
}

/*------------------ Forget Publisher Declaration ------------------*/
int8_t _z_forget_pub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_forget_pub_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_FORGET_PUBLISHER\n");

    // Encode the body
    ret = _z_keyexpr_encode(wbf, header, &dcl->_key);

    return ret;
}

void _z_forget_pub_decl_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_forget_pub_decl_result_t *r) {
    _Z_DEBUG("Decoding _Z_DECL_FORGET_PUBLISHER\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_res = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_res, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_res._value;
}

_z_forget_pub_decl_result_t _z_forget_pub_decl_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_forget_pub_decl_result_t r;
    _z_forget_pub_decl_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Forget Subscriber Declaration ------------------*/
int8_t _z_forget_sub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_forget_sub_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_FORGET_PUBLISHER\n");

    // Encode the body
    ret = _z_keyexpr_encode(wbf, header, &dcl->_key);

    return ret;
}

void _z_forget_sub_decl_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_forget_sub_decl_result_t *r) {
    _Z_DEBUG("Decoding _Z_DECL_FORGET_PUBLISHER\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_res = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_res, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_res._value;
}

_z_forget_sub_decl_result_t _z_forget_sub_decl_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_forget_sub_decl_result_t r;
    _z_forget_sub_decl_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Forget Queryable Declaration ------------------*/
int8_t _z_forget_qle_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _z_forget_qle_decl_t *dcl) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DECL_FORGET_QUERYABLE\n");

    ret = _z_keyexpr_encode(wbf, header, &dcl->_key);

    return ret;
}

void _z_forget_qle_decl_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_forget_qle_decl_result_t *r) {
    _Z_DEBUG("Decoding _Z_DECL_FORGET_QUERYABLE\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_res = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_res, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_res._value;
}

_z_forget_qle_decl_result_t _z_forget_qle_decl_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_forget_qle_decl_result_t r;
    _z_forget_qle_decl_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Declaration Field ------------------*/
int8_t _z_declaration_encode(_z_wbuf_t *wbf, _z_declaration_t *dcl) {
    int8_t ret = _Z_RES_OK;
    // Encode the header
    _Z_EC(_z_wbuf_write(wbf, dcl->_header))
    // Encode the body
    uint8_t did = _Z_MID(dcl->_header);
    switch (did) {
        case _Z_DECL_RESOURCE: {
            ret = _z_res_decl_encode(wbf, dcl->_header, &dcl->_body._res);
        } break;

        case _Z_DECL_PUBLISHER: {
            ret = _z_pub_decl_encode(wbf, dcl->_header, &dcl->_body._pub);
        } break;

        case _Z_DECL_SUBSCRIBER: {
            ret = _z_sub_decl_encode(wbf, dcl->_header, &dcl->_body._sub);
        } break;

        case _Z_DECL_QUERYABLE: {
            ret = _z_qle_decl_encode(wbf, dcl->_header, &dcl->_body._qle);
        } break;

        case _Z_DECL_FORGET_RESOURCE: {
            ret = _z_forget_res_decl_encode(wbf, &dcl->_body._forget_res);
        } break;

        case _Z_DECL_FORGET_PUBLISHER: {
            ret = _z_forget_pub_decl_encode(wbf, dcl->_header, &dcl->_body._forget_pub);
        } break;

        case _Z_DECL_FORGET_SUBSCRIBER: {
            ret = _z_forget_sub_decl_encode(wbf, dcl->_header, &dcl->_body._forget_sub);
        } break;

        case _Z_DECL_FORGET_QUERYABLE: {
            ret = _z_forget_qle_decl_encode(wbf, dcl->_header, &dcl->_body._forget_qle);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to encode declaration with unknown ID(%d)\n", did);
            ret = _Z_ERR_UNKNOWN_MESSAGE;
        } break;
    }

    return ret;
}

void _z_declaration_decode_na(_z_zbuf_t *zbf, _z_declaration_result_t *r) {
    r->_tag = _Z_RES_OK;

    // Decode the header
    _z_uint8_result_t r_uint8 = _z_uint8_decode(zbf);
    _ASSURE_P_RESULT(r_uint8, r, _Z_ERR_PARSE_DECLARATION)

    r->_value._header = r_uint8._value;
    uint8_t mid = _Z_MID(r->_value._header);

    _z_res_decl_result_t r_rdcl;
    _z_pub_decl_result_t r_pdcl;
    _z_sub_decl_result_t r_sdcl;
    _z_qle_decl_result_t r_qdcl;
    _z_forget_res_decl_result_t r_frdcl;
    _z_forget_pub_decl_result_t r_fpdcl;
    _z_forget_sub_decl_result_t r_fsdcl;
    _z_forget_qle_decl_result_t r_fqdcl;

    switch (mid) {
        case _Z_DECL_RESOURCE: {
            r_rdcl = _z_res_decl_decode(zbf, r->_value._header);
            _ASSURE_P_RESULT(r_rdcl, r, _Z_ERR_PARSE_DECLARATION)
            r->_value._body._res = r_rdcl._value;
        } break;

        case _Z_DECL_PUBLISHER: {
            r_pdcl = _z_pub_decl_decode(zbf, r->_value._header);
            _ASSURE_P_RESULT(r_pdcl, r, _Z_ERR_PARSE_DECLARATION)
            r->_value._body._pub = r_pdcl._value;
        } break;

        case _Z_DECL_SUBSCRIBER: {
            r_sdcl = _z_sub_decl_decode(zbf, r->_value._header);
            _ASSURE_P_RESULT(r_sdcl, r, _Z_ERR_PARSE_DECLARATION)
            r->_value._body._sub = r_sdcl._value;
        } break;

        case _Z_DECL_QUERYABLE: {
            r_qdcl = _z_qle_decl_decode(zbf, r->_value._header);
            _ASSURE_P_RESULT(r_qdcl, r, _Z_ERR_PARSE_DECLARATION)
            r->_value._body._qle = r_qdcl._value;
        } break;

        case _Z_DECL_FORGET_RESOURCE: {
            r_frdcl = _z_forget_res_decl_decode(zbf);
            _ASSURE_P_RESULT(r_frdcl, r, _Z_ERR_PARSE_DECLARATION)
            r->_value._body._forget_res = r_frdcl._value;
        } break;

        case _Z_DECL_FORGET_PUBLISHER: {
            r_fpdcl = _z_forget_pub_decl_decode(zbf, r->_value._header);
            _ASSURE_P_RESULT(r_fpdcl, r, _Z_ERR_PARSE_DECLARATION)
            r->_value._body._forget_pub = r_fpdcl._value;
        } break;

        case _Z_DECL_FORGET_SUBSCRIBER: {
            r_fsdcl = _z_forget_sub_decl_decode(zbf, r->_value._header);
            _ASSURE_P_RESULT(r_fsdcl, r, _Z_ERR_PARSE_DECLARATION)
            r->_value._body._forget_sub = r_fsdcl._value;
        } break;

        case _Z_DECL_FORGET_QUERYABLE: {
            r_fqdcl = _z_forget_qle_decl_decode(zbf, r->_value._header);
            _ASSURE_P_RESULT(r_fqdcl, r, _Z_ERR_PARSE_DECLARATION)
            r->_value._body._forget_qle = r_fqdcl._value;
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to decode declaration with unknown ID(%d)\n", mid);
            r->_tag = _Z_ERR_PARSE_DECLARATION;
        } break;
    }
}

_z_declaration_result_t _z_declaration_decode(_z_zbuf_t *zbf) {
    _z_declaration_result_t r;
    _z_declaration_decode_na(zbf, &r);
    return r;
}

/*------------------ Declaration Message ------------------*/
int8_t _z_declare_encode(_z_wbuf_t *wbf, const _z_msg_declare_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_DECLARE\n");

    // Encode the body
    _Z_EC(_z_zint_encode(wbf, msg->_declarations._len))
    for (_z_zint_t i = 0; i < msg->_declarations._len; i++) {
        _Z_EC(_z_declaration_encode(wbf, &msg->_declarations._val[i]));
    }

    return ret;
}

void _z_declare_decode_na(_z_zbuf_t *zbf, _z_declare_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_DECLARE\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_dlen = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_dlen, r, _Z_ERR_PARSE_ZINT)
    size_t len = (size_t)r_dlen._value;

    r->_value._declarations = _z_declaration_array_make(len);

    _z_declaration_result_t r_decl;
    for (size_t i = 0; i < len; i++) {
        _z_declaration_decode_na(zbf, &r_decl);
        if (r_decl._tag == _Z_RES_OK) {
            r->_value._declarations._val[i] = r_decl._value;
        } else {
            r->_tag = _Z_ERR_PARSE_ZENOH_MESSAGE;

            _z_msg_clear_declare(&r->_value);

            break;
        }
    }
}

_z_declare_result_t _z_declare_decode(_z_zbuf_t *zbf) {
    _z_declare_result_t r;
    _z_declare_decode_na(zbf, &r);
    return r;
}

/*------------------ Data Info Field ------------------*/
int8_t _z_data_info_encode(_z_wbuf_t *wbf, const _z_data_info_t *fld) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_DATA_INFO\n");

    // Encode the flags
    // WARNING: we do not support sliced content in zenoh-pico.
    //          Disable the SLICED flag to be on the safe side.
    _Z_EC(_z_zint_encode(wbf, fld->_flags & ~_Z_DATA_INFO_SLICED))

    // Encode the body
    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_KIND) == true) {
        _Z_EC(_z_zint_encode(wbf, fld->_kind))
    }
    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_ENC) == true) {
        _Z_EC(_z_zint_encode(wbf, fld->_encoding.prefix))
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
    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_RTR_ID) == true) {
        _Z_EC(_z_bytes_encode(wbf, &fld->_first_router_id))
    }
    if (_Z_HAS_FLAG(fld->_flags, _Z_DATA_INFO_RTR_SN) == true) {
        _Z_EC(_z_zint_encode(wbf, fld->_first_router_sn))
    }

    return ret;
}

void _z_data_info_decode_na(_z_zbuf_t *zbf, _z_data_info_result_t *r) {
    _Z_DEBUG("Decoding _Z_DATA_INFO\n");
    r->_tag = _Z_RES_OK;

    // Decode the flags
    _z_zint_result_t r_flags = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_flags, r, _Z_ERR_PARSE_ZINT)
    r->_value._flags = r_flags._value;

    // WARNING: we do not support sliced content in zenoh-pico.
    //          Return error in case the payload is sliced.
    if (_Z_HAS_FLAG(r->_value._flags, _Z_DATA_INFO_SLICED) == false) {
        if (_Z_HAS_FLAG(r->_value._flags, _Z_DATA_INFO_KIND) == true) {
            _z_zint_result_t r_knd = _z_zint_decode(zbf);
            _ASSURE_P_RESULT(r_knd, r, _Z_ERR_PARSE_ZINT)
            r->_value._kind = r_knd._value;
        } else {
            r->_value._kind = Z_SAMPLE_KIND_PUT;
        }

        if (_Z_HAS_FLAG(r->_value._flags, _Z_DATA_INFO_ENC) == true) {
            _z_zint_result_t r_enc = _z_zint_decode(zbf);
            _ASSURE_P_RESULT(r_enc, r, _Z_ERR_PARSE_ZINT)
            r->_value._encoding.prefix = r_enc._value;

            _z_bytes_result_t r_bytes = _z_bytes_decode(zbf);
            _ASSURE_P_RESULT(r_bytes, r, _Z_ERR_PARSE_STRING)
            r->_value._encoding.suffix = r_bytes._value;
        }

        if (_Z_HAS_FLAG(r->_value._flags, _Z_DATA_INFO_TSTAMP) == true) {
            _z_timestamp_result_t r_tsp = _z_timestamp_decode(zbf);
            _ASSURE_P_RESULT(r_tsp, r, _Z_ERR_PARSE_TIMESTAMP)
            r->_value._tstamp = r_tsp._value;
        } else {
            _z_timestamp_reset(&r->_value._tstamp);
        }

        if (_Z_HAS_FLAG(r->_value._flags, _Z_DATA_INFO_SRC_ID) == true) {
            _z_bytes_result_t r_sid = _z_bytes_decode(zbf);
            _ASSURE_P_RESULT(r_sid, r, _Z_ERR_PARSE_BYTES)
            r->_value._source_id = r_sid._value;
        }

        if (_Z_HAS_FLAG(r->_value._flags, _Z_DATA_INFO_SRC_SN) == true) {
            _z_zint_result_t r_ssn = _z_zint_decode(zbf);
            _ASSURE_P_RESULT(r_ssn, r, _Z_ERR_PARSE_ZINT)
            r->_value._source_sn = r_ssn._value;
        }

        if (_Z_HAS_FLAG(r->_value._flags, _Z_DATA_INFO_RTR_ID) == true) {
            _z_bytes_result_t r_rid = _z_bytes_decode(zbf);
            _ASSURE_P_RESULT(r_rid, r, _Z_ERR_PARSE_BYTES)
            r->_value._first_router_id = r_rid._value;
        }

        if (_Z_HAS_FLAG(r->_value._flags, _Z_DATA_INFO_RTR_SN) == true) {
            _z_zint_result_t r_rsn = _z_zint_decode(zbf);
            _ASSURE_P_RESULT(r_rsn, r, _Z_ERR_PARSE_ZINT)
            r->_value._first_router_sn = r_rsn._value;
        }
    } else {
        r->_tag = _Z_ERR_PARSE_PAYLOAD;
    }
}

_z_data_info_result_t _z_data_info_decode(_z_zbuf_t *zbf) {
    _z_data_info_result_t r;
    _z_data_info_decode_na(zbf, &r);
    return r;
}

/*------------------ Data Message ------------------*/
int8_t _z_data_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_data_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_DATA\n");

    // Encode the body
    _Z_EC(_z_keyexpr_encode(wbf, header, &msg->_key))

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_I) == true) {
        _Z_EC(_z_data_info_encode(wbf, &msg->_info))
    }
    _Z_EC(_z_payload_encode(wbf, &msg->_payload))

    return ret;
}

void _z_data_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_data_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_DATA\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_key = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_key, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_key._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_I) == true) {
        _z_data_info_result_t r_dti = _z_data_info_decode(zbf);
        _ASSURE_P_RESULT(r_dti, r, _Z_ERR_PARSE_ZENOH_MESSAGE)
        r->_value._info = r_dti._value;
    } else {
        (void)memset(&r->_value._info, 0, sizeof(_z_data_info_t));
    }

    _z_payload_result_t r_pld = _z_payload_decode(zbf);
    _ASSURE_P_RESULT(r_pld, r, _Z_ERR_PARSE_ZINT)
    r->_value._payload = r_pld._value;
}

_z_data_result_t _z_data_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_data_result_t r;
    _z_data_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Pull Message ------------------*/
int8_t _z_pull_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_pull_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_PULL\n");

    // Encode the body
    _Z_EC(_z_keyexpr_encode(wbf, header, &msg->_key))

    _Z_EC(_z_zint_encode(wbf, msg->_pull_id))

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_N) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_max_samples))
    }

    return ret;
}

void _z_pull_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_pull_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_PULL\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_key = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_key, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_key._value;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._pull_id = r_zint._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_N) == true) {
        r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._max_samples = r_zint._value;
    }
}

_z_pull_result_t _z_pull_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_pull_result_t r;
    _z_pull_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Query Message ------------------*/
int8_t _z_msg_query_target_encode(_z_wbuf_t *wbf, const z_query_target_t *qt) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _QUERY_TARGET\n");
    _z_zint_t target = *qt;

    ret = _z_zint_encode(wbf, target);

    return ret;
}

int8_t _z_query_consolidation_encode(_z_wbuf_t *wbf, const z_consolidation_mode_t *qc) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _QUERY_CONSOLIDATION\n");
    _z_zint_t mode = *qc;

    ret = _z_zint_encode(wbf, mode);

    return ret;
}

int8_t _z_query_encode(_z_wbuf_t *wbf, uint8_t header, const _z_msg_query_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_QUERY\n");

    // Encode the body
    _Z_EC(_z_keyexpr_encode(wbf, header, &msg->_key))

    _Z_EC(_z_str_encode(wbf, msg->_parameters))

    _Z_EC(_z_zint_encode(wbf, msg->_qid))

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_T) == true) {
        _Z_EC(_z_msg_query_target_encode(wbf, &msg->_target))
    }
    ret = _z_query_consolidation_encode(wbf, &msg->_consolidation);

    return ret;
}

_z_query_target_result_t _z_msg_query_target_decode(_z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _QUERY_TARGET\n");
    _z_query_target_result_t r;
    r._tag = _Z_RES_OK;

    _z_zint_result_t r_tag = _z_zint_decode(zbf);
    _ASSURE_RESULT(r_tag, r, _Z_ERR_PARSE_ZINT)
    r._value = r_tag._value;

    return r;
}

_z_query_consolidation_result_t _z_query_consolidation_decode(_z_zbuf_t *zbf) {
    _Z_DEBUG("Decoding _QUERY_CONSOLIDATION\n");
    _z_query_consolidation_result_t r;
    r._tag = _Z_RES_OK;

    _z_zint_result_t r_con = _z_zint_decode(zbf);
    _ASSURE_RESULT(r_con, r, _Z_ERR_PARSE_ZINT)

    unsigned int mode = r_con._value & 0x03;
    switch (mode) {
        case Z_CONSOLIDATION_MODE_NONE:
        case Z_CONSOLIDATION_MODE_MONOTONIC:
        case Z_CONSOLIDATION_MODE_LATEST: {
            r._value = mode;
        } break;

        default: {
            r._tag = _Z_ERR_PARSE_CONSOLIDATION;
        } break;
    }

    return r;
}

void _z_query_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_query_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_QUERY\n");
    r->_tag = _Z_RES_OK;

    _z_keyexpr_result_t r_key = _z_keyexpr_decode(zbf, header);
    _ASSURE_P_RESULT(r_key, r, _Z_ERR_PARSE_RESKEY)
    r->_value._key = r_key._value;

    _z_str_result_t r_str = _z_str_decode(zbf);
    _ASSURE_P_RESULT(r_str, r, _Z_ERR_PARSE_STRING)
    r->_value._parameters = r_str._value;

    _z_zint_result_t r_qid = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_qid, r, _Z_ERR_PARSE_ZINT)
    r->_value._qid = r_qid._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_T) == true) {
        _z_query_target_result_t r_qt = _z_msg_query_target_decode(zbf);
        _ASSURE_P_RESULT(r_qt, r, _Z_ERR_PARSE_ZINT)
        r->_value._target = r_qt._value;
    } else {
        r->_value._target = Z_QUERY_TARGET_BEST_MATCHING;
    }

    _z_query_consolidation_result_t r_con = _z_query_consolidation_decode(zbf);
    _ASSURE_P_RESULT(r_con, r, _Z_ERR_PARSE_CONSOLIDATION)
    r->_value._consolidation = r_con._value;
}

_z_query_result_t _z_query_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_query_result_t r;
    _z_query_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Zenoh Message ------------------*/
int8_t _z_zenoh_message_encode(_z_wbuf_t *wbf, const _z_zenoh_message_t *msg) {
    int8_t ret = _Z_RES_OK;

    // Encode the decorators if present
    if (msg->_attachment != NULL) {
        _Z_EC(_z_attachment_encode(wbf, msg->_attachment))
    }
    if (msg->_reply_context != NULL) {
        _Z_EC(_z_reply_context_encode(wbf, msg->_reply_context))
    }

    // Encode the header
    _Z_EC(_z_wbuf_write(wbf, msg->_header))

    // Encode the body
    uint8_t mid = _Z_MID(msg->_header);
    switch (mid) {
        case _Z_MID_DECLARE: {
            ret = _z_declare_encode(wbf, &msg->_body._declare);
        } break;

        case _Z_MID_DATA: {
            ret = _z_data_encode(wbf, msg->_header, &msg->_body._data);
        } break;

        case _Z_MID_PULL: {
            ret = _z_pull_encode(wbf, msg->_header, &msg->_body._pull);
        } break;

        case _Z_MID_QUERY: {
            ret = _z_query_encode(wbf, msg->_header, &msg->_body._query);
        } break;

        case _Z_MID_UNIT: {
            // Do nothing. Unit messages have no body
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
            ret = _Z_ERR_UNKNOWN_MESSAGE;
        } break;
    }

    return ret;
}

void _z_zenoh_message_decode_na(_z_zbuf_t *zbf, _z_zenoh_message_result_t *r) {
    _Bool is_last = false;
    r->_tag = _Z_RES_OK;

    r->_value._attachment = NULL;
    r->_value._reply_context = NULL;
    do {
        _z_uint8_result_t r_uint8 = _z_uint8_decode(zbf);
        _ASSURE_P_RESULT(r_uint8, r, _Z_ERR_PARSE_ZENOH_MESSAGE)
        r->_value._header = r_uint8._value;

        uint8_t mid = _Z_MID(r->_value._header);
        switch (mid) {
            case _Z_MID_DATA: {
                _z_data_result_t r_da = _z_data_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_da, r, _Z_ERR_PARSE_ZENOH_MESSAGE)
                r->_value._body._data = r_da._value;
                is_last = true;
            } break;

            case _Z_MID_ATTACHMENT: {
                _z_attachment_p_result_t r_at = _z_attachment_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_at, r, _Z_ERR_PARSE_ZENOH_MESSAGE)
                r->_value._attachment = r_at._value;
            } break;

            case _Z_MID_REPLY_CONTEXT: {
                _z_reply_context_p_result_t r_rc = _z_reply_context_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_rc, r, _Z_ERR_PARSE_ZENOH_MESSAGE)
                r->_value._reply_context = r_rc._value;
            } break;

            case _Z_MID_DECLARE: {
                _z_declare_result_t r_de = _z_declare_decode(zbf);
                _ASSURE_P_RESULT(r_de, r, _Z_ERR_PARSE_ZENOH_MESSAGE)
                r->_value._body._declare = r_de._value;
                is_last = true;
            } break;

            case _Z_MID_QUERY: {
                _z_query_result_t r_qu = _z_query_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_qu, r, _Z_ERR_PARSE_ZENOH_MESSAGE)
                r->_value._body._query = r_qu._value;
                is_last = true;
            } break;

            case _Z_MID_PULL: {
                _z_pull_result_t r_pu = _z_pull_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_pu, r, _Z_ERR_PARSE_ZENOH_MESSAGE)
                r->_value._body._pull = r_pu._value;
                is_last = true;
            } break;

            case _Z_MID_UNIT: {
                // Do nothing. Unit messages have no body.
                is_last = true;
            } break;

            case _Z_MID_PRIORITY: {
                // Ignore the priority decorator for the time being since zenoh-pico does not
                // perform any routing. Hence, priority information does not need to be propagated.
            } break;

            case _Z_MID_LINK_STATE_LIST: {
                _Z_DEBUG("WARNING: Link state not supported in zenoh-pico\n");
                r->_tag = _Z_ERR_PARSE_ZENOH_MESSAGE;
                is_last = true;
            } break;

            default: {
                _Z_DEBUG("WARNING: Trying to decode zenoh message with unknown ID(%d)\n", mid);
                _z_msg_clear(&r->_value);

                r->_tag = _Z_ERR_PARSE_ZENOH_MESSAGE;
                is_last = true;
            } break;
        }

        if (is_last == true) {  // Do not break in case of decorators
            break;
        }
    } while (1);
}

_z_zenoh_message_result_t _z_zenoh_message_decode(_z_zbuf_t *zbf) {
    _z_zenoh_message_result_t r;
    _z_zenoh_message_decode_na(zbf, &r);
    return r;
}

/*=============================*/
/*       Transport Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
int8_t _z_scout_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_scout_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_SCOUT\n");

    // Encode the body
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_W) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_what))
    }

    return ret;
}

void _z_scout_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_scout_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_SCOUT\n");
    r->_tag = _Z_RES_OK;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_W) == true) {
        _z_zint_result_t r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._what = r_zint._value;
    }
}

_z_scout_result_t _z_scout_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_scout_result_t r;
    _z_scout_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Hello Message ------------------*/
int8_t _z_hello_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_hello_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_HELLO\n");

    // Encode the body
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I) == true) {
        _Z_EC(_z_bytes_encode(wbf, &msg->_pid))
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_W) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_whatami))
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_L) == true) {
        _Z_EC(_z_locators_encode(wbf, &msg->_locators))
    }

    return ret;
}

void _z_hello_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_hello_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_HELLO\n");
    r->_tag = _Z_RES_OK;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I) == true) {
        _z_bytes_result_t r_arr = _z_bytes_decode(zbf);
        _ASSURE_P_RESULT(r_arr, r, _Z_ERR_PARSE_BYTES)
        r->_value._pid = r_arr._value;
    }

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_W) == true) {
        _z_zint_result_t r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._whatami = r_zint._value;
    }

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_L) == true) {
        _z_locator_array_result_t r_locs = _z_locators_decode(zbf);
        _ASSURE_P_RESULT(r_locs, r, _Z_ERR_PARSE_BYTES)
        _z_locator_array_move(&r->_value._locators, &r_locs._value);
    }
}

_z_hello_result_t _z_hello_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_hello_result_t r;
    _z_hello_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Join Message ------------------*/
int8_t _z_join_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_join_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_JOIN\n");

    // Encode the body
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_O) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_options))
    }
    _Z_EC(_z_uint8_encode(wbf, msg->_version))
    _Z_EC(_z_zint_encode(wbf, msg->_whatami))
    _Z_EC(_z_bytes_encode(wbf, &msg->_pid))

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_T1) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_lease / 1000))
    } else {
        _Z_EC(_z_zint_encode(wbf, msg->_lease))
    }

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_S) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_sn_resolution))
    }
    if (_Z_HAS_FLAG(msg->_options, _Z_OPT_JOIN_QOS) == true) {
        for (uint8_t i = 0; i < Z_PRIORITIES_NUM; i++) {
            _Z_EC(_z_zint_encode(wbf, msg->_next_sns._val._qos[i]._reliable))
            _Z_EC(_z_zint_encode(wbf, msg->_next_sns._val._qos[i]._best_effort))
        }
    } else {
        _Z_EC(_z_zint_encode(wbf, msg->_next_sns._val._plain._reliable))
        _Z_EC(_z_zint_encode(wbf, msg->_next_sns._val._plain._best_effort))
    }

    return ret;
}

void _z_join_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_join_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_JOIN\n");
    r->_tag = _Z_RES_OK;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_O) == true) {
        _z_zint_result_t r_opts = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_opts, r, _Z_ERR_PARSE_ZINT)
        r->_value._options = r_opts._value;
    } else {
        r->_value._options = 0;
    }

    _z_uint8_result_t r_ver = _z_uint8_decode(zbf);
    _ASSURE_P_RESULT(r_ver, r, _Z_ERR_PARSE_UINT8)
    r->_value._version = r_ver._value;

    _z_zint_result_t r_wami = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_wami, r, _Z_ERR_PARSE_ZINT)
    r->_value._whatami = r_wami._value;

    _z_bytes_result_t r_pid = _z_bytes_decode(zbf);
    _ASSURE_P_RESULT(r_pid, r, _Z_ERR_PARSE_BYTES)
    r->_value._pid = r_pid._value;

    _z_zint_result_t r_lease = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_lease, r, _Z_ERR_PARSE_ZINT)
    r->_value._lease = r_lease._value;
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_T1) == true) {
        r->_value._lease *= 1000;
    }

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_S) == true) {
        _z_zint_result_t r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._sn_resolution = r_zint._value;
    }

    if (_Z_HAS_FLAG(r->_value._options, _Z_OPT_JOIN_QOS) == true) {
        r->_value._next_sns._is_qos = 1;

        for (uint8_t i = 0; i < Z_PRIORITIES_NUM; i++) {
            _z_zint_result_t r_zint = _z_zint_decode(zbf);
            _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
            r->_value._next_sns._val._qos[i]._reliable = r_zint._value;

            r_zint = _z_zint_decode(zbf);
            _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
            r->_value._next_sns._val._qos[i]._best_effort = r_zint._value;
        }
    } else {
        r->_value._next_sns._is_qos = 0;

        _z_zint_result_t r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._next_sns._val._plain._reliable = r_zint._value;

        r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._next_sns._val._plain._best_effort = r_zint._value;
    }
}

_z_join_result_t _z_join_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_join_result_t r;
    _z_join_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Init Message ------------------*/
int8_t _z_init_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_init_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_INIT\n");

    // Encode the body
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_O) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_options))
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_A) == false) {
        _Z_EC(_z_wbuf_write(wbf, msg->_version))
    }
    _Z_EC(_z_zint_encode(wbf, msg->_whatami))
    _Z_EC(_z_bytes_encode(wbf, &msg->_pid))
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_S) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_sn_resolution))
    }
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_A) == true) {
        _Z_EC(_z_bytes_encode(wbf, &msg->_cookie))
    }

    return ret;
}

void _z_init_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_init_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_INIT\n");
    r->_tag = _Z_RES_OK;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_O) == true) {
        _z_zint_result_t r_opts = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_opts, r, _Z_ERR_PARSE_ZINT)
        r->_value._options = r_opts._value;
    } else {
        r->_value._options = 0;
    }

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_A) == false) {
        _z_uint8_result_t r_uint8 = _z_uint8_decode(zbf);
        _ASSURE_P_RESULT(r_uint8, r, _Z_ERR_PARSE_UINT8)
        r->_value._version = r_uint8._value;
    }

    _z_zint_result_t r_wami = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_wami, r, _Z_ERR_PARSE_ZINT)
    r->_value._whatami = r_wami._value;

    _z_bytes_result_t r_pid = _z_bytes_decode(zbf);
    _ASSURE_P_RESULT(r_pid, r, _Z_ERR_PARSE_BYTES)
    r->_value._pid = r_pid._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_S) == true) {
        _z_zint_result_t r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._sn_resolution = r_zint._value;
    }

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_A) == true) {
        _z_bytes_result_t r_cke = _z_bytes_decode(zbf);
        _ASSURE_P_RESULT(r_cke, r, _Z_ERR_PARSE_BYTES)
        r->_value._cookie = r_cke._value;
    }
}

_z_init_result_t _z_init_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_init_result_t r;
    _z_init_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Open Message ------------------*/
int8_t _z_open_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_open_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_OPEN\n");

    // Encode the body
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_T2) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_lease / 1000))
    } else {
        _Z_EC(_z_zint_encode(wbf, msg->_lease))
    }

    _Z_EC(_z_zint_encode(wbf, msg->_initial_sn))
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_A) == false) {
        _Z_EC(_z_bytes_encode(wbf, &msg->_cookie))
    }

    return ret;
}

void _z_open_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_open_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_OPEN\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_lease = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_lease, r, _Z_ERR_PARSE_ZINT)
    r->_value._lease = r_lease._value;
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_T2) == true) {
        r->_value._lease *= 1000;
    }

    _z_zint_result_t r_isn = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_isn, r, _Z_ERR_PARSE_ZINT)
    r->_value._initial_sn = r_isn._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_A) == false) {
        _z_bytes_result_t r_cke = _z_bytes_decode(zbf);
        _ASSURE_P_RESULT(r_cke, r, _Z_ERR_PARSE_BYTES)
        r->_value._cookie = r_cke._value;
    }
}

_z_open_result_t _z_open_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_open_result_t r;
    _z_open_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Close Message ------------------*/
int8_t _z_close_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_close_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_CLOSE\n");

    // Encode the body
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I) == true) {
        _Z_EC(_z_bytes_encode(wbf, &msg->_pid))
    }
    ret = _z_wbuf_write(wbf, msg->_reason);

    return ret;
}

void _z_close_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_close_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_CLOSE\n");
    r->_tag = _Z_RES_OK;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I) == true) {
        _z_bytes_result_t r_arr = _z_bytes_decode(zbf);
        _ASSURE_P_RESULT(r_arr, r, _Z_ERR_PARSE_BYTES)
        r->_value._pid = r_arr._value;
    }

    _z_uint8_result_t r_uint8 = _z_uint8_decode(zbf);
    _ASSURE_P_RESULT(r_uint8, r, _Z_ERR_PARSE_UINT8)
    r->_value._reason = r_uint8._value;
}

_z_close_result_t _z_close_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_close_result_t r;
    _z_close_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Sync Message ------------------*/
int8_t _z_sync_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_sync_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_SYNC\n");

    // Encode the body
    _Z_EC(_z_zint_encode(wbf, msg->_sn))

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_C) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_count))
    }

    return ret;
}

void _z_sync_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_sync_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_SYNC\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._sn = r_zint._value;

    if ((_Z_HAS_FLAG(header, _Z_FLAG_T_R) != 0) && (_Z_HAS_FLAG(header, _Z_FLAG_T_C) != 0)) {
        r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._count = r_zint._value;
    }
}

_z_sync_result_t _z_sync_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_sync_result_t r;
    _z_sync_decode_na(zbf, header, &r);
    return r;
}

/*------------------ AckNack Message ------------------*/
int8_t _z_ack_nack_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_ack_nack_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_ACK_NACK\n");

    // Encode the body
    _Z_EC(_z_zint_encode(wbf, msg->_sn))

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_M) == true) {
        _Z_EC(_z_zint_encode(wbf, msg->_mask))
    }

    return ret;
}

void _z_ack_nack_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_ack_nack_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_ACK_NACK\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._sn = r_zint._value;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_M) == true) {
        r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
        r->_value._mask = r_zint._value;
    }
}

_z_ack_nack_result_t _z_ack_nack_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_ack_nack_result_t r;
    _z_ack_nack_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Keep Alive Message ------------------*/
int8_t _z_keep_alive_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_keep_alive_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_KEEP_ALIVE\n");

    // Encode the body
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I) == true) {
        _Z_EC(_z_bytes_encode(wbf, &msg->_pid))
    }

    return ret;
}

void _z_keep_alive_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_keep_alive_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_KEEP_ALIVE\n");
    r->_tag = _Z_RES_OK;

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_I) == true) {
        _z_bytes_result_t r_arr = _z_bytes_decode(zbf);
        _ASSURE_P_RESULT(r_arr, r, _Z_ERR_PARSE_BYTES)
        r->_value._pid = r_arr._value;
    }
}

_z_keep_alive_result_t _z_keep_alive_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_keep_alive_result_t r;
    _z_keep_alive_decode_na(zbf, header, &r);
    return r;
}

/*------------------ PingPong Messages ------------------*/
int8_t _z_ping_pong_encode(_z_wbuf_t *wbf, const _z_t_msg_ping_pong_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_PING_PONG\n");

    // Encode the body
    ret = _z_zint_encode(wbf, msg->_hash);

    return ret;
}

void _z_ping_pong_decode_na(_z_zbuf_t *zbf, _z_ping_pong_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_PING_PONG\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._hash = r_zint._value;
}

_z_ping_pong_result_t _z_ping_pong_decode(_z_zbuf_t *zbf) {
    _z_ping_pong_result_t r;
    _z_ping_pong_decode_na(zbf, &r);
    return r;
}

/*------------------ Frame Message ------------------*/
int8_t _z_frame_encode(_z_wbuf_t *wbf, uint8_t header, const _z_t_msg_frame_t *msg) {
    int8_t ret = _Z_RES_OK;
    _Z_DEBUG("Encoding _Z_MID_FRAME\n");

    // Encode the body
    _Z_EC(_z_zint_encode(wbf, msg->_sn))

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_F) == true) {
        // Do not write the fragment as _z_bytes_t since the total frame length
        // is eventually prepended to the frame. There is no need to encode the fragment length.
        ret = _z_wbuf_write_bytes(wbf, msg->_payload._fragment.start, 0, msg->_payload._fragment.len);
    } else {
        size_t len = _z_zenoh_message_vec_len(&msg->_payload._messages);
        for (size_t i = 0; i < len; i++) {
            _Z_EC(_z_zenoh_message_encode(wbf, _z_zenoh_message_vec_get(&msg->_payload._messages, i)))
        }
    }

    return ret;
}

void _z_frame_decode_na(_z_zbuf_t *zbf, uint8_t header, _z_frame_result_t *r) {
    _Z_DEBUG("Decoding _Z_MID_FRAME\n");
    r->_tag = _Z_RES_OK;

    _z_zint_result_t r_zint = _z_zint_decode(zbf);
    _ASSURE_P_RESULT(r_zint, r, _Z_ERR_PARSE_ZINT)
    r->_value._sn = r_zint._value;

    // Decode the payload
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_F) == true) {
        // Read all the remaining bytes in the buffer as the fragment
        r->_value._payload._fragment = _z_bytes_wrap(_z_zbuf_get_rptr(zbf), _z_zbuf_len(zbf));

        // We need to manually move the r_pos to w_pos, we have read it all
        _z_zbuf_set_rpos(zbf, _z_zbuf_get_wpos(zbf));
    } else {
        r->_value._payload._messages = _z_zenoh_message_vec_make(_ZENOH_PICO_FRAME_MESSAGES_VEC_SIZE);
        while (_z_zbuf_len(zbf) > 0) {
            // Mark the reading position of the iobfer
            size_t r_pos = _z_zbuf_get_rpos(zbf);
            _z_zenoh_message_result_t r_zm = _z_zenoh_message_decode(zbf);
            if (r_zm._tag == _Z_RES_OK) {
                _z_zenoh_message_t *zm = (_z_zenoh_message_t *)z_malloc(sizeof(_z_zenoh_message_t));
                (void)memcpy(zm, &r_zm._value, sizeof(_z_zenoh_message_t));
                _z_zenoh_message_vec_append(&r->_value._payload._messages, zm);
            } else {
                // Restore the reading position of the iobfer
                _z_zbuf_set_rpos(zbf, r_pos);
                break;
            }
        }
    }
}

_z_frame_result_t _z_frame_decode(_z_zbuf_t *zbf, uint8_t header) {
    _z_frame_result_t r;
    _z_frame_decode_na(zbf, header, &r);
    return r;
}

/*------------------ Transport Message ------------------*/
int8_t _z_transport_message_encode(_z_wbuf_t *wbf, const _z_transport_message_t *msg) {
    int8_t ret = _Z_RES_OK;

    // Encode the decorators if present
    if (msg->_attachment != NULL) {
        _Z_EC(_z_attachment_encode(wbf, msg->_attachment))
    }

    // Encode the header
    _Z_EC(_z_wbuf_write(wbf, msg->_header))

    // Encode the body
    switch (_Z_MID(msg->_header)) {
        case _Z_MID_FRAME: {
            ret = _z_frame_encode(wbf, msg->_header, &msg->_body._frame);
        } break;

        case _Z_MID_SCOUT: {
            ret = _z_scout_encode(wbf, msg->_header, &msg->_body._scout);
        } break;

        case _Z_MID_HELLO: {
            ret = _z_hello_encode(wbf, msg->_header, &msg->_body._hello);
        } break;

        case _Z_MID_JOIN: {
            ret = _z_join_encode(wbf, msg->_header, &msg->_body._join);
        } break;

        case _Z_MID_INIT: {
            ret = _z_init_encode(wbf, msg->_header, &msg->_body._init);
        } break;

        case _Z_MID_OPEN: {
            ret = _z_open_encode(wbf, msg->_header, &msg->_body._open);
        } break;

        case _Z_MID_CLOSE: {
            ret = _z_close_encode(wbf, msg->_header, &msg->_body._close);
        } break;

        case _Z_MID_SYNC: {
            ret = _z_sync_encode(wbf, msg->_header, &msg->_body._sync);
        } break;

        case _Z_MID_ACK_NACK: {
            ret = _z_ack_nack_encode(wbf, msg->_header, &msg->_body._ack_nack);
        } break;

        case _Z_MID_KEEP_ALIVE: {
            ret = _z_keep_alive_encode(wbf, msg->_header, &msg->_body._keep_alive);
        } break;

        case _Z_MID_PING_PONG: {
            ret = _z_ping_pong_encode(wbf, &msg->_body._ping_pong);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to encode session message with unknown ID(%d)\n", _Z_MID(msg->_header));
            ret = _Z_ERR_UNKNOWN_MESSAGE;
        } break;
    }

    return ret;
}

void _z_transport_message_decode_na(_z_zbuf_t *zbf, _z_transport_message_result_t *r) {
    _Bool is_last = false;
    r->_tag = _Z_RES_OK;
    r->_value._attachment = NULL;

    do {
        // Decode the header
        _z_uint8_result_t r_uint8 = _z_uint8_decode(zbf);
        _ASSURE_P_RESULT(r_uint8, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
        r->_value._header = r_uint8._value;

        uint8_t mid = _Z_MID(r->_value._header);
        switch (mid) {
            case _Z_MID_FRAME: {
                _z_frame_result_t r_fr = _z_frame_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_fr, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._frame = r_fr._value;
                is_last = true;
            } break;

            case _Z_MID_ATTACHMENT: {
                _z_attachment_p_result_t r_at = _z_attachment_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_at, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._attachment = r_at._value;
            } break;

            case _Z_MID_SCOUT: {
                _z_scout_result_t r_sc = _z_scout_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_sc, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._scout = r_sc._value;
                is_last = true;
            } break;

            case _Z_MID_HELLO: {
                _z_hello_result_t r_he = _z_hello_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_he, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._hello = r_he._value;
                is_last = true;
            } break;

            case _Z_MID_JOIN: {
                _z_join_result_t r_jo = _z_join_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_jo, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._join = r_jo._value;
                is_last = true;
            } break;

            case _Z_MID_INIT: {
                _z_init_result_t r_in = _z_init_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_in, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._init = r_in._value;
                is_last = true;
            } break;

            case _Z_MID_OPEN: {
                _z_open_result_t r_op = _z_open_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_op, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._open = r_op._value;
                is_last = true;
            } break;

            case _Z_MID_CLOSE: {
                _z_close_result_t r_cl = _z_close_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_cl, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._close = r_cl._value;
                is_last = true;
            } break;

            case _Z_MID_SYNC: {
                _z_sync_result_t r_sy = _z_sync_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_sy, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._sync = r_sy._value;
                is_last = true;
            } break;

            case _Z_MID_ACK_NACK: {
                _z_ack_nack_result_t r_an = _z_ack_nack_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_an, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._ack_nack = r_an._value;
                is_last = true;
            } break;

            case _Z_MID_KEEP_ALIVE: {
                _z_keep_alive_result_t r_ka = _z_keep_alive_decode(zbf, r->_value._header);
                _ASSURE_P_RESULT(r_ka, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._keep_alive = r_ka._value;
                is_last = true;
            } break;

            case _Z_MID_PING_PONG: {
                _z_ping_pong_result_t r_pp = _z_ping_pong_decode(zbf);
                _ASSURE_P_RESULT(r_pp, r, _Z_ERR_PARSE_TRANSPORT_MESSAGE)
                r->_value._body._ping_pong = r_pp._value;
                is_last = true;
            } break;

            case _Z_MID_PRIORITY: {  // FIXME: implement PRIORITY parsing
                _Z_DEBUG("WARNING: Priorities are not supported in zenoh-pico\n");
                r->_tag = _Z_ERR_PARSE_TRANSPORT_MESSAGE;
                is_last = true;
            } break;

            default: {
                _Z_DEBUG("WARNING: Trying to decode session message with unknown ID(%d)\n", mid);
                r->_tag = _Z_ERR_PARSE_TRANSPORT_MESSAGE;
                is_last = true;
            } break;
        }

        if (is_last == true) {  // Do not break in case of decorators
            break;
        }

    } while (1);
}

_z_transport_message_result_t _z_transport_message_decode(_z_zbuf_t *zbf) {
    _z_transport_message_result_t r;
    _z_transport_message_decode_na(zbf, &r);
    return r;
}
