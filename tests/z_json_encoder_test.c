//
// Copyright (c) 2026 ZettaScale Technology
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/utils/json_encoder.h"
#include "zenoh-pico/utils/result.h"

#undef NDEBUG
#include <assert.h>

#if Z_FEATURE_ADMIN_SPACE == 1

static void assert_bytes_eq_buf(const z_owned_bytes_t *b, const uint8_t *expected, size_t expected_len) {
    assert(b != NULL);
    assert(expected != NULL);

    assert(z_bytes_len(z_bytes_loan(b)) == expected_len);

    uint8_t *buf = (uint8_t *)z_malloc(expected_len);
    assert(buf != NULL);

    size_t n = _z_bytes_to_buf(&b->_val, buf, expected_len);
    assert(n == expected_len);

    assert(memcmp(buf, expected, expected_len) == 0);
    z_free(buf);
}

#define ASSERT_BYTES_EQ_LIT(bytes_ptr, lit) assert_bytes_eq_buf((bytes_ptr), (const uint8_t *)(lit), sizeof(lit) - 1)

static void test_finish_requires_done(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    z_owned_bytes_t out;

    // Nothing written => finish must fail (validate checks !done)
    assert(_z_json_encoder_finish(&je, &out) == _Z_ERR_INVALID);

    _z_json_encoder_clear(&je);
}

static void test_empty_object(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    z_owned_bytes_t out;

    assert(_z_json_encoder_start_object(&je) == _Z_RES_OK);
    assert(_z_json_encoder_end_object(&je) == _Z_RES_OK);
    assert(_z_json_encoder_finish(&je, &out) == _Z_RES_OK);

    ASSERT_BYTES_EQ_LIT(&out, "{}");
    z_bytes_drop(z_bytes_move(&out));
    _z_json_encoder_clear(&je);
}

static void test_simple_object_string(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    z_owned_bytes_t out;

    assert(_z_json_encoder_start_object(&je) == _Z_RES_OK);
    assert(_z_json_encoder_write_key(&je, "k") == _Z_RES_OK);
    assert(_z_json_encoder_write_string(&je, "v") == _Z_RES_OK);
    assert(_z_json_encoder_end_object(&je) == _Z_RES_OK);
    assert(_z_json_encoder_finish(&je, &out) == _Z_RES_OK);

    ASSERT_BYTES_EQ_LIT(&out, "{\"k\":\"v\"}");
    z_bytes_drop(z_bytes_move(&out));
    _z_json_encoder_clear(&je);
}

static void test_array_two_values(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    z_owned_bytes_t out;

    assert(_z_json_encoder_start_array(&je) == _Z_RES_OK);
    assert(_z_json_encoder_write_string(&je, "a") == _Z_RES_OK);
    assert(_z_json_encoder_write_string(&je, "b") == _Z_RES_OK);
    assert(_z_json_encoder_end_array(&je) == _Z_RES_OK);
    assert(_z_json_encoder_finish(&je, &out) == _Z_RES_OK);

    ASSERT_BYTES_EQ_LIT(&out, "[\"a\",\"b\"]");
    z_bytes_drop(z_bytes_move(&out));
    _z_json_encoder_clear(&je);
}

static void test_nested_object_array_numbers(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    z_owned_bytes_t out;

    assert(_z_json_encoder_start_object(&je) == _Z_RES_OK);
    assert(_z_json_encoder_write_key(&je, "a") == _Z_RES_OK);

    assert(_z_json_encoder_start_array(&je) == _Z_RES_OK);
    assert(_z_json_encoder_write_i64(&je, -1) == _Z_RES_OK);
    assert(_z_json_encoder_write_u64(&je, 2) == _Z_RES_OK);
    assert(_z_json_encoder_end_array(&je) == _Z_RES_OK);

    assert(_z_json_encoder_end_object(&je) == _Z_RES_OK);
    assert(_z_json_encoder_finish(&je, &out) == _Z_RES_OK);

    ASSERT_BYTES_EQ_LIT(&out, "{\"a\":[-1,2]}");
    z_bytes_drop(z_bytes_move(&out));
    _z_json_encoder_clear(&je);
}

