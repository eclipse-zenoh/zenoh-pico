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
static const char* SUB_EXPR = "zenoh-pico/matching/**";
static const char* KEY_EXPR_WRONG = "zenoh-pico/matching/test_wrong/*";

static const char* QUERYABLE_EXPR = "zenoh-pico/matching/query_test/val";
static const char* QUERIER_EXPR = "zenoh-pico/matching/query_test/*";
static const char* QUERYABLE_EXPR_WILD = "zenoh-pico/matching/query_test/**";

static unsigned long DEFAULT_TIMEOUT_S = 10;

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
static bool _context_wait(context_t* c, context_state_t state, unsigned long timeout_s) {
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
            return false;
        }
#endif
        if (c->state != state) {
            fprintf(stderr, "Expected state %d, got %d\n", state, c->state);
            return false;
        }
    }
    c->state = NONE;
    z_mutex_unlock(z_mutex_loan_mut(&c->m));
    return true;
}

static bool _context_wait_none(context_t* c, unsigned long timeout_s) {
    z_sleep_s(timeout_s);
    z_mutex_lock(z_mutex_loan_mut(&c->m));
    context_state_t s = c->state;
    z_mutex_unlock(z_mutex_loan_mut(&c->m));
    if (s != NONE) {
        fprintf(stderr, "Expected state %d, got %d\n", NONE, s);
        return false;
    }
    return true;
}
#else
static bool _context_wait_none(context_t* c, unsigned long timeout_s) {
    unsigned long tm = timeout_s * 1000;
    while (c->state == NONE && tm > 0) {
        zp_read(c->s1, NULL);
        zp_send_keep_alive(c->s1, NULL);
        zp_read(c->s2, NULL);
        zp_send_keep_alive(c->s2, NULL);
        z_sleep_ms(100);
        tm -= 100;
    }
    if (c->state != NONE) {
        fprintf(stderr, "Expected state %d, got %d\n", NONE, c->state);
        return false;
    }
    return true;
}

static bool _context_wait(context_t* c, context_state_t state, unsigned long timeout_s) {
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
        return false;
    }
    if (c->state != state) {
        fprintf(stderr, "Expected state %d, got %d\n", state, c->state);
        return false;
    }
    c->state = NONE;
    return true;
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

void test_matching_listener_publisher(bool background) {
    printf("test_matching_listener_publisher: background=%d\n", background);
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
    z_owned_subscriber_t sub, sub2;
    z_owned_closure_sample_t callback, callback2;
    z_closure_sample(&callback, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub, z_view_keyexpr_loan(&k_pub),
                                   z_closure_sample_move(&callback), NULL));

    assert(_context_wait(&context, MATCH, DEFAULT_TIMEOUT_S));

    z_closure_sample(&callback2, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub2, z_view_keyexpr_loan(&k_sub),
                                   z_closure_sample_move(&callback2), NULL));
    z_sleep_s(1);
    assert(_context_wait_none(&context, 1));
    z_subscriber_drop(z_subscriber_move(&sub));
    assert(_context_wait_none(&context, 1));
    z_subscriber_drop(z_subscriber_move(&sub2));
    assert(_context_wait(&context, UNMATCH, DEFAULT_TIMEOUT_S));
    z_publisher_drop(z_publisher_move(&pub));

    assert(_context_wait(&context, DROP, DEFAULT_TIMEOUT_S));
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

