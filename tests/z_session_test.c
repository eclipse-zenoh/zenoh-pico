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
#include "zenoh-pico/api/primitives.h"

#undef NDEBUG
#include <assert.h>

static void test_autostart_defaults(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");

    z_owned_session_t s;
    assert(z_open(&s, z_move(config), NULL) == _Z_RES_OK);

    // Tasks should already be running; repeated start calls must be OK
    assert(zp_read_task_is_running(z_loan(s)));
    assert(zp_lease_task_is_running(z_loan(s)));
    assert(zp_start_read_task(z_loan_mut(s), NULL) == _Z_RES_OK);
    assert(zp_start_lease_task(z_loan_mut(s), NULL) == _Z_RES_OK);

    // Stop and close
    assert(zp_stop_read_task(z_loan_mut(s)) == _Z_RES_OK);
    assert(zp_stop_lease_task(z_loan_mut(s)) == _Z_RES_OK);
    z_drop(z_move(s));
}

static void test_manual_start(void) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");

    z_open_options_t opts;
    z_open_options_default(&opts);
    opts.auto_start_read_task = false;
    opts.auto_start_lease_task = false;
#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
    opts.auto_start_periodic_task = false;
#endif
#endif

    z_owned_session_t s;
    assert(z_open(&s, z_move(config), &opts) == _Z_RES_OK);

    // Nothing auto-started: manual start should succeed
    assert(!zp_read_task_is_running(z_loan(s)));
    assert(!zp_lease_task_is_running(z_loan(s)));
    assert(zp_start_lease_task(z_loan_mut(s), NULL) == _Z_RES_OK);
    assert(zp_start_read_task(z_loan_mut(s), NULL) == _Z_RES_OK);
#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
    assert(!zp_periodic_scheduler_task_is_running(z_loan(s)));
#endif
#endif

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1
    assert(zp_start_periodic_scheduler_task(z_loan_mut(s), NULL) == _Z_RES_OK);
    assert(zp_periodic_scheduler_task_is_running(z_loan(s)));
    assert(zp_stop_periodic_scheduler_task(z_loan_mut(s)) == _Z_RES_OK);
#endif
#endif

    assert(zp_stop_read_task(z_loan_mut(s)) == _Z_RES_OK);
    assert(zp_stop_lease_task(z_loan_mut(s)) == _Z_RES_OK);
    z_drop(z_move(s));
}

int main(void) {
    test_autostart_defaults();
    test_manual_start();
    return 0;
}
