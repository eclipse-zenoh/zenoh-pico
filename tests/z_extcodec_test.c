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

#define ZENOH_PICO_TEST_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/extcodec.h"
#include "zenoh-pico/protocol/iobuf.h"

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

void print_message_extension_type(uint8_t header) {
    switch (_Z_EXT_ID(header)) {
        case _Z_MOCK_EXTENSION_UNIT:
            printf("Mock Extension UNIT");
            break;
        case _Z_MOCK_EXTENSION_ZINT:
            printf("Mock Extension ZINT");
            break;
        case _Z_MOCK_EXTENSION_ZBUF:
            printf("Mock Extension ZBUF");
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

uint8_t gen_uint8(void) { return z_random_u8() % 255; }

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

void assert_eq_zint_extension(_z_msg_ext_zint_t *left, _z_msg_ext_zint_t *right) { assert(left->_val == right->_val); }

void zint_extension(void) {
    printf("\n>> ZINT Extension\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_msg_ext_t ext = gen_zint_extension();
    assert(_Z_EXT_ENC(ext._header) == _Z_MSG_EXT_ENC_UNIT);

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
    assert_eq_uint8_array(&left->_val, &right->_val);
}

void zbuf_extension(void) {
    printf("\n>> ZBUF Extension\n");
    _z_wbuf_t wbf = gen_wbuf(65535);

    // Initialize
    _z_msg_ext_t ext = gen_zbuf_extension();
    assert(_Z_EXT_ENC(ext._header) == _Z_MSG_EXT_ENC_UNIT);

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
    int8_t res = _z_msg_ext_encode(&wbf, &e_me);
    assert(res == _Z_RES_OK);
    (void)(res);

    // Decode
    _z_zbuf_t zbf = _z_wbuf_to_zbuf(&wbf);
    _z_msg_ext_t d_me;
    res = _z_msg_ext_decode(&d_me, &zbf);
    assert(res == _Z_RES_OK);

    assert_eq_message_extension(&e_me, &d_me);

    // Free
    _z_msg_ext_clear(&e_me);
    _z_msg_ext_clear(&d_me);
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

        unit_extension();
        zint_extension();
        zbuf_extension();

        message_extension();
    }

    return 0;
}
