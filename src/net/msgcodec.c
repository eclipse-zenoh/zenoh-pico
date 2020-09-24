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

#include <stdio.h>
#include "zenoh/private/logging.h"
#include "zenoh/net/private/msgcodec.h"

/*=============================*/
/*           Fields            */
/*=============================*/
/*------------------ Payload field ------------------*/
void _zn_payload_encode(z_iobuf_t *buf, const z_iobuf_t *bs)
{
    z_iobuf_write_slice(buf, bs->buf, bs->r_pos, bs->w_pos);
}

z_iobuf_t _zn_payload_decode(z_iobuf_t *buf)
{
    z_zint_t len = z_iobuf_readable(buf);
    uint8_t *bs = z_iobuf_read_n(buf, len);
    z_iobuf_t iob = z_iobuf_wrap_wo(bs, len, 0, len);
    return iob;
}

/*------------------ Timestamp Field ------------------*/
void _zn_timestamp_encode(z_iobuf_t *buf, const z_timestamp_t *ts)
{
    _Z_DEBUG("Encoding _TIMESTAMP\n");

    z_zint_encode(buf, ts->time);
    z_uint8_array_encode(buf, &ts->id);
}

void _zn_timestamp_decode_na(z_iobuf_t *buf, _zn_timestamp_result_t *r)
{
    _Z_DEBUG("Decoding _TIMESTAMP\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.timestamp.time = r_zint.value.zint;

    z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
    r->value.timestamp.id = r_arr.value.uint8_array;
}

_zn_timestamp_result_t
_zn_timestamp_decode(z_iobuf_t *buf)
{
    _zn_timestamp_result_t r;
    _zn_timestamp_decode_na(buf, &r);
    return r;
}

/*------------------ SubMode Field ------------------*/
void _zn_sub_mode_encode(z_iobuf_t *buf, const zn_sub_mode_t *fld)
{
    _Z_DEBUG("Encoding _SUB_MODE\n");

    // Encode the header
    uint8_t hdr = fld->kind;
    if (fld->period)
        hdr |= _ZN_FLAG_Z_P;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Write the body
    if (fld->period)
        zn_temporal_property_encode(buf, fld->period);
}

void _zn_sub_mode_decode_na(z_iobuf_t *buf, _zn_sub_mode_result_t *r)
{
    _Z_DEBUG("Decoding _SUB_MODE\n");
    r->tag = Z_OK_TAG;

    // Decode the header
    uint8_t hdr = z_iobuf_read(buf);
    uint8_t id = _ZN_MID(hdr);
    uint8_t flags = _ZN_FLAGS(hdr);

    // Decode the body
    r->value.sub_mode.kind = id;
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_Z_P)
    {
        zn_temporal_property_result_t r_tp = zn_temporal_property_decode(buf);
        ASSURE_P_RESULT(r_tp, r, ZN_SUB_MODE_PARSE_ERROR)
        r->value.sub_mode.period = &r_tp.value.temporal_property;
    }
}

_zn_sub_mode_result_t
_zn_sub_mode_decode(z_iobuf_t *buf)
{
    _zn_sub_mode_result_t r;
    _zn_sub_mode_decode_na(buf, &r);
    return r;
}

/*------------------ ResKey Field ------------------*/
void _zn_res_key_encode(z_iobuf_t *buf, const zn_res_key_t *fld)
{
    _Z_DEBUG("Encoding _RES_KEY\n");

    z_zint_encode(buf, fld->rid);
    if (fld->rname)
        z_string_encode(buf, fld->rname);
}

void _zn_res_key_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_res_key_result_t *r)
{
    _Z_DEBUG("Decoding _RES_KEY\n");
    r->tag = Z_OK_TAG;

    // Decode the header
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.res_key.rid = r_zint.value.zint;

    if (!_ZN_HAS_FLAG(flags, _ZN_FLAG_Z_K))
    {
        z_string_result_t r_str = z_string_decode(buf);
        ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
        r->value.res_key.rname = r_str.value.string;
    }
}

_zn_res_key_result_t
_zn_res_key_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_res_key_result_t r;
    _zn_res_key_decode_na(flags, buf, &r);
    return r;
}

/*=============================*/
/*     Message decorators      */
/*=============================*/
/*------------------ Attachement Decorator ------------------*/
void _zn_attachment_encode(z_iobuf_t *buf, const _zn_attachment_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ATTACHMENT\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_ATTACHMENT | _ZN_FLAGS(msg->encoding);

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_payload_encode(buf, &msg->buffer);
}

void _zn_attachment_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_attachment_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ATTACHMENT\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.attachment.encoding = flags;

    // Decode the body
    r->value.attachment.buffer = _zn_payload_decode(buf);
}

_zn_attachment_result_t
_zn_attachment_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_attachment_result_t r;
    _zn_attachment_decode_na(flags, buf, &r);
    return r;
}

/*------------------ ReplyContext Decorator ------------------*/
void _zn_reply_context_encode(z_iobuf_t *buf, const _zn_reply_context_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_REPLY_CONTEXT\n");

    // Encode the header
    uint8_t header = _ZN_MID_REPLY_CONTEXT;
    if (msg->is_final)
        header |= _ZN_FLAG_Z_F;

    // Write the header
    z_iobuf_write(buf, header);

    // Encode the body
    z_zint_encode(buf, msg->qid);
    z_zint_encode(buf, msg->source_kind);
    if (!msg->is_final)
        z_uint8_array_encode(buf, msg->replier_id);
}

void _zn_reply_context_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_reply_context_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_REPLY_CONTEXT\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.reply_context.qid = r_zint.value.zint;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.reply_context.source_kind = r_zint.value.zint;

    if _ZN_HAS_FLAG (flags, _ZN_FLAG_Z_F)
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.reply_context.replier_id = &r_arr.value.uint8_array;
    }
}

