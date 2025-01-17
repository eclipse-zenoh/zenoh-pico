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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zenoh-pico.h"

#if Z_FEATURE_MATCHING == 1 && Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1

#undef NDEBUG
#include <assert.h>

typedef enum { NONE, MATCH, UNMATCH, DROP } context_state_t;

typedef struct context_t {
    z_owned_condvar_t cv;
    z_owned_mutex_t m;
    context_state_t state;
} context_t;

static void _context_init(context_t* c) {
    z_condvar_init(&c->cv);
    z_mutex_init(&c->m);
    c->state = NONE;
}

static void _context_drop(context_t* c) {
    z_condvar_drop(z_condvar_move(&c->cv));
    z_mutex_drop(z_mutex_move(&c->m));
}

static void _context_wait(context_t* c, context_state_t state, int timeout_s) {
    z_mutex_lock(z_mutex_loan_mut(&c->m));
    if (c->state != state) {
        printf("Waiting for state %d...\n", state);
#ifdef ZENOH_MACOS
        _ZP_UNUSED(timeout_s);
        z_condvar_wait(z_condvar_loan_mut(&c->cv), z_mutex_loan_mut(&c->m));
#else
        z_clock_t clock = z_clock_now();
        z_clock_advance_s(&clock, timeout_s);
        z_result_t res = z_condvar_wait_until(z_condvar_loan_mut(&c->cv), z_mutex_loan_mut(&c->m), &clock);
        if (res == Z_ETIMEDOUT) {
            fprintf(stderr, "Timeout waiting for state %d\n", state);
            assert(false);
        }
#endif
        if (c->state != state) {
            fprintf(stderr, "Expected state %d, got %d\n", state, c->state);
            assert(false);
        }
    }
    c->state = NONE;
    z_mutex_unlock(z_mutex_loan_mut(&c->m));
}

static void _context_notify(context_t* c, context_state_t state) {
    z_mutex_lock(z_mutex_loan_mut(&c->m));
    if (c->state != NONE) {
        fprintf(stderr, "State already set %d\n", c->state);
        assert(false);
    }
    c->state = state;
    fprintf(stderr, "State recieved %d\n", state);
    z_condvar_signal(z_condvar_loan_mut(&c->cv));
    z_mutex_unlock(z_mutex_loan_mut(&c->m));
}

#define assert_ok(x)                                     \
    {                                                    \
        int ret = (int)x;                                \
        if (ret != Z_OK) {                               \
            fprintf(stderr, "%s failed: %d\n", #x, ret); \
            assert(false);                               \
        }                                                \
    }

const char* pub_expr = "zenoh-pico/matching/test/val";
const char* sub_expr = "zenoh-pico/matching/test/*";
const char* sub_expr_wrong = "zenoh-pico/matching/test_wrong/*";

void on_receive(const z_matching_status_t* s, void* context) {
    context_t* c = (context_t*)context;
    _context_notify(c, s->matching ? MATCH : UNMATCH);
}

void on_drop(void* context) {
    context_t* c = (context_t*)context;
    _context_notify(c, DROP);
}

void test_matching_sub(bool background) {
    printf("test_matching_sub: background=%d\n", background);

    context_t context = {0};
    _context_init(&context);

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_sub, k_pub;
    z_view_keyexpr_from_str(&k_sub, sub_expr);
    z_view_keyexpr_from_str(&k_pub, pub_expr);

    assert_ok(z_open(&s1, z_config_move(&c1), NULL));
    assert_ok(z_open(&s2, z_config_move(&c2), NULL));

    assert_ok(zp_start_read_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_read_task(z_loan_mut(s2), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s2), NULL));

    z_owned_publisher_t pub;
    assert_ok(z_declare_publisher(z_session_loan(&s1), &pub, z_view_keyexpr_loan(&k_pub), NULL));

    z_owned_closure_matching_status_t closure;
    z_closure_matching_status(&closure, on_receive, on_drop, (void*)(&context));

    z_owned_matching_listener_t matching_listener;
    if (background) {
        assert_ok(z_publisher_declare_background_matching_listener(z_publisher_loan(&pub),
                                                                   z_closure_matching_status_move(&closure)));
    } else {
        assert_ok(z_publisher_declare_matching_listener(z_publisher_loan(&pub), &matching_listener,
                                                        z_closure_matching_status_move(&closure)));
    }

    z_owned_subscriber_t sub;
    z_owned_closure_sample_t callback;
    z_closure_sample(&callback, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub, z_view_keyexpr_loan(&k_sub),
                                   z_closure_sample_move(&callback), NULL));

    _context_wait(&context, MATCH, 10);

    z_subscriber_drop(z_subscriber_move(&sub));

    _context_wait(&context, UNMATCH, 10);

    z_publisher_drop(z_publisher_move(&pub));

    _context_wait(&context, DROP, 10);

    if (!background) {
        z_matching_listener_drop(z_matching_listener_move(&matching_listener));
    }

    assert_ok(zp_stop_read_task(z_loan_mut(s1)));
    assert_ok(zp_stop_read_task(z_loan_mut(s2)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s1)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s2)));

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));

    _context_drop(&context);
}

static void _check_status(z_owned_publisher_t* pub, bool expected) {
    z_matching_status_t status;
    status.matching = !expected;
    z_clock_t clock = z_clock_now();
    while (status.matching != expected && z_clock_elapsed_s(&clock) < 10) {
        assert_ok(z_publisher_get_matching_status(z_publisher_loan(pub), &status));
        z_sleep_ms(100);
    }
    if (status.matching != expected) {
        fprintf(stderr, "Expected matching status %d, got %d\n", expected, status.matching);
        assert(false);
    }
}

void test_matching_get(void) {
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_sub, k_pub, k_sub_wrong;
    z_view_keyexpr_from_str(&k_sub, sub_expr);
    z_view_keyexpr_from_str(&k_pub, pub_expr);
    z_view_keyexpr_from_str(&k_sub_wrong, sub_expr_wrong);

    assert_ok(z_open(&s1, z_config_move(&c1), NULL));
    assert_ok(z_open(&s2, z_config_move(&c2), NULL));

    assert_ok(zp_start_read_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_read_task(z_loan_mut(s2), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s2), NULL));

    z_owned_publisher_t pub;
    assert_ok(z_declare_publisher(z_session_loan(&s1), &pub, z_view_keyexpr_loan(&k_pub), NULL));
    z_sleep_s(1);

    _check_status(&pub, false);

    z_owned_subscriber_t sub_wrong;
    z_owned_closure_sample_t callback_wrong;
    z_closure_sample(&callback_wrong, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub_wrong, z_view_keyexpr_loan(&k_sub_wrong),
                                   z_closure_sample_move(&callback_wrong), NULL));
    z_sleep_s(1);

    _check_status(&pub, false);

    z_owned_subscriber_t sub;
    z_owned_closure_sample_t callback;
    z_closure_sample(&callback, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub, z_view_keyexpr_loan(&k_sub),
                                   z_closure_sample_move(&callback), NULL));

    _check_status(&pub, true);

    z_subscriber_drop(z_subscriber_move(&sub));

    _check_status(&pub, false);

    z_publisher_drop(z_publisher_move(&pub));
    z_subscriber_drop(z_subscriber_move(&sub_wrong));

    assert_ok(zp_stop_read_task(z_loan_mut(s1)));
    assert_ok(zp_stop_read_task(z_loan_mut(s2)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s1)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s2)));

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    test_matching_sub(true);
    test_matching_sub(false);
    test_matching_get();
}

#else
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
}
#endif
