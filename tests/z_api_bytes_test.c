//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#undef NDEBUG
#include <assert.h>

void test_reader_seek(void) {
    uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    z_owned_bytes_t payload;
    z_bytes_from_buf(&payload, data, 10, NULL, NULL);

    z_bytes_reader_t reader = z_bytes_get_reader(z_bytes_loan(&payload));
    assert(z_bytes_reader_tell(&reader) == 0);

    assert(0 == z_bytes_reader_seek(&reader, 5, SEEK_CUR));
    assert(z_bytes_reader_tell(&reader) == 5);

    assert(0 == z_bytes_reader_seek(&reader, 7, SEEK_SET));
    assert(z_bytes_reader_tell(&reader) == 7);

    assert(0 == z_bytes_reader_seek(&reader, -1, SEEK_END));
    assert(z_bytes_reader_tell(&reader) == 9);

    assert(z_bytes_reader_seek(&reader, 20, SEEK_SET) < 0);

    assert(0 == z_bytes_reader_seek(&reader, 5, SEEK_SET));
    assert(z_bytes_reader_tell(&reader) == 5);

    assert(z_bytes_reader_seek(&reader, 10, SEEK_CUR) < 0);
    assert(z_bytes_reader_seek(&reader, 10, SEEK_END) < 0);
    assert(z_bytes_reader_seek(&reader, -20, SEEK_END) < 0);

    z_bytes_drop(z_bytes_move(&payload));
}

void test_reader_read(void) {
    uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t data_out[10] = {0};

    z_owned_bytes_t payload;
    z_bytes_from_buf(&payload, data, 10, NULL, NULL);
    z_bytes_reader_t reader = z_bytes_get_reader(z_bytes_loan(&payload));

    assert(5 == z_bytes_reader_read(&reader, data_out, 5));

    z_bytes_reader_seek(&reader, 2, SEEK_CUR);
    assert(2 == z_bytes_reader_read(&reader, data_out + 7, 2));

    z_bytes_reader_seek(&reader, 5, SEEK_SET);
    assert(2 == z_bytes_reader_read(&reader, data_out + 5, 2));

    z_bytes_reader_seek(&reader, -1, SEEK_END);
    assert(1 == z_bytes_reader_read(&reader, data_out + 9, 10));

    assert(0 == z_bytes_reader_read(&reader, data_out, 10));

    assert(!memcmp(data, data_out, 10));

    z_bytes_drop(z_bytes_move(&payload));
}

void test_writer(void) {
    uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t data_out[10] = {0};

    z_owned_bytes_t payload;
    z_bytes_empty(&payload);

    z_bytes_writer_t writer = z_bytes_get_writer(z_bytes_loan_mut(&payload));

    assert(z_bytes_writer_write_all(&writer, data, 3) == 0);
    assert(z_bytes_writer_write_all(&writer, data + 3, 5) == 0);
    assert(z_bytes_writer_write_all(&writer, data + 8, 2) == 0);

    z_bytes_reader_t reader = z_bytes_get_reader(z_bytes_loan(&payload));

    assert(10 == z_bytes_reader_read(&reader, data_out, 10));
    assert(0 == memcmp(data, data_out, 10));

    z_bytes_drop(z_bytes_move(&payload));
}

void test_bounded(void) {
    uint32_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint32_t data_out[10] = {0};

    z_owned_bytes_t payload;
    z_bytes_empty(&payload);

    z_bytes_writer_t writer = z_bytes_get_writer(z_bytes_loan_mut(&payload));
    for (size_t i = 0; i < 10; ++i) {
        z_owned_bytes_t b;
        z_bytes_serialize_from_uint32(&b, data[i]);
        assert(z_bytes_writer_append_bounded(&writer, z_bytes_move(&b)) == 0);
    }
    {
        z_owned_bytes_t b;
        z_bytes_serialize_from_str(&b, "test");
        assert(z_bytes_writer_append_bounded(&writer, z_bytes_move(&b)) == 0);
    }

    z_bytes_reader_t reader = z_bytes_get_reader(z_bytes_loan(&payload));

    for (size_t i = 0; i < 10; ++i) {
        z_owned_bytes_t b;
        assert(z_bytes_reader_read_bounded(&reader, &b) == 0);
        assert(z_bytes_deserialize_into_uint32(z_bytes_loan(&b), &data_out[i]) == 0);
        z_bytes_drop(z_bytes_move(&b));
    }
    assert(!memcmp(data, data_out, 10));
    {
        z_owned_string_t s;
        z_owned_bytes_t b;
        assert(z_bytes_reader_read_bounded(&reader, &b) == 0);
        z_bytes_deserialize_into_string(z_bytes_loan(&b), &s);
        assert(strncmp("test", z_string_data(z_string_loan(&s)), z_string_len(z_string_loan(&s))) == 0);
        z_bytes_drop(z_bytes_move(&b));
        z_string_drop(z_string_move(&s));
    }
    uint8_t d;
    assert(0 == z_bytes_reader_read(&reader, &d, 1));  // we reached the end of the payload

    z_bytes_drop(z_bytes_move(&payload));
}

void test_append(void) {
    uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t data_out[10] = {0};

    z_owned_bytes_t payload;
    z_bytes_empty(&payload);

    z_bytes_writer_t writer = z_bytes_get_writer(z_bytes_loan_mut(&payload));
    z_bytes_writer_write_all(&writer, data, 5);
    {
        z_owned_bytes_t b;
        z_bytes_serialize_from_buf(&b, data + 5, 5);
        assert(z_bytes_writer_append(&writer, z_bytes_move(&b)) == 0);
    }

    z_bytes_reader_t reader = z_bytes_get_reader(z_bytes_loan(&payload));
    z_bytes_reader_read(&reader, data_out, 10);

    assert(!memcmp(data, data_out, 10));

    uint8_t d;
    assert(0 == z_bytes_reader_read(&reader, &d, 1));  // we reached the end of the payload

    z_bytes_drop(z_bytes_move(&payload));
}