_zn_reply_context_result_t
_zn_reply_context_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_reply_context_result_t r;
    _zn_reply_context_decode_na(flags, buf, &r);
    return r;
}

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/
/*------------------ Resource Declaration ------------------*/
void _zn_res_decl_encode(z_iobuf_t *buf, const _zn_res_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_RESOURCE\n");

    // Encode the header
    uint8_t hdr = _ZN_DECL_RESOURCE;
    if (!dcl->key.rname)
        hdr |= _ZN_FLAG_Z_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    z_zint_encode(buf, dcl->rid);
    _zn_res_key_encode(buf, &dcl->key);
}

void _zn_res_decl_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_res_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_RESOURCE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.res_decl.rid = r_zint.value.zint;

    _zn_res_key_result_t r_res = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.res_decl.key = r_res.value.res_key;
}

_zn_res_decl_result_t
_zn_res_decl_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_res_decl_result_t r;
    _zn_res_decl_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Publisher Declaration ------------------*/
void _zn_pub_decl_encode(z_iobuf_t *buf, const _zn_pub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_PUBLISHER\n");

    // Encode the header
    uint8_t hdr = _ZN_DECL_PUBLISHER;
    if (!dcl->key.rname)
        hdr |= _ZN_FLAG_Z_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &dcl->key);
}

void _zn_pub_decl_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_pub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_PUBLISHER\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    _zn_res_key_result_t r_res = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.pub_decl.key = r_res.value.res_key;
}

_zn_pub_decl_result_t
_zn_pub_decl_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_pub_decl_result_t r;
    _zn_pub_decl_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Subscriber Declaration ------------------*/
void _zn_sub_decl_encode(z_iobuf_t *buf, const _zn_sub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_SUBSCRIBER\n");

    // Encode the header
    uint8_t hdr = _ZN_DECL_SUBSCRIBER;
    if (dcl->is_reliable)
        hdr |= _ZN_FLAG_Z_R;
    if (dcl->sub_mode)
        hdr |= _ZN_FLAG_Z_S;
    if (!dcl->key.rname)
        hdr |= _ZN_FLAG_Z_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &dcl->key);
    if (dcl->sub_mode)
        _zn_sub_mode_encode(buf, dcl->sub_mode);
}

void _zn_sub_decl_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_sub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_SUBSCRIBER\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.sub_decl.is_reliable = _ZN_HAS_FLAG(flags, _ZN_FLAG_Z_R);

    // Decode the body
    _zn_res_key_result_t r_res = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.sub_decl.key = r_res.value.res_key;

    if _ZN_HAS_FLAG (flags, _ZN_FLAG_Z_S)
    {
        _zn_sub_mode_result_t r_smd = _zn_sub_mode_decode(buf);
        ASSURE_P_RESULT(r_smd, r, ZN_SUB_MODE_PARSE_ERROR)
        r->value.sub_decl.sub_mode = &r_smd.value.sub_mode;
    }
}

_zn_sub_decl_result_t
_zn_sub_decl_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_sub_decl_result_t r;
    _zn_sub_decl_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Queryable Declaration ------------------*/
void _zn_qle_decl_encode(z_iobuf_t *buf, const _zn_qle_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_QUERYABLE\n");

    // Encode the header
    uint8_t hdr = _ZN_DECL_QUERYABLE;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &dcl->key);
}

void _zn_qle_decl_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_qle_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_QUERYABLE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    _zn_res_key_result_t r_res = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.qle_decl.key = r_res.value.res_key;
}

_zn_qle_decl_result_t
_zn_qle_decl_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_qle_decl_result_t r;
    _zn_qle_decl_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Forget Resource Declaration ------------------*/
void _zn_forget_res_decl_encode(z_iobuf_t *buf, const _zn_forget_res_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_RESOURCE\n");

    // Encode the header
    uint8_t hdr = _ZN_DECL_FORGET_RESOURCE;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    z_zint_encode(buf, dcl->rid);
}

void _zn_forget_res_decl_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_forget_res_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_RESOURCE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.forget_res_decl.rid = r_zint.value.zint;
}

_zn_forget_res_decl_result_t
_zn_forget_res_decl_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_forget_res_decl_result_t r;
    _zn_forget_res_decl_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Forget Publisher Declaration ------------------*/
void _zn_forget_pub_decl_encode(z_iobuf_t *buf, const _zn_forget_pub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_PUBLISHER\n");

    // Encode the header
    uint8_t hdr = _ZN_DECL_FORGET_PUBLISHER;
    if (!dcl->key.rname)
        hdr |= _ZN_FLAG_Z_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &dcl->key);
}

void _zn_forget_pub_decl_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_forget_pub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_PUBLISHER\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    _zn_res_key_result_t r_res = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.forget_pub_decl.key = r_res.value.res_key;
}

_zn_forget_pub_decl_result_t
_zn_forget_pub_decl_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_forget_pub_decl_result_t r;
    _zn_forget_pub_decl_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Forget Subscriber Declaration ------------------*/
void _zn_forget_sub_decl_encode(z_iobuf_t *buf, const _zn_forget_sub_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_PUBLISHER\n");

    // Encode the header
    uint8_t hdr = _ZN_DECL_FORGET_SUBSCRIBER;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &dcl->key);
}

void _zn_forget_sub_decl_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_forget_sub_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_PUBLISHER\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    _zn_res_key_result_t r_res = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.forget_sub_decl.key = r_res.value.res_key;
}

_zn_forget_sub_decl_result_t
_zn_forget_sub_decl_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_forget_sub_decl_result_t r;
    _zn_forget_sub_decl_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Forget Queryable Declaration ------------------*/
void _zn_forget_qle_decl_encode(z_iobuf_t *buf, const _zn_forget_qle_decl_t *dcl)
{
    _Z_DEBUG("Encoding _ZN_DECL_FORGET_QUERYABLE\n");

    // Encode the header
    uint8_t hdr = _ZN_DECL_FORGET_QUERYABLE;
    if (!dcl->key.rname)
        hdr |= _ZN_FLAG_Z_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &dcl->key);
}

void _zn_forget_qle_decl_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_forget_qle_decl_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DECL_FORGET_QUERYABLE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    _zn_res_key_result_t r_res = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_res, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.forget_qle_decl.key = r_res.value.res_key;
}

_zn_forget_qle_decl_result_t
_zn_forget_qle_decl_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_forget_qle_decl_result_t r;
    _zn_forget_qle_decl_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Declaration Field ------------------*/
