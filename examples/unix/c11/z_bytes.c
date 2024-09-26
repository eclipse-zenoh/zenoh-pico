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
#include <zenoh-pico/api/serialization.h>

typedef struct custom_struct_t {
    float f;
    uint64_t u[2][3];
    const char *c;
} custom_struct_t;

typedef struct kv_pair_t {
    int32_t key;
    z_owned_string_t value;
} kv_pair_t;

static void print_slice_data(z_view_slice_t *slice);

int main(void) {
    // Wrapping raw data into z_bytes_t
    z_owned_bytes_t payload;
    {
        const uint8_t input_bytes[] = {1, 2, 3, 4};
        z_owned_slice_t output_bytes;
        z_bytes_copy_from_buf(&payload, input_bytes, sizeof(input_bytes));
        z_bytes_into_slice(z_loan(payload), &output_bytes);
        assert(memcmp(input_bytes, z_slice_data(z_loan(output_bytes)), z_slice_len(z_loan(output_bytes))) == 0);
        z_drop(z_move(payload));
        z_drop(z_move(output_bytes));

        // The same can be done for const char*
        const char *input_str = "test";
        z_owned_string_t output_string;
        z_bytes_copy_from_str(&payload, input_str);
        z_bytes_into_string(z_loan(payload), &output_string);
        assert(strncmp(input_str, z_string_data(z_loan(output_string)), z_string_len(z_loan(output_string))) == 0);
        z_drop(z_move(payload));
        z_drop(z_move(output_string));
    }

    // Serialization
    {
        // Arithmetic types: uint8, uint16, uint32, uint64, int8, int16, int32, int64, float, double
        uint32_t input_u32 = 1234;
        uint32_t output_u32 = 0;
        ze_serialize_from_uint32(&payload, input_u32);
        ze_deserialize_into_uint32(z_loan(payload), &output_u32);
        assert(input_u32 == output_u32);
        z_drop(z_move(payload));
        // Corresponding encoding to be used in operations options like `z_put()`, `z_get()`, etc.
        // const z_loaned_encoding* encoding = z_encoding_zenoh_uint32();
    }

    // Writer/reader for raw bytes
    {
        uint8_t input_writer[] = {0, 1, 2, 3, 4};
        uint8_t output_reader[5] = {0};
        z_bytes_empty(&payload);
        z_bytes_writer_t writer = z_bytes_get_writer(z_loan_mut(payload));
        z_bytes_writer_write_all(&writer, input_writer, 3);
        z_bytes_writer_write_all(&writer, input_writer + 3, 2);
        z_bytes_reader_t reader = z_bytes_get_reader(z_loan(payload));
        z_bytes_reader_read(&reader, output_reader, sizeof(output_reader));
        assert(0 == memcmp(input_writer, output_reader, sizeof(output_reader)));
        z_drop(z_move(payload));
    }

    // Using serializer/deserializer for composite types
    {
        // A sequence of primitive types
        int32_t input_vec[] = {1, 2, 3, 4};
        int32_t output_vec[4] = {0};
        ze_serializer_t serializer = ze_serializer(z_loan_mut(payload));
        ze_serializer_serialize_sequence_begin(&serializer, 4);
        for (size_t i = 0; i < 4; ++i) {
            ze_serializer_serialize_int32(&serializer, input_vec[i]);
        }
        ze_serializer_serialize_sequence_end(&serializer);

        ze_deserializer_t deserializer = ze_deserializer(z_loan(payload));
        size_t num_elements = 0;
        ze_deserializer_deserialize_sequence_begin(&deserializer, &num_elements);
        assert(num_elements == 4);
        for (size_t i = 0; i < num_elements; ++i) {
            ze_deserializer_deserialize_int32(&deserializer, &output_vec[i]);
        }
        ze_deserializer_deserialize_sequence_end(&deserializer);

        for (size_t i = 0; i < 4; ++i) {
            assert(input_vec[i] == output_vec[i]);
        }
        z_drop(z_move(payload));
    }

    {
        // Sequence of key-value pairs
        kv_pair_t kvs_input[2];
        kvs_input[0].key = 0;
        z_string_from_str(&kvs_input[0].value, "abc", NULL, NULL);
        kvs_input[1].key = 1;
        z_string_from_str(&kvs_input[1].value, "def", NULL, NULL);

        ze_serializer_t serializer = ze_serializer(z_loan_mut(payload));
        ze_serializer_serialize_sequence_begin(&serializer, 2);
        for (size_t i = 0; i < 2; ++i) {
            ze_serializer_serialize_int32(&serializer, kvs_input[i].key);
            ze_serializer_serialize_string(&serializer, z_loan(kvs_input[i].value));
        }
        ze_serializer_serialize_sequence_end(&serializer);

        ze_deserializer_t deserializer = ze_deserializer(z_loan(payload));
        size_t num_elements = 0;
        ze_deserializer_deserialize_sequence_begin(&deserializer, &num_elements);
        assert(num_elements == 2);
        kv_pair_t kvs_output[2];
        for (size_t i = 0; i < num_elements; ++i) {
            ze_deserializer_deserialize_int32(&deserializer, &kvs_output[i].key);
            ze_deserializer_deserialize_string(&deserializer, &kvs_output[i].value);
        }
        ze_deserializer_deserialize_sequence_end(&deserializer);

        for (size_t i = 0; i < 2; ++i) {
            assert(kvs_input[i].key == kvs_output[i].key);
            assert(strncmp(z_string_data(z_loan(kvs_input[i].value)), z_string_data(z_loan(kvs_output[i].value)),
                           z_string_len(z_loan(kvs_input[i].value))) == 0);
            z_drop(z_move(kvs_input[i].value));
            z_drop(z_move(kvs_output[i].value));
        }
        z_drop(z_move(payload));
    }

    {
        // Custom struct/tuple serializaiton
        custom_struct_t cs = (custom_struct_t){.f = 1.0f, .u = {{1, 2, 3}, {4, 5, 6}}, .c = "test"};

        ze_serializer_t serializer = ze_serializer(z_loan_mut(payload));
        ze_serializer_serialize_float(&serializer, cs.f);
        ze_serializer_serialize_sequence_begin(&serializer, 2);
        for (size_t i = 0; i < 2; ++i) {
            ze_serializer_serialize_sequence_begin(&serializer, 3);
            for (size_t j = 0; j < 3; ++j) {
                ze_serializer_serialize_uint64(&serializer, cs.u[i][j]);
            }
            ze_serializer_serialize_sequence_end(&serializer);
        }
        ze_serializer_serialize_sequence_end(&serializer);
        ze_serializer_serialize_str(&serializer, cs.c);

        float f = 0.0f;
        uint64_t u = 0;
        z_owned_string_t c;

        ze_deserializer_t deserializer = ze_deserializer(z_loan(payload));
        ze_deserializer_deserialize_float(&deserializer, &f);
        assert(f == cs.f);
        size_t num_elements0 = 0;
        ze_deserializer_deserialize_sequence_begin(&deserializer, &num_elements0);
        assert(num_elements0 == 2);
        for (size_t i = 0; i < 2; ++i) {
            size_t num_elements1 = 0;
            ze_deserializer_deserialize_sequence_begin(&deserializer, &num_elements1);
            assert(num_elements1 == 3);
            for (size_t j = 0; j < 3; ++j) {
                ze_deserializer_deserialize_uint64(&deserializer, &u);
                assert(u == cs.u[i][j]);
            }
            ze_deserializer_deserialize_sequence_end(&deserializer);
        }
        ze_deserializer_deserialize_sequence_end(&deserializer);
        ze_deserializer_deserialize_string(&deserializer, &c);
        assert(strncmp(cs.c, z_string_data(z_loan(c)), z_string_len(z_loan(c))) == 0);

        z_drop(z_move(c));
        z_drop(z_move(payload));
    }

    // Slice iterator
    {
        /// fill z_bytes with some data
        float input_vec[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
        ze_serializer_t serializer = ze_serializer(z_loan_mut(payload));
        ze_serializer_serialize_sequence_begin(&serializer, 5);
        for (size_t i = 0; i < 5; ++i) {
            ze_serializer_serialize_float(&serializer, input_vec[i]);
        }
        ze_serializer_serialize_sequence_end(&serializer);

        z_bytes_slice_iterator_t slice_iter = z_bytes_get_slice_iterator(z_bytes_loan(&payload));
        z_view_slice_t curr_slice;
        while (z_bytes_slice_iterator_next(&slice_iter, &curr_slice)) {
            printf("slice len: %d, slice data: '", (int)z_slice_len(z_view_slice_loan(&curr_slice)));
            print_slice_data(&curr_slice);
            printf("'\n");
        }
        z_drop(z_move(payload));
    }
    return 0;
}

static void print_slice_data(z_view_slice_t *slice) {
    for (size_t i = 0; i < z_slice_len(z_view_slice_loan(slice)); i++) {
        printf("0x%02x ", z_slice_data(z_view_slice_loan(slice))[i]);
    }
}
