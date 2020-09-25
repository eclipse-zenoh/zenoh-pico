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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "zenoh/iobuf.h"

#define ZENOH_C_NET_MSGCODEC_T
#include "zenoh/net/private/msgcodec.h"

#define RUNS 1000

/*=============================*/
/*       Helper functions      */
/*=============================*/
void print_iobuf(z_iobuf_t *buf)
{
    printf("Capacity: %u, Rpos: %u, Wpos: %u, Buffer: [", buf->capacity, buf->r_pos, buf->w_pos);
    for (unsigned int i = 0; i < buf->capacity; ++i)
    {
        printf("%02x", buf->buf[i]);
        if (i < buf->capacity - 1)
            printf(" ");
    }
    printf("]");
}

/*=============================*/
/*    Generating functions     */
/*=============================*/
int gen_bool()
{
    return rand() % 2;
}

uint8_t gen_uint8()
{
    return (uint8_t)rand() % 255;
}

z_zint_t gen_zint()
{
    return (z_zint_t)rand();
}

z_iobuf_t gen_iobuf(unsigned int len)
{
    z_iobuf_t buf = z_iobuf_make(len);
    for (unsigned int i = 0; i < len; ++i)
        z_iobuf_write(&buf, gen_uint8());

    return buf;
}

z_uint8_array_t gen_uint8_array(unsigned int len)
{
    z_uint8_array_t arr;
    arr.length = len;
    arr.elem = (uint8_t *)malloc(sizeof(uint8_t) * len);
    for (unsigned int i = 0; i < len; ++i)
        arr.elem[i] = gen_uint8();

    return arr;
}

uint64_t gen_time()
{
    return (uint64_t)time(NULL);
}

z_timestamp_t gen_timestamp()
{
    z_timestamp_t ts;
    ts.time = (u_int64_t)time(NULL);
    ts.id = gen_uint8_array(16);

    return ts;
}

char *gen_string(unsigned int size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ/";
    char *str = (char *)malloc((sizeof(char) * size) + 1);
    for (unsigned int i = 0; i < size; ++i)
    {
        int key = rand() % (unsigned int)(sizeof(charset) - 1);
        str[i] = charset[key];
    }
    str[size] = '\0';
    return str;
}

zn_res_key_t gen_res_key()
{
    zn_res_key_t key;
    key.rid = gen_zint();
    int is_numerical = gen_bool();
    if (is_numerical)
        key.rname = NULL;
    else
        key.rname = gen_string((rand() % 16) + 1);

    return key;
}

zn_sub_mode_t gen_sub_mode()
{
    uint8_t kind[2] = {ZN_PUSH_MODE, ZN_PULL_MODE};

    zn_sub_mode_t sm;
    sm.header = kind[rand() % 4];

    if (gen_bool())
    {
        _ZN_SET_FLAG(sm.header, _ZN_FLAG_Z_P);
        zn_temporal_property_t t_p;
        t_p.origin = gen_zint();
        t_p.period = gen_zint();
        t_p.duration = gen_zint();
        sm.period = t_p;
    }

    return sm;
}

zn_data_info_t gen_data_info()
{
    zn_data_info_t di;

    di.flags = 0;

    if (gen_bool())
    {
        di.source_id = gen_uint8_array(16);
        _ZN_SET_FLAG(di.flags, ZN_DATA_INFO_SRC_ID);
    }
    if (gen_bool())
    {
        di.source_sn = gen_zint();
        _ZN_SET_FLAG(di.flags, ZN_DATA_INFO_SRC_SN);
    }
    if (gen_bool())
    {
        di.first_router_id = gen_uint8_array(16);
        _ZN_SET_FLAG(di.flags, ZN_DATA_INFO_RTR_ID);
    }
    if (gen_bool())
    {
        di.first_router_sn = gen_zint();
        _ZN_SET_FLAG(di.flags, ZN_DATA_INFO_RTR_SN);
    }
    if (gen_bool())
    {
        di.tstamp = gen_timestamp();
        _ZN_SET_FLAG(di.flags, ZN_DATA_INFO_TSTAMP);
    }
    if (gen_bool())
    {
        di.kind = gen_zint();
        _ZN_SET_FLAG(di.flags, ZN_DATA_INFO_KIND);
    }
    if (gen_bool())
    {
        di.encoding = gen_zint();
        _ZN_SET_FLAG(di.flags, ZN_DATA_INFO_ENC);
    }

    return di;
}