void _zn_declaration_encode(z_iobuf_t *buf, _zn_declaration_t *dcl)
{
    switch (dcl->id)
    {
    case _ZN_DECL_RESOURCE:
        _zn_res_decl_encode(buf, &dcl->body.res);
        break;
    case _ZN_DECL_PUBLISHER:
        _zn_pub_decl_encode(buf, &dcl->body.pub);
        break;
    case _ZN_DECL_SUBSCRIBER:
        _zn_sub_decl_encode(buf, &dcl->body.sub);
        break;
    case _ZN_DECL_QUERYABLE:
        _zn_qle_decl_encode(buf, &dcl->body.qle);
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        _zn_forget_res_decl_encode(buf, &dcl->body.forget_res);
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        _zn_forget_pub_decl_encode(buf, &dcl->body.forget_pub);
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        _zn_forget_sub_decl_encode(buf, &dcl->body.forget_sub);
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        _zn_forget_qle_decl_encode(buf, &dcl->body.forget_qle);
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode declaration with unknown ID(%d)\n", dcl->id);
        break;
    }
}

void _zn_declaration_decode_na(z_iobuf_t *buf, _zn_declaration_result_t *r)
{
    r->tag = Z_OK_TAG;

    // Decode the header
    uint8_t hdr = z_iobuf_read(buf);
    uint8_t mid = _ZN_MID(hdr);
    uint8_t flags = _ZN_FLAGS(hdr);

    r->value.declaration.id = mid;

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
        r_rdcl = _zn_res_decl_decode(flags, buf);
        ASSURE_P_RESULT(r_rdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.res = r_rdcl.value.res_decl;
        break;
    case _ZN_DECL_PUBLISHER:
        r_pdcl = _zn_pub_decl_decode(flags, buf);
        ASSURE_P_RESULT(r_pdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.pub = r_pdcl.value.pub_decl;
        break;
    case _ZN_DECL_SUBSCRIBER:
        r_sdcl = _zn_sub_decl_decode(flags, buf);
        ASSURE_P_RESULT(r_sdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.sub = r_sdcl.value.sub_decl;
        break;
    case _ZN_DECL_QUERYABLE:
        r_qdcl = _zn_qle_decl_decode(flags, buf);
        ASSURE_P_RESULT(r_qdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.qle = r_qdcl.value.qle_decl;
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        r_frdcl = _zn_forget_res_decl_decode(flags, buf);
        ASSURE_P_RESULT(r_frdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.forget_res = r_frdcl.value.forget_res_decl;
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        r_fpdcl = _zn_forget_pub_decl_decode(flags, buf);
        ASSURE_P_RESULT(r_fpdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.forget_pub = r_fpdcl.value.forget_pub_decl;
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        r_fsdcl = _zn_forget_sub_decl_decode(flags, buf);
        ASSURE_P_RESULT(r_fsdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.forget_sub = r_fsdcl.value.forget_sub_decl;
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        r_fqdcl = _zn_forget_qle_decl_decode(flags, buf);
        ASSURE_P_RESULT(r_fqdcl, r, ZN_DECLARATION_PARSE_ERROR)
        r->value.declaration.body.forget_qle = r_fqdcl.value.forget_qle_decl;
        break;
    default:
        _Z_ERROR("WARNING: Trying to decode declaration with unknown ID(%d)\n", mid);
        break;
    }
}

_zn_declaration_result_t
_zn_declaration_decode(z_iobuf_t *buf)
{
    _zn_declaration_result_t r;
    _zn_declaration_decode_na(buf, &r);
    return r;
}

/*------------------ Declaration Message ------------------*/
void _zn_declare_encode(z_iobuf_t *buf, const _zn_declare_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_DECLARE\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_DECLARE;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    unsigned int len = msg->declarations.length;
    z_zint_encode(buf, len);
    for (unsigned int i = 0; i < len; ++i)
        _zn_declaration_encode(buf, &msg->declarations.elem[i]);
}

void _zn_declare_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_declare_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_DECLARE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_dlen = z_zint_decode(buf);
    ASSURE_P_RESULT(r_dlen, r, Z_ZINT_PARSE_ERROR)
    size_t len = r_dlen.value.zint;
    r->value.declare.declarations.length = len;
    r->value.declare.declarations.elem = (_zn_declaration_t *)malloc(sizeof(_zn_declaration_t) * len);

    _zn_declaration_result_t *r_decl = (_zn_declaration_result_t *)malloc(sizeof(_zn_declaration_result_t));
    for (size_t i = 0; i < len; ++i)
    {
        _zn_declaration_decode_na(buf, r_decl);
        if (r_decl->tag == Z_OK_TAG)
        {
            r->value.declare.declarations.elem[i] = r_decl->value.declaration;
        }
        else
        {
            r->value.declare.declarations.length = 0;
            free(r->value.declare.declarations.elem);
            free(r_decl);
            r->tag = Z_ERROR_TAG;
            r->value.error = ZN_ZENOH_MESSAGE_PARSE_ERROR;
            return;
        }
    }
    free(r_decl);
}

_zn_declare_result_t
_zn_declare_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_declare_result_t r;
    _zn_declare_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Data Info Field ------------------*/
void _zn_data_info_encode(z_iobuf_t *buf, const zn_data_info_t *fld)
{
    _Z_DEBUG("Encoding _ZN_DATA_INFO\n");

    // Encode the flags
    z_zint_t flags = 0;
    if (fld->source_id)
        flags |= _ZN_DATA_INFO_SRC_ID;
    if (fld->source_sn)
        flags |= _ZN_DATA_INFO_SRC_SN;
    if (fld->first_router_id)
        flags |= _ZN_DATA_INFO_RTR_ID;
    if (fld->first_router_sn)
        flags |= _ZN_DATA_INFO_RTR_SN;
    if (fld->tstamp)
        flags |= _ZN_DATA_INFO_TSTAMP;
    if (fld->kind)
        flags |= _ZN_DATA_INFO_KIND;
    if (fld->encoding)
        flags |= _ZN_DATA_INFO_ENC;

    // Write the flags
    z_zint_encode(buf, flags);

    // Encode the body
    if (fld->source_id)
        z_uint8_array_encode(buf, fld->source_id);
    if (fld->source_sn)
        z_zint_encode(buf, *fld->source_sn);
    if (fld->first_router_id)
        z_uint8_array_encode(buf, fld->first_router_id);
    if (fld->first_router_sn)
        z_zint_encode(buf, *fld->first_router_sn);
    if (fld->tstamp)
        _zn_timestamp_encode(buf, fld->tstamp);
    if (fld->kind)
        z_zint_encode(buf, *fld->kind);
    if (fld->encoding)
        z_zint_encode(buf, *fld->encoding);
}

void _zn_data_info_decode_na(z_iobuf_t *buf, _zn_data_info_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_DATA_INFO\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    z_zint_result_t r_flags = z_zint_decode(buf);
    ASSURE_P_RESULT(r_flags, r, Z_ZINT_PARSE_ERROR)
    z_zint_t flags = r_flags.value.zint;

    if _ZN_HAS_FLAG (flags, _ZN_DATA_INFO_SRC_ID)
    {
        z_uint8_array_result_t r_sid = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_sid, r, Z_ARRAY_PARSE_ERROR)
        r->value.data_info.source_id = &r_sid.value.uint8_array;
    }
    if _ZN_HAS_FLAG (flags, _ZN_DATA_INFO_SRC_SN)
    {
        z_zint_result_t r_ssn = z_zint_decode(buf);
        ASSURE_P_RESULT(r_ssn, r, Z_ZINT_PARSE_ERROR)
        r->value.data_info.source_sn = &r_ssn.value.zint;
    }
    if _ZN_HAS_FLAG (flags, _ZN_DATA_INFO_RTR_ID)
    {
        z_uint8_array_result_t r_rid = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_rid, r, Z_ARRAY_PARSE_ERROR)
        r->value.data_info.first_router_id = &r_rid.value.uint8_array;
    }
    if _ZN_HAS_FLAG (flags, _ZN_DATA_INFO_RTR_SN)
    {
        z_zint_result_t r_rsn = z_zint_decode(buf);
        ASSURE_P_RESULT(r_rsn, r, Z_ZINT_PARSE_ERROR)
        r->value.data_info.first_router_sn = &r_rsn.value.zint;
    }
    if _ZN_HAS_FLAG (flags, _ZN_DATA_INFO_TSTAMP)
    {
        _zn_timestamp_result_t r_tsp = _zn_timestamp_decode(buf);
        ASSURE_P_RESULT(r_tsp, r, ZN_TIMESTAMP_PARSE_ERROR)
        r->value.data_info.tstamp = &r_tsp.value.timestamp;
    }
    if _ZN_HAS_FLAG (flags, _ZN_DATA_INFO_KIND)
    {
        z_zint_result_t r_knd = z_zint_decode(buf);
        ASSURE_P_RESULT(r_knd, r, Z_ZINT_PARSE_ERROR)
        r->value.data_info.kind = &r_knd.value.zint;
    }
    if _ZN_HAS_FLAG (flags, _ZN_DATA_INFO_ENC)
    {
        z_zint_result_t r_enc = z_zint_decode(buf);
        ASSURE_P_RESULT(r_enc, r, Z_ZINT_PARSE_ERROR)
        r->value.data_info.encoding = &r_enc.value.zint;
    }
}

_zn_data_info_result_t
_zn_data_info_decode(z_iobuf_t *buf)
{
    _zn_data_info_result_t r;
    _zn_data_info_decode_na(buf, &r);
    return r;
}

/*------------------ Data Message ------------------*/
void _zn_data_encode(z_iobuf_t *buf, const _zn_data_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_DATA\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_DATA;
    if (msg->is_droppable)
        hdr |= _ZN_FLAG_Z_D;
    if (msg->info)
        hdr |= _ZN_FLAG_Z_I;
    if (!msg->key.rname)
        hdr |= _ZN_FLAG_Z_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &msg->key);
    if (msg->info)
        _zn_data_info_encode(buf, msg->info);
    _zn_payload_encode(buf, &msg->payload);
}

void _zn_data_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_data_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_DATA\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.data.is_droppable = _ZN_HAS_FLAG(flags, _ZN_FLAG_Z_D);

    // Decode the body
    _zn_res_key_result_t r_key = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_key, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.data.key = r_key.value.res_key;

    if _ZN_HAS_FLAG (flags, _ZN_FLAG_Z_I)
    {
        _zn_data_info_result_t r_dti = _zn_data_info_decode(buf);
        ASSURE_P_RESULT(r_dti, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
        r->value.data.info = &r_dti.value.data_info;
    }

    r->value.data.payload = _zn_payload_decode(buf);
}

_zn_data_result_t
_zn_data_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_data_result_t r;
    _zn_data_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Unit Message ------------------*/
void _zn_unit_encode(z_iobuf_t *buf, const _zn_unit_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_UNIT\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_UNIT;
    if (msg->is_droppable)
        hdr |= _ZN_FLAG_Z_D;

    // Write the header
    z_iobuf_write(buf, hdr);
}

void _zn_unit_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_unit_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_UNIT\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.unit.is_droppable = _ZN_HAS_FLAG(flags, _ZN_FLAG_Z_D);
}

_zn_unit_result_t
_zn_unit_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_unit_result_t r;
    _zn_unit_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Pull Message ------------------*/
void _zn_pull_encode(z_iobuf_t *buf, const _zn_pull_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_PULL\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_PULL;
    if (msg->is_final)
        hdr |= _ZN_FLAG_Z_F;
    if (msg->max_samples)
        hdr |= _ZN_FLAG_Z_N;
    if (!msg->key.rname)
        hdr |= _ZN_FLAG_Z_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &msg->key);
    z_zint_encode(buf, msg->pull_id);
    if (msg->max_samples)
        z_zint_encode(buf, *msg->max_samples);
}

