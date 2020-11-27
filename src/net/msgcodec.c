/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/private/codec.h"
#include "zenoh-pico/private/collection.h"
#include "zenoh-pico/private/logging.h"
#include "zenoh-pico/net/private/codec.h"
#include "zenoh-pico/net/private/msgcodec.h"

/*=============================*/
/*           Fields            */
/*=============================*/
/*------------------ Payload field ------------------*/
int _zn_payload_encode(_z_wbuf_t *wbf, const _zn_payload_t *pld)
{
    _Z_DEBUG("Encoding _PAYLOAD\n");

    // Encode the body
    return _z_bytes_encode(wbf, pld);
}

void _zn_payload_decode_na(_z_rbuf_t *rbf, _zn_payload_result_t *r)
{
    _Z_DEBUG("Decoding _PAYLOAD\n");
    r->tag = _z_res_t_OK;

    _z_bytes_result_t r_arr = _z_bytes_decode(rbf);
    _ASSURE_P_RESULT(r_arr, r, _z_err_t_PARSE_BYTES);
    r->value.payload = r_arr.value.bytes;
}

_zn_payload_result_t _zn_payload_decode(_z_rbuf_t *rbf)
{
    _zn_payload_result_t r;
    _zn_payload_decode_na(rbf, &r);
    return r;
}

void _zn_payload_free(_zn_payload_t *p)
{
    (void)(p);
}

/*------------------ Timestamp Field ------------------*/
int _z_timestamp_encode(_z_wbuf_t *wbf, const z_timestamp_t *ts)
{
    _Z_DEBUG("Encoding _TIMESTAMP\n");

    // Encode the body
    _ZN_EC(_z_zint_encode(wbf, ts->time))
    return _z_bytes_encode(wbf, &ts->id);
}

void _z_timestamp_decode_na(_z_rbuf_t *rbf, _zn_timestamp_result_t *r)
{
    _Z_DEBUG("Decoding _TIMESTAMP\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.timestamp.time = r_zint.value.zint;

    _z_bytes_result_t r_arr = _z_bytes_decode(rbf);
    _ASSURE_P_RESULT(r_arr, r, _z_err_t_PARSE_BYTES);
    r->value.timestamp.id = r_arr.value.bytes;
}

_zn_timestamp_result_t _z_timestamp_decode(_z_rbuf_t *rbf)
{
    _zn_timestamp_result_t r;
    _z_timestamp_decode_na(rbf, &r);
    return r;
}

void _z_timestamp_free(z_timestamp_t *ts)
{
    (void)(&ts->id);
}

/*------------------ SubMode Field ------------------*/
int _zn_subinfo_encode(_z_wbuf_t *wbf, const zn_subinfo_t *fld)
{
    _Z_DEBUG("Encoding _SUB_MODE\n");

    // Encode the header
    uint8_t header = fld->mode;
    if (fld->period)
        _ZN_SET_FLAG(header, _ZN_FLAG_Z_P);
    _ZN_EC(_z_wbuf_write(wbf, header))

    // Encode the body
    if (fld->period)
        return _zn_period_encode(wbf, fld->period);

    return 0;
}

void _zn_subinfo_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_subinfo_result_t *r)
{
    _Z_DEBUG("Decoding _SUB_MODE\n");
    r->tag = _z_res_t_OK;

    // Decode the header
    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_R)
        r->value.subinfo.reliability = zn_reliability_t_RELIABLE;
    else
        r->value.subinfo.reliability = zn_reliability_t_BEST_EFFORT;

    // Decode the body
    uint8_t mode = _z_rbuf_read(rbf);
    r->value.subinfo.mode = _ZN_MID(mode);
    if _ZN_HAS_FLAG (mode, _ZN_FLAG_Z_P)
    {
        _zn_period_result_t r_tp = _zn_period_decode(rbf);
        _ASSURE_P_RESULT(r_tp, r, _zn_err_t_PARSE_PERIOD)
        zn_period_t *p_per = (zn_period_t *)malloc(sizeof(zn_period_t));
        memcpy(p_per, &r_tp.value.period, sizeof(zn_period_t));
        r->value.subinfo.period = p_per;
    }
    else
    {
        r->value.subinfo.period = NULL;
    }
}

_zn_subinfo_result_t _zn_subinfo_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_subinfo_result_t r;
    _zn_subinfo_decode_na(rbf, header, &r);
    return r;
}

void _zn_subinfo_free(zn_subinfo_t *si)
{
    if (si->period)
        free(si->period);
}

/*------------------ ResKey Field ------------------*/
int _zn_reskey_encode(_z_wbuf_t *wbf, uint8_t header, const zn_reskey_t *fld)
{
    _Z_DEBUG("Encoding _RESKEY\n");

    // Encode the body
    _ZN_EC(_z_zint_encode(wbf, fld->rid))
    if (!_ZN_HAS_FLAG(header, _ZN_FLAG_Z_K))
    {
        return _z_str_encode(wbf, fld->rname);
    }

    return 0;
}

void _zn_reskey_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_reskey_result_t *r)
{
    _Z_DEBUG("Decoding _RESKEY\n");
    r->tag = _z_res_t_OK;

    // Decode the header
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.reskey.rid = r_zint.value.zint;

    if (!_ZN_HAS_FLAG(header, _ZN_FLAG_Z_K))
    {
        _z_str_result_t r_str = _z_str_decode(rbf);
        _ASSURE_P_RESULT(r_str, r, _z_err_t_PARSE_STRING)
        r->value.reskey.rname = r_str.value.str;
    }
    else
    {
        r->value.reskey.rname = NULL;
    }
}

_zn_reskey_result_t _zn_reskey_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_reskey_result_t r;
    _zn_reskey_decode_na(rbf, header, &r);
    return r;
}

void _zn_reskey_free(zn_reskey_t *rk)
{
    rk->rid = 0;
    if (rk->rname)
    {
        free(rk->rname);
        rk->rname = NULL;
    }
}

/*------------------ Locators Field ------------------*/
int _zn_locators_encode(_z_wbuf_t *wbf, const _zn_locators_t *ls)
{
    _ZN_EC(_z_zint_encode(wbf, ls->len))
    // Encode the locators
    for (size_t i = 0; i < ls->len; ++i)
    {
        _ZN_EC(_z_str_encode(wbf, (z_str_t)ls->val[i]))
    }

    return 0;
}

void _zn_locators_decode_na(_z_rbuf_t *rbf, _zn_locators_result_t *r)
{
    r->tag = _z_res_t_OK;

    // Decode the number of elements
    _z_zint_result_t r_n = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_n, r, _z_err_t_PARSE_ZINT)
    size_t len = (size_t)r_n.value.zint;

    _z_str_array_init(&r->value.locators, len);

    // Decode the elements
    for (size_t i = 0; i < len; ++i)
    {
        _z_str_result_t r_s = _z_str_decode(rbf);
        _ASSURE_P_RESULT(r_s, r, _z_err_t_PARSE_STRING);
        ((z_str_t *)r->value.locators.val)[i] = r_s.value.str;
    }
}

_zn_locators_result_t _zn_locators_decode(_z_rbuf_t *rbf)
{
    _zn_locators_result_t r;
    _zn_locators_decode_na(rbf, &r);
    return r;
}

void _zn_locators_free(_zn_locators_t *ls)
{
    _z_str_array_free(ls);
}

/*=============================*/
/*     Message decorators      */
/*=============================*/
/*------------------ Attachement Decorator ------------------*/
int _zn_attachment_encode(_z_wbuf_t *wbf, const _zn_attachment_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ATTACHMENT\n");

    // Encode the header
    _ZN_EC(_z_wbuf_write(wbf, msg->header))

    // Encode the body
    return _zn_payload_encode(wbf, &msg->payload);
}

void _zn_attachment_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_attachment_p_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ATTACHMENT\n");
    r->tag = _z_res_t_OK;

    // Store the header
    r->value.attachment->header = header;

    // Decode the body
    _zn_payload_result_t r_pld = _zn_payload_decode(rbf);
    _ASSURE_FREE_P_RESULT(r_pld, r, _zn_err_t_PARSE_PAYLOAD, attachment)
    r->value.attachment->payload = r_pld.value.payload;
}