static void test_boolean_null_double(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    z_owned_bytes_t out;

    assert(_z_json_encoder_start_array(&je) == _Z_RES_OK);
    assert(_z_json_encoder_write_boolean(&je, true) == _Z_RES_OK);
    assert(_z_json_encoder_write_boolean(&je, false) == _Z_RES_OK);
    assert(_z_json_encoder_write_null(&je) == _Z_RES_OK);
    assert(_z_json_encoder_write_double(&je, 1.25) == _Z_RES_OK);
    assert(_z_json_encoder_end_array(&je) == _Z_RES_OK);

    assert(_z_json_encoder_finish(&je, &out) == _Z_RES_OK);

    // write_double uses "%.17g" => 1.25
    ASSERT_BYTES_EQ_LIT(&out, "[true,false,null,1.25]");
    z_bytes_drop(z_bytes_move(&out));
    _z_json_encoder_clear(&je);
}

static void test_string_escaping_key_and_value(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    z_owned_bytes_t out;

    assert(_z_json_encoder_start_object(&je) == _Z_RES_OK);

    // key contains quote and backslash
    assert(_z_json_encoder_write_key(&je, "k\"\\") == _Z_RES_OK);

    // value contains newline and tab
    assert(_z_json_encoder_write_string(&je, "line1\n\tline2") == _Z_RES_OK);

    assert(_z_json_encoder_end_object(&je) == _Z_RES_OK);
    assert(_z_json_encoder_finish(&je, &out) == _Z_RES_OK);

    ASSERT_BYTES_EQ_LIT(&out, "{\"k\\\"\\\\\":\"line1\\n\\tline2\"}");
    z_bytes_drop(z_bytes_move(&out));
    _z_json_encoder_clear(&je);
}

static void test_invalid_write_key_outside_object(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    assert(_z_json_encoder_write_key(&je, "k") == _Z_ERR_INVALID);
    _z_json_encoder_clear(&je);
}

static void test_invalid_value_in_object_without_key(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);

    assert(_z_json_encoder_start_object(&je) == _Z_RES_OK);
    // In object, value not allowed until key written
    assert(_z_json_encoder_write_string(&je, "v") == _Z_ERR_INVALID);

    _z_json_encoder_clear(&je);
}

static void test_invalid_end_object_when_expect_value(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);

    assert(_z_json_encoder_start_object(&je) == _Z_RES_OK);
    assert(_z_json_encoder_write_key(&je, "k") == _Z_RES_OK);
    // Expecting value => cannot end object
    assert(_z_json_encoder_end_object(&je) == _Z_ERR_INVALID);

    _z_json_encoder_clear(&je);
}

static void test_depth_overflow(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);

    for (size_t i = 0; i < Z_JSON_MAX_DEPTH; i++) {
        assert(_z_json_encoder_start_array(&je) == _Z_RES_OK);
    }
    assert(_z_json_encoder_start_array(&je) == _Z_ERR_OVERFLOW);

    _z_json_encoder_clear(&je);
}

static void test_finish_resets_encoder_state(void) {
    _z_json_encoder_t je;
    assert(_z_json_encoder_empty(&je) == _Z_RES_OK);
    z_owned_bytes_t out1;
    z_owned_bytes_t out2;

    assert(_z_json_encoder_write_string(&je, "one") == _Z_RES_OK);
    assert(_z_json_encoder_finish(&je, &out1) == _Z_RES_OK);
    ASSERT_BYTES_EQ_LIT(&out1, "\"one\"");
    z_bytes_drop(z_bytes_move(&out1));

    // After finish, encoder should allow a new top-level value
    assert(_z_json_encoder_write_string(&je, "two") == _Z_RES_OK);
    assert(_z_json_encoder_finish(&je, &out2) == _Z_RES_OK);
    ASSERT_BYTES_EQ_LIT(&out2, "\"two\"");
    z_bytes_drop(z_bytes_move(&out2));

    _z_json_encoder_clear(&je);
}

int main(void) {
    test_finish_requires_done();
    test_empty_object();
    test_simple_object_string();
    test_array_two_values();
    test_nested_object_array_numbers();
    test_boolean_null_double();
    test_string_escaping_key_and_value();
    test_invalid_write_key_outside_object();
    test_invalid_value_in_object_without_key();
    test_invalid_end_object_when_expect_value();
    test_depth_overflow();
    test_finish_resets_encoder_state();
    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("Missing config token to build this test. This test requires: Z_FEATURE_ADMIN_SPACE\n");
    return 0;
}

#endif  // Z_FEATURE_ADMIN_SPACE == 1
