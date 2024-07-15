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

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/protocol/codec/message.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#define ZENOH_PICO_TEST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/codec/declarations.h"
#include "zenoh-pico/protocol/codec/ext.h"
#include "zenoh-pico/protocol/codec/interest.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/interest.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/system/platform.h"

#undef NDEBUG
#include <assert.h>

#define RUNS 1000

#define _Z_MOCK_EXTENSION_UNIT 0x01
#define _Z_MOCK_EXTENSION_ZINT 0x02
#define _Z_MOCK_EXTENSION_ZBUF 0x03

#if defined(_WIN32) || defined(WIN32)
#pragma warning(push)
#pragma warning(disable : 4244 4267)  // Disable truncation warnings in MSVC,
                                      // as it is used voluntarily in this file when working with RNG
#endif

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

void print_uint8_array(_z_slice_t *arr) {
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

uint16_t gen_uint16(void) { return z_random_u16(); }

uint64_t gen_uint64(void) {
    uint64_t ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

uint32_t gen_uint32(void) {
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

_z_slice_t gen_slice(size_t len) {
    _z_slice_t arr;
    arr._is_alloc = true;
    arr.len = len;
    arr.start = NULL;
    if (len == 0) return arr;

    arr.start = (uint8_t *)z_malloc(sizeof(uint8_t) * len);
    for (_z_zint_t i = 0; i < len; i++) {
        ((uint8_t *)arr.start)[i] = gen_uint8() & 0x7f;  // 0b01111111
    }

    return arr;
}

_z_bytes_t gen_payload(size_t len) {
    _z_slice_t pld = gen_slice(len);
    _z_bytes_t b;
    _z_bytes_from_slice(&b, pld);

    return b;
}

_z_bytes_t gen_bytes(size_t len) {
    _z_slice_t s = gen_slice(len);
    _z_bytes_t b;
    _z_bytes_from_slice(&b, s);
    return b;
}

_z_id_t gen_zid(void) {
    _z_id_t id = _z_id_empty();
    uint8_t hash = 55;
    uint8_t len = gen_uint8() % 16;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = gen_uint8();
        id.id[i] = byte;
        hash ^= i * byte;
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

_z_string_svec_t gen_str_array(size_t size) {
    _z_string_svec_t sa = _z_string_svec_make(size);
    for (size_t i = 0; i < size; i++) {
        _z_string_t s = _z_string_make(gen_str(16));
        _z_string_svec_append(&sa, &s);
    }

    return sa;
}

_z_string_t gen_string(size_t len) { return _z_string_wrap(gen_str(len)); }

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

_z_encoding_t gen_encoding(void) {
    _z_encoding_t en;
    en.id = gen_uint16();
    if (gen_bool()) {
        en.schema = gen_string(16);
    } else {
        en.schema = _z_string_null();
    }
    return en;
}

_z_value_t gen_value(void) {
    _z_value_t val;
    val.encoding = gen_encoding();
    if (gen_bool()) {
        val.payload = _z_bytes_null();
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

void assert_eq_uint8_array(const _z_slice_t *left, const _z_slice_t *right) {
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

void assert_eq_str_array(_z_string_svec_t *left, _z_string_svec_t *right) {
    printf("Array -> ");
    printf("Length (%zu:%zu), ", left->_len, right->_len);

    assert(left->_len == right->_len);
    printf("Content (");
    for (size_t i = 0; i < left->_len; i++) {
        const char *l = _z_string_svec_get(left, i)->val;
        const char *r = _z_string_svec_get(right, i)->val;

        printf("%s:%s", l, r);
        if (i < left->_len - 1) printf(" ");

        assert(_z_str_eq(l, r) == true);
    }
    printf(")");
}

void assert_eq_locator_array(const _z_locator_array_t *left, const _z_locator_array_t *right) {
    printf("Locators -> ");
    printf("Length (%zu:%zu), ", left->_len, right->_len);

    assert(left->_len == right->_len);
    printf("Content (");
    for (size_t i = 0; i < left->_len; i++) {
        const _z_locator_t *l = &left->_val[i];
        const _z_locator_t *r = &right->_val[i];

        _z_string_t ls = _z_locator_to_string(l);
        _z_string_t rs = _z_locator_to_string(r);

        printf("%s:%s", ls.val, rs.val);
        if (i < left->_len - 1) printf(" ");

        _z_string_clear(&ls);
        _z_string_clear(&rs);

        assert(_z_locator_eq(l, r) == true);
    }
    printf(")");
}

/*=============================*/
/*      Zenoh Core Fields      */
/*=============================*/
void zint(void) {
    printf("\n>> ZINT\n");
    _z_wbuf_t wbf = gen_wbuf(9);

    // Initialize
    _z_zint_t e_z = gen_zint();

    // Encode
    int8_t res = _z_zsize_encode(&wbf, e_z);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_zint_t d_z;
    res = _z_zsize_decode(&d_z, &zbf);
    assert(res == _Z_RES_OK);
    assert(e_z == d_z);

    // Free
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
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
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

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
    printf("    ZINT (%ju:%ju), ", (uintmax_t)left->_val, (uintmax_t)right->_val);
    assert(left->_val == right->_val);
}

void zint_extension(void) {
    printf("\n>> ZINT Extension\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

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
    _z_slice_t val = gen_slice(gen_uint8());
    return _z_msg_ext_make_zbuf(_Z_MOCK_EXTENSION_ZBUF, val);
}

void assert_eq_zbuf_extension(_z_msg_ext_zbuf_t *left, _z_msg_ext_zbuf_t *right) {
    printf("    ZBUF (");
    assert_eq_uint8_array(&left->_val, &right->_val);
    printf(")");
}

void zbuf_extension(void) {
    printf("\n>> ZBUF Extension\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

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

/*=============================*/
/*       Message Fields        */
/*=============================*/
/*------------------ Payload field ------------------*/
void assert_eq_slice(const _z_slice_t *left, const _z_slice_t *right) { assert_eq_uint8_array(left, right); }

void assert_eq_string(const _z_string_t *left, const _z_string_t *right) {
    assert(left->len == right->len);
    if (left->len > 0) {
        assert(_z_str_eq(left->val, right->val) == true);
    }
}

void assert_eq_bytes(const _z_bytes_t *left, const _z_bytes_t *right) {
    size_t len_left = _z_bytes_len(left);
    size_t len_right = _z_bytes_len(right);
    printf("Array -> ");
    printf("Length (%zu:%zu), ", len_left, len_right);

    assert(len_left == len_right);
    printf("Content (");
    _z_bytes_reader_t reader_left = _z_bytes_get_reader(left);
    _z_bytes_reader_t reader_right = _z_bytes_get_reader(right);
    for (size_t i = 0; i < len_left; i++) {
        uint8_t l = 0, r = 0;
        _z_bytes_reader_read(&reader_left, &l, 1);
        _z_bytes_reader_read(&reader_right, &r, 1);

        printf("%02x:%02x", l, r);
        if (i < len_left - 1) printf(" ");

        assert(l == r);
    }
    printf(")");
}

void payload_field(void) {
    printf("\n>> Payload field\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_bytes_t e_pld = gen_payload(64);

    // Encode
    int8_t res = _z_bytes_encode(&wbf, &e_pld);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);

    _z_bytes_t d_pld;
    res = _z_bytes_decode(&d_pld, &zbf);
    assert(res == _Z_RES_OK);
    printf("   ");
    assert_eq_bytes(&e_pld, &d_pld);
    printf("\n");

    // Free
    _z_bytes_drop(&e_pld);
    _z_bytes_drop(&d_pld);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_source_info_t gen_source_info(void) {
    return (_z_source_info_t){
        ._id = gen_zid(), ._source_sn = (uint32_t)gen_uint64(), ._entity_id = (uint32_t)gen_uint64()};
}

void assert_eq_source_info(const _z_source_info_t *left, const _z_source_info_t *right) {
    assert(left->_source_sn == right->_source_sn);
    assert(left->_entity_id == right->_entity_id);
    assert(memcmp(left->_id.id, right->_id.id, 16) == 0);
}
void assert_eq_encoding(const _z_encoding_t *left, const _z_encoding_t *right) {
    assert(left->id == right->id);
    assert_eq_string(&left->schema, &right->schema);
}
void assert_eq_value(const _z_value_t *left, const _z_value_t *right) {
    assert_eq_encoding(&left->encoding, &right->encoding);
    assert_eq_bytes(&left->payload, &right->payload);
}
/*------------------ Timestamp field ------------------*/
_z_timestamp_t gen_timestamp(void) {
    _z_timestamp_t ts;
    ts.time = gen_uint64();
    for (size_t i = 0; i < 16; i++) {
        ts.id.id[i] = gen_uint8() & 0x7f;  // 0b01111111
    }
    return ts;
}

void assert_eq_timestamp(const _z_timestamp_t *left, const _z_timestamp_t *right) {
    printf("Timestamp -> ");
    printf("Time (%llu:%llu), ", (unsigned long long)left->time, (unsigned long long)right->time);
    assert(left->time == right->time);

    printf("ID (");
    for (int i = 0; i < 16; i++) {
        printf("%02x:%02x, ", left->id.id[i], right->id.id[i]);
    }
    printf(")\n");
    assert(memcmp(left->id.id, right->id.id, 16) == 0);
}
/*------------------ Timestamp field ------------------*/

void timestamp_field(void) {
    printf("\n>> Timestamp field\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

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
    printf("\n");
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
    sm.reliability = gen_bool() ? Z_RELIABILITY_RELIABLE : Z_RELIABILITY_BEST_EFFORT;

    return sm;
}

void assert_eq_subinfo(_z_subinfo_t *left, _z_subinfo_t *right) {
    printf("SubInfo -> ");
    printf("Reliable (%u:%u), ", left->reliability, right->reliability);
    assert(left->reliability == right->reliability);
}

/*------------------ ResKey field ------------------*/
_z_keyexpr_t gen_keyexpr(void) {
    _z_keyexpr_t key;
    key._id = gen_uint16();
    key._mapping._val = gen_uint8();
    _Bool is_numerical = gen_bool();
    if (is_numerical == true) {
        key._suffix = NULL;
        _z_keyexpr_set_owns_suffix(&key, false);
    } else {
        key._suffix = gen_str(gen_zint() % 16);
        _z_keyexpr_set_owns_suffix(&key, true);
    }
    return key;
}

void assert_eq_keyexpr(const _z_keyexpr_t *left, const _z_keyexpr_t *right) {
    printf("ResKey -> ");
    printf("ID (%u:%u), ", left->_id, right->_id);
    assert(left->_id == right->_id);
    assert(!(_z_keyexpr_has_suffix(*left) ^ _z_keyexpr_has_suffix(*right)));

    printf("Name (");
    if (_z_keyexpr_has_suffix(*left)) {
        printf("%s:%s", left->_suffix, right->_suffix);
        assert(_z_str_eq(left->_suffix, right->_suffix) == true);
    } else {
        printf("NULL:NULL");
    }
    printf(")");
}

void keyexpr_field(void) {
    printf("\n>> ResKey field\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

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
    assert_eq_keyexpr(&e_rk, &d_rk);
    printf("\n");

    // Free
    _z_keyexpr_clear(&e_rk);
    _z_keyexpr_clear(&d_rk);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*      Declaration Fields     */
/*=============================*/
/*------------------ Resource declaration ------------------*/
_z_decl_kexpr_t gen_resource_declaration(void) {
    return (_z_decl_kexpr_t){._id = gen_uint16(), ._keyexpr = gen_keyexpr()};
}

void assert_eq_resource_declaration(const _z_decl_kexpr_t *left, const _z_decl_kexpr_t *right) {
    printf("RID (%u:%u), ", left->_id, right->_id);
    assert(left->_id == right->_id);
    assert_eq_keyexpr(&left->_keyexpr, &right->_keyexpr);
}

void resource_declaration(void) {
    printf("\n>> Resource declaration\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_decl_kexpr_t e_rd = gen_resource_declaration();

    // Encode
    int8_t res = _z_decl_kexpr_encode(&wbf, &e_rd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_decl_kexpr_t d_rd;
    uint8_t e_hdr;
    _z_uint8_decode(&e_hdr, &zbf);
    res = _z_decl_kexpr_decode(&d_rd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_resource_declaration(&e_rd, &d_rd);
    printf("\n");

    // Free
    _z_keyexpr_clear(&e_rd._keyexpr);
    _z_keyexpr_clear(&d_rd._keyexpr);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Subscriber declaration ------------------*/
_z_decl_subscriber_t gen_subscriber_declaration(void) {
    _z_subinfo_t subinfo = gen_subinfo();
    _z_decl_subscriber_t e_sd = {._keyexpr = gen_keyexpr(),
                                 ._id = (uint32_t)gen_uint64(),
                                 ._ext_subinfo = {._reliable = subinfo.reliability == Z_RELIABILITY_RELIABLE}};
    return e_sd;
}

void assert_eq_subscriber_declaration(const _z_decl_subscriber_t *left, const _z_decl_subscriber_t *right) {
    assert_eq_keyexpr(&left->_keyexpr, &right->_keyexpr);
    assert(left->_id == right->_id);
    assert(left->_ext_subinfo._reliable == right->_ext_subinfo._reliable);
}

void subscriber_declaration(void) {
    printf("\n>> Subscriber declaration\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_decl_subscriber_t e_sd = gen_subscriber_declaration();
    // Encode
    int8_t res = _z_decl_subscriber_encode(&wbf, &e_sd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_decl_subscriber_t d_sd;
    uint8_t e_hdr;
    _z_uint8_decode(&e_hdr, &zbf);
    res = _z_decl_subscriber_decode(&d_sd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_subscriber_declaration(&e_sd, &d_sd);
    printf("\n");

    // Free
    _z_keyexpr_clear(&e_sd._keyexpr);
    _z_keyexpr_clear(&d_sd._keyexpr);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Queryable declaration ------------------*/
_z_decl_queryable_t gen_queryable_declaration(void) {
    _z_decl_queryable_t e_qd = {._keyexpr = gen_keyexpr(),
                                ._id = (uint32_t)gen_uint64(),
                                ._ext_queryable_info = {._complete = gen_uint8(), ._distance = gen_uint16()}};

    return e_qd;
}

void assert_eq_queryable_declaration(const _z_decl_queryable_t *left, const _z_decl_queryable_t *right) {
    assert_eq_keyexpr(&left->_keyexpr, &right->_keyexpr);

    printf("Complete (%u:%u), ", left->_ext_queryable_info._complete, right->_ext_queryable_info._complete);
    assert(left->_ext_queryable_info._complete == right->_ext_queryable_info._complete);
    printf("Distance (%u:%u), ", left->_ext_queryable_info._distance, right->_ext_queryable_info._distance);
    assert(left->_ext_queryable_info._distance == right->_ext_queryable_info._distance);
}

void queryable_declaration(void) {
    printf("\n>> Queryable declaration\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_decl_queryable_t e_qd = gen_queryable_declaration();

    // Encode
    int8_t res = _z_decl_queryable_encode(&wbf, &e_qd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_decl_queryable_t d_qd;
    uint8_t e_hdr = 0;
    _z_uint8_decode(&e_hdr, &zbf);
    res = _z_decl_queryable_decode(&d_qd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_queryable_declaration(&e_qd, &d_qd);
    printf("\n");

    // Free
    _z_keyexpr_clear(&e_qd._keyexpr);
    _z_keyexpr_clear(&d_qd._keyexpr);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Forget Resource declaration ------------------*/
_z_undecl_kexpr_t gen_forget_resource_declaration(void) {
    _z_undecl_kexpr_t e_frd;

    e_frd._id = gen_uint16();

    return e_frd;
}

void assert_eq_forget_resource_declaration(const _z_undecl_kexpr_t *left, const _z_undecl_kexpr_t *right) {
    printf("RID (%u:%u)", left->_id, right->_id);
    assert(left->_id == right->_id);
}

void forget_resource_declaration(void) {
    printf("\n>> Forget resource declaration\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_undecl_kexpr_t e_frd = gen_forget_resource_declaration();

    // Encode
    int8_t res = _z_undecl_kexpr_encode(&wbf, &e_frd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_undecl_kexpr_t d_frd;
    uint8_t hdr;
    _z_uint8_decode(&hdr, &zbf);
    res = _z_undecl_kexpr_decode(&d_frd, &zbf, hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_forget_resource_declaration(&e_frd, &d_frd);
    printf("\n");

    // Free
    // NOTE: forget_res_decl does not involve any heap allocation
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Forget Subscriber declaration ------------------*/
_z_undecl_subscriber_t gen_forget_subscriber_declaration(void) {
    _z_undecl_subscriber_t e_fsd = {._ext_keyexpr = gen_keyexpr(), ._id = (uint32_t)gen_uint64()};
    return e_fsd;
}

void assert_eq_forget_subscriber_declaration(const _z_undecl_subscriber_t *left, const _z_undecl_subscriber_t *right) {
    assert_eq_keyexpr(&left->_ext_keyexpr, &right->_ext_keyexpr);
    assert(left->_id == right->_id);
}

void forget_subscriber_declaration(void) {
    printf("\n>> Forget subscriber declaration\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_undecl_subscriber_t e_fsd = gen_forget_subscriber_declaration();

    // Encode
    int8_t res = _z_undecl_subscriber_encode(&wbf, &e_fsd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_undecl_subscriber_t d_fsd = {._id = 0, ._ext_keyexpr = {0}};
    uint8_t e_hdr = 0;
    _z_uint8_decode(&e_hdr, &zbf);
    res = _z_undecl_subscriber_decode(&d_fsd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_forget_subscriber_declaration(&e_fsd, &d_fsd);
    printf("\n");

    // Free
    _z_keyexpr_clear(&e_fsd._ext_keyexpr);
    _z_keyexpr_clear(&d_fsd._ext_keyexpr);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Forget Queryable declaration ------------------*/
_z_undecl_queryable_t gen_forget_queryable_declaration(void) {
    _z_undecl_queryable_t e_fqd = {._ext_keyexpr = gen_keyexpr(), ._id = (uint32_t)gen_zint()};
    return e_fqd;
}

void assert_eq_forget_queryable_declaration(const _z_undecl_queryable_t *left, const _z_undecl_queryable_t *right) {
    assert_eq_keyexpr(&left->_ext_keyexpr, &right->_ext_keyexpr);
    assert(left->_id == right->_id);
}

void forget_queryable_declaration(void) {
    printf("\n>> Forget queryable declaration\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_undecl_queryable_t e_fqd = gen_forget_queryable_declaration();

    // Encode
    int8_t res = _z_undecl_queryable_encode(&wbf, &e_fqd);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t e_hdr = 0;
    _z_uint8_decode(&e_hdr, &zbf);
    _z_undecl_queryable_t d_fqd = {._ext_keyexpr = _z_keyexpr_null()};
    res = _z_undecl_queryable_decode(&d_fqd, &zbf, e_hdr);
    assert(res == _Z_RES_OK);

    printf("   ");
    assert_eq_forget_queryable_declaration(&e_fqd, &d_fqd);
    printf("\n");

    // Free
    _z_keyexpr_clear(&e_fqd._ext_keyexpr);
    _z_keyexpr_clear(&d_fqd._ext_keyexpr);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Declaration ------------------*/
_z_declaration_t gen_declaration(void) {
    uint8_t decl[] = {
        _Z_DECL_KEXPR,        _Z_UNDECL_KEXPR,   _Z_DECL_SUBSCRIBER,
        _Z_UNDECL_SUBSCRIBER, _Z_DECL_QUERYABLE, _Z_UNDECL_QUERYABLE,
    };

    _z_declaration_t d;
    d._tag = decl[gen_uint8() % (sizeof(decl) / sizeof(uint8_t))];

    switch (d._tag) {
        case _Z_DECL_KEXPR: {
            d._body._decl_kexpr = gen_resource_declaration();
        } break;
        case _Z_UNDECL_KEXPR: {
            d._body._undecl_kexpr = gen_forget_resource_declaration();
        } break;
        case _Z_DECL_SUBSCRIBER: {
            d._body._decl_subscriber = gen_subscriber_declaration();
        } break;
        case _Z_UNDECL_SUBSCRIBER: {
            d._body._undecl_subscriber = gen_forget_subscriber_declaration();
        } break;
        case _Z_DECL_QUERYABLE: {
            d._body._decl_queryable = gen_queryable_declaration();
        } break;
        case _Z_UNDECL_QUERYABLE: {
            d._body._undecl_queryable = gen_forget_queryable_declaration();
        } break;
        default:
            assert(false);
    }

    return d;
}

void assert_eq_declaration(const _z_declaration_t *left, const _z_declaration_t *right) {
    printf("Declaration -> ");
    printf("Header (%x:%x), ", left->_tag, right->_tag);
    assert(left->_tag == right->_tag);

    switch (left->_tag) {
        case _Z_DECL_KEXPR:
            assert_eq_resource_declaration(&left->_body._decl_kexpr, &right->_body._decl_kexpr);
            break;
        case _Z_DECL_SUBSCRIBER:
            assert_eq_subscriber_declaration(&left->_body._decl_subscriber, &right->_body._decl_subscriber);
            break;
        case _Z_DECL_QUERYABLE:
            assert_eq_queryable_declaration(&left->_body._decl_queryable, &right->_body._decl_queryable);
            break;
        case _Z_UNDECL_KEXPR:
            assert_eq_forget_resource_declaration(&left->_body._undecl_kexpr, &right->_body._undecl_kexpr);
            break;
        case _Z_UNDECL_SUBSCRIBER:
            assert_eq_forget_subscriber_declaration(&left->_body._undecl_subscriber, &right->_body._undecl_subscriber);
            break;
        case _Z_UNDECL_QUERYABLE:
            assert_eq_forget_queryable_declaration(&left->_body._undecl_queryable, &right->_body._undecl_queryable);
            break;
        default:
            assert(false);
    }
}

/*------------------ Declare message ------------------*/
_z_network_message_t gen_declare_message(void) {
    _z_declaration_t declaration = gen_declaration();
    _Bool has_id = gen_bool();
    uint32_t id = gen_uint32();
    return _z_n_msg_make_declare(declaration, has_id, id);
}

void assert_eq_declare_message(_z_n_msg_declare_t *left, _z_n_msg_declare_t *right) {
    printf("   ");
    assert(left->has_interest_id == right->has_interest_id);
    if (left->has_interest_id) {
        assert(left->_interest_id == right->_interest_id);
    }
    assert_eq_declaration(&left->_decl, &right->_decl);
    printf("\n");
}

void declare_message(void) {
    printf("\n>> Declare message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_network_message_t n_msg = gen_declare_message();

    // Encode
    int8_t res = _z_network_message_encode(&wbf, &n_msg);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_network_message_t d_dcl = {0};
    res = _z_network_message_decode(&d_dcl, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_declare_message(&n_msg._body._declare, &d_dcl._body._declare);

    // Free
    _z_n_msg_clear(&d_dcl);
    _z_n_msg_clear(&n_msg);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*------------------ Interest ------------------*/
#define _Z_MSGCODEC_INTEREST_BASE_FLAGS_MASK 0x8f  // Used to remove R, C & F flags

_z_interest_t gen_interest(void) {
    _z_interest_t i = {0};
    _Bool is_final = gen_bool();  // To determine if interest is final or not
    // Generate interest id
    i._id = gen_uint32();
    printf("Gen interest %d\n", is_final);
    // Add regular interest data
    if (!is_final) {
        // Generate base flags
        i.flags = gen_uint8() & _Z_MSGCODEC_INTEREST_BASE_FLAGS_MASK;
        // Generate test cases
        _Bool is_restricted = gen_bool();
        uint8_t cf_type = gen_uint8() % 3;  // Flags must be c, f or cf
        switch (cf_type) {
            default:
            case 0:
                i.flags |= _Z_INTEREST_FLAG_CURRENT;
                break;
            case 1:
                i.flags |= _Z_INTEREST_FLAG_FUTURE;
                break;
            case 2:
                i.flags |= (_Z_INTEREST_FLAG_CURRENT | _Z_INTEREST_FLAG_FUTURE);
                break;
        }
        if (is_restricted) {
            i.flags |= _Z_INTEREST_FLAG_RESTRICTED;
            // Generate ke
            i._keyexpr = gen_keyexpr();
        }
    };
    return i;
}

_z_network_message_t gen_interest_message(void) {
    _z_interest_t interest = gen_interest();
    return _z_n_msg_make_interest(interest);
}

void assert_eq_interest(const _z_interest_t *left, const _z_interest_t *right) {
    printf("Interest: 0x%x, 0x%x, %u, %u\n", left->flags, right->flags, left->_id, right->_id);
    printf("Interest ke: %d, %d, %d, %d, %s, %s\n", left->_keyexpr._id, right->_keyexpr._id,
           left->_keyexpr._mapping._val, right->_keyexpr._mapping._val, left->_keyexpr._suffix,
           right->_keyexpr._suffix);
    assert(left->flags == right->flags);
    assert(left->_id == right->_id);
    assert_eq_keyexpr(&left->_keyexpr, &right->_keyexpr);
}
void interest_message(void) {
    printf("\n>> Interest message\n");
    // Init
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_network_message_t expected = gen_interest_message();
    // Encode
    assert(_z_n_interest_encode(&wbf, &expected._body._interest) == _Z_RES_OK);
    // Decode
    _z_n_msg_interest_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t header = _z_zbuf_read(&zbf);
    assert(_z_n_interest_decode(&decoded, &zbf, header) == _Z_RES_OK);
    // Check
    assert_eq_interest(&expected._body._interest._interest, &decoded._interest);
    // Clean-up
    _z_n_msg_interest_clear(&decoded);
    _z_n_msg_interest_clear(&expected._body._interest);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*        Zenoh Messages       */
/*=============================*/
/*------------------ Push message ------------------*/
_z_push_body_t gen_push_body(void) {
    _Bool isput = gen_bool();
    _z_timestamp_t ts = gen_bool() ? gen_timestamp() : _z_timestamp_null();
    _z_source_info_t sinfo = gen_bool() ? gen_source_info() : _z_source_info_null();
    _z_m_push_commons_t commons = {._source_info = sinfo, ._timestamp = ts};
    if (isput) {
        return (_z_push_body_t){._is_put = true,
                                ._body._put = {
                                    ._commons = commons,
                                    ._payload = gen_bytes(64),
                                    ._encoding = gen_encoding(),
                                }};
    } else {
        return (_z_push_body_t){._is_put = false, ._body._del = {._commons = commons}};
    }
}

void assert_eq_push_body(const _z_push_body_t *left, const _z_push_body_t *right) {
    assert(left->_is_put == right->_is_put);
    if (left->_is_put) {
        assert_eq_bytes(&left->_body._put._payload, &right->_body._put._payload);
        assert_eq_encoding(&left->_body._put._encoding, &right->_body._put._encoding);
        assert_eq_timestamp(&left->_body._put._commons._timestamp, &right->_body._put._commons._timestamp);
        assert_eq_source_info(&left->_body._put._commons._source_info, &right->_body._put._commons._source_info);
    } else {
        assert_eq_timestamp(&left->_body._del._commons._timestamp, &right->_body._del._commons._timestamp);
        assert_eq_source_info(&left->_body._del._commons._source_info, &right->_body._del._commons._source_info);
    }
}

void push_body_message(void) {
    printf("\n>> Put/Del message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);

    // Initialize
    _z_push_body_t e_da = gen_push_body();

    // Encode
    int8_t res = _z_push_body_encode(&wbf, &e_da);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_push_body_t d_da = {0};
    uint8_t header = _z_zbuf_read(&zbf);
    res = _z_push_body_decode(&d_da, &zbf, header);
    assert(res == _Z_RES_OK);

    assert_eq_push_body(&e_da, &d_da);

    // Free
    _z_push_body_clear(&d_da);
    _z_push_body_clear(&e_da);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_msg_query_t gen_query(void) {
    return (_z_msg_query_t){
        ._consolidation = (gen_uint8() % 4) - 1,
        ._ext_info = gen_source_info(),
        ._parameters = gen_slice(16),
        ._ext_value = gen_bool() ? gen_value() : _z_value_null(),
    };
}

void assert_eq_query(const _z_msg_query_t *left, const _z_msg_query_t *right) {
    assert(left->_consolidation == right->_consolidation);
    assert_eq_source_info(&left->_ext_info, &right->_ext_info);
    assert_eq_slice(&left->_parameters, &right->_parameters);
    assert_eq_value(&left->_ext_value, &right->_ext_value);
}

void query_message(void) {
    printf("\n>> Query message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_msg_query_t expected = gen_query();
    assert(_z_query_encode(&wbf, &expected) == _Z_RES_OK);
    _z_msg_query_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t header = _z_zbuf_read(&zbf);
    int8_t res = _z_query_decode(&decoded, &zbf, header);
    assert(_Z_RES_OK == res);
    assert_eq_query(&expected, &decoded);
    _z_msg_query_clear(&decoded);
    _z_msg_query_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_msg_err_t gen_err(void) {
    size_t len = 1 + gen_uint8();
    return (_z_msg_err_t){
        ._encoding = gen_encoding(),
        ._ext_source_info = gen_bool() ? gen_source_info() : _z_source_info_null(),
        ._payload = gen_payload(len),  // Hangs if 0
    };
}

void assert_eq_err(const _z_msg_err_t *left, const _z_msg_err_t *right) {
    assert_eq_encoding(&left->_encoding, &right->_encoding);
    assert_eq_source_info(&left->_ext_source_info, &right->_ext_source_info);
    assert_eq_bytes(&left->_payload, &right->_payload);
}

void err_message(void) {
    printf("\n>> Err message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_msg_err_t expected = gen_err();
    assert(_z_err_encode(&wbf, &expected) == _Z_RES_OK);
    _z_msg_err_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t header = _z_zbuf_read(&zbf);
    assert(_Z_RES_OK == _z_err_decode(&decoded, &zbf, header));
    assert_eq_err(&expected, &decoded);
    _z_msg_err_clear(&decoded);
    _z_msg_err_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_msg_reply_t gen_reply(void) {
    return (_z_msg_reply_t){
        ._consolidation = (gen_uint8() % 4) - 1,
        ._body = gen_push_body(),
    };
}

void assert_eq_reply(const _z_msg_reply_t *left, const _z_msg_reply_t *right) {
    assert(left->_consolidation == right->_consolidation);
    assert_eq_push_body(&left->_body, &right->_body);
}

void reply_message(void) {
    printf("\n>> Reply message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_msg_reply_t expected = gen_reply();
    assert(_z_reply_encode(&wbf, &expected) == _Z_RES_OK);
    _z_msg_reply_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t header = _z_zbuf_read(&zbf);
    assert(_Z_RES_OK == _z_reply_decode(&decoded, &zbf, header));
    assert_eq_reply(&expected, &decoded);
    _z_msg_reply_clear(&decoded);
    _z_msg_reply_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_n_msg_push_t gen_push(void) {
    return (_z_n_msg_push_t){
        ._body = gen_push_body(),
        ._key = gen_keyexpr(),
        ._qos = _z_n_qos_make(gen_bool(), gen_bool(), gen_uint8() % 8),
        ._timestamp = gen_timestamp(),
    };
}

void assert_eq_push(const _z_n_msg_push_t *left, const _z_n_msg_push_t *right) {
    assert_eq_timestamp(&left->_timestamp, &right->_timestamp);
    assert_eq_keyexpr(&left->_key, &right->_key);
    assert(left->_qos._val == right->_qos._val);
    assert_eq_push_body(&left->_body, &right->_body);
}

void push_message(void) {
    printf("\n>> Push message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_n_msg_push_t expected = gen_push();
    assert(_z_push_encode(&wbf, &expected) == _Z_RES_OK);
    _z_n_msg_push_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t header = _z_zbuf_read(&zbf);
    assert(_Z_RES_OK == _z_push_decode(&decoded, &zbf, header));
    assert_eq_push(&expected, &decoded);
    _z_n_msg_push_clear(&decoded);
    _z_n_msg_push_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_n_msg_request_t gen_request(void) {
    _z_n_msg_request_t request = {
        ._rid = gen_uint64(),
        ._key = gen_keyexpr(),
        ._ext_qos = gen_bool() ? _z_n_qos_make(gen_bool(), gen_bool(), gen_uint8() % 8) : _Z_N_QOS_DEFAULT,
        ._ext_timestamp = gen_bool() ? gen_timestamp() : _z_timestamp_null(),
        ._ext_target = gen_uint8() % 3,
        ._ext_budget = gen_bool() ? (uint32_t)gen_uint64() : 0,
        ._ext_timeout_ms = gen_bool() ? (uint32_t)gen_uint64() : 0,
    };
    switch (gen_uint8() % 2) {
        case 0: {
            request._tag = _Z_REQUEST_QUERY;
            request._body._query = gen_query();
        } break;
        case 1:
        default: {
            _z_push_body_t body = gen_push_body();
            if (body._is_put) {
                request._tag = _Z_REQUEST_PUT;
                request._body._put = body._body._put;
            } else {
                request._tag = _Z_REQUEST_DEL;
                request._body._del = body._body._del;
            }
        }
    }
    return request;
}

void assert_eq_request(const _z_n_msg_request_t *left, const _z_n_msg_request_t *right) {
    assert(left->_rid == right->_rid);
    assert_eq_keyexpr(&left->_key, &right->_key);
    assert(left->_ext_qos._val == right->_ext_qos._val);
    assert_eq_timestamp(&left->_ext_timestamp, &right->_ext_timestamp);
    assert(left->_ext_target == right->_ext_target);
    assert(left->_ext_budget == right->_ext_budget);
    assert(left->_ext_timeout_ms == right->_ext_timeout_ms);
    assert(left->_tag == right->_tag);
    switch (left->_tag) {
        case _Z_REQUEST_QUERY: {
            assert_eq_query(&left->_body._query, &right->_body._query);
        } break;
        case _Z_REQUEST_PUT: {
            assert_eq_push_body(&(_z_push_body_t){._is_put = true, ._body._put = left->_body._put},
                                &(_z_push_body_t){._is_put = true, ._body._put = right->_body._put});
        } break;
        case _Z_REQUEST_DEL: {
            assert_eq_push_body(&(_z_push_body_t){._is_put = false, ._body._del = left->_body._del},
                                &(_z_push_body_t){._is_put = false, ._body._del = right->_body._del});
        } break;
    }
}

void request_message(void) {
    printf("\n>> Request message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_n_msg_request_t expected = gen_request();
    assert(_z_request_encode(&wbf, &expected) == _Z_RES_OK);
    _z_n_msg_request_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t header = _z_zbuf_read(&zbf);
    int8_t ret = _z_request_decode(&decoded, &zbf, header);
    assert(_Z_RES_OK == ret);
    assert_eq_request(&expected, &decoded);
    _z_n_msg_request_clear(&decoded);
    _z_n_msg_request_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_n_msg_response_t gen_response(void) {
    _z_n_msg_response_t ret = {
        ._key = gen_keyexpr(),
        ._request_id = gen_uint64(),
        ._ext_qos = _z_n_qos_make(gen_bool(), gen_bool(), gen_uint8() % 8),
        ._ext_timestamp = gen_bool() ? gen_timestamp() : _z_timestamp_null(),
        ._ext_responder = {._eid = gen_uint16(), ._zid = gen_zid()},
    };
    switch (gen_uint32() % 2) {
        case 0: {
            ret._tag = _Z_RESPONSE_BODY_ERR;
            ret._body._err = gen_err();
        } break;
        case 1:
        default: {
            ret._tag = _Z_RESPONSE_BODY_REPLY;
            ret._body._reply = gen_reply();
        } break;
    }
    return ret;
}

void assert_eq_response(const _z_n_msg_response_t *left, const _z_n_msg_response_t *right) {
    assert_eq_keyexpr(&left->_key, &right->_key);
    assert_eq_timestamp(&left->_ext_timestamp, &right->_ext_timestamp);
    assert(left->_request_id == right->_request_id);
    assert(left->_ext_qos._val == right->_ext_qos._val);
    assert(left->_ext_responder._eid == right->_ext_responder._eid);
    assert(memcmp(left->_ext_responder._zid.id, right->_ext_responder._zid.id, 16) == 0);
    assert(left->_tag == right->_tag);
    switch (left->_tag) {
        case _Z_RESPONSE_BODY_REPLY: {
            assert_eq_reply(&left->_body._reply, &right->_body._reply);
        } break;
        case _Z_RESPONSE_BODY_ERR: {
            assert_eq_err(&left->_body._err, &right->_body._err);

        } break;
    }
}

void response_message(void) {
    printf("\n>> Response message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_n_msg_response_t expected = gen_response();
    assert(_z_response_encode(&wbf, &expected) == _Z_RES_OK);
    _z_n_msg_response_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t header = _z_zbuf_read(&zbf);
    int8_t ret = _z_response_decode(&decoded, &zbf, header);
    assert(_Z_RES_OK == ret);
    assert_eq_response(&expected, &decoded);
    _z_n_msg_response_clear(&decoded);
    _z_n_msg_response_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_n_msg_response_final_t gen_response_final(void) { return (_z_n_msg_response_final_t){._request_id = gen_uint64()}; }
void assert_eq_response_final(const _z_n_msg_response_final_t *left, const _z_n_msg_response_final_t *right) {
    assert(left->_request_id == right->_request_id);
}
void response_final_message(void) {
    printf("\n>> Request Final message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_n_msg_response_final_t expected = gen_response_final();
    assert(_z_response_final_encode(&wbf, &expected) == _Z_RES_OK);
    _z_n_msg_response_final_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    uint8_t header = _z_zbuf_read(&zbf);
    int8_t ret = _z_response_final_decode(&decoded, &zbf, header);
    assert(_Z_RES_OK == ret);
    assert_eq_response_final(&expected, &decoded);
    _z_n_msg_response_final_clear(&decoded);
    _z_n_msg_response_final_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_transport_message_t gen_join(void) {
    _z_conduit_sn_list_t conduit = {._is_qos = gen_bool()};
    if (conduit._is_qos) {
        for (int i = 0; i < Z_PRIORITIES_NUM; i++) {
            conduit._val._qos[i]._best_effort = gen_uint64();
            conduit._val._qos[i]._reliable = gen_uint64();
        }
    } else {
        conduit._val._plain._best_effort = gen_uint64();
        conduit._val._plain._reliable = gen_uint64();
    }
    return _z_t_msg_make_join(_z_whatami_from_uint8((gen_uint8() % 3)), gen_uint64(), gen_zid(), conduit);
}
void assert_eq_join(const _z_t_msg_join_t *left, const _z_t_msg_join_t *right) {
    assert(memcmp(left->_zid.id, right->_zid.id, 16) == 0);
    assert(left->_lease == right->_lease);
    assert(left->_batch_size == right->_batch_size);
    assert(left->_whatami == right->_whatami);
    assert(left->_req_id_res == right->_req_id_res);
    assert(left->_seq_num_res == right->_seq_num_res);
    assert(left->_version == right->_version);
    assert(left->_next_sn._is_qos == right->_next_sn._is_qos);
    if (left->_next_sn._is_qos) {
        for (int i = 0; i < Z_PRIORITIES_NUM; i++) {
            assert(left->_next_sn._val._qos[i]._best_effort == right->_next_sn._val._qos[i]._best_effort);
            assert(left->_next_sn._val._qos[i]._reliable == right->_next_sn._val._qos[i]._reliable);
        }
    } else {
        assert(left->_next_sn._val._plain._best_effort == right->_next_sn._val._plain._best_effort);
        assert(left->_next_sn._val._plain._reliable == right->_next_sn._val._plain._reliable);
    }
}
void join_message(void) {
    printf("\n>> Join message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_transport_message_t expected = gen_join();
    assert(_z_join_encode(&wbf, expected._header, &expected._body._join) == _Z_RES_OK);
    _z_t_msg_join_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_join_decode(&decoded, &zbf, expected._header);
    assert(_Z_RES_OK == ret);
    assert_eq_join(&expected._body._join, &decoded);
    _z_t_msg_join_clear(&decoded);
    _z_t_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_transport_message_t gen_init(void) {
    if (gen_bool()) {
        return _z_t_msg_make_init_syn(_z_whatami_from_uint8((gen_uint8() % 3)), gen_zid());
    } else {
        return _z_t_msg_make_init_ack(_z_whatami_from_uint8((gen_uint8() % 3)), gen_zid(), gen_slice(16));
    }
}
void assert_eq_init(const _z_t_msg_init_t *left, const _z_t_msg_init_t *right) {
    assert(left->_batch_size == right->_batch_size);
    assert(left->_req_id_res == right->_req_id_res);
    assert(left->_seq_num_res == right->_seq_num_res);
    assert_eq_slice(&left->_cookie, &right->_cookie);
    assert(memcmp(left->_zid.id, right->_zid.id, 16) == 0);
    assert(left->_version == right->_version);
    assert(left->_whatami == right->_whatami);
}
void init_message(void) {
    printf("\n>> Init message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_transport_message_t expected = gen_init();
    assert(_z_init_encode(&wbf, expected._header, &expected._body._init) == _Z_RES_OK);
    _z_t_msg_init_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_init_decode(&decoded, &zbf, expected._header);
    assert(_Z_RES_OK == ret);
    assert_eq_init(&expected._body._init, &decoded);
    _z_t_msg_init_clear(&decoded);
    _z_t_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_transport_message_t gen_open(void) {
    if (gen_bool()) {
        return _z_t_msg_make_open_syn(gen_uint32(), gen_uint32(), gen_slice(16));
    } else {
        return _z_t_msg_make_open_ack(gen_uint32(), gen_uint32());
    }
}
void assert_eq_open(const _z_t_msg_open_t *left, const _z_t_msg_open_t *right) {
    assert_eq_slice(&left->_cookie, &right->_cookie);
    assert(left->_initial_sn == right->_initial_sn);
    assert(left->_lease == right->_lease);
}
void open_message(void) {
    printf("\n>> open message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_transport_message_t expected = gen_open();
    assert(_z_open_encode(&wbf, expected._header, &expected._body._open) == _Z_RES_OK);
    _z_t_msg_open_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_open_decode(&decoded, &zbf, expected._header);
    assert(_Z_RES_OK == ret);
    assert_eq_open(&expected._body._open, &decoded);
    _z_t_msg_open_clear(&decoded);
    _z_t_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_transport_message_t gen_close(void) { return _z_t_msg_make_close(gen_uint8(), gen_bool()); }
void assert_eq_close(const _z_t_msg_close_t *left, const _z_t_msg_close_t *right) {
    assert(left->_reason == right->_reason);
}
void close_message(void) {
    printf("\n>> close message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_transport_message_t expected = gen_close();
    assert(_z_close_encode(&wbf, expected._header, &expected._body._close) == _Z_RES_OK);
    _z_t_msg_close_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_close_decode(&decoded, &zbf, expected._header);
    assert(_Z_RES_OK == ret);
    assert_eq_close(&expected._body._close, &decoded);
    _z_t_msg_close_clear(&decoded);
    _z_t_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_transport_message_t gen_keep_alive(void) { return _z_t_msg_make_keep_alive(); }
void assert_eq_keep_alive(const _z_t_msg_keep_alive_t *left, const _z_t_msg_keep_alive_t *right) {
    (void)left;
    (void)right;
}
void keep_alive_message(void) {
    printf("\n>> keep_alive message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_transport_message_t expected = gen_keep_alive();
    assert(_z_keep_alive_encode(&wbf, expected._header, &expected._body._keep_alive) == _Z_RES_OK);
    _z_t_msg_keep_alive_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_keep_alive_decode(&decoded, &zbf, expected._header);
    assert(_Z_RES_OK == ret);
    assert_eq_keep_alive(&expected._body._keep_alive, &decoded);
    _z_t_msg_keep_alive_clear(&decoded);
    _z_t_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}
_z_network_message_t gen_net_msg(void) {
    switch (gen_uint8() % 6) {
        default:
        case 0: {
            return gen_declare_message();
        } break;
        case 1: {
            return (_z_network_message_t){._tag = _Z_N_PUSH, ._body._push = gen_push()};
        } break;
        case 2: {
            return (_z_network_message_t){._tag = _Z_N_REQUEST, ._body._request = gen_request()};
        } break;
        case 3: {
            return (_z_network_message_t){._tag = _Z_N_RESPONSE, ._body._response = gen_response()};
        } break;
        case 4: {
            return (_z_network_message_t){._tag = _Z_N_RESPONSE_FINAL, ._body._response_final = gen_response_final()};
        } break;
        case 5: {
            return (_z_network_message_t){._tag = _Z_N_INTEREST, ._body._interest._interest = gen_interest()};
        } break;
    }
}
void assert_eq_net_msg(const _z_network_message_t *left, const _z_network_message_t *right) {
    assert(left->_tag == right->_tag);
    switch (left->_tag) {
        case _Z_N_DECLARE: {
            assert_eq_declaration(&left->_body._declare._decl, &right->_body._declare._decl);
            assert_eq_timestamp(&left->_body._declare._ext_timestamp, &right->_body._declare._ext_timestamp);
            assert(left->_body._declare._ext_qos._val == right->_body._declare._ext_qos._val);
        } break;
        case _Z_N_PUSH: {
            assert_eq_push(&left->_body._push, &right->_body._push);
        } break;
        case _Z_N_REQUEST: {
            assert_eq_request(&left->_body._request, &right->_body._request);
        } break;
        case _Z_N_RESPONSE: {
            assert_eq_response(&left->_body._response, &right->_body._response);
        } break;
        case _Z_N_RESPONSE_FINAL: {
            assert_eq_response_final(&left->_body._response_final, &right->_body._response_final);
        } break;
        case _Z_N_INTEREST: {
            assert_eq_interest(&left->_body._interest._interest, &right->_body._interest._interest);
        } break;
        default:
            assert(false);
            break;
    }
}
_z_network_message_vec_t gen_net_msgs(size_t n) {
    _z_network_message_vec_t ret = _z_network_message_vec_make(n);
    for (size_t i = 0; i < n; i++) {
        _z_network_message_t *msg = (_z_network_message_t *)z_malloc(sizeof(_z_network_message_t));
        memset(msg, 0, sizeof(_z_network_message_t));
        *msg = gen_net_msg();
        _z_network_message_vec_append(&ret, msg);
    }
    return ret;
}

_z_transport_message_t gen_frame(void) {
    return _z_t_msg_make_frame(gen_uint32(), gen_net_msgs(gen_uint8() % 16), gen_bool());
}
void assert_eq_frame(const _z_t_msg_frame_t *left, const _z_t_msg_frame_t *right) {
    assert(left->_sn == right->_sn);
    assert(left->_messages._len == right->_messages._len);
    for (size_t i = 0; i < left->_messages._len; i++) {
        assert_eq_net_msg(left->_messages._val[i], right->_messages._val[i]);
    }
}
void frame_message(void) {
    printf("\n>> frame message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_transport_message_t expected = gen_frame();
    assert(_z_frame_encode(&wbf, expected._header, &expected._body._frame) == _Z_RES_OK);
    _z_t_msg_frame_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_frame_decode(&decoded, &zbf, expected._header);
    assert(_Z_RES_OK == ret);
    assert_eq_frame(&expected._body._frame, &decoded);
    _z_t_msg_frame_clear(&decoded);
    _z_t_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_transport_message_t gen_fragment(void) {
    return _z_t_msg_make_fragment(gen_uint32(), gen_slice(gen_uint8()), gen_bool(), gen_bool());
}
void assert_eq_fragment(const _z_t_msg_fragment_t *left, const _z_t_msg_fragment_t *right) {
    assert(left->_sn == right->_sn);
    assert_eq_slice(&left->_payload, &right->_payload);
}
void fragment_message(void) {
    printf("\n>> fragment message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_transport_message_t expected = gen_fragment();
    assert(_z_fragment_encode(&wbf, expected._header, &expected._body._fragment) == _Z_RES_OK);
    _z_t_msg_fragment_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_fragment_decode(&decoded, &zbf, expected._header);
    assert(_Z_RES_OK == ret);
    assert_eq_fragment(&expected._body._fragment, &decoded);
    _z_t_msg_fragment_clear(&decoded);
    _z_t_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_transport_message_t gen_transport(void) {
    switch (gen_uint8() % 7) {
        case 0: {
            return gen_join();
        };
        case 1: {
            return gen_init();
        };
        case 2: {
            return gen_open();
        };
        case 3: {
            return gen_close();
        };
        case 4: {
            return gen_keep_alive();
        };
        case 5: {
            return gen_frame();
        };
        case 6:
        default: {
            return gen_fragment();
        };
    }
}
void assert_eq_transport(const _z_transport_message_t *left, const _z_transport_message_t *right) {
    assert(left->_header == right->_header);
    switch (_Z_MID(left->_header)) {
        case _Z_MID_T_JOIN: {
            assert_eq_join(&left->_body._join, &right->_body._join);
        } break;
        case _Z_MID_T_CLOSE: {
            assert_eq_close(&left->_body._close, &right->_body._close);
        } break;
        case _Z_MID_T_INIT: {
            assert_eq_init(&left->_body._init, &right->_body._init);
        } break;
        case _Z_MID_T_OPEN: {
            assert_eq_open(&left->_body._open, &right->_body._open);
        } break;
        case _Z_MID_T_KEEP_ALIVE: {
            assert_eq_keep_alive(&left->_body._keep_alive, &right->_body._keep_alive);
        } break;
        case _Z_MID_T_FRAME: {
            assert_eq_frame(&left->_body._frame, &right->_body._frame);
        } break;
        case _Z_MID_T_FRAGMENT: {
            assert_eq_fragment(&left->_body._fragment, &right->_body._fragment);
        } break;
        default:
            assert(false);
    }
}
void transport_message(void) {
    printf("\n>> transport message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_transport_message_t expected = gen_transport();
    assert(_z_transport_message_encode(&wbf, &expected) == _Z_RES_OK);
    _z_transport_message_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_transport_message_decode(&decoded, &zbf);
    assert(_Z_RES_OK == ret);
    assert_eq_transport(&expected, &decoded);
    _z_t_msg_clear(&decoded);
    _z_t_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

_z_scouting_message_t gen_scouting(void) {
    if (gen_bool()) {
        return _z_s_msg_make_scout((z_what_t)_z_whatami_from_uint8((gen_uint8() % 3)), gen_zid());
    } else {
        return _z_s_msg_make_hello(_z_whatami_from_uint8((gen_uint8() % 3)), gen_zid(),
                                   gen_locator_array((gen_uint8() % 16) + 1));
    }
}
void assert_eq_scouting(const _z_scouting_message_t *left, const _z_scouting_message_t *right) {
    assert(left->_header == right->_header);
    switch (_Z_MID(left->_header)) {
        case _Z_MID_SCOUT: {
            assert(left->_body._scout._version == right->_body._scout._version);
            assert(left->_body._scout._what == right->_body._scout._what);
            assert(memcmp(left->_body._scout._zid.id, right->_body._scout._zid.id, 16) == 0);
        } break;
        case _Z_MID_HELLO: {
            assert(left->_body._hello._version == right->_body._hello._version);
            assert(left->_body._hello._whatami == right->_body._hello._whatami);
            assert(memcmp(left->_body._hello._zid.id, right->_body._hello._zid.id, 16) == 0);
            assert_eq_locator_array(&left->_body._hello._locators, &right->_body._hello._locators);
        } break;
        default:
            assert(false);
    }
}
void scouting_message(void) {
    printf("\n>> scouting message\n");
    _z_wbuf_t wbf = gen_wbuf(UINT16_MAX);
    _z_scouting_message_t expected = gen_scouting();
    assert(_z_scouting_message_encode(&wbf, &expected) == _Z_RES_OK);
    _z_scouting_message_t decoded;
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    int8_t ret = _z_scouting_message_decode(&decoded, &zbf);
    assert(_Z_RES_OK == ret);
    assert_eq_scouting(&expected, &decoded);
    _z_s_msg_clear(&decoded);
    _z_s_msg_clear(&expected);
    _z_zbuf_clear(&zbf);
    _z_wbuf_clear(&wbf);
}

/*=============================*/
/*            Main             */
/*=============================*/
int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 1024);

    for (unsigned int i = 0; i < RUNS; i++) {
        printf("\n\n== RUN %u", i);

        // Core
        zint();

        // Message fields
        payload_field();
        timestamp_field();
        keyexpr_field();

        // Zenoh declarations
        resource_declaration();
        subscriber_declaration();
        queryable_declaration();
        forget_resource_declaration();
        forget_subscriber_declaration();
        forget_queryable_declaration();

        // Zenoh messages
        declare_message();
        push_body_message();
        query_message();
        err_message();
        reply_message();
        interest_message();

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

        // Scouting messages
        scouting_message();
    }

    return 0;
}

#if defined(_WIN32) || defined(WIN32)
#pragma warning(pop)
#endif