void _zn_pull_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_pull_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_PULL\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.pull.is_final = _ZN_HAS_FLAG(flags, _ZN_FLAG_Z_F);

    // Decode the body
    _zn_res_key_result_t r_key = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_key, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.pull.key = r_key.value.res_key;

    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.pull.pull_id = r_zint.value.zint;

    if _ZN_HAS_FLAG (flags, _ZN_FLAG_Z_N)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.pull.max_samples = &r_zint.value.zint;
    }
}

_zn_pull_result_t
_zn_pull_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_pull_result_t r;
    _zn_pull_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Query Message ------------------*/
void _zn_query_encode(z_iobuf_t *buf, const _zn_query_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_QUERY\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_QUERY;
    if (msg->target)
        hdr |= _ZN_FLAG_Z_T;
    if (!msg->key.rname)
        hdr |= _ZN_FLAG_Z_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    _zn_res_key_encode(buf, &msg->key);
    z_string_encode(buf, msg->predicate);
    z_zint_encode(buf, msg->qid);
    if (msg->target)
        z_zint_encode(buf, *msg->target);
    z_zint_encode(buf, msg->consolidation);
}

void _zn_query_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_query_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_QUERY\n");
    r->tag = Z_OK_TAG;

    _zn_res_key_result_t r_key = _zn_res_key_decode(flags, buf);
    ASSURE_P_RESULT(r_key, r, ZN_RES_KEY_PARSE_ERROR)
    r->value.query.key = r_key.value.res_key;

    z_string_result_t r_str = z_string_decode(buf);
    ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
    r->value.query.predicate = r_str.value.string;

    z_zint_result_t r_qid = z_zint_decode(buf);
    ASSURE_P_RESULT(r_qid, r, Z_ZINT_PARSE_ERROR)
    r->value.query.qid = r_qid.value.zint;

    if _ZN_HAS_FLAG (flags, _ZN_FLAG_Z_T)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.query.target = &r_zint.value.zint;
    }

    z_zint_result_t r_con = z_zint_decode(buf);
    ASSURE_P_RESULT(r_con, r, Z_ZINT_PARSE_ERROR)
    r->value.query.consolidation = r_con.value.zint;
}

