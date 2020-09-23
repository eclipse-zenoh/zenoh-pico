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
        zn_temporal_property_encode(buf, &fld->period);
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
        ASSURE_P_RESULT(r_tp, r, ZN_ZENOH_MESSAGE_PARSE_ERROR)
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
    uint8_t header = _ZN_MID_ATTACHMENT | _ZN_FLAGS(msg->encoding);

    // Write the header
    z_iobuf_write(buf, header);

    // Encode the body
    _zn_payload_encode(buf, &msg->buffer);
}

void z_attachment_decode_na(z_iobuf_t *buf, _zn_attachment_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_ATTACHMENT\n");
    r->tag = Z_OK_TAG;

    // Decode the header
    uint8_t header = z_iobuf_read(buf);
    r->value.attachment.encoding = _ZN_FLAGS(header);

    // Decode the body
    r->value.attachment.buffer = _zn_payload_decode(buf);
}

_zn_attachment_result_t
z_attachment_decode(z_iobuf_t *buf)
{
    _zn_attachment_result_t r;
    z_attachment_decode_na(buf, &r);
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

void z_reply_context_decode_na(z_iobuf_t *buf, _zn_reply_context_result_t *r)
{
    _Z_DEBUG("Decoding _ZN_MID_REPLY_CONTEXT\n");
    r->tag = Z_OK_TAG;

    // Decode the header
    uint8_t header = z_iobuf_read(buf);

    // Decode the body
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.reply_context.qid = r_zint.value.zint;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.reply_context.source_kind = r_zint.value.zint;

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_F)
    {
        z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
        r->value.reply_context.replier_id = &r_arr.value.uint8_array;
    }
}