_zn_attachment_p_result_t _zn_attachment_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_attachment_p_result_t r;
    _zn_attachment_p_result_init(&r);
    _zn_attachment_decode_na(rbf, header, &r);
    return r;
}

void _zn_attachment_free(_zn_attachment_t *a)
{
    _zn_payload_free(&a->payload);
}

/*------------------ ReplyContext Decorator ------------------*/
int _zn_reply_context_encode(_z_wbuf_t *wbf, const _zn_reply_context_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_REPLY_CONTEXT\n");

    // Encode the header
    _ZN_EC(_z_wbuf_write(wbf, msg->header))

    // Encode the body
    _ZN_EC(_z_zint_encode(wbf, msg->qid))
    _ZN_EC(_z_zint_encode(wbf, msg->source_kind))
    if (!_ZN_HAS_FLAG(msg->header, _ZN_FLAG_Z_F))
        return _z_bytes_encode(wbf, &msg->replier_id);

    return 0;
}

void _zn_reply_context_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_reply_context_p_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_REPLY_CONTEXT\n");
    r->tag = _z_res_t_OK;

    // Store the header
    r->value.reply_context->header = header;

    // Decode the body
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_FREE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT, reply_context);
    r->value.reply_context->qid = r_zint.value.zint;

    r_zint = _z_zint_decode(rbf);
    _ASSURE_FREE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT, reply_context)
    r->value.reply_context->source_kind = r_zint.value.zint;

    if (!_ZN_HAS_FLAG(header, _ZN_FLAG_Z_F))
    {
        _z_bytes_result_t r_arr = _z_bytes_decode(rbf);
        _ASSURE_FREE_P_RESULT(r_arr, r, _z_err_t_PARSE_BYTES, reply_context)
        r->value.reply_context->replier_id = r_arr.value.bytes;
    }
}

_zn_reply_context_p_result_t _zn_reply_context_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_reply_context_p_result_t r;
    _zn_reply_context_p_result_init(&r);
    _zn_reply_context_decode_na(rbf, header, &r);
    return r;
}

void _zn_reply_context_free(_zn_reply_context_t *rc)
{
    if (!_ZN_HAS_FLAG(rc->header, _ZN_FLAG_Z_F))
        (void)(&rc->replier_id);
}

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/
/*------------------ Resource Declaration ------------------*/
int _zn_res_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_res_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_RESOURCE\n");

    // Encode the body
    _ZN_EC(_z_zint_encode(wbf, dcl->id))
    return _zn_reskey_encode(wbf, header, &dcl->key);
}

void _zn_res_decl_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_res_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_RESOURCE\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.res_decl.id = r_zint.value.zint;

    _zn_reskey_result_t r_res = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_res, r, _zn_err_t_PARSE_RESKEY)
    r->value.res_decl.key = r_res.value.reskey;
}

_zn_res_decl_result_t _zn_res_decl_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_res_decl_result_t r;
    _zn_res_decl_decode_na(rbf, header, &r);
    return r;
}

void _zn_res_decl_free(_zn_res_decl_t *dcl)
{
    _zn_reskey_free(&dcl->key);
}

/*------------------ Publisher Declaration ------------------*/
int _zn_pub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_pub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_PUBLISHER\n");

    // Encode the body
    return _zn_reskey_encode(wbf, header, &dcl->key);
}

void _zn_pub_decl_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_pub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_PUBLISHER\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _zn_reskey_result_t r_res = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_res, r, _zn_err_t_PARSE_RESKEY)
    r->value.pub_decl.key = r_res.value.reskey;
}

_zn_pub_decl_result_t _zn_pub_decl_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_pub_decl_result_t r;
    _zn_pub_decl_decode_na(rbf, header, &r);
    return r;
}

void _zn_pub_decl_free(_zn_pub_decl_t *dcl)
{
    _zn_reskey_free(&dcl->key);
}

/*------------------ Subscriber Declaration ------------------*/
int _zn_sub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_sub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_SUBSCRIBER\n");

    // Encode the body
    _ZN_EC(_zn_reskey_encode(wbf, header, &dcl->key))
    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_S)
        return _zn_subinfo_encode(wbf, &dcl->subinfo);

    return 0;
}

void _zn_sub_decl_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_sub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_SUBSCRIBER\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _zn_reskey_result_t r_res = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_res, r, _zn_err_t_PARSE_RESKEY)
    r->value.sub_decl.key = r_res.value.reskey;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_S)
    {
        _zn_subinfo_result_t r_smd = _zn_subinfo_decode(rbf, header);
        _ASSURE_P_RESULT(r_smd, r, _zn_err_t_PARSE_SUBMODE)
        r->value.sub_decl.subinfo = r_smd.value.subinfo;
    }
    else
    {
        // Default subscription mode is non-periodic PUSH
        r->value.sub_decl.subinfo.mode = zn_submode_t_PUSH;
        r->value.sub_decl.subinfo.period = NULL;
        if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_R)
            r->value.sub_decl.subinfo.reliability = zn_reliability_t_RELIABLE;
        else
            r->value.sub_decl.subinfo.reliability = zn_reliability_t_BEST_EFFORT;
    }
}

_zn_sub_decl_result_t _zn_sub_decl_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_sub_decl_result_t r;
    _zn_sub_decl_decode_na(rbf, header, &r);
    return r;
}

void _zn_sub_decl_free(_zn_sub_decl_t *dcl)
{
    _zn_reskey_free(&dcl->key);
    _zn_subinfo_free(&dcl->subinfo);
}

/*------------------ Queryable Declaration ------------------*/
int _zn_qle_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_qle_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_QUERYABLE\n");

    // Encode the body
    return _zn_reskey_encode(wbf, header, &dcl->key);
}

void _zn_qle_decl_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_qle_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_QUERYABLE\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _zn_reskey_result_t r_res = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_res, r, _zn_err_t_PARSE_RESKEY)
    r->value.qle_decl.key = r_res.value.reskey;
}

_zn_qle_decl_result_t _zn_qle_decl_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_qle_decl_result_t r;
    _zn_qle_decl_decode_na(rbf, header, &r);
    return r;
}

void _zn_qle_decl_free(_zn_qle_decl_t *dcl)
{
    _zn_reskey_free(&dcl->key);
}

/*------------------ Forget Resource Declaration ------------------*/
int _zn_forget_res_decl_encode(_z_wbuf_t *wbf, const _zn_forget_res_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_RESOURCE\n");

    // Encode the body
    return _z_zint_encode(wbf, dcl->rid);
}

void _zn_forget_res_decl_decode_na(_z_rbuf_t *rbf, _zn_forget_res_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_RESOURCE\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.forget_res_decl.rid = r_zint.value.zint;
}

_zn_forget_res_decl_result_t _zn_forget_res_decl_decode(_z_rbuf_t *rbf)
{
    _zn_forget_res_decl_result_t r;
    _zn_forget_res_decl_decode_na(rbf, &r);
    return r;
}

void _zn_forget_res_decl_free(_zn_forget_res_decl_t *dcl)
{
    // NOTE: forget_res_decl does not require any heap allocation
    (void)(dcl);
}

/*------------------ Forget Publisher Declaration ------------------*/
int _zn_forget_pub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_forget_pub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_PUBLISHER\n");

    // Encode the body
    return _zn_reskey_encode(wbf, header, &dcl->key);
}

void _zn_forget_pub_decl_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_forget_pub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_PUBLISHER\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _zn_reskey_result_t r_res = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_res, r, _zn_err_t_PARSE_RESKEY)
    r->value.forget_pub_decl.key = r_res.value.reskey;
}

_zn_forget_pub_decl_result_t _zn_forget_pub_decl_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_forget_pub_decl_result_t r;
    _zn_forget_pub_decl_decode_na(rbf, header, &r);
    return r;
}

void _zn_forget_pub_decl_free(_zn_forget_pub_decl_t *dcl)
{
    _zn_reskey_free(&dcl->key);
}

