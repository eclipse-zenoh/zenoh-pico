//
// Copyright (c) 2025 ZettaScale Technology
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

#include "zenoh-pico.h"

#undef NDEBUG
#include <assert.h>

#if Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_LOCAL_SUBSCRIBER == 1 && \
    Z_FEATURE_MULTI_THREAD == 1

static const char *PUB_EXPR = "zenoh-pico/locality/pub-sub";
static const char *PAYLOAD = "payload";

void test_put_sub(z_locality_t pub_allowed_destination, z_locality_t s1_allowed_origin,
                  z_locality_t s2_allowed_origin) {
    printf("Testing: %d %d %d\n", pub_allowed_destination, s1_allowed_origin, s2_allowed_origin);
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, PUB_EXPR);

    assert(z_open(&s1, z_config_move(&c1), NULL) == Z_OK);
    assert(z_open(&s2, z_config_move(&c2), NULL) == Z_OK);

    assert(zp_start_read_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_read_task(z_loan_mut(s2), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s2), NULL) == Z_OK);

    z_owned_subscriber_t subscriber1, subscriber2;
    z_subscriber_options_t opts1, opts2;
    z_subscriber_options_default(&opts1);
    z_subscriber_options_default(&opts2);
    opts1.allowed_origin = s1_allowed_origin;
    opts2.allowed_origin = s2_allowed_origin;

    z_owned_closure_sample_t subscriber_callback1, subscriber_callback2;
    z_owned_fifo_handler_sample_t subscriber_handler1, subscriber_handler2;
    z_fifo_channel_sample_new(&subscriber_callback1, &subscriber_handler1, 16);
    z_declare_subscriber(z_session_loan(&s1), &subscriber1, z_view_keyexpr_loan(&ke),
                         z_closure_sample_move(&subscriber_callback1), &opts1);

    z_fifo_channel_sample_new(&subscriber_callback2, &subscriber_handler2, 16);
    z_declare_subscriber(z_session_loan(&s2), &subscriber2, z_view_keyexpr_loan(&ke),
                         z_closure_sample_move(&subscriber_callback2), &opts2);

    z_put_options_t opts;
    z_put_options_default(&opts);
    opts.allowed_destination = pub_allowed_destination;

    z_sleep_s(1);
    z_owned_bytes_t payload;
    z_bytes_copy_from_str(&payload, PAYLOAD);

    z_put(z_session_loan(&s1), z_view_keyexpr_loan(&ke), z_bytes_move(&payload), &opts);

    z_sleep_s(1);

    z_owned_sample_t sample1, sample2;

    z_result_t ret1 = z_fifo_handler_sample_try_recv(z_fifo_handler_sample_loan(&subscriber_handler1), &sample1);
    z_result_t ret2 = z_fifo_handler_sample_try_recv(z_fifo_handler_sample_loan(&subscriber_handler2), &sample2);
    if (_z_locality_allows_local(pub_allowed_destination) && _z_locality_allows_local(s1_allowed_origin)) {
        assert(ret1 == Z_OK);
        const z_loaned_sample_t *sample = z_sample_loan(&sample1);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t payloadstr;
        z_bytes_to_string(z_sample_payload(sample), &payloadstr);
        // SAFETY: test.
        // Flawfinder: ignore [CWE-126]
        assert(strncmp(z_string_data(z_loan(payloadstr)), PAYLOAD, strlen(PAYLOAD)) == 0);
        // SAFETY: test.
        // Flawfinder: ignore [CWE-126]
        assert(strncmp(z_string_data(z_loan(keystr)), PUB_EXPR, strlen(PUB_EXPR)) == 0);
        z_string_drop(z_string_move(&payloadstr));
    } else {
        assert(ret1 != Z_OK);
    }
    if (_z_locality_allows_remote(pub_allowed_destination) && _z_locality_allows_remote(s2_allowed_origin)) {
        assert(ret2 == Z_OK);
        const z_loaned_sample_t *sample = z_sample_loan(&sample2);
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t payloadstr;
        z_bytes_to_string(z_sample_payload(sample), &payloadstr);

        // SAFETY: test.
        // Flawfinder: ignore [CWE-126]
        assert(strncmp(z_string_data(z_loan(payloadstr)), PAYLOAD, strlen(PAYLOAD)) == 0);
        // SAFETY: test.
        // Flawfinder: ignore [CWE-126]
        assert(strncmp(z_string_data(z_loan(keystr)), PUB_EXPR, strlen(PUB_EXPR)) == 0);
        z_string_drop(z_string_move(&payloadstr));
    } else {
        assert(ret2 != Z_OK);
    }

    z_fifo_handler_sample_drop(z_fifo_handler_sample_move(&subscriber_handler1));
    z_fifo_handler_sample_drop(z_fifo_handler_sample_move(&subscriber_handler2));

    zp_stop_read_task(z_loan_mut(s1));
    zp_stop_read_task(z_loan_mut(s2));
    zp_stop_lease_task(z_loan_mut(s1));
    zp_stop_lease_task(z_loan_mut(s2));

    z_subscriber_drop(z_subscriber_move(&subscriber1));
    z_subscriber_drop(z_subscriber_move(&subscriber2));
    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    z_locality_t localities[] = {Z_LOCALITY_ANY, Z_LOCALITY_REMOTE, Z_LOCALITY_SESSION_LOCAL};
    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            for (size_t k = 0; k < 3; k++) {
                test_put_sub(localities[i], localities[j], localities[k]);
            }
        }
    }
}

#else
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
}
#endif
