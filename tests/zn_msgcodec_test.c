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

#define _ZENOH_NET_PICO_MSGCODEC_H_T

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/private/collection.h"
#include "zenoh-pico/private/iobuf.h"
#include "zenoh-pico/net/private/internal.h"
#include "zenoh-pico/net/private/msgcodec.h"

#define RUNS 1000

/*=============================*/
/*       Helper functions      */
/*=============================*/
void print_iosli(_z_iosli_t *ios)
{
    printf("IOSli: Capacity: %zu, Rpos: %zu, Wpos: %zu, Buffer: [", ios->capacity, ios->r_pos, ios->w_pos);
    for (size_t i = 0; i < ios->capacity; ++i)
    {
        printf("%02x", ios->buf[i]);
        if (i < ios->capacity - 1)
            printf(" ");
    }
    printf("]");
}

void print_wbuf(_z_wbuf_t *wbf)
{
    printf("   WBuf: %zu, RIdx: %zu, WIdx: %zu\n", _z_wbuf_len_iosli(wbf), wbf->r_idx, wbf->w_idx);
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++)
    {
        printf("   - ");
        print_iosli(_z_wbuf_get_iosli(wbf, i));
        printf("\n");
    }
}

void print_uint8_array(z_bytes_t *arr)
{
    printf("Length: %zu, Buffer: [", arr->len);
    for (size_t i = 0; i < arr->len; ++i)
    {
        printf("%02x", arr->val[i]);
        if (i < arr->len - 1)
            printf(" ");
    }
    printf("]");
}

void print_session_message_type(uint8_t header)
{
    switch (_ZN_MID(header))
    {
    case _ZN_MID_SCOUT:
        printf("Scout message");
        break;
    case _ZN_MID_HELLO:
        printf("Hello message");
        break;
    case _ZN_MID_OPEN:
        printf("Open message");
        break;
    case _ZN_MID_ACCEPT:
        printf("Accept message");
        break;
    case _ZN_MID_CLOSE:
        printf("Close message");
        break;
    case _ZN_MID_SYNC:
        printf("Sync message");
        break;
    case _ZN_MID_ACK_NACK:
        printf("AckNack message");
        break;
    case _ZN_MID_KEEP_ALIVE:
        printf("KeepAlive message");
        break;
    case _ZN_MID_PING_PONG:
        printf("PingPong message");
        break;
    case _ZN_MID_FRAME:
        printf("Frame message");
        break;
    default:
        assert(0);
        break;
    }
}

/*=============================*/
/*    Generating functions     */
/*=============================*/
int gen_bool(void)
{
    return rand() % 2;
}

uint8_t gen_uint8(void)
{
    return (uint8_t)rand() % 255;
}

z_zint_t gen_zint(void)
{
    return (z_zint_t)rand();
}

_z_wbuf_t gen_wbuf(size_t len)
{
    int is_expandable = 0;

    if (gen_bool())
    {
        is_expandable = 1;
        len = 1 + (gen_zint() % len);
    }

    return _z_wbuf_make(len, is_expandable);
}

_zn_payload_t gen_payload(size_t len)
{
    _zn_payload_t pld;
    pld.len = len;
    pld.val = (uint8_t *)malloc(len * sizeof(uint8_t));
    for (size_t i = 0; i < len; ++i)
        ((uint8_t *)pld.val)[i] = 0xff;

    return pld;
}

z_bytes_t gen_bytes(size_t len)
{
    z_bytes_t arr;
    arr.len = len;
    arr.val = (uint8_t *)malloc(sizeof(uint8_t) * len);
    for (z_zint_t i = 0; i < len; ++i)
        ((uint8_t *)arr.val)[i] = gen_uint8();

    return arr;
}

uint64_t gen_time(void)
{
    return (uint64_t)time(NULL);
}

char *gen_string(size_t size)
{
    char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ/";
    char *str = (char *)malloc((sizeof(char) * size) + 1);
    for (z_zint_t i = 0; i < size; ++i)
    {
        int key = rand() % (int)(sizeof(charset) - 1);
        str[i] = charset[key];
    }
    str[size] = '\0';
    return str;
}

z_str_array_t gen_str_array(size_t size)
{
    z_str_array_t sa = _z_str_array_make(size);
    for (size_t i = 0; i < size; ++i)
        ((z_str_t *)sa.val)[i] = gen_string(16);

    return sa;
}

/*=============================*/
/*     Asserting functions     */
/*=============================*/
void assert_eq_iosli(_z_iosli_t *left, _z_iosli_t *right)
{
    printf("IOSli -> ");
    printf("Capacity (%zu:%zu), ", left->capacity, right->capacity);

    assert(left->capacity == right->capacity);

    printf("Content (");
    for (z_zint_t i = 0; i < left->capacity; ++i)
    {
        uint8_t l = left->buf[i];
        uint8_t r = right->buf[i];

        printf("%02x:%02x", l, r);
        if (i < left->capacity - 1)
            printf(" ");

        assert(l == r);
    }
    printf(")");
}

void assert_eq_uint8_array(z_bytes_t *left, z_bytes_t *right)
{
    printf("Array -> ");
    printf("Length (%zu:%zu), ", left->len, right->len);

    assert(left->len == right->len);
    printf("Content (");
    for (size_t i = 0; i < left->len; ++i)
    {
        uint8_t l = left->val[i];
        uint8_t r = right->val[i];

        printf("%02x:%02x", l, r);
        if (i < left->len - 1)
            printf(" ");

        assert(l == r);
    }
    printf(")");
}

void assert_eq_str_array(z_str_array_t *left, z_str_array_t *right)
{
    printf("Array -> ");
    printf("Length (%zu:%zu), ", left->len, right->len);

    assert(left->len == right->len);
    printf("Content (");
    for (size_t i = 0; i < left->len; ++i)
    {
        const char *l = left->val[i];
        const char *r = right->val[i];

        printf("%s:%s", l, r);
        if (i < left->len - 1)
            printf(" ");

        assert(strcmp(l, r) == 0);
    }
    printf(")");
}

/*=============================*/
/*       Message Fields        */
/*=============================*/
/*------------------ Payload field ------------------*/
void assert_eq_payload(_zn_payload_t *left, _zn_payload_t *right)
{
    assert_eq_uint8_array(left, right);
}