_zn_declaration_t gen_declaration()
{
    uint8_t decl[] = {
        _ZN_DECL_RESOURCE,
        _ZN_DECL_PUBLISHER,
        _ZN_DECL_SUBSCRIBER,
        _ZN_DECL_QUERYABLE,
        _ZN_DECL_FORGET_RESOURCE,
        _ZN_DECL_FORGET_PUBLISHER,
        _ZN_DECL_FORGET_SUBSCRIBER,
        _ZN_DECL_FORGET_QUERYABLE};

    _zn_declaration_t d;
    d.header = decl[gen_uint8() % 8];

    switch (d.header)
    {
    case _ZN_DECL_RESOURCE:
        d.body.res.rid = gen_zint();
        d.body.res.key = gen_res_key();
        _ZN_SET_FLAG(d.header, (d.body.res.key.rname) ? 0 : _ZN_FLAG_Z_K);
        break;
    case _ZN_DECL_PUBLISHER:
        d.body.pub.key = gen_res_key();
        _ZN_SET_FLAG(d.header, (d.body.pub.key.rname) ? 0 : _ZN_FLAG_Z_K);
        break;
    case _ZN_DECL_SUBSCRIBER:
        d.body.sub.key = gen_res_key();
        _ZN_SET_FLAG(d.header, (d.body.sub.key.rname) ? 0 : _ZN_FLAG_Z_K);
        if (gen_bool())
        {
            d.body.sub.sub_mode = gen_sub_mode();
            _ZN_SET_FLAG(d.header, _ZN_FLAG_Z_S);
        }
        _ZN_SET_FLAG(d.header, (gen_bool()) ? _ZN_FLAG_Z_R : 0);
        break;
    case _ZN_DECL_QUERYABLE:
        d.body.qle.key = gen_res_key();
        _ZN_SET_FLAG(d.header, (d.body.qle.key.rname) ? 0 : _ZN_FLAG_Z_K);
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        d.body.forget_res.rid = gen_zint();
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        d.body.forget_pub.key = gen_res_key();
        _ZN_SET_FLAG(d.header, (d.body.forget_pub.key.rname) ? 0 : _ZN_FLAG_Z_K);
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        d.body.forget_sub.key = gen_res_key();
        _ZN_SET_FLAG(d.header, (d.body.forget_sub.key.rname) ? 0 : _ZN_FLAG_Z_K);
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        d.body.forget_qle.key = gen_res_key();
        _ZN_SET_FLAG(d.header, (d.body.forget_qle.key.rname) ? 0 : _ZN_FLAG_Z_K);
        break;
    }

    return d;
}

/*=============================*/
/*     Asserting functions     */
/*=============================*/
void assert_eq_iobuf(const z_iobuf_t *left, const z_iobuf_t *right)
{
    printf("IOBuf -> ");
    printf("Capacity (%u:%u), ", left->capacity, right->capacity);

    assert(left->capacity == right->capacity);
    printf("Content (");
    for (unsigned int i = 0; i < left->capacity; ++i)
    {
        uint8_t l = left->buf[i];
        uint8_t r = right->buf[i];

        printf("%u:%u", l, r);
        if (i < left->capacity - 1)
            printf(" ");

        assert(l == r);
    }
    printf(")");
}

void assert_eq_uint8_array(const z_uint8_array_t *left, const z_uint8_array_t *right)
{
    printf("Array -> ");
    printf("Length (%u:%u), ", left->length, right->length);

    assert(left->length == right->length);
    printf("Content (");
    for (unsigned int i = 0; i < left->length; ++i)
    {
        uint8_t l = left->elem[i];
        uint8_t r = right->elem[i];

        printf("%u:%u", l, r);
        if (i < left->length - 1)
            printf(" ");

        assert(l == r);
    }
    printf(")");
}