/*------------------ Forget Subscriber Declaration ------------------*/
int _zn_forget_sub_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_forget_sub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_PUBLISHER\n");

    // Encode the body
    return _zn_reskey_encode(wbf, header, &dcl->key);
}

void _zn_forget_sub_decl_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_forget_sub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_PUBLISHER\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _zn_reskey_result_t r_res = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_res, r, _zn_err_t_PARSE_RESKEY)
    r->value.forget_sub_decl.key = r_res.value.reskey;
}

_zn_forget_sub_decl_result_t _zn_forget_sub_decl_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_forget_sub_decl_result_t r;
    _zn_forget_sub_decl_decode_na(rbf, header, &r);
    return r;
}

void _zn_forget_sub_decl_free(_zn_forget_sub_decl_t *dcl)
{
    _zn_reskey_free(&dcl->key);
}

/*------------------ Forget Queryable Declaration ------------------*/
int _zn_forget_qle_decl_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_forget_qle_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_QUERYABLE\n");

    // Encode the body
    return _zn_reskey_encode(wbf, header, &dcl->key);
}

void _zn_forget_qle_decl_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_forget_qle_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_QUERYABLE\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _zn_reskey_result_t r_res = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_res, r, _zn_err_t_PARSE_RESKEY)
    r->value.forget_qle_decl.key = r_res.value.reskey;
}

_zn_forget_qle_decl_result_t _zn_forget_qle_decl_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_forget_qle_decl_result_t r;
    _zn_forget_qle_decl_decode_na(rbf, header, &r);
    return r;
}

void _zn_forget_qle_decl_free(_zn_forget_qle_decl_t *dcl)
{
    _zn_reskey_free(&dcl->key);
}

/*------------------ Declaration Field ------------------*/
int _zn_declaration_encode(_z_wbuf_t *wbf, _zn_declaration_t *dcl)
{
    // Encode the header
    _ZN_EC(_z_wbuf_write(wbf, dcl->header))
    // Encode the body
    uint8_t did = _ZN_MID(dcl->header);
    switch (did)
    {
    case _ZN_DECL_RESOURCE:
        return _zn_res_decl_encode(wbf, dcl->header, &dcl->body.res);
    case _ZN_DECL_PUBLISHER:
        return _zn_pub_decl_encode(wbf, dcl->header, &dcl->body.pub);
    case _ZN_DECL_SUBSCRIBER:
        return _zn_sub_decl_encode(wbf, dcl->header, &dcl->body.sub);
    case _ZN_DECL_QUERYABLE:
        return _zn_qle_decl_encode(wbf, dcl->header, &dcl->body.qle);
    case _ZN_DECL_FORGET_RESOURCE:
        return _zn_forget_res_decl_encode(wbf, &dcl->body.forget_res);
    case _ZN_DECL_FORGET_PUBLISHER:
        return _zn_forget_pub_decl_encode(wbf, dcl->header, &dcl->body.forget_pub);
    case _ZN_DECL_FORGET_SUBSCRIBER:
        return _zn_forget_sub_decl_encode(wbf, dcl->header, &dcl->body.forget_sub);
    case _ZN_DECL_FORGET_QUERYABLE:
        return _zn_forget_qle_decl_encode(wbf, dcl->header, &dcl->body.forget_qle);
    default:
        _Z_ERROR("WARNING: Trying to encode declaration with unknown ID(%d)\n", did);
        return -1;
    }
}

void _zn_declaration_decode_na(_z_rbuf_t *rbf, _zn_declaration_result_t *r)
{
    r->tag = _z_res_t_OK;

    // Decode the header
    r->value.declaration.header = _z_rbuf_read(rbf);
    uint8_t mid = _ZN_MID(r->value.declaration.header);

    // Decode the body
    _zn_res_decl_result_t r_rdcl;
    _zn_pub_decl_result_t r_pdcl;
    _zn_sub_decl_result_t r_sdcl;
    _zn_qle_decl_result_t r_qdcl;
    _zn_forget_res_decl_result_t r_frdcl;
    _zn_forget_pub_decl_result_t r_fpdcl;
    _zn_forget_sub_decl_result_t r_fsdcl;
    _zn_forget_qle_decl_result_t r_fqdcl;

    switch (mid)
    {
    case _ZN_DECL_RESOURCE:
        r_rdcl = _zn_res_decl_decode(rbf, r->value.declaration.header);
        _ASSURE_P_RESULT(r_rdcl, r, _zn_err_t_PARSE_DECLARATION)
        r->value.declaration.body.res = r_rdcl.value.res_decl;
        break;
    case _ZN_DECL_PUBLISHER:
        r_pdcl = _zn_pub_decl_decode(rbf, r->value.declaration.header);
        _ASSURE_P_RESULT(r_pdcl, r, _zn_err_t_PARSE_DECLARATION)
        r->value.declaration.body.pub = r_pdcl.value.pub_decl;
        break;
    case _ZN_DECL_SUBSCRIBER:
        r_sdcl = _zn_sub_decl_decode(rbf, r->value.declaration.header);
        _ASSURE_P_RESULT(r_sdcl, r, _zn_err_t_PARSE_DECLARATION)
        r->value.declaration.body.sub = r_sdcl.value.sub_decl;
        break;
    case _ZN_DECL_QUERYABLE:
        r_qdcl = _zn_qle_decl_decode(rbf, r->value.declaration.header);
        _ASSURE_P_RESULT(r_qdcl, r, _zn_err_t_PARSE_DECLARATION)
        r->value.declaration.body.qle = r_qdcl.value.qle_decl;
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        r_frdcl = _zn_forget_res_decl_decode(rbf);
        _ASSURE_P_RESULT(r_frdcl, r, _zn_err_t_PARSE_DECLARATION)
        r->value.declaration.body.forget_res = r_frdcl.value.forget_res_decl;
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        r_fpdcl = _zn_forget_pub_decl_decode(rbf, r->value.declaration.header);
        _ASSURE_P_RESULT(r_fpdcl, r, _zn_err_t_PARSE_DECLARATION)
        r->value.declaration.body.forget_pub = r_fpdcl.value.forget_pub_decl;
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        r_fsdcl = _zn_forget_sub_decl_decode(rbf, r->value.declaration.header);
        _ASSURE_P_RESULT(r_fsdcl, r, _zn_err_t_PARSE_DECLARATION)
        r->value.declaration.body.forget_sub = r_fsdcl.value.forget_sub_decl;
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        r_fqdcl = _zn_forget_qle_decl_decode(rbf, r->value.declaration.header);
        _ASSURE_P_RESULT(r_fqdcl, r, _zn_err_t_PARSE_DECLARATION)
        r->value.declaration.body.forget_qle = r_fqdcl.value.forget_qle_decl;
        break;
    default:
        _Z_ERROR("WARNING: Trying to decode declaration with unknown ID(%d)\n", mid);
        break;
    }
}

_zn_declaration_result_t _zn_declaration_decode(_z_rbuf_t *rbf)
{
    _zn_declaration_result_t r;
    _zn_declaration_decode_na(rbf, &r);
    return r;
}

void _zn_declaration_free(_zn_declaration_t *dcl)
{
    uint8_t did = _ZN_MID(dcl->header);
    switch (did)
    {
    case _ZN_DECL_RESOURCE:
        _zn_res_decl_free(&dcl->body.res);
        break;
    case _ZN_DECL_PUBLISHER:
        _zn_pub_decl_free(&dcl->body.pub);
        break;
    case _ZN_DECL_SUBSCRIBER:
        _zn_sub_decl_free(&dcl->body.sub);
        break;
    case _ZN_DECL_QUERYABLE:
        _zn_qle_decl_free(&dcl->body.qle);
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        _zn_forget_res_decl_free(&dcl->body.forget_res);
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        _zn_forget_pub_decl_free(&dcl->body.forget_pub);
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        _zn_forget_sub_decl_free(&dcl->body.forget_sub);
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        _zn_forget_qle_decl_free(&dcl->body.forget_qle);
        break;
    default:
        _Z_ERROR("WARNING: Trying to free declaration with unknown ID(%d)\n", did);
        break;
    }
}

