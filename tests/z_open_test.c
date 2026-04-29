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
#if defined(ZENOH_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"
#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/transport/transport.h"

#define OPEN_TEST_UNUSED_LOCATOR_1 "tcp/127.0.0.1:18101"
#define OPEN_TEST_UNUSED_LOCATOR_2 "tcp/127.0.0.1:18102"
#define OPEN_TEST_UNUSED_LOCATOR_3 "tcp/127.0.0.1:18103"
#define OPEN_TEST_UNUSED_LOCATOR_4 "tcp/127.0.0.1:18104"
#define OPEN_TEST_ALT_LOCATOR "tcp/127.0.0.1:18105"

#define OPEN_TEST_MT_LOCATOR_1 "tcp/127.0.0.1:18111"
#define OPEN_TEST_MT_LOCATOR_2 "tcp/127.0.0.1:18112"
#define OPEN_TEST_MT_LOCATOR_3 "tcp/127.0.0.1:18113"
#define OPEN_TEST_MT_LOCATOR_4 "tcp/127.0.0.1:18114"
#define OPEN_TEST_MT_LOCATOR_5 "tcp/127.0.0.1:18115"
#define OPEN_TEST_MT_LOCATOR_6 "tcp/127.0.0.1:18116"
#define OPEN_TEST_MT_LOCATOR_7 "tcp/127.0.0.1:18117"

#define OPEN_TEST_ST_LOCATOR_1 "tcp/127.0.0.1:18121"
#define OPEN_TEST_ST_LOCATOR_2 "tcp/127.0.0.1:18122"
#define OPEN_TEST_ST_LOCATOR_3 "tcp/127.0.0.1:18123"

// Keep this conservative: busy CI runners may delay executor progress after z_open().
#define OPEN_TEST_LISTENER_SETTLE_MS 1000

/*
 * These OS thread helpers are test-harness concurrency only. They let the tests
 * exercise a single-threaded zenoh-pico build while another in-process session
 * is blocked in z_open(), and the main test thread can keep spinning listeners.
 */
#if defined(ZENOH_WINDOWS)
typedef HANDLE open_test_task_t;
typedef DWORD open_test_task_ret_t;
#define OPEN_TEST_TASK_CALL WINAPI
#define OPEN_TEST_TASK_RETURN 0
#else
typedef pthread_t open_test_task_t;
typedef void *open_test_task_ret_t;
#define OPEN_TEST_TASK_CALL
#define OPEN_TEST_TASK_RETURN NULL
#endif

typedef struct {
    z_owned_config_t config;
    z_owned_session_t session;
    uint32_t delay_ms;
    volatile bool done;
    z_result_t ret;
} open_test_async_open_t;

typedef struct {
    z_owned_session_t *session;
    volatile bool done;
} open_test_async_spin_t;

static z_result_t open_test_task_init(open_test_task_t *task, open_test_task_ret_t(OPEN_TEST_TASK_CALL *fun)(void *),
                                      void *arg) {
#if defined(ZENOH_WINDOWS)
    *task = CreateThread(NULL, 0, fun, arg, 0, NULL);
    return (*task != NULL) ? _Z_RES_OK : _Z_ERR_GENERIC;
#else
    return (pthread_create(task, NULL, fun, arg) == 0) ? _Z_RES_OK : _Z_ERR_GENERIC;
#endif
}

static z_result_t open_test_task_join(open_test_task_t *task) {
#if defined(ZENOH_WINDOWS)
    DWORD ret = WaitForSingleObject(*task, INFINITE);
    CloseHandle(*task);
    return (ret == WAIT_OBJECT_0) ? _Z_RES_OK : _Z_ERR_GENERIC;
#else
    return (pthread_join(*task, NULL) == 0) ? _Z_RES_OK : _Z_ERR_GENERIC;
#endif
}

static open_test_task_ret_t OPEN_TEST_TASK_CALL open_test_async_open_task(void *arg) {
    open_test_async_open_t *ctx = (open_test_async_open_t *)arg;

    z_sleep_ms(ctx->delay_ms);
    ctx->ret = z_open(&ctx->session, z_move(ctx->config), NULL);
    ctx->done = true;

    return OPEN_TEST_TASK_RETURN;
}