void custom_deleter(void *data, void *context) {
    (void)data;
    size_t *cnt = (size_t *)context;
    (*cnt)++;
}

_Bool z_check_and_drop_payload(z_owned_bytes_t *payload, uint8_t *data, size_t len) {
    z_owned_slice_t out;
    z_bytes_deserialize_into_slice(z_bytes_loan(payload), &out);
    z_bytes_drop(z_bytes_move(payload));
    _Bool res = memcmp(data, z_slice_data(z_slice_loan(&out)), len) == 0;
    z_slice_drop(z_slice_move(&out));

    return res;
}

void test_slice(void) {
    uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    size_t cnt = 0;
    z_owned_bytes_t payload;
    z_bytes_from_buf(&payload, data, 10, custom_deleter, (void *)&cnt);

    z_owned_slice_t out;
    z_bytes_deserialize_into_slice(z_bytes_loan(&payload), &out);

    assert(cnt == 0);
    z_bytes_drop(z_bytes_move(&payload));
    assert(cnt == 1);

    assert(!memcmp(data, z_slice_data(z_slice_loan(&out)), 10));
    z_slice_drop(z_slice_move(&out));

    z_owned_bytes_t payload2;
    z_owned_slice_t s;
    z_slice_copy_from_buf(&s, data, 10);
    z_bytes_serialize_from_slice(&payload2, z_slice_loan(&s));
    assert(z_internal_slice_check(&s));
    z_slice_drop(z_slice_move(&s));
    assert(z_check_and_drop_payload(&payload2, data, 10));

    z_owned_bytes_t payload3;
    z_slice_copy_from_buf(&s, data, 10);
    z_bytes_from_slice(&payload3, z_slice_move(&s));
    assert(!z_internal_slice_check(&s));
    assert(z_check_and_drop_payload(&payload3, data, 10));

    z_owned_bytes_t payload4;
    z_bytes_serialize_from_buf(&payload4, data, 10);
    assert(z_check_and_drop_payload(&payload4, data, 10));

    z_owned_bytes_t payload5;
    z_bytes_from_static_buf(&payload5, data, 10);
    assert(z_check_and_drop_payload(&payload5, data, 10));
}

#define TEST_ARITHMETIC(TYPE, EXT, VAL)                               \
    {                                                                 \
        TYPE in = VAL, out;                                           \
        z_owned_bytes_t payload;                                      \
        z_bytes_serialize_from_##EXT(&payload, in);                   \
        z_bytes_deserialize_into_##EXT(z_bytes_loan(&payload), &out); \
        assert(in == out);                                            \
        z_bytes_drop(z_bytes_move(&payload));                         \
    }

void test_arithmetic(void) {
    TEST_ARITHMETIC(uint8_t, uint8, 5);
    TEST_ARITHMETIC(uint16_t, uint16, 1000);
    TEST_ARITHMETIC(uint32_t, uint32, 51000000);
    TEST_ARITHMETIC(uint64_t, uint64, 1000000000005);

    TEST_ARITHMETIC(int8_t, int8, 5);
    TEST_ARITHMETIC(int16_t, int16, -1000);
    TEST_ARITHMETIC(int32_t, int32, 51000000);
    TEST_ARITHMETIC(int64_t, int64, -1000000000005);

    TEST_ARITHMETIC(float, float, 10.1f);
    TEST_ARITHMETIC(double, double, -105.001);
}

bool iter_body(z_owned_bytes_t *b, void *context) {
    uint8_t *val = (uint8_t *)context;
    if (*val >= 10) {
        return false;
    } else {
        z_bytes_serialize_from_uint8(b, *val);
    }
    *val = *val + 1;
    return true;
}

void test_iter(void) {
    uint8_t data_out[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    z_owned_bytes_t payload;
    uint8_t context = 0;
    z_bytes_from_iter(&payload, iter_body, (void *)(&context));

    z_bytes_iterator_t it = z_bytes_get_iterator(z_bytes_loan(&payload));

    size_t i = 0;
    z_owned_bytes_t out;
    while (z_bytes_iterator_next(&it, &out)) {
        uint8_t res;
        z_bytes_deserialize_into_uint8(z_bytes_loan(&out), &res);
        assert(res == data_out[i]);
        i++;
        z_bytes_drop(z_bytes_move(&out));
    }
    assert(i == 10);
    z_bytes_drop(z_bytes_move(&payload));
}

void test_pair(void) {
    z_owned_bytes_t payload, payload1, payload2, payload1_out, payload2_out;
    z_bytes_serialize_from_int16(&payload1, -500);
    z_bytes_serialize_from_double(&payload2, 123.45);
    z_bytes_from_pair(&payload, z_bytes_move(&payload1), z_bytes_move(&payload2));

    z_bytes_deserialize_into_pair(z_bytes_loan(&payload), &payload1_out, &payload2_out);

    int16_t i;
    double d;
    z_bytes_deserialize_into_int16(z_bytes_loan(&payload1_out), &i);
    z_bytes_deserialize_into_double(z_bytes_loan(&payload2_out), &d);

    assert(i == -500);
    assert(d == 123.45);
}

int main(void) {
    test_reader_seek();
    test_reader_read();
    test_writer();
    test_slice();
    test_arithmetic();
    test_bounded();
    test_append();
    test_iter();
    test_pair();
}
