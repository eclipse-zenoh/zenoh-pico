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

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/transport/common/tx.h"

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_FRAGMENTATION == 1 && \
    Z_FEATURE_MULTI_THREAD == 0

#define MAX_READS 10

z_result_t _z_transport_message_encode_override(_z_wbuf_t *wbf, const _z_transport_message_t *msg, bool *handled) {
    z_result_t res = _Z_RES_OK;
    if (_Z_MID(msg->_header) == _Z_MID_T_FRAGMENT) {
        res = _z_fragment_encode(wbf, msg->_header, &msg->_body._fragment);
        if (res < 0) {
            return res;
        }
        _z_transport_message_t *msg_mut = (_z_transport_message_t *)msg;
        msg_mut->_header = 0;
        *handled = true;
    }
    return res;
}

static void dump_zbuf_state(const char *prefix, _z_zbuf_t *zbuf) {
    printf("%s rpos=%d wpos=%d capacity=%d len=%d left=%d\n", prefix, (int)_z_zbuf_get_rpos(zbuf),
           (int)_z_zbuf_get_wpos(zbuf), (int)_z_zbuf_capacity(zbuf), (int)_z_zbuf_len(zbuf),
           (int)_z_zbuf_space_left(zbuf));
}

/**
 * Read until sample is received or error occurs.
 *
 * Returns:
 *   _Z_RES_OK if a sample was received
 *   Z_CHANNEL_NODATA if no sample was received and no read error occurred
 *   <0 on zp_read()/z_try_recv() failure
 */
static z_result_t read_until_sample(const z_loaned_session_t *zs, const z_loaned_fifo_handler_sample_t *handler,
                                    _z_zbuf_t *zbuf, int max_reads) {
    z_result_t res = _Z_RES_OK;
    for (int i = 0; i < max_reads; i++) {
        res = zp_read(zs, NULL);
        if (res < 0) {
            printf("Failed to read from session: %d\n", res);
            dump_zbuf_state("[read error zbuf]", zbuf);
            return res;
        }

        z_owned_sample_t sample;
        res = z_try_recv(handler, &sample);
        if (res == Z_CHANNEL_NODATA) {
            printf("No sample received on read iteration %d.\n", i);
            dump_zbuf_state("[recv no data zbuf]", zbuf);
        } else if (res == _Z_RES_OK) {
            z_view_string_t keystr;
            z_keyexpr_as_view_string(z_sample_keyexpr(z_loan(sample)), &keystr);
            printf("[rx] Received packet on %.*s, len: %ld\n", (int)z_string_len(z_loan(keystr)),
                   z_string_data(z_loan(keystr)), z_bytes_len(z_sample_payload(z_loan(sample))));
            z_drop(z_move(sample));
            dump_zbuf_state("[recv success zbuf]", zbuf);
            return res;
        } else {
            printf("Failed to receive sample: %d\n", res);
            dump_zbuf_state("[recv error zbuf]", zbuf);
            return res;
        }
    }

    return res;
}

static z_result_t publish_buf(const z_loaned_publisher_t *pub, uint8_t *value, size_t size) {
    z_owned_bytes_t payload;
    z_result_t res = z_bytes_from_buf(&payload, value, size, NULL, NULL);
    if (res < 0) {
        printf("Unable to create payload from buffer.\n");
        return res;
    }

    res = z_publisher_put(pub, z_move(payload), NULL);
    if (res < 0) {
        printf("Failed to publish sample.\n");
        return res;
    }

    return _Z_RES_OK;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const char *keyexpr = "test/zenoh-pico-single-thread-fragment";
    uint8_t *value = NULL;
    size_t size = 3000;

    value = z_malloc(size);
    ASSERT_NOT_NULL(value);

    for (size_t i = 0; i < size; i++) {
        value[i] = (uint8_t)i;
    }

    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, "udp/224.0.0.224:7447#iface=lo");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_LISTEN_KEY, "udp/224.0.0.224:7447#iface=lo");

    z_owned_session_t s1, s2;
    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    ASSERT_OK(z_open(&s2, z_move(c2), NULL));

    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    ASSERT_OK(z_fifo_channel_sample_new(&closure, &handler, 3));
    z_owned_subscriber_t sub;
    z_view_keyexpr_t ke;
    ASSERT_OK(z_view_keyexpr_from_str(&ke, keyexpr));
    ASSERT_OK(z_declare_subscriber(z_loan(s1), &sub, z_loan(ke), z_move(closure), NULL));

    z_owned_publisher_t pub;
    ASSERT_OK(z_declare_publisher(z_loan(s2), &pub, z_loan(ke), NULL));

    z_sleep_s(1);

    printf("[tx]: Sending valid packet on %s, len: %d\n", keyexpr, (int)size);
    ASSERT_OK(publish_buf(z_loan(pub), value, size));

    z_sleep_s(1);

    _z_session_t *session = _Z_RC_IN_VAL(z_loan(s1));
    _z_zbuf_t *zbuf = &session->_tp._transport._multicast._common._zbuf;

    dump_zbuf_state("[initial zbuf]", zbuf);

    z_result_t res = read_until_sample(z_loan(s1), z_loan(handler), zbuf, MAX_READS);
    ASSERT_OK(res);
    dump_zbuf_state("[zbuf after sample]", zbuf);
    ASSERT_EQ_U32(_z_zbuf_get_rpos(zbuf), _z_zbuf_get_wpos(zbuf));

    _z_transport_set_message_encode_override(_z_transport_message_encode_override);

    printf("[tx]: Sending corrupted packet on %s, len: %d\n", keyexpr, (int)size);
    ASSERT_OK(publish_buf(z_loan(pub), value, size));

    z_sleep_s(1);

    res = read_until_sample(z_loan(s1), z_loan(handler), zbuf, MAX_READS);
    ASSERT_ERR(res, _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN);
    dump_zbuf_state("[zbuf after bad sample]", zbuf);
    ASSERT_EQ_U32(_z_zbuf_get_rpos(zbuf), _z_zbuf_get_wpos(zbuf));

    z_drop(z_move(sub));
    z_drop(z_move(handler));
    z_drop(z_move(pub));
    z_drop(z_move(s1));
    z_drop(z_move(s2));
    z_free(value);
    return 0;
}

#else

int main(void) {
    printf(
        "Missing config token to build this test. This test requires: Z_FEATURE_SUBSCRIPTION=1, "
        "Z_FEATURE_PUBLICATION=1, "
        "Z_FEATURE_FRAGMENTATION=1 and Z_FEATURE_MULTI_THREAD=0\n");
    return 0;
}

#endif