static void open_test_start_async_open(open_test_task_t *task, open_test_async_open_t *ctx, z_owned_config_t config,
                                       uint32_t delay_ms) {
    ctx->config = config;
    ctx->delay_ms = delay_ms;
    ctx->done = false;
    ctx->ret = _Z_ERR_GENERIC;
    ASSERT_OK(open_test_task_init(task, open_test_async_open_task, ctx));
}

static void open_test_spin_once(z_owned_session_t *session) {
#if Z_FEATURE_MULTI_THREAD == 0
    (void)zp_spin_once(z_session_loan(session));
#else
    _ZP_UNUSED(session);
#endif
}

#if Z_FEATURE_MULTI_THREAD == 0
static open_test_task_ret_t OPEN_TEST_TASK_CALL open_test_async_spin_task(void *arg) {
    open_test_async_spin_t *ctx = (open_test_async_spin_t *)arg;
    while (!ctx->done) {
        open_test_spin_once(ctx->session);
        z_sleep_ms(10);
    }

    return OPEN_TEST_TASK_RETURN;
}

static void open_test_start_async_spin(open_test_task_t *task, open_test_async_spin_t *ctx,
                                       z_owned_session_t *session) {
    ctx->session = session;
    ctx->done = false;
    ASSERT_OK(open_test_task_init(task, open_test_async_spin_task, ctx));
}

static void open_test_stop_async_spin(open_test_task_t *task, open_test_async_spin_t *ctx) {
    ctx->done = true;
    ASSERT_OK(open_test_task_join(task));
}
#endif

static size_t open_test_peer_count(z_owned_session_t *session) {
    _z_session_t *zs = _Z_RC_IN_VAL(&session->_rc);
    if (zs->_tp._type != _Z_TRANSPORT_UNICAST_TYPE) {
        return 0;
    }

    _z_transport_unicast_t *ztu = &zs->_tp._transport._unicast;
    _z_transport_peer_mutex_lock(&ztu->_common);
    size_t len = _z_transport_peer_unicast_slist_len(ztu->_peers);
    _z_transport_peer_mutex_unlock(&ztu->_common);
    return len;
}

static bool open_test_wait_for_peer_count(z_owned_session_t *target, size_t expected_count,
                                          z_owned_session_t **sessions, size_t session_count, uint32_t timeout_ms) {
    z_clock_t start = z_clock_now();
    while (z_clock_elapsed_ms(&start) < timeout_ms) {
        if (open_test_peer_count(target) >= expected_count) {
            return true;
        }

        for (size_t i = 0; i < session_count; i++) {
            open_test_spin_once(sessions[i]);
        }
        z_sleep_ms(50);
    }

    return open_test_peer_count(target) >= expected_count;
}

static void open_test_settle_listener(z_owned_session_t **sessions, size_t session_count) {
    z_clock_t start = z_clock_now();
    do {
        for (size_t i = 0; i < session_count; i++) {
            open_test_spin_once(sessions[i]);
        }
        z_sleep_ms(10);
    } while (z_clock_elapsed_ms(&start) < OPEN_TEST_LISTENER_SETTLE_MS);
}

static void open_test_wait_for_async_open(open_test_async_open_t *ctx, z_owned_session_t **sessions,
                                          size_t session_count, uint32_t timeout_ms) {
    z_clock_t start = z_clock_now();
    while (!ctx->done && z_clock_elapsed_ms(&start) < timeout_ms) {
        for (size_t i = 0; i < session_count; i++) {
            open_test_spin_once(sessions[i]);
        }
        z_sleep_ms(10);
    }
    ASSERT_TRUE(ctx->done);
}

