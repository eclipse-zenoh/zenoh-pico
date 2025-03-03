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
#include <assert.h>
#include <stddef.h>

#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/collections/bytes.h"

#undef NDEBUG
#include <assert.h>

#define SEND(closure, v)                                                                            \
    do {                                                                                            \
        _z_bytes_t payload;                                                                         \
        _z_bytes_from_slice(&payload, (_z_slice_t){.start = (const uint8_t *)v, .len = strlen(v)}); \
        z_loaned_sample_t sample = {                                                                \
            .keyexpr = _z_rname("key"),                                                             \
            .payload = payload,                                                                     \
            .timestamp = _z_timestamp_null(),                                                       \
            .encoding = _z_encoding_null(),                                                         \
            .kind = 0,                                                                              \
            .qos = {0},                                                                             \
            .attachment = _z_bytes_null(),                                                          \
        };                                                                                          \
        z_call(*z_loan(closure), &sample);                                                          \
    } while (0);

#define _RECV(handler, method, buf)                                             \
    do {                                                                        \
        z_owned_sample_t sample;                                                \
        z_result_t res = method(z_loan(handler), &sample);                      \
        if (res == Z_CHANNEL_DISCONNECTED) {                                    \
            strcpy(buf, "closed");                                              \
        } else if (res == Z_OK) {                                               \
            z_owned_slice_t value;                                              \
            z_bytes_to_slice(z_sample_payload(z_loan(sample)), &value);         \
            size_t value_len = z_slice_len(z_loan(value));                      \
            strncpy(buf, (const char *)z_slice_data(z_loan(value)), value_len); \
            buf[value_len] = '\0';                                              \
            z_drop(z_move(sample));                                             \
            z_drop(z_move(value));                                              \
        } else if (res == Z_CHANNEL_NODATA) {                                   \
            strcpy(buf, "nodata");                                              \
        }                                                                       \
    } while (0);

#define RECV(handler, buf) _RECV(handler, z_recv, buf)
#define TRY_RECV(handler, buf) _RECV(handler, z_try_recv, buf)

void sample_fifo_channel_test(void) {
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    z_fifo_channel_sample_new(&closure, &handler, 10);

    SEND(closure, "v1")
    SEND(closure, "v22")
    SEND(closure, "v333")
    SEND(closure, "v4444")

    char buf[100];

    RECV(handler, buf)
    assert(strcmp(buf, "v1") == 0);
    RECV(handler, buf)
    assert(strcmp(buf, "v22") == 0);
    RECV(handler, buf)
    assert(strcmp(buf, "v333") == 0);
    RECV(handler, buf)
    assert(strcmp(buf, "v4444") == 0);

    z_drop(z_move(closure));

    RECV(handler, buf)
    assert(strcmp(buf, "closed") == 0);

    z_drop(z_move(handler));
}

void sample_fifo_channel_test_try_recv(void) {
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    z_fifo_channel_sample_new(&closure, &handler, 10);

    char buf[100];

    TRY_RECV(handler, buf)
    assert(strcmp(buf, "nodata") == 0);

    SEND(closure, "v1")
    SEND(closure, "v22")
    SEND(closure, "v333")
    SEND(closure, "v4444")

    TRY_RECV(handler, buf)
    assert(strcmp(buf, "v1") == 0);
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "v22") == 0);
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "v333") == 0);
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "v4444") == 0);
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "nodata") == 0);

    z_drop(z_move(closure));
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "closed") == 0);

    z_drop(z_move(handler));
}

void sample_ring_channel_test_in_size(void) {
    z_owned_closure_sample_t closure;
    z_owned_ring_handler_sample_t handler;
    z_ring_channel_sample_new(&closure, &handler, 10);

    char buf[100];

    TRY_RECV(handler, buf)
    assert(strcmp(buf, "nodata") == 0);

    SEND(closure, "v1")
    SEND(closure, "v22")
    SEND(closure, "v333")
    SEND(closure, "v4444")

    RECV(handler, buf)
    assert(strcmp(buf, "v1") == 0);
    RECV(handler, buf)
    assert(strcmp(buf, "v22") == 0);
    RECV(handler, buf)
    assert(strcmp(buf, "v333") == 0);
    RECV(handler, buf)
    assert(strcmp(buf, "v4444") == 0);
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "nodata") == 0);

    z_drop(z_move(closure));
    RECV(handler, buf)
    assert(strcmp(buf, "closed") == 0);

    z_drop(z_move(handler));
}

void sample_ring_channel_test_over_size(void) {
    z_owned_closure_sample_t closure;
    z_owned_ring_handler_sample_t handler;
    z_ring_channel_sample_new(&closure, &handler, 3);

    char buf[100];
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "nodata") == 0);

    SEND(closure, "v1")
    SEND(closure, "v22")
    SEND(closure, "v333")
    SEND(closure, "v4444")

    RECV(handler, buf)
    assert(strcmp(buf, "v22") == 0);
    RECV(handler, buf)
    assert(strcmp(buf, "v333") == 0);
    RECV(handler, buf)
    assert(strcmp(buf, "v4444") == 0);
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "nodata") == 0);
    z_drop(z_move(closure));
    TRY_RECV(handler, buf)
    assert(strcmp(buf, "closed") == 0);

    z_drop(z_move(handler));
}

void zero_size_test(void) {
    z_owned_closure_sample_t closure;

    z_owned_fifo_handler_sample_t fifo_handler;
    assert(z_fifo_channel_sample_new(&closure, &fifo_handler, 0) != Z_OK);
    assert(z_fifo_channel_sample_new(&closure, &fifo_handler, 1) == Z_OK);
    z_drop(z_move(closure));
    z_drop(z_move(fifo_handler));

    z_owned_ring_handler_sample_t ring_handler;
    assert(z_ring_channel_sample_new(&closure, &ring_handler, 0) != Z_OK);
    assert(z_ring_channel_sample_new(&closure, &ring_handler, 1) == Z_OK);
    z_drop(z_move(closure));
    z_drop(z_move(ring_handler));
}

int main(void) {
    sample_fifo_channel_test();
    sample_fifo_channel_test_try_recv();
    sample_ring_channel_test_in_size();
    sample_ring_channel_test_over_size();
    zero_size_test();
}