void payload_field(void)
{
    printf("\n>> Payload field\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    _zn_payload_t e_pld = gen_payload(64);

    // Encode
    int res = _zn_payload_encode(&wbf, &e_pld);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_payload_result_t r_pld = _zn_payload_decode(&rbf);
    assert(r_pld.tag == _z_res_t_OK);
    _zn_payload_t d_pld = r_pld.value.payload;
    printf("   ");
    assert_eq_payload(&e_pld, &d_pld);
    printf("\n");

    // Free
    _zn_payload_free(&d_pld);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Timestamp field ------------------*/
z_timestamp_t gen_timestamp(void)
{
    z_timestamp_t ts;
    ts.time = (u_int64_t)time(NULL);
    ts.id = gen_bytes(16);

    return ts;
}

void assert_eq_timestamp(z_timestamp_t *left, z_timestamp_t *right)
{
    printf("Timestamp -> ");
    printf("Time (%llu:%llu), ", (unsigned long long)left->time, (unsigned long long)right->time);
    assert(left->time == right->time);

    printf("ID (");
    assert_eq_uint8_array(&left->id, &right->id);
    printf(")");
}

void timestamp_field(void)
{
    printf("\n>> Timestamp field\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    z_timestamp_t e_ts = gen_timestamp();

    // Encode
    assert(_z_timestamp_encode(&wbf, &e_ts) == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_timestamp_result_t r_ts = _z_timestamp_decode(&rbf);
    assert(r_ts.tag == _z_res_t_OK);

    z_timestamp_t d_ts = r_ts.value.timestamp;
    printf("   ");
    assert_eq_timestamp(&e_ts, &d_ts);
    printf("\n");

    // Free
    _z_timestamp_free(&d_ts);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ SubInfo field ------------------*/
zn_subinfo_t gen_subinfo(void)
{
    zn_subinfo_t sm;
    sm.mode = gen_bool() ? zn_submode_t_PUSH : zn_submode_t_PULL;
    sm.reliability = gen_bool() ? zn_reliability_t_RELIABLE : zn_reliability_t_BEST_EFFORT;
    sm.period = gen_bool() ? (zn_period_t *)malloc(sizeof(zn_period_t)) : NULL;

    if (sm.period)
    {
        sm.period->origin = gen_zint();
        sm.period->period = gen_zint();
        sm.period->duration = gen_zint();
    }

    return sm;
}

void assert_eq_subinfo(zn_subinfo_t *left, zn_subinfo_t *right)
{
    printf("SubInfo -> ");
    printf("Mode (%u:%u), ", left->mode, right->mode);
    assert(left->mode == right->mode);

    printf("Reliable (%u:%u), ", left->reliability, right->reliability);
    assert(left->reliability == right->reliability);

    printf("Periodic (%d:%d), ", left->period ? 1 : 0, right->period ? 1 : 0);
    assert((left->period == NULL) == (right->period == NULL));

    printf("Period (");
    if (left->period)
        printf("<%u:%u,%u>", left->period->origin, left->period->period, left->period->duration);
    else
        printf("NULL");
    printf(":");
    if (right->period)
        printf("<%u:%u,%u>", right->period->origin, right->period->period, right->period->duration);
    else
        printf("NULL");
    printf(")");
    if (left->period && right->period)
    {
        assert(left->period->origin == right->period->origin);
        assert(left->period->period == right->period->period);
        assert(left->period->duration == right->period->duration);
    }
}

void subinfo_field(void)
{
    printf("\n>> SubInfo field\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    zn_subinfo_t e_sm = gen_subinfo();

    // Encode
    uint8_t header = e_sm.reliability == zn_reliability_t_RELIABLE ? _ZN_FLAG_Z_R : 0;
    int res = _zn_subinfo_encode(&wbf, &e_sm);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_subinfo_result_t r_sm = _zn_subinfo_decode(&rbf, header);
    assert(r_sm.tag == _z_res_t_OK);

    zn_subinfo_t d_sm = r_sm.value.subinfo;
    printf("   ");
    assert_eq_subinfo(&e_sm, &d_sm);
    printf("\n");

    // Free
    // NOTE: subinfo does not involve any heap allocation
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ ResKey field ------------------*/
zn_reskey_t gen_res_key(void)
{
    zn_reskey_t key;
    key.rid = gen_zint();
    int is_numerical = gen_bool();
    if (is_numerical)
        key.rname = NULL;
    else
        key.rname = gen_string((gen_zint() % 16) + 1);

    return key;
}

void assert_eq_res_key(zn_reskey_t *left, zn_reskey_t *right, uint8_t header)
{
    printf("ResKey -> ");
    printf("ID (%lu:%lu), ", left->rid, right->rid);
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

void res_key_field(void)
{
    printf("\n>> ResKey field\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    zn_reskey_t e_rk = gen_res_key();

    // Encode
    uint8_t header = (e_rk.rname) ? 0 : _ZN_FLAG_Z_K;
    int res = _zn_reskey_encode(&wbf, header, &e_rk);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_reskey_result_t r_rk = _zn_reskey_decode(&rbf, header);
    assert(r_rk.tag == _z_res_t_OK);

    zn_reskey_t d_rk = r_rk.value.reskey;
    printf("   ");
    assert_eq_res_key(&e_rk, &d_rk, header);
    printf("\n");

    // Free
    _zn_reskey_free(&d_rk);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ DataInfo field ------------------*/
_zn_data_info_t gen_data_info(void)
{
    _zn_data_info_t di;

    di.flags = 0;

    if (gen_bool())
    {
        di.source_id = gen_bytes(16);
        _ZN_SET_FLAG(di.flags, _ZN_DATA_INFO_SRC_ID);
    }
    if (gen_bool())
    {
        di.source_sn = gen_zint();
        _ZN_SET_FLAG(di.flags, _ZN_DATA_INFO_SRC_SN);
    }
    if (gen_bool())
    {
        di.first_router_id = gen_bytes(16);
        _ZN_SET_FLAG(di.flags, _ZN_DATA_INFO_RTR_ID);
    }
    if (gen_bool())
    {
        di.first_router_sn = gen_zint();
        _ZN_SET_FLAG(di.flags, _ZN_DATA_INFO_RTR_SN);
    }
    if (gen_bool())
    {
        di.tstamp = gen_timestamp();
        _ZN_SET_FLAG(di.flags, _ZN_DATA_INFO_TSTAMP);
    }
    if (gen_bool())
    {
        di.kind = gen_zint();
        _ZN_SET_FLAG(di.flags, _ZN_DATA_INFO_KIND);
    }
    if (gen_bool())
    {
        di.encoding = gen_zint();
        _ZN_SET_FLAG(di.flags, _ZN_DATA_INFO_ENC);
    }

    return di;
}

void assert_eq_data_info(_zn_data_info_t *left, _zn_data_info_t *right)
{
    printf("DataInfo -> ");
    printf("Flags (%zu:%zu), ", left->flags, right->flags);
    assert(left->flags == right->flags);

    if _ZN_HAS_FLAG (left->flags, _ZN_DATA_INFO_SRC_ID)
    {
        printf("Src ID -> ");
        assert_eq_uint8_array(&left->source_id, &right->source_id);
        printf(", ");
    }
    if _ZN_HAS_FLAG (left->flags, _ZN_DATA_INFO_SRC_SN)
    {
        printf("Src SN (%zu:%zu), ", left->source_sn, right->source_sn);
        assert(left->source_sn == right->source_sn);
    }
    if _ZN_HAS_FLAG (left->flags, _ZN_DATA_INFO_RTR_ID)
    {
        printf("Rtr ID -> ");
        assert_eq_uint8_array(&left->first_router_id, &right->first_router_id);
        printf(", ");
    }
    if _ZN_HAS_FLAG (left->flags, _ZN_DATA_INFO_RTR_SN)
    {
        printf("Rtr SN (%zu:%zu), ", left->first_router_sn, right->first_router_sn);
        assert(left->first_router_sn == right->first_router_sn);
    }
    if _ZN_HAS_FLAG (left->flags, _ZN_DATA_INFO_TSTAMP)
    {
        printf("Tstamp -> ");
        assert_eq_timestamp(&left->tstamp, &right->tstamp);
        printf(", ");
    }
    if _ZN_HAS_FLAG (left->flags, _ZN_DATA_INFO_KIND)
    {
        printf("Kind (%zu:%zu), ", left->kind, right->kind);
        assert(left->kind == right->kind);
    }
    if _ZN_HAS_FLAG (left->flags, _ZN_DATA_INFO_ENC)
    {
        printf("Encoding (%zu:%zu), ", left->encoding, right->encoding);
        assert(left->encoding == right->encoding);
    }
}

void data_info_field(void)
{
    printf("\n>> DataInfo field\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    _zn_data_info_t e_di = gen_data_info();

    // Encode
    int res = _zn_data_info_encode(&wbf, &e_di);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_data_info_result_t r_di = _zn_data_info_decode(&rbf);
    assert(r_di.tag == _z_res_t_OK);

    _zn_data_info_t d_di = r_di.value.data_info;
    printf("   ");
    assert_eq_data_info(&e_di, &d_di);
    printf("\n");

    // Free
    _zn_data_info_free(&d_di);
    _z_rbuf_free(&rbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*     Message decorators      */
/*=============================*/
/*------------------ Attachment decorator ------------------*/
void print_attachment(_zn_attachment_t *att)
{
    printf("      Header: %x\n", att->header);
    printf("      Payload: ");
    print_uint8_array(&att->payload);
    printf("\n");
}

_zn_attachment_t *gen_attachment(void)
{
    _zn_attachment_t *p_at = (_zn_attachment_t *)malloc(sizeof(_zn_attachment_t));

    p_at->header = _ZN_MID_ATTACHMENT;
    _ZN_SET_FLAG(p_at->header, _ZN_FLAGS(gen_uint8()));
    p_at->payload = gen_payload(64);

    return p_at;
}

void assert_eq_attachment(_zn_attachment_t *left, _zn_attachment_t *right)
{
    printf("Header (%x:%x), ", left->header, right->header);
    assert(left->header == right->header);
    assert_eq_payload(&left->payload, &right->payload);
}

void attachment_decorator(void)
{
    printf("\n>> Attachment decorator\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    _zn_attachment_t *e_at = gen_attachment();

    // Encode
    int res = _zn_attachment_encode(&wbf, e_at);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    uint8_t header = _z_rbuf_read(&rbf);
    _zn_attachment_p_result_t r_at = _zn_attachment_decode(&rbf, header);
    assert(r_at.tag == _z_res_t_OK);

    _zn_attachment_t *d_at = r_at.value.attachment;
    printf("   ");
    assert_eq_attachment(e_at, d_at);
    printf("\n");

    // Free
    free(e_at);
    _zn_attachment_free(d_at);
    _zn_attachment_p_result_free(&r_at);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ ReplyContext decorator ------------------*/
void print_reply_context(_zn_reply_context_t *rc)
{
    printf("      Header: %x\n", rc->header);
    printf("      QID: %zu\n", rc->qid);
    printf("      Source Kind: %zu\n", rc->source_kind);
    if (!_ZN_HAS_FLAG(rc->header, _ZN_FLAG_Z_F))
    {
        printf("      Reply ID: ");
        print_uint8_array((z_bytes_t *)&rc->replier_id);
    }
    printf("\n");
}

_zn_reply_context_t *gen_reply_context(void)
{
    _zn_reply_context_t *p_rc = (_zn_reply_context_t *)malloc(sizeof(_zn_reply_context_t));

    p_rc->header = _ZN_MID_REPLY_CONTEXT;
    p_rc->qid = gen_zint();
    p_rc->source_kind = gen_zint();
    if (gen_bool())
    {
        z_bytes_t id = gen_bytes(16);
        p_rc->replier_id = id;
    }
    else
    {
        _ZN_SET_FLAG(p_rc->header, _ZN_FLAG_Z_F);
    }

    return p_rc;
}

void assert_eq_reply_context(_zn_reply_context_t *left, _zn_reply_context_t *right)
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

void reply_contex_decorator(void)
{
    printf("\n>> ReplyContext decorator\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    _zn_reply_context_t *e_rc = gen_reply_context();

    // Encode
    int res = _zn_reply_context_encode(&wbf, e_rc);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    uint8_t header = _z_rbuf_read(&rbf);
    _zn_reply_context_p_result_t r_rc = _zn_reply_context_decode(&rbf, header);
    assert(r_rc.tag == _z_res_t_OK);

    _zn_reply_context_t *d_rc = r_rc.value.reply_context;
    printf("   ");
    assert_eq_reply_context(e_rc, d_rc);
    printf("\n");

    // Free
    free(e_rc);
    _zn_reply_context_free(d_rc);
    _zn_reply_context_p_result_free(&r_rc);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*=============================*/
/*      Declaration Fields     */
/*=============================*/
/*------------------ Resource declaration ------------------*/
_zn_res_decl_t gen_resource_declaration(uint8_t *header)
{
    _zn_res_decl_t e_rd;

    e_rd.id = gen_zint();
    e_rd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_rd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_rd;
}

void assert_eq_resource_declaration(_zn_res_decl_t *left, _zn_res_decl_t *right, uint8_t header)
{
    printf("RID (%zu:%zu), ", left->id, right->id);
    assert(left->id == right->id);
    assert_eq_res_key(&left->key, &right->key, header);
}

void resource_declaration(void)
{
    printf("\n>> Resource declaration\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_res_decl_t e_rd = gen_resource_declaration(&e_hdr);

    // Encode
    int res = _zn_res_decl_encode(&wbf, e_hdr, &e_rd);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_res_decl_result_t r_rd = _zn_res_decl_decode(&rbf, e_hdr);
    assert(r_rd.tag == _z_res_t_OK);

    _zn_res_decl_t d_rd = r_rd.value.res_decl;
    printf("   ");
    assert_eq_resource_declaration(&e_rd, &d_rd, e_hdr);
    printf("\n");

    // Free
    _zn_res_decl_free(&d_rd);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Publisher declaration ------------------*/
_zn_pub_decl_t gen_publisher_declaration(uint8_t *header)
{
    _zn_pub_decl_t e_pd;

    e_pd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_pd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_pd;
}

void assert_eq_publisher_declaration(_zn_pub_decl_t *left, _zn_pub_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void publisher_declaration(void)
{
    printf("\n>> Publisher declaration\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_pub_decl_t e_pd = gen_publisher_declaration(&e_hdr);

    // Encode
    int res = _zn_pub_decl_encode(&wbf, e_hdr, &e_pd);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_pub_decl_result_t r_pd = _zn_pub_decl_decode(&rbf, e_hdr);
    assert(r_pd.tag == _z_res_t_OK);

    _zn_pub_decl_t d_pd = r_pd.value.pub_decl;
    printf("   ");
    assert_eq_publisher_declaration(&e_pd, &d_pd, e_hdr);
    printf("\n");

    // Free
    _zn_pub_decl_free(&d_pd);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Subscriber declaration ------------------*/
_zn_sub_decl_t gen_subscriber_declaration(uint8_t *header)
{
    _zn_sub_decl_t e_sd;

    e_sd.subinfo = gen_subinfo();
    if (e_sd.subinfo.mode != zn_submode_t_PUSH || e_sd.subinfo.period)
        _ZN_SET_FLAG(*header, _ZN_FLAG_Z_S);
    if (e_sd.subinfo.reliability == zn_reliability_t_RELIABLE)
        _ZN_SET_FLAG(*header, _ZN_FLAG_Z_R);

    e_sd.key = gen_res_key();
    if (!e_sd.key.rname)
        _ZN_SET_FLAG(*header, _ZN_FLAG_Z_K);

    return e_sd;
}

void assert_eq_subscriber_declaration(_zn_sub_decl_t *left, _zn_sub_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
    if _ZN_HAS_FLAG (header, _ZN_FLAG_Z_S)
    {
        printf(", ");
        assert_eq_subinfo(&left->subinfo, &right->subinfo);
    }
}

void subscriber_declaration(void)
{
    printf("\n>> Subscriber declaration\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_sub_decl_t e_sd = gen_subscriber_declaration(&e_hdr);

    // Encode
    int res = _zn_sub_decl_encode(&wbf, e_hdr, &e_sd);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_sub_decl_result_t r_pd = _zn_sub_decl_decode(&rbf, e_hdr);
    assert(r_pd.tag == _z_res_t_OK);

    _zn_sub_decl_t d_sd = r_pd.value.sub_decl;
    printf("   ");
    assert_eq_subscriber_declaration(&e_sd, &d_sd, e_hdr);
    printf("\n");

    // Free
    _zn_sub_decl_free(&d_sd);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Queryable declaration ------------------*/
_zn_qle_decl_t gen_queryable_declaration(uint8_t *header)
{
    _zn_qle_decl_t e_qd;

    e_qd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_qd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_qd;
}

void assert_eq_queryable_declaration(_zn_qle_decl_t *left, _zn_qle_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void queryable_declaration(void)
{
    printf("\n>> Queryable declaration\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_qle_decl_t e_qd = gen_queryable_declaration(&e_hdr);

    // Encode
    int res = _zn_qle_decl_encode(&wbf, e_hdr, &e_qd);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_qle_decl_result_t r_qd = _zn_qle_decl_decode(&rbf, e_hdr);
    assert(r_qd.tag == _z_res_t_OK);

    _zn_qle_decl_t d_qd = r_qd.value.qle_decl;
    printf("   ");
    assert_eq_queryable_declaration(&e_qd, &d_qd, e_hdr);
    printf("\n");

    // Free
    _zn_qle_decl_free(&d_qd);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Forget Resource declaration ------------------*/
_zn_forget_res_decl_t gen_forget_resource_declaration(void)
{
    _zn_forget_res_decl_t e_frd;

    e_frd.rid = gen_zint();

    return e_frd;
}

void assert_eq_forget_resource_declaration(_zn_forget_res_decl_t *left, _zn_forget_res_decl_t *right)
{
    printf("RID (%zu:%zu)", left->rid, right->rid);
    assert(left->rid == right->rid);
}

void forget_resource_declaration(void)
{
    printf("\n>> Forget resource declaration\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    _zn_forget_res_decl_t e_frd = gen_forget_resource_declaration();

    // Encode
    int res = _zn_forget_res_decl_encode(&wbf, &e_frd);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_forget_res_decl_result_t r_frd = _zn_forget_res_decl_decode(&rbf);
    assert(r_frd.tag == _z_res_t_OK);

    _zn_forget_res_decl_t d_frd = r_frd.value.forget_res_decl;
    printf("   ");
    assert_eq_forget_resource_declaration(&e_frd, &d_frd);
    printf("\n");

    // Free
    // NOTE: forget_res_decl does not involve any heap allocation
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Forget Publisher declaration ------------------*/
_zn_forget_pub_decl_t gen_forget_publisher_declaration(uint8_t *header)
{
    _zn_forget_pub_decl_t e_fpd;

    e_fpd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_fpd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_fpd;
}

void assert_eq_forget_publisher_declaration(_zn_forget_pub_decl_t *left, _zn_forget_pub_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void forget_publisher_declaration(void)
{
    printf("\n>> Forget publisher declaration\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_forget_pub_decl_t e_fpd = gen_forget_publisher_declaration(&e_hdr);

    // Encode
    int res = _zn_forget_pub_decl_encode(&wbf, e_hdr, &e_fpd);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_forget_pub_decl_result_t r_fpd = _zn_forget_pub_decl_decode(&rbf, e_hdr);
    assert(r_fpd.tag == _z_res_t_OK);

    _zn_forget_pub_decl_t d_fpd = r_fpd.value.forget_pub_decl;
    printf("   ");
    assert_eq_forget_publisher_declaration(&e_fpd, &d_fpd, e_hdr);
    printf("\n");

    // Free
    _zn_forget_pub_decl_free(&d_fpd);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Forget Subscriber declaration ------------------*/
_zn_forget_sub_decl_t gen_forget_subscriber_declaration(uint8_t *header)
{
    _zn_forget_sub_decl_t e_fsd;

    e_fsd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_fsd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_fsd;
}

void assert_eq_forget_subscriber_declaration(_zn_forget_sub_decl_t *left, _zn_forget_sub_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void forget_subscriber_declaration(void)
{
    printf("\n>> Forget subscriber declaration\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_forget_sub_decl_t e_fsd = gen_forget_subscriber_declaration(&e_hdr);

    // Encode
    int res = _zn_forget_sub_decl_encode(&wbf, e_hdr, &e_fsd);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_forget_sub_decl_result_t r_fsd = _zn_forget_sub_decl_decode(&rbf, e_hdr);
    assert(r_fsd.tag == _z_res_t_OK);

    _zn_forget_sub_decl_t d_fsd = r_fsd.value.forget_sub_decl;
    printf("   ");
    assert_eq_forget_subscriber_declaration(&e_fsd, &d_fsd, e_hdr);
    printf("\n");

    // Free
    _zn_forget_sub_decl_free(&d_fsd);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Forget Queryable declaration ------------------*/
_zn_forget_qle_decl_t gen_forget_queryable_declaration(uint8_t *header)
{
    _zn_forget_qle_decl_t e_fqd;

    e_fqd.key = gen_res_key();
    _ZN_SET_FLAG(*header, (e_fqd.key.rname) ? 0 : _ZN_FLAG_Z_K);

    return e_fqd;
}

void assert_eq_forget_queryable_declaration(_zn_forget_qle_decl_t *left, _zn_forget_qle_decl_t *right, uint8_t header)
{
    assert_eq_res_key(&left->key, &right->key, header);
}

void forget_queryable_declaration(void)
{
    printf("\n>> Forget queryable declaration\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_forget_qle_decl_t e_fqd = gen_forget_queryable_declaration(&e_hdr);

    // Encode
    int res = _zn_forget_qle_decl_encode(&wbf, e_hdr, &e_fqd);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_forget_qle_decl_result_t r_fqd = _zn_forget_qle_decl_decode(&rbf, e_hdr);
    assert(r_fqd.tag == _z_res_t_OK);

    _zn_forget_qle_decl_t d_fqd = r_fqd.value.forget_qle_decl;
    printf("   ");
    assert_eq_forget_queryable_declaration(&e_fqd, &d_fqd, e_hdr);
    printf("\n");

    // Free
    _zn_forget_qle_decl_free(&d_fqd);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Declaration ------------------*/
_zn_declaration_t gen_declaration(void)
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
    d.header = decl[gen_uint8() % (sizeof(decl) / sizeof(uint8_t))];

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

void assert_eq_declaration(_zn_declaration_t *left, _zn_declaration_t *right)
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
_zn_declare_t gen_declare_message(void)
{
    _zn_declare_t e_dcl;
    e_dcl.declarations.len = gen_zint() % 16;
    e_dcl.declarations.val = (_zn_declaration_t *)malloc(sizeof(_zn_declaration_t) * e_dcl.declarations.len);

    for (z_zint_t i = 0; i < e_dcl.declarations.len; ++i)
        e_dcl.declarations.val[i] = gen_declaration();

    return e_dcl;
}

void assert_eq_declare_message(_zn_declare_t *left, _zn_declare_t *right)
{
    assert(left->declarations.len == right->declarations.len);

    for (z_zint_t i = 0; i < left->declarations.len; ++i)
    {
        printf("   ");
        assert_eq_declaration(&left->declarations.val[i], &right->declarations.val[i]);
        printf("\n");
    }
}

void declare_message(void)
{
    printf("\n>> Declare message\n");
    _z_wbuf_t wbf = gen_wbuf(512);

    // Initialize
    _zn_declare_t e_dcl = gen_declare_message();

    // Encode
    int res = _zn_declare_encode(&wbf, &e_dcl);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_declare_result_t r_dcl = _zn_declare_decode(&rbf);
    assert(r_dcl.tag == _z_res_t_OK);

    _zn_declare_t d_dcl = r_dcl.value.declare;
    assert_eq_declare_message(&e_dcl, &d_dcl);

    // Free
    _zn_declare_free(&d_dcl);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
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
    e_da.payload = gen_payload(1 + gen_zint() % 64);

    return e_da;
}

void assert_eq_data_message(_zn_data_t *left, _zn_data_t *right, uint8_t header)
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
    assert_eq_payload(&left->payload, &right->payload);
    printf("\n");
}

void data_message(void)
{
    printf("\n>> Data message\n");
    _z_wbuf_t wbf = gen_wbuf(256);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_data_t e_da = gen_data_message(&e_hdr);

    // Encode
    int res = _zn_data_encode(&wbf, e_hdr, &e_da);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_data_result_t r_da = _zn_data_decode(&rbf, e_hdr);
    assert(r_da.tag == _z_res_t_OK);

    _zn_data_t d_da = r_da.value.data;
    assert_eq_data_message(&e_da, &d_da, e_hdr);

    // Free
    _zn_data_free(&d_da);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
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

void assert_eq_pull_message(_zn_pull_t *left, _zn_pull_t *right, uint8_t header)
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

void pull_message(void)
{
    printf("\n>> Pull message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_pull_t e_pu = gen_pull_message(&e_hdr);

    // Encode
    int res = _zn_pull_encode(&wbf, e_hdr, &e_pu);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_pull_result_t r_pu = _zn_pull_decode(&rbf, e_hdr);
    assert(r_pu.tag == _z_res_t_OK);

    _zn_pull_t d_pu = r_pu.value.pull;
    assert_eq_pull_message(&e_pu, &d_pu, e_hdr);

    // Free
    _zn_pull_free(&d_pu);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
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
        e_qy.target.kind = gen_uint8();

        uint8_t tgt[] = {
            zn_target_t_BEST_MATCHING,
            zn_target_t_COMPLETE,
            zn_target_t_ALL,
            zn_target_t_NONE};
        e_qy.target.target.tag = tgt[gen_uint8() % (sizeof(tgt) / sizeof(uint8_t))];
        if (e_qy.target.target.tag == zn_target_t_COMPLETE)
            e_qy.target.target.type.complete.n = gen_uint8();

        _ZN_SET_FLAG(*header, _ZN_FLAG_Z_T);
    }

    uint8_t con[] = {
        zn_consolidation_mode_t_FULL,
        zn_consolidation_mode_t_LAZY,
        zn_consolidation_mode_t_NONE};
    e_qy.consolidation.first_routers = con[gen_uint8() % (sizeof(con) / sizeof(uint8_t))];
    e_qy.consolidation.last_router = con[gen_uint8() % (sizeof(con) / sizeof(uint8_t))];
    e_qy.consolidation.reception = con[gen_uint8() % (sizeof(con) / sizeof(uint8_t))];

    return e_qy;
}

void assert_eq_query_message(_zn_query_t *left, _zn_query_t *right, uint8_t header)
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
        printf("   Target => ");
        printf("Kind (%u:%u), ", left->target.kind, right->target.kind);
        assert(left->target.kind == right->target.kind);

        printf("Tag (%u:%u)", left->target.target.tag, right->target.target.tag);
        assert(left->target.target.tag == right->target.target.tag);

        if (left->target.target.tag == zn_target_t_COMPLETE)
        {
            printf(", N (%u:%u)", left->target.target.type.complete.n, right->target.target.type.complete.n);
            assert(left->target.target.type.complete.n == right->target.target.type.complete.n);
        }

        printf("\n");
    }

    printf("   Consolidation (%u:%u, %u:%u, %u:%u)",
           left->consolidation.first_routers, right->consolidation.first_routers,
           left->consolidation.last_router, right->consolidation.last_router,
           left->consolidation.reception, right->consolidation.reception);
    assert(left->consolidation.first_routers == right->consolidation.first_routers);
    assert(left->consolidation.last_router == right->consolidation.last_router);
    assert(left->consolidation.reception == right->consolidation.reception);
    printf("\n");
}

void query_message(void)
{
    printf("\n>> Query message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_query_t e_qy = gen_query_message(&e_hdr);

    // Encode
    int res = _zn_query_encode(&wbf, e_hdr, &e_qy);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_query_result_t r_qy = _zn_query_decode(&rbf, e_hdr);
    assert(r_qy.tag == _z_res_t_OK);

    _zn_query_t d_qy = r_qy.value.query;
    assert_eq_query_message(&e_qy, &d_qy, e_hdr);

    // Free
    _zn_query_free(&d_qy);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Zenoh message ------------------*/
_zn_zenoh_message_t *gen_zenoh_message(void)
{
    _zn_zenoh_message_t *p_zm = (_zn_zenoh_message_t *)malloc(sizeof(_zn_zenoh_message_t));
    if (gen_bool())
        p_zm->attachment = gen_attachment();
    else
        p_zm->attachment = NULL;
    if (gen_bool())
        p_zm->reply_context = gen_reply_context();
    else
        p_zm->reply_context = NULL;

    uint8_t mids[] = {
        _ZN_MID_DECLARE,
        _ZN_MID_DATA,
        _ZN_MID_PULL,
        _ZN_MID_QUERY,
        _ZN_MID_UNIT};
    uint8_t i = gen_uint8() % (sizeof(mids) / sizeof(uint8_t));
    switch (mids[i])
    {
    case _ZN_MID_DECLARE:
        p_zm->header = _ZN_MID_DECLARE;
        p_zm->body.declare = gen_declare_message();
        break;
    case _ZN_MID_DATA:
        p_zm->header = _ZN_MID_DATA;
        p_zm->body.data = gen_data_message(&p_zm->header);
        break;
    case _ZN_MID_PULL:
        p_zm->header = _ZN_MID_PULL;
        p_zm->body.pull = gen_pull_message(&p_zm->header);
        break;
    case _ZN_MID_QUERY:
        p_zm->header = _ZN_MID_QUERY;
        p_zm->body.query = gen_query_message(&p_zm->header);
        break;
    case _ZN_MID_UNIT:
        p_zm->header = _ZN_MID_UNIT;
        _ZN_SET_FLAG(p_zm->header, (gen_bool()) ? _ZN_FLAG_Z_D : 0);
        // Unit messages have no body
        break;
    default:
        assert(0);
        break;
    }

    return p_zm;
}

void assert_eq_zenoh_message(_zn_zenoh_message_t *left, _zn_zenoh_message_t *right)
{
    // Test message decorators
    if (left->attachment && right->attachment)
    {
        printf("   ");
        assert_eq_attachment(left->attachment, right->attachment);
        printf("\n");
    }
    else
    {
        printf("   Attachment: %p:%p\n", (void *)left->attachment, (void *)right->attachment);
        assert(left->attachment == right->attachment);
    }

    if (left->reply_context && right->reply_context)
    {
        printf("   ");
        assert_eq_reply_context(left->reply_context, right->reply_context);
        printf("\n");
    }
    else
    {
        printf("   Reply Context: %p:%p\n", (void *)left->reply_context, (void *)right->reply_context);
        assert(left->reply_context == right->reply_context);
    }

    // Test message
    printf("   Header (%x:%x)", left->header, right->header);
    assert(left->header == right->header);
    printf("\n");

    switch (_ZN_MID(left->header))
    {
    case _ZN_MID_DECLARE:
        assert_eq_declare_message(&left->body.declare, &right->body.declare);
        break;
    case _ZN_MID_DATA:
        assert_eq_data_message(&left->body.data, &right->body.data, left->header);
        break;
    case _ZN_MID_PULL:
        assert_eq_pull_message(&left->body.pull, &right->body.pull, left->header);
        break;
    case _ZN_MID_QUERY:
        assert_eq_query_message(&left->body.query, &right->body.query, left->header);
        break;
    case _ZN_MID_UNIT:
        // Do nothing. Unit messages have no body
        break;
    default:
        assert(0);
        break;
    }
}

void zenoh_message(void)
{
    printf("\n>> Zenoh message\n");
    _z_wbuf_t wbf = gen_wbuf(1024);

    // Initialize
    _zn_zenoh_message_t *e_zm = gen_zenoh_message();

    printf(" - ");
    switch (_ZN_MID(e_zm->header))
    {
    case _ZN_MID_DECLARE:
        printf("Declare message");
        break;
    case _ZN_MID_DATA:
        printf("Data message");
        break;
    case _ZN_MID_PULL:
        printf("Pull message");
        break;
    case _ZN_MID_QUERY:
        printf("Query message");
        break;
    case _ZN_MID_UNIT:
        printf("Unit message");
        break;
    default:
        assert(0);
        break;
    }
    if (e_zm->attachment)
    {
        printf("   Attachment\n");
        print_attachment(e_zm->attachment);
        printf("\n");
    }
    if (e_zm->reply_context)
    {
        printf("   Reply Context\n");
        print_reply_context(e_zm->reply_context);
        printf("\n");
    }
    printf("\n");

    // Encode
    int res = _zn_zenoh_message_encode(&wbf, e_zm);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_zenoh_message_p_result_t r_zm = _zn_zenoh_message_decode(&rbf);
    assert(r_zm.tag == _z_res_t_OK);

    _zn_zenoh_message_t *d_zm = r_zm.value.zenoh_message;
    assert_eq_zenoh_message(e_zm, d_zm);

    // Free
    free(e_zm);
    _zn_zenoh_message_free(d_zm);
    _zn_zenoh_message_p_result_free(&r_zm);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*=============================*/
/*       Session Messages      */
/*=============================*/
/*------------------ Scout Message ------------------*/
_zn_scout_t gen_scout_message(uint8_t *header)
{
    _zn_scout_t e_sc;

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

void assert_eq_scout_message(_zn_scout_t *left, _zn_scout_t *right, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_W)
    {
        printf("   What (%zu:%zu)\n", left->what, right->what);
        assert(left->what == right->what);
    }
}

void scout_message(void)
{
    printf("\n>> Scout message\n");
    _z_wbuf_t wbf = gen_wbuf(1024);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_scout_t e_sc = gen_scout_message(&e_hdr);

    // Encode
    int res = _zn_scout_encode(&wbf, e_hdr, &e_sc);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_scout_result_t r_sc = _zn_scout_decode(&rbf, e_hdr);
    assert(r_sc.tag == _z_res_t_OK);

    _zn_scout_t d_sc = r_sc.value.scout;
    assert_eq_scout_message(&e_sc, &d_sc, e_hdr);

    // Free
    // NOTE: scout does not involve any heap allocation
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Hello Message ------------------*/
_zn_hello_t gen_hello_message(uint8_t *header)
{
    _zn_hello_t e_he;

    if (gen_bool())
    {
        e_he.pid = gen_bytes(16);
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_I);
    }
    if (gen_bool())
    {
        e_he.whatami = gen_zint();
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_W);
    }
    if (gen_bool())
    {
        e_he.locators = gen_str_array((gen_uint8() % 4) + 1);
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_L);
    }

    return e_he;
}

void assert_eq_hello_message(_zn_hello_t *left, _zn_hello_t *right, uint8_t header)
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
        printf("   ");
        assert_eq_str_array(&left->locators, &right->locators);
        printf("\n");
    }
}

void hello_message(void)
{
    printf("\n>> Hello message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_hello_t e_he = gen_hello_message(&e_hdr);

    // Encode
    int res = _zn_hello_encode(&wbf, e_hdr, &e_he);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_hello_result_t r_he = _zn_hello_decode(&rbf, e_hdr);
    assert(r_he.tag == _z_res_t_OK);

    _zn_hello_t d_he = r_he.value.hello;
    assert_eq_hello_message(&e_he, &d_he, e_hdr);

    // Free
    _zn_hello_free(&d_he, e_hdr);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Open Message ------------------*/
_zn_open_t gen_open_message(uint8_t *header)
{
    _zn_open_t e_op;

    e_op.version = gen_uint8();
    e_op.whatami = gen_zint();
    e_op.opid = gen_bytes(16);
    e_op.lease = gen_zint();
    e_op.initial_sn = gen_zint();
    if (gen_bool())
    {
        e_op.sn_resolution = gen_zint();
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_S);
    }
    if (gen_bool())
    {
        e_op.locators = gen_str_array((gen_uint8() % 4) + 1);
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_L);
    }

    return e_op;
}

void assert_eq_open_message(_zn_open_t *left, _zn_open_t *right, uint8_t header)
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

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_S)
    {
        printf("   SN Resolution (%zu:%zu)", left->sn_resolution, right->sn_resolution);
        assert(left->sn_resolution == right->sn_resolution);
        printf("\n");
    }

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
    {
        printf("   ");
        assert_eq_str_array(&left->locators, &right->locators);
        printf("\n");
    }
}

void open_message(void)
{
    printf("\n>> Open message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_open_t e_op = gen_open_message(&e_hdr);

    // Encode
    int res = _zn_open_encode(&wbf, e_hdr, &e_op);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_open_result_t r_op = _zn_open_decode(&rbf, e_hdr);
    assert(r_op.tag == _z_res_t_OK);

    _zn_open_t d_op = r_op.value.open;
    assert_eq_open_message(&e_op, &d_op, e_hdr);

    // Free
    _zn_open_free(&d_op, e_hdr);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Accept Message ------------------*/
_zn_accept_t gen_accept_message(uint8_t *header)
{
    _zn_accept_t e_ac;

    e_ac.whatami = gen_zint();
    e_ac.opid = gen_bytes(16);
    e_ac.apid = gen_bytes(16);
    e_ac.initial_sn = gen_zint();
    if (gen_bool())
    {
        e_ac.sn_resolution = gen_zint();
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_S);
    }
    if (gen_bool())
    {
        e_ac.locators = gen_str_array((gen_uint8() % 4) + 1);
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_L);
    }

    return e_ac;
}

void assert_eq_accept_message(_zn_accept_t *left, _zn_accept_t *right, uint8_t header)
{
    printf("   WhatAmI (%zu:%zu)", left->whatami, right->whatami);
    assert(left->whatami == right->whatami);
    printf("\n");

    printf("   ");
    assert_eq_uint8_array(&left->opid, &right->opid);
    printf("\n");

    printf("   ");
    assert_eq_uint8_array(&left->apid, &right->apid);
    printf("\n");

    printf("   LEase (%zu:%zu)", left->lease, right->lease);
    assert(left->lease == right->lease);
    printf("\n");

    printf("   Initial SN (%zu:%zu)", left->initial_sn, right->initial_sn);
    assert(left->initial_sn == right->initial_sn);
    printf("\n");

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_S)
    {
        printf("   SN Resolution (%zu:%zu)", left->sn_resolution, right->sn_resolution);
        assert(left->sn_resolution == right->sn_resolution);
        printf("\n");
    }

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_L)
    {
        printf("   ");
        assert_eq_str_array(&left->locators, &right->locators);
        printf("\n");
    }
}

void accept_message(void)
{
    printf("\n>> Accept message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_accept_t e_ac = gen_accept_message(&e_hdr);

    // Encode
    int res = _zn_accept_encode(&wbf, e_hdr, &e_ac);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_accept_result_t r_ac = _zn_accept_decode(&rbf, e_hdr);
    assert(r_ac.tag == _z_res_t_OK);

    _zn_accept_t d_ac = r_ac.value.accept;
    assert_eq_accept_message(&e_ac, &d_ac, e_hdr);

    // Free
    _zn_accept_free(&d_ac, e_hdr);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Close Message ------------------*/
_zn_close_t gen_close_message(uint8_t *header)
{
    _zn_close_t e_cl;

    _ZN_SET_FLAG(*header, (gen_bool()) ? _ZN_FLAG_S_K : 0);
    if (gen_bool())
    {
        e_cl.pid = gen_bytes(16);
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_I);
    }
    e_cl.reason = gen_uint8();

    return e_cl;
}

void assert_eq_close_message(_zn_close_t *left, _zn_close_t *right, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        printf("   ");
        assert_eq_uint8_array(&left->pid, &right->pid);
        printf("\n");
    }

    printf("   Reason (%u:%u)", left->reason, right->reason);
    assert(left->reason == right->reason);
    printf("\n");
}

void close_message(void)
{
    printf("\n>> Close message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_close_t e_cl = gen_close_message(&e_hdr);

    // Encode
    int res = _zn_close_encode(&wbf, e_hdr, &e_cl);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_close_result_t r_cl = _zn_close_decode(&rbf, e_hdr);
    assert(r_cl.tag == _z_res_t_OK);

    _zn_close_t d_cl = r_cl.value.close;
    assert_eq_close_message(&e_cl, &d_cl, e_hdr);

    // Free
    _zn_close_free(&d_cl, e_hdr);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Sync Message ------------------*/
_zn_sync_t gen_sync_message(uint8_t *header)
{
    _zn_sync_t e_sy;

    _ZN_SET_FLAG(*header, (gen_bool()) ? _ZN_FLAG_S_R : 0);
    e_sy.sn = gen_zint();
    if _ZN_HAS_FLAG (*header, _ZN_FLAG_S_R)
    {
        if (gen_bool())
        {
            e_sy.count = gen_zint();
            _ZN_SET_FLAG(*header, _ZN_FLAG_S_C);
        }
    }

    return e_sy;
}

void assert_eq_sync_message(_zn_sync_t *left, _zn_sync_t *right, uint8_t header)
{
    printf("   SN (%zu:%zu)", left->sn, right->sn);
    assert(left->sn == right->sn);
    printf("\n");

    if (_ZN_HAS_FLAG(header, _ZN_FLAG_S_R) && _ZN_HAS_FLAG(header, _ZN_FLAG_S_C))
    {
        printf("   Count (%zu:%zu)", left->count, right->count);
        assert(left->count == right->count);
        printf("\n");
    }
}

void sync_message(void)
{
    printf("\n>> Sync message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_sync_t e_sy = gen_sync_message(&e_hdr);

    // Encode
    int res = _zn_sync_encode(&wbf, e_hdr, &e_sy);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_sync_result_t r_sy = _zn_sync_decode(&rbf, e_hdr);
    assert(r_sy.tag == _z_res_t_OK);

    _zn_sync_t d_sy = r_sy.value.sync;
    assert_eq_sync_message(&e_sy, &d_sy, e_hdr);

    // Free
    // NOTE: sync does not involve any heap allocation
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ AckNack Message ------------------*/
_zn_ack_nack_t gen_ack_nack_message(uint8_t *header)
{
    _zn_ack_nack_t e_an;

    e_an.sn = gen_zint();
    if (gen_bool())
    {
        e_an.mask = gen_zint();
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_M);
    }

    return e_an;
}

void assert_eq_ack_nack_message(_zn_ack_nack_t *left, _zn_ack_nack_t *right, uint8_t header)
{
    printf("   SN (%zu:%zu)", left->sn, right->sn);
    assert(left->sn == right->sn);
    printf("\n");

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_M)
    {
        printf("   Mask (%zu:%zu)", left->mask, right->mask);
        assert(left->mask == right->mask);
        printf("\n");
    }
}

void ack_nack_message(void)
{
    printf("\n>> AckNack message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_ack_nack_t e_an = gen_ack_nack_message(&e_hdr);

    // Encode
    int res = _zn_ack_nack_encode(&wbf, e_hdr, &e_an);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_ack_nack_result_t r_an = _zn_ack_nack_decode(&rbf, e_hdr);
    assert(r_an.tag == _z_res_t_OK);

    _zn_ack_nack_t d_an = r_an.value.ack_nack;
    assert_eq_ack_nack_message(&e_an, &d_an, e_hdr);

    // Free
    // NOTE: ack_nack does not involve any heap allocation
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ KeepAlive Message ------------------*/
_zn_keep_alive_t gen_keep_alive_message(uint8_t *header)
{
    _zn_keep_alive_t e_ka;

    if (gen_bool())
    {
        e_ka.pid = gen_bytes(16);
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_I);
    }
    else
    {
        // NOTE: this should not be needed. However, the compiler complains
        // that the variable might be used initialized. Initialize it to 0.
        e_ka.pid = gen_bytes(0);
    }

    return e_ka;
}

void assert_eq_keep_alive_message(_zn_keep_alive_t *left, _zn_keep_alive_t *right, uint8_t header)
{
    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_I)
    {
        printf("   ");
        assert_eq_uint8_array(&left->pid, &right->pid);
        printf("\n");
    }
}

void keep_alive_message(void)
{
    printf("\n>> KeepAlive message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_keep_alive_t e_ka = gen_keep_alive_message(&e_hdr);

    // Encode
    int res = _zn_keep_alive_encode(&wbf, e_hdr, &e_ka);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_keep_alive_result_t r_ka = _zn_keep_alive_decode(&rbf, e_hdr);
    assert(r_ka.tag == _z_res_t_OK);

    _zn_keep_alive_t d_ka = r_ka.value.keep_alive;
    assert_eq_keep_alive_message(&e_ka, &d_ka, e_hdr);

    // Free
    _zn_keep_alive_free(&d_ka, e_hdr);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ PingPong Message ------------------*/
_zn_ping_pong_t gen_ping_pong_message(uint8_t *header)
{
    _zn_ping_pong_t e_pp;

    _ZN_SET_FLAG(*header, (gen_bool()) ? _ZN_FLAG_S_P : 0);
    e_pp.hash = gen_zint();

    return e_pp;
}

void assert_eq_ping_pong_message(_zn_ping_pong_t *left, _zn_ping_pong_t *right)
{
    printf("   Hash (%zu:%zu)", left->hash, right->hash);
    assert(left->hash == right->hash);
    printf("\n");
}

void ping_pong_message(void)
{
    printf("\n>> PingPong message\n");
    _z_wbuf_t wbf = gen_wbuf(128);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_ping_pong_t e_pp = gen_ping_pong_message(&e_hdr);

    // Encode
    int res = _zn_ping_pong_encode(&wbf, &e_pp);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_ping_pong_result_t r_pp = _zn_ping_pong_decode(&rbf);
    assert(r_pp.tag == _z_res_t_OK);

    _zn_ping_pong_t d_pp = r_pp.value.ping_pong;
    assert_eq_ping_pong_message(&e_pp, &d_pp);

    // Free
    // NOTE: ping_pong does not involve any heap allocation
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Frame Message ------------------*/
_zn_frame_t gen_frame_message(uint8_t *header, int can_be_fragment)
{
    _zn_frame_t e_fr;

    e_fr.sn = gen_zint();

    if (can_be_fragment && gen_bool())
    {
        _ZN_SET_FLAG(*header, _ZN_FLAG_S_F);
        _ZN_SET_FLAG(*header, (gen_bool()) ? _ZN_FLAG_S_E : 0);
        e_fr.payload.fragment = gen_payload(64);
    }
    else
    {
        z_zint_t num = (gen_zint() % 4) + 1;
        e_fr.payload.messages = _z_vec_make(num);
        for (z_zint_t i = 0; i < num; ++i)
        {
            _zn_zenoh_message_t *p_zm = gen_zenoh_message();
            _z_vec_append(&e_fr.payload.messages, p_zm);
        }
    }

    return e_fr;
}

void assert_eq_frame_message(_zn_frame_t *left, _zn_frame_t *right, uint8_t header)
{
    printf("   SN (%zu:%zu)", left->sn, right->sn);
    assert(left->sn == right->sn);
    printf("\n");

    if _ZN_HAS_FLAG (header, _ZN_FLAG_S_F)
    {
        printf("   ");
        assert_eq_payload(&left->payload.fragment, &right->payload.fragment);
        printf("\n");
    }
    else
    {
        size_t l_len = _z_vec_len(&left->payload.messages);
        size_t r_len = _z_vec_len(&right->payload.messages);
        printf("   Lenght (%zu:%zu)", l_len, r_len);
        assert(r_len == r_len);

        for (size_t i = 0; i < l_len; ++i)
            assert_eq_zenoh_message((_zn_zenoh_message_t *)_z_vec_get(&left->payload.messages, i), (_zn_zenoh_message_t *)_z_vec_get(&right->payload.messages, i));
    }
}

void frame_message(void)
{
    printf("\n>> Frame message\n");
    _z_wbuf_t wbf = gen_wbuf(1024);

    // Initialize
    uint8_t e_hdr = 0;
    _zn_frame_t e_fr = gen_frame_message(&e_hdr, 1);

    // Encode
    int res = _zn_frame_encode(&wbf, e_hdr, &e_fr);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_frame_result_t r_fr = _zn_frame_decode(&rbf, e_hdr);
    assert(r_fr.tag == _z_res_t_OK);

    _zn_frame_t d_fr = r_fr.value.frame;
    assert_eq_frame_message(&e_fr, &d_fr, e_hdr);

    // Frame
    _zn_frame_free(&d_fr, e_hdr);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Session Message ------------------*/
_zn_session_message_t *gen_session_message(int can_be_fragment)
{
    _zn_session_message_t *p_sm = (_zn_session_message_t *)malloc(sizeof(_zn_session_message_t));

    if (gen_bool())
        p_sm->attachment = gen_attachment();
    else
        p_sm->attachment = NULL;

    uint8_t mids[] = {
        _ZN_MID_SCOUT,
        _ZN_MID_HELLO,
        _ZN_MID_OPEN,
        _ZN_MID_ACCEPT,
        _ZN_MID_CLOSE,
        _ZN_MID_SYNC,
        _ZN_MID_ACK_NACK,
        _ZN_MID_KEEP_ALIVE,
        _ZN_MID_PING_PONG,
        _ZN_MID_FRAME,
    };
    uint8_t i = gen_uint8() % (sizeof(mids) / sizeof(uint8_t));
    switch (mids[i])
    {
    case _ZN_MID_SCOUT:
        p_sm->header = _ZN_MID_SCOUT;
        p_sm->body.scout = gen_scout_message(&p_sm->header);
        break;
    case _ZN_MID_HELLO:
        p_sm->header = _ZN_MID_HELLO;
        p_sm->body.hello = gen_hello_message(&p_sm->header);
        break;
    case _ZN_MID_OPEN:
        p_sm->header = _ZN_MID_OPEN;
        p_sm->body.open = gen_open_message(&p_sm->header);
        break;
    case _ZN_MID_ACCEPT:
        p_sm->header = _ZN_MID_ACCEPT;
        p_sm->body.accept = gen_accept_message(&p_sm->header);
        break;
    case _ZN_MID_CLOSE:
        p_sm->header = _ZN_MID_CLOSE;
        p_sm->body.close = gen_close_message(&p_sm->header);
        break;
    case _ZN_MID_SYNC:
        p_sm->header = _ZN_MID_SYNC;
        p_sm->body.sync = gen_sync_message(&p_sm->header);
        break;
    case _ZN_MID_ACK_NACK:
        p_sm->header = _ZN_MID_ACK_NACK;
        p_sm->body.ack_nack = gen_ack_nack_message(&p_sm->header);
        break;
    case _ZN_MID_KEEP_ALIVE:
        p_sm->header = _ZN_MID_KEEP_ALIVE;
        p_sm->body.keep_alive = gen_keep_alive_message(&p_sm->header);
        break;
    case _ZN_MID_PING_PONG:
        p_sm->header = _ZN_MID_PING_PONG;
        p_sm->body.ping_pong = gen_ping_pong_message(&p_sm->header);
        break;
    case _ZN_MID_FRAME:
        p_sm->header = _ZN_MID_FRAME;
        p_sm->body.frame = gen_frame_message(&p_sm->header, can_be_fragment);
        break;
    default:
        assert(0);
        break;
    }

    return p_sm;
}

void assert_eq_session_message(_zn_session_message_t *left, _zn_session_message_t *right)
{
    // Test message decorators
    if (left->attachment && right->attachment)
    {
        printf("   ");
        assert_eq_attachment(left->attachment, right->attachment);
        printf("\n");
    }
    else
        assert(left->attachment == right->attachment);

    // Test message
    printf("   Header (%x:%x)", left->header, right->header);
    assert(left->header == right->header);
    printf("\n");

    switch (_ZN_MID(left->header))
    {
    case _ZN_MID_SCOUT:
        assert_eq_scout_message(&left->body.scout, &right->body.scout, left->header);
        break;
    case _ZN_MID_HELLO:
        assert_eq_hello_message(&left->body.hello, &right->body.hello, left->header);
        break;
    case _ZN_MID_OPEN:
        assert_eq_open_message(&left->body.open, &right->body.open, left->header);
        break;
    case _ZN_MID_ACCEPT:
        assert_eq_accept_message(&left->body.accept, &right->body.accept, left->header);
        break;
    case _ZN_MID_CLOSE:
        assert_eq_close_message(&left->body.close, &right->body.close, left->header);
        break;
    case _ZN_MID_SYNC:
        assert_eq_sync_message(&left->body.sync, &right->body.sync, left->header);
        break;
    case _ZN_MID_ACK_NACK:
        assert_eq_ack_nack_message(&left->body.ack_nack, &right->body.ack_nack, left->header);
        break;
    case _ZN_MID_KEEP_ALIVE:
        assert_eq_keep_alive_message(&left->body.keep_alive, &right->body.keep_alive, left->header);
        break;
    case _ZN_MID_PING_PONG:
        assert_eq_ping_pong_message(&left->body.ping_pong, &right->body.ping_pong);
        break;
    case _ZN_MID_FRAME:
        assert_eq_frame_message(&left->body.frame, &right->body.frame, left->header);
        break;
    default:
        assert(0);
        break;
    }
}

void session_message(void)
{
    printf("\n>> Session message\n");
    _z_wbuf_t wbf = gen_wbuf(1024);

    // Initialize
    _zn_session_message_t *e_sm = gen_session_message(1);
    printf(" - ");
    print_session_message_type(e_sm->header);
    printf("\n");

    // Encode
    int res = _zn_session_message_encode(&wbf, e_sm);
    assert(res == 0);

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    _zn_session_message_p_result_t r_zm = _zn_session_message_decode(&rbf);
    assert(r_zm.tag == _z_res_t_OK);

    _zn_session_message_t *d_sm = r_zm.value.session_message;
    assert_eq_session_message(e_sm, d_sm);

    // Free
    free(e_sm);
    _zn_session_message_free(d_sm);
    _zn_session_message_p_result_free(&r_zm);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Batch ------------------*/
void batch(void)
{
    printf("\n>> Batch\n");
    uint8_t bef_num = (gen_uint8() % 3);
    uint8_t frm_num = 2 + (gen_uint8() % 3);
    uint8_t aft_num = (gen_uint8() % 3);
    uint8_t tot_num = bef_num + frm_num + aft_num;
    _z_wbuf_t wbf = gen_wbuf(tot_num * 1024);

    // Initialize
    _zn_session_message_t **e_sm = (_zn_session_message_t **)malloc(tot_num * sizeof(_zn_session_message_t *));
    for (uint8_t i = 0; i < bef_num; ++i)
    {
        // Initialize random session message
        e_sm[i] = gen_session_message(0);
        // Encode
        int res = _zn_session_message_encode(&wbf, e_sm[i]);
        assert(res == 0);
    }
    for (uint8_t i = bef_num; i < bef_num + frm_num; ++i)
    {
        // Initialize random session message
        e_sm[i] = gen_session_message(0);
        // Override it with a frame message
        e_sm[i]->header = _ZN_MID_FRAME;
        e_sm[i]->body.frame = gen_frame_message(&e_sm[i]->header, 0);
        // Encode
        int res = _zn_session_message_encode(&wbf, e_sm[i]);
        assert(res == 0);
    }
    for (uint8_t i = bef_num + frm_num; i < bef_num + frm_num + aft_num; ++i)
    {
        // Initialize random session message
        e_sm[i] = gen_session_message(0);
        // Encode
        int res = _zn_session_message_encode(&wbf, e_sm[i]);
        assert(res == 0);
    }

    // Decode
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);
    for (uint8_t i = 0; i < tot_num; ++i)
    {
        _zn_session_message_p_result_t r_sm = _zn_session_message_decode(&rbf);
        assert(r_sm.tag == _z_res_t_OK);

        _zn_session_message_t *d_sm = r_sm.value.session_message;
        printf(" - ");
        print_session_message_type(d_sm->header);
        printf("\n");
        assert_eq_session_message(e_sm[i], d_sm);

        // Free
        _zn_session_message_free(d_sm);
        _zn_session_message_p_result_free(&r_sm);
        free(e_sm[i]);
    }

    free(e_sm);
    _z_rbuf_free(&rbf);
    _z_wbuf_free(&wbf);
}

/*------------------ Fragmentation ------------------*/
_zn_session_message_t _zn_frame_header(int is_reliable, int is_fragment, int is_final, z_zint_t sn)
{
    // Create the frame session message that carries the zenoh message
    _zn_session_message_t s_msg = _zn_session_message_init(_ZN_MID_FRAME);
    s_msg.body.frame.sn = sn;

    if (is_reliable)
        _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_R);

    if (is_fragment)
    {
        _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_F);

        if (is_final)
            _ZN_SET_FLAG(s_msg.header, _ZN_FLAG_S_E);

        // Do not add the payload
        s_msg.body.frame.payload.fragment.len = 0;
        s_msg.body.frame.payload.fragment.val = NULL;
    }
    else
    {
        // Do not allocate the vector containing the messages
        s_msg.body.frame.payload.messages._capacity = 0;
        s_msg.body.frame.payload.messages._len = 0;
        s_msg.body.frame.payload.messages._val = NULL;
    }

    return s_msg;
}

void _zn_wbuf_prepare(_z_wbuf_t *wbf)
{
    // Clear the buffer for serialization
    _z_wbuf_clear(wbf);

    for (size_t i = 0; i < _z_wbuf_space_left(wbf); i++)
        _z_wbuf_put(wbf, 0xff, i);
}

int _zn_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, int is_reliable, size_t sn)
{
    // Assume first that this is not the final fragment
    int is_final = 0;
    do
    {
        // Mark the buffer for the writing operation
        size_t w_pos = _z_wbuf_get_wpos(dst);
        // Get the frame header
        _zn_session_message_t f_hdr = _zn_frame_header(is_reliable, 1, is_final, sn);
        // Encode the frame header
        int res = _zn_session_message_encode(dst, &f_hdr);
        if (res == 0)
        {
            size_t space_left = _z_wbuf_space_left(dst);
            size_t bytes_left = _z_wbuf_len(src);
            // Check if it is really the final fragment
            if (!is_final && (bytes_left <= space_left))
            {
                // Revert the buffer
                _z_wbuf_set_wpos(dst, w_pos);
                // It is really the finally fragment, reserialize the header
                is_final = 1;
                continue;
            }
            // Write the fragment
            size_t to_copy = bytes_left <= space_left ? bytes_left : space_left;
            printf("  -Bytes left: %zu, Space left: %zu, Fragment size: %zu, Is final: %d\n", bytes_left, space_left, to_copy, is_final);
            return _z_wbuf_copy_into(dst, src, to_copy);
        }
        else
        {
            return 0;
        }
    } while (1);
}

void fragmentation(void)
{
    printf("\n>> Fragmentation\n");
    size_t len = 16;
    _z_wbuf_t wbf = _z_wbuf_make(len, 0);
    _z_wbuf_t fbf = _z_wbuf_make(len, 1);
    _z_wbuf_t dbf = _z_wbuf_make(0, 1);

    _zn_zenoh_message_t *e_zm;

    do
    {
        _z_wbuf_clear(&fbf);
        // Generate and serialize the message
        e_zm = gen_zenoh_message();
        _zn_zenoh_message_encode(&fbf, e_zm);
        // Check that the message actually requires fragmentation
        if (_z_wbuf_len(&fbf) <= len)
            _zn_zenoh_message_free(e_zm);
        else
            break;
    } while (1);

    // Fragment the message
    int is_reliable = gen_bool();
    // Fix the sn resoulution to 128 in such a way it requires only 1 byte for encoding
    size_t sn_resolution = 128;
    size_t sn = sn_resolution - 1;

    printf(" - Message serialized\n");
    print_wbuf(&fbf);

    printf(" - Start fragmenting\n");
    int res = 0;
    while (_z_wbuf_len(&fbf) > 0)
    {
        // Clear the buffer for serialization
        _zn_wbuf_prepare(&wbf);

        // Get the fragment sequence number
        sn = (sn + 1) % sn_resolution;

        size_t written = _z_wbuf_len(&fbf);
        res = _zn_serialize_zenoh_fragment(&wbf, &fbf, is_reliable, sn);
        assert(res == 0);
        written -= _z_wbuf_len(&fbf);

        printf("  -Encoded Fragment: ");
        print_wbuf(&wbf);

        // Decode the message
        _z_rbuf_t rbf = _z_wbuf_to_rbuf(&wbf);

        _zn_session_message_p_result_t r_sm = _zn_session_message_decode(&rbf);
        assert(r_sm.tag == _z_res_t_OK);

        z_bytes_t fragment = r_sm.value.session_message->body.frame.payload.fragment;
        printf("  -Decoded Fragment length: %zu\n", fragment.len);
        assert(fragment.len == written);

        // Create an iosli for decoding
        _z_wbuf_add_iosli_from(&dbf, fragment.val, fragment.len);

        print_wbuf(&dbf);

        // Free the read buffer
        _z_rbuf_free(&rbf);
    }

    printf(" - Start defragmenting\n");
    print_wbuf(&dbf);
    _z_rbuf_t rbf = _z_wbuf_to_rbuf(&dbf);
    printf("   Defragmented: ");
    print_iosli(&rbf.ios);
    printf("\n");
    _zn_zenoh_message_p_result_t r_sm = _zn_zenoh_message_decode(&rbf);
    assert(r_sm.tag == _z_res_t_OK);
    _zn_zenoh_message_t *d_zm = r_sm.value.zenoh_message;
    assert_eq_zenoh_message(e_zm, d_zm);

    _z_wbuf_free(&dbf);
    _z_wbuf_free(&wbf);
    _z_wbuf_free(&fbf);
}

/*=============================*/
/*            Main             */
/*=============================*/
int main(void)
{
    setbuf(stdout, NULL);

    for (unsigned int i = 0; i < RUNS; ++i)
    {
        printf("\n\n== RUN %u", i);
        // Message fields
        payload_field();
        timestamp_field();
        subinfo_field();
        res_key_field();
        data_info_field();
        // Message decorators
        attachment_decorator();
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
        accept_message();
        close_message();
        sync_message();
        ack_nack_message();
        keep_alive_message();
        ping_pong_message();
        frame_message();
        session_message();
        batch();
        fragmentation();
    }

    return 0;
}