void assert_eq_timestamp(const z_timestamp_t *left, const z_timestamp_t *right)
{
    printf("Timestamp -> ");
    printf("Time (%llu:%llu), ", left->time, right->time);
    assert(left->time == right->time);

    printf("ID (");
    assert_eq_uint8_array(&left->id, &right->id);
    printf(")");
}

void assert_eq_res_key(const zn_res_key_t *left, const zn_res_key_t *right, uint8_t header)
{
    printf("ResKey -> ");
    printf("ID (%zu:%zu), ", left->rid, right->rid);
    assert(left->rid == right->rid);

    printf("Name (");
    if (!_ZN_HAS_FLAG(header, _ZN_FLAG_Z_K))
    {
        printf("%s:%s", left->rname, right->rname);
        assert(!strcmp(left->rname, right->rname));
    }
    else
    {
        printf("NULL:NULL");
    }
    printf(")");
}

void assert_eq_sub_mode(const zn_sub_mode_t *left, const zn_sub_mode_t *right)
{
    printf("SubMode -> ");
    printf("Kind (%u:%u), ", left->header, right->header);
    assert(left->header == right->header);

    printf("Period (");
    if _ZN_HAS_FLAG (left->header, _ZN_FLAG_Z_P)
        printf("<%zu:%zu,%zu>", left->period.origin, left->period.period, left->period.duration);
    else
        printf("NULL");
    printf(":");
    if _ZN_HAS_FLAG (right->header, _ZN_FLAG_Z_P)
        printf("<%zu:%zu,%zu>", right->period.origin, right->period.period, right->period.duration);
    else
        printf("NULL");
    printf(")");
    if (_ZN_HAS_FLAG(left->header, _ZN_FLAG_Z_P) && _ZN_HAS_FLAG(right->header, _ZN_FLAG_Z_P))
    {
        assert(left->period.origin == right->period.origin);
        assert(left->period.period == right->period.period);
        assert(left->period.duration == right->period.duration);
    }
}

void assert_eq_data_info(const zn_data_info_t *left, const zn_data_info_t *right)
{
    printf("DataInfo -> ");
    printf("Flags (%zu:%zu), ", left->flags, right->flags);
    assert(left->flags == right->flags);

    if _ZN_HAS_FLAG (left->flags, ZN_DATA_INFO_SRC_ID)
    {
        printf("Src ID -> ");
        assert_eq_uint8_array(&left->source_id, &right->source_id);
        printf(", ");
    }
    if _ZN_HAS_FLAG (left->flags, ZN_DATA_INFO_SRC_SN)
    {
        printf("Src SN (%zu:%zu), ", left->source_sn, right->source_sn);
        assert(left->source_sn == right->source_sn);
    }
    if _ZN_HAS_FLAG (left->flags, ZN_DATA_INFO_RTR_ID)
    {
        printf("Rtr ID -> ");
        assert_eq_uint8_array(&left->first_router_id, &right->first_router_id);
        printf(", ");
    }
    if _ZN_HAS_FLAG (left->flags, ZN_DATA_INFO_RTR_SN)
    {
        printf("Rtr SN (%zu:%zu), ", left->first_router_sn, right->first_router_sn);
        assert(left->first_router_sn == right->first_router_sn);
    }
    if _ZN_HAS_FLAG (left->flags, ZN_DATA_INFO_TSTAMP)
    {
        printf("Tstamp -> ");
        assert_eq_timestamp(&left->tstamp, &right->tstamp);
        printf(", ");
    }
    if _ZN_HAS_FLAG (left->flags, ZN_DATA_INFO_KIND)
    {
        printf("Kind (%zu:%zu), ", left->kind, right->kind);
        assert(left->kind == right->kind);
    }
    if _ZN_HAS_FLAG (left->flags, ZN_DATA_INFO_ENC)
    {
        printf("Encoding (%zu:%zu), ", left->encoding, right->encoding);
        assert(left->encoding == right->encoding);
    }
}