_zn_query_result_t
_zn_query_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_query_result_t r;
    _zn_query_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Zenoh Message ------------------*/
void _zn_zenoh_message_encode(z_iobuf_t *buf, const _zn_zenoh_message_t *msg)
{
    // Encode the decorators if present
    if (msg->attachment)
        _zn_attachment_encode(buf, msg->attachment);
    if (msg->reply_context)
        _zn_reply_context_encode(buf, msg->reply_context);

    // Encode the message
    switch (msg->id)
    {
    case _ZN_MID_DECLARE:
        _zn_declare_encode(buf, &msg->body.declare);
        break;
    case _ZN_MID_DATA:
        _zn_data_encode(buf, &msg->body.data);
        break;
    case _ZN_MID_QUERY:
        _zn_query_encode(buf, &msg->body.query);
        break;
    case _ZN_MID_PULL:
        _zn_pull_encode(buf, &msg->body.pull);
        break;
    case _ZN_MID_UNIT:
        _zn_unit_encode(buf, &msg->body.unit);
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", msg->id);
        break;
    }
}

void _zn_zenoh_message_decode_na(z_iobuf_t *buf, _zn_zenoh_message_p_result_t *r)
{
    r->tag = Z_OK_TAG;

    uint8_t hdr;
    uint8_t mid;
    uint8_t flags;

    _zn_attachment_result_t r_at;
    _zn_reply_context_result_t r_rc;
    _zn_declare_result_t r_de;
    _zn_data_result_t r_da;
    _zn_query_result_t r_qu;
    _zn_pull_result_t r_pu;
    _zn_unit_result_t r_un;

    do
    {
        hdr = z_iobuf_read(buf);
        mid = _ZN_MID(hdr);
        flags = _ZN_FLAGS(hdr);

        r->value.zenoh_message->id = mid;

        switch (mid)
        {
        case _ZN_MID_ATTACHMENT:
            r_at = _zn_attachment_decode(flags, buf);
            ASSURE_P_RESULT(r_at, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->attachment = &r_at.value.attachment;
            break;
        case _ZN_MID_REPLY_CONTEXT:
            r_rc = _zn_reply_context_decode(flags, buf);
            ASSURE_P_RESULT(r_rc, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->reply_context = &r_rc.value.reply_context;
            break;
        case _ZN_MID_DECLARE:
            r_de = _zn_declare_decode(flags, buf);
            ASSURE_P_RESULT(r_de, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.declare = r_de.value.declare;
            return;
        case _ZN_MID_DATA:
            r_da = _zn_data_decode(flags, buf);
            ASSURE_P_RESULT(r_da, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.data = r_da.value.data;
            return;
        case _ZN_MID_QUERY:
            r_qu = _zn_query_decode(flags, buf);
            ASSURE_P_RESULT(r_qu, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.query = r_qu.value.query;
            return;
        case _ZN_MID_PULL:
            r_pu = _zn_pull_decode(flags, buf);
            ASSURE_P_RESULT(r_pu, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.pull = r_pu.value.pull;
            return;
        case _ZN_MID_UNIT:
            r_un = _zn_unit_decode(flags, buf);
            ASSURE_P_RESULT(r_un, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
            r->value.zenoh_message->body.unit = r_un.value.unit;
            return;
        default:
            r->tag = Z_ERROR_TAG;
            r->value.error = ZN_ZENOH_MESSAGE_PARSE_ERROR;
            _Z_ERROR("WARNING: Trying to decode zenoh message with unknown ID(%d)\n", mid);
            return;
        }
    } while (1);
}

_zn_zenoh_message_p_result_t
_zn_zenoh_message_decode(z_iobuf_t *buf)
{
    _zn_zenoh_message_p_result_t r;
    _zn_zenoh_message_p_result_init(&r);
    _zn_zenoh_message_decode_na(buf, &r);
    return r;
}

/*=============================*/
/*     Scout/Hello Messages    */
/*=============================*/
/*------------------ Scout Message ------------------*/
void _zn_scout_encode(z_iobuf_t *buf, const _zn_scout_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_SCOUT\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_SCOUT;
    if (msg->pid_request)
        hdr |= _ZN_FLAG_S_I;
    if (msg->what)
        hdr |= _ZN_FLAG_S_W;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    if (msg->what)
        z_zint_encode(buf, *msg->what);
}

void z_scout_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_scout_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_SCOUT\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.scout.pid_request = _ZN_HAS_FLAG(flags, _ZN_FLAG_S_I);

    // Decode the body
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_W)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.scout.what = &r_zint.value.zint;
    }
}

_zn_scout_result_t
z_scout_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_scout_result_t r;
    z_scout_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Hello Message ------------------*/
void _zn_hello_encode(z_iobuf_t *buf, const _zn_hello_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_HELLO\n");

    // Build the header
    uint8_t hdr = _ZN_MID_HELLO;
    if (msg->pid)
        hdr |= _ZN_FLAG_S_I;
    if (msg->whatami)
        hdr |= _ZN_FLAG_S_W;
    if (msg->locators)
        hdr |= _ZN_FLAG_S_L;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    if (msg->pid)
        z_uint8_array_encode(buf, msg->pid);
    if (msg->whatami)
        z_zint_encode(buf, *msg->whatami);
    if (msg->locators)
        z_locators_encode(buf, msg->locators);
}

void _zn_hello_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_hello_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_HELLO\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_I)
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.hello.pid = &r_arr.value.uint8_array;
    }
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_W)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.hello.whatami = &r_zint.value.zint;
    }
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_L)
    {
        z_locators_result_t r_locs = z_locators_decode(buf);
        ASSURE_P_RESULT(r_locs, r, Z_LOCATORS_PARSE_ERROR)
        r->value.hello.locators = &r_locs.value.locators;
    }
}

