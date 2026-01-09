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
//

#include <assert.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"

#if Z_FEATURE_ADMIN_SPACE

/**
 * Test starting and stopping the admin space multiple times.
 */
void test_start_stop_admin_space(void) {
    z_owned_session_t s;
    z_owned_config_t c;
    z_config_default(&c);
    ASSERT_OK(z_open(&s, z_move(c), NULL));
    ASSERT_EQ_U32(_Z_RC_IN_VAL(z_loan(s))->_admin_space_queryable_id, 0);

    for (int i = 0; i < 2; i++) {
        ASSERT_OK(ze_start_admin_space(z_loan_mut(s)));
        ASSERT_TRUE(_Z_RC_IN_VAL(z_loan(s))->_admin_space_queryable_id > 0);
        ASSERT_OK(ze_stop_admin_space(z_loan_mut(s)));
        ASSERT_EQ_U32(_Z_RC_IN_VAL(z_loan(s))->_admin_space_queryable_id, 0);
    }
    z_drop(z_move(s));
}

/**
 * Test auto starting of the admin space.
 */
void test_auto_start_admin_space(void) {
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_open_options_t opt1, opt2;
    z_open_options_default(&opt1);
    z_open_options_default(&opt2);
    opt2.auto_start_admin_space = true;

    ASSERT_OK(z_open(&s1, z_move(c1), &opt1));
    ASSERT_EQ_U32(_Z_RC_IN_VAL(z_loan(s1))->_admin_space_queryable_id, 0);
    z_drop(z_move(s1));

    ASSERT_OK(z_open(&s2, z_move(c2), &opt2));
    ASSERT_TRUE(_Z_RC_IN_VAL(z_loan(s2))->_admin_space_queryable_id > 0);
    z_drop(z_move(s2));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_start_stop_admin_space();
    test_auto_start_admin_space();
    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("Missing config token to build this test. This test requires: Z_FEATURE_ADMIN_SPACE\n");
    return 0;
}

#endif