static void test_open_timeout_single_locator(void) {
    printf("Running test_open_timeout_single_locator() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_KEY, OPEN_TEST_UNUSED_LOCATOR_1);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_TIMEOUT_KEY, "1000");

    z_owned_session_t s;

    z_clock_t start = z_clock_now();
    z_result_t ret = z_open(&s, z_move(c), NULL);
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);

    ASSERT_ERR(ret, _Z_ERR_TRANSPORT_OPEN_FAILED);
    ASSERT_TRUE(elapsed_ms >= 1000);
}

static void test_open_client_connect_exit_on_failure_false_still_requires_transport(void) {
    printf("Running test_open_client_connect_exit_on_failure_false_still_requires_transport() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "client");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_KEY, OPEN_TEST_UNUSED_LOCATOR_2);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_TIMEOUT_KEY, "1000");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_EXIT_ON_FAILURE_KEY, "false");

    z_owned_session_t s;

    z_clock_t start = z_clock_now();
    z_result_t ret = z_open(&s, z_move(c), NULL);
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);

    ASSERT_ERR(ret, _Z_ERR_TRANSPORT_OPEN_FAILED);
    ASSERT_TRUE(elapsed_ms >= 1000);
}

static void test_open_invalid_timeout_value(void) {
    printf("Running test_open_invalid_timeout_value() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_KEY, OPEN_TEST_UNUSED_LOCATOR_3);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_TIMEOUT_KEY, "not-a-number");

    z_owned_session_t s;
    ASSERT_ERR(z_open(&s, z_move(c), NULL), _Z_ERR_CONFIG_INVALID_VALUE);
}

static void test_open_timeout_overflow_value(void) {
    printf("Running test_open_timeout_overflow_value() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_KEY, OPEN_TEST_UNUSED_LOCATOR_3);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_TIMEOUT_KEY, "2147483648");

    z_owned_session_t s;
    ASSERT_ERR(z_open(&s, z_move(c), NULL), _Z_ERR_CONFIG_INVALID_VALUE);
}

static void test_open_invalid_bool_value(void) {
    printf("Running test_open_invalid_bool_value() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_KEY, OPEN_TEST_UNUSED_LOCATOR_4);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_EXIT_ON_FAILURE_KEY, "maybe");

    z_owned_session_t s;
    ASSERT_ERR(z_open(&s, z_move(c), NULL), _Z_ERR_CONFIG_INVALID_VALUE);
}

static void test_open_rejects_negative_connect_timeout_below_minus_one(void) {
    printf("Running test_open_rejects_negative_connect_timeout_below_minus_one() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_KEY, OPEN_TEST_UNUSED_LOCATOR_1);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_CONNECT_TIMEOUT_KEY, "-2");

    z_owned_session_t s;
    ASSERT_ERR(z_open(&s, z_move(c), NULL), _Z_ERR_CONFIG_INVALID_VALUE);
}

static void test_open_rejects_negative_listen_timeout_below_minus_one(void) {
    printf("Running test_open_rejects_negative_listen_timeout_below_minus_one() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_LISTEN_KEY, OPEN_TEST_UNUSED_LOCATOR_2);
    zp_config_insert(z_loan_mut(c), Z_CONFIG_LISTEN_TIMEOUT_KEY, "-2");

    z_owned_session_t s;
    ASSERT_ERR(z_open(&s, z_move(c), NULL), _Z_ERR_CONFIG_INVALID_VALUE);
}

static void test_open_multiple_listen_locators_are_rejected(void) {
    printf("Running test_open_multiple_listen_locators_are_rejected() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    _z_str_intmap_insert_push(z_loan_mut(c), Z_CONFIG_LISTEN_KEY, _z_str_clone(OPEN_TEST_UNUSED_LOCATOR_1));
    _z_str_intmap_insert_push(z_loan_mut(c), Z_CONFIG_LISTEN_KEY, _z_str_clone(OPEN_TEST_UNUSED_LOCATOR_2));

    z_owned_session_t s;
    ASSERT_ERR(z_open(&s, z_move(c), NULL), _Z_ERR_CONFIG_LOCATOR_INVALID);
}

static void test_open_peer_listen_succeeds(void) {
    printf("Running test_open_peer_listen_succeeds() ...\n");

    z_owned_config_t c;
    z_config_default(&c);

    zp_config_insert(z_loan_mut(c), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c), Z_CONFIG_LISTEN_KEY, OPEN_TEST_ALT_LOCATOR);

    z_owned_session_t s;
    ASSERT_OK(z_open(&s, z_move(c), NULL));
    z_drop(z_move(s));
}

static void test_open_peer_uses_next_connect_locator_for_primary_transport(void) {
    printf("Running test_open_peer_uses_next_connect_locator_for_primary_transport() ...\n");

    z_owned_config_t c1;
    z_owned_config_t c2;

    z_config_default(&c1);
    z_config_default(&c2);

    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, OPEN_TEST_ALT_LOCATOR);

    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, OPEN_TEST_UNUSED_LOCATOR_1);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, OPEN_TEST_ALT_LOCATOR);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_TIMEOUT_KEY, "1000");

    z_owned_session_t s1;
    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    z_owned_session_t *listener_sessions[] = {&s1};
    open_test_settle_listener(listener_sessions, _ZP_ARRAY_SIZE(listener_sessions));

    open_test_task_t task;
    open_test_async_open_t ctx;
    open_test_start_async_open(&task, &ctx, c2, 0);
    open_test_wait_for_async_open(&ctx, listener_sessions, _ZP_ARRAY_SIZE(listener_sessions), 3000);
    ASSERT_OK(open_test_task_join(&task));
    ASSERT_OK(ctx.ret);

    z_owned_session_t *sessions[] = {&s1, &ctx.session};
    ASSERT_TRUE(open_test_wait_for_peer_count(&s1, 1, sessions, _ZP_ARRAY_SIZE(sessions), 1000));

    z_drop(z_move(ctx.session));
    z_drop(z_move(s1));
}

#if Z_FEATURE_UNICAST_PEER == 1
static void _test_open_timeout_partial_connectivity(const char *connect_exit_on_failure, z_result_t expected_ret,
                                                    const char *good_locator, const char *bad_locator) {
    z_owned_config_t c1;
    z_owned_config_t c2;

    z_config_default(&c1);
    z_config_default(&c2);

    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, good_locator);

    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, good_locator);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, bad_locator);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_TIMEOUT_KEY, "1000");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_EXIT_ON_FAILURE_KEY, connect_exit_on_failure);

    z_owned_session_t s1;

    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    z_owned_session_t *listener_sessions[] = {&s1};
    open_test_settle_listener(listener_sessions, _ZP_ARRAY_SIZE(listener_sessions));

    z_clock_t start = z_clock_now();
    open_test_task_t task;
    open_test_async_open_t ctx;
    open_test_start_async_open(&task, &ctx, c2, 0);
    open_test_wait_for_async_open(&ctx, listener_sessions, _ZP_ARRAY_SIZE(listener_sessions), 3000);
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);
    ASSERT_OK(open_test_task_join(&task));
    z_result_t ret = ctx.ret;

    ASSERT_ERR(ret, expected_ret);

    if ((expected_ret == _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY) ||
        (expected_ret == _Z_ERR_TRANSPORT_OPEN_FAILED)) {
        ASSERT_TRUE(elapsed_ms >= 1000);
    }

    if (ret == _Z_RES_OK) {
        ASSERT_TRUE(open_test_peer_count(&ctx.session) >= 1);
        z_drop(z_move(ctx.session));
    }
    z_drop(z_move(s1));
}

