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

z_string_t gen_string(unsigned int size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ/";
    z_string_t str = (char *)malloc((sizeof(char) * size) + 1);
    for (unsigned int i = 0; i < size; ++i)
    {
        int key = rand() % (unsigned int)(sizeof(charset) - 1);
        str[i] = charset[key];
    }
    str[size] = '\0';
    return str;
}

z_string_array_t gen_string_array(unsigned int size)
{
    z_string_array_t sa;
    sa.length = size;
    sa.elem = (z_string_t *)malloc(sizeof(z_string_t) * size);

    for (unsigned int i = 0; i < size; ++i)
        sa.elem[i] = gen_string(16);

    return sa;
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

void assert_eq_string_array(const z_string_array_t *left, const z_string_array_t *right)
{
    printf("Array -> ");
    printf("Length (%u:%u), ", left->length, right->length);

    assert(left->length == right->length);
    printf("Content (");
    for (unsigned int i = 0; i < left->length; ++i)
    {
        z_string_t l = left->elem[i];
        z_string_t r = right->elem[i];

        printf("%s:%s", l, r);
        if (i < left->length - 1)
            printf(" ");

        assert(!strcmp(l, r));
    }
    printf(")");
}

/*=============================*/
/*       Message Fields        */
/*=============================*/
/*------------------ Payload field ------------------*/
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

/*------------------ Timestamp field ------------------*/
z_timestamp_t gen_timestamp()
{
    z_timestamp_t ts;
    ts.time = (u_int64_t)time(NULL);
    ts.id = gen_uint8_array(16);

    return ts;
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

/*------------------ SubMode field ------------------*/
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

/*------------------ ResKey field ------------------*/
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

/*------------------ DataInfo field ------------------*/
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
/*------------------ Attachment decorator ------------------*/
_zn_attachment_t gen_attachment()
{
    _zn_attachment_t e_at;

    e_at.header = _ZN_MID_ATTACHMENT;
    _ZN_SET_FLAG(e_at.header, _ZN_FLAGS(gen_uint8()));
    e_at.buffer = gen_iobuf(64);

    return e_at;
}

void assert_eq_attachement(const _zn_attachment_t *left, const _zn_attachment_t *right)
{
    printf("Header (%x:%x), ", left->header, right->header);
    assert(left->header == right->header);
    assert_eq_iobuf(&left->buffer, &right->buffer);
}

void attachement_decorator()
{
    printf("\n>> Attachment decorator\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_attachment_t e_at = gen_attachment();

    // Encode
    _zn_attachment_encode(&buf, &e_at);

    // Decode
    uint8_t header = z_iobuf_read(&buf);
    _zn_attachment_result_t r_at = _zn_attachment_decode(&buf, header);
    assert(r_at.tag == Z_OK_TAG);

    _zn_attachment_t d_at = r_at.value.attachment;
    printf("   ");
    assert_eq_attachement(&e_at, &d_at);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ ReplyContext decorator ------------------*/
_zn_reply_context_t gen_reply_context()
{
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

    return e_rc;
}

void assert_eq_reply_context(const _zn_reply_context_t *left, const _zn_reply_context_t *right)
{
    printf("Header (%x:%x), ", left->header, right->header);
    assert(left->header == right->header);

    printf("QID (%zu:%zu), ", left->qid, right->qid);
    assert(left->qid == right->qid);

    printf("Source Kind (%zu:%zu), ", left->source_kind, right->source_kind);
    assert(left->source_kind == right->source_kind);

    printf("Replier ID (");
    if (!_ZN_HAS_FLAG(left->header, _ZN_FLAG_Z_F))
        assert_eq_uint8_array(&left->replier_id, &right->replier_id);
    else
        printf("NULL:NULL");
    printf(")");
}

void reply_contex_decorator()
{
    printf("\n>> ReplyContext decorator\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_reply_context_t e_rc = gen_reply_context();

    // Encode
    _zn_reply_context_encode(&buf, &e_rc);

    // Decode
    uint8_t header = z_iobuf_read(&buf);
    _zn_reply_context_result_t r_rc = _zn_reply_context_decode(&buf, header);
    assert(r_rc.tag == Z_OK_TAG);

    _zn_reply_context_t d_rc = r_rc.value.reply_context;
    printf("   ");
    assert_eq_reply_context(&e_rc, &d_rc);
    printf("\n");

    z_iobuf_free(&buf);
}

/*=============================*/
/*      Declaration Fields     */
/*=============================*/
/*------------------ Resource declaration ------------------*/
_zn_res_decl_t gen_resource_declaration(uint8_t *header)
{
    _zn_res_decl_t e_rd;

    e_rd.rid = gen_zint();
    e_rd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_rd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_rd;
}

void assert_eq_resource_declaration(const _zn_res_decl_t *left, const _zn_res_decl_t *right, uint8_t header)
{
    printf("RID (%zu:%zu), ", left->rid, right->rid);
    assert(left->rid == right->rid);
    assert_eq_res_key(&left->key, &right->key, header);
}

void resource_declaration()
{
    printf("\n>> Resource declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_res_decl_t e_rd = gen_resource_declaration(&e_hdr);

    // Encode
    _zn_res_decl_encode(&buf, e_hdr, &e_rd);

    // Decode
    _zn_res_decl_result_t r_rd = _zn_res_decl_decode(&buf, e_hdr);
    assert(r_rd.tag == Z_OK_TAG);

    _zn_res_decl_t d_rd = r_rd.value.res_decl;
    printf("   ");
    assert_eq_resource_declaration(&e_rd, &d_rd, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Publisher declaration ------------------*/
_zn_pub_decl_t gen_publisher_declaration(uint8_t *header)
{
    _zn_pub_decl_t e_pd;

    e_pd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_pd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_pd;
}

void assert_eq_publisher_declaration(const _zn_pub_decl_t *left, const _zn_pub_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void publisher_declaration()
{
    printf("\n>> Publisher declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_pub_decl_t e_pd = gen_publisher_declaration(&e_hdr);

    // Encode
    _zn_pub_decl_encode(&buf, e_hdr, &e_pd);

    // Decode
    _zn_pub_decl_result_t r_pd = _zn_pub_decl_decode(&buf, e_hdr);
    assert(r_pd.tag == Z_OK_TAG);

    _zn_pub_decl_t d_pd = r_pd.value.pub_decl;
    printf("   ");
    assert_eq_publisher_declaration(&e_pd, &d_pd, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Subscriber declaration ------------------*/
_zn_sub_decl_t gen_subscriber_declaration(uint8_t *header)
{
    _zn_sub_decl_t e_sd;

    _ZN_SET_FLAG(*header, (gen_bool()) ? _ZN_FLAG_Z_R : 0);
    if (gen_bool())
    {
        _ZN_SET_FLAG(*header, _ZN_FLAG_Z_S);
        e_sd.sub_mode = gen_sub_mode();
    }
    e_sd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_sd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_sd;
}

void assert_eq_subscriber_declaration(const _zn_sub_decl_t *left, const _zn_sub_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_S)
    {
        printf(", ");
        assert_eq_sub_mode(&left->sub_mode, &right->sub_mode);
    }
}

void subscriber_declaration()
{
    printf("\n>> Subscriber declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_sub_decl_t e_sd = gen_subscriber_declaration(&e_hdr);

    // Encode
    _zn_sub_decl_encode(&buf, e_hdr, &e_sd);

    // Decode
    _zn_sub_decl_result_t r_pd = _zn_sub_decl_decode(&buf, e_hdr);
    assert(r_pd.tag == Z_OK_TAG);

    _zn_sub_decl_t d_sd = r_pd.value.sub_decl;
    printf("   ");
    assert_eq_subscriber_declaration(&e_sd, &d_sd, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Queryable declaration ------------------*/
_zn_qle_decl_t gen_queryable_declaration(uint8_t *header)
{
    _zn_qle_decl_t e_qd;

    e_qd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_qd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_qd;
}

void assert_eq_queryable_declaration(const _zn_qle_decl_t *left, const _zn_qle_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void queryable_declaration()
{
    printf("\n>> Queryable declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_qle_decl_t e_qd = gen_queryable_declaration(&e_hdr);

    // Encode
    _zn_qle_decl_encode(&buf, e_hdr, &e_qd);

    // Decode
    _zn_qle_decl_result_t r_qd = _zn_qle_decl_decode(&buf, e_hdr);
    assert(r_qd.tag == Z_OK_TAG);

    _zn_qle_decl_t d_qd = r_qd.value.qle_decl;
    printf("   ");
    assert_eq_queryable_declaration(&e_qd, &d_qd, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Forget Resource declaration ------------------*/
_zn_forget_res_decl_t gen_forget_resource_declaration()
{
    _zn_forget_res_decl_t e_frd;

    e_frd.rid = gen_zint();

    return e_frd;
}

void assert_eq_forget_resource_declaration(const _zn_forget_res_decl_t *left, const _zn_forget_res_decl_t *right)
{
    printf("RID (%zu:%zu)", left->rid, right->rid);
    assert(left->rid == right->rid);
}

void forget_resource_declaration()
{
    printf("\n>> Forget resource declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_forget_res_decl_t e_frd = gen_forget_resource_declaration();

    // Encode
    _zn_forget_res_decl_encode(&buf, &e_frd);

    // Decode
    _zn_forget_res_decl_result_t r_frd = _zn_forget_res_decl_decode(&buf);
    assert(r_frd.tag == Z_OK_TAG);

    _zn_forget_res_decl_t d_frd = r_frd.value.forget_res_decl;
    printf("   ");
    assert_eq_forget_resource_declaration(&e_frd, &d_frd);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Forget Publisher declaration ------------------*/
_zn_forget_pub_decl_t gen_forget_publisher_declaration(uint8_t *header)
{
    _zn_forget_pub_decl_t e_fpd;

    e_fpd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_fpd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_fpd;
}

void assert_eq_forget_publisher_declaration(const _zn_forget_pub_decl_t *left, const _zn_forget_pub_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void forget_publisher_declaration()
{
    printf("\n>> Forget publisher declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_forget_pub_decl_t e_fpd = gen_forget_publisher_declaration(&e_hdr);

    // Encode
    _zn_forget_pub_decl_encode(&buf, e_hdr, &e_fpd);

    // Decode
    _zn_forget_pub_decl_result_t r_fpd = _zn_forget_pub_decl_decode(&buf, e_hdr);
    assert(r_fpd.tag == Z_OK_TAG);

    _zn_forget_pub_decl_t d_fpd = r_fpd.value.forget_pub_decl;
    printf("   ");
    assert_eq_forget_publisher_declaration(&e_fpd, &d_fpd, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Forget Subscriber declaration ------------------*/
_zn_forget_sub_decl_t gen_forget_subscriber_declaration(uint8_t *header)
{
    _zn_forget_sub_decl_t e_fsd;

    e_fsd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_fsd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_fsd;
}

void assert_eq_forget_subscriber_declaration(const _zn_forget_sub_decl_t *left, const _zn_forget_sub_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void forget_subscriber_declaration()
{
    printf("\n>> Forget subscriber declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_forget_sub_decl_t e_fsd = gen_forget_subscriber_declaration(&e_hdr);

    // Encode
    _zn_forget_sub_decl_encode(&buf, e_hdr, &e_fsd);

    // Decode
    _zn_forget_sub_decl_result_t r_fsd = _zn_forget_sub_decl_decode(&buf, e_hdr);
    assert(r_fsd.tag == Z_OK_TAG);

    _zn_forget_sub_decl_t d_fsd = r_fsd.value.forget_sub_decl;
    printf("   ");
    assert_eq_forget_subscriber_declaration(&e_fsd, &d_fsd, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Forget Queryable declaration ------------------*/
_zn_forget_qle_decl_t gen_forget_queryable_declaration(uint8_t *header)
{
    _zn_forget_qle_decl_t e_fqd;

    e_fqd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_fqd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_fqd;
}

void assert_eq_forget_queryable_declaration(const _zn_forget_qle_decl_t *left, const _zn_forget_qle_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void forget_queryable_declaration()
{
    printf("\n>> Forget queryable declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_forget_qle_decl_t e_fqd = gen_forget_queryable_declaration(&e_hdr);

    // Encode
    _zn_forget_qle_decl_encode(&buf, e_hdr, &e_fqd);

    // Decode
    _zn_forget_qle_decl_result_t r_fqd = _zn_forget_qle_decl_decode(&buf, e_hdr);
    assert(r_fqd.tag == Z_OK_TAG);

    _zn_forget_qle_decl_t d_fqd = r_fqd.value.forget_qle_decl;
    printf("   ");
    assert_eq_forget_queryable_declaration(&e_fqd, &d_fqd, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Declaration ------------------*/
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
        d.body.res = gen_resource_declaration(&d.header);
        break;
    case _ZN_DECL_PUBLISHER:
        d.body.pub = gen_publisher_declaration(&d.header);
        break;
    case _ZN_DECL_SUBSCRIBER:
        d.body.sub = gen_subscriber_declaration(&d.header);
        break;
    case _ZN_DECL_QUERYABLE:
        d.body.qle = gen_queryable_declaration(&d.header);
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        d.body.forget_res = gen_forget_resource_declaration();
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        d.body.forget_pub = gen_forget_publisher_declaration(&d.header);
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        d.body.forget_sub = gen_forget_subscriber_declaration(&d.header);
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        d.body.forget_qle = gen_forget_queryable_declaration(&d.header);
        break;
    }

    return d;
}

void assert_eq_declaration(const _zn_declaration_t *left, const _zn_declaration_t *right)
{
    printf("Declaration -> ");
    printf("Header (%x:%x), ", left->header, right->header);
    assert(left->header == right->header);

    switch (left->header)
    {
    case _ZN_DECL_RESOURCE:
        assert_eq_resource_declaration(&left->body.res, &right->body.res, left->header);
        break;
    case _ZN_DECL_PUBLISHER:
        assert_eq_publisher_declaration(&left->body.pub, &right->body.pub, left->header);
        break;
    case _ZN_DECL_SUBSCRIBER:
        assert_eq_subscriber_declaration(&left->body.sub, &right->body.sub, left->header);
        break;
    case _ZN_DECL_QUERYABLE:
        assert_eq_queryable_declaration(&left->body.qle, &right->body.qle, left->header);
        break;
    case _ZN_DECL_FORGET_RESOURCE:
        assert_eq_forget_resource_declaration(&left->body.forget_res, &right->body.forget_res);
        break;
    case _ZN_DECL_FORGET_PUBLISHER:
        assert_eq_forget_publisher_declaration(&left->body.forget_pub, &right->body.forget_pub, left->header);
        break;
    case _ZN_DECL_FORGET_SUBSCRIBER:
        assert_eq_forget_subscriber_declaration(&left->body.forget_sub, &right->body.forget_sub, left->header);
        break;
    case _ZN_DECL_FORGET_QUERYABLE:
        assert_eq_forget_queryable_declaration(&left->body.forget_qle, &right->body.forget_qle, left->header);
        break;
    }
}

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/
/*------------------ Declare message ------------------*/
_zn_declare_t gen_declare_message()
{
    _zn_declare_t e_dcl;
    e_dcl.declarations.length = gen_zint() % 16;
    e_dcl.declarations.elem = (_zn_declaration_t *)malloc(sizeof(_zn_declaration_t) * e_dcl.declarations.length);

    for (z_zint_t i = 0; i < e_dcl.declarations.length; ++i)
        e_dcl.declarations.elem[i] = gen_declaration();

    return e_dcl;
}

void assert_eq_declare_message(const _zn_declare_t *left, const _zn_declare_t *right)
{
    assert(left->declarations.length == right->declarations.length);

    for (z_zint_t i = 0; i < left->declarations.length; ++i)
    {
        printf("   ");
        assert_eq_declaration(&left->declarations.elem[i], &right->declarations.elem[i]);
        printf("\n");
    }
}

void declare_message()
{
    printf("\n>> Declare message\n");
    z_iobuf_t buf = z_iobuf_make(512);

    // Initialize
    _zn_declare_t e_dcl = gen_declare_message();

    // Encode
    _zn_declare_encode(&buf, &e_dcl);

    // Decode
    _zn_declare_result_t r_dcl = _zn_declare_decode(&buf);
    assert(r_dcl.tag == Z_OK_TAG);

    _zn_declare_t d_dcl = r_dcl.value.declare;
    assert_eq_declare_message(&e_dcl, &d_dcl);

    z_iobuf_free(&buf);
}

/*------------------ Data message ------------------*/
_zn_data_t gen_data_message(uint8_t *header)
{
    _zn_data_t e_da;

    e_da.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_da.key.rname) ? 0 : _ZN_FLAG_Z_K);
    if (gen_bool())
    {
        e_da.info = gen_data_info();
        _ZN_SET_FLAG(*header, _ZN_FLAG_Z_I);
    }
    _ZN_SET_FLAG(*header, (gen_bool()) ? _ZN_FLAG_Z_D : 0);
    e_da.payload = gen_iobuf(gen_uint8() % 64);

    return e_da;
}

void assert_eq_data_message(const _zn_data_t *left, const _zn_data_t *right, uint8_t header)
{
    printf("   ");
    assert_eq_res_key(&left->key, &right->key, header);
    printf("\n");
    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_I)
    {
        printf("   ");
        assert_eq_data_info(&left->info, &right->info);
        printf("\n");
    }
    printf("   ");
    assert_eq_iobuf(&left->payload, &right->payload);
    printf("\n");
}

void data_message()
{
    printf("\n>> Data message\n");
    z_iobuf_t buf = z_iobuf_make(256);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_data_t e_da = gen_data_message(&e_hdr);

    // Encode
    _zn_data_encode(&buf, e_hdr, &e_da);

    // Decode
    _zn_data_result_t r_da = _zn_data_decode(&buf, e_hdr);
    assert(r_da.tag == Z_OK_TAG);

    _zn_data_t d_da = r_da.value.data;
    assert_eq_data_message(&e_da, &d_da, e_hdr);

    z_iobuf_free(&buf);
}

/*------------------ Pull message ------------------*/
_zn_pull_t gen_pull_message(uint8_t *header)
{
    _zn_pull_t e_pu;

    e_pu.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_pu.key.rname) ? 0 : _ZN_FLAG_Z_K);
    e_pu.pull_id = gen_zint();
    if (gen_bool())
    {
        e_pu.max_samples = gen_zint();
        _ZN_SET_FLAG(*header, _ZN_FLAG_Z_N);
    }
    _ZN_SET_FLAG(*header, (gen_bool()) ? _ZN_FLAG_Z_F : 0);

    return e_pu;
}

void assert_eq_pull_message(const _zn_pull_t *left, const _zn_pull_t *right, uint8_t header)
{
    printf("   ");
    assert_eq_res_key(&left->key, &right->key, header);
    printf("\n");

    printf("   Pull ID (%zu:%zu)", left->pull_id, right->pull_id);
    assert(left->pull_id == right->pull_id);
    printf("\n");

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_N)
    {
        printf("   Max samples (%zu:%zu)", left->max_samples, right->max_samples);
        assert(left->max_samples == right->max_samples);
        printf("\n");
    }
}

void pull_message()
{
    printf("\n>> Pull message\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_pull_t e_pu = gen_pull_message(&e_hdr);

    // Encode
    _zn_pull_encode(&buf, e_hdr, &e_pu);

    // Decode
    _zn_pull_result_t r_pu = _zn_pull_decode(&buf, e_hdr);
    assert(r_pu.tag == Z_OK_TAG);

    _zn_pull_t d_pu = r_pu.value.pull;
    assert_eq_pull_message(&e_pu, &d_pu, e_hdr);

    z_iobuf_free(&buf);
}

/*------------------ Query message ------------------*/
_zn_query_t gen_query_message(uint8_t *header)
{
    _zn_query_t e_qy;

    e_qy.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_qy.key.rname) ? 0 : _ZN_FLAG_Z_K);
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
        _ZN_SET_FLAG(*header, _ZN_FLAG_Z_T);
    }
    uint8_t con[] = {
        ZN_QCON_NONE,
        ZN_QCON_LAST_HOP,
        ZN_QCON_INCREMENTAL};
    e_qy.consolidation = con[gen_uint8() % 3];

    return e_qy;
}

void assert_eq_query_message(const _zn_query_t *left, const _zn_query_t *right, uint8_t header)
{
    printf("   ");
    assert_eq_res_key(&left->key, &right->key, header);
    printf("\n");

    printf("   Predicate (%s:%s)", left->predicate, right->predicate);
    assert(!strcmp(left->predicate, right->predicate));
    printf("\n");

    printf("   Query ID (%zu:%zu)", left->qid, right->qid);
    assert(left->qid == right->qid);
    printf("\n");

    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_T)
    {
        printf("   Target (%zu:%zu)", left->target, right->target);
        assert(left->target == right->target);
        printf("\n");
    }

    printf("   Consolidation (%zu:%zu)", left->consolidation, right->consolidation);
    assert(left->consolidation == right->consolidation);
    printf("\n");
}

void query_message()
{
    printf("\n>> Query message\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_query_t e_qy = gen_query_message(&e_hdr);

    // Encode
    _zn_query_encode(&buf, e_hdr, &e_qy);

    // Decode
    _zn_query_result_t r_qy = _zn_query_decode(&buf, e_hdr);
    assert(r_qy.tag == Z_OK_TAG);

    _zn_query_t d_qy = r_qy.value.query;
    assert_eq_query_message(&e_qy, &d_qy, e_hdr);

    z_iobuf_free(&buf);
}

/*------------------ Zenoh message ------------------*/
void zenoh_message()
{
    printf("\n>> Zenoh message\n");
    z_iobuf_t buf = z_iobuf_make(1024);

    // Initialize
    zn_zenoh_message_t e_zm;
    // @TODO: test non-null attachment and reply_context
    // if (gen_bool())
    // {
    //     _zn_attachment_t e_at = gen_attachment();
    //     e_zm.attachment = &e_at;
    // }
    // else
    // {
    e_zm.attachment = NULL;
    // }
    // if (gen_bool())
    // {
    //     _zn_reply_context_t e_rc = gen_reply_context();
    //     e_zm.reply_context = &e_rc;
    // }
    // else
    // {
    e_zm.reply_context = NULL;
    // }

    uint8_t mids[] = {_ZN_MID_DECLARE, _ZN_MID_DATA, _ZN_MID_PULL, _ZN_MID_QUERY, _ZN_MID_UNIT};
    uint8_t i = gen_uint8() % 5;
    switch (mids[i])
    {
    case _ZN_MID_DECLARE:
        e_zm.header = _ZN_MID_DECLARE;
        e_zm.body.declare = gen_declare_message();
        break;
    case _ZN_MID_DATA:
        e_zm.header = _ZN_MID_DATA;
        e_zm.body.data = gen_data_message(&e_zm.header);
        break;
    case _ZN_MID_PULL:
        e_zm.header = _ZN_MID_PULL;
        e_zm.body.pull = gen_pull_message(&e_zm.header);
        break;
    case _ZN_MID_QUERY:
        e_zm.header = _ZN_MID_QUERY;
        e_zm.body.query = gen_query_message(&e_zm.header);
        break;
    case _ZN_MID_UNIT:
        e_zm.header = _ZN_MID_UNIT;
        _ZN_SET_FLAG(e_zm.header, (gen_bool()) ? _ZN_FLAG_Z_D : 0);
        // Unit messages have no body
        break;
    default:
        assert(0);
        break;
    }

    // Encode
    zn_zenoh_message_encode(&buf, &e_zm);

    // Decode
    zn_zenoh_message_p_result_t r_zm = zn_zenoh_message_decode(&buf);
    assert(r_zm.tag == Z_OK_TAG);

    zn_zenoh_message_t *d_zm = r_zm.value.zenoh_message;
    printf("   Header (%x:%x)", e_zm.header, d_zm->header);
    assert(e_zm.header == d_zm->header);

    switch (_ZN_MID(e_zm.header))
    {
    case _ZN_MID_DECLARE:
        assert_eq_declare_message(&e_zm.body.declare, &d_zm->body.declare);
        break;
    case _ZN_MID_DATA:
        assert_eq_data_message(&e_zm.body.data, &d_zm->body.data, e_zm.header);
        break;
    case _ZN_MID_PULL:
        assert_eq_pull_message(&e_zm.body.pull, &d_zm->body.pull, e_zm.header);
        break;
    case _ZN_MID_QUERY:
        assert_eq_query_message(&e_zm.body.query, &d_zm->body.query, e_zm.header);
        break;
    case _ZN_MID_UNIT:
        // Do nothin. Unit messages have no body
        break;
    default:
        assert(0);
        break;
    }

    zn_zenoh_message_p_result_free(&r_zm);
    z_iobuf_free(&buf);
}

/*=============================*/
/*     Scout/Hello Messages    */
/*=============================*/
/*------------------ Scout Message ------------------*/
zn_scout_t gen_scout_message(uint8_t *header)
{
    zn_scout_t e_sc;

    _ZN_SET_FLAG(*header, (gen_bool()) ? _ZN_FLAG_S_I : 0);
    if (gen_bool())
    {
        e_sc.what = gen_zint();
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_W);
    }
    else
    {
        e_sc.what = 0;
    }

    return e_sc;
}

void assert_eq_scout_message(const zn_scout_t *left, const zn_scout_t *right, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
    {
        printf("What (%zu:%zu)", left->what, right->what);
        assert(left->what == right->what);
    }
}

void scout_message()
{
    printf("\n>> Scout message\n");
    z_iobuf_t buf = z_iobuf_make(1024);

    // Initialize
    uint8_t e_hdr = 0;
    zn_scout_t e_sc = gen_scout_message(&e_hdr);

    // Encode
    zn_scout_encode(&buf, e_hdr, &e_sc);

    // Decode
    zn_scout_result_t r_sc = zn_scout_decode(&buf, e_hdr);
    assert(r_sc.tag == Z_OK_TAG);

    zn_scout_t d_sc = r_sc.value.scout;
    printf("   ");
    assert_eq_scout_message(&e_sc, &d_sc, e_hdr);
    printf("\n");

    z_iobuf_free(&buf);
}

/*------------------ Hello Message ------------------*/
zn_hello_t gen_hello_message(uint8_t *header)
{
    zn_hello_t e_he;

    if (gen_bool())
    {
        e_he.pid = gen_uint8_array(16);
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_I);
    }
    if (gen_bool())
    {
        e_he.whatami = gen_zint();
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_W);
    }
    if (gen_bool())
    {
        e_he.locators = gen_string_array((gen_uint8() % 4) + 1);
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_L);
    }

    return e_he;
}

void assert_eq_hello_message(const zn_hello_t *left, const zn_hello_t *right, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        printf("   ");
        assert_eq_uint8_array(&left->pid, &right->pid);
        printf("\n");
    }
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
    {
        printf("   What (%zu:%zu)", left->whatami, right->whatami);
        assert(left->whatami == right->whatami);
        printf("\n");
    }
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
    {
        assert_eq_string_array(&left->locators, &right->locators);
        printf("\n");
    }
}

void hello_message()
{
    printf("\n>> Hello message\n");
    z_iobuf_t buf = z_iobuf_make(1024);

    // Initialize
    uint8_t e_hdr = 0;
    zn_hello_t e_sc = gen_hello_message(&e_hdr);

    // Encode
    zn_hello_encode(&buf, e_hdr, &e_sc);

    // Decode
    zn_hello_result_t r_sc = zn_hello_decode(&buf, e_hdr);
    assert(r_sc.tag == Z_OK_TAG);

    zn_hello_t d_sc = r_sc.value.hello;
    assert_eq_hello_message(&e_sc, &d_sc, e_hdr);

    z_iobuf_free(&buf);
}

/*------------------ Open Message ------------------*/
_zn_open_t gen_open_message(uint8_t *header)
{
    _zn_open_t e_op;

    e_op.version = gen_uint8();
    e_op.whatami = gen_zint();
    e_op.opid = gen_uint8_array(16);
    e_op.lease = gen_zint();
    e_op.initial_sn = gen_zint();
    e_op.options = 0;
    if (gen_bool())
    {
        e_op.sn_resolution = gen_zint();
        _ZN_SET_FLAG(e_op.options, _ZN_FLAG_S_S);
    }
    if (gen_bool())
    {
        e_op.locators = gen_string_array((gen_uint8() % 4) + 1);
        _ZN_SET_FLAG(e_op.options, _ZN_FLAG_S_L);
    }
    if (e_op.options)
    {
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_O);
    }

    return e_op;
}

void assert_eq_open_message(const _zn_open_t *left, const _zn_open_t *right, uint8_t header)
{
    printf("   Version (%u:%u)", left->version, right->version);
    assert(left->version == right->version);
    printf("\n");

    printf("   WhatAmI (%zu:%zu)", left->whatami, right->whatami);
    assert(left->whatami == right->whatami);
    printf("\n");

    printf("   ");
    assert_eq_uint8_array(&left->opid, &right->opid);
    printf("\n");

    printf("   Lease (%zu:%zu)", left->lease, right->lease);
    assert(left->lease == right->lease);
    printf("\n");

    printf("   Initial SN (%zu:%zu)", left->initial_sn, right->initial_sn);
    assert(left->initial_sn == right->initial_sn);
    printf("\n");

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_O)
    {
        printf("   Options (%x:%xu)", left->options, right->options);
        assert(left->options == right->options);
        printf("\n");

        if _ZN_HAS_FLAG (left->options, _ZN_FLAG_S_S)
        {
            printf("   SN Resolution (%zu:%zu)", left->sn_resolution, right->sn_resolution);
            assert(left->sn_resolution == right->sn_resolution);
            printf("\n");
        }
        if _ZN_HAS_FLAG (left->options, _ZN_FLAG_S_L)
        {
            printf("   ");
            assert_eq_string_array(&left->locators, &right->locators);
            printf("\n");
        }
    }
}

void open_message()
{
    printf("\n>> Open message\n");
    z_iobuf_t buf = z_iobuf_make(1024);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_open_t e_op = gen_open_message(&e_hdr);

    // Encode
    _zn_open_encode(&buf, e_hdr, &e_op);

    // Decode
    _zn_open_result_t r_sc = _zn_open_decode(&buf, e_hdr);
    assert(r_sc.tag == Z_OK_TAG);

    _zn_open_t d_op = r_sc.value.open;
    assert_eq_open_message(&e_op, &d_op, e_hdr);

    z_iobuf_free(&buf);
}

/*=============================*/
/*       Session Messages      */
/*=============================*/
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
        forget_queryable_declaration();
        // Zenoh messages
        declare_message();
        data_message();
        pull_message();
        query_message();
        zenoh_message();
        // Session messages
        scout_message();
        hello_message();
        open_message();
    }

    return 0;
}