/*------------------ Declaration Message ------------------*/
int _zn_declare_encode(_z_wbuf_t *wbf, const _zn_declare_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_DECLARE\n");

    // Encode the body
    z_zint_t len = msg->declarations.len;
    _ZN_EC(_z_zint_encode(wbf, len))
    for (z_zint_t i = 0; i < len; ++i)
    {
        _ZN_EC(_zn_declaration_encode(wbf, &msg->declarations.val[i]))
    }

    return 0;
}

void _zn_declare_decode_na(_z_rbuf_t *rbf, _zn_declare_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_DECLARE\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_dlen = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_dlen, r, _z_err_t_PARSE_ZINT)
    size_t len = (size_t)r_dlen.value.zint;

    r->value.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));
    r->value.declare.declarations.len = len;

    _zn_declaration_result_t *r_decl = (_zn_declaration_result_t *)malloc(sizeof(_zn_declaration_result_t));
    for (size_t i = 0; i < len; ++i)
    {
        _zn_declaration_decode_na(rbf, r_decl);
        if (r_decl->tag == _z_res_t_OK)
        {
            r->value.declare.declarations.val[i] = r_decl->value.declaration;
        }
        else
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_PARSE_ZENOH_MESSAGE;

            for (size_t j = 0; j < i; ++j)
                _zn_declaration_free(&r->value.declare.declarations.val[j]);
            free(r->value.declare.declarations.val);

            break;
        }
    }
    free(r_decl);
}

_zn_declare_result_t _zn_declare_decode(_z_rbuf_t *rbf)
{
    _zn_declare_result_t r;
    _zn_declare_decode_na(rbf, &r);
    return r;
}

void _zn_declare_free(_zn_declare_t *dcl)
{
    for (size_t i = 0; i < dcl->declarations.len; ++i)
        _zn_declaration_free(&dcl->declarations.val[i]);
    free(dcl->declarations.val);
}

/*------------------ Data Info Field ------------------*/
int _zn_data_info_encode(_z_wbuf_t *wbf, const _zn_data_info_t *fld)
{
    _Z_DEBUG("Encoding _ZN_DATA_INFO\n");

    // Encode the flags
    _ZN_EC(_z_zint_encode(wbf, fld->flags))

    // Encode the body
    if _ZN_HAS_FLAG (fld->flags, _ZN_DATA_INFO_SRC_ID)
        _ZN_EC(_z_bytes_encode(wbf, &fld->source_id))

    if _ZN_HAS_FLAG (fld->flags, _ZN_DATA_INFO_SRC_SN)
        _ZN_EC(_z_zint_encode(wbf, fld->source_sn))

    if _ZN_HAS_FLAG (fld->flags, _ZN_DATA_INFO_RTR_ID)
        _ZN_EC(_z_bytes_encode(wbf, &fld->first_router_id))

    if _ZN_HAS_FLAG (fld->flags, _ZN_DATA_INFO_RTR_SN)
        _ZN_EC(_z_zint_encode(wbf, fld->first_router_sn))

    if _ZN_HAS_FLAG (fld->flags, _ZN_DATA_INFO_TSTAMP)
        _ZN_EC(_z_timestamp_encode(wbf, &fld->tstamp))

    if _ZN_HAS_FLAG (fld->flags, _ZN_DATA_INFO_KIND)
        _ZN_EC(_z_zint_encode(wbf, fld->kind))

    if _ZN_HAS_FLAG (fld->flags, _ZN_DATA_INFO_ENC)
        _ZN_EC(_z_zint_encode(wbf, fld->encoding))

    return 0;
}

void _zn_data_info_decode_na(_z_rbuf_t *rbf, _zn_data_info_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DATA_INFO\n");
    r->tag = _z_res_t_OK;

    // Decode the flags
    _z_zint_result_t r_flags = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_flags, r, _z_err_t_PARSE_ZINT)
    r->value.data_info.flags = r_flags.value.zint;

    // Decode the body
    if _ZN_HAS_FLAG (r->value.data_info.flags, _ZN_DATA_INFO_SRC_ID)
    {
        _z_bytes_result_t r_sid = _z_bytes_decode(rbf);
        _ASSURE_P_RESULT(r_sid, r, _z_err_t_PARSE_BYTES)
        r->value.data_info.source_id = r_sid.value.bytes;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, _ZN_DATA_INFO_SRC_SN)
    {
        _z_zint_result_t r_ssn = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_ssn, r, _z_err_t_PARSE_ZINT)
        r->value.data_info.source_sn = r_ssn.value.zint;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, _ZN_DATA_INFO_RTR_ID)
    {
        _z_bytes_result_t r_rid = _z_bytes_decode(rbf);
        _ASSURE_P_RESULT(r_rid, r, _z_err_t_PARSE_BYTES)
        r->value.data_info.first_router_id = r_rid.value.bytes;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, _ZN_DATA_INFO_RTR_SN)
    {
        _z_zint_result_t r_rsn = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_rsn, r, _z_err_t_PARSE_ZINT)
        r->value.data_info.first_router_sn = r_rsn.value.zint;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, _ZN_DATA_INFO_TSTAMP)
    {
        _zn_timestamp_result_t r_tsp = _z_timestamp_decode(rbf);
        _ASSURE_P_RESULT(r_tsp, r, _zn_err_t_PARSE_TIMESTAMP)
        r->value.data_info.tstamp = r_tsp.value.timestamp;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, _ZN_DATA_INFO_KIND)
    {
        _z_zint_result_t r_knd = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_knd, r, _z_err_t_PARSE_ZINT)
        r->value.data_info.kind = r_knd.value.zint;
    }

    if _ZN_HAS_FLAG (r->value.data_info.flags, _ZN_DATA_INFO_ENC)
    {
        _z_zint_result_t r_enc = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_enc, r, _z_err_t_PARSE_ZINT)
        r->value.data_info.encoding = r_enc.value.zint;
    }
}

_zn_data_info_result_t _zn_data_info_decode(_z_rbuf_t *rbf)
{
    _zn_data_info_result_t r;
    _zn_data_info_decode_na(rbf, &r);
    return r;
}

void _zn_data_info_free(_zn_data_info_t *di)
{
    // NOTE: the following fiels do not involve any heap allocation:
    //   - source_sn
    //   - first_router_sn
    //   - kind
    //   - encoding

    if _ZN_HAS_FLAG (di->flags, _ZN_DATA_INFO_SRC_ID)
        (void)(&di->source_id);

    if _ZN_HAS_FLAG (di->flags, _ZN_DATA_INFO_RTR_ID)
        (void)(&di->first_router_id);

    if _ZN_HAS_FLAG (di->flags, _ZN_DATA_INFO_TSTAMP)
        _z_timestamp_free(&di->tstamp);
}

/*------------------ Data Message ------------------*/
int _zn_data_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_data_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_DATA\n");

    // Encode the body
    _ZN_EC(_zn_reskey_encode(wbf, header, &msg->key))

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_I)
        _ZN_EC(_zn_data_info_encode(wbf, &msg->info))

    return _zn_payload_encode(wbf, &msg->payload);
}

void _zn_data_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_data_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_DATA\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _zn_reskey_result_t r_key = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_key, r, _zn_err_t_PARSE_RESKEY)
    r->value.data.key = r_key.value.reskey;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_I)
    {
        _zn_data_info_result_t r_dti = _zn_data_info_decode(rbf);
        _ASSURE_P_RESULT(r_dti, r, _zn_err_t_PARSE_ZENOH_MESSAGE)
        r->value.data.info = r_dti.value.data_info;
    }
    else
    {
        r->value.data.info.flags = 0;
    }

    _zn_payload_result_t r_pld = _zn_payload_decode(rbf);
    _ASSURE_P_RESULT(r_pld, r, _z_err_t_PARSE_ZINT)
    r->value.data.payload = r_pld.value.payload;
}