void assert_eq_declaration(const _zn_declaration_t *left, const _zn_declaration_t *right)
{
    printf("Declaration -> ");
    printf("Header (%u:%u), ", left->header, right->header);
    assert(left->header == right->header);

    switch (left->header)
    {
    case _ZN_DECL_RESOURCE:
        printf("RID (%zu%zu), ", left->body.res.rid, right->body.res.rid);
        assert(left->body.res.rid == right->body.res.rid);
        assert_eq_res_key(&left->body.res.key, &right->body.res.key, left->header);
        break;
    case _ZN_DECL_PUBLISHER:
        assert_eq_res_key(&left->body.pub.key, &right->body.pub.key, left->header);
        break;
    case _ZN_DECL_SUBSCRIBER:
        assert_eq_res_key(&left->body.sub.key, &right->body.sub.key, left->header);
        if _ZN_HAS_FLAG (left->header, _ZN_FLAG_Z_S)
            assert_eq_sub_mode(&left->body.sub.sub_mode, &right->body.sub.sub_mode);
        break;
    case _ZN_DECL_QUERYABLE:
        assert_eq_res_key(&left->body.qle.key, &right->body.qle.key, left->header);
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        printf("RID (%zu%zu), ", left->body.forget_res.rid, right->body.forget_res.rid);
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        assert_eq_res_key(&left->body.forget_pub.key, &right->body.forget_pub.key, left->header);
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        assert_eq_res_key(&left->body.forget_sub.key, &right->body.forget_sub.key, left->header);
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        assert_eq_res_key(&left->body.forget_qle.key, &right->body.forget_qle.key, left->header);
        break;
    }
}

