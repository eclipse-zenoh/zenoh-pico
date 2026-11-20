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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico.h"

#undef NDEBUG
#include <assert.h>

// Declarations that a session keeps of its peers' entities, for publishers that are declared later.
//
// Every session numbers its entities in the same way, so the first subscriber of two peers has the same id. A
// session that is linked to both has to tell the two apart by the peer they belong to.

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_MULTI_THREAD == 1 &&  \
    Z_FEATURE_LOCAL_SUBSCRIBER == 0 && Z_FEATURE_UNICAST_PEER == 1 && Z_FEATURE_MATCHING == 1 && \
    Z_FEATURE_INTEREST == 1

#define LOCATOR_A "tcp/127.0.0.1:17481"
#define LOCATOR_B "tcp/127.0.0.1:17482"
#define KEY_A "test/peer_declares/a"
#define KEY_B "test/peer_declares/b"
#define MSG_NB 3

static void data_handler(z_loaned_sample_t *sample, void *ctx) {
    _ZP_UNUSED(sample);
    (*(int *)ctx)++;
}

static void open_peer(z_owned_session_t *session, const char *listen, const char *connect1, const char *connect2) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "peer");
    if (listen != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, listen);
    }
    if (connect1 != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, connect1);
    }
    if (connect2 != NULL) {
        zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, connect2);
    }
    assert(z_open(session, z_move(config), NULL) == Z_OK);
}

static void declare_subscriber(z_owned_session_t *session, z_owned_subscriber_t *sub, const char *key, int *count) {
    z_view_keyexpr_t ke;
    z_owned_closure_sample_t callback;
    z_view_keyexpr_from_str(&ke, key);
    z_closure(&callback, data_handler, NULL, count);
    assert(z_declare_subscriber(z_loan(*session), sub, z_loan(ke), z_move(callback), NULL) == Z_OK);
}

static void declare_publisher(z_owned_session_t *session, z_owned_publisher_t *pub, const char *key) {
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, key);
    assert(z_declare_publisher(z_loan(*session), pub, z_loan(ke), NULL) == Z_OK);
}

static bool is_matching(z_owned_publisher_t *pub) {
    z_matching_status_t status = {.matching = false};
    assert(z_publisher_get_matching_status(z_loan(*pub), &status) == Z_OK);
    return status.matching;
}

static void put(z_owned_publisher_t *pub) {
    for (int i = 0; i < MSG_NB; i++) {
        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, "data");
        assert(z_publisher_put(z_loan(*pub), z_move(payload), NULL) == Z_OK);
    }
}

int main(void) {
    z_owned_session_t a, b, m;
    z_owned_subscriber_t sub_a, sub_b;
    z_owned_publisher_t pub_a, pub_b;
    int received_a = 0;
    int received_b = 0;

    // m is linked to a and to b
    open_peer(&a, LOCATOR_A, NULL, NULL);
    open_peer(&b, LOCATOR_B, NULL, NULL);
    open_peer(&m, NULL, LOCATOR_A, LOCATOR_B);
    z_sleep_s(1);

    // The first entity of a and the first entity of b: the same id
    declare_subscriber(&a, &sub_a, KEY_A, &received_a);
    declare_subscriber(&b, &sub_b, KEY_B, &received_b);
    z_sleep_s(1);

    // Publishers declared after the declarations arrived find both subscribers
    printf("Publishers declared after the subscribers of two peers\n");
    declare_publisher(&m, &pub_a, KEY_A);
    declare_publisher(&m, &pub_b, KEY_B);
    z_sleep_s(1);
    assert(is_matching(&pub_a));
    assert(is_matching(&pub_b));
    put(&pub_a);
    put(&pub_b);
    z_sleep_s(1);
    printf("Received: a %d, b %d\n", received_a, received_b);
    assert(received_a == MSG_NB);
    assert(received_b == MSG_NB);
    z_drop(z_move(pub_a));
    z_drop(z_move(pub_b));

    // One peer undeclares its subscriber: the other peer's stays
    printf("Subscriber of a undeclared\n");
    z_drop(z_move(sub_a));
    z_sleep_s(1);
    declare_publisher(&m, &pub_a, KEY_A);
    declare_publisher(&m, &pub_b, KEY_B);
    z_sleep_s(1);
    assert(!is_matching(&pub_a));
    assert(is_matching(&pub_b));
    z_drop(z_move(pub_a));
    z_drop(z_move(pub_b));

    // A peer that goes away without undeclaring its subscriber leaves none behind
    printf("Peer b closed\n");
    z_drop(z_move(b));
    z_sleep_s(1);
    declare_publisher(&m, &pub_b, KEY_B);
    z_sleep_s(1);
    assert(!is_matching(&pub_b));
    z_drop(z_move(pub_b));
    z_drop(z_move(sub_b));

    z_drop(z_move(m));
    z_drop(z_move(a));
    printf("Test passed\n");
    return 0;
}

#else
int main(void) {
    printf(
        "Missing config token to build this test. This test requires: Z_FEATURE_SUBSCRIPTION, "
        "Z_FEATURE_PUBLICATION, Z_FEATURE_MULTI_THREAD, Z_FEATURE_UNICAST_PEER, Z_FEATURE_MATCHING, "
        "Z_FEATURE_INTEREST and Z_FEATURE_LOCAL_SUBSCRIBER disabled\n");
    return 0;
}
#endif