_zn_reply_context_result_t
z_reply_context_decode(z_iobuf_t *buf)
{
    _zn_reply_context_result_t r;
    z_reply_context_decode_na(buf, &r);
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
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.accept.whatami = r_zint.value.zint;

    z_uint8_array_result_t r_arr = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
    r->value.accept.opid = r_arr.value.uint8_array;

    r_arr = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_arr, r, Z_ARRAY_PARSE_ERROR)
    r->value.accept.apid = r_arr.value.uint8_array;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.accept.initial_sn = r_zint.value.zint;

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
            z_zenoh_message_encode(buf, z_vec_get(&msg->payload.messages, i));
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
        _zn_zenoh_message_p_result_t r_l;
        r_l.tag = Z_OK_TAG;
        while (r_l.tag == Z_OK_TAG)
        {
            // r_l = z_string_decode(buf);
            // ASSURE_P_RESULT(r_l, r, Z_STRING_PARSE_ERROR)
            // z_vec_append(&r->value.locators, r_l.value.string);
            // @TODO
        }
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
    {
        _zn_attachment_encode(buf, &msg->attachement);
    }

    switch (msg->id)
    {
    case _ZN_MID_SCOUT:
        _zn_scout_encode(buf, &msg->body.scout);
        break;
    case _ZN_MID_HELLO:
        _zn_hellp_encode(buf, &msg->body.hello);
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
    uint8_t hdr = z_iobuf_read(buf);
    uint8_t mid = _ZN_MID(hdr);
    uint8_t flags = _ZN_FLAGS(hdr);

    // @TODO: decode decorator

    r->tag = Z_OK_TAG;
    r->value.session_message->id = mid;

    switch (mid)
    {
    case _ZN_MID_SCOUT:
        _zn_scout_result_t r_sc = z_scout_decode(flags, buf);
        ASSURE_P_RESULT(r_sc, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.scout = r_sc.value.scout;
        break;
    case _ZN_MID_HELLO:
        _zn_hello_result_t r_he = z_hello_decode(flags, buf);
        ASSURE_P_RESULT(r_he, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.hello = r_he.value.hello;
        break;
    case _ZN_MID_OPEN:
        _zn_open_result_t r_op = z_open_decode(flags, buf);
        ASSURE_P_RESULT(r_op, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.open = r_op.value.open;
        break;
    case _ZN_MID_ACCEPT:
        _zn_accept_result_t r_ac = _zn_accept_decode(flags, buf);
        ASSURE_P_RESULT(r_ac, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.accept = r_ac.value.accept;
        break;
    case _ZN_MID_CLOSE:
        _zn_close_result_t r_cl = _zn_close_decode(flags, buf);
        ASSURE_P_RESULT(r_cl, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.close = r_cl.value.close;
        break;
    case _ZN_MID_SYNC:
        _zn_sync_result_t r_sy = _zn_sync_decode(flags, buf);
        ASSURE_P_RESULT(r_sy, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.sync = r_sy.value.sync;
        break;
    case _ZN_MID_ACK_NACK:
        _zn_ack_nack_result_t r_an = _zn_ack_nack_decode(flags, buf);
        ASSURE_P_RESULT(r_an, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.ack_nack = r_an.value.ack_nack;
        break;
    case _ZN_MID_KEEP_ALIVE:
        _zn_keep_alive_result_t r_ka = _zn_keep_alive_decode(flags, buf);
        ASSURE_P_RESULT(r_ka, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.keep_alive = r_ka.value.keep_alive;
        break;
    case _ZN_MID_PING_PONG:
        _zn_ping_pong_result_t r_pp = _zn_ping_pong_decode(flags, buf);
        ASSURE_P_RESULT(r_pp, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.ping_pong = r_pp.value.ping_pong;
    case _ZN_MID_FRAME:
        _zn_frame_result_t r_fr = _zn_frame_decode(flags, buf);
        ASSURE_P_RESULT(r_fr, r, ZN_SESSION_MESSAGE_PARSE_ERROR)
        r->value.session_message->body.frame = r_fr.value.frame;
        break;
    default:
        r->tag = Z_ERROR_TAG;
        r->value.error = ZN_SESSION_MESSAGE_PARSE_ERROR;
        _Z_ERROR("WARNING: Trying to decode session message with unknown ID(%d)\n", mid);
    }
}

_zn_session_message_p_result_t
_zn_session_message_decode(z_iobuf_t *buf)
{
    _zn_session_message_p_result_t r;
    _zn_session_message_p_result_init(&r);
    _zn_session_message_decode_na(buf, &r);
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
        _zn_sub_mode_encode(buf, &dcl->sub_mode);
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
        _Z_DEBUG("Declaration ID not supported\n");
        break;
    }
}

// @TODO: from here
void _zn_declaration_decode_na(z_iobuf_t *buf, _zn_declaration_result_t *r)
{
    r->tag = Z_OK_TAG;
    r->value.declaration.header = z_iobuf_read(buf);

    switch (dcl->id)
    {
    case _ZN_DECL_RESOURCE:
        _zn_res_decl_result_t r_rdcl = _zn_res_decl_decode(buf);
        ASSURE_P_RESULT(r_rdcl, r, Z_ZINT_PARSE_ERROR)
        r->value.declaration.body.res = r_rdcl.value.res_decl;
        break;
    case _ZN_DECL_PUBLISHER:
        break;
    case _ZN_DECL_SUBSCRIBER:
        break;
    case _ZN_DECL_QUERYABLE:
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        break;
    default:
        break;
    }
}

_zn_declaration_result_t
_zn_declare_decode(z_iobuf_t *buf)
{
    _zn_declaration_result_t r;
    _zn_declaration_decode_na(buf, &r);
    return r;
}

/*------------------ Declaration Message ------------------*/
void _zn_declare_encode(z_iobuf_t *buf, const _zn_declare_t *m)
{
    z_zint_encode(buf, m->sn);
    unsigned int len = m->declarations.length;
    z_zint_encode(buf, len);
    for (unsigned int i = 0; i < len; ++i)
    {
        _zn_declaration_encode(buf, &m->declarations.elem[i]);
    }
}

void _zn_declare_decode_na(z_iobuf_t *buf, _zn_declare_result_t *r)
{
    _zn_declaration_result_t *r_decl;
    z_zint_result_t r_sn = z_zint_decode(buf);
    r->tag = Z_OK_TAG;
    ASSURE_P_RESULT(r_sn, r, Z_ZINT_PARSE_ERROR)

    z_zint_result_t r_dlen = z_zint_decode(buf);
    ASSURE_P_RESULT(r_dlen, r, Z_ZINT_PARSE_ERROR)
    size_t len = r_dlen.value.zint;
    r->value.declare.declarations.length = len;
    r->value.declare.declarations.elem = (_zn_declaration_t *)malloc(sizeof(_zn_declaration_t) * len);

    r_decl = (_zn_declaration_result_t *)malloc(sizeof(_zn_declaration_result_t));
    for (size_t i = 0; i < len; ++i)
    {
        _zn_declaration_decode_na(buf, r_decl);
        if (r_decl->tag != Z_ERROR_TAG)
            r->value.declare.declarations.elem[i] = r_decl->value.declaration;
        else
        {
            r->value.declare.declarations.length = 0;
            free(r->value.declare.declarations.elem);
            free(r_decl);
            r->tag = Z_ERROR_TAG;
            r->value.error = ZN_MESSAGE_PARSE_ERROR;
            return;
        }
    }
    free(r_decl);
}

_zn_declare_result_t
_zn_declare_decode(z_iobuf_t *buf)
{
    _zn_declare_result_t r;
    _zn_declare_decode_na(buf, &r);
    return r;
}

void _zn_compact_data_encode(z_iobuf_t *buf, const _zn_compact_data_t *m)
{
    z_zint_encode(buf, m->sn);
    z_zint_encode(buf, m->rid);
    _zn_payload_encode(buf, &m->payload);
}

void _zn_compact_data_decode_na(z_iobuf_t *buf, _zn_compact_data_result_t *r)
{
    r->tag = Z_OK_TAG;
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)

    r->value.compact_data.sn = r_zint.value.zint;
    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.compact_data.rid = r_zint.value.zint;
    r->value.compact_data.payload = _zn_payload_decode(buf);
}

_zn_compact_data_result_t
_zn_compact_data_decode(z_iobuf_t *buf)
{
    _zn_compact_data_result_t r;
    _zn_compact_data_decode_na(buf, &r);
    return r;
}

void _zn_payload_header_encode(z_iobuf_t *buf, const _zn_payload_header_t *ph)
{
    uint8_t flags = ph->flags;
    _Z_DEBUG_VA("z_payload_header_encode flags = 0x%x\n", flags);
    z_iobuf_write(buf, flags);
    if (flags & _ZN_SRC_ID)
    {
        _Z_DEBUG("Encoding _Z_SRC_ID\n");
        z_iobuf_write_slice(buf, (uint8_t *)ph->src_id, 0, 16);
    }
    if (flags & _ZN_SRC_SN)
    {
        _Z_DEBUG("Encoding _Z_SRC_SN\n");
        z_zint_encode(buf, ph->src_sn);
    }
    if (flags & _ZN_BRK_ID)
    {
        _Z_DEBUG("Encoding _Z_BRK_ID\n");
        z_iobuf_write_slice(buf, (uint8_t *)ph->brk_id, 0, 16);
    }
    if (flags & _ZN_BRK_SN)
    {
        _Z_DEBUG("Encoding _Z_BRK_SN\n");
        z_zint_encode(buf, ph->brk_sn);
    }
    if (flags & _ZN_KIND)
    {
        _Z_DEBUG("Encoding _Z_KIND\n");
        z_zint_encode(buf, ph->kind);
    }
    if (flags & _ZN_ENCODING)
    {
        _Z_DEBUG("Encoding _Z_ENCODING\n");
        z_zint_encode(buf, ph->encoding);
    }

    _zn_payload_encode(buf, &ph->payload);
}

void _zn_payload_header_decode_na(z_iobuf_t *buf, _zn_payload_header_result_t *r)
{
    z_zint_result_t r_zint;
    uint8_t flags = z_iobuf_read(buf);
    _Z_DEBUG_VA("Payload header flags: 0x%x\n", flags);

    if (flags & _ZN_SRC_ID)
    {
        _Z_DEBUG("Decoding _Z_SRC_ID\n");
        z_iobuf_read_to_n(buf, r->value.payload_header.src_id, 16);
    }

    if (flags & _ZN_T_STAMP)
    {
        _Z_DEBUG("Decoding _Z_T_STAMP\n");
        r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.payload_header.tstamp.time = r_zint.value.zint;
        memcpy(r->value.payload_header.tstamp.clock_id, buf->buf + buf->r_pos, 16);
        buf->r_pos += 16;
    }

    if (flags & _ZN_SRC_SN)
    {
        _Z_DEBUG("Decoding _Z_SRC_SN\n");
        r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.payload_header.src_sn = r_zint.value.zint;
    }

    if (flags & _ZN_BRK_ID)
    {
        _Z_DEBUG("Decoding _Z_BRK_ID\n");
        z_iobuf_read_to_n(buf, r->value.payload_header.brk_id, 16);
    }

    if (flags & _ZN_BRK_SN)
    {
        _Z_DEBUG("Decoding _Z_BRK_SN\n");
        r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.payload_header.brk_sn = r_zint.value.zint;
    }

    if (flags & _ZN_KIND)
    {
        _Z_DEBUG("Decoding _Z_KIND\n");
        r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.payload_header.kind = r_zint.value.zint;
    }

    if (flags & _ZN_ENCODING)
    {
        _Z_DEBUG("Decoding _Z_ENCODING\n");
        r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.payload_header.encoding = r_zint.value.zint;
        _Z_DEBUG("Done Decoding _Z_ENCODING\n");
    }

    _Z_DEBUG("Decoding payload\n");
    r->value.payload_header.flags = flags;
    r->value.payload_header.payload = _zn_payload_decode(buf);
    r->tag = Z_OK_TAG;
}

_zn_payload_header_result_t
_zn_payload_header_decode(z_iobuf_t *buf)
{
    _zn_payload_header_result_t r;
    _zn_payload_header_decode_na(buf, &r);
    return r;
}

void _zn_stream_data_encode(z_iobuf_t *buf, const _zn_stream_data_t *m)
{
    z_zint_encode(buf, m->sn);
    z_zint_encode(buf, m->rid);
    z_zint_t len = z_iobuf_readable(&m->payload_header);
    z_zint_encode(buf, len);
    z_iobuf_write_slice(buf, m->payload_header.buf, m->payload_header.r_pos, m->payload_header.w_pos);
}

void _zn_stream_data_decode_na(z_iobuf_t *buf, _zn_stream_data_result_t *r)
{
    r->tag = Z_OK_TAG;
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.stream_data.sn = r_zint.value.zint;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.stream_data.rid = r_zint.value.zint;

    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    uint8_t *ph = z_iobuf_read_n(buf, r_zint.value.zint);
    r->value.stream_data.payload_header = z_iobuf_wrap_wo(ph, r_zint.value.zint, 0, r_zint.value.zint);
    r->value.stream_data.payload_header.w_pos = r_zint.value.zint;
}

_zn_stream_data_result_t
_zn_stream_data_decode(z_iobuf_t *buf)
{
    _zn_stream_data_result_t r;
    _zn_stream_data_decode_na(buf, &r);
    return r;
}

void _zn_write_data_encode(z_iobuf_t *buf, const _zn_write_data_t *m)
{
    z_zint_encode(buf, m->sn);
    z_string_encode(buf, m->rname);
    z_zint_t len = z_iobuf_readable(&m->payload_header);
    z_zint_encode(buf, len);
    z_iobuf_write_slice(buf, m->payload_header.buf, m->payload_header.r_pos, m->payload_header.w_pos);
}

void _zn_write_data_decode_na(z_iobuf_t *buf, _zn_write_data_result_t *r)
{
    r->tag = Z_OK_TAG;
    z_string_result_t r_str;
    z_zint_result_t r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    r->value.write_data.sn = r_zint.value.zint;

    r_str = z_string_decode(buf);
    ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
    r->value.write_data.rname = r_str.value.string;
    _Z_DEBUG_VA("Decoding write data for resource %s\n", r_str.value.string);
    r_zint = z_zint_decode(buf);
    ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
    uint8_t *ph = z_iobuf_read_n(buf, r_zint.value.zint);
    r->value.write_data.payload_header = z_iobuf_wrap_wo(ph, r_zint.value.zint, 0, r_zint.value.zint);
    r->value.write_data.payload_header.w_pos = r_zint.value.zint;
}

_zn_write_data_result_t
_zn_write_data_decode(z_iobuf_t *buf)
{
    _zn_write_data_result_t r;
    _zn_write_data_decode_na(buf, &r);
    return r;
}

void _zn_pull_encode(z_iobuf_t *buf, const _zn_pull_t *m)
{
    z_zint_encode(buf, m->sn);
    z_zint_encode(buf, m->id);
}

void _zn_query_encode(z_iobuf_t *buf, const _zn_query_t *m)
{
    z_uint8_array_encode(buf, &(m->pid));
    z_zint_encode(buf, m->qid);
    z_string_encode(buf, m->rname);
    z_string_encode(buf, m->predicate);
}

void _zn_query_decode_na(z_iobuf_t *buf, _zn_query_result_t *r)
{
    r->tag = Z_OK_TAG;

    z_uint8_array_result_t r_pid = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_pid, r, Z_ARRAY_PARSE_ERROR)
    r->value.query.pid = r_pid.value.uint8_array;

    z_zint_result_t r_qid = z_zint_decode(buf);
    ASSURE_P_RESULT(r_qid, r, Z_ZINT_PARSE_ERROR)
    r->value.query.qid = r_qid.value.zint;

    z_string_result_t r_str = z_string_decode(buf);
    ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
    r->value.query.rname = r_str.value.string;

    r_str = z_string_decode(buf);
    ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
    r->value.query.predicate = r_str.value.string;
}

_zn_query_result_t
_zn_query_decode(z_iobuf_t *buf)
{
    _zn_query_result_t r;
    _zn_query_decode_na(buf, &r);
    return r;
}

void _zn_reply_encode(z_iobuf_t *buf, const _zn_reply_t *m, uint8_t header)
{
    z_uint8_array_encode(buf, &(m->qpid));
    z_zint_encode(buf, m->qid);

    if (header & _ZN_F_FLAG)
    {
        z_uint8_array_encode(buf, &(m->srcid));
        z_zint_encode(buf, m->rsn);
        z_string_encode(buf, m->rname);
        z_zint_t len = z_iobuf_readable(&m->payload_header);
        z_zint_encode(buf, len);
        z_iobuf_write_slice(buf, m->payload_header.buf, m->payload_header.r_pos, m->payload_header.w_pos);
    }
}

void _zn_reply_decode_na(z_iobuf_t *buf, uint8_t header, _zn_reply_result_t *r)
{
    r->tag = Z_OK_TAG;

    z_uint8_array_result_t r_qpid = z_uint8_array_decode(buf);
    ASSURE_P_RESULT(r_qpid, r, Z_ARRAY_PARSE_ERROR)
    r->value.reply.qpid = r_qpid.value.uint8_array;

    z_zint_result_t r_qid = z_zint_decode(buf);
    ASSURE_P_RESULT(r_qid, r, Z_ZINT_PARSE_ERROR)
    r->value.reply.qid = r_qid.value.zint;

    if (header & _ZN_F_FLAG)
    {
        z_uint8_array_result_t r_srcid = z_uint8_array_decode(buf);
        ASSURE_P_RESULT(r_srcid, r, Z_ARRAY_PARSE_ERROR)
        r->value.reply.srcid = r_srcid.value.uint8_array;

        z_zint_result_t r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        r->value.reply.rsn = r_zint.value.zint;

        z_string_result_t r_str = z_string_decode(buf);
        ASSURE_P_RESULT(r_str, r, Z_STRING_PARSE_ERROR)
        r->value.reply.rname = r_str.value.string;

        r_zint = z_zint_decode(buf);
        ASSURE_P_RESULT(r_zint, r, Z_ZINT_PARSE_ERROR)
        uint8_t *ph = z_iobuf_read_n(buf, r_zint.value.zint);
        r->value.reply.payload_header = z_iobuf_wrap_wo(ph, r_zint.value.zint, 0, r_zint.value.zint);
        r->value.reply.payload_header.w_pos = r_zint.value.zint;
    }
}

_zn_reply_result_t
_zn_reply_decode(z_iobuf_t *buf, uint8_t header)
{
    _zn_reply_result_t r;
    _zn_reply_decode_na(buf, header, &r);
    return r;
}

void _zn_session_message_encode(z_iobuf_t *buf, const _zn_session_message_t *msg)
{
    // Encode decorators
    if (msg->attachement)
        _zn_attachment_encode(buf, msg->header);
    // Encode message
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_SCOUT:
        _zn_scout_encode(&msg->header, buf, &msg->body.scout);
        break;
    case _ZN_MID_HELLO:
        _zn_hello_encode(&msg->header, buf, &msg->body.hello);
        break;
    case _ZN_MID_OPEN:
        _zn_open_encode(&msg->header, buf, &msg->body.open);
        break;
    case _ZN_MID_ACCEPT:
        _zn_accept_encode(&msg->header, buf, &msg->body.accept);
        break;
    case _ZN_MID_CLOSE:
        break;
    case _ZN_MID_SYNC:
        break;
    case _ZN_MID_ACK_NACK:
        break;
    case _ZN_MID_KEEP_ALIVE:
        break;
    case _ZN_MID_PING_PONG:
        break;
    case _ZN_MID_FRAME:
        break;
    default:
        _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
        return;
    }
}

// void _zn_message_encode(z_iobuf_t *buf, const _zn_message_t *m)
// {
//     z_iobuf_write(buf, m->header);
//     uint8_t mid = _ZN_MID(m->header);
//     switch (mid)
//     {
//     case _ZN_COMPACT_DATA:
//         _zn_compact_data_encode(buf, &m->payload.compact_data);
//         break;
//     case _ZN_STREAM_DATA:
//         _zn_stream_data_encode(buf, &m->payload.stream_data);
//         break;
//     case _ZN_WRITE_DATA:
//         _zn_write_data_encode(buf, &m->payload.write_data);
//         break;
//     case _ZN_PULL:
//         _zn_pull_encode(buf, &m->payload.pull);
//         break;
//     case _ZN_QUERY:
//         _zn_query_encode(buf, &m->payload.query);
//         if (m->header & _ZN_P_FLAG)
//         {
//             zn_properties_encode(buf, m->properties);
//         }
//         break;
//     case _ZN_REPLY:
//         _zn_reply_encode(buf, &m->payload.reply, m->header);
//         break;
//     case _ZN_OPEN:
//         _zn_open_encode(buf, &m->payload.open);
//         if (m->header & _ZN_P_FLAG)
//         {
//             zn_properties_encode(buf, m->properties);
//         }
//         break;
//     case _ZN_CLOSE:
//         _zn_close_encode(buf, &m->payload.close);
//         break;
//     case _ZN_DECLARE:
//         _zn_declare_encode(buf, &m->payload.declare);
//         break;
//     default:
//         _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", mid);
//         return;
//     }
// }

// void _zn_message_decode_na(z_iobuf_t *buf, _zn_message_p_result_t *r)
// {
//     _zn_compact_data_result_t r_cd;
//     _zn_stream_data_result_t r_sd;
//     _zn_write_data_result_t r_wd;
//     _zn_query_result_t r_q;
//     _zn_reply_result_t r_r;
//     _zn_accept_result_t r_a;
//     _zn_close_result_t r_c;
//     _zn_declare_result_t r_d;
//     _zn_hello_result_t r_h;

//     uint8_t h = z_iobuf_read(buf);
//     r->tag = Z_OK_TAG;
//     r->value.message->header = h;

//     uint8_t mid = _ZN_MID(h);
//     switch (mid)
//     {
//     case _ZN_COMPACT_DATA:
//         r->tag = Z_OK_TAG;
//         r_cd = _zn_compact_data_decode(buf);
//         ASSURE_P_RESULT(r_cd, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.compact_data = r_cd.value.compact_data;
//         break;
//     case _ZN_STREAM_DATA:
//         r->tag = Z_OK_TAG;
//         r_sd = _zn_stream_data_decode(buf);
//         ASSURE_P_RESULT(r_sd, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.stream_data = r_sd.value.stream_data;
//         break;
//     case _ZN_WRITE_DATA:
//         r->tag = Z_OK_TAG;
//         r_wd = _zn_write_data_decode(buf);
//         ASSURE_P_RESULT(r_wd, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.write_data = r_wd.value.write_data;
//         break;
//     case _ZN_QUERY:
//         r->tag = Z_OK_TAG;
//         r_q = _zn_query_decode(buf);
//         ASSURE_P_RESULT(r_q, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.query = r_q.value.query;
//         break;
//     case _ZN_REPLY:
//         r->tag = Z_OK_TAG;
//         r_r = _zn_reply_decode(buf, h);
//         ASSURE_P_RESULT(r_r, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.reply = r_r.value.reply;
//         break;
//     case _ZN_ACCEPT:
//         r->tag = Z_OK_TAG;
//         r_a = _zn_accept_decode(buf);
//         ASSURE_P_RESULT(r_a, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.accept = r_a.value.accept;
//         break;
//     case _ZN_CLOSE:
//         r->tag = Z_OK_TAG;
//         r_c = _zn_close_decode(buf);
//         ASSURE_P_RESULT(r_c, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.close = r_c.value.close;
//         break;
//     case _ZN_DECLARE:
//         r->tag = Z_OK_TAG;
//         r_d = _zn_declare_decode(buf);
//         ASSURE_P_RESULT(r_d, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.declare = r_d.value.declare;
//         break;
//     case _ZN_HELLO:
//         r->tag = Z_OK_TAG;
//         r_h = z_hello_decode(buf);
//         ASSURE_P_RESULT(r_h, r, ZN_MESSAGE_PARSE_ERROR)
//         r->value.message->payload.hello = r_h.value.hello;
//         break;
//     default:
//         r->tag = Z_ERROR_TAG;
//         r->value.error = ZN_MESSAGE_PARSE_ERROR;
//         _Z_ERROR("WARNING: Trying to decode message with unknown ID(%d)\n", mid);
//     }
// }

// _zn_message_p_result_t
// _zn_message_decode(z_iobuf_t *buf)
// {
//     _zn_message_p_result_t r;
//     _zn_message_p_result_init(&r);
//     _zn_message_decode_na(buf, &r);
//     return r;
// }
