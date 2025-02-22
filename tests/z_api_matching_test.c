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

static const char* PUB_EXPR = "zenoh-pico/matching/test/val";
static const char* SUB_EXPR = "zenoh-pico/matching/test/*";
static const char* SUB_EXPR_WRONG = "zenoh-pico/matching/test_wrong/*";

static const char* QUERIABLE_EXPR = "zenoh-pico/matching/query_test/val";
static const char* QUERY_EXPR = "zenoh-pico/matching/query_test/*";

static unsigned long DEFAULT_TIMEOUT_S = 10;
static int SUBSCRIBER_TESTS_COUNT = 3;

typedef enum { NONE, MATCH, UNMATCH, DROP } context_state_t;

typedef struct context_t {
#if Z_FEATURE_MULTI_THREAD == 1
    z_owned_condvar_t cv;
    z_owned_mutex_t m;
#else
    z_loaned_session_t* s1;
    z_loaned_session_t* s2;
#endif
    context_state_t state;
} context_t;

static void _context_init(context_t* c) {
#if Z_FEATURE_MULTI_THREAD == 1
    z_condvar_init(&c->cv);
    z_mutex_init(&c->m);
#endif
    c->state = NONE;
}

static void _context_drop(context_t* c) {
#if Z_FEATURE_MULTI_THREAD == 1
    z_condvar_drop(z_condvar_move(&c->cv));
    z_mutex_drop(z_mutex_move(&c->m));
#else
    (void)c;
#endif
}

#if Z_FEATURE_MULTI_THREAD == 1
static void _context_wait(context_t* c, context_state_t state, unsigned long timeout_s) {
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
#else
static void _context_wait(context_t* c, context_state_t state, unsigned long timeout_s) {
    unsigned long tm = timeout_s * 1000;
    while (c->state == NONE && tm > 0) {
        zp_read(c->s1, NULL);
        zp_send_keep_alive(c->s1, NULL);
        zp_read(c->s2, NULL);
        zp_send_keep_alive(c->s2, NULL);
        z_sleep_ms(100);
        tm -= 100;
    }
    if (tm <= 0) {
        fprintf(stderr, "Timeout waiting for state %d\n", state);
        assert(false);
    }
    if (c->state != state) {
        fprintf(stderr, "Expected state %d, got %d\n", state, c->state);
    }
    c->state = NONE;
}
#endif

static void _context_notify(context_t* c, context_state_t state) {
#if Z_FEATURE_MULTI_THREAD == 1
    z_mutex_lock(z_mutex_loan_mut(&c->m));
#endif
    if (c->state != NONE) {
        fprintf(stderr, "State already set %d\n", c->state);
        assert(false);
    }
    c->state = state;
    fprintf(stderr, "State recieved %d\n", state);
#if Z_FEATURE_MULTI_THREAD == 1
    z_condvar_signal(z_condvar_loan_mut(&c->cv));
    z_mutex_unlock(z_mutex_loan_mut(&c->m));
#endif
}

#define assert_ok(x)                                     \
    {                                                    \
        int ret = (int)x;                                \
        if (ret != Z_OK) {                               \
            fprintf(stderr, "%s failed: %d\n", #x, ret); \
            assert(false);                               \
        }                                                \
    }

void on_receive(const z_matching_status_t* s, void* context) {
    context_t* c = (context_t*)context;
    _context_notify(c, s->matching ? MATCH : UNMATCH);
}

void on_drop(void* context) {
    context_t* c = (context_t*)context;
    _context_notify(c, DROP);
}

void test_matching_publisher_sub(bool background) {
    printf("test_publisher_matching_sub: background=%d\n", background);

    context_t context = {0};
    _context_init(&context);

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_sub, k_pub;
    z_view_keyexpr_from_str(&k_sub, SUB_EXPR);
    z_view_keyexpr_from_str(&k_pub, PUB_EXPR);

    assert_ok(z_open(&s1, z_config_move(&c1), NULL));
    assert_ok(z_open(&s2, z_config_move(&c2), NULL));

#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_start_read_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_read_task(z_loan_mut(s2), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s2), NULL));
#else
    context.s1 = z_loan_mut(s1);
    context.s2 = z_loan_mut(s2);
#endif

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

    for (int i = 0; i != SUBSCRIBER_TESTS_COUNT; i++) {
        z_owned_subscriber_t sub;
        z_owned_closure_sample_t callback;
        z_closure_sample(&callback, NULL, NULL, NULL);
        assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub, z_view_keyexpr_loan(&k_sub),
                                       z_closure_sample_move(&callback), NULL));

        _context_wait(&context, MATCH, DEFAULT_TIMEOUT_S);

        z_subscriber_drop(z_subscriber_move(&sub));

        _context_wait(&context, UNMATCH, DEFAULT_TIMEOUT_S);
    }

    z_publisher_drop(z_publisher_move(&pub));

    _context_wait(&context, DROP, DEFAULT_TIMEOUT_S);

    if (!background) {
        z_matching_listener_drop(z_matching_listener_move(&matching_listener));
    }
#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_stop_read_task(z_loan_mut(s1)));
    assert_ok(zp_stop_read_task(z_loan_mut(s2)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s1)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s2)));