_zn_data_result_t _zn_data_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_data_result_t r;
    _zn_data_decode_na(rbf, header, &r);
    return r;
}

void _zn_data_free(_zn_data_t *msg)
{
    _zn_reskey_free(&msg->key);
    _zn_data_info_free(&msg->info);
    _zn_payload_free(&msg->payload);
}

/*------------------ Pull Message ------------------*/
int _zn_pull_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_pull_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_PULL\n");

    // Encode the body
    _ZN_EC(_zn_reskey_encode(wbf, header, &msg->key))

    _ZN_EC(_z_zint_encode(wbf, msg->pull_id))

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_N)
        _ZN_EC(_z_zint_encode(wbf, msg->max_samples))

    return 0;
}

void _zn_pull_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_pull_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_PULL\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _zn_reskey_result_t r_key = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_key, r, _zn_err_t_PARSE_RESKEY)
    r->value.pull.key = r_key.value.reskey;

    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.pull.pull_id = r_zint.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_N)
    {
        _z_zint_result_t r_zint = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
        r->value.pull.max_samples = r_zint.value.zint;
    }
}

_zn_pull_result_t _zn_pull_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_pull_result_t r;
    _zn_pull_decode_na(rbf, header, &r);
    return r;
}

void _zn_pull_free(_zn_pull_t *msg)
{
    _zn_reskey_free(&msg->key);
}

/*------------------ Query Message ------------------*/
int _zn_query_target_encode(_z_wbuf_t *wbf, const zn_query_target_t *qt)
{
    _ZN_EC(_z_zint_encode(wbf, qt->kind))

    _ZN_EC(_z_zint_encode(wbf, qt->target.tag))
    if (qt->target.tag == zn_target_t_COMPLETE)
        _ZN_EC(_z_zint_encode(wbf, qt->target.type.complete.n))

    return 0;
}

int _zn_query_consolidation_encode(_z_wbuf_t *wbf, const zn_query_consolidation_t *qc)
{
    z_zint_t consolidation = qc->reception;
    consolidation |= qc->last_router << 2;
    consolidation |= qc->first_routers << 4;

    return _z_zint_encode(wbf, consolidation);
}

int _zn_query_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_query_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_QUERY\n");

    // Encode the body
    _ZN_EC(_zn_reskey_encode(wbf, header, &msg->key))

    _ZN_EC(_z_str_encode(wbf, msg->predicate))

    _ZN_EC(_z_zint_encode(wbf, msg->qid))

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_T)
        _ZN_EC(_zn_query_target_encode(wbf, &msg->target))

    return _zn_query_consolidation_encode(wbf, &msg->consolidation);
}

_zn_query_target_result_t _zn_query_target_decode(_z_rbuf_t *rbf)
{
    _zn_query_target_result_t r;
    r.tag = _z_res_t_OK;

    _z_zint_result_t r_kind = _z_zint_decode(rbf);
    _ASSURE_RESULT(r_kind, r, _z_err_t_PARSE_ZINT)
    r.value.query_target.kind = r_kind.value.zint;

    _z_zint_result_t r_tag = _z_zint_decode(rbf);
    _ASSURE_RESULT(r_tag, r, _z_err_t_PARSE_ZINT)
    r.value.query_target.target.tag = r_tag.value.zint;

    if (r.value.query_target.target.tag == zn_target_t_COMPLETE)
    {
        _z_zint_result_t r_n = _z_zint_decode(rbf);
        _ASSURE_RESULT(r_n, r, _z_err_t_PARSE_ZINT)
        r.value.query_target.target.type.complete.n = r_n.value.zint;
    }

    return r;
}

_zn_query_consolidation_result_t _zn_query_consolidation_decode(_z_rbuf_t *rbf)
{
    _zn_query_consolidation_result_t r;
    r.tag = _z_res_t_OK;

    _z_zint_result_t r_con = _z_zint_decode(rbf);
    _ASSURE_RESULT(r_con, r, _z_err_t_PARSE_ZINT)

    unsigned int mode = (r_con.value.zint >> 4) & 0x03;
    switch (mode)
    {
    case zn_consolidation_mode_t_NONE:
    case zn_consolidation_mode_t_LAZY:
    case zn_consolidation_mode_t_FULL:
        r.value.query_consolidation.first_routers = mode;
        break;
    default:
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_PARSE_CONSOLIDATION;
        return r;
    }

    mode = (r_con.value.zint >> 2) & 0x03;
    switch (mode)
    {
    case zn_consolidation_mode_t_NONE:
    case zn_consolidation_mode_t_LAZY:
    case zn_consolidation_mode_t_FULL:
        r.value.query_consolidation.last_router = mode;
        break;
    default:
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_PARSE_CONSOLIDATION;
        return r;
    }

    mode = r_con.value.zint & 0x03;
    switch (mode)
    {
    case zn_consolidation_mode_t_NONE:
    case zn_consolidation_mode_t_LAZY:
    case zn_consolidation_mode_t_FULL:
        r.value.query_consolidation.reception = mode;
        break;
    default:
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_PARSE_CONSOLIDATION;
        return r;
    }

    return r;
}

void _zn_query_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_query_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_QUERY\n");
    r->tag = _z_res_t_OK;

    _zn_reskey_result_t r_key = _zn_reskey_decode(rbf, header);
    _ASSURE_P_RESULT(r_key, r, _zn_err_t_PARSE_RESKEY)
    r->value.query.key = r_key.value.reskey;

    _z_str_result_t r_str = _z_str_decode(rbf);
    _ASSURE_P_RESULT(r_str, r, _z_err_t_PARSE_STRING)
    r->value.query.predicate = r_str.value.str;

    _z_zint_result_t r_qid = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_qid, r, _z_err_t_PARSE_ZINT)
    r->value.query.qid = r_qid.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_T)
    {
        _zn_query_target_result_t r_qt = _zn_query_target_decode(rbf);
        _ASSURE_P_RESULT(r_qt, r, _z_err_t_PARSE_ZINT)
        r->value.query.target = r_qt.value.query_target;
    }
    else
    {
        r->value.query.target.kind = ZN_QUERYABLE_ALL_KINDS;
        r->value.query.target.target.tag = zn_target_t_BEST_MATCHING;
    }

    _zn_query_consolidation_result_t r_con = _zn_query_consolidation_decode(rbf);
    _ASSURE_P_RESULT(r_con, r, _zn_err_t_PARSE_CONSOLIDATION)
    r->value.query.consolidation = r_con.value.query_consolidation;
}

_zn_query_result_t _zn_query_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_query_result_t r;
    _zn_query_decode_na(rbf, header, &r);
    return r;
}

void _zn_query_free(_zn_query_t *msg)
{
    _zn_reskey_free(&msg->key);
    free(msg->predicate);
}

/*------------------ Zenoh Message ------------------*/
int _zn_zenoh_message_encode(_z_wbuf_t *wbf, const _zn_zenoh_message_t *msg)
{
    // Encode the decorators if present
    if (msg->attachment)
        _ZN_EC(_zn_attachment_encode(wbf, msg->attachment))

    if (msg->reply_context)
        _ZN_EC(_zn_reply_context_encode(wbf, msg->reply_context))

    // Encode the header
    _ZN_EC(_z_wbuf_write(wbf, msg->header))

    // Encode the body
    uint8_t mid = _ZN_MID(msg->header);
    switch (mid)
    {
    case _ZN_MID_DECLARE:
        return _zn_declare_encode(wbf, &msg->body.declare);
    case _ZN_MID_DATA:
        return _zn_data_encode(wbf, msg->header, &msg->body.data);
    case _ZN_MID_PULL:
        return _zn_pull_encode(wbf, msg->header, &msg->body.pull);
    case _ZN_MID_QUERY:
        return _zn_query_encode(wbf, msg->header, &msg->body.query);
    case _ZN_MID_UNIT:
        // Do nothing. Unit messages have no body
        return 0;
    default:
        _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
        return -1;
    }
}

