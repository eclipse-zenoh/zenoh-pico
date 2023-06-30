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

#include <stdbool.h>
#include <string.h>

#include "zenoh-pico/protocol/core.h"
#define ZENOH_PICO_TEST_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/extcodec.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/system/platform.h"

#define RUNS 1000

#define _Z_MOCK_EXTENSION_UNIT 0x01
#define _Z_MOCK_EXTENSION_ZINT 0x02
#define _Z_MOCK_EXTENSION_ZBUF 0x03

/*=============================*/
/*       Helper functions      */
/*=============================*/
void print_iosli(_z_iosli_t *ios) {
    printf("IOSli: Capacity: %zu, Rpos: %zu, Wpos: %zu, Buffer: [", ios->_capacity, ios->_r_pos, ios->_w_pos);
    for (size_t i = ios->_r_pos; i < ios->_w_pos; i++) {
        printf("%02x", ios->_buf[i]);
        if (i < ios->_w_pos - ios->_r_pos - 1) printf(" ");
    }
    printf("]");
}

void print_wbuf(_z_wbuf_t *wbf) {
    printf("   WBuf: %zu, RIdx: %zu, WIdx: %zu\n", _z_wbuf_len_iosli(wbf), wbf->_r_idx, wbf->_w_idx);
    for (size_t i = 0; i < _z_wbuf_len_iosli(wbf); i++) {
        printf("   - ");
        print_iosli(_z_wbuf_get_iosli(wbf, i));
        printf("\n");
    }
}

void print_zbuf(_z_zbuf_t *zbf) { print_iosli(&zbf->_ios); }

void print_uint8_array(_z_bytes_t *arr) {
    printf("Length: %zu, Buffer: [", arr->len);
    for (size_t i = 0; i < arr->len; i++) {
        printf("%02x", arr->start[i]);
        if (i < arr->len - 1) printf(" ");
    }
    printf("]");
}

void print_transport_message_type(uint8_t header) {
    switch (_Z_MID(header)) {
        case _Z_MID_T_JOIN:
            printf("Join message");
            break;
        case _Z_MID_T_INIT:
            printf("Init message");
            break;
        case _Z_MID_T_OPEN:
            printf("Open message");
            break;
        case _Z_MID_T_CLOSE:
            printf("Close message");
            break;
        case _Z_MID_T_KEEP_ALIVE:
            printf("KeepAlive message");
            break;
        case _Z_MID_T_FRAME:
            printf("Frame message");
            break;
        case _Z_MID_T_FRAGMENT:
            printf("Frame message");
            break;
        default:
            assert(0);
            break;
    }
}

void print_scouting_message_type(uint8_t header) {
    switch (_Z_MID(header)) {
        case _Z_MID_SCOUT:
            printf("Scout message");
            break;
        case _Z_MID_HELLO:
            printf("Hello message");
            break;
        default:
            assert(0);
            break;
    }
}

/*=============================*/
/*    Generating functions     */
/*=============================*/
_Bool gen_bool(void) { return z_random_u8() % 2; }

uint8_t gen_uint8(void) { return z_random_u8(); }

uint8_t gen_uint16(void) { return z_random_u16(); }