_zn_hello_result_t
z_hello_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_hello_result_t r;
    _zn_hello_decode_na(flags, buf, &r);
    return r;
}

/*=============================*/
/*       Session Messages      */
/*=============================*/
/*------------------ Open Message ------------------*/
void _zn_open_encode(z_iobuf_t *buf, const _zn_open_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_OPEN\n");

    // Encode the option flags
    uint8_t options = 0;
    if (msg->sn_resolution)
        options |= _ZN_FLAG_S_S;
    if (msg->locators)
        options |= _ZN_FLAG_S_L;

    // Encode the header
    uint8_t hdr = _ZN_MID_OPEN;
    if (options)
        hdr |= _ZN_FLAG_S_O;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    z_iobuf_write(buf, msg->version);
    z_uint8_array_encode(buf, &msg->opid);
    z_zint_encode(buf, msg->lease);
    z_zint_encode(buf, msg->initial_sn);

    // Encode and write the options if any
    if (options)
    {
        z_iobuf_write(buf, options);
        if (msg->sn_resolution)
            z_zint_encode(buf, *msg->sn_resolution);
        if (msg->locators)
            z_locators_encode(buf, msg->locators);
    }
}

void _zn_open_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_open_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_OPEN\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    r->value.open.version = z_iobuf_read(buf);

    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.open.whatami = r_zint.value.zint;

    z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
    r->value.open.opid = r_arr.value.uint8_array;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.open.lease = r_zint.value.zint;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.open.initial_sn = r_zint.value.zint;

    // Decode the options if any
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_O)
    {
        uint8_t options = z_iobuf_read(buf);
        if _ZN_HAS_FLAG (options, _ZN_FLAG_S_S)
        {
            z_zint_result_t r_zint = z_zint_decode(buf);
            ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
            r->value.open.sn_resolution = &r_zint.value.zint;
        }
        if _ZN_HAS_FLAG (options, _ZN_FLAG_S_L)
        {
            z_locators_result_t r_locs = z_locators_decode(buf);
            ASSURE_P_RESULT(r_locs, r, Z_LOCATORS_PARSE_ERROR)
            r->value.open.locators = &r_locs.value.locators;
        }
    }
}

_zn_open_result_t
z_open_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_open_result_t r;
    _zn_open_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Accept Message ------------------*/
void _zn_accept_encode(z_iobuf_t *buf, const _zn_accept_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ACCEPT\n");

    // Encode the option flags
    uint8_t options = 0;
    if (msg->sn_resolution)
        options |= _ZN_FLAG_S_S;
    if (msg->lease)
        options |= _ZN_FLAG_S_D;
    if (msg->locators)
        options |= _ZN_FLAG_S_L;

    // Encode the header
    uint8_t hdr = _ZN_MID_ACCEPT;
    if (options)
        hdr |= _ZN_FLAG_S_O;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    z_zint_encode(buf, msg->whatami);
    z_uint8_array_encode(buf, &msg->opid);
    z_uint8_array_encode(buf, &msg->apid);
    z_zint_encode(buf, msg->initial_sn);

    // Encode and write the options
    if (options)
    {
        z_iobuf_write(buf, options);
        if (msg->sn_resolution)
            z_zint_encode(buf, *msg->sn_resolution);
        if (msg->lease)
            z_zint_encode(buf, *msg->lease);
        if (msg->locators)
            z_locators_encode(buf, msg->locators);
    }
}

void _zn_accept_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_accept_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ACCEPT\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_wami = z_zint_decode(buf);
    ASSURE_P_RESULT(r_wami, r, Z_ZINT_PARSE_ERROR)
    r->value.accept.whatami = r_wami.value.zint;

    z_uint8_array_result_t r_opid = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_opid, r, Z_ARRAY_PARSE_ERROR)
    r->value.accept.opid = r_opid.value.uint8_array;

    z_uint8_array_result_t r_apid = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_apid, r, Z_ARRAY_PARSE_ERROR)
    r->value.accept.apid = r_apid.value.uint8_array;

    z_zint_result_t r_insn = z_zint_decode(buf);
    ASSURE_P_RESULT(r_insn, r, Z_ZINT_PARSE_ERROR)
    r->value.accept.initial_sn = r_insn.value.zint;

    // Decode the options
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_O)
    {
        uint8_t options = z_iobuf_read(buf);
        if _ZN_HAS_FLAG (options, _ZN_FLAG_S_S)
        {
            z_zint_result_t r_zint = z_zint_decode(buf);
            ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
            r->value.accept.sn_resolution = &r_zint.value.zint;
        }
        if _ZN_HAS_FLAG (options, _ZN_FLAG_S_D)
        {
            z_zint_result_t r_zint = z_zint_decode(buf);
            ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
            r->value.accept.lease = &r_zint.value.zint;
        }
        if _ZN_HAS_FLAG (options, _ZN_FLAG_S_L)
        {
            z_locators_result_t r_locs = z_locators_decode(buf);
            ASSURE_P_RESULT(r_locs, r, Z_LOCATORS_PARSE_ERROR)
            r->value.accept.locators = &r_locs.value.locators;
        }
    }
}

