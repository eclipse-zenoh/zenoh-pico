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
    uint8_t kind[4] = {ZN_PUSH_MODE, ZN_PULL_MODE, ZN_PERIODIC_PUSH_MODE, ZN_PERIODIC_PULL_MODE};

    zn_sub_mode_t sm;
    sm.kind = kind[rand() % 4];

    if (sm.kind == ZN_PERIODIC_PUSH_MODE || sm.kind == ZN_PERIODIC_PULL_MODE)
    {
        zn_temporal_property_t *t_p = (zn_temporal_property_t *)malloc(sizeof(zn_temporal_property_t));
        t_p->origin = gen_zint();
        t_p->period = gen_zint();
        t_p->duration = gen_zint();
        sm.period = t_p;
    }
    else
    {
        sm.period = NULL;
    }

    return sm;
}

/*=============================*/
/*     Asserting functions     */
/*=============================*/
void assert_eq_iobuf(const z_iobuf_t *left, const z_iobuf_t *right)
{
    printf("Buffers (left:right) -> ");
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
    printf("Array (left:right) -> ");
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
    printf("Timestamp (left:right) -> ");
    printf("Time (%llu:%llu), ", left->time, right->time);
    assert(left->time == right->time);

    printf("ID (");
    assert_eq_uint8_array(&left->id, &right->id);
    printf(")");
}

void assert_eq_res_key(const zn_res_key_t *left, const zn_res_key_t *right)
{
    printf("ResKey (left:right) -> ");
    printf("ID (%zu:%zu), ", left->rid, right->rid);
    assert(left->rid == right->rid);

    printf("Name (");
    if (left->rname)
        printf("%s", left->rname);
    else
        printf("NULL");
    printf(":");
    if (right->rname)
        printf("%s", right->rname);
    else
        printf("NULL");
    printf(")");

    if (left->rname && right->rname)
        assert(!strcmp(left->rname, right->rname));
    else
        assert(left->rname == right->rname);
}