static void test_open_timeout_partial_connectivity_exit_on_failure_false(void) {
    printf("Running test_open_timeout_partial_connectivity_exit_on_failure_false() ...\n");

    _test_open_timeout_partial_connectivity("false", _Z_RES_OK, OPEN_TEST_MT_LOCATOR_1, OPEN_TEST_MT_LOCATOR_2);
}

static void test_open_timeout_partial_connectivity_exit_on_failure_true(void) {
    printf("Running test_open_timeout_partial_connectivity_exit_on_failure_true() ...\n");

    _test_open_timeout_partial_connectivity("true", _Z_ERR_TRANSPORT_OPEN_PARTIAL_CONNECTIVITY, OPEN_TEST_MT_LOCATOR_3,
                                            OPEN_TEST_MT_LOCATOR_4);
}

static void test_open_peer_listen_failure_strict_fails_before_connect(void) {
    printf("Running test_open_peer_listen_failure_strict_fails_before_connect() ...\n");

    z_owned_config_t c1;
    z_config_default(&c1);

    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, OPEN_TEST_MT_LOCATOR_5);

    z_owned_session_t s1;
    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    z_owned_session_t *listener_sessions[] = {&s1};
    open_test_settle_listener(listener_sessions, _ZP_ARRAY_SIZE(listener_sessions));

    z_owned_config_t c2;
    z_config_default(&c2);

    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_LISTEN_KEY, OPEN_TEST_MT_LOCATOR_5);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_LISTEN_EXIT_ON_FAILURE_KEY, "true");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, OPEN_TEST_MT_LOCATOR_5);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_TIMEOUT_KEY, "1000");

    z_owned_session_t s2;
    ASSERT_ERR(z_open(&s2, z_move(c2), NULL), _Z_ERR_TRANSPORT_OPEN_FAILED);

    z_drop(z_move(s1));
}

