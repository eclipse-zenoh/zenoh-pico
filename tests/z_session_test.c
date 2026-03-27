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

#if Z_FEATURE_MULTI_THREAD == 1
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
#endif

static void test_open_timeout_single_locator(void) {
    z_owned_config_t c;

    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:18002");

    z_owned_session_t s;

    z_open_options_t opts;
    z_open_options_default(&opts);
    opts.connect_timeout_ms = 1000;

    z_clock_t start = z_clock_now();
    z_result_t ret = z_open(&s, z_move(c), &opts);
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);

    assert(ret == _Z_ERR_TRANSPORT_OPEN_FAILED);
    assert(elapsed_ms >= 1000);
}

#if Z_FEATURE_UNICAST_PEER == 1
static void test_open_timeout_partial_connectivity(void) {
    z_owned_config_t c1;
    z_owned_config_t c2;

    z_config_default(&c1);
    z_config_default(&c2);

    // Listener peer
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, "tcp/127.0.0.1:18001");

    // Connecting peer: one good endpoint, one bad endpoint
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:18001");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:18002");

    z_owned_session_t s1;
    z_owned_session_t s2;

    assert(z_open(&s1, z_move(c1), NULL) == _Z_RES_OK);

    z_open_options_t opts;
    z_open_options_default(&opts);
    opts.connect_timeout_ms = 1000;

    z_clock_t start = z_clock_now();
    z_result_t ret = z_open(&s2, z_move(c2), &opts);
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);

    assert(ret == _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY);
    assert(elapsed_ms >= 1000);

    z_drop(z_move(s2));
    z_drop(z_move(s1));
}
#endif

#if Z_FEATURE_MULTI_THREAD == 1
typedef struct {
    z_owned_config_t config;
    z_owned_session_t session;
    uint32_t delay_ms;
    z_result_t ret;
} delayed_open_arg_t;

static void *delayed_open_task(void *arg) {
    delayed_open_arg_t *ctx = (delayed_open_arg_t *)arg;

    z_sleep_ms(ctx->delay_ms);
    ctx->ret = z_open(&ctx->session, z_move(ctx->config), NULL);

    return NULL;
}

static void test_open_late_joining_endpoint(void) {
    z_owned_config_t c1;
    z_owned_config_t c2;

    z_config_default(&c1);
    z_config_default(&c2);

    // Late listener peer
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, "tcp/127.0.0.1:18001");

    // Connecting peer
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:18001");

    delayed_open_arg_t ctx;
    ctx.config = c1;
    ctx.delay_ms = 1000;
    ctx.ret = _Z_ERR_GENERIC;

    _z_task_t task;
    assert(_z_task_init(&task, NULL, delayed_open_task, &ctx) == _Z_RES_OK);

    z_open_options_t opts;
    z_open_options_default(&opts);
    opts.connect_timeout_ms = 5000;

    z_owned_session_t s2;
    z_clock_t start = z_clock_now();
    z_result_t ret = z_open(&s2, z_move(c2), &opts);
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);

    assert(ret == _Z_RES_OK);
    assert(elapsed_ms >= 1000);

    assert(_z_task_join(&task) == _Z_RES_OK);
    assert(ctx.ret == _Z_RES_OK);

    z_drop(z_move(s2));
    z_drop(z_move(ctx.session));
}
#endif

static void test_open_wait_for_all_false(void) {
    z_owned_config_t c1;
    z_owned_config_t c2;

    z_config_default(&c1);
    z_config_default(&c2);

    // Listener peer
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, "tcp/127.0.0.1:18001");

    // Connecting peer: one good endpoint, one bad endpoint
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:18001");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:18002");

    z_owned_session_t s1;
    z_owned_session_t s2;

    assert(z_open(&s1, z_move(c1), NULL) == _Z_RES_OK);

    z_open_options_t opts;
    z_open_options_default(&opts);
    opts.connect_timeout_ms = 1000;
    opts.connect_wait_for_all = false;

    z_clock_t start = z_clock_now();
    z_result_t ret = z_open(&s2, z_move(c2), &opts);
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);

    assert(ret == _Z_RES_OK);
    assert(elapsed_ms < 1000);

    z_drop(z_move(s2));
    z_drop(z_move(s1));
}

int main(void) {
#if Z_FEATURE_MULTI_THREAD == 1
    test_autostart_defaults();
    test_manual_start();
    test_open_late_joining_endpoint();
#endif
    test_open_timeout_single_locator();
#if Z_FEATURE_UNICAST_PEER == 1
    test_open_timeout_partial_connectivity();
#endif
    test_open_wait_for_all_false();
    return 0;
}