/*=============================*/
/*       Message Fields        */
/*=============================*/
void payload_field()
{
    printf("\n>> Payload field\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t len = 64;
    z_iobuf_t e_pld = gen_iobuf(len);

    // Encode
    _zn_payload_encode(&buf, &e_pld);

    // Decode
    z_iobuf_t d_pld = _zn_payload_decode(&buf);
    printf("   ");
    assert_eq_iobuf(&e_pld, &d_pld);
    printf("\n");

    z_iobuf_free(&buf);
    z_iobuf_free(&e_pld);
    z_iobuf_free(&d_pld);
}

void timestamp_field()
{
    printf("\n>> Timestamp field\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    z_timestamp_t e_ts = gen_timestamp();

    // Encode
    _zn_timestamp_encode(&buf, &e_ts);

    // Decode
    _zn_timestamp_result_t r_ts = _zn_timestamp_decode(&buf);
    assert(r_ts.tag == Z_OK_TAG);

    z_timestamp_t d_ts = r_ts.value.timestamp;
    printf("   ");
    assert_eq_timestamp(&e_ts, &d_ts);
    printf("\n");

    z_iobuf_free(&buf);
}

void sub_mode_field()
{
    printf("\n>> SubMode field\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    zn_sub_mode_t e_sm = gen_sub_mode();

    // Encode
    zn_sub_mode_encode(&buf, &e_sm);

    // Decode
    zn_sub_mode_result_t r_sm = zn_sub_mode_decode(&buf);
    assert(r_sm.tag == Z_OK_TAG);

    zn_sub_mode_t d_sm = r_sm.value.sub_mode;
    printf("   ");
    assert_eq_sub_mode(&e_sm, &d_sm);
    printf("\n");

    z_iobuf_free(&buf);
}

void res_key_field()
{
    printf("\n>> ResKey field\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    zn_res_key_t e_rk = gen_res_key();

    // Encode
    uint8_t header = (e_rk.rname) ? 0 : _ZN_FLAG_Z_K;
    zn_res_key_encode(&buf, header, &e_rk);

    // Decode
    zn_res_key_result_t r_rk = zn_res_key_decode(&buf, header);
    assert(r_rk.tag == Z_OK_TAG);

    zn_res_key_t d_rk = r_rk.value.res_key;
    printf("   ");
    assert_eq_res_key(&e_rk, &d_rk, header);
    printf("\n");

    z_iobuf_free(&buf);
}

void data_info_field()
{
    printf("\n>> DataInfo field\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    zn_data_info_t e_di = gen_data_info();

    // Encode
    zn_data_info_encode(&buf, &e_di);

    // Decode
    zn_data_info_result_t r_di = zn_data_info_decode(&buf);
    assert(r_di.tag == Z_OK_TAG);

    zn_data_info_t d_di = r_di.value.data_info;
    printf("   ");
    assert_eq_data_info(&e_di, &d_di);
    printf("\n");

    z_iobuf_clear(&buf);
}

/*=============================*/
/*     Message decorators      */
/*=============================*/
void attachement_decorator()
{
    printf("\n>> Attachment decorator\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_attachment_t e_at;
    e_at.header = _ZN_MID_ATTACHMENT;
    _ZN_SET_FLAG(e_at.header, _ZN_FLAGS(gen_uint8()));
    e_at.buffer = gen_iobuf(64);

    // Encode
    _zn_attachment_encode(&buf, &e_at);

    // Decode
    uint8_t header = z_iobuf_read(&buf);
    _zn_attachment_result_t r_at = _zn_attachment_decode(&buf, header);
    assert(r_at.tag == Z_OK_TAG);

    _zn_attachment_t d_at = r_at.value.attachment;
    printf("   Header (%x:%x), ", e_at.header, d_at.header);
    assert(e_at.header == d_at.header);

    assert_eq_iobuf(&e_at.buffer, &d_at.buffer);
    printf("\n");

    z_iobuf_free(&buf);
}

void reply_contex_decorator()
{
    printf("\n>> ReplyContext decorator\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_reply_context_t e_rc;
    e_rc.header = _ZN_MID_REPLY_CONTEXT;
    e_rc.qid = gen_zint();
    e_rc.source_kind = gen_zint();
    if (gen_bool())
    {
        z_uint8_array_t id = gen_uint8_array(16);
        e_rc.replier_id = id;
    }
    else
    {
        _ZN_SET_FLAG(e_rc.header, _ZN_FLAG_Z_F);
    }

    // Encode
    _zn_reply_context_encode(&buf, &e_rc);

    // Decode
    uint8_t header = z_iobuf_read(&buf);
    _zn_reply_context_result_t r_rc = _zn_reply_context_decode(&buf, header);
    assert(r_rc.tag == Z_OK_TAG);

    _zn_reply_context_t d_rc = r_rc.value.reply_context;
    printf("   Header (%x:%x), ", e_rc.header, d_rc.header);
    assert(e_rc.header == d_rc.header);

    printf("QID (%zu:%zu), ", e_rc.qid, d_rc.qid);
    assert(e_rc.qid == d_rc.qid);

    printf("Source Kind (%zu:%zu), ", e_rc.source_kind, d_rc.source_kind);
    assert(e_rc.source_kind == d_rc.source_kind);

    printf("Replier ID (");
    if (!_ZN_HAS_FLAG(e_rc.header, _ZN_FLAG_Z_F))
        assert_eq_uint8_array(&e_rc.replier_id, &d_rc.replier_id);
    else
        printf("NULL:NULL");
    printf(")\n");

    z_iobuf_free(&buf);
}

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/
void resource_declaration()
{
    printf("\n>> Resource declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_res_decl_t e_rd;
    e_rd.rid = gen_zint();
    e_rd.key = gen_res_key();

    // Encode
    uint8_t e_hdr = (e_rd.key.rname) ? 0 : _ZN_FLAG_Z_K;
    _zn_res_decl_encode(&buf, e_hdr, &e_rd);

    // Decode
    _zn_res_decl_result_t r_rd = _zn_res_decl_decode(&buf, e_hdr);
    assert(r_rd.tag == Z_OK_TAG);

    _zn_res_decl_t d_rd = r_rd.value.res_decl;
    printf("   RID (%zu:%zu), ", e_rd.rid, d_rd.rid);
    assert(e_rd.rid == d_rd.rid);

    assert_eq_res_key(&e_rd.key, &d_rd.key, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

void publisher_declaration()
{
    printf("\n>> Publisher declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_pub_decl_t e_pd;
    e_pd.key = gen_res_key();

    // Encode
    uint8_t e_hdr = (e_pd.key.rname) ? 0 : _ZN_FLAG_Z_K;
    _zn_pub_decl_encode(&buf, e_hdr, &e_pd);

    // Decode
    _zn_pub_decl_result_t r_pd = _zn_pub_decl_decode(&buf, e_hdr);
    assert(r_pd.tag == Z_OK_TAG);

    _zn_pub_decl_t d_pd = r_pd.value.pub_decl;
    printf("   ");
    assert_eq_res_key(&e_pd.key, &d_pd.key, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

void subscriber_declaration()
{
    printf("\n>> Subscriber declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_sub_decl_t e_sd;
    uint8_t e_hdr = 0;
    _ZN_SET_FLAG(e_hdr, (gen_bool()) ? _ZN_FLAG_Z_R : 0);
    if (gen_bool())
    {
        _ZN_SET_FLAG(e_hdr, _ZN_FLAG_Z_S);
        e_sd.sub_mode = gen_sub_mode();
    }
    e_sd.key = gen_res_key();
    _ZN_SET_FLAG(e_hdr, (e_sd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    // Encode
    _zn_sub_decl_encode(&buf, e_hdr, &e_sd);

    // Decode
    _zn_sub_decl_result_t r_pd = _zn_sub_decl_decode(&buf, e_hdr);
    assert(r_pd.tag == Z_OK_TAG);

    _zn_sub_decl_t d_sd = r_pd.value.sub_decl;
    printf("   ");
    assert_eq_res_key(&e_sd.key, &d_sd.key, e_hdr);
    printf("\n");
    if _ZN_HAS_FLAG (e_hdr, _ZN_FLAG_Z_S)
    {
        printf("   ");
        assert_eq_sub_mode(&e_sd.sub_mode, &d_sd.sub_mode);
        printf("\n");
    }

    z_iobuf_free(&buf);
}

void queryable_declaration()
{
    printf("\n>> Queryable declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_pub_decl_t e_qd;
    e_qd.key = gen_res_key();

    // Encode
    uint8_t e_hdr = (e_qd.key.rname) ? 0 : _ZN_FLAG_Z_K;
    _zn_pub_decl_encode(&buf, e_hdr, &e_qd);

    // Decode
    _zn_pub_decl_result_t r_qd = _zn_pub_decl_decode(&buf, e_hdr);
    assert(r_qd.tag == Z_OK_TAG);

    _zn_pub_decl_t d_qd = r_qd.value.pub_decl;
    printf("   ");
    assert_eq_res_key(&e_qd.key, &d_qd.key, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

void forget_resource_declaration()
{
    printf("\n>> Forget resource declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_forget_res_decl_t e_frd;
    e_frd.rid = gen_zint();

    // Encode
    _zn_forget_res_decl_encode(&buf, &e_frd);

    // Decode
    _zn_forget_res_decl_result_t r_frd = _zn_forget_res_decl_decode(&buf);
    assert(r_frd.tag == Z_OK_TAG);

    _zn_forget_res_decl_t d_frd = r_frd.value.forget_res_decl;
    printf("   RID (%zu:%zu)", e_frd.rid, d_frd.rid);
    assert(e_frd.rid == d_frd.rid);
    printf("\n");

    z_iobuf_free(&buf);
}

void forget_publisher_declaration()
{
    printf("\n>> Forget publisher declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_forget_pub_decl_t e_fpd;
    e_fpd.key = gen_res_key();

    // Encode
    uint8_t e_hdr = (e_fpd.key.rname) ? 0 : _ZN_FLAG_Z_K;
    _zn_forget_pub_decl_encode(&buf, e_hdr, &e_fpd);

    // Decode
    _zn_forget_pub_decl_result_t r_fpd = _zn_forget_pub_decl_decode(&buf, e_hdr);
    assert(r_fpd.tag == Z_OK_TAG);

    _zn_forget_pub_decl_t d_fpd = r_fpd.value.forget_pub_decl;
    printf("   ");
    assert_eq_res_key(&e_fpd.key, &d_fpd.key, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

void forget_subscriber_declaration()
{
    printf("\n>> Forget subscriber declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_forget_sub_decl_t e_fsd;
    e_fsd.key = gen_res_key();

    // Encode
    uint8_t e_hdr = (e_fsd.key.rname) ? 0 : _ZN_FLAG_Z_K;
    _zn_forget_sub_decl_encode(&buf, e_hdr, &e_fsd);

    // Decode
    _zn_forget_sub_decl_result_t r_fsd = _zn_forget_sub_decl_decode(&buf, e_hdr);
    assert(r_fsd.tag == Z_OK_TAG);

    _zn_forget_sub_decl_t d_fsd = r_fsd.value.forget_sub_decl;
    printf("   ");
    assert_eq_res_key(&e_fsd.key, &d_fsd.key, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

void declare_message()
{
    printf("\n>> Declare message\n");
    z_iobuf_t buf = z_iobuf_make(512);

    // Initialize
    _zn_declare_t e_dcl;
    e_dcl.declarations.length = gen_zint() % 16;
    e_dcl.declarations.elem = (_zn_declaration_t *)malloc(sizeof(_zn_declaration_t) * e_dcl.declarations.length);

    for (z_zint_t i = 0; i < e_dcl.declarations.length; ++i)
        e_dcl.declarations.elem[i] = gen_declaration();

    // Encode
    _zn_declare_encode(&buf, &e_dcl);

    // Decode
    _zn_declare_result_t r_dcl = _zn_declare_decode(&buf);
    assert(r_dcl.tag == Z_OK_TAG);

    _zn_declare_t d_dcl = r_dcl.value.declare;
    assert(e_dcl.declarations.length == d_dcl.declarations.length);

    for (z_zint_t i = 0; i < e_dcl.declarations.length; ++i)
    {
        printf("   ");
        assert_eq_declaration(&e_dcl.declarations.elem[i], &d_dcl.declarations.elem[i]);
        printf("\n");
    }

    z_iobuf_free(&buf);
}

void data_message()
{
    printf("\n>> Data message\n");
    z_iobuf_t buf = z_iobuf_make(256);

    // Initialize
    _zn_data_t e_da;
    uint8_t e_hdr = 0;

    e_da.key = gen_res_key();
    _ZN_SET_FLAG(e_hdr, (e_da.key.rname) ? 0 : _ZN_FLAG_Z_K);
    if (gen_bool())
    {
        e_da.info = gen_data_info();
        _ZN_SET_FLAG(e_hdr, _ZN_FLAG_Z_I);
    }
    _ZN_SET_FLAG(e_hdr, (gen_bool()) ? _ZN_FLAG_Z_D : 0);
    e_da.payload = gen_iobuf(gen_uint8() % 64);

    // Encode
    _zn_data_encode(&buf, e_hdr, &e_da);

    // Decode
    _zn_data_result_t r_da = _zn_data_decode(&buf, e_hdr);
    assert(r_da.tag == Z_OK_TAG);

    _zn_data_t d_da = r_da.value.data;
    printf("   ");
    assert_eq_res_key(&e_da.key, &d_da.key, e_hdr);
    printf("\n");
    if _ZN_HAS_FLAG (e_hdr, _ZN_FLAG_Z_I)
    {
        printf("   ");
        assert_eq_data_info(&e_da.info, &d_da.info);
        printf("\n");
    }
    printf("   ");
    assert_eq_iobuf(&e_da.payload, &d_da.payload);
    printf("\n");
}

void pull_message()
{
    printf("\n>> Pull message\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_pull_t e_pu;
    uint8_t e_hdr = 0;

    e_pu.key = gen_res_key();
    _ZN_SET_FLAG(e_hdr, (e_pu.key.rname) ? 0 : _ZN_FLAG_Z_K);
    e_pu.pull_id = gen_zint();
    if (gen_bool())
    {
        e_pu.max_samples = gen_zint();
        _ZN_SET_FLAG(e_hdr, _ZN_FLAG_Z_N);
    }
    _ZN_SET_FLAG(e_hdr, (gen_bool()) ? _ZN_FLAG_Z_F : 0);

    // Encode
    _zn_pull_encode(&buf, e_hdr, &e_pu);

    // Decode
    _zn_pull_result_t r_pu = _zn_pull_decode(&buf, e_hdr);
    assert(r_pu.tag == Z_OK_TAG);

    _zn_pull_t d_pu = r_pu.value.pull;
    printf("   ");
    assert_eq_res_key(&e_pu.key, &d_pu.key, e_hdr);
    printf("\n");

    printf("   Pull ID (%zu:%zu)", e_pu.pull_id, d_pu.pull_id);
    assert(e_pu.pull_id == d_pu.pull_id);
    printf("\n");

    if _ZN_HAS_FLAG (e_hdr, _ZN_FLAG_Z_N)
    {
        printf("   Max samples (%zu:%zu)", e_pu.max_samples, d_pu.max_samples);
        assert(e_pu.max_samples == d_pu.max_samples);
        printf("\n");
    }

    z_iobuf_free(&buf);
}

void query_message()
{
    printf("\n>> Query message\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_query_t e_qy;
    uint8_t e_hdr = 0;

    e_qy.key = gen_res_key();
    _ZN_SET_FLAG(e_hdr, (e_qy.key.rname) ? 0 : _ZN_FLAG_Z_K);

    e_qy.predicate = gen_string(gen_uint8() % 16);

    e_qy.qid = gen_zint();

    if (gen_bool())
    {
        uint8_t tgt[] = {
            ZN_QTGT_BEST_MATCH,
            ZN_QTGT_COMPLETE,
            ZN_QTGT_ALL,
            ZN_QTGT_NONE};
        e_qy.target = tgt[gen_uint8() % 4];
        _ZN_SET_FLAG(e_hdr, _ZN_FLAG_Z_T);
    }

    uint8_t con[] = {
        ZN_QCON_NONE,
        ZN_QCON_LAST_HOP,
        ZN_QCON_INCREMENTAL};
    e_qy.consolidation = con[gen_uint8() % 3];

    // Encode
    _zn_query_encode(&buf, e_hdr, &e_qy);

    // Decode
    _zn_query_result_t r_qy = _zn_query_decode(&buf, e_hdr);
    assert(r_qy.tag == Z_OK_TAG);

    _zn_query_t d_qy = r_qy.value.query;
    printf("   ");
    assert_eq_res_key(&e_qy.key, &d_qy.key, e_hdr);
    printf("\n");

    printf("   Predicate (%s:%s)", e_qy.predicate, d_qy.predicate);
    assert(!strcmp(e_qy.predicate, d_qy.predicate));
    printf("\n");

    printf("   Query ID (%zu:%zu)", e_qy.qid, d_qy.qid);
    assert(e_qy.qid == d_qy.qid);
    printf("\n");

    if _ZN_HAS_FLAG (e_hdr, _ZN_FLAG_Z_T)
    {
        printf("   Target (%zu:%zu)", e_qy.target, d_qy.target);
        assert(e_qy.target == d_qy.target);
        printf("\n");
    }

    printf("   Consolidation (%zu:%zu)", e_qy.consolidation, d_qy.consolidation);
    assert(e_qy.consolidation == d_qy.consolidation);
    printf("\n");

    z_iobuf_free(&buf);
}

int main()
{
    for (unsigned int i = 0; i < RUNS; ++i)
    {
        printf("\n\n== RUN %u", i);
        // Message fields
        payload_field();
        timestamp_field();
        sub_mode_field();
        res_key_field();
        data_info_field();
        // Message decorators
        attachement_decorator();
        reply_contex_decorator();
        // Zenoh declarations
        resource_declaration();
        publisher_declaration();
        subscriber_declaration();
        queryable_declaration();
        forget_resource_declaration();
        forget_publisher_declaration();
        forget_subscriber_declaration();
        // Zenoh messages
        declare_message();
        data_message();
        pull_message();
        query_message();
    }

    return 0;
}