static void test_open_peer_listen_failure_can_fallback_to_connect(void) {
    printf("Running test_open_peer_listen_failure_can_fallback_to_connect() ...\n");

    z_owned_config_t c1;
    z_config_default(&c1);

    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, OPEN_TEST_MT_LOCATOR_6);

    z_owned_session_t s1;
    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    z_owned_session_t *listener_sessions[] = {&s1};
    open_test_settle_listener(listener_sessions, _ZP_ARRAY_SIZE(listener_sessions));

    z_owned_config_t c2;
    z_config_default(&c2);

    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_LISTEN_KEY, OPEN_TEST_MT_LOCATOR_6);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_LISTEN_EXIT_ON_FAILURE_KEY, "false");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, OPEN_TEST_MT_LOCATOR_6);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_TIMEOUT_KEY, "1000");

    open_test_task_t task;
    open_test_async_open_t ctx;
    open_test_start_async_open(&task, &ctx, c2, 0);
    open_test_wait_for_async_open(&ctx, listener_sessions, _ZP_ARRAY_SIZE(listener_sessions), 3000);
    ASSERT_OK(open_test_task_join(&task));
    ASSERT_OK(ctx.ret);

    z_owned_session_t *sessions[] = {&s1, &ctx.session};
    ASSERT_TRUE(open_test_wait_for_peer_count(&ctx.session, 1, sessions, _ZP_ARRAY_SIZE(sessions), 1000));

    z_drop(z_move(ctx.session));
    z_drop(z_move(s1));
}
#endif

#if Z_FEATURE_UNICAST_PEER == 1
static void test_open_pending_peer_progresses_after_partial_connectivity(void) {
    printf("Running test_open_pending_peer_progresses_after_partial_connectivity() ...\n");

    z_owned_config_t c1;
    z_owned_config_t c2;
    z_owned_config_t c3;

    z_config_default(&c1);
    z_config_default(&c2);
    z_config_default(&c3);

    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, OPEN_TEST_ST_LOCATOR_1);

    /*
     * s2 opens its own listen locator, so it has a primary transport and z_open()
     * can return without requiring the connect locators to complete immediately.
     *
     * The second connect locator is left pending until s3 starts listening.
     * This verifies that the background add-peers task completes the pending
     * connection after z_open() has accepted partial connectivity.
     */
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_LISTEN_KEY, OPEN_TEST_ST_LOCATOR_2);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, OPEN_TEST_ST_LOCATOR_1);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, OPEN_TEST_ST_LOCATOR_3);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_TIMEOUT_KEY, "5000");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_EXIT_ON_FAILURE_KEY, "false");

    zp_config_insert(z_loan_mut(c3), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c3), Z_CONFIG_LISTEN_KEY, OPEN_TEST_ST_LOCATOR_3);

    z_owned_session_t s1;
    z_owned_session_t s3;

    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    z_owned_session_t *listener_sessions[] = {&s1};
    open_test_settle_listener(listener_sessions, _ZP_ARRAY_SIZE(listener_sessions));

    open_test_task_t task;
    open_test_async_open_t ctx;
    open_test_start_async_open(&task, &ctx, c2, 0);
    open_test_wait_for_async_open(&ctx, listener_sessions, _ZP_ARRAY_SIZE(listener_sessions), 7000);
    ASSERT_OK(open_test_task_join(&task));
    ASSERT_OK(ctx.ret);

    z_owned_session_t *initial_sessions[] = {&s1, &ctx.session};
    ASSERT_TRUE(
        open_test_wait_for_peer_count(&ctx.session, 1, initial_sessions, _ZP_ARRAY_SIZE(initial_sessions), 1000));

    ASSERT_OK(z_open(&s3, z_move(c3), NULL));
    z_owned_session_t *late_listener_sessions[] = {&s1, &s3};
    open_test_settle_listener(late_listener_sessions, _ZP_ARRAY_SIZE(late_listener_sessions));

