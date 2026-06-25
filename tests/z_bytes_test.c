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

#include "zenoh-pico/collections/bytes.h"

#undef NDEBUG
#include <assert.h>

void test_null_bytes(void) {
    _z_bytes_t b = _z_bytes_null();
    assert(_z_bytes_len(&b) == 0);
    assert(_z_bytes_is_empty(&b));
    assert(!_z_bytes_check(&b));
    assert(_z_bytes_num_slices(&b) == 0);
    _z_bytes_clear(&b);  // no crash
}

void test_slice(void) {
    uint8_t data[5] = {1, 2, 3, 4, 5};
    uint8_t data_out[5] = {0};
    _z_slice_t s = _z_slice_copy_from_buf(data, 5);
    _z_slice_t s_check = _z_slice_copy_from_buf(data, 5);
    _z_bytes_t b;
    _z_bytes_from_slice(&b, &s);  // takes ownership of `s`

    assert(_z_bytes_len(&b) == 5);
    assert(!_z_bytes_is_empty(&b));
    assert(_z_bytes_check(&b));
    assert(_z_bytes_num_slices(&b) == 1);
    assert(_z_slice_eq(_z_bytes_get_slice(&b, 0), &s_check));

    assert(_z_bytes_to_buf(&b, data_out, 5) == 5);
    assert(memcmp(data, data_out, 5) == 0);

    _z_bytes_clear(&b);
    _z_slice_clear(&s_check);
}

void test_append(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};
    uint8_t data_in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t data_out[10] = {0};

    // Build the three (sub)slices that, concatenated, yield data_in.
    _z_slice_t s1 = _z_slice_copy_from_buf(data1, 5);      // {1,2,3,4,5}
    _z_slice_t s2 = _z_slice_copy_from_buf(data2 + 2, 3);  // {6,7,8}
    _z_slice_t s3 = _z_slice_copy_from_buf(data3 + 1, 2);  // {9,10}

    // Keep copies to compare against after the slices are moved into `b`.
    _z_slice_t s1_check = _z_slice_copy_from_buf(data1, 5);
    _z_slice_t s2_check = _z_slice_copy_from_buf(data2 + 2, 3);
    _z_slice_t s3_check = _z_slice_copy_from_buf(data3 + 1, 2);

    _z_bytes_t b = _z_bytes_null();

    assert(_z_bytes_append_slice(&b, &s1) == _Z_RES_OK);
    assert(_z_bytes_append_slice(&b, &s2) == _Z_RES_OK);
    assert(_z_bytes_append_slice(&b, &s3) == _Z_RES_OK);

    assert(_z_bytes_len(&b) == 10);
    assert(!_z_bytes_is_empty(&b));
    assert(_z_bytes_check(&b));
    assert(_z_bytes_num_slices(&b) == 3);
    assert(_z_slice_eq(_z_bytes_get_slice(&b, 0), &s1_check));
    assert(_z_slice_eq(_z_bytes_get_slice(&b, 1), &s2_check));
    assert(_z_slice_eq(_z_bytes_get_slice(&b, 2), &s3_check));

    assert(_z_bytes_to_buf(&b, data_out, 15) == 10);
    assert(memcmp(data_in, data_out, 10) == 0);

    _z_bytes_clear(&b);
    _z_slice_clear(&s1_check);
    _z_slice_clear(&s2_check);
    _z_slice_clear(&s3_check);
}

void test_reader_read(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};
    uint8_t data_in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t data_out[10] = {0};

    _z_slice_t s1 = _z_slice_copy_from_buf(data1, 5);      // {1,2,3,4,5}
    _z_slice_t s2 = _z_slice_copy_from_buf(data2 + 2, 3);  // {6,7,8}
    _z_slice_t s3 = _z_slice_copy_from_buf(data3 + 1, 2);  // {9,10}

    _z_bytes_t b = _z_bytes_null();

    assert(_z_bytes_append_slice(&b, &s1) == _Z_RES_OK);
    assert(_z_bytes_append_slice(&b, &s2) == _Z_RES_OK);
    assert(_z_bytes_append_slice(&b, &s3) == _Z_RES_OK);

    _z_bytes_reader_t reader = _z_bytes_get_reader(&b);

    uint8_t out;
    assert(_z_bytes_reader_tell(&reader) == 0);
    _z_bytes_reader_read(&reader, &out, 1);
    assert(_z_bytes_reader_tell(&reader) == 1);
    assert(out == 1);

    _z_bytes_reader_read(&reader, data_out, 3);
    assert(_z_bytes_reader_tell(&reader) == 4);
    assert(memcmp(data_out, data_in + 1, 3) == 0);

    _z_bytes_reader_read(&reader, data_out, 6);
    assert(_z_bytes_reader_tell(&reader) == 10);
    assert(memcmp(data_out, data_in + 4, 6) == 0);

    _z_bytes_clear(&b);
}