void assert_eq_sub_mode(const zn_sub_mode_t *left, const zn_sub_mode_t *right)
{

    printf("SubMode (left:right) -> ");
    printf("Kind (%u:%u), ", left->kind, right->kind);
    assert(left->kind == right->kind);

    printf("Period (");
    if (left->period)
        printf("<%zu,%zu,%zu>", left->period->origin, left->period->period, left->period->duration);
    else
        printf("NULL");
    printf(":");
    if (right->period)
        printf("<%zu,%zu,%zu>", right->period->origin, right->period->period, right->period->duration);
    else
        printf("NULL");
    printf(")");
    if (left->period && right->period)
    {
        assert(left->period->origin == right->period->origin);
        assert(left->period->period == right->period->period);
        assert(left->period->duration == right->period->duration);
    }
    else
    {
        assert(left->period == right->period);
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
    for (unsigned int i = 0; i < buf.capacity; ++i)
        buf.buf[i] = 0;

    // Initialize
    zn_sub_mode_t e_sm = gen_sub_mode();

    // Encode
    _zn_sub_mode_encode(&buf, &e_sm);

    // Decode
    _zn_sub_mode_result_t r_sm = _zn_sub_mode_decode(&buf);
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
    _zn_res_key_encode(&buf, &e_rk);

    // Decode
    uint8_t flags = (e_rk.rname) ? 0 : _ZN_FLAG_Z_K;
    _zn_res_key_result_t r_rk = _zn_res_key_decode(flags, &buf);
    assert(r_rk.tag == Z_OK_TAG);

    zn_res_key_t d_rk = r_rk.value.res_key;
    printf("   ");
    assert_eq_res_key(&e_rk, &d_rk);
    printf("\n");

    z_iobuf_free(&buf);
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
    e_at.encoding = _ZN_FLAGS(gen_uint8());
    e_at.buffer = gen_iobuf(64);

    // Encode
    _zn_attachment_encode(&buf, &e_at);

    // Decode
    uint8_t hdr = z_iobuf_read(&buf);
    uint8_t flags = _ZN_FLAGS(hdr);
    _zn_attachment_result_t r_at = _zn_attachment_decode(flags, &buf);
    assert(r_at.tag == Z_OK_TAG);

    _zn_attachment_t d_at = r_at.value.attachment;
    printf("   Encoding (0x%x,0x%x), ", e_at.encoding, d_at.encoding);
    assert(e_at.encoding == d_at.encoding);
    assert_eq_iobuf(&e_at.buffer, &d_at.buffer);
    printf("\n");

    z_iobuf_free(&buf);
}

void reply_contex_decorator()
{
    printf("\n>> ReplyContext decorator\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    int is_final[2] = {0};
    for (unsigned int i = 0; i < 2; ++i)
    {
        _zn_reply_context_t e_rc;
        e_rc.qid = gen_zint();
        e_rc.source_kind = gen_zint();
        if (is_final[i])
        {
            e_rc.replier_id = NULL;
        }
        else
        {
            z_uint8_array_t id = gen_uint8_array(16);
            e_rc.replier_id = &id;
        }

        // Encode
        _zn_reply_context_encode(&buf, &e_rc);

        // Decode
        uint8_t hdr = z_iobuf_read(&buf);
        uint8_t flags = _ZN_FLAGS(hdr);
        _zn_reply_context_result_t r_at = _zn_reply_context_decode(flags, &buf);
        assert(r_at.tag == Z_OK_TAG);

        _zn_reply_context_t d_rc = r_at.value.reply_context;
        printf("   ");
        printf("QID (%zu,%zu), ", e_rc.qid, d_rc.qid);
        assert(e_rc.qid == d_rc.qid);
        printf("Source Kind (%zu,%zu), ", e_rc.source_kind, d_rc.source_kind);
        assert(e_rc.source_kind == d_rc.source_kind);
        printf("Replier ID (");
        if (e_rc.replier_id && d_rc.replier_id)
        {
            assert_eq_uint8_array(e_rc.replier_id, d_rc.replier_id);
        }
        else
        {
            printf("NULL:NULL");
            assert(e_rc.replier_id == d_rc.replier_id);
        }
        printf(")\n");

        z_iobuf_clear(&buf);
    }

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
    _zn_res_decl_encode(&buf, &e_rd);

    // Decode
    uint8_t hdr = z_iobuf_read(&buf);
    uint8_t flags = _ZN_FLAGS(hdr);
    _zn_res_decl_result_t r_rd = _zn_res_decl_decode(flags, &buf);
    assert(r_rd.tag == Z_OK_TAG);

    _zn_res_decl_t d_rd = r_rd.value.res_decl;
    printf("   RID (%zu,%zu), ", e_rd.rid, d_rd.rid);
    assert(e_rd.rid == d_rd.rid);
    assert_eq_res_key(&e_rd.key, &d_rd.key);
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
    _zn_pub_decl_encode(&buf, &e_pd);

    // Decode
    uint8_t hdr = z_iobuf_read(&buf);
    uint8_t flags = _ZN_FLAGS(hdr);
    _zn_pub_decl_result_t r_pd = _zn_pub_decl_decode(flags, &buf);
    assert(r_pd.tag == Z_OK_TAG);

    _zn_pub_decl_t d_pd = r_pd.value.pub_decl;
    printf("   ");
    assert_eq_res_key(&e_pd.key, &d_pd.key);
    printf("\n");

    z_iobuf_free(&buf);
}

void subscriber_declaration()
{
    printf("\n>> Subscriber declaration\n");
    z_iobuf_t buf = z_iobuf_make(128);

    // Initialize
    _zn_sub_decl_t e_sd;
    e_sd.is_reliable = gen_bool();
    e_sd.key = gen_res_key();
    zn_sub_mode_t sm = gen_sub_mode();
    e_sd.sub_mode = &sm;

    // Encode
    _zn_sub_decl_encode(&buf, &e_sd);

    // Decode
    uint8_t hdr = z_iobuf_read(&buf);
    uint8_t flags = _ZN_FLAGS(hdr);
    _zn_sub_decl_result_t r_pd = _zn_sub_decl_decode(flags, &buf);
    assert(r_pd.tag == Z_OK_TAG);

    _zn_sub_decl_t d_sd = r_pd.value.sub_decl;
    printf("   ");
    assert_eq_res_key(&e_sd.key, &d_sd.key);
    printf("\n");
    if (e_sd.sub_mode && d_sd.sub_mode)
    {
        printf("   ");
        assert_eq_sub_mode(e_sd.sub_mode, d_sd.sub_mode);
        printf("\n");
    }
    else
    {
        assert(e_sd.sub_mode == d_sd.sub_mode);
    }

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
        // Message decorators
        attachement_decorator();
        reply_contex_decorator();
        // Zenoh messages
        resource_declaration();
        publisher_declaration();
        subscriber_declaration();
    }

    return 0;
}
