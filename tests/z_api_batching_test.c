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

#include <assert.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"

#if Z_FEATURE_BATCHING == 1

void test_batching_while_connected(void) {
    printf("test_batching_while_connected\n");
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    ASSERT_OK(z_open(&s2, z_move(c2), NULL));

    z_sleep_ms(1000);  // Wait for connection to establish

    ASSERT_OK(zp_batch_start(z_loan_mut(s2)));
    ASSERT_OK(zp_batch_flush(z_loan_mut(s2)));
    ASSERT_OK(zp_batch_stop(z_loan_mut(s2)));

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

void test_batching_after_disconnection(void) {
    printf("test_batching_after_disconnection\n");
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, "tcp/127.0.0.1:12345");

    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "client");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:12345");

    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    ASSERT_OK(z_open(&s2, z_move(c2), NULL));

    // Wait for connection to establish
    z_sleep_ms(1000);

    // Drop session and wait for it to be detected as dropped
    z_session_drop(z_session_move(&s1));
    z_sleep_ms(10000);

    ASSERT_ERR(zp_batch_start(z_loan_mut(s2)), _Z_ERR_TRANSPORT_NOT_AVAILABLE);
    ASSERT_ERR(zp_batch_flush(z_loan_mut(s2)), _Z_ERR_TRANSPORT_NOT_AVAILABLE);
    ASSERT_ERR(zp_batch_stop(z_loan_mut(s2)), _Z_ERR_TRANSPORT_NOT_AVAILABLE);

    z_session_drop(z_session_move(&s2));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_batching_while_connected();
    test_batching_after_disconnection();

    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("Missing config token to build this test. This test requires: Z_FEATURE_BATCHING\n");
    return 0;
}

#endif  // Z_FEATURE_BATCHING == 1