#if Z_FEATURE_MULTI_THREAD == 0
    open_test_task_t spin_task;
    open_test_async_spin_t spin_ctx;
    open_test_start_async_spin(&spin_task, &spin_ctx, &ctx.session);
#endif

    z_owned_session_t *sessions[] = {&s1, &s3};
    ASSERT_TRUE(open_test_wait_for_peer_count(&ctx.session, 2, sessions, _ZP_ARRAY_SIZE(sessions), 5000));
#if Z_FEATURE_MULTI_THREAD == 0
    open_test_stop_async_spin(&spin_task, &spin_ctx);
#endif

    z_drop(z_move(s3));
    z_drop(z_move(ctx.session));
    z_drop(z_move(s1));
}
#endif

static void test_open_late_joining_endpoint(void) {
    printf("Running test_open_late_joining_endpoint() ...\n");

    z_owned_config_t c1;
    z_owned_config_t c2;

    z_config_default(&c1);
    z_config_default(&c2);

    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, OPEN_TEST_MT_LOCATOR_7);

    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, OPEN_TEST_MT_LOCATOR_7);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_TIMEOUT_KEY, "5000");

    open_test_task_t task;
    open_test_async_open_t ctx;
    z_clock_t start = z_clock_now();
    open_test_start_async_open(&task, &ctx, c2, 0);

    z_sleep_ms(1000);
    z_owned_session_t s1;
    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    z_owned_session_t *listener_sessions[] = {&s1};
    open_test_wait_for_async_open(&ctx, listener_sessions, _ZP_ARRAY_SIZE(listener_sessions), 5000);
    unsigned long elapsed_ms = z_clock_elapsed_ms(&start);

    ASSERT_OK(open_test_task_join(&task));
    ASSERT_OK(ctx.ret);
    ASSERT_TRUE(elapsed_ms >= 1000);

    z_drop(z_move(ctx.session));
    z_drop(z_move(s1));
}

int main(void) {
    test_open_timeout_single_locator();
    test_open_client_connect_exit_on_failure_false_still_requires_transport();
    test_open_invalid_timeout_value();
    test_open_timeout_overflow_value();
    test_open_invalid_bool_value();
    test_open_rejects_negative_connect_timeout_below_minus_one();
    test_open_rejects_negative_listen_timeout_below_minus_one();
    test_open_multiple_listen_locators_are_rejected();
    test_open_peer_listen_succeeds();
    test_open_peer_uses_next_connect_locator_for_primary_transport();

#if Z_FEATURE_UNICAST_PEER == 1
    test_open_timeout_partial_connectivity_exit_on_failure_false();
    test_open_timeout_partial_connectivity_exit_on_failure_true();
    test_open_peer_listen_failure_strict_fails_before_connect();
    test_open_peer_listen_failure_can_fallback_to_connect();
    test_open_pending_peer_progresses_after_partial_connectivity();
#endif

    test_open_late_joining_endpoint();

    return 0;
}