void _zn_zenoh_message_decode_na(_z_rbuf_t *rbf, _zn_zenoh_message_p_result_t *r)
{
    r->tag = _z_res_t_OK;

    r->value.zenoh_message->attachment = NULL;
    r->value.zenoh_message->reply_context = NULL;
    do
    {
        r->value.zenoh_message->header = _z_rbuf_read(rbf);

        switch (_ZN_MID(r->value.zenoh_message->header))
        {
        case _ZN_MID_DATA:
        {
            _zn_data_result_t r_da = _zn_data_decode(rbf, r->value.zenoh_message->header);
            _ASSURE_P_RESULT(r_da, r, _zn_err_t_PARSE_ZENOH_MESSAGE)
            r->value.zenoh_message->body.data = r_da.value.data;
            return;
        }
        case _ZN_MID_ATTACHMENT:
        {
            _zn_attachment_p_result_t r_at = _zn_attachment_decode(rbf, r->value.zenoh_message->header);
            _ASSURE_P_RESULT(r_at, r, _zn_err_t_PARSE_ZENOH_MESSAGE)
            r->value.zenoh_message->attachment = r_at.value.attachment;
            break;
        }
        case _ZN_MID_REPLY_CONTEXT:
        {
            _zn_reply_context_p_result_t r_rc = _zn_reply_context_decode(rbf, r->value.zenoh_message->header);
            _ASSURE_P_RESULT(r_rc, r, _zn_err_t_PARSE_ZENOH_MESSAGE)
            r->value.zenoh_message->reply_context = r_rc.value.reply_context;
            break;
        }
        case _ZN_MID_DECLARE:
        {
            _zn_declare_result_t r_de = _zn_declare_decode(rbf);
            _ASSURE_P_RESULT(r_de, r, _zn_err_t_PARSE_ZENOH_MESSAGE)
            r->value.zenoh_message->body.declare = r_de.value.declare;
            return;
        }
        case _ZN_MID_QUERY:
        {

            _zn_query_result_t r_qu = _zn_query_decode(rbf, r->value.zenoh_message->header);
            _ASSURE_P_RESULT(r_qu, r, _zn_err_t_PARSE_ZENOH_MESSAGE)
            r->value.zenoh_message->body.query = r_qu.value.query;
            return;
        }
        case _ZN_MID_PULL:
        {
            _zn_pull_result_t r_pu = _zn_pull_decode(rbf, r->value.zenoh_message->header);
            _ASSURE_P_RESULT(r_pu, r, _zn_err_t_PARSE_ZENOH_MESSAGE)
            r->value.zenoh_message->body.pull = r_pu.value.pull;
            return;
        }
        case _ZN_MID_UNIT:
        {
            // Do nothing. Unit messages have no body.
            return;
        }
        default:
        {
            _zn_zenoh_message_p_result_free(r);
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_PARSE_ZENOH_MESSAGE;
            return;
        }
        }
    } while (1);
}

_zn_zenoh_message_p_result_t _zn_zenoh_message_decode(_z_rbuf_t *rbf)
{
    _zn_zenoh_message_p_result_t r;
    _zn_zenoh_message_p_result_init(&r);
    _zn_zenoh_message_decode_na(rbf, &r);
    return r;
}

void _zn_zenoh_message_free(_zn_zenoh_message_t *msg)
{
    if (msg->attachment)
    {
        _zn_attachment_free(msg->attachment);
        free(msg->attachment);
    }
    if (msg->reply_context)
    {
        _zn_reply_context_free(msg->reply_context);
        free(msg->reply_context);
    }

    uint8_t mid = _ZN_MID(msg->header);
    switch (mid)
    {
    case _ZN_MID_DECLARE:
        _zn_declare_free(&msg->body.declare);
        break;
    case _ZN_MID_DATA:
        _zn_data_free(&msg->body.data);
        break;
    case _ZN_MID_PULL:
        _zn_pull_free(&msg->body.pull);
        break;
    case _ZN_MID_QUERY:
        _zn_query_free(&msg->body.query);
        break;
    case _ZN_MID_UNIT:
        // Do nothing. Unit messages have no body
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
        break;
    }
}

/*=============================*/
/*       Session Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
int _zn_scout_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_scout_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_SCOUT\n");

    // Encode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
        _ZN_EC(_z_zint_encode(wbf, msg->what))

    return 0;
}

void _zn_scout_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_scout_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_SCOUT\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
    {
        _z_zint_result_t r_zint = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
        r->value.scout.what = r_zint.value.zint;
    }
}

_zn_scout_result_t _zn_scout_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_scout_result_t r;
    _zn_scout_decode_na(rbf, header, &r);
    return r;
}

void _zn_scout_free(_zn_scout_t *msg)
{
    // NOTE: scout does not involve any heap allocation
    (void)(msg);
}

/*------------------ Hello Message ------------------*/
int _zn_hello_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_hello_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_HELLO\n");

    // Encode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        _ZN_EC(_z_bytes_encode(wbf, &msg->pid))

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
        _ZN_EC(_z_zint_encode(wbf, msg->whatami))

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
        _ZN_EC(_zn_locators_encode(wbf, &msg->locators))

    return 0;
}

void _zn_hello_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_hello_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_HELLO\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        _z_bytes_result_t r_arr = _z_bytes_decode(rbf);
        _ASSURE_P_RESULT(r_arr, r, _z_err_t_PARSE_BYTES)
        r->value.hello.pid = r_arr.value.bytes;
    }

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
    {
        _z_zint_result_t r_zint = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
        r->value.hello.whatami = r_zint.value.zint;
    }

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
    {
        _zn_locators_result_t r_locs = _zn_locators_decode(rbf);
        _ASSURE_P_RESULT(r_locs, r, _z_err_t_PARSE_BYTES)
        _z_str_array_move(&r->value.hello.locators, &r_locs.value.locators);
    }
}

_zn_hello_result_t _zn_hello_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_hello_result_t r;
    _zn_hello_decode_na(rbf, header, &r);
    return r;
}

void _zn_hello_free(_zn_hello_t *msg, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        (void)(&msg->pid);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
        _zn_locators_free(&msg->locators);
}

/*------------------ Open Message ------------------*/
int _zn_open_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_open_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_OPEN\n");

    // Encode the body
    _ZN_EC(_z_wbuf_write(wbf, msg->version))
    _ZN_EC(_z_zint_encode(wbf, msg->whatami))
    _ZN_EC(_z_bytes_encode(wbf, &msg->opid))
    _ZN_EC(_z_zint_encode(wbf, msg->lease))
    _ZN_EC(_z_zint_encode(wbf, msg->initial_sn))
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_S)
        _ZN_EC(_z_zint_encode(wbf, msg->sn_resolution))
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
        _ZN_EC(_zn_locators_encode(wbf, &msg->locators))

    return 0;
}

void _zn_open_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_open_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_OPEN\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    r->value.open.version = _z_rbuf_read(rbf);

    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.open.whatami = r_zint.value.zint;

    _z_bytes_result_t r_arr = _z_bytes_decode(rbf);
    _ASSURE_P_RESULT(r_arr, r, _z_err_t_PARSE_BYTES)
    r->value.open.opid = r_arr.value.bytes;

    r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.open.lease = r_zint.value.zint;

    r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.open.initial_sn = r_zint.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_S)
    {
        _z_zint_result_t r_zint = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
        r->value.open.sn_resolution = r_zint.value.zint;
    }

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
    {
        _zn_locators_result_t r_locs = _zn_locators_decode(rbf);
        _ASSURE_P_RESULT(r_locs, r, _z_err_t_PARSE_BYTES)
        _z_str_array_move(&r->value.open.locators, &r_locs.value.locators);
    }
}

_zn_open_result_t _zn_open_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_open_result_t r;
    _zn_open_decode_na(rbf, header, &r);
    return r;
}

void _zn_open_free(_zn_open_t *msg, uint8_t header)
{
    (void)(&msg->opid);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
        _zn_locators_free(&msg->locators);
}

