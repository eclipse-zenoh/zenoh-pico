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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"
#include "zenoh-pico/api/macros.h"

#undef NDEBUG
#include <assert.h>

#if Z_FEATURE_QUERYABLE

void test_queryable_keyexpr(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");

    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    if (zp_start_read_task(z_loan_mut(s), NULL) < 0 || zp_start_lease_task(z_loan_mut(s), NULL) < 0) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        exit(-1);
    }

    z_view_keyexpr_t ke1;
    z_view_keyexpr_from_str(&ke1, "test/queryable_keyexpr");

    z_owned_queryable_t q1;
    z_owned_closure_query_t cb1;
    z_owned_fifo_handler_query_t h1;
    z_fifo_channel_query_new(&cb1, &h1, 16);
    z_declare_queryable(z_loan(s), &q1, z_loan(ke1), z_move(cb1), NULL);

    z_view_string_t sv1;
    z_keyexpr_as_view_string(z_queryable_keyexpr(z_loan(q1)), &sv1);
    assert(strncmp("test/queryable_keyexpr", z_string_data(z_loan(sv1)), z_string_len(z_loan(sv1))) == 0);
    z_drop(z_move(q1));
    z_drop(z_move(h1));

    z_view_keyexpr_t ke2;
    z_owned_keyexpr_t ke2_declared;
    z_view_keyexpr_from_str(&ke2, "test/queryable_declared_keyexpr");
    z_declare_keyexpr(z_loan(s), &ke2_declared, z_loan(ke2));

    z_owned_queryable_t q2;
    z_owned_closure_query_t cb2;
    z_owned_fifo_handler_query_t h2;
    z_fifo_channel_query_new(&cb2, &h2, 16);
    z_declare_queryable(z_loan(s), &q2, z_loan(ke2_declared), z_move(cb2), NULL);

    z_view_string_t sv2;
    z_keyexpr_as_view_string(z_queryable_keyexpr(z_loan(q2)), &sv2);
    assert(strncmp("test/queryable_declared_keyexpr", z_string_data(z_loan(sv2)), z_string_len(z_loan(sv2))) == 0);
    z_drop(z_move(ke2_declared));
    z_drop(z_move(q2));
    z_drop(z_move(h2));

    z_drop(z_move(s));
}

int main(void) {
    test_queryable_keyexpr();
    return 0;
}
#else
int main(void) {}
#endif
