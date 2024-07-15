//
// Copyright (c) 2022 ZettaScale Technology
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

#include "stdio.h"
#include "zenoh-pico.h"
#include "zenoh-pico/api/primitives.h"

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1
void callback(const z_loaned_sample_t* sample, void* context) {
    const z_loaned_publisher_t* pub = z_publisher_loan((z_owned_publisher_t*)context);
    z_owned_slice_t value;
    z_bytes_deserialize_into_slice(z_sample_payload(sample), &value);

    // Create payload
    z_owned_bytes_t payload;
    z_bytes_serialize_from_slice(&payload, z_slice_data(z_slice_loan(&value)), z_slice_len(z_slice_loan(&value)));

    z_publisher_put(pub, z_bytes_move(&payload), NULL);
    z_slice_drop(z_slice_move(&value));
}
void drop(void* context) {
    z_owned_publisher_t* pub = (z_owned_publisher_t*)context;
    z_undeclare_publisher(pub);
    // A note on lifetimes:
    //  here, `sub` takes ownership of `pub` and will drop it before returning from its own `drop`,
    //  which makes passing a pointer to the stack safe as long as `sub` is dropped in a scope where `pub` is still
    //  valid.
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    z_owned_config_t config;
    z_config_default(&config);
    z_owned_session_t session;
    z_open(&session, z_config_move(&config));
    if (!z_session_check(&session)) {
        printf("Unable to open session!\n");
        return -1;
    }

    if (zp_start_read_task(z_session_loan_mut(&session), NULL) < 0 ||
        zp_start_lease_task(z_session_loan_mut(&session), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_close(z_session_move(&session));
        return -1;
    }

    z_view_keyexpr_t pong;
    z_view_keyexpr_from_str_unchecked(&pong, "test/pong");
    z_owned_publisher_t pub;
    if (z_declare_publisher(&pub, z_session_loan(&session), z_view_keyexpr_loan(&pong), NULL) < 0) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    z_view_keyexpr_t ping;
    z_view_keyexpr_from_str_unchecked(&ping, "test/ping");
    z_owned_closure_sample_t respond;
    z_closure_sample(&respond, callback, drop, (void*)z_publisher_move(&pub));
    z_owned_subscriber_t sub;
    if (z_declare_subscriber(&sub, z_session_loan(&session), z_view_keyexpr_loan(&ping),
                             z_closure_sample_move(&respond), NULL) < 0) {
        printf("Unable to declare subscriber for key expression.\n");
        return -1;
    }

    while (getchar() != 'q') {
    }

    z_undeclare_subscriber(z_subscriber_move(&sub));

    z_close(z_session_move(&session));
}
#else
int main(void) {
    printf(
        "ERROR: Zenoh pico was compiled without Z_FEATURE_SUBSCRIPTION or Z_FEATURE_PUBLICATION but this example "
        "requires them.\n");
    return -2;
}
#endif