/*------------------ Accept Message ------------------*/
int _zn_accept_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_accept_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ACCEPT\n");

    // Encode the body
    _ZN_EC(_z_zint_encode(wbf, msg->whatami))
    _ZN_EC(_z_bytes_encode(wbf, &msg->opid))
    _ZN_EC(_z_bytes_encode(wbf, &msg->apid))
    _ZN_EC(_z_zint_encode(wbf, msg->lease))
    _ZN_EC(_z_zint_encode(wbf, msg->initial_sn))
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_S)
        _ZN_EC(_z_zint_encode(wbf, msg->sn_resolution))
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
        _ZN_EC(_zn_locators_encode(wbf, &msg->locators))

    return 0;
}

void _zn_accept_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_accept_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ACCEPT\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_wami = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_wami, r, _z_err_t_PARSE_ZINT)
    r->value.accept.whatami = r_wami.value.zint;

    _z_bytes_result_t r_opid = _z_bytes_decode(rbf);
    _ASSURE_P_RESULT(r_opid, r, _z_err_t_PARSE_BYTES)
    r->value.accept.opid = r_opid.value.bytes;

    _z_bytes_result_t r_apid = _z_bytes_decode(rbf);
    _ASSURE_P_RESULT(r_apid, r, _z_err_t_PARSE_BYTES)
    r->value.accept.apid = r_apid.value.bytes;

    _z_zint_result_t r_lease = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_lease, r, _z_err_t_PARSE_ZINT)
    r->value.accept.lease = r_lease.value.zint;

    _z_zint_result_t r_insn = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_insn, r, _z_err_t_PARSE_ZINT)
    r->value.accept.initial_sn = r_insn.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_S)
    {
        _z_zint_result_t r_zint = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
        r->value.accept.sn_resolution = r_zint.value.zint;
    }

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
    {
        _zn_locators_result_t r_locs = _zn_locators_decode(rbf);
        _ASSURE_P_RESULT(r_locs, r, _z_err_t_PARSE_BYTES)
        _z_str_array_move(&r->value.accept.locators, &r_locs.value.locators);
    }
}

_zn_accept_result_t _zn_accept_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_accept_result_t r;
    _zn_accept_decode_na(rbf, header, &r);
    return r;
}

void _zn_accept_free(_zn_accept_t *msg, uint8_t header)
{
    (void)(&msg->opid);
    (void)(&msg->apid);

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
        _zn_locators_free(&msg->locators);
}

/*------------------ Close Message ------------------*/
int _zn_close_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_close_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_CLOSE\n");

    // Encode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        _ZN_EC(_z_bytes_encode(wbf, &msg->pid))

    return _z_wbuf_write(wbf, msg->reason);
}

void _zn_close_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_close_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_CLOSE\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        _z_bytes_result_t r_arr = _z_bytes_decode(rbf);
        _ASSURE_P_RESULT(r_arr, r, _z_err_t_PARSE_BYTES)
        r->value.close.pid = r_arr.value.bytes;
    }

    r->value.close.reason = _z_rbuf_read(rbf);
}

_zn_close_result_t _zn_close_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_close_result_t r;
    _zn_close_decode_na(rbf, header, &r);
    return r;
}

void _zn_close_free(_zn_close_t *msg, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        (void)(&msg->pid);
}

/*------------------ Sync Message ------------------*/
int _zn_sync_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_sync_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_SYNC\n");

    // Encode the body
    _ZN_EC(_z_zint_encode(wbf, msg->sn))

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_C)
        _ZN_EC(_z_zint_encode(wbf, msg->count))

    return 0;
}

void _zn_sync_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_sync_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_SYNC\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.sync.sn = r_zint.value.zint;

    if (_ZN_HAS_FLAG(header, _ZN_FLAG_S_R) && _ZN_HAS_FLAG(header, _ZN_FLAG_S_C))
    {
        _z_zint_result_t r_zint = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
        r->value.sync.count = r_zint.value.zint;
    }
}

_zn_sync_result_t _zn_sync_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_sync_result_t r;
    _zn_sync_decode_na(rbf, header, &r);
    return r;
}

void _zn_sync_free(_zn_sync_t *msg)
{
    // NOTE: sync does not involve any heap allocation
    (void)(msg);
}

/*------------------ AckNack Message ------------------*/
int _zn_ack_nack_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_ack_nack_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ACK_NACK\n");

    // Encode the body
    _ZN_EC(_z_zint_encode(wbf, msg->sn))

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_M)
        _ZN_EC(_z_zint_encode(wbf, msg->mask))

    return 0;
}

void _zn_ack_nack_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_ack_nack_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ACK_NACK\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.ack_nack.sn = r_zint.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_M)
    {
        _z_zint_result_t r_zint = _z_zint_decode(rbf);
        _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
        r->value.ack_nack.mask = r_zint.value.zint;
    }
}

_zn_ack_nack_result_t _zn_ack_nack_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_ack_nack_result_t r;
    _zn_ack_nack_decode_na(rbf, header, &r);
    return r;
}

void _zn_ack_nack_free(_zn_ack_nack_t *msg)
{
    // NOTE: ack_nack does not involve any heap allocation
    (void)(msg);
}

/*------------------ Keep Alive Message ------------------*/
int _zn_keep_alive_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_keep_alive_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_KEEP_ALIVE\n");

    // Encode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        _ZN_EC(_z_bytes_encode(wbf, &msg->pid))

    return 0;
}

void _zn_keep_alive_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_keep_alive_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_KEEP_ALIVE\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        _z_bytes_result_t r_arr = _z_bytes_decode(rbf);
        _ASSURE_P_RESULT(r_arr, r, _z_err_t_PARSE_BYTES)
        r->value.keep_alive.pid = r_arr.value.bytes;
    }
}

_zn_keep_alive_result_t _zn_keep_alive_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_keep_alive_result_t r;
    _zn_keep_alive_decode_na(rbf, header, &r);
    return r;
}

void _zn_keep_alive_free(_zn_keep_alive_t *msg, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
        (void)(&msg->pid);
}

/*------------------ PingPong Messages ------------------*/
int _zn_ping_pong_encode(_z_wbuf_t *wbf, const _zn_ping_pong_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_PING_PONG\n");

    // Encode the body
    return _z_zint_encode(wbf, msg->hash);
}

void _zn_ping_pong_decode_na(_z_rbuf_t *rbf, _zn_ping_pong_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_PING_PONG\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.ping_pong.hash = r_zint.value.zint;
}

_zn_ping_pong_result_t _zn_ping_pong_decode(_z_rbuf_t *rbf)
{
    _zn_ping_pong_result_t r;
    _zn_ping_pong_decode_na(rbf, &r);
    return r;
}

void _zn_ping_pong_free(_zn_ping_pong_t *msg)
{
    // NOTE: ping_pong does not involve any heap allocation
    (void)(msg);
}

/*------------------ Frame Message ------------------*/
int _zn_frame_encode(_z_wbuf_t *wbf, uint8_t header, const _zn_frame_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_FRAME\n");

    // Encode the body
    _ZN_EC(_z_zint_encode(wbf, msg->sn))

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_F)
    {
        // Do not write the fragment as z_bytes_t since the total frame length
        // is eventually prepended to the frame. There is no need to encode the fragment length.
        return _z_wbuf_write_bytes(wbf, msg->payload.fragment.val, 0, msg->payload.fragment.len);
    }
    else
    {
        size_t len = _z_vec_len(&msg->payload.messages);
        for (size_t i = 0; i < len; ++i)
            _ZN_EC(_zn_zenoh_message_encode(wbf, _z_vec_get(&msg->payload.messages, i)))

        return 0;
    }
}