uint64_t gen_uint64(void) {
    uint64_t ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

unsigned int gen_uint(void) {
    unsigned int ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

_z_zint_t gen_zint(void) {
    _z_zint_t ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

_z_wbuf_t gen_wbuf(size_t len) {
    _Bool is_expandable = false;

    if (gen_bool()) {
        is_expandable = true;
        len = 1 + (gen_zint() % len);
    }

    return _z_wbuf_make(len, is_expandable);
}

_z_bytes_t gen_payload(size_t len) {
    _z_bytes_t pld;
    pld._is_alloc = true;
    pld.len = len;
    pld.start = (uint8_t *)z_malloc(len);
    z_random_fill((uint8_t *)pld.start, pld.len);

    return pld;
}

_z_bytes_t gen_bytes(size_t len) {
    _z_bytes_t arr;
    arr._is_alloc = true;
    arr.len = len;
    arr.start = NULL;
    if (len == 0) return arr;

    arr.start = (uint8_t *)z_malloc(sizeof(uint8_t) * len);
    for (_z_zint_t i = 0; i < len; i++) ((uint8_t *)arr.start)[i] = gen_uint8();

    return arr;
}

_z_id_t gen_zid(void) {
    _z_id_t id = _z_id_empty();
    uint8_t hash = 55;
    uint8_t len = gen_uint8() % 16;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = gen_uint8();
        id.id[i] = byte;
        hash ^= byte;
    }
    id.id[0] = hash;
    return id;
}

char *gen_str(size_t size) {
    char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *str = (char *)z_malloc(size + 1);
    for (_z_zint_t i = 0; i < size; i++) {
        uint32_t key = z_random_u32() % (sizeof(charset) - 1);
        str[i] = charset[key];
    }
    str[size] = '\0';
    return str;
}

_z_str_array_t gen_str_array(size_t size) {
    _z_str_array_t sa = _z_str_array_make(size);
    for (size_t i = 0; i < size; i++) ((char **)sa.val)[i] = gen_str(16);

    return sa;
}

_z_locator_array_t gen_locator_array(size_t size) {
    _z_locator_array_t la = _z_locator_array_make(size);
    for (size_t i = 0; i < size; i++) {
        _z_locator_t *val = &la._val[i];
        val->_protocol = gen_str(3);
        val->_address = gen_str(12);
        val->_metadata = _z_str_intmap_make();  // @TODO: generate metadata
    }

    return la;
}

_z_value_t gen_value(void) {
    _z_value_t val;
    val.encoding.prefix = gen_zint();
    if (gen_bool()) {
        val.encoding.suffix = gen_bytes(8);
    } else {
        val.encoding.suffix = _z_bytes_empty();
    }

    if (gen_bool()) {
        val.payload = _z_bytes_empty();
    } else {
        val.payload = gen_bytes(16);
    }

    return val;
}

/*=============================*/
/*     Asserting functions     */
/*=============================*/
void assert_eq_iosli(_z_iosli_t *left, _z_iosli_t *right) {
    printf("IOSli -> ");
    printf("Capacity (%zu:%zu), ", left->_capacity, right->_capacity);

    assert(left->_capacity == right->_capacity);

    printf("Content (");
    for (_z_zint_t i = 0; i < left->_capacity; i++) {
        uint8_t l = left->_buf[i];
        uint8_t r = right->_buf[i];

        printf("%02x:%02x", l, r);
        if (i < left->_capacity - 1) printf(" ");

        assert(l == r);
    }
    printf(")");
}

void assert_eq_uint8_array(_z_bytes_t *left, _z_bytes_t *right) {
    printf("Array -> ");
    printf("Length (%zu:%zu), ", left->len, right->len);

    assert(left->len == right->len);
    printf("Content (");
    for (size_t i = 0; i < left->len; i++) {
        uint8_t l = left->start[i];
        uint8_t r = right->start[i];

        printf("%02x:%02x", l, r);
        if (i < left->len - 1) printf(" ");

        assert(l == r);
    }
    printf(")");
}

void assert_eq_str_array(_z_str_array_t *left, _z_str_array_t *right) {
    printf("Array -> ");
    printf("Length (%zu:%zu), ", left->len, right->len);

    assert(left->len == right->len);
    printf("Content (");
    for (size_t i = 0; i < left->len; i++) {
        const char *l = left->val[i];
        const char *r = right->val[i];

        printf("%s:%s", l, r);
        if (i < left->len - 1) printf(" ");

        assert(_z_str_eq(l, r) == true);
    }
    printf(")");
}

void assert_eq_locator_array(_z_locator_array_t *left, _z_locator_array_t *right) {
    printf("Locators -> ");
    printf("Length (%zu:%zu), ", left->_len, right->_len);

    assert(left->_len == right->_len);
    printf("Content (");
    for (size_t i = 0; i < left->_len; i++) {
        const _z_locator_t *l = &left->_val[i];
        const _z_locator_t *r = &right->_val[i];

        char *ls = _z_locator_to_str(l);
        char *rs = _z_locator_to_str(r);

        printf("%s:%s", ls, rs);
        if (i < left->_len - 1) printf(" ");

        z_free(ls);
        z_free(rs);

        assert(_z_locator_eq(l, r) == true);
    }
    printf(")");
}

/*=============================*/
/*  Zenoh Messages Extensions  */
/*=============================*/
/*------------------ UNIT extension ------------------*/
_z_msg_ext_t gen_unit_extension(void) { return _z_msg_ext_make_unit(_Z_MOCK_EXTENSION_UNIT); }

void assert_eq_unit_extension(_z_msg_ext_unit_t *left, _z_msg_ext_unit_t *right) {
    (void)(left);
    (void)(right);
}

void unit_extension(void) {
    printf("\n>> UNIT Extension\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_msg_ext_t ext = gen_unit_extension();
    assert(_Z_EXT_ENC(ext._header) == _Z_MSG_EXT_ENC_UNIT);

    _z_msg_ext_unit_t e_u = ext._body._unit;

    // Encode
    int8_t res = _z_msg_ext_encode_unit(&wbf, &e_u);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_ext_unit_t d_u;
    res = _z_msg_ext_decode_unit(&d_u, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_unit_extension(&e_u, &d_u);

    // Free
    _z_msg_ext_clear_unit(&d_u);
    _z_msg_ext_clear(&ext);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ ZINT extension ------------------*/
_z_msg_ext_t gen_zint_extension(void) {
    _z_zint_t val = gen_zint();
    return _z_msg_ext_make_zint(_Z_MOCK_EXTENSION_ZINT, val);
}

void assert_eq_zint_extension(_z_msg_ext_zint_t *left, _z_msg_ext_zint_t *right) {
    printf("    ZINT (%zu:%zu), ", left->_val, right->_val);
    assert(left->_val == right->_val);
}

void zint_extension(void) {
    printf("\n>> ZINT Extension\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_msg_ext_t ext = gen_zint_extension();
    assert(_Z_EXT_ENC(ext._header) == _Z_MSG_EXT_ENC_ZINT);

    _z_msg_ext_zint_t e_u = ext._body._zint;

    // Encode
    int8_t res = _z_msg_ext_encode_zint(&wbf, &e_u);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_ext_zint_t d_u;
    res = _z_msg_ext_decode_zint(&d_u, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_zint_extension(&e_u, &d_u);

    // Free
    _z_msg_ext_clear_zint(&d_u);
    _z_msg_ext_clear(&ext);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Unit extension ------------------*/
_z_msg_ext_t gen_zbuf_extension(void) {
    _z_bytes_t val = gen_bytes(gen_uint8());
    return _z_msg_ext_make_zbuf(_Z_MOCK_EXTENSION_ZBUF, val);
}

void assert_eq_zbuf_extension(_z_msg_ext_zbuf_t *left, _z_msg_ext_zbuf_t *right) {
    printf("    ZBUF (");
    assert_eq_uint8_array(&left->_val, &right->_val);
    printf(")");
}

void zbuf_extension(void) {
    printf("\n>> ZBUF Extension\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_msg_ext_t ext = gen_zbuf_extension();
    assert(_Z_EXT_ENC(ext._header) == _Z_MSG_EXT_ENC_ZBUF);

    _z_msg_ext_zbuf_t e_u = ext._body._zbuf;

    // Encode
    int8_t res = _z_msg_ext_encode_zbuf(&wbf, &e_u);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_ext_zbuf_t d_u;
    res = _z_msg_ext_decode_zbuf(&d_u, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_zbuf_extension(&e_u, &d_u);

    // Free
    _z_msg_ext_clear_zbuf(&d_u);
    _z_msg_ext_clear(&ext);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_msg_ext_t gen_message_extension(void) {
    _z_msg_ext_t p_zm;

    uint8_t mids[] = {_Z_MOCK_EXTENSION_UNIT, _Z_MOCK_EXTENSION_ZINT, _Z_MOCK_EXTENSION_ZBUF};
    uint8_t i = gen_uint8() % (sizeof(mids) / sizeof(uint8_t));

    switch (mids[i]) {
        case _Z_MOCK_EXTENSION_UNIT:
            p_zm = gen_unit_extension();
            break;
        case _Z_MOCK_EXTENSION_ZINT:
            p_zm = gen_zint_extension();
            break;
        case _Z_MOCK_EXTENSION_ZBUF:
            p_zm = gen_zbuf_extension();
            break;
        default:
            assert(0);
            break;
    }

    return p_zm;
}

void assert_eq_message_extension(_z_msg_ext_t *left, _z_msg_ext_t *right) {
    printf("   Header (%x:%x)", left->_header, right->_header);
    assert(left->_header == right->_header);
    printf("\n");

    switch (_Z_EXT_ID(left->_header)) {
        case _Z_MOCK_EXTENSION_UNIT:
            assert_eq_unit_extension(&left->_body._unit, &left->_body._unit);
            break;
        case _Z_MOCK_EXTENSION_ZINT:
            assert_eq_zint_extension(&left->_body._zint, &left->_body._zint);
            break;
        case _Z_MOCK_EXTENSION_ZBUF:
            assert_eq_zbuf_extension(&left->_body._zbuf, &left->_body._zbuf);
            break;
        default:
            assert(0);
            break;
    }
}

void message_extension(void) {
    printf("\n>> Message extension\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_msg_ext_t e_me = gen_message_extension();

    printf(" - ");
    switch (_Z_EXT_ID(e_me._header)) {
        case _Z_MOCK_EXTENSION_UNIT:
            printf("UNIT extension");
            break;
        case _Z_MOCK_EXTENSION_ZINT:
            printf("ZINT extension");
            break;
        case _Z_MOCK_EXTENSION_ZBUF:
            printf("ZBUF extension");
            break;
        default:
            assert(0);
            break;
    }
    printf("\n");

    // Encode
    int8_t res = _z_msg_ext_encode(&wbf, &e_me, true);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_ext_t d_me;
    _Bool has_next;
    res = _z_msg_ext_decode(&d_me, &zbf, &has_next);
    assert(res == _Z_RES_OK);

    assert_eq_message_extension(&e_me, &d_me);

    // Free
    _z_msg_ext_clear(&e_me);
    _z_msg_ext_clear(&d_me);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*       Message Fields        */
/*=============================*/
/*------------------ Payload field ------------------*/
void assert_eq_payload(_z_bytes_t *left, _z_bytes_t *right) { assert_eq_uint8_array(left, right); }

void payload_field(void) {
    printf("\n>> Payload field\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_bytes_t e_pld = gen_payload(64);

    // Encode
    int8_t res = _z_payload_encode(&wbf, &e_pld);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);

    _z_bytes_t d_pld;
    res = _z_payload_decode(&d_pld, &zbf);
    assert(res == _Z_RES_OK);
    printf("   ");
    assert_eq_payload(&e_pld, &d_pld);
    printf("\n");

    // Free
    _z_payload_clear(&e_pld);
    _z_payload_clear(&d_pld);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Timestamp field ------------------*/
_z_timestamp_t gen_timestamp(void) {
    _z_timestamp_t ts;
    ts.time = gen_uint64();
    _z_bytes_t id = gen_bytes(16);
    memcpy(ts.id.id, id.start, id.len);
    return ts;
}

void assert_eq_timestamp(_z_timestamp_t *left, _z_timestamp_t *right) {
    printf("Timestamp -> ");
    printf("Time (%llu:%llu), ", (unsigned long long)left->time, (unsigned long long)right->time);
    assert(left->time == right->time);

    printf("ID (");
    assert(memcmp(left->id.id, right->id.id, 16) == 0);
    printf(")");
}

void timestamp_field(void) {
    printf("\n>> Timestamp field\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_timestamp_t e_ts = gen_timestamp();

    // Encode
    int8_t res = _z_timestamp_encode(&wbf, &e_ts);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    print_wbuf(&wbf);
    print_iosli(&zbf._ios);
    _z_timestamp_t d_ts;
    res = _z_timestamp_decode(&d_ts, &zbf);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_timestamp(&e_ts, &d_ts);
    printf("\n");

    // Free
    _z_timestamp_clear(&e_ts);
    _z_timestamp_clear(&d_ts);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ SubInfo field ------------------*/
_z_subinfo_t gen_subinfo(void) {
    _z_subinfo_t sm;
    sm.mode = gen_bool() ? Z_SUBMODE_PUSH : Z_SUBMODE_PULL;
    sm.reliability = gen_bool() ? Z_RELIABILITY_RELIABLE : Z_RELIABILITY_BEST_EFFORT;
    if (gen_bool()) {
        sm.period.origin = gen_uint();
        sm.period.period = gen_uint();
        sm.period.duration = gen_uint();
    } else {
        sm.period.origin = 0;
        sm.period.period = 0;
        sm.period.duration = 0;
    }

    return sm;
}

void assert_eq_subinfo(_z_subinfo_t *left, _z_subinfo_t *right) {
    printf("SubInfo -> ");
    printf("Mode (%u:%u), ", left->mode, right->mode);
    assert(left->mode == right->mode);

    printf("Reliable (%u:%u), ", left->reliability, right->reliability);
    assert(left->reliability == right->reliability);

    printf("Period (");
    printf("<%u:%u,%u>", left->period.origin, left->period.period, left->period.duration);
    printf(":");
    printf("<%u:%u,%u>", right->period.origin, right->period.period, right->period.duration);
    printf(")");
    assert(left->period.origin == right->period.origin);
    assert(left->period.period == right->period.period);
    assert(left->period.duration == right->period.duration);
}

void subinfo_field(void) {
    printf("\n>> SubInfo field\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_subinfo_t e_sm = gen_subinfo();

    // Encode
    uint8_t header = e_sm.reliability == Z_RELIABILITY_RELIABLE ? _Z_FLAG_Z_R : 0;
    int8_t res = _z_subinfo_encode(&wbf, &e_sm);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_subinfo_t d_sm;
    res = _z_subinfo_decode(&d_sm, &zbf, header);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_subinfo(&e_sm, &d_sm);
    printf("\n");

    // Free
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ ResKey field ------------------*/
_z_keyexpr_t gen_keyexpr(void) {
    _z_keyexpr_t key;
    key._id = gen_zint();
    _Bool is_numerical = gen_bool();
    if (is_numerical == true)
        key._suffix = NULL;
    else
        key._suffix = gen_str(gen_zint() % 16);

    return key;
}

void assert_eq_keyexpr(_z_keyexpr_t *left, _z_keyexpr_t *right, uint8_t header) {
    printf("ResKey -> ");
    printf("ID (%zu:%zu), ", left->_id, right->_id);
    assert(left->_id == right->_id);

    printf("Name (");
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_K) == true) {
        printf("%s:%s", left->_suffix, right->_suffix);
        assert(_z_str_eq(left->_suffix, right->_suffix) == true);
    } else {
        printf("NULL:NULL");
    }
    printf(")");
}

void keyexpr_field(void) {
    printf("\n>> ResKey field\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_keyexpr_t e_rk = gen_keyexpr();

    // Encode
    uint8_t header = (e_rk._suffix) ? _Z_FLAG_Z_K : 0;
    int8_t res = _z_keyexpr_encode(&wbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K), &e_rk);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_keyexpr_t d_rk;
    res = _z_keyexpr_decode(&d_rk, &zbf, _Z_HAS_FLAG(header, _Z_FLAG_Z_K));
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_keyexpr(&e_rk, &d_rk, header);
    printf("\n");

    // Free
    _z_keyexpr_clear(&e_rk);
    _z_keyexpr_clear(&d_rk);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ DataInfo field ------------------*/
_z_data_info_t gen_data_info(void) {
    _z_data_info_t di;

    di._flags = 0;

    if (gen_bool()) {
        di._kind = gen_uint8();
        _Z_SET_FLAG(di._flags, _Z_DATA_INFO_KIND);
    } else {
        di._kind = 0;
    }

    if (gen_bool()) {
        di._encoding.prefix = gen_uint8();
        if (gen_bool()) {
            di._encoding.suffix = gen_bytes(8);
        } else {
            di._encoding.suffix = _z_bytes_empty();
        }
        _Z_SET_FLAG(di._flags, _Z_DATA_INFO_ENC);
    } else {
        di._encoding.prefix = 0;
        di._encoding.suffix = _z_bytes_empty();
    }

    if (gen_bool()) {
        di._tstamp = gen_timestamp();
        _Z_SET_FLAG(di._flags, _Z_DATA_INFO_TSTAMP);
    } else {
        _z_timestamp_reset(&di._tstamp);
    }

    // WARNING: we do not support sliced content in zenoh-pico.

    if (gen_bool()) {
        di._source_id = gen_bytes(16);
        _Z_SET_FLAG(di._flags, _Z_DATA_INFO_SRC_ID);
    } else {
        di._source_id = _z_bytes_empty();
    }
    if (gen_bool()) {
        di._source_sn = gen_zint();
        _Z_SET_FLAG(di._flags, _Z_DATA_INFO_SRC_SN);
    } else {
        di._source_sn = 0;
    }

    return di;
}

void assert_eq_data_info(_z_data_info_t *left, _z_data_info_t *right) {
    printf("DataInfo -> ");
    printf("Flags (%zu:%zu), ", left->_flags, right->_flags);
    assert(left->_flags == right->_flags);

    if (_Z_HAS_FLAG(left->_flags, _Z_DATA_INFO_KIND) == true) {
        printf("Kind (%d:%d), ", left->_kind, right->_kind);
        assert(left->_kind == right->_kind);
    }
    if (_Z_HAS_FLAG(left->_flags, _Z_DATA_INFO_ENC) == true) {
        printf("Encoding (%u %.*s:%u %.*s), ", left->_encoding.prefix, (int)left->_encoding.suffix.len,
               left->_encoding.suffix.start, right->_encoding.prefix, (int)right->_encoding.suffix.len,
               right->_encoding.suffix.start);
        assert(left->_encoding.prefix == right->_encoding.prefix);
        assert_eq_uint8_array(&left->_encoding.suffix, &right->_encoding.suffix);
    }
    if (_Z_HAS_FLAG(left->_flags, _Z_DATA_INFO_TSTAMP) == true) {
        printf("Tstamp -> ");
        assert_eq_timestamp(&left->_tstamp, &right->_tstamp);
        printf(", ");
    }

    if (_Z_HAS_FLAG(left->_flags, _Z_DATA_INFO_SRC_ID) == true) {
        printf("Src ID -> ");
        assert_eq_uint8_array(&left->_source_id, &right->_source_id);
        printf(", ");
    }
    if (_Z_HAS_FLAG(left->_flags, _Z_DATA_INFO_SRC_SN) == true) {
        printf("Src SN (%zu:%zu), ", left->_source_sn, right->_source_sn);
        assert(left->_source_sn == right->_source_sn);
    }
}

void data_info_field(void) {
    printf("\n>> DataInfo field\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_data_info_t e_di = gen_data_info();

    // Encode
    int8_t res = _z_data_info_encode(&wbf, &e_di);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_data_info_t d_di;
    res = _z_data_info_decode(&d_di, &zbf);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_data_info(&e_di, &d_di);
    printf("\n");

    // Free
    _z_data_info_clear(&d_di);
    _z_data_info_clear(&e_di);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*     Message decorators      */
/*=============================*/

/*------------------ ReplyContext decorator ------------------*/
void print_reply_context(_z_reply_context_t *rc) {
    printf("      Header: %x\n", rc->_header);
    printf("      QID: %zu\n", rc->_qid);
    if (_Z_HAS_FLAG(rc->_header, _Z_FLAG_Z_F) == false) {
        printf("      Replier ID: ");
        print_uint8_array((_z_bytes_t *)&rc->_replier_id);
    }
    printf("\n");
}

_z_reply_context_t *gen_reply_context(void) {
    _z_zint_t qid = gen_zint();
    _z_id_t replier_id = gen_zid();
    _Bool is_final = gen_bool();
    if (is_final == true) {
        replier_id = _z_id_empty();
    }

    return _z_msg_make_reply_context(qid, replier_id, is_final);
}

void assert_eq_reply_context(_z_reply_context_t *left, _z_reply_context_t *right) {
    printf("Header (%x:%x), ", left->_header, right->_header);
    assert(left->_header == right->_header);

    printf("QID (%zu:%zu), ", left->_qid, right->_qid);
    assert(left->_qid == right->_qid);

    printf("Replier ID (");
    if (_Z_HAS_FLAG(left->_header, _Z_FLAG_Z_F) == false) {
        assert(memcmp(left->_replier_id.id, right->_replier_id.id, 16) == 0);
    } else {
        printf("NULL:NULL");
    }
    printf(")");
}

/*=============================*/
/*      Declaration Fields     */
/*=============================*/
/*------------------ Resource declaration ------------------*/
_z_res_decl_t gen_resource_declaration(uint8_t *header) {
    _z_res_decl_t e_rd;

    e_rd._id = gen_zint();
    e_rd._key = gen_keyexpr();
    _Z_SET_FLAG(*header, (e_rd._key._suffix) ? _Z_FLAG_Z_K : 0);

    return e_rd;
}

void assert_eq_resource_declaration(_z_res_decl_t *left, _z_res_decl_t *right, uint8_t header) {
    printf("RID (%zu:%zu), ", left->_id, right->_id);
    assert(left->_id == right->_id);
    assert_eq_keyexpr(&left->_key, &right->_key, header);
}

void resource_declaration(void) {
    printf("\n>> Resource declaration\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    uint8_t e_hdr = 0;
    _z_res_decl_t e_rd = gen_resource_declaration(&e_hdr);

    // Encode
    int8_t res = _z_res_decl_encode(&wbf, e_hdr, &e_rd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_res_decl_t d_rd;
    res = _z_res_decl_decode(&d_rd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_resource_declaration(&e_rd, &d_rd, e_hdr);
    printf("\n");

    // Free
    _z_declaration_clear_resource(&e_rd);
    _z_declaration_clear_resource(&d_rd);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Publisher declaration ------------------*/
_z_pub_decl_t gen_publisher_declaration(uint8_t *header) {
    _z_pub_decl_t e_pd;

    e_pd._key = gen_keyexpr();
    _Z_SET_FLAG(*header, (e_pd._key._suffix) ? _Z_FLAG_Z_K : 0);

    return e_pd;
}

void assert_eq_publisher_declaration(_z_pub_decl_t *left, _z_pub_decl_t *right, uint8_t header) {
    assert_eq_keyexpr(&left->_key, &right->_key, header);
}

void publisher_declaration(void) {
    printf("\n>> Publisher declaration\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    uint8_t e_hdr = 0;
    _z_pub_decl_t e_pd = gen_publisher_declaration(&e_hdr);

    // Encode
    int8_t res = _z_pub_decl_encode(&wbf, e_hdr, &e_pd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_pub_decl_t d_pd;
    res = _z_pub_decl_decode(&d_pd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_publisher_declaration(&e_pd, &d_pd, e_hdr);
    printf("\n");

    // Free
    _z_declaration_clear_publisher(&e_pd);
    _z_declaration_clear_publisher(&d_pd);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Subscriber declaration ------------------*/
_z_sub_decl_t gen_subscriber_declaration(uint8_t *header) {
    _z_sub_decl_t e_sd;

    e_sd._subinfo = gen_subinfo();
    if (e_sd._subinfo.mode != Z_SUBMODE_PUSH || e_sd._subinfo.period.period != 0) _Z_SET_FLAG(*header, _Z_FLAG_Z_S);
    if (e_sd._subinfo.reliability == Z_RELIABILITY_RELIABLE) _Z_SET_FLAG(*header, _Z_FLAG_Z_R);

    e_sd._key = gen_keyexpr();
    if (e_sd._key._suffix) _Z_SET_FLAG(*header, _Z_FLAG_Z_K);

    return e_sd;
}

void assert_eq_subscriber_declaration(_z_sub_decl_t *left, _z_sub_decl_t *right, uint8_t header) {
    assert_eq_keyexpr(&left->_key, &right->_key, header);
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_S) == true) {
        printf(", ");
        assert_eq_subinfo(&left->_subinfo, &right->_subinfo);
    }
}

void subscriber_declaration(void) {
    printf("\n>> Subscriber declaration\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    uint8_t e_hdr = 0;
    _z_sub_decl_t e_sd = gen_subscriber_declaration(&e_hdr);

    // Encode
    int8_t res = _z_sub_decl_encode(&wbf, e_hdr, &e_sd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_sub_decl_t d_sd;
    res = _z_sub_decl_decode(&d_sd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_subscriber_declaration(&e_sd, &d_sd, e_hdr);
    printf("\n");

    // Free
    _z_declaration_clear_subscriber(&e_sd);
    _z_declaration_clear_subscriber(&d_sd);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Queryable declaration ------------------*/
_z_qle_decl_t gen_queryable_declaration(uint8_t *header) {
    _z_qle_decl_t e_qd;

    e_qd._key = gen_keyexpr();
    _Z_SET_FLAG(*header, (e_qd._key._suffix) ? _Z_FLAG_Z_K : 0);

    if (gen_bool()) {
        e_qd._complete = gen_zint();
        e_qd._distance = gen_zint();
        _Z_SET_FLAG(*header, _Z_FLAG_Z_Q);
    }

    return e_qd;
}

void assert_eq_queryable_declaration(_z_qle_decl_t *left, _z_qle_decl_t *right, uint8_t header) {
    assert_eq_keyexpr(&left->_key, &right->_key, header);

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_I) == true) {
        printf("Complete (%zu:%zu), ", left->_complete, right->_complete);
        assert(left->_complete == right->_complete);
        printf("Distance (%zu:%zu), ", left->_distance, right->_distance);
        assert(left->_distance == right->_distance);
    }
}

void queryable_declaration(void) {
    printf("\n>> Queryable declaration\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    uint8_t e_hdr = 0;
    _z_qle_decl_t e_qd = gen_queryable_declaration(&e_hdr);

    // Encode
    int8_t res = _z_qle_decl_encode(&wbf, e_hdr, &e_qd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_qle_decl_t d_qd;
    res = _z_qle_decl_decode(&d_qd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_queryable_declaration(&e_qd, &d_qd, e_hdr);
    printf("\n");

    // Free
    _z_declaration_clear_queryable(&e_qd);
    _z_declaration_clear_queryable(&d_qd);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Forget Resource declaration ------------------*/
_z_forget_res_decl_t gen_forget_resource_declaration(void) {
    _z_forget_res_decl_t e_frd;

    e_frd._rid = gen_zint();

    return e_frd;
}

void assert_eq_forget_resource_declaration(_z_forget_res_decl_t *left, _z_forget_res_decl_t *right) {
    printf("RID (%zu:%zu)", left->_rid, right->_rid);
    assert(left->_rid == right->_rid);
}

void forget_resource_declaration(void) {
    printf("\n>> Forget resource declaration\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_forget_res_decl_t e_frd = gen_forget_resource_declaration();

    // Encode
    int8_t res = _z_forget_res_decl_encode(&wbf, &e_frd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_forget_res_decl_t d_frd;
    res = _z_forget_res_decl_decode(&d_frd, &zbf);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_forget_resource_declaration(&e_frd, &d_frd);
    printf("\n");

    // Free
    // NOTE: forget_res_decl does not involve any heap allocation
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Forget Publisher declaration ------------------*/
_z_forget_pub_decl_t gen_forget_publisher_declaration(uint8_t *header) {
    _z_forget_pub_decl_t e_fpd;

    e_fpd._key = gen_keyexpr();
    _Z_SET_FLAG(*header, (e_fpd._key._suffix) ? _Z_FLAG_Z_K : 0);

    return e_fpd;
}

void assert_eq_forget_publisher_declaration(_z_forget_pub_decl_t *left, _z_forget_pub_decl_t *right, uint8_t header) {
    assert_eq_keyexpr(&left->_key, &right->_key, header);
}

void forget_publisher_declaration(void) {
    printf("\n>> Forget publisher declaration\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    uint8_t e_hdr = 0;
    _z_forget_pub_decl_t e_fpd = gen_forget_publisher_declaration(&e_hdr);

    // Encode
    int8_t res = _z_forget_pub_decl_encode(&wbf, e_hdr, &e_fpd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_forget_pub_decl_t d_fpd;
    res = _z_forget_pub_decl_decode(&d_fpd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_forget_publisher_declaration(&e_fpd, &d_fpd, e_hdr);
    printf("\n");

    // Free
    _z_declaration_clear_forget_publisher(&e_fpd);
    _z_declaration_clear_forget_publisher(&d_fpd);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Forget Subscriber declaration ------------------*/
_z_forget_sub_decl_t gen_forget_subscriber_declaration(uint8_t *header) {
    _z_forget_sub_decl_t e_fsd;

    e_fsd._key = gen_keyexpr();
    _Z_SET_FLAG(*header, (e_fsd._key._suffix) ? _Z_FLAG_Z_K : 0);

    return e_fsd;
}

void assert_eq_forget_subscriber_declaration(_z_forget_sub_decl_t *left, _z_forget_sub_decl_t *right, uint8_t header) {
    assert_eq_keyexpr(&left->_key, &right->_key, header);
}

void forget_subscriber_declaration(void) {
    printf("\n>> Forget subscriber declaration\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    uint8_t e_hdr = 0;
    _z_forget_sub_decl_t e_fsd = gen_forget_subscriber_declaration(&e_hdr);

    // Encode
    int8_t res = _z_forget_sub_decl_encode(&wbf, e_hdr, &e_fsd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_forget_sub_decl_t d_fsd;
    res = _z_forget_sub_decl_decode(&d_fsd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_forget_subscriber_declaration(&e_fsd, &d_fsd, e_hdr);
    printf("\n");

    // Free
    _z_declaration_clear_forget_subscriber(&e_fsd);
    _z_declaration_clear_forget_subscriber(&d_fsd);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Forget Queryable declaration ------------------*/
_z_forget_qle_decl_t gen_forget_queryable_declaration(uint8_t *header) {
    _z_forget_qle_decl_t e_fqd;

    e_fqd._key = gen_keyexpr();
    _Z_SET_FLAG(*header, (e_fqd._key._suffix) ? _Z_FLAG_Z_K : 0);

    return e_fqd;
}

void assert_eq_forget_queryable_declaration(_z_forget_qle_decl_t *left, _z_forget_qle_decl_t *right, uint8_t header) {
    assert_eq_keyexpr(&left->_key, &right->_key, header);
}

void forget_queryable_declaration(void) {
    printf("\n>> Forget queryable declaration\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    uint8_t e_hdr = 0;
    _z_forget_qle_decl_t e_fqd = gen_forget_queryable_declaration(&e_hdr);

    // Encode
    int8_t res = _z_forget_qle_decl_encode(&wbf, e_hdr, &e_fqd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_forget_qle_decl_t d_fqd;
    res = _z_forget_qle_decl_decode(&d_fqd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_forget_queryable_declaration(&e_fqd, &d_fqd, e_hdr);
    printf("\n");

    // Free
    _z_declaration_clear_forget_queryable(&e_fqd);
    _z_declaration_clear_forget_queryable(&d_fqd);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Declaration ------------------*/
_z_declaration_t gen_declaration(void) {
    uint8_t decl[] = {_Z_DECL_RESOURCE,          _Z_DECL_PUBLISHER,       _Z_DECL_SUBSCRIBER,
                      _Z_DECL_QUERYABLE,         _Z_DECL_FORGET_RESOURCE, _Z_DECL_FORGET_PUBLISHER,
                      _Z_DECL_FORGET_SUBSCRIBER, _Z_DECL_FORGET_QUERYABLE};

    _z_declaration_t d;
    d._header = decl[gen_uint8() % (sizeof(decl) / sizeof(uint8_t))];

    switch (d._header) {
        case _Z_DECL_RESOURCE:
            d._body._res = gen_resource_declaration(&d._header);
            break;
        case _Z_DECL_PUBLISHER:
            d._body._pub = gen_publisher_declaration(&d._header);
            break;
        case _Z_DECL_SUBSCRIBER:
            d._body._sub = gen_subscriber_declaration(&d._header);
            break;
        case _Z_DECL_QUERYABLE:
            d._body._qle = gen_queryable_declaration(&d._header);
            break;
        case _Z_DECL_FORGET_RESOURCE:
            d._body._forget_res = gen_forget_resource_declaration();
            break;
        case _Z_DECL_FORGET_PUBLISHER:
            d._body._forget_pub = gen_forget_publisher_declaration(&d._header);
            break;
        case _Z_DECL_FORGET_SUBSCRIBER:
            d._body._forget_sub = gen_forget_subscriber_declaration(&d._header);
            break;
        case _Z_DECL_FORGET_QUERYABLE:
            d._body._forget_qle = gen_forget_queryable_declaration(&d._header);
            break;
    }

    return d;
}

void assert_eq_declaration(_z_declaration_t *left, _z_declaration_t *right) {
    printf("Declaration -> ");
    printf("Header (%x:%x), ", left->_header, right->_header);
    assert(left->_header == right->_header);

    switch (left->_header) {
        case _Z_DECL_RESOURCE:
            assert_eq_resource_declaration(&left->_body._res, &right->_body._res, left->_header);
            break;
        case _Z_DECL_PUBLISHER:
            assert_eq_publisher_declaration(&left->_body._pub, &right->_body._pub, left->_header);
            break;
        case _Z_DECL_SUBSCRIBER:
            assert_eq_subscriber_declaration(&left->_body._sub, &right->_body._sub, left->_header);
            break;
        case _Z_DECL_QUERYABLE:
            assert_eq_queryable_declaration(&left->_body._qle, &right->_body._qle, left->_header);
            break;
        case _Z_DECL_FORGET_RESOURCE:
            assert_eq_forget_resource_declaration(&left->_body._forget_res, &right->_body._forget_res);
            break;
        case _Z_DECL_FORGET_PUBLISHER:
            assert_eq_forget_publisher_declaration(&left->_body._forget_pub, &right->_body._forget_pub, left->_header);
            break;
        case _Z_DECL_FORGET_SUBSCRIBER:
            assert_eq_forget_subscriber_declaration(&left->_body._forget_sub, &right->_body._forget_sub, left->_header);
            break;
        case _Z_DECL_FORGET_QUERYABLE:
            assert_eq_forget_queryable_declaration(&left->_body._forget_qle, &right->_body._forget_qle, left->_header);
            break;
    }
}

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/
/*------------------ Data message ------------------*/
_z_zenoh_message_t gen_data_message(void) {
    _z_keyexpr_t key = gen_keyexpr();

    _z_data_info_t info;
    if (gen_bool()) {
        info = gen_data_info();
    } else {
        info._flags = 0;
        info._kind = Z_SAMPLE_KIND_PUT;
        info._encoding.prefix = Z_ENCODING_PREFIX_EMPTY;
        info._encoding.suffix = _z_bytes_empty();
        info._source_id = _z_bytes_empty();
        info._source_sn = 0;
        _z_timestamp_reset(&info._tstamp);
    }

    _Bool can_be_dropped = gen_bool();
    _z_bytes_t payload = gen_payload(1 + gen_zint() % 64);

    return _z_msg_make_data(key, info, payload, can_be_dropped);
}

void assert_eq_data_message(_z_msg_data_t *left, _z_msg_data_t *right, uint8_t header) {
    printf("   ");
    assert_eq_keyexpr(&left->_key, &right->_key, header);
    printf("\n");
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_I) == true) {
        printf("   ");
        assert_eq_data_info(&left->_info, &right->_info);
        printf("\n");
    }
    printf("   ");
    assert_eq_payload(&left->_payload, &right->_payload);
    printf("\n");
}

void data_message(void) {
    printf("\n>> Data message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_zenoh_message_t z_msg = gen_data_message();
    assert(_Z_MID(z_msg._header) == _Z_MID_Z_DATA);

    _z_msg_data_t e_da = z_msg._body._data;

    // Encode
    int8_t res = _z_data_encode(&wbf, z_msg._header, &e_da);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_data_t d_da;
    res = _z_data_decode(&d_da, &zbf, z_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_data_message(&e_da, &d_da, z_msg._header);

    // Free
    _z_msg_clear_data(&d_da);
    _z_msg_clear(&z_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Pull message ------------------*/
_z_zenoh_message_t gen_pull_message(void) {
    _z_keyexpr_t key = gen_keyexpr();
    _z_zint_t pull_id = gen_zint();
    _z_zint_t max_samples = gen_bool() ? gen_zint() : 0;
    _Bool is_final = gen_bool();

    return _z_msg_make_pull(key, pull_id, max_samples, is_final);
}

void assert_eq_pull_message(_z_msg_pull_t *left, _z_msg_pull_t *right, uint8_t header) {
    printf("   ");
    assert_eq_keyexpr(&left->_key, &right->_key, header);
    printf("\n");

    printf("   Pull ID (%zu:%zu)", left->_pull_id, right->_pull_id);
    assert(left->_pull_id == right->_pull_id);
    printf("\n");

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_N) == true) {
        printf("   Max samples (%zu:%zu)", left->_max_samples, right->_max_samples);
        assert(left->_max_samples == right->_max_samples);
        printf("\n");
    }
}

void pull_message(void) {
    printf("\n>> Pull message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_zenoh_message_t z_msg = gen_pull_message();
    assert(_Z_MID(z_msg._header) == _Z_MID_Z_PULL);

    _z_msg_pull_t e_pu = z_msg._body._pull;

    // Encode
    int8_t res = _z_pull_encode(&wbf, z_msg._header, &e_pu);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_pull_t d_pu;
    res = _z_pull_decode(&d_pu, &zbf, z_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_pull_message(&e_pu, &d_pu, z_msg._header);

    // Free
    _z_msg_clear_pull(&d_pu);
    _z_msg_clear(&z_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Query message ------------------*/
_z_zenoh_message_t gen_query_message(void) {
    _z_keyexpr_t key = gen_keyexpr();
    char *parameters = gen_str(gen_uint8() % 16);
    _z_zint_t qid = gen_zint();

    z_query_target_t target;
    if (gen_bool()) {
        uint8_t tgt[] = {Z_QUERY_TARGET_BEST_MATCHING, Z_QUERY_TARGET_ALL_COMPLETE, Z_QUERY_TARGET_ALL};
        target = tgt[gen_uint8() % (sizeof(tgt) / sizeof(uint8_t))];
    } else {
        target = Z_QUERY_TARGET_BEST_MATCHING;
    }

    uint8_t con[] = {Z_CONSOLIDATION_MODE_LATEST, Z_CONSOLIDATION_MODE_MONOTONIC, Z_CONSOLIDATION_MODE_NONE};
    z_consolidation_mode_t consolidation;
    consolidation = con[gen_uint8() % (sizeof(con) / sizeof(uint8_t))];

    _z_value_t value;
    if (gen_bool()) {
        value = gen_value();
    } else {
        value.encoding.prefix = Z_ENCODING_PREFIX_EMPTY;
        value.encoding.suffix = _z_bytes_empty();
        value.payload = _z_bytes_empty();
    }

    return _z_msg_make_query(key, parameters, qid, target, consolidation, value);
}

void assert_eq_query_message(_z_msg_query_t *left, _z_msg_query_t *right, uint8_t header) {
    printf("   Consolidation ( %u:%u)", left->_consolidation, right->_consolidation);
    assert(left->_consolidation == right->_consolidation);
    printf("\n");

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_B) == true) {
        printf("   ");
        assert_eq_data_info(&left->_info, &right->_info);
        printf("\n");
        printf("   ");
        assert_eq_payload(&left->_payload, &right->_payload);
        printf("\n");
    }
}

void query_message(void) {
    printf("\n>> Query message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    uint8_t e_hdr = 0;
    _z_zenoh_message_t z_msg = gen_query_message();
    assert(_Z_MID(z_msg._header) == _Z_MID_Z_QUERY);

    _z_msg_query_t e_qy = z_msg._body._query;

    // Encode
    int8_t res = _z_query_encode(&wbf, e_hdr, &e_qy);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_query_t d_qy;
    res = _z_query_decode(&d_qy, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    assert_eq_query_message(&e_qy, &d_qy, e_hdr);

    // Free
    _z_msg_query_clear(&d_qy);
    _z_msg_clear(&z_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Unit message ------------------*/
_z_zenoh_message_t gen_unit_message(void) {
    _Bool can_be_dropped = gen_bool();
    return _z_msg_make_unit(can_be_dropped);
}

/*------------------ Zenoh message ------------------*/
_z_zenoh_message_t gen_zenoh_message(void) {
    _z_zenoh_message_t p_zm;

    uint8_t mids[] = {_Z_MID_Z_DATA, _Z_MID_Z_PULL, _Z_MID_Z_QUERY, _Z_MID_Z_UNIT};
    uint8_t i = gen_uint8() % (sizeof(mids) / sizeof(uint8_t));

    switch (mids[i]) {
        case _Z_MID_Z_DATA:
            p_zm = gen_data_message();
            break;
        case _Z_MID_Z_PULL:
            p_zm = gen_pull_message();
            break;
        case _Z_MID_Z_QUERY:
            p_zm = gen_query_message();
            break;
        case _Z_MID_Z_UNIT:
            p_zm = gen_unit_message();
            break;
        default:
            assert(0);
            break;
    }

    if (gen_bool()) {
        p_zm._reply_context = gen_reply_context();
    } else {
        p_zm._reply_context = NULL;
    }

    return p_zm;
}

void assert_eq_zenoh_message(_z_zenoh_message_t *left, _z_zenoh_message_t *right) {
    // Test message decorators

    if (left->_reply_context && right->_reply_context) {
        printf("   ");
        assert_eq_reply_context(left->_reply_context, right->_reply_context);
        printf("\n");
    } else {
        printf("   Reply Context: %p:%p\n", (void *)left->_reply_context, (void *)right->_reply_context);
        assert(left->_reply_context == right->_reply_context);
    }

    // Test message
    printf("   Header (%x:%x)", left->_header, right->_header);
    assert(left->_header == right->_header);
    printf("\n");

    switch (_Z_MID(left->_header)) {
        case _Z_MID_Z_DATA:
            assert_eq_data_message(&left->_body._data, &right->_body._data, left->_header);
            break;
        case _Z_MID_Z_PULL:
            assert_eq_pull_message(&left->_body._pull, &right->_body._pull, left->_header);
            break;
        case _Z_MID_Z_QUERY:
            assert_eq_query_message(&left->_body._query, &right->_body._query, left->_header);
            break;
        case _Z_MID_Z_UNIT:
            // Do nothing. Unit messages have no body
            break;
        default:
            assert(0);
            break;
    }
}

void zenoh_message(void) {
    printf("\n>> Zenoh message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_zenoh_message_t e_zm = gen_zenoh_message();

    printf(" - ");
    switch (_Z_MID(e_zm._header)) {
        case _Z_MID_Z_DATA:
            printf("Data message");
            break;
        case _Z_MID_Z_PULL:
            printf("Pull message");
            break;
        case _Z_MID_Z_QUERY:
            printf("Query message");
            break;
        case _Z_MID_Z_UNIT:
            printf("Unit message");
            break;
        default:
            assert(0);
            break;
    }
    if (e_zm._reply_context) {
        printf("   Reply Context\n");
        print_reply_context(e_zm._reply_context);
        printf("\n");
    }
    printf("\n");

    // Encode
    int8_t res = _z_zenoh_message_encode(&wbf, &e_zm);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_zenoh_message_t d_zm;
    res = _z_zenoh_message_decode(&d_zm, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_zenoh_message(&e_zm, &d_zm);

    // Free
    _z_msg_clear(&e_zm);
    _z_msg_clear(&d_zm);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*        Network Messages     */
/*=============================*/
/*------------------ Declare message ------------------*/
_z_network_message_t gen_declare_message(void) {
    _z_declaration_t declaration = gen_declaration();

    return _z_n_msg_make_declare(declaration);
}

void assert_eq_declare_message(_z_n_msg_declare_t *left, _z_n_msg_declare_t *right) {
    printf("   ");
    assert_eq_declaration(&left->_declaration, &right->_declaration);
    printf("\n");
}

void declare_message(void) {
    printf("\n>> Declare message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_network_message_t n_msg = gen_declare_message();
    assert(_Z_MID(n_msg._header) == _Z_MID_N_DECLARE);

    _z_n_msg_declare_t e_dcl = n_msg._body._declare;

    // Encode
    int8_t res = _z_declare_encode(&wbf, &e_dcl);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_n_msg_declare_t d_dcl;
    res = _z_declare_decode(&d_dcl, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_declare_message(&e_dcl, &d_dcl);

    // Free
    _z_n_msg_clear_declare(&d_dcl);
    _z_n_msg_clear(&n_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Network message ------------------*/
_z_network_message_t gen_network_message(void) {
    _z_network_message_t p_nm;

    uint8_t mids[] = {_Z_MID_N_DECLARE};
    uint8_t i = gen_uint8() % (sizeof(mids) / sizeof(uint8_t));

    switch (mids[i]) {
        case _Z_MID_N_DECLARE:
            p_nm = gen_declare_message();
            break;
        default:
            assert(0);
            break;
    }

    return p_nm;
}

void assert_eq_network_message(_z_network_message_t *left, _z_network_message_t *right) {
    // Test message
    printf("   Header (%x:%x)", left->_header, right->_header);
    assert(left->_header == right->_header);
    printf("\n");

    switch (_Z_MID(left->_header)) {
        case _Z_MID_N_DECLARE:
            assert_eq_declare_message(&left->_body._declare, &right->_body._declare);
            break;
        default:
            assert(0);
            break;
    }
}

void network_message(void) {
    printf("\n>> Network message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_network_message_t e_nm = gen_network_message();

    printf(" - ");
    switch (_Z_MID(e_nm._header)) {
        case _Z_MID_N_DECLARE:
            printf("Declare message");
            break;
        default:
            assert(0);
            break;
    }
    printf("\n");

    // Encode
    int8_t res = _z_network_message_encode(&wbf, &e_nm);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_network_message_t d_nm;
    res = _z_network_message_decode(&d_nm, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_network_message(&e_nm, &d_nm);

    // Free
    _z_n_msg_clear(&e_nm);
    _z_n_msg_clear(&d_nm);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*        Network Messages     */
/*=============================*/
/*------------------ Push Message ------------------*/
_z_network_message_t gen_push_message(void) {
    _z_network_message_t n_msg;

    _z_keyexpr_t key = gen_keyexpr();
    _Bool is_remote = gen_bool();
    n_msg = _z_n_msg_make_push(key, (_z_push_body_t){}, is_remote);  // TODO[protocol]: body

    return n_msg;
}

void assert_eq_push_message(_z_n_msg_push_t *left, _z_n_msg_push_t *right, uint8_t header) {
    printf("   ");
    assert_eq_keyexpr(&left->_key, &right->_key, header);
    printf("\n");

    // TODO[protocol]: body
}

void push_message(void) {
    printf("\n>> Push message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_network_message_t n_msg = gen_push_message();
    assert(_Z_MID(n_msg._header) == _Z_MID_N_PUSH);

    _z_n_msg_push_t e_pu = n_msg._body._push;

    // Encode
    int8_t res = _z_push_encode(&wbf, n_msg._header, &e_pu);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_n_msg_push_t d_pu;
    res = _z_push_decode(&d_pu, &zbf, n_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_push_message(&e_pu, &d_pu, n_msg._header);

    // Free
    _z_n_msg_push_clear(&d_pu);
    _z_n_msg_clear(&n_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Request Message ------------------*/
_z_network_message_t gen_request_message(void) {
    _z_network_message_t n_msg;

    _z_zint_t rid = gen_zint();
    _z_keyexpr_t key = gen_keyexpr();
    _Bool is_remote = gen_bool();
    n_msg = _z_n_msg_make_request(rid, key, (_z_request_body_t){}, is_remote);  // TODO[protocol]: body

    return n_msg;
}

void assert_eq_request_message(_z_n_msg_request_t *left, _z_n_msg_request_t *right, uint8_t header) {
    printf("   ");
    assert(left->_rid == right->_rid);
    printf("\n");

    printf("   ");
    assert_eq_keyexpr(&left->_key, &right->_key, header);
    printf("\n");

    // TODO[protocol]: body
}

void request_message(void) {
    printf("\n>> Request message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_network_message_t n_msg = gen_request_message();
    assert(_Z_MID(n_msg._header) == _Z_MID_N_REQUEST);

    _z_n_msg_request_t e_re = n_msg._body._request;

    // Encode
    int8_t res = _z_request_encode(&wbf, n_msg._header, &e_re);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_n_msg_request_t d_re;
    res = _z_request_decode(&d_re, &zbf, n_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_request_message(&e_re, &d_re, n_msg._header);

    // Free
    _z_n_msg_request_clear(&d_re);
    _z_n_msg_clear(&n_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Response Message ------------------*/
_z_network_message_t gen_response_message(void) {
    _z_network_message_t n_msg;

    _z_zint_t rid = gen_zint();
    _z_keyexpr_t key = gen_keyexpr();
    _Bool is_remote = gen_bool();
    n_msg = _z_n_msg_make_response(rid, key, (_z_response_body_t){}, is_remote);  // TODO[protocol]: body

    return n_msg;
}

void assert_eq_response_message(_z_msg_reply_t *left, _z_msg_reply_t *right, uint8_t header) {
    printf("   ");
    assert(left->_rid == right->_rid);
    printf("\n");

    printf("   ");
    assert_eq_keyexpr(&left->_key, &right->_key, header);
    printf("\n");

    // TODO[protocol]: body
}

void response_message(void) {
    printf("\n>> Response message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_network_message_t n_msg = gen_response_message();
    assert(_Z_MID(n_msg._header) == _Z_MID_N_RESPONSE);

    _z_msg_reply_t e_re = n_msg._body._response;

    // Encode
    int8_t res = _z_response_encode(&wbf, n_msg._header, &e_re);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_reply_t d_re;
    res = _z_response_decode(&d_re, &zbf, n_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_response_message(&e_re, &d_re, n_msg._header);

    // Free
    _z_msg_reply_clear(&d_re);
    _z_n_msg_clear(&n_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Response Final Message ------------------*/
_z_network_message_t gen_response_final_message(void) {
    _z_network_message_t n_msg;

    _z_zint_t rid = gen_zint();
    n_msg = _z_n_msg_make_response_final(rid);

    return n_msg;
}

void assert_eq_response_final_message(_z_n_msg_response_final_t *left, _z_n_msg_response_final_t *right) {
    printf("   ");
    assert(left->_requestid == right->_requestid);
    printf("\n");

    // TODO[protocol]: body
}

void response_final_message(void) {
    printf("\n>> Response Final message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_network_message_t n_msg = gen_response_final_message();
    assert(_Z_MID(n_msg._header) == _Z_MID_N_RESPONSE_FINAL);

    _z_n_msg_response_final_t e_re = n_msg._body._response_f;

    // Encode
    int8_t res = _z_response_final_encode(&wbf, n_msg._header, &e_re);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_n_msg_response_final_t d_re;
    res = _z_response_final_decode(&d_re, &zbf, n_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_response_final_message(&e_re, &d_re);

    // Free
    _z_n_msg_response_final_clear(&d_re);
    _z_n_msg_clear(&n_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*        Scouting Messages    */
/*=============================*/
/*------------------ Scout Message ------------------*/
_z_scouting_message_t gen_scout_message(void) {
    z_what_t what = gen_uint8() % 7;
    _z_id_t zid = gen_zid();
    return _z_s_msg_make_scout(what, zid);
}

void assert_eq_scout_message(_z_s_msg_scout_t *left, _z_s_msg_scout_t *right, uint8_t header) {
    (void)(header);
    printf("   Version (%u:%u)\n", left->_version, right->_version);
    assert(left->_version == right->_version);
    printf("   What (%u:%u)\n", left->_what, right->_what);
    assert(left->_what == right->_what);
    printf("   ");
    assert(memcmp(left->_zid.id, right->_zid.id, 16) == 0);
    printf("\n");
}

void scout_message(void) {
    printf("\n>> Scout message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_scouting_message_t s_msg = gen_scout_message();
    _z_s_msg_scout_t e_sc = s_msg._body._scout;

    // Encode
    int8_t res = _z_scout_encode(&wbf, s_msg._header, &e_sc);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_s_msg_scout_t d_sc;
    res = _z_scout_decode(&d_sc, &zbf, s_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_scout_message(&e_sc, &d_sc, s_msg._header);

    // Free
    _z_s_msg_clear_scout(&d_sc);
    _z_s_msg_clear(&s_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Hello Message ------------------*/
_z_scouting_message_t gen_hello_message(void) {
    z_whatami_t whatami = (gen_uint8() % 2) + 1;
    _z_id_t zid = gen_zid();

    _z_locator_array_t locators;
    if (gen_bool() == true)
        locators = gen_locator_array((gen_uint8() % 4) + 1);
    else
        locators = gen_locator_array(0);

    return _z_s_msg_make_hello(whatami, zid, locators);
}

void assert_eq_hello_message(_z_s_msg_hello_t *left, _z_s_msg_hello_t *right, uint8_t header) {
    printf("   Version (%u:%u)", left->_version, right->_version);
    assert(left->_version == right->_version);
    printf("\n");

    printf("   What (%u:%u)", left->_whatami, right->_whatami);
    assert(left->_whatami == right->_whatami);
    printf("\n");

    printf("   ");
    assert(memcmp(left->_zid.id, right->_zid.id, 16) == 0);
    printf("\n");

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_HELLO_L) == true) {
        printf("   ");
        assert_eq_locator_array(&left->_locators, &right->_locators);
        printf("\n");
    }
}

void hello_message(void) {
    printf("\n>> Hello message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_scouting_message_t s_msg = gen_hello_message();
    assert(_Z_MID(s_msg._header) == _Z_MID_HELLO);

    _z_s_msg_hello_t e_he = s_msg._body._hello;

    // Encode
    int8_t res = _z_hello_encode(&wbf, s_msg._header, &e_he);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_s_msg_hello_t d_he;
    res = _z_hello_decode(&d_he, &zbf, s_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_hello_message(&e_he, &d_he, s_msg._header);

    // Free
    _z_s_msg_clear_hello(&d_he);
    _z_s_msg_clear(&s_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*       Transport Messages    */
/*=============================*/
/*------------------ Join Message ------------------*/
_z_transport_message_t gen_join_message(void) {
    z_whatami_t whatami = (gen_uint8() % 2) + 1;
    _z_id_t zid = gen_zid();
    _z_zint_t lease = gen_bool() ? gen_zint() * 1000 : gen_zint();

    _z_conduit_sn_list_t next_sn;
    if (gen_bool()) {
        for (uint8_t i = 0; i < Z_PRIORITIES_NUM; i++) {
            next_sn._val._qos[i]._reliable = gen_zint();
            next_sn._val._qos[i]._best_effort = gen_zint();
        }
    } else {
        next_sn._val._plain._reliable = gen_zint();
        next_sn._val._plain._best_effort = gen_zint();
    }

    _z_transport_message_t t_msg = _z_t_msg_make_join(whatami, lease, zid, next_sn);

    if (gen_bool()) {
        t_msg._body._join._batch_size = gen_uint16();
    }

    if (gen_bool()) {
        t_msg._body._join._seq_num_res = gen_uint8() % 4;
    }

    if (gen_bool()) {
        t_msg._body._join._req_id_res = gen_uint8() % 4;
    }

    return t_msg;
}

void assert_eq_join_message(_z_t_msg_join_t *left, _z_t_msg_join_t *right, uint8_t header) {
    printf("   Version (%u:%u)", left->_version, right->_version);
    assert(left->_version == right->_version);
    printf("\n");

    printf("   WhatAmI (%u:%u)", left->_whatami, right->_whatami);
    assert(left->_whatami == right->_whatami);
    printf("\n");

    printf("   ");
    assert(memcmp(left->_zid.id, right->_zid.id, 16) == 0);
    printf("\n");

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_JOIN_S) == true) {
        printf("   SN Resolution (%hhu:%hhu)", left->_seq_num_res, right->_seq_num_res);
        assert(left->_seq_num_res == right->_seq_num_res);
        printf("\n");

        printf("   Request ID Resolution (%hhu:%hhu)", left->_req_id_res, right->_req_id_res);
        assert(left->_req_id_res == right->_req_id_res);
        printf("\n");

        printf("   Batch Size (%hu:%hu)", left->_batch_size, right->_batch_size);
        assert(left->_batch_size == right->_batch_size);
        printf("\n");
    }

    printf("   Lease (%zu:%zu)", left->_lease, right->_lease);
    assert(left->_lease == right->_lease);
    printf("\n");

    printf("   Next SNs: ");
    // TODO[protocol]
    // if (_Z_HAS_FLAG(left->_options, _Z_OPT_JOIN_QOS) == true) {
    //     assert(left->_next_sn._is_qos == true);
    //     assert(right->_next_sn._is_qos == true);

    //     for (uint8_t i = 0; i < Z_PRIORITIES_NUM; i++) {
    //         printf("R:%zu:%zu ", left->_next_sn._val._qos[i]._reliable, right->_next_sn._val._qos[i]._reliable);
    //         assert(left->_next_sn._val._qos[i]._reliable == right->_next_sn._val._qos[i]._reliable);
    //         printf("B:%zu:%zu ", left->_next_sn._val._qos[i]._best_effort,
    //         right->_next_sn._val._qos[i]._best_effort); assert(left->_next_sn._val._qos[i]._best_effort ==
    //         right->_next_sn._val._qos[i]._best_effort);
    //     }
    //     printf("\n");
    // } else {
    // assert(left->_next_sn._is_qos == false);
    // assert(right->_next_sn._is_qos == false);

    printf("R: %zu:%zu", left->_next_sn._val._plain._reliable, right->_next_sn._val._plain._reliable);
    assert(left->_next_sn._val._plain._reliable == right->_next_sn._val._plain._reliable);
    printf("B: %zu:%zu", left->_next_sn._val._plain._best_effort, right->_next_sn._val._plain._best_effort);
    assert(left->_next_sn._val._plain._best_effort == right->_next_sn._val._plain._best_effort);
    printf("\n");
    // }
}

void join_message(void) {
    printf("\n>> Join message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_transport_message_t t_msg = gen_join_message();
    assert(_Z_MID(t_msg._header) == _Z_MID_T_JOIN);

    _z_t_msg_join_t e_it = t_msg._body._join;

    // Encode
    int8_t res = _z_join_encode(&wbf, t_msg._header, &t_msg._body._join);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_t_msg_join_t d_it;
    res = _z_join_decode(&d_it, &zbf, t_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_join_message(&e_it, &d_it, t_msg._header);

    // Free
    _z_t_msg_clear_join(&d_it);
    _z_t_msg_clear(&t_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Init Message ------------------*/
_z_transport_message_t gen_init_message(void) {
    z_whatami_t whatami = (gen_uint8() % 2) + 1;
    _z_id_t zid = gen_zid();

    _z_transport_message_t t_msg;
    if (gen_bool()) {
        t_msg = _z_t_msg_make_init_syn(whatami, zid);
    } else {
        _z_bytes_t cookie = gen_bytes(64);
        t_msg = _z_t_msg_make_init_ack(whatami, zid, cookie);
    }

    if (gen_bool()) {
        t_msg._body._init._batch_size = gen_uint16();
    }

    if (gen_bool()) {
        t_msg._body._init._seq_num_res = gen_uint8() % 4;
    }

    if (gen_bool()) {
        t_msg._body._init._req_id_res = gen_uint8() % 4;
    }

    return t_msg;
}

void assert_eq_init_message(_z_t_msg_init_t *left, _z_t_msg_init_t *right, uint8_t header) {
    printf("   Version (%u:%u)", left->_version, right->_version);
    assert(left->_version == right->_version);
    printf("\n");

    printf("   WhatAmI (%u:%u)", left->_whatami, right->_whatami);
    assert(left->_whatami == right->_whatami);
    printf("\n");

    printf("   ");
    assert(memcmp(left->_zid.id, right->_zid.id, 16) == 0);
    printf("\n");

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_INIT_S) == true) {
        printf("   SN Resolution (%hhu:%hhu)", left->_seq_num_res, right->_seq_num_res);
        assert(left->_seq_num_res == right->_seq_num_res);
        printf("\n");

        printf("   Request ID Resolution (%hhu:%hhu)", left->_req_id_res, right->_req_id_res);
        assert(left->_req_id_res == right->_req_id_res);
        printf("\n");

        printf("   Batch Size (%hu:%hu)", left->_batch_size, right->_batch_size);
        assert(left->_batch_size == right->_batch_size);
        printf("\n");
    }

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_INIT_A) == true) {
        printf("   ");
        assert_eq_uint8_array(&left->_cookie, &right->_cookie);
        printf("\n");
    }
}

void init_message(void) {
    printf("\n>> Init message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_transport_message_t t_msg = gen_init_message();
    assert(_Z_MID(t_msg._header) == _Z_MID_T_INIT);

    _z_t_msg_init_t e_it = t_msg._body._init;

    // Encode
    int8_t res = _z_init_encode(&wbf, t_msg._header, &e_it);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_t_msg_init_t d_it;
    res = _z_init_decode(&d_it, &zbf, t_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_init_message(&e_it, &d_it, t_msg._header);

    // Free
    _z_t_msg_clear_init(&d_it);
    _z_t_msg_clear(&t_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Open Message ------------------*/
_z_transport_message_t gen_open_message(void) {
    _z_zint_t lease = gen_bool() ? gen_zint() * 1000 : gen_zint();
    _z_zint_t initial_sn = gen_zint();

    if (gen_bool()) {
        _z_bytes_t cookie = gen_bytes(64);
        return _z_t_msg_make_open_syn(lease, initial_sn, cookie);
    } else {
        return _z_t_msg_make_open_ack(lease, initial_sn);
    }
}

void assert_eq_open_message(_z_t_msg_open_t *left, _z_t_msg_open_t *right, uint8_t header) {
    printf("   Lease (%zu:%zu)", left->_lease, right->_lease);
    assert(left->_lease == right->_lease);
    printf("\n");

    printf("   Initial SN (%zu:%zu)", left->_initial_sn, right->_initial_sn);
    assert(left->_initial_sn == right->_initial_sn);
    printf("\n");

    if (_Z_HAS_FLAG(header, _Z_FLAG_T_OPEN_A) == false) {
        printf("   ");
        assert_eq_uint8_array(&left->_cookie, &right->_cookie);
        printf("\n");
    }
}

void open_message(void) {
    printf("\n>> Open message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_transport_message_t t_msg = gen_open_message();
    assert(_Z_MID(t_msg._header) == _Z_MID_T_OPEN);

    _z_t_msg_open_t e_op = t_msg._body._open;

    // Encode
    int8_t res = _z_open_encode(&wbf, t_msg._header, &e_op);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_t_msg_open_t d_op;
    res = _z_open_decode(&d_op, &zbf, t_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_open_message(&e_op, &d_op, t_msg._header);

    // Free
    _z_t_msg_clear_open(&d_op);
    _z_t_msg_clear(&t_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Close Message ------------------*/
_z_transport_message_t gen_close_message(void) {
    uint8_t reason = gen_uint8();
    _Bool link_only = gen_bool();

    return _z_t_msg_make_close(reason, link_only);
}

void assert_eq_close_message(_z_t_msg_close_t *left, _z_t_msg_close_t *right, uint8_t header) {
    (void)(header);

    printf("   Reason (%u:%u)", left->_reason, right->_reason);
    assert(left->_reason == right->_reason);
    printf("\n");
}

void close_message(void) {
    printf("\n>> Close message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_transport_message_t t_msg = gen_close_message();
    assert(_Z_MID(t_msg._header) == _Z_MID_T_CLOSE);

    _z_t_msg_close_t e_cl = t_msg._body._close;

    // Encode
    int8_t res = _z_close_encode(&wbf, t_msg._header, &e_cl);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_t_msg_close_t d_cl;
    res = _z_close_decode(&d_cl, &zbf, t_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_close_message(&e_cl, &d_cl, t_msg._header);

    // Free
    _z_t_msg_clear_close(&d_cl);
    _z_t_msg_clear(&t_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ KeepAlive Message ------------------*/
_z_transport_message_t gen_keep_alive_message(void) { return _z_t_msg_make_keep_alive(); }

void assert_eq_keep_alive_message(_z_t_msg_keep_alive_t *left, _z_t_msg_keep_alive_t *right, uint8_t header) {
    (void)(left);
    (void)(right);
    (void)(header);
}

void keep_alive_message(void) {
    printf("\n>> KeepAlive message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_transport_message_t t_msg = gen_keep_alive_message();
    _z_t_msg_keep_alive_t e_ka = t_msg._body._keep_alive;
    assert(_Z_MID(t_msg._header) == _Z_MID_T_KEEP_ALIVE);

    // Encode
    int8_t res = _z_keep_alive_encode(&wbf, t_msg._header, &e_ka);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_t_msg_keep_alive_t d_ka;
    res = _z_keep_alive_decode(&d_ka, &zbf, t_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_keep_alive_message(&e_ka, &d_ka, t_msg._header);

    // Free
    _z_t_msg_clear_keep_alive(&d_ka);
    _z_t_msg_clear(&t_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Frame Message ------------------*/
_z_transport_message_t gen_frame_message(void) {
    _z_zint_t sn = gen_zint();
    _Bool is_reliable = gen_bool();

    _z_network_message_vec_t messages;
    _z_zint_t num = (gen_zint() % 4) + 1;
    messages = _z_network_message_vec_make(num);
    for (_z_zint_t i = 0; i < num; i++) {
        _z_network_message_t *p_zm = (_z_network_message_t *)z_malloc(sizeof(_z_network_message_t));
        *p_zm = gen_network_message();
        _z_network_message_vec_append(&messages, p_zm);
    }

    return _z_t_msg_make_frame(sn, messages, is_reliable);
}

void assert_eq_frame_message(_z_t_msg_frame_t *left, _z_t_msg_frame_t *right, uint8_t header) {
    (void)(header);

    printf("   SN (%zu:%zu)", left->_sn, right->_sn);
    assert(left->_sn == right->_sn);
    printf("\n");

    size_t l_len = _z_vec_len(&left->_messages);
    size_t r_len = _z_vec_len(&right->_messages);
    printf("   Lenght (%zu:%zu)", l_len, r_len);
    assert(l_len == r_len);

    for (size_t i = 0; i < l_len; i++)
        assert_eq_network_message((_z_network_message_t *)_z_vec_get(&left->_messages, i),
                                  (_z_network_message_t *)_z_vec_get(&right->_messages, i));
}

void frame_message(void) {
    printf("\n>> Frame message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_transport_message_t t_msg = gen_frame_message();
    assert(_Z_MID(t_msg._header) == _Z_MID_T_FRAME);

    _z_t_msg_frame_t e_fr = t_msg._body._frame;

    // Encode
    int8_t res = _z_frame_encode(&wbf, t_msg._header, &e_fr);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_t_msg_frame_t d_fr;
    res = _z_frame_decode(&d_fr, &zbf, t_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_frame_message(&e_fr, &d_fr, t_msg._header);

    // Frame
    _z_t_msg_clear_frame(&d_fr);
    _z_t_msg_clear(&t_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Fragment Message ------------------*/
_z_transport_message_t gen_fragment_message(void) {
    _z_zint_t sn = gen_zint();
    _Bool is_reliable = gen_bool();
    _Bool is_last = gen_bool();
    _z_bytes_t payload = gen_bytes(gen_uint16() % 60000);  // 60000 is just to make sure that we are not generating a
                                                           // payload bigger than what the fragment can hold
    return _z_t_msg_make_fragment(sn, payload, is_reliable, is_last);
}

void assert_eq_fragment_message(_z_t_msg_fragment_t *left, _z_t_msg_fragment_t *right, uint8_t header) {
    (void)(header);

    printf("   SN (%zu:%zu)", left->_sn, right->_sn);
    assert(left->_sn == right->_sn);
    printf("\n");

    assert_eq_payload(&left->_payload, &right->_payload);
}

void fragment_message(void) {
    printf("\n>> Fragment message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_transport_message_t t_msg = gen_fragment_message();
    assert(_Z_MID(t_msg._header) == _Z_MID_T_FRAGMENT);

    _z_t_msg_fragment_t e_fr = t_msg._body._fragment;

    // Encode
    int8_t res = _z_fragment_encode(&wbf, t_msg._header, &e_fr);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_t_msg_fragment_t d_fr;
    res = _z_fragment_decode(&d_fr, &zbf, t_msg._header);
    assert(res == _Z_RES_OK);

    assert_eq_fragment_message(&e_fr, &d_fr, t_msg._header);

    // Frame
    _z_t_msg_clear_fragment(&d_fr);
    _z_t_msg_clear(&t_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Transport Message ------------------*/
_z_transport_message_t gen_transport_message(void) {
    _z_transport_message_t e_tm;

    uint8_t mids[] = {_Z_MID_T_JOIN,       _Z_MID_T_INIT,  _Z_MID_T_OPEN,    _Z_MID_T_CLOSE,
                      _Z_MID_T_KEEP_ALIVE, _Z_MID_T_FRAME, _Z_MID_T_FRAGMENT};

    uint8_t i = gen_uint8() % (sizeof(mids) / sizeof(uint8_t));
    switch (mids[i]) {
        case _Z_MID_T_JOIN:
            e_tm = gen_join_message();
            break;
        case _Z_MID_T_INIT:
            e_tm = gen_init_message();
            break;
        case _Z_MID_T_OPEN:
            e_tm = gen_open_message();
            break;
        case _Z_MID_T_CLOSE:
            e_tm = gen_close_message();
            break;
        case _Z_MID_T_KEEP_ALIVE:
            e_tm = gen_keep_alive_message();
            break;
        case _Z_MID_T_FRAME:
            e_tm = gen_frame_message();
            break;
        case _Z_MID_T_FRAGMENT:
            e_tm = gen_fragment_message();
            break;
        default:
            assert(0);
            break;
    }

    return e_tm;
}

void assert_eq_transport_message(_z_transport_message_t *left, _z_transport_message_t *right) {
    // FIXME[protocol]: This is here to set the extensions flags that is only known at encoding time
    // Test message
    printf("   Header (%x:%x)", left->_header, right->_header);
    assert(left->_header == right->_header);
    printf("\n");

    switch (_Z_MID(left->_header)) {
        case _Z_MID_T_JOIN:
            assert_eq_join_message(&left->_body._join, &right->_body._join, left->_header);
            break;
        case _Z_MID_T_INIT:
            assert_eq_init_message(&left->_body._init, &right->_body._init, left->_header);
            break;
        case _Z_MID_T_OPEN:
            assert_eq_open_message(&left->_body._open, &right->_body._open, left->_header);
            break;
        case _Z_MID_T_CLOSE:
            assert_eq_close_message(&left->_body._close, &right->_body._close, left->_header);
            break;
        case _Z_MID_T_KEEP_ALIVE:
            assert_eq_keep_alive_message(&left->_body._keep_alive, &right->_body._keep_alive, left->_header);
            break;
        case _Z_MID_T_FRAME:
            assert_eq_frame_message(&left->_body._frame, &right->_body._frame, left->_header);
            break;
        case _Z_MID_T_FRAGMENT:
            assert_eq_fragment_message(&left->_body._fragment, &right->_body._fragment, left->_header);
            break;
        default:
            assert(0);
            break;
    }
}

void transport_message(void) {
    printf("\n>> Transport message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_transport_message_t e_tm = gen_transport_message();
    printf(" - ");
    print_transport_message_type(e_tm._header);
    printf("\n");

    // Encode
    int8_t res = _z_transport_message_encode(&wbf, &e_tm);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_transport_message_t d_sm;
    res = _z_transport_message_decode(&d_sm, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_transport_message(&e_tm, &d_sm);

    // Free
    _z_t_msg_clear(&e_tm);
    _z_t_msg_clear(&d_sm);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Scouting Message ------------------*/
_z_scouting_message_t gen_scouting_message(void) {
    _z_scouting_message_t e_sm;

    uint8_t mids[] = {_Z_MID_SCOUT, _Z_MID_HELLO};

    uint8_t i = gen_uint8() % (sizeof(mids) / sizeof(uint8_t));
    switch (mids[i]) {
        case _Z_MID_SCOUT:
            e_sm = gen_scout_message();
            break;
        case _Z_MID_HELLO:
            e_sm = gen_hello_message();
            break;
        default:
            assert(0);
            break;
    }

    return e_sm;
}

void assert_eq_scouting_message(_z_scouting_message_t *left, _z_scouting_message_t *right) {
    // FIXME[protocol]: This is here to set the extensions flags that is only known at encoding time
    // if (_z_msg_ext_vec_len(&left->_extensions) > (size_t)0) {
    //     left->_header |= _Z_FLAG_T_Z;
    // }

    // Test message
    printf("   Header (%x:%x)", left->_header, right->_header);
    assert(left->_header == right->_header);
    printf("\n");

    switch (_Z_MID(left->_header)) {
        case _Z_MID_SCOUT:
            assert_eq_scout_message(&left->_body._scout, &right->_body._scout, left->_header);
            break;
        case _Z_MID_HELLO:
            assert_eq_hello_message(&left->_body._hello, &right->_body._hello, left->_header);
            break;
        default:
            assert(0);
            break;
    }

    // size_t left_n_ext = _z_msg_ext_vec_len(&left->_extensions);
    // size_t right_n_ext = _z_msg_ext_vec_len(&right->_extensions);
    // printf("   # of extensions (%zu:%zu)", left_n_ext, right_n_ext);
    // assert(left_n_ext == right_n_ext);
    // for (size_t i = 0; i < left_n_ext; i++) {
    //     assert_eq_message_extension(_z_msg_ext_vec_get(&left->_extensions, i),
    //                                 _z_msg_ext_vec_get(&right->_extensions, i));
    // }
}

void scouting_message(void) {
    printf("\n>> Scouting message\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_scouting_message_t e_tm = gen_scouting_message();
    printf(" - ");
    print_scouting_message_type(e_tm._header);
    printf("\n");

    // Encode
    int8_t res = _z_scouting_message_encode(&wbf, &e_tm);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_scouting_message_t d_sm;
    res = _z_scouting_message_decode(&d_sm, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_scouting_message(&e_tm, &d_sm);

    // Free
    _z_s_msg_clear(&e_tm);
    _z_s_msg_clear(&d_sm);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Batch ------------------*/
void batch(void) {
    printf("\n>> Batch\n");
    uint8_t bef_num = (gen_uint8() % 3);
    uint8_t frm_num = 2 + (gen_uint8() % 3);
    uint8_t aft_num = (gen_uint8() % 3);
    uint8_t tot_num = bef_num + frm_num + aft_num;
    _z_wbuf_t wbf = gen_wbuf(tot_num * 1024);

    // Initialize
    _z_transport_message_t *e_tm = (_z_transport_message_t *)z_malloc(tot_num * sizeof(_z_transport_message_t));
    for (uint8_t i = 0; i < bef_num; i++) {
        // Initialize random transport message
        e_tm[i] = gen_transport_message();
        // Encode
        int8_t res = _z_transport_message_encode(&wbf, &e_tm[i]);
        assert(res == _Z_RES_OK);
        (void)(res);
    }
    for (uint8_t i = bef_num; i < bef_num + frm_num; i++) {
        // Initialize a frame message
        e_tm[i] = gen_frame_message();
        // Encode
        int8_t res = _z_transport_message_encode(&wbf, &e_tm[i]);
        assert(res == _Z_RES_OK);
        (void)(res);
    }
    for (uint8_t i = bef_num + frm_num; i < bef_num + frm_num + aft_num; i++) {
        // Initialize random transport message
        e_tm[i] = gen_transport_message();
        // Encode
        int8_t res = _z_transport_message_encode(&wbf, &e_tm[i]);
        assert(res == _Z_RES_OK);
        (void)(res);
    }

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    for (uint8_t i = 0; i < tot_num; i++) {
        _z_transport_message_t d_sm;
        int8_t res = _z_transport_message_decode(&d_sm, &zbf);
        assert(res == _Z_RES_OK);

        printf(" - ");
        print_transport_message_type(d_sm._header);
        printf("\n");
        assert_eq_transport_message(&e_tm[i], &d_sm);

        // Free
        _z_t_msg_clear(&e_tm[i]);
        _z_t_msg_clear(&d_sm);
    }

    z_free(e_tm);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Fragmentation ------------------*/
// _z_transport_message_t z_fragment_header(_Bool is_reliable, _Bool is_fragment, _Bool is_final, _z_zint_t sn) {
//     // Create the frame session message that carries the zenoh message
//     _z_transport_message_t t_msg;
//     t_msg._header = _Z_MID_T_FRAME;
//     t_msg._body._frame._sn = sn;

//     if (is_reliable == true) {
//         _Z_SET_FLAG(t_msg._header, _Z_FLAG_T_FRAGMENT_R);
//     };

//     if (is_fragment == true) {
//         _Z_SET_FLAG(t_msg._header, _Z_FLAG_T_F);

//         if (is_final == true) {
//             _Z_SET_FLAG(t_msg._header, _Z_FLAG_T_E);
//         }

//         // Do not add the payload
//         t_msg._body._frame._payload._fragment.len = 0;
//         t_msg._body._frame._payload._fragment.start = NULL;
//     } else {
//         // Do not allocate the vector containing the messages
//         t_msg._body._frame._payload._messages = _z_network_message_vec_make(0);
//     }

//     return t_msg;
// }

// void z_wbuf_prepare(_z_wbuf_t *wbf) {
//     // Clear the buffer for serialization
//     _z_wbuf_reset(wbf);

//     for (size_t i = 0; i < _z_wbuf_space_left(wbf); i++) _z_wbuf_put(wbf, 0xff, i);
// }

// int8_t z_serialize_zenoh_fragment(_z_wbuf_t *dst, _z_wbuf_t *src, _Bool is_reliable, size_t sn) {
//     // Assume first that this is not the final fragment
//     _Bool is_final = false;
//     do {
//         // Mark the buffer for the writing operation
//         size_t w_pos = _z_wbuf_get_wpos(dst);
//         // Get the frame header
//         _z_transport_message_t f_hdr = z_fragment_header(is_reliable, true, is_final, sn);
//         // Encode the frame header
//         int8_t res = _z_transport_message_encode(dst, &f_hdr);
//         if (res == _Z_RES_OK) {
//             size_t space_left = _z_wbuf_space_left(dst);
//             size_t bytes_left = _z_wbuf_len(src);
//             // Check if it is really the final fragment
//             if ((is_final == false) && (bytes_left <= space_left)) {
//                 // Revert the buffer
//                 _z_wbuf_set_wpos(dst, w_pos);
//                 // It is really the finally fragment, reserialize the header
//                 is_final = true;
//                 continue;
//             }
//             // Write the fragment
//             size_t to_copy = bytes_left <= space_left ? bytes_left : space_left;
//             printf("  -Bytes left: %zu, Space left: %zu, Fragment size: %zu, Is final: %d\n", bytes_left, space_left,
//                    to_copy, is_final);
//             return _z_wbuf_siphon(dst, src, to_copy);
//         } else {
//             return 0;
//         }
//     } while (1);
// }

// void fragmentation(void) {
//     printf("\n>> Fragmentation\n");
//     size_t len = 16;
//     _z_wbuf_t wbf = _z_wbuf_make(len, false);
//     _z_wbuf_t fbf = _z_wbuf_make(len, true);
//     _z_wbuf_t dbf = _z_wbuf_make(0, true);

//     _z_zenoh_message_t e_zm;

//     do {
//         _z_wbuf_reset(&fbf);
//         // Generate and serialize the message
//         e_zm = gen_zenoh_message();
//         _z_zenoh_message_encode(&fbf, &e_zm);
//         // Check that the message actually requires fragmentation
//         if (_z_wbuf_len(&fbf) <= len)
//             _z_msg_clear(&e_zm);
//         else
//             break;
//     } while (1);

//     // Fragment the message
//     _Bool is_reliable = gen_bool();
//     // Fix the sn resoulution to 128 in such a way it requires only 1 byte for encoding
//     size_t seq_num_res = 128;
//     size_t sn = seq_num_res - 1;

//     printf(" - Message serialized\n");
//     print_wbuf(&fbf);

//     printf(" - Start fragmenting\n");
//     while (_z_wbuf_len(&fbf) > 0) {
//         // Clear the buffer for serialization
//         z_wbuf_prepare(&wbf);

//         // Get the fragment sequence number
//         sn = (sn + 1) % seq_num_res;

//         size_t written = _z_wbuf_len(&fbf);
//         int8_t res = z_serialize_zenoh_fragment(&wbf, &fbf, is_reliable, sn);
//         assert(res == _Z_RES_OK);
//         (void)(res);
//         written = written - _z_wbuf_len(&fbf);

//         printf("  -Encoded Fragment: ");
//         print_wbuf(&wbf);

//         // Decode the message
//         _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);

//         _z_transport_message_t r_sm;
//         res = _z_transport_message_decode(&r_sm, &zbf);
//         assert(res == _Z_RES_OK);

//         _z_bytes_t fragment = r_sm._body._frame._payload._fragment;
//         printf("  -Decoded Fragment length: %zu\n", fragment.len);
//         assert(fragment.len == written);

//         // Create an iosli for decoding
//         _z_wbuf_write_bytes(&dbf, fragment.start, 0, fragment.len);

//         print_wbuf(&dbf);

//         // Free the read buffer
//         _z_zbuf_clear(&zbf);
//     }

//     printf(" - Start defragmenting\n");
//     print_wbuf(&dbf);
//     _z_zbuf_t zbf = _z_wbuf_to_zbuf(&dbf);
//     printf("   Defragmented: ");
//     print_iosli(&zbf._ios);
//     printf("\n");

//     _z_zenoh_message_t d_zm;
//     int8_t res = _z_zenoh_message_decode(&d_zm, &zbf);
//     assert(res == _Z_RES_OK);

//     assert_eq_zenoh_message(&e_zm, &d_zm);
//     _z_msg_clear(&e_zm);
//     _z_msg_clear(&d_zm);

//     _z_wbuf_clear(&dbf);
//     _z_wbuf_clear(&wbf);
//     _z_wbuf_clear(&fbf);
//     _z_zbuf_clear(&zbf);
// }

/*=============================*/
/*            Main             */
/*=============================*/
int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 1024);

    for (unsigned int i = 0; i < RUNS; i++) {
        printf("\n\n== RUN %u", i);

        // Message extensions
        unit_extension();
        zint_extension();
        zbuf_extension();
        message_extension();

        // Message fields
        payload_field();
        timestamp_field();
        subinfo_field();
        keyexpr_field();
        data_info_field();

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

        // Network messages
        push_message();
        request_message();
        response_message();
        response_final_message();

        // Transport messages
        join_message();
        init_message();
        open_message();
        close_message();
        keep_alive_message();
        frame_message();
        fragment_message();
        transport_message();
        batch();
        // fragmentation();

        // Scouting messages
        scout_message();
        hello_message();
    }

    return 0;
}