void test_reader_seek(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};

    _z_slice_t s1 = _z_slice_copy_from_buf(data1, 5);      // {1,2,3,4,5}
    _z_slice_t s2 = _z_slice_copy_from_buf(data2 + 2, 3);  // {6,7,8}
    _z_slice_t s3 = _z_slice_copy_from_buf(data3 + 1, 2);  // {9,10}

    _z_bytes_t b = _z_bytes_null();

    assert(_z_bytes_append_slice(&b, &s1) == _Z_RES_OK);
    assert(_z_bytes_append_slice(&b, &s2) == _Z_RES_OK);
    assert(_z_bytes_append_slice(&b, &s3) == _Z_RES_OK);

    _z_bytes_reader_t reader = _z_bytes_get_reader(&b);

    assert(_z_bytes_reader_tell(&reader) == 0);
    assert(_z_bytes_reader_seek(&reader, 3, SEEK_CUR) == 0);
    assert(_z_bytes_reader_tell(&reader) == 3);
    assert(_z_bytes_reader_seek(&reader, 5, SEEK_CUR) == 0);
    assert(_z_bytes_reader_tell(&reader) == 8);
    assert(_z_bytes_reader_seek(&reader, 10, SEEK_CUR) != 0);

    assert(_z_bytes_reader_seek(&reader, 0, SEEK_SET) == 0);
    assert(_z_bytes_reader_tell(&reader) == 0);

    assert(_z_bytes_reader_seek(&reader, 3, SEEK_SET) == 0);
    assert(_z_bytes_reader_tell(&reader) == 3);
    assert(_z_bytes_reader_seek(&reader, 8, SEEK_SET) == 0);
    assert(_z_bytes_reader_tell(&reader) == 8);
    assert(_z_bytes_reader_seek(&reader, 20, SEEK_SET) != 0);
    assert(_z_bytes_reader_seek(&reader, -20, SEEK_SET) != 0);

    assert(_z_bytes_reader_seek(&reader, 0, SEEK_END) == 0);
    assert(_z_bytes_reader_tell(&reader) == 10);
    assert(_z_bytes_reader_seek(&reader, -3, SEEK_END) == 0);
    assert(_z_bytes_reader_tell(&reader) == 7);
    assert(_z_bytes_reader_seek(&reader, -8, SEEK_END) == 0);
    assert(_z_bytes_reader_tell(&reader) == 2);
    assert(_z_bytes_reader_seek(&reader, -10, SEEK_END) == 0);
    assert(_z_bytes_reader_tell(&reader) == 0);
    assert(_z_bytes_reader_seek(&reader, -20, SEEK_END) != 0);
    assert(_z_bytes_reader_seek(&reader, 5, SEEK_END) != 0);

    _z_bytes_clear(&b);
}

void test_writer(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};

    uint8_t data_out[13] = {1, 2, 3, 4, 5, 1, 2, 6, 7, 8, 3, 9, 10};

    _z_bytes_writer_t writer = _z_bytes_writer_empty();

    _z_bytes_writer_write_all(&writer, data1, 5);
    _z_bytes_writer_write_all(&writer, data2, 5);
    _z_bytes_writer_write_all(&writer, data3, 3);
    _z_bytes_t b = _z_bytes_writer_finish(&writer);

    // All `write_all` data accumulates in the contiguous cache and is flushed as a single slice.
    assert(_z_bytes_len(&b) == 13);
    assert(_z_bytes_num_slices(&b) == 1);

    assert(_z_bytes_get_slice(&b, 0)->len == 13);
    assert(memcmp(data_out, _z_bytes_get_slice(&b, 0)->start, 13) == 0);
    _z_bytes_clear(&b);
}

// Verifies the underlying variant representation: a single slice stays inline,
// and a second slice promotes the container to a vector. Also checks that copy
// and append_bytes preserve content across both representations.
void test_variant_representation(void) {
    uint8_t data[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    // Single slice: stored inline (still reported as a single slice).
    _z_bytes_t b = _z_bytes_null();
    _z_slice_t s0 = _z_slice_copy_from_buf(data, 4);
    assert(_z_bytes_append_slice(&b, &s0) == _Z_RES_OK);
    assert(_z_bytes_num_slices(&b) == 1);
    assert(_z_bytes_len(&b) == 4);

    // Appending a second slice promotes to a vector of two slices.
    _z_slice_t s1 = _z_slice_copy_from_buf(data + 4, 4);
    assert(_z_bytes_append_slice(&b, &s1) == _Z_RES_OK);
    assert(_z_bytes_num_slices(&b) == 2);
    assert(_z_bytes_len(&b) == 8);

    // Copy preserves the multi-slice content.
    _z_bytes_t copy = _z_bytes_null();
    assert(_z_bytes_copy(&copy, &b) == _Z_RES_OK);
    assert(_z_bytes_num_slices(&copy) == 2);
    uint8_t out[8] = {0};
    assert(_z_bytes_to_buf(&copy, out, 8) == 8);
    assert(memcmp(data, out, 8) == 0);

    // append_bytes onto an empty container moves wholesale (stays a 2-slice vec).
    _z_bytes_t dst = _z_bytes_null();
    assert(_z_bytes_append_bytes(&dst, &copy) == _Z_RES_OK);
    assert(_z_bytes_num_slices(&dst) == 2);
    assert(_z_bytes_num_slices(&copy) == 0);  // moved out

    _z_bytes_clear(&dst);
    _z_bytes_clear(&b);
}

int main(void) {
    test_null_bytes();
    test_slice();
    test_append();
    test_reader_read();
    test_reader_seek();
    test_writer();
    test_variant_representation();
    return 0;
}