void _zn_frame_decode_na(_z_rbuf_t *rbf, uint8_t header, _zn_frame_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_FRAME\n");
    r->tag = _z_res_t_OK;

    // Decode the body
    _z_zint_result_t r_zint = _z_zint_decode(rbf);
    _ASSURE_P_RESULT(r_zint, r, _z_err_t_PARSE_ZINT)
    r->value.frame.sn = r_zint.value.zint;

    // Decode the payload
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_F)
    {
        // Read all the remaining bytes in the buffer as the fragment
        r->value.frame.payload.fragment.len = _z_rbuf_len(rbf);
        r->value.frame.payload.fragment.val = _z_rbuf_get_rptr(rbf);

        // We need to manually move the r_pos to w_pos, we have read it all
        _z_rbuf_set_rpos(rbf, _z_rbuf_get_wpos(rbf));
    }
    else
    {
        r->value.frame.payload.messages = _z_vec_make(_ZENOH_PICO_FRAME_MESSAGES_VEC_SIZE);
        while (_z_rbuf_len(rbf))
        {
            // Mark the reading position of the iobfer
            size_t r_pos = _z_rbuf_get_rpos(rbf);
            _zn_zenoh_message_p_result_t r_zm = _zn_zenoh_message_decode(rbf);
            if (r_zm.tag == _z_res_t_OK)
            {
                _z_vec_append(&r->value.frame.payload.messages, r_zm.value.zenoh_message);
            }
            else
            {
                // Restore the reading position of the iobfer
                _z_rbuf_set_rpos(rbf, r_pos);
                return;
            }
        }
    }
}

_zn_frame_result_t _zn_frame_decode(_z_rbuf_t *rbf, uint8_t header)
{
    _zn_frame_result_t r;
    _zn_frame_decode_na(rbf, header, &r);
    return r;
}

void _zn_frame_free(_zn_frame_t *msg, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_F)
    {
        _zn_payload_free(&msg->payload.fragment);
    }
    else
    {
        for (size_t i = 0; i < _z_vec_len(&msg->payload.messages); ++i)
            _zn_zenoh_message_free((_zn_zenoh_message_t *)_z_vec_get(&msg->payload.messages, i));
        _z_vec_free(&msg->payload.messages);
    }
}

/*------------------ Session Message ------------------*/
int _zn_session_message_encode(_z_wbuf_t *wbf, const _zn_session_message_t *msg)
{
    // Encode the decorators if present
    if (msg->attachment)
        _ZN_EC(_zn_attachment_encode(wbf, msg->attachment))

    // Encode the header
    _ZN_EC(_z_wbuf_write(wbf, msg->header))

    // Encode the body
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_FRAME:
        return _zn_frame_encode(wbf, msg->header, &msg->body.frame);
    case _ZN_MID_SCOUT:
        return _zn_scout_encode(wbf, msg->header, &msg->body.scout);
    case _ZN_MID_HELLO:
        return _zn_hello_encode(wbf, msg->header, &msg->body.hello);
    case _ZN_MID_OPEN:
        return _zn_open_encode(wbf, msg->header, &msg->body.open);
    case _ZN_MID_ACCEPT:
        return _zn_accept_encode(wbf, msg->header, &msg->body.accept);
    case _ZN_MID_CLOSE:
        return _zn_close_encode(wbf, msg->header, &msg->body.close);
    case _ZN_MID_SYNC:
        return _zn_sync_encode(wbf, msg->header, &msg->body.sync);
    case _ZN_MID_ACK_NACK:
        return _zn_ack_nack_encode(wbf, msg->header, &msg->body.ack_nack);
    case _ZN_MID_KEEP_ALIVE:
        return _zn_keep_alive_encode(wbf, msg->header, &msg->body.keep_alive);
    case _ZN_MID_PING_PONG:
        return _zn_ping_pong_encode(wbf, &msg->body.ping_pong);
    default:
        _Z_ERROR("WARNING: Trying to encode session message with unknown ID(%d)\n", _ZN_MID(msg->header));
        return -1;
    }
}

void _zn_session_message_decode_na(_z_rbuf_t *rbf, _zn_session_message_p_result_t *r)
{
    r->tag = _z_res_t_OK;

    r->value.session_message->attachment = NULL;
    do
    {
        // Decode the header
        r->value.session_message->header = _z_rbuf_read(rbf);

        // Decode the body
        uint8_t mid = _ZN_MID(r->value.session_message->header);
        switch (mid)
        {
        case _ZN_MID_FRAME:
        {
            _zn_frame_result_t r_fr = _zn_frame_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_fr, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.frame = r_fr.value.frame;
            return;
        }
        case _ZN_MID_ATTACHMENT:
        {
            _zn_attachment_p_result_t r_at = _zn_attachment_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_at, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->attachment = r_at.value.attachment;
            break;
        }
        case _ZN_MID_SCOUT:
        {
            _zn_scout_result_t r_sc = _zn_scout_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_sc, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.scout = r_sc.value.scout;
            return;
        }
        case _ZN_MID_HELLO:
        {
            _zn_hello_result_t r_he = _zn_hello_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_he, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.hello = r_he.value.hello;
            return;
        }
        case _ZN_MID_OPEN:
        {
            _zn_open_result_t r_op = _zn_open_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_op, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.open = r_op.value.open;
            return;
        }
        case _ZN_MID_ACCEPT:
        {
            _zn_accept_result_t r_ac = _zn_accept_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_ac, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.accept = r_ac.value.accept;
            return;
        }
        case _ZN_MID_CLOSE:
        {
            _zn_close_result_t r_cl = _zn_close_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_cl, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.close = r_cl.value.close;
            return;
        }
        case _ZN_MID_SYNC:
        {
            _zn_sync_result_t r_sy = _zn_sync_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_sy, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.sync = r_sy.value.sync;
            return;
        }
        case _ZN_MID_ACK_NACK:
        {
            _zn_ack_nack_result_t r_an = _zn_ack_nack_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_an, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.ack_nack = r_an.value.ack_nack;
            return;
        }
        case _ZN_MID_KEEP_ALIVE:
        {
            _zn_keep_alive_result_t r_ka = _zn_keep_alive_decode(rbf, r->value.session_message->header);
            _ASSURE_P_RESULT(r_ka, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.keep_alive = r_ka.value.keep_alive;
            return;
        }
        case _ZN_MID_PING_PONG:
        {
            _zn_ping_pong_result_t r_pp = _zn_ping_pong_decode(rbf);
            _ASSURE_P_RESULT(r_pp, r, _zn_err_t_PARSE_SESSION_MESSAGE)
            r->value.session_message->body.ping_pong = r_pp.value.ping_pong;
            return;
        }
        default:
        {
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_PARSE_SESSION_MESSAGE;
            _Z_ERROR("WARNING: Trying to decode session message with unknown ID(%d)\n", mid);
            return;
        }
        }
    } while (1);
}

_zn_session_message_p_result_t _zn_session_message_decode(_z_rbuf_t *rbf)
{
    _zn_session_message_p_result_t r;
    _zn_session_message_p_result_init(&r);
    _zn_session_message_decode_na(rbf, &r);
    return r;
}

void _zn_session_message_free(_zn_session_message_t *msg)
{
    if (msg->attachment)
    {
        _zn_attachment_free(msg->attachment);
        free(msg->attachment);
    }

    uint8_t mid = _ZN_MID(msg->header);
    switch (mid)
    {
    case _ZN_MID_SCOUT:
        _zn_scout_free(&msg->body.scout);
        break;
    case _ZN_MID_HELLO:
        _zn_hello_free(&msg->body.hello, msg->header);
        break;
    case _ZN_MID_OPEN:
        _zn_open_free(&msg->body.open, msg->header);
        break;
    case _ZN_MID_ACCEPT:
        _zn_accept_free(&msg->body.accept, msg->header);
        break;
    case _ZN_MID_CLOSE:
        _zn_close_free(&msg->body.close, msg->header);
        break;
    case _ZN_MID_SYNC:
        _zn_sync_free(&msg->body.sync);
        break;
    case _ZN_MID_ACK_NACK:
        _zn_ack_nack_free(&msg->body.ack_nack);
        break;
    case _ZN_MID_KEEP_ALIVE:
        _zn_keep_alive_free(&msg->body.keep_alive, msg->header);
        break;
    case _ZN_MID_PING_PONG:
        _zn_ping_pong_free(&msg->body.ping_pong);
        break;
    case _ZN_MID_FRAME:
        _zn_frame_free(&msg->body.frame, msg->header);
        return;
    default:
        _Z_ERROR("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
        return;
    }
}
