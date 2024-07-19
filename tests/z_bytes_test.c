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
    _z_bytes_drop(&b);  // no crush
}

void test_slice(void) {
    uint8_t data[5] = {1, 2, 3, 4, 5};
    uint8_t data_out[5] = {0};
    _z_slice_t s = _z_slice_wrap_copy(data, 5);
    _z_bytes_t b;
    _z_bytes_from_slice(&b, s);

    assert(_z_bytes_len(&b) == 5);
    assert(!_z_bytes_is_empty(&b));
    assert(_z_bytes_check(&b));
    assert(_z_bytes_num_slices(&b) == 1);
    assert(_z_slice_eq(_Z_RC_IN_VAL(&_z_bytes_get_slice(&b, 0)->slice), &s));

    assert(_z_bytes_to_buf(&b, data_out, 5) == 5);
    assert(memcmp(data, data_out, 5) == 0);

    _z_bytes_drop(&b);
}

void test_append(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};
    uint8_t data_in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t data_out[10] = {0};
    _z_arc_slice_t s1 = _z_arc_slice_wrap(_z_slice_wrap_copy(data1, 5), 0, 5);
    _z_arc_slice_t s2 = _z_arc_slice_wrap(_z_slice_wrap_copy(data2, 5), 2, 3);
    _z_arc_slice_t s3 = _z_arc_slice_wrap(_z_slice_wrap_copy(data3, 3), 1, 2);

    _z_bytes_t b = _z_bytes_null();

    _z_bytes_append_slice(&b, &s1);
    _z_bytes_append_slice(&b, &s2);
    _z_bytes_append_slice(&b, &s3);

    assert(_z_bytes_len(&b) == 10);
    assert(!_z_bytes_is_empty(&b));
    assert(_z_bytes_check(&b));
    assert(_z_bytes_num_slices(&b) == 3);
    assert(_z_slice_eq(_Z_RC_IN_VAL(&_z_bytes_get_slice(&b, 0)->slice), _Z_RC_IN_VAL(&s1.slice)));
    assert(_z_slice_eq(_Z_RC_IN_VAL(&_z_bytes_get_slice(&b, 1)->slice), _Z_RC_IN_VAL(&s2.slice)));
    assert(_z_slice_eq(_Z_RC_IN_VAL(&_z_bytes_get_slice(&b, 2)->slice), _Z_RC_IN_VAL(&s3.slice)));

    assert(_z_bytes_to_buf(&b, data_out, 15) == 10);
    assert(memcmp(data_in, data_out, 10) == 0);

    _z_bytes_drop(&b);
}

void test_reader_read(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};
    uint8_t data_in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t data_out[10] = {0};
    _z_arc_slice_t s1 = _z_arc_slice_wrap(_z_slice_wrap_copy(data1, 5), 0, 5);
    _z_arc_slice_t s2 = _z_arc_slice_wrap(_z_slice_wrap_copy(data2, 5), 2, 3);
    _z_arc_slice_t s3 = _z_arc_slice_wrap(_z_slice_wrap_copy(data3, 3), 1, 2);

    _z_bytes_t b = _z_bytes_null();

    _z_bytes_append_slice(&b, &s1);
    _z_bytes_append_slice(&b, &s2);
    _z_bytes_append_slice(&b, &s3);

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

    _z_bytes_drop(&b);
}

void test_reader_seek(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};
    _z_arc_slice_t s1 = _z_arc_slice_wrap(_z_slice_wrap_copy(data1, 5), 0, 5);
    _z_arc_slice_t s2 = _z_arc_slice_wrap(_z_slice_wrap_copy(data2, 5), 2, 3);
    _z_arc_slice_t s3 = _z_arc_slice_wrap(_z_slice_wrap_copy(data3, 3), 1, 2);

    _z_bytes_t b = _z_bytes_null();

    _z_bytes_append_slice(&b, &s1);
    _z_bytes_append_slice(&b, &s2);
    _z_bytes_append_slice(&b, &s3);

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

    _z_bytes_drop(&b);
}

void test_writer_no_cache(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};

    _z_bytes_t b = _z_bytes_null();
    _z_bytes_writer_t writer = _z_bytes_get_writer(&b, 0);

    _z_bytes_writer_write(&writer, data1, 5);
    assert(_z_bytes_len(&b) == 5);
    assert(_z_bytes_num_slices(&b) == 1);
    _z_bytes_writer_write(&writer, data2, 5);
    assert(_z_bytes_len(&b) == 10);
    assert(_z_bytes_num_slices(&b) == 2);
    _z_bytes_writer_write(&writer, data3, 3);
    assert(_z_bytes_len(&b) == 13);
    assert(_z_bytes_num_slices(&b) == 3);

    assert(memcmp(data1, _z_arc_slice_data(_z_bytes_get_slice(&b, 0)), 5) == 0);
    assert(memcmp(data2, _z_arc_slice_data(_z_bytes_get_slice(&b, 1)), 5) == 0);
    assert(memcmp(data3, _z_arc_slice_data(_z_bytes_get_slice(&b, 2)), 3) == 0);
    _z_bytes_drop(&b);
}

void test_writer_with_cache(void) {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};

    uint8_t data1_out[7] = {1, 2, 3, 4, 5, 1, 2};
    uint8_t data2_out[6] = {6, 7, 8, 3, 9, 10};
    _z_bytes_t b = _z_bytes_null();
    _z_bytes_writer_t writer = _z_bytes_get_writer(&b, 7);

    _z_bytes_writer_write(&writer, data1, 5);
    assert(_z_bytes_len(&b) == 5);
    assert(_z_bytes_num_slices(&b) == 1);
    _z_bytes_writer_write(&writer, data2, 5);
    assert(_z_bytes_len(&b) == 10);
    assert(_z_bytes_num_slices(&b) == 2);
    _z_bytes_writer_write(&writer, data3, 3);
    assert(_z_bytes_len(&b) == 13);
    assert(_z_bytes_num_slices(&b) == 2);

    assert(_z_arc_slice_len(_z_bytes_get_slice(&b, 0)) == 7);
    assert(_z_arc_slice_len(_z_bytes_get_slice(&b, 1)) == 6);
    assert(memcmp(data1_out, _z_arc_slice_data(_z_bytes_get_slice(&b, 0)), 7) == 0);
    assert(memcmp(data2_out, _z_arc_slice_data(_z_bytes_get_slice(&b, 1)), 6) == 0);
    _z_bytes_drop(&b);
}

int main(void) {
    test_null_bytes();
    test_slice();
    test_append();
    test_reader_read();
    test_reader_seek();
    test_writer_no_cache();
    test_writer_with_cache();
    return 0;
}