_zn_accept_result_t
_zn_accept_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_accept_result_t r;
    _zn_accept_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Close Message ------------------*/
void _zn_close_encode(z_iobuf_t *buf, const _zn_close_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_CLOSE\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_CLOSE;
    if (msg->pid)
        hdr |= _ZN_FLAG_S_I;
    if (msg->link_only)
        hdr |= _ZN_FLAG_S_K;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    if (msg->pid)
        z_uint8_array_encode(buf, msg->pid);
    z_iobuf_write(buf, msg->reason);
}

void _zn_close_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_close_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_CLOSE\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.close.link_only = _ZN_HAS_FLAG(flags, _ZN_FLAG_S_K);

    // Decode the body
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_I)
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.close.pid = &r_arr.value.uint8_array;
    }
    r->value.close.reason = z_iobuf_read(buf);
}

_zn_close_result_t
_zn_close_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_close_result_t r;
    _zn_close_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Sync Message ------------------*/
void _zn_sync_encode(z_iobuf_t *buf, const _zn_sync_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_SYNC\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_SYNC;
    if (msg->is_reliable)
        hdr |= _ZN_FLAG_S_R;
    if (msg->count)
        hdr |= _ZN_FLAG_S_C;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    z_zint_encode(buf, msg->sn);
    if (msg->count)
        z_zint_encode(buf, *msg->count);
}

void _zn_sync_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_sync_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_SYNC\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.sync.is_reliable = _ZN_HAS_FLAG(flags, _ZN_FLAG_S_R);

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.sync.sn = r_zint.value.zint;

    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_C)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.sync.count = &r_zint.value.zint;
    }
}

_zn_sync_result_t
_zn_sync_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_sync_result_t r;
    _zn_sync_decode_na(flags, buf, &r);
    return r;
}

/*------------------ AckNack Message ------------------*/
void _zn_ack_nack_encode(z_iobuf_t *buf, const _zn_ack_nack_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_ACK_NACK\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_ACK_NACK;
    if (msg->mask)
        hdr |= _ZN_FLAG_S_M;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    z_zint_encode(buf, msg->sn);
    if (msg->mask)
        z_zint_encode(buf, *msg->mask);
}

void _zn_ack_nack_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_ack_nack_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ACK_NACK\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.ack_nack.sn = r_zint.value.zint;

    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_M)
    {
        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.ack_nack.mask = &r_zint.value.zint;
    }
}

_zn_ack_nack_result_t
_zn_ack_nack_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_ack_nack_result_t r;
    _zn_ack_nack_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Keep Alive Message ------------------*/
void _zn_keep_alive_encode(z_iobuf_t *buf, const _zn_keep_alive_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_KEEP_ALIVE\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_KEEP_ALIVE;
    if (msg->pid)
        hdr |= _ZN_FLAG_S_I;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    if (msg->pid)
        z_uint8_array_encode(buf, msg->pid);
}

void _zn_keep_alive_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_keep_alive_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_KEEP_ALIVE\n");
    r->tag = Z_OK_TAG;

    // Decode the body
    if _ZN_HAS_FLAG (flags, _ZN_FLAG_S_I)
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.keep_alive.pid = &r_arr.value.uint8_array;
    }
}

_zn_keep_alive_result_t
_zn_keep_alive_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_keep_alive_result_t r;
    _zn_keep_alive_decode_na(flags, buf, &r);
    return r;
}

/*------------------ PingPong Messages ------------------*/
void _zn_ping_pong_encode(z_iobuf_t *buf, const _zn_ping_pong_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_PING_PONG\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_PING_PONG;
    if (msg->is_ping)
        hdr |= _ZN_FLAG_S_P;

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    z_zint_encode(buf, msg->hash);
}

void _zn_ping_pong_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_ping_pong_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_PING_PONG\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.ping_pong.is_ping = _ZN_HAS_FLAG(flags, _ZN_FLAG_S_P);

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.ping_pong.hash = r_zint.value.zint;
}

_zn_ping_pong_result_t
_zn_ping_pong_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_ping_pong_result_t r;
    _zn_ping_pong_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Frame Message ------------------*/
void _zn_frame_encode(z_iobuf_t *buf, const _zn_frame_t *msg)
{
    _Z_DEBUG("Encoding _ZN_MID_FRAME\n");

    // Encode the header
    uint8_t hdr = _ZN_MID_FRAME;
    if (msg->is_reliable)
        hdr |= _ZN_FLAG_S_R;
    if (msg->is_fragment)
    {
        hdr |= _ZN_FLAG_S_F;
        if (msg->payload.fragment.is_final)
            hdr |= _ZN_FLAG_S_E;
    }

    // Write the header
    z_iobuf_write(buf, hdr);

    // Encode the body
    z_zint_encode(buf, msg->sn);
    if (msg->is_fragment)
    {
        _zn_payload_encode(buf, &msg->payload.fragment.buffer);
    }
    else
    {
        unsigned int len = z_vec_length(&msg->payload.messages);
        for (unsigned int i = 0; i < len; ++i)
            _zn_zenoh_message_encode(buf, z_vec_get(&msg->payload.messages, i));
    }
}

