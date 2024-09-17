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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zenoh-pico.h>

#undef NDEBUG
#include <assert.h>

#include "zenoh-pico/system/platform.h"

typedef struct kv_pair_t {
    const char *key;
    const char *value;
} kv_pair_t;

typedef struct kv_pairs_tx_t {
    const kv_pair_t *data;
    uint32_t len;
    uint32_t current_idx;
} kv_pairs_tx_t;

typedef struct kv_pair_decoded_t {
    z_owned_string_t key;
    z_owned_string_t value;
} kv_pair_decoded_t;

typedef struct kv_pairs_rx_t {
    kv_pair_decoded_t *data;
    uint32_t len;
    uint32_t current_idx;
} kv_pairs_rx_t;

static bool hashmap_iter(z_owned_bytes_t *kv_pair, void *context);
static bool iter_body(z_owned_bytes_t *b, void *context);
static void parse_hashmap(kv_pairs_rx_t *kvp, const z_loaned_bytes_t *hashmap);
static void drop_hashmap(kv_pairs_rx_t *kvp);

int main(void) {
    z_owned_bytes_t payload;
    z_owned_encoding_t encoding;
    (void)encoding;

    // Number types: uint8, uint16, uint32, uint64, int8, int16, int32, int64, float, double
    uint32_t input_u32 = 123456;
    uint32_t output_u32 = 0;
    z_bytes_serialize_from_uint32(&payload, input_u32);
    z_bytes_deserialize_into_uint32(z_loan(payload), &output_u32);
    assert(input_u32 == output_u32);
    z_drop(z_move(payload));
    // Corresponding encoding to be used in operations options like `z_put()`, `z_get()`, etc.
    encoding = ENCODING_ZENOH_UINT32;

    // String, also work with and z_owned_string_t
    const char *input_str = "test";
    z_owned_string_t output_string;
    z_bytes_serialize_from_str(&payload, input_str);
    z_bytes_deserialize_into_string(z_loan(payload), &output_string);
    assert(strncmp(input_str, z_string_data(z_loan(output_string)), strlen(input_str)) == 0);
    z_drop(z_move(payload));
    z_drop(z_move(output_string));
    // Corresponding encoding to be used in operations options like `z_put()`, `z_get()`, etc.
    encoding = ENCODING_ZENOH_STRING;

    // Bytes, also work with z_owned_slice_t
    const uint8_t input_bytes[] = {1, 2, 3, 4};
    z_owned_slice_t output_bytes;
    z_bytes_serialize_from_buf(&payload, input_bytes, sizeof(input_bytes));
    z_bytes_deserialize_into_slice(z_loan(payload), &output_bytes);
    assert(memcmp(input_bytes, z_slice_data(z_loan(output_bytes)), sizeof(input_bytes)) == 0);
    z_drop(z_move(payload));
    z_drop(z_move(output_bytes));
    // Corresponding encoding to be used in operations options like `z_put()`, `z_get()`, etc.
    encoding = ENCODING_ZENOH_BYTES;  // That's the default value

    // Writer reader
    uint8_t input_writer[] = {0, 1, 2, 3, 4};
    uint8_t output_reader[5] = {0};
    z_bytes_empty(&payload);
    z_bytes_writer_t writer = z_bytes_get_writer(z_bytes_loan_mut(&payload));
    z_bytes_writer_write_all(&writer, input_writer, 3);
    z_bytes_writer_write_all(&writer, input_writer + 3, 2);
    z_bytes_reader_t reader = z_bytes_get_reader(z_bytes_loan(&payload));
    z_bytes_reader_read(&reader, output_reader, sizeof(output_reader));
    assert(0 == memcmp(input_writer, output_reader, sizeof(output_reader)));
    z_drop(z_move(payload));

    // Iterator
    uint8_t result_iter[] = {0, 1, 2, 3, 4};
    uint8_t output_iter[5] = {0};
    uint8_t context = 0;
    z_bytes_from_iter(&payload, iter_body, (void *)(&context));
    z_bytes_iterator_t it = z_bytes_get_iterator(z_bytes_loan(&payload));

    z_owned_bytes_t current_item;
    size_t i = 0;
    while (z_bytes_iterator_next(&it, &current_item)) {
        z_bytes_deserialize_into_uint8(z_bytes_loan(&current_item), &output_reader[i]);
        z_bytes_drop(z_bytes_move(&current_item));
        i++;
    }
    assert(memcmp(output_iter, result_iter, sizeof(output_iter)));
    z_drop(z_move(payload));

    // Hash map
    kv_pair_t input_hashmap[1];
    input_hashmap[0] = (kv_pair_t){.key = "test_key", .value = "test_value"};
    kv_pairs_tx_t ctx = (kv_pairs_tx_t){.data = input_hashmap, .current_idx = 0, .len = 1};
    z_bytes_from_iter(&payload, hashmap_iter, (void *)&ctx);
    kv_pairs_rx_t output_hashmap = {
        .current_idx = 0, .len = 16, .data = (kv_pair_decoded_t *)malloc(16 * sizeof(kv_pair_decoded_t))};
    parse_hashmap(&output_hashmap, z_loan(payload));
    assert(strncmp(input_hashmap[0].key, _z_string_data(z_loan(output_hashmap.data[0].key)),
                   strlen(input_hashmap[0].key)) == 0);
    assert(strncmp(input_hashmap[0].value, _z_string_data(z_loan(output_hashmap.data[0].value)),
                   strlen(input_hashmap[0].value)) == 0);
    z_drop(z_move(payload));
    drop_hashmap(&output_hashmap);
    return 0;
}

static bool iter_body(z_owned_bytes_t *b, void *context) {
    uint8_t *val = (uint8_t *)context;
    if (*val >= 5) {
        return false;
    } else {
        z_bytes_serialize_from_uint8(b, *val);
    }
    *val = *val + 1;
    return true;
}

static bool hashmap_iter(z_owned_bytes_t *kv_pair, void *context) {
    kv_pairs_tx_t *kvs = (kv_pairs_tx_t *)(context);
    z_owned_bytes_t k, v;
    if (kvs->current_idx >= kvs->len) {
        return false;
    } else {
        z_bytes_serialize_from_str(&k, kvs->data[kvs->current_idx].key);
        z_bytes_serialize_from_str(&v, kvs->data[kvs->current_idx].value);
        z_bytes_from_pair(kv_pair, z_move(k), z_move(v));
        kvs->current_idx++;
        return true;
    }
}

static void parse_hashmap(kv_pairs_rx_t *kvp, const z_loaned_bytes_t *hashmap) {
    z_owned_bytes_t kv, first, second;
    z_bytes_iterator_t iter = z_bytes_get_iterator(hashmap);

    while (kvp->current_idx < kvp->len && z_bytes_iterator_next(&iter, &kv)) {
        z_bytes_deserialize_into_pair(z_loan(kv), &first, &second);
        z_bytes_deserialize_into_string(z_loan(first), &kvp->data[kvp->current_idx].key);
        z_bytes_deserialize_into_string(z_loan(second), &kvp->data[kvp->current_idx].value);
        z_bytes_drop(z_bytes_move(&first));
        z_bytes_drop(z_bytes_move(&second));
        z_bytes_drop(z_bytes_move(&kv));
        kvp->current_idx++;
    }
}

static void drop_hashmap(kv_pairs_rx_t *kvp) {
    for (size_t i = 0; i < kvp->current_idx; i++) {
        z_string_drop(z_string_move(&kvp->data[i].key));
        z_string_drop(z_string_move(&kvp->data[i].value));
    }
    z_free(kvp->data);
}