#endif

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));

    _context_drop(&context);
}

void test_matching_querier_sub(bool background) {
    printf("test_matching_querier_sub: background=%d\n", background);

    context_t context = {0};
    _context_init(&context);

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_queryable, k_querier;
    z_view_keyexpr_from_str(&k_queryable, QUERIABLE_EXPR);
    z_view_keyexpr_from_str(&k_querier, QUERY_EXPR);

    assert_ok(z_open(&s1, z_config_move(&c1), NULL));
    assert_ok(z_open(&s2, z_config_move(&c2), NULL));

#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_start_read_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_read_task(z_loan_mut(s2), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s2), NULL));
#else
    context.s1 = z_loan_mut(s1);
    context.s2 = z_loan_mut(s2);
#endif

    z_owned_querier_t querier;
    assert_ok(z_declare_querier(z_session_loan(&s1), &querier, z_view_keyexpr_loan(&k_querier), NULL));

    z_owned_closure_matching_status_t closure;
    z_closure_matching_status(&closure, on_receive, on_drop, (void*)(&context));

    z_owned_matching_listener_t matching_listener;
    if (background) {
        assert_ok(z_querier_declare_background_matching_listener(z_querier_loan(&querier),
                                                                 z_closure_matching_status_move(&closure)));
    } else {
        assert_ok(z_querier_declare_matching_listener(z_querier_loan(&querier), &matching_listener,
                                                      z_closure_matching_status_move(&closure)));
    }

    for (int i = 0; i != SUBSCRIBER_TESTS_COUNT; i++) {
        z_owned_queryable_t queryable;
        z_owned_closure_query_t callback;
        z_closure_query(&callback, NULL, NULL, NULL);
        assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable, z_view_keyexpr_loan(&k_queryable),
                                      z_closure_query_move(&callback), NULL));

        _context_wait(&context, MATCH, DEFAULT_TIMEOUT_S);

        z_queryable_drop(z_queryable_move(&queryable));

        _context_wait(&context, UNMATCH, DEFAULT_TIMEOUT_S);
    }

    z_querier_drop(z_querier_move(&querier));

    _context_wait(&context, DROP, DEFAULT_TIMEOUT_S);

    if (!background) {
        z_matching_listener_drop(z_matching_listener_move(&matching_listener));
    }

#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_stop_read_task(z_loan_mut(s1)));
    assert_ok(zp_stop_read_task(z_loan_mut(s2)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s1)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s2)));
#endif

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));

    _context_drop(&context);
}

static void _check_publisher_status(z_owned_publisher_t* pub, z_loaned_session_t* s1, z_loaned_session_t* s2,
                                    bool expected) {
    z_matching_status_t status;
    status.matching = !expected;
    z_clock_t clock = z_clock_now();
    while (status.matching != expected && z_clock_elapsed_s(&clock) < DEFAULT_TIMEOUT_S) {
        assert_ok(z_publisher_get_matching_status(z_publisher_loan(pub), &status));
        z_sleep_ms(100);
#if Z_FEATURE_MULTI_THREAD == 1
        (void)s1;
        (void)s2;
#else
        zp_read(s1, NULL);
        zp_send_keep_alive(s1, NULL);
        zp_read(s2, NULL);
        zp_send_keep_alive(s2, NULL);
#endif
    }
    if (status.matching != expected) {
        fprintf(stderr, "Expected matching status %d, got %d\n", expected, status.matching);
        assert(false);
    }
}