void test_matching_listener_querier(bool complete, bool background) {
    printf("test_matching_listener_querier: complete=%d, background=%d\n", complete, background);

    context_t context = {0};
    _context_init(&context);

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_queryable, k_querier, k_queryable_wild, k_queryable_wrong;
    z_view_keyexpr_from_str(&k_queryable, QUERYABLE_EXPR);
    z_view_keyexpr_from_str(&k_queryable_wild, QUERYABLE_EXPR_WILD);
    z_view_keyexpr_from_str(&k_querier, QUERIER_EXPR);
    z_view_keyexpr_from_str(&k_queryable_wrong, KEY_EXPR_WRONG);

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

    z_querier_options_t querier_opts;
    z_querier_options_default(&querier_opts);
    querier_opts.target = complete ? Z_QUERY_TARGET_ALL_COMPLETE : Z_QUERY_TARGET_BEST_MATCHING;

    z_owned_querier_t querier;
    assert_ok(z_declare_querier(z_session_loan(&s1), &querier, z_view_keyexpr_loan(&k_querier), &querier_opts));

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

    z_sleep_s(1);
    assert(_context_wait_none(&context, 1));

    z_owned_queryable_t queryable_wrong;
    z_owned_closure_query_t callback_wrong;
    z_closure_query(&callback_wrong, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable_wrong, z_view_keyexpr_loan(&k_queryable_wrong),
                                  z_closure_query_move(&callback_wrong), NULL));
    assert(_context_wait_none(&context, 1));

    z_queryable_options_t queryable_options;
    z_queryable_options_default(&queryable_options);
    queryable_options.complete = false;

    z_owned_queryable_t queryable1, queryable2, queryable3;
    z_owned_closure_query_t callback1, callback2, callback3;

    z_closure_query(&callback1, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable1, z_view_keyexpr_loan(&k_queryable),
                                  z_closure_query_move(&callback1), &queryable_options));
    if (complete) {
        assert(_context_wait_none(&context, 1));
    } else {
        assert(_context_wait(&context, MATCH, DEFAULT_TIMEOUT_S));
    }

    queryable_options.complete = false;
    z_closure_query(&callback2, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable2, z_view_keyexpr_loan(&k_queryable_wild),
                                  z_closure_query_move(&callback2), &queryable_options));
    assert(_context_wait_none(&context, 1));

    queryable_options.complete = true;
    z_closure_query(&callback3, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable3, z_view_keyexpr_loan(&k_queryable_wild),
                                  z_closure_query_move(&callback3), &queryable_options));
    if (!complete) {
        assert(_context_wait_none(&context, 1));
    } else {
        assert(_context_wait(&context, MATCH, DEFAULT_TIMEOUT_S));
    }

    z_queryable_drop(z_queryable_move(&queryable2));
    assert(_context_wait_none(&context, 1));

    z_queryable_drop(z_queryable_move(&queryable3));
    if (complete) {
        assert(_context_wait(&context, UNMATCH, DEFAULT_TIMEOUT_S));
    } else {
        assert(_context_wait_none(&context, 1));
    }

    z_queryable_drop(z_queryable_move(&queryable1));
    if (!complete) {
        assert(_context_wait(&context, UNMATCH, DEFAULT_TIMEOUT_S));
    } else {
        assert(_context_wait_none(&context, 1));
    }

    z_querier_drop(z_querier_move(&querier));
    z_queryable_drop(z_queryable_move(&queryable_wrong));

    assert(_context_wait(&context, DROP, DEFAULT_TIMEOUT_S));
#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_stop_read_task(z_loan_mut(s1)));
    assert_ok(zp_stop_read_task(z_loan_mut(s2)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s1)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s2)));
#endif

    if (!background) {
        z_matching_listener_drop(z_matching_listener_move(&matching_listener));
    }

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

static bool _check_publisher_status(z_owned_publisher_t* pub, z_loaned_session_t* s1, z_loaned_session_t* s2,
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
        return false;
    }
    return true;
}

void test_matching_status_publisher(void) {
    printf("test_matching_status_publisher\n");

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_sub, k_pub, k_sub_wrong;
    z_view_keyexpr_from_str(&k_sub, SUB_EXPR);
    z_view_keyexpr_from_str(&k_pub, PUB_EXPR);
    z_view_keyexpr_from_str(&k_sub_wrong, KEY_EXPR_WRONG);

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

    assert(_check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), false));

    z_owned_subscriber_t sub_wrong;
    z_owned_closure_sample_t callback_wrong;
    z_closure_sample(&callback_wrong, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub_wrong, z_view_keyexpr_loan(&k_sub_wrong),
                                   z_closure_sample_move(&callback_wrong), NULL));
    z_sleep_s(1);

    assert(_check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), false));

    z_owned_subscriber_t sub;
    z_owned_closure_sample_t callback;
    z_closure_sample(&callback, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub, z_view_keyexpr_loan(&k_sub),
                                   z_closure_sample_move(&callback), NULL));
    assert(_check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), true));

    z_owned_subscriber_t sub2;
    z_owned_closure_sample_t callback2;
    z_closure_sample(&callback2, NULL, NULL, NULL);
    assert_ok(z_declare_subscriber(z_session_loan(&s2), &sub2, z_view_keyexpr_loan(&k_pub),
                                   z_closure_sample_move(&callback), NULL));
    assert(_check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), true));

    z_subscriber_drop(z_subscriber_move(&sub));
    assert(_check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), true));
    z_subscriber_drop(z_subscriber_move(&sub2));
    assert(_check_publisher_status(&pub, z_loan_mut(s1), z_loan_mut(s2), false));

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

