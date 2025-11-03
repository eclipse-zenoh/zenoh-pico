//
// Copyright (c) 2025 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

// Integration tests for the public/unstable periodic scheduler API

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"

#if Z_FEATURE_PERIODIC_TASKS == 1 && defined Z_FEATURE_UNSTABLE_API

// -------------------- Test task + drop callbacks ------------------------
typedef struct {
    volatile int hits;
    volatile int drops;
} user_task_ctx_t;

static void user_task_cb(void *arg) {
    user_task_ctx_t *c = (user_task_ctx_t *)arg;
    c->hits++;
}

static void user_task_drop(void *arg) {
    user_task_ctx_t *c = (user_task_ctx_t *)arg;
    c->drops++;
}

static _zp_closure_periodic_task_t mk_closure(void *ctx) {
    _zp_closure_periodic_task_t cl = {.context = ctx, .call = user_task_cb, .drop = user_task_drop};
    return cl;
}

// -------------------- Tests  ----------------------
static void test_process_periodic_tasks(void) {
    printf("test_process_periodic_tasks()\n");

    z_owned_session_t s;
    z_owned_config_t c;
    z_config_default(&c);
    ASSERT_OK(z_open(&s, z_config_move(&c), NULL));

    _z_session_t *inner = _Z_RC_IN_VAL(z_loan(s));
    ASSERT_TRUE(inner != NULL);

    user_task_ctx_t ctx = {0};
    _zp_closure_periodic_task_t cl = mk_closure(&ctx);
    uint32_t id = 0;

    // Add a job that should fire ~every 50 ms
    ASSERT_OK(_zp_periodic_task_add(inner, &cl, 50, &id));
    ASSERT_TRUE(id != 0);

    // Give time to pass, but *drive* the scheduler via the user API
    z_sleep_ms(60);
    ASSERT_OK(zp_process_periodic_tasks(z_loan(s)));
    int first = ctx.hits;
    ASSERT_TRUE(first >= 1);

    z_sleep_ms(60);
    ASSERT_OK(zp_process_periodic_tasks(z_loan(s)));
    int second = ctx.hits;
    ASSERT_TRUE(second >= first + 1);

    /* Skip-missed-periods behavior: if we sleep long (>2 periods) and then process once,
     * we should observe at most +1 hit in that processing step (no catch-up loop).
     */
    z_sleep_ms(175);  // > 3 periods
    int before = ctx.hits;
    ASSERT_OK(zp_process_periodic_tasks(z_loan(s)));
    int after = ctx.hits;
    ASSERT_TRUE(after == before + 1);

    // Remove the task; no more hits afterwards
    int drops_before = ctx.drops;
    ASSERT_OK(_zp_periodic_task_remove(inner, id));
    z_sleep_ms(70);
    ASSERT_OK(zp_process_periodic_tasks(z_loan(s)));
    ASSERT_TRUE(ctx.hits == after);
    ASSERT_TRUE(ctx.drops == drops_before + 1);

    z_drop(z_move(s));
}

static void test_scheduler_clears_on_close(void) {
    printf("test_scheduler_clears_on_close()\n");

    z_owned_session_t s;
    z_owned_config_t c;
    z_config_default(&c);
    ASSERT_OK(z_open(&s, z_config_move(&c), NULL));

    _z_session_t *inner = _Z_RC_IN_VAL(z_loan(s));
    ASSERT_TRUE(inner != NULL);

    zp_task_periodic_scheduler_options_t opts;
    zp_task_periodic_scheduler_options_default(&opts);

    ASSERT_OK(zp_start_periodic_scheduler_task(z_loan_mut(s), &opts));

    user_task_ctx_t ctx = {0};
    _zp_closure_periodic_task_t cl = mk_closure(&ctx);
    uint32_t id = 0;

    ASSERT_OK(_zp_periodic_task_add(inner, &cl, 200, &id));
    ASSERT_TRUE(id != 0);

    // Close without removing: scheduler must drop() remaining task(s)
    z_drop(z_move(s));
    ASSERT_TRUE(ctx.drops >= 1);
}

#if Z_FEATURE_MULTI_THREAD == 1
static void test_start_stop_scheduler_task(void) {
    printf("test_start_stop_scheduler_task()\n");

    z_owned_session_t s;
    z_owned_config_t c;
    z_config_default(&c);
    ASSERT_OK(z_open(&s, z_config_move(&c), NULL));

    _z_session_t *inner = _Z_RC_IN_VAL(z_loan(s));
    ASSERT_TRUE(inner != NULL);

    zp_task_periodic_scheduler_options_t opts;
    zp_task_periodic_scheduler_options_default(&opts);

    ASSERT_OK(zp_start_periodic_scheduler_task(z_loan_mut(s), &opts));

    user_task_ctx_t ctx = {0};
    _zp_closure_periodic_task_t cl = mk_closure(&ctx);
    uint32_t id = 0;

    // Add a 500 ms job
    ASSERT_OK(_zp_periodic_task_add(inner, &cl, 500, &id));
    ASSERT_TRUE(id != 0);

    // Allow a few periods to elapse
    z_sleep_ms(1700);
    int hits_after_start = ctx.hits;
    ASSERT_TRUE(hits_after_start >= 2);  // typically 3–4, but allow margin

    // Stop the scheduler: hits must stop increasing
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s)));
    int stop_snapshot = ctx.hits;
    z_sleep_ms(1500);
    ASSERT_TRUE(ctx.hits == stop_snapshot);

    // Remove task; should call drop exactly once
    int drops_before = ctx.drops;
    ASSERT_OK(_zp_periodic_task_remove(inner, id));
    ASSERT_TRUE(ctx.drops == drops_before + 1);

    z_drop(z_move(s));
}
#endif  // Z_FEATURE_MULTI_THREAD == 1

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_process_periodic_tasks();
    test_scheduler_clears_on_close();
#if Z_FEATURE_MULTI_THREAD == 1
    test_start_stop_scheduler_task();
#endif
    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf(
        "Missing config token to build this test. This test requires: Z_FEATURE_PERIODIC_TASKS and"
        " Z_FEATURE_UNSTABLE_API\n");
    return 0;
}

#endif