void test_matching_publisher_get(void) {
    printf("test_matching_publisher_get\n");

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_sub, k_pub, k_sub_wrong;
    z_view_keyexpr_from_str(&k_sub, SUB_EXPR);
    z_view_keyexpr_from_str(&k_pub, PUB_EXPR);
    z_view_keyexpr_from_str(&k_sub_wrong, SUB_EXPR_WRONG);

    assert_ok(z_open(&s1, z_config_move(&c1), NULL));
    assert_ok(z_open(&s2, z_config_move(&c2), NULL));

#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_start_read_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_read_task(z_loan_mut(s2), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s2), NULL));
#endif

    z_owned_publisher_t pub;
    assert_ok(z_declare_publisher(z_session_loan(&s1), &pub, z_view_keyexpr_loan(&k_pub), NULL));
    z_sleep_s(1);

    _check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), false);

    z_owned_subscriber_t sub_wrong;
    z_owned_closure_sample_t callback_wrong;
    z_closure_sample(&callback_wrong, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub_wrong, z_view_keyexpr_loan(&k_sub_wrong),
                                   z_closure_sample_move(&callback_wrong), NULL));
    z_sleep_s(1);

    _check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), false);

    z_owned_subscriber_t sub;
    z_owned_closure_sample_t callback;
    z_closure_sample(&callback, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub, z_view_keyexpr_loan(&k_sub),
                                   z_closure_sample_move(&callback), NULL));

    _check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), true);

    z_subscriber_drop(z_subscriber_move(&sub));

    _check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), false);

    z_publisher_drop(z_publisher_move(&pub));
    z_subscriber_drop(z_subscriber_move(&sub_wrong));

#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_stop_read_task(z_loan_mut(s1)));
    assert_ok(zp_stop_read_task(z_loan_mut(s2)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s1)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s2)));
#endif

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

static void _check_querier_status(z_owned_querier_t* querier, z_loaned_session_t* s1, z_loaned_session_t* s2,
                                  bool expected) {
    z_matching_status_t status;
    status.matching = !expected;
    z_clock_t clock = z_clock_now();
    while (status.matching != expected && z_clock_elapsed_s(&clock) < DEFAULT_TIMEOUT_S) {
        assert_ok(z_querier_get_matching_status(z_querier_loan(querier), &status));
#if Z_FEATURE_MULTI_THREAD == 1
        (void)s1;
        (void)s2;
#else
        zp_read(s1, NULL);
        zp_send_keep_alive(s1, NULL);
        zp_read(s2, NULL);
        zp_send_keep_alive(s2, NULL);
#endif
        z_sleep_ms(100);
    }
    if (status.matching != expected) {
        fprintf(stderr, "Expected matching status %d, got %d\n", expected, status.matching);
        assert(false);
    }
}

void test_matching_querier_get(void) {
    printf("test_matching_querier_get\n");

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_sub, k_querier, k_querier_wrong;
    z_view_keyexpr_from_str(&k_sub, QUERIABLE_EXPR);
    z_view_keyexpr_from_str(&k_querier, QUERY_EXPR);
    z_view_keyexpr_from_str(&k_querier_wrong, SUB_EXPR_WRONG);

    assert_ok(z_open(&s1, z_config_move(&c1), NULL));
    assert_ok(z_open(&s2, z_config_move(&c2), NULL));

#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_start_read_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_read_task(z_loan_mut(s2), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s2), NULL));
#endif

    z_owned_querier_t querier;
    assert_ok(z_declare_querier(z_session_loan(&s1), &querier, z_view_keyexpr_loan(&k_querier), NULL));
    z_sleep_s(1);

    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), false);

    z_owned_queryable_t queryable_wrong;
    z_owned_closure_query_t callback_wrong;
    z_closure_query(&callback_wrong, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable_wrong, z_view_keyexpr_loan(&k_querier_wrong),
                                  z_closure_query_move(&callback_wrong), NULL));
    z_sleep_s(1);

    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), false);

    z_owned_queryable_t queryable;
    z_owned_closure_query_t callback;
    z_closure_query(&callback, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable, z_view_keyexpr_loan(&k_sub),
                                  z_closure_query_move(&callback), NULL));

    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), true);

    z_queryable_drop(z_queryable_move(&queryable));

    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), false);

    z_querier_drop(z_querier_move(&querier));
    z_queryable_drop(z_queryable_move(&queryable_wrong));

#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_stop_read_task(z_loan_mut(s1)));
    assert_ok(zp_stop_read_task(z_loan_mut(s2)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s1)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s2)));
#endif

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    test_matching_publisher_sub(true);
    test_matching_publisher_sub(false);
    test_matching_publisher_get();

    test_matching_querier_sub(true);
    test_matching_querier_sub(false);
    test_matching_querier_get();
}

#else
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
}
#endif
