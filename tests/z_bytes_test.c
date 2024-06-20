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

void test_null_bytes() {
    _zz_bytes_t b = _zz_bytes_null();
    assert(_zz_bytes_len(&b) == 0);
    assert(_zz_bytes_is_empty(&b));
    assert(!_zz_bytes_check(&b));
    assert(_zz_bytes_num_slices(&b) == 0);
    _zz_bytes_clear(&b); // no crush
}

void test_slice() {
    uint8_t data[5] = {1, 2, 3, 4, 5};
    uint8_t data_out[5] = {};
    _z_slice_t s = _z_slice_wrap_copy(data, 5);
    _zz_bytes_t b = _zz_bytes_from_slice(s);

    assert(_zz_bytes_len(&b) == 5);
    assert(!_zz_bytes_is_empty(&b));
    assert(_zz_bytes_check(&b));
    assert(_zz_bytes_num_slices(&b) == 1);
    assert(_z_slice_eq(&_zz_bytes_get_slice(&b, 0)->slice.in->val, &s));

    _zz_bytes_to_buf(&b, data_out, 5);
    assert(memcmp(data, data_out, 5) == 0);
    
    _zz_bytes_clear(&b);
}

void test_append() {
    uint8_t data1[5] = {1, 2, 3, 4, 5};
    uint8_t data2[5] = {1, 2, 6, 7, 8};
    uint8_t data3[3] = {3, 9, 10};
    uint8_t data_in[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t data_out[10] = {};
    _z_arc_slice_t s1 = _z_arc_slice_wrap(_z_slice_wrap_copy(data1, 5), 0, 5);
    _z_arc_slice_t s2 = _z_arc_slice_wrap(_z_slice_wrap_copy(data2, 5), 2, 3);
    _z_arc_slice_t s3 = _z_arc_slice_wrap(_z_slice_wrap_copy(data3, 3), 1, 2);

    _zz_bytes_t b = _zz_bytes_null();
   
    _zz_bytes_append_slice(&b, &s1);
    _zz_bytes_append_slice(&b, &s2);
    _zz_bytes_append_slice(&b, &s3);

    printf("elements: %zu, ", _zz_bytes_len(&b));
    assert(_zz_bytes_len(&b) == 10);
    assert(!_zz_bytes_is_empty(&b));
    assert(_zz_bytes_check(&b));
    assert(_zz_bytes_num_slices(&b) == 3);
    assert(_z_slice_eq(&_zz_bytes_get_slice(&b, 0)->slice.in->val, &s1.slice.in->val));
    assert(_z_slice_eq(&_zz_bytes_get_slice(&b, 0)->slice.in->val, &s2.slice.in->val));
    assert(_z_slice_eq(&_zz_bytes_get_slice(&b, 0)->slice.in->val, &s3.slice.in->val));

    _zz_bytes_to_buf(&b, data_out, 10);
    assert(memcmp(data_in, data_out, 10) == 0);
    
    _zz_bytes_clear(&b);
}

int main(void) {
    test_null_bytes();
    test_slice();
    test_append();
    return 0;
}