static bool _check_querier_status(z_owned_querier_t* querier, z_loaned_session_t* s1, z_loaned_session_t* s2,
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
        return false;
    }
    return true;
}

void test_matching_status_querier(bool complete) {
    printf("test_matching_status_querier: complete=%d\n", complete);

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k_queryable, k_querier, k_queryable_wrong, k_queryable_wild;
    z_view_keyexpr_from_str(&k_queryable_wild, QUERYABLE_EXPR_WILD);
    z_view_keyexpr_from_str(&k_queryable, QUERYABLE_EXPR);
    z_view_keyexpr_from_str(&k_querier, QUERIER_EXPR);
    z_view_keyexpr_from_str(&k_queryable_wrong, KEY_EXPR_WRONG);

    assert_ok(z_open(&s1, z_config_move(&c1), NULL));
    assert_ok(z_open(&s2, z_config_move(&c2), NULL));

#if Z_FEATURE_MULTI_THREAD == 1
    assert_ok(zp_start_read_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_read_task(z_loan_mut(s2), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s2), NULL));
#endif

    z_querier_options_t querier_opts;
    z_querier_options_default(&querier_opts);
    querier_opts.target = complete ? Z_QUERY_TARGET_ALL_COMPLETE : Z_QUERY_TARGET_BEST_MATCHING;

    z_owned_querier_t querier;
    assert_ok(z_declare_querier(z_session_loan(&s1), &querier, z_view_keyexpr_loan(&k_querier), &querier_opts));

    z_sleep_s(1);
    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), false);

    z_owned_queryable_t queryable_wrong;
    z_owned_closure_query_t callback_wrong;
    z_closure_query(&callback_wrong, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable_wrong, z_view_keyexpr_loan(&k_queryable_wrong),
                                  z_closure_query_move(&callback_wrong), NULL));
    z_sleep_s(1);
    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), false);

    z_queryable_options_t queryable_options;
    z_queryable_options_default(&queryable_options);
    queryable_options.complete = false;

    z_owned_queryable_t queryable1, queryable2, queryable3;
    z_owned_closure_query_t callback1, callback2, callback3;

    z_closure_query(&callback1, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable1, z_view_keyexpr_loan(&k_queryable),
                                  z_closure_query_move(&callback1), &queryable_options));
    z_sleep_s(1);
    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), !complete);

    queryable_options.complete = false;
    z_closure_query(&callback2, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable2, z_view_keyexpr_loan(&k_queryable_wild),
                                  z_closure_query_move(&callback2), &queryable_options));
    z_sleep_s(1);
    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), !complete);

    queryable_options.complete = true;
    z_closure_query(&callback3, NULL, NULL, NULL);
    assert_ok(z_declare_queryable(z_session_loan(&s2), &queryable3, z_view_keyexpr_loan(&k_queryable_wild),
                                  z_closure_query_move(&callback3), &queryable_options));
    z_sleep_s(1);
    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), true);

    z_queryable_drop(z_queryable_move(&queryable2));
    z_sleep_s(1);
    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), true);

    z_queryable_drop(z_queryable_move(&queryable3));
    z_sleep_s(1);
    _check_querier_status(&querier, z_loan_mut(s1), z_loan_mut(s2), !complete);

    z_queryable_drop(z_queryable_move(&queryable1));
    z_sleep_s(1);

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
    test_matching_listener_publisher(true);
    test_matching_listener_publisher(false);
    test_matching_status_publisher();

    test_matching_listener_querier(false, false);
    test_matching_listener_querier(true, false);
    test_matching_listener_querier(true, true);
    test_matching_listener_querier(false, true);
    test_matching_status_querier(false);
    test_matching_status_querier(true);
}

#else
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
}
#endif