void _zn_frame_decode_na(uint8_t flags, z_iobuf_t *buf, _zn_frame_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_FRAME\n");
    r->tag = Z_OK_TAG;

    // Decode the flags
    r->value.frame.is_reliable = _ZN_HAS_FLAG(flags, _ZN_FLAG_S_R);
    r->value.frame.is_fragment = _ZN_HAS_FLAG(flags, _ZN_FLAG_S_F);

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.frame.sn = r_zint.value.zint;

    // Decode the payload
    if (r->value.frame.is_fragment)
    {
        r->value.frame.payload.fragment.is_final = _ZN_HAS_FLAG(flags, _ZN_FLAG_S_E);
        r->value.frame.payload.fragment.buffer = _zn_payload_decode(buf);
    }
    else
    {
        r->value.frame.payload.messages = z_vec_make(1);
        do
        {
            _zn_zenoh_message_p_result_t r_zm = _zn_zenoh_message_decode(buf);
            if (r_zm.tag == Z_OK_TAG)
                z_vec_append(&r->value.frame.payload.messages, r_zm.value.zenoh_message);
            else
                return;
        } while (1);
    }
}

_zn_frame_result_t
_zn_frame_decode(uint8_t flags, z_iobuf_t *buf)
{
    _zn_frame_result_t r;
    _zn_frame_decode_na(flags, buf, &r);
    return r;
}

/*------------------ Session Message ------------------*/
void _zn_session_message_encode(z_iobuf_t *buf, const _zn_session_message_t *msg)
{
    if (msg->attachement)
        _zn_attachment_encode(buf, msg->attachement);

    switch (msg->id)
    {
    case _ZN_MID_SCOUT:
        _zn_scout_encode(buf, &msg->body.scout);
        break;
    case _ZN_MID_HELLO:
        _zn_hello_encode(buf, &msg->body.hello);
        break;
    case _ZN_MID_OPEN:
        _zn_open_encode(buf, &msg->body.open);
        break;
    case _ZN_MID_ACCEPT:
        _zn_accept_encode(buf, &msg->body.accept);
        break;
    case _ZN_MID_CLOSE:
        _zn_close_encode(buf, &msg->body.close);
        break;
    case _ZN_MID_SYNC:
        _zn_sync_encode(buf, &msg->body.sync);
        break;
    case _ZN_MID_ACK_NACK:
        _zn_ack_nack_encode(buf, &msg->body.ack_nack);
        break;
    case _ZN_MID_KEEP_ALIVE:
        _zn_keep_alive_encode(buf, &msg->body.keep_alive);
        break;
    case _ZN_MID_PING_PONG:
        _zn_ping_pong_encode(buf, &msg->body.ping_pong);
        break;
    case _ZN_MID_FRAME:
        _zn_frame_encode(buf, &msg->body.frame);
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode session message with unknown ID(%d)\n", msg->id);
        return;
    }
}

void _zn_session_message_decode_na(z_iobuf_t *buf, _zn_session_message_p_result_t *r)
{
    r->tag = Z_OK_TAG;

    uint8_t hdr;
    uint8_t mid;
    uint8_t flags;

    _zn_attachment_result_t r_at;
    _zn_scout_result_t r_sc;
    _zn_hello_result_t r_he;
    _zn_open_result_t r_op;
    _zn_accept_result_t r_ac;
    _zn_close_result_t r_cl;
    _zn_sync_result_t r_sy;
    _zn_ack_nack_result_t r_an;
    _zn_keep_alive_result_t r_ka;
    _zn_ping_pong_result_t r_pp;
    _zn_frame_result_t r_fr;

    do
    {
        hdr = z_iobuf_read(buf);
        mid = _ZN_MID(hdr);
        flags = _ZN_FLAGS(hdr);

        // Decode the session message
        switch (mid)
        {
        case _ZN_MID_ATTACHMENT:
            r_at = _zn_attachment_decode(flags, buf);
            ASSURE_P_RESULT(r_at, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->attachement = &r_at.value.attachment;
            break;
        case _ZN_MID_SCOUT:
            r_sc = z_scout_decode(flags, buf);
            ASSURE_P_RESULT(r_sc, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.scout = r_sc.value.scout;
            return;
        case _ZN_MID_HELLO:
            r_he = z_hello_decode(flags, buf);
            ASSURE_P_RESULT(r_he, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.hello = r_he.value.hello;
            return;
        case _ZN_MID_OPEN:
            r_op = z_open_decode(flags, buf);
            ASSURE_P_RESULT(r_op, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.open = r_op.value.open;
            return;
        case _ZN_MID_ACCEPT:
            r_ac = _zn_accept_decode(flags, buf);
            ASSURE_P_RESULT(r_ac, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.accept = r_ac.value.accept;
            return;
        case _ZN_MID_CLOSE:
            r_cl = _zn_close_decode(flags, buf);
            ASSURE_P_RESULT(r_cl, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.close = r_cl.value.close;
            return;
        case _ZN_MID_SYNC:
            r_sy = _zn_sync_decode(flags, buf);
            ASSURE_P_RESULT(r_sy, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.sync = r_sy.value.sync;
            return;
        case _ZN_MID_ACK_NACK:
            r_an = _zn_ack_nack_decode(flags, buf);
            ASSURE_P_RESULT(r_an, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.ack_nack = r_an.value.ack_nack;
            return;
        case _ZN_MID_KEEP_ALIVE:
            r_ka = _zn_keep_alive_decode(flags, buf);
            ASSURE_P_RESULT(r_ka, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.keep_alive = r_ka.value.keep_alive;
            return;
        case _ZN_MID_PING_PONG:
            r_pp = _zn_ping_pong_decode(flags, buf);
            ASSURE_P_RESULT(r_pp, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.ping_pong = r_pp.value.ping_pong;
            return;
        case _ZN_MID_FRAME:
            r_fr = _zn_frame_decode(flags, buf);
            ASSURE_P_RESULT(r_fr, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
            r->value.session_message->body.frame = r_fr.value.frame;
            return;
        default:
            r->tag = Z_ERROR_TAG;
            r->value.error = ZN_SESSION_MESSAGE_PARSE_ERROR;
            _Z_ERROR("WARNING: Trying to decode session message with unknown ID(%d)\n", mid);
            return;
        }
    } while (1);
}

_zn_session_message_p_result_t
_zn_session_message_decode(z_iobuf_t *buf)
{
    _zn_session_message_p_result_t r;
    _zn_session_message_p_result_init(&r);
    _zn_session_message_decode_na(buf, &r);
    return r;
}