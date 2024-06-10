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
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/net/sample.h"

#undef NDEBUG
#include <assert.h>

#define SEND(channel, v)                                                             \
    do {                                                                             \
        _z_sample_t s = {.keyexpr = _z_rname("key"),                                 \
                         .payload = {.start = (const uint8_t *)v, .len = strlen(v)}, \
                         .timestamp = _z_timestamp_null(),                           \
                         .encoding = _z_encoding_null(),                             \
                         .kind = 0,                                                  \
                         .qos = {0}};                                                \
        z_loaned_sample_t sample = _z_sample_rc_new_from_val(s);                     \
        z_call(channel.send, &sample);                                               \
    } while (0);

#define _RECV(channel, method, buf)                                             \
    do {                                                                        \
        z_owned_sample_t sample;                                                \
        z_sample_null(&sample);                                                 \
        z_call(channel.method, &sample);                                        \
        if (z_check(sample)) {                                                  \
            const z_loaned_bytes_t *payload = z_sample_payload(z_loan(sample)); \
            strncpy(buf, (const char *)payload->start, (size_t)payload->len);   \
            buf[payload->len] = '\0';                                           \
            z_drop(z_move(sample));                                             \
        } else {                                                                \
            buf[0] = '\0';                                                      \
        }                                                                       \
    } while (0);

#define RECV(channel, buf) _RECV(channel, recv, buf)
#define TRY_RECV(channel, buf) _RECV(channel, try_recv, buf)

void sample_fifo_channel_test(void) {
    z_owned_sample_fifo_channel_t channel;
    z_sample_fifo_channel_new(&channel, 10);

    SEND(channel, "v1")
    SEND(channel, "v22")
    SEND(channel, "v333")
    SEND(channel, "v4444")

    char buf[100];

    RECV(channel, buf)
    assert(strcmp(buf, "v1") == 0);
    RECV(channel, buf)
    assert(strcmp(buf, "v22") == 0);
    RECV(channel, buf)
    assert(strcmp(buf, "v333") == 0);
    RECV(channel, buf)
    assert(strcmp(buf, "v4444") == 0);

    z_drop(z_move(channel));
}

void sample_fifo_channel_test_try_recv(void) {
    z_owned_sample_fifo_channel_t channel;
    z_sample_fifo_channel_new(&channel, 10);

    char buf[100];

    TRY_RECV(channel, buf)
    assert(strcmp(buf, "") == 0);

    SEND(channel, "v1")
    SEND(channel, "v22")
    SEND(channel, "v333")
    SEND(channel, "v4444")

    TRY_RECV(channel, buf)
    assert(strcmp(buf, "v1") == 0);
    TRY_RECV(channel, buf)
    assert(strcmp(buf, "v22") == 0);
    TRY_RECV(channel, buf)
    assert(strcmp(buf, "v333") == 0);
    TRY_RECV(channel, buf)
    assert(strcmp(buf, "v4444") == 0);
    TRY_RECV(channel, buf)
    assert(strcmp(buf, "") == 0);

    z_drop(z_move(channel));
}

void sample_ring_channel_test_in_size(void) {
    z_owned_sample_ring_channel_t channel;
    z_sample_ring_channel_new(&channel, 10);

    char buf[100];

    TRY_RECV(channel, buf)
    assert(strcmp(buf, "") == 0);

    SEND(channel, "v1")
    SEND(channel, "v22")
    SEND(channel, "v333")
    SEND(channel, "v4444")

    RECV(channel, buf)
    assert(strcmp(buf, "v1") == 0);
    RECV(channel, buf)
    assert(strcmp(buf, "v22") == 0);
    RECV(channel, buf)
    assert(strcmp(buf, "v333") == 0);
    RECV(channel, buf)
    assert(strcmp(buf, "v4444") == 0);
    TRY_RECV(channel, buf)
    assert(strcmp(buf, "") == 0);

    z_drop(z_move(channel));
}

void sample_ring_channel_test_over_size(void) {
    z_owned_sample_ring_channel_t channel;
    z_sample_ring_channel_new(&channel, 3);

    char buf[100];
    TRY_RECV(channel, buf)
    assert(strcmp(buf, "") == 0);

    SEND(channel, "v1")
    SEND(channel, "v22")
    SEND(channel, "v333")
    SEND(channel, "v4444")

    RECV(channel, buf)
    assert(strcmp(buf, "v22") == 0);
    RECV(channel, buf)
    assert(strcmp(buf, "v333") == 0);
    RECV(channel, buf)
    assert(strcmp(buf, "v4444") == 0);
    TRY_RECV(channel, buf)
    assert(strcmp(buf, "") == 0);

    z_drop(z_move(channel));
}

int main(void) {
    sample_fifo_channel_test();
    sample_fifo_channel_test_try_recv();
    sample_ring_channel_test_in_size();
    sample_ring_channel_test_over_size();
}
