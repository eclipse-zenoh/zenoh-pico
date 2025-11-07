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
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"

#ifdef Z_ADVANCED_PUBSUB_TEST_USE_TCP_PROXY
#include "utils/tcp_proxy.h"
#endif

#if Z_FEATURE_ADVANCED_PUBLICATION == 1 && Z_FEATURE_ADVANCED_SUBSCRIPTION == 1

// ---- Common test timing constants ----
#define TEST_SLEEP_MS 4000

// Reconnect waits used across tests
#define TEST_RECONNECT_MS 10000

// Feature-specific periods used in tests
#define TEST_PERIODIC_QUERY_MS 1000
#define TEST_HEARTBEAT_PERIOD_MS 4000
// ------------------------------------------------------------

static void put_str(const ze_loaned_advanced_publisher_t *pub, const char *s) {
    z_owned_bytes_t payload;
    ASSERT_OK(z_bytes_copy_from_str(&payload, s));
    ASSERT_OK(ze_advanced_publisher_put(pub, z_move(payload), NULL));
}

static void expect_next(const z_loaned_fifo_handler_sample_t *handler, const char *expected) {
    z_owned_sample_t sample;
    z_internal_sample_null(&sample);
    z_owned_string_t value;
    z_internal_string_null(&value);

    ASSERT_OK(z_try_recv(handler, &sample));
    assert(z_sample_kind(z_loan(sample)) == Z_SAMPLE_KIND_PUT);

    ASSERT_OK(z_bytes_to_string(z_sample_payload(z_loan(sample)), &value));

    const char *ptr = z_string_data(z_loan(value));
    size_t len = z_string_len(z_loan(value));
    // Flawfinder: ignore [CWE-126]
    size_t exp_len = strlen(expected);

    if (len != exp_len || memcmp(ptr, expected, len) != 0) {
        fprintf(stderr,
                "Mismatch:\n"
                "  expected (%zu): \"%.*s\"\n"
                "  actual   (%zu): \"%.*s\"\n",
                exp_len, (int)exp_len, expected, len, (int)len, ptr);
        fflush(stderr);
        assert(false && "expect_next() value mismatch");
    }
    z_drop(z_move(value));
    z_drop(z_move(sample));
}

static void expect_empty(const z_loaned_fifo_handler_sample_t *handler) {
    z_owned_sample_t sample;
    ASSERT_NOT_OK(z_try_recv(handler, &sample));
}

static void test_advanced_history(bool p2p) {
    printf("test_advanced_history: peer to peer=%d\n", p2p);

    const char *expr = "zenoh-pico/advanced-pubsub/test/history";

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k;
    ASSERT_OK(z_view_keyexpr_from_str(&k, expr));

    if (p2p) {
        zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
        zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, "udp/224.0.0.224:7447#iface=lo");

        zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
        zp_config_insert(z_loan_mut(c2), Z_CONFIG_LISTEN_KEY, "udp/224.0.0.224:7447#iface=lo");
    }

    ASSERT_OK(z_open(&s1, z_config_move(&c1), NULL));
    ASSERT_OK(z_open(&s2, z_config_move(&c2), NULL));

    ASSERT_OK(zp_start_read_task(z_loan_mut(s1), NULL));
    ASSERT_OK(zp_start_read_task(z_loan_mut(s2), NULL));
    ASSERT_OK(zp_start_lease_task(z_loan_mut(s1), NULL));
    ASSERT_OK(zp_start_lease_task(z_loan_mut(s2), NULL));
    ASSERT_OK(zp_start_periodic_scheduler_task(z_loan_mut(s1), NULL));
    ASSERT_OK(zp_start_periodic_scheduler_task(z_loan_mut(s2), NULL));

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    pub_opts.cache.max_samples = 3;
    pub_opts.cache.is_enabled = true;
    ASSERT_OK(ze_declare_advanced_publisher(z_loan(s1), &pub, z_loan(k), &pub_opts));

    char buf[16];
    for (int idx = 1; idx <= 4; idx++) {
        snprintf(buf, sizeof(buf), "%d", idx);
        put_str(z_loan(pub), buf);
    }
    z_sleep_ms(TEST_SLEEP_MS);

    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    ze_advanced_subscriber_history_options_default(&sub_opts.history);
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    ASSERT_OK(z_fifo_channel_sample_new(&closure, &handler, 10));
    ASSERT_OK(ze_declare_advanced_subscriber(z_loan(s2), &sub, z_loan(k), z_move(closure), &sub_opts));
    z_sleep_ms(TEST_SLEEP_MS);

    put_str(z_loan(pub), "5");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "2");
    expect_next(z_loan(handler), "3");
    expect_next(z_loan(handler), "4");
    expect_next(z_loan(handler), "5");
    expect_empty(z_loan(handler));

    ze_advanced_subscriber_drop(z_move(sub));
    ze_advanced_publisher_drop(z_move(pub));
    z_fifo_handler_sample_drop(z_move(handler));

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s2)));

    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

#ifdef Z_ADVANCED_PUBSUB_TEST_USE_TCP_PROXY
static void setup_two_peers_with_proxy(z_owned_session_t *s1, z_owned_session_t *s2, tcp_proxy_t **proxy,
                                       uint16_t upstream_listen_port) {
    *proxy = tcp_proxy_create("127.0.0.1", "127.0.0.1", upstream_listen_port);
    assert(*proxy != NULL);

    int port = tcp_proxy_start(*proxy, 100);
    assert(port > 0);

    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    char uri[128];

    // s1 listens on the fixed upstream port
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    snprintf(uri, sizeof(uri), "tcp/127.0.0.1:%u#iface=lo", (unsigned)upstream_listen_port);
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, uri);

    // s2 connects via proxy ephemeral port
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    snprintf(uri, sizeof(uri), "tcp/127.0.0.1:%u#iface=lo", (unsigned)port);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, uri);

    ASSERT_OK(z_open(s1, z_config_move(&c1), NULL));
    ASSERT_OK(z_open(s2, z_config_move(&c2), NULL));

    ASSERT_OK(zp_start_read_task(z_loan_mut(*s1), NULL));
    ASSERT_OK(zp_start_read_task(z_loan_mut(*s2), NULL));
    ASSERT_OK(zp_start_lease_task(z_loan_mut(*s1), NULL));
    ASSERT_OK(zp_start_lease_task(z_loan_mut(*s2), NULL));
    ASSERT_OK(zp_start_periodic_scheduler_task(z_loan_mut(*s1), NULL));
    ASSERT_OK(zp_start_periodic_scheduler_task(z_loan_mut(*s2), NULL));
}

static void test_advanced_retransmission(void) {
    printf("test_advanced_retransmission\n");

    const char *expr = "zenoh-pico/advanced-pubsub/test/retransmission";

    tcp_proxy_t *tcp_proxy;
    z_owned_session_t s1, s2;
    setup_two_peers_with_proxy(&s1, &s2, &tcp_proxy, 9000);

    z_view_keyexpr_t k;
    ASSERT_OK(z_view_keyexpr_from_str(&k, expr));

    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    ze_advanced_subscriber_recovery_options_default(&sub_opts.recovery);
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    ASSERT_OK(z_fifo_channel_sample_new(&closure, &handler, 10));
    ASSERT_OK(ze_declare_advanced_subscriber(z_loan(s2), &sub, z_loan(k), z_move(closure), &sub_opts));
    z_sleep_ms(TEST_SLEEP_MS);

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    ze_advanced_publisher_cache_options_default(&pub_opts.cache);
    pub_opts.cache.max_samples = 10;
    ze_advanced_publisher_sample_miss_detection_options_default(&pub_opts.sample_miss_detection);
    ASSERT_OK(ze_declare_advanced_publisher(z_loan(s1), &pub, z_loan(k), &pub_opts));

    put_str(z_loan(pub), "1");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "1");
    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, false);
    z_sleep_ms(TEST_SLEEP_MS);

    put_str(z_loan(pub), "2");
    put_str(z_loan(pub), "3");
    put_str(z_loan(pub), "4");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, true);
    z_sleep_ms(TEST_RECONNECT_MS);

    put_str(z_loan(pub), "5");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "2");
    expect_next(z_loan(handler), "3");
    expect_next(z_loan(handler), "4");
    expect_next(z_loan(handler), "5");
    expect_empty(z_loan(handler));

    z_drop(z_move(sub));
    z_drop(z_move(pub));
    z_drop(z_move(handler));

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s2)));

    z_drop(z_move(s1));
    z_drop(z_move(s2));

    tcp_proxy_stop(tcp_proxy);
    tcp_proxy_destroy(tcp_proxy);
}

static void test_advanced_retransmission_periodic(void) {
    printf("test_advanced_retransmission_periodic\n");

    const char *expr = "zenoh-pico/advanced-pubsub/test/retransmission_periodic";

    tcp_proxy_t *tcp_proxy;
    z_owned_session_t s1, s2;
    setup_two_peers_with_proxy(&s1, &s2, &tcp_proxy, 9000);

    z_view_keyexpr_t k;
    ASSERT_OK(z_view_keyexpr_from_str(&k, expr));

    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    ze_advanced_subscriber_recovery_options_default(&sub_opts.recovery);
    ze_advanced_subscriber_last_sample_miss_detection_options_default(&sub_opts.recovery.last_sample_miss_detection);
    sub_opts.recovery.last_sample_miss_detection.periodic_queries_period_ms = TEST_PERIODIC_QUERY_MS;
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    ASSERT_OK(z_fifo_channel_sample_new(&closure, &handler, 10));
    ASSERT_OK(ze_declare_advanced_subscriber(z_loan(s2), &sub, z_loan(k), z_move(closure), &sub_opts));
    z_sleep_ms(TEST_SLEEP_MS);

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    ze_advanced_publisher_cache_options_default(&pub_opts.cache);
    pub_opts.cache.max_samples = 10;
    ze_advanced_publisher_sample_miss_detection_options_default(&pub_opts.sample_miss_detection);
    ASSERT_OK(ze_declare_advanced_publisher(z_loan(s1), &pub, z_loan(k), &pub_opts));

    put_str(z_loan(pub), "1");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "1");
    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, false);
    z_sleep_ms(TEST_SLEEP_MS);

    put_str(z_loan(pub), "2");
    put_str(z_loan(pub), "3");
    put_str(z_loan(pub), "4");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, true);
    z_sleep_ms(TEST_RECONNECT_MS);

    put_str(z_loan(pub), "5");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "2");
    expect_next(z_loan(handler), "3");
    expect_next(z_loan(handler), "4");
    expect_next(z_loan(handler), "5");
    expect_empty(z_loan(handler));

    z_drop(z_move(sub));
    z_drop(z_move(pub));
    z_drop(z_move(handler));

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s2)));

    z_drop(z_move(s1));
    z_drop(z_move(s2));

    tcp_proxy_stop(tcp_proxy);
    tcp_proxy_destroy(tcp_proxy);
}

static void test_advanced_retransmission_heartbeat(void) {
    printf("test_advanced_retransmission_heartbeat\n");

    const char *expr = "zenoh-pico/advanced-pubsub/test/retransmission_heartbeat";

    tcp_proxy_t *tcp_proxy;
    z_owned_session_t s1, s2;
    setup_two_peers_with_proxy(&s1, &s2, &tcp_proxy, 9000);

    z_view_keyexpr_t k;
    ASSERT_OK(z_view_keyexpr_from_str(&k, expr));

    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    ze_advanced_subscriber_recovery_options_default(&sub_opts.recovery);
    ze_advanced_subscriber_last_sample_miss_detection_options_default(&sub_opts.recovery.last_sample_miss_detection);
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    ASSERT_OK(z_fifo_channel_sample_new(&closure, &handler, 10));
    ASSERT_OK(ze_declare_advanced_subscriber(z_loan(s2), &sub, z_loan(k), z_move(closure), &sub_opts));
    z_sleep_ms(TEST_SLEEP_MS);

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    ze_advanced_publisher_cache_options_default(&pub_opts.cache);
    pub_opts.cache.max_samples = 10;
    ze_advanced_publisher_sample_miss_detection_options_default(&pub_opts.sample_miss_detection);
    pub_opts.sample_miss_detection.heartbeat_mode = ZE_ADVANCED_PUBLISHER_HEARTBEAT_MODE_PERIODIC;
    pub_opts.sample_miss_detection.heartbeat_period_ms = TEST_HEARTBEAT_PERIOD_MS;
    ASSERT_OK(ze_declare_advanced_publisher(z_loan(s1), &pub, z_loan(k), &pub_opts));

    put_str(z_loan(pub), "1");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "1");
    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, false);
    z_sleep_ms(TEST_SLEEP_MS);

    put_str(z_loan(pub), "2");
    put_str(z_loan(pub), "3");
    put_str(z_loan(pub), "4");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, true);
    z_sleep_ms(TEST_RECONNECT_MS);

    expect_next(z_loan(handler), "2");
    expect_next(z_loan(handler), "3");
    expect_next(z_loan(handler), "4");
    expect_empty(z_loan(handler));

    z_drop(z_move(sub));
    z_drop(z_move(pub));
    z_drop(z_move(handler));

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s2)));

    z_drop(z_move(s1));
    z_drop(z_move(s2));

    tcp_proxy_stop(tcp_proxy);
    tcp_proxy_destroy(tcp_proxy);
}

#define MAX_MISS_EVENTS 16

typedef struct {
    atomic_int event_count;         // number of miss events recorded
    atomic_uint_fast64_t total_nb;  // sum of all nb across events
    z_entity_global_id_t sources[MAX_MISS_EVENTS];
    uint64_t nbs[MAX_MISS_EVENTS];
} miss_ctx_t;

static void miss_ctx_init(miss_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    atomic_store_explicit(&ctx->event_count, 0, memory_order_relaxed);
    atomic_store_explicit(&ctx->total_nb, 0, memory_order_relaxed);
}

static void miss_ctx_assert_single(const miss_ctx_t *ctx, const _z_entity_global_id_t *expected_pub_id,
                                   uint64_t expected_nb) {
    int count = atomic_load_explicit(&ctx->event_count, memory_order_acquire);
    uint64_t sum = atomic_load_explicit(&ctx->total_nb, memory_order_acquire);

    if (count != 1) {
        fprintf(stderr, "miss_ctx_assert_single: expected exactly 1 miss event, got %d\n", count);
        fflush(stderr);
        assert(false && "expected exactly one miss event");
    }

    if (sum != expected_nb) {
        fprintf(stderr, "total missed samples mismatch: expected=%" PRIu64 ", actual=%" PRIu64 "\n", expected_nb, sum);
        fflush(stderr);
        assert(false && "total missed samples does not match");
    }

    if (ctx->nbs[0] != expected_nb) {
        fprintf(stderr, "single-event nb mismatch: expected=%" PRIu64 ", actual=%" PRIu64 "\n", expected_nb,
                ctx->nbs[0]);
        fflush(stderr);
        assert(false && "unexpected nb in the single miss event");
    }

    if (!_z_entity_global_id_eq(&ctx->sources[0], expected_pub_id)) {
        z_owned_string_t id_string, expected_id_string;
        ASSERT_OK(z_id_to_string(&ctx->sources[0].zid, &id_string));
        ASSERT_OK(z_id_to_string(&expected_pub_id->zid, &expected_id_string));

        fprintf(stderr,
                "miss source GID mismatch:\n"
                "   expected: (zid=%.*s, eid=%d)\n"
                "   actual  : (zid=%.*s, eid=%d)\n",
                (int)z_string_len(z_loan(expected_id_string)), z_string_data(z_loan(expected_id_string)),
                expected_pub_id->eid, (int)z_string_len(z_loan(id_string)), z_string_data(z_loan(id_string)),
                ctx->sources[0].eid);
        fflush(stderr);
        z_drop(z_move(id_string));
        z_drop(z_move(expected_id_string));
        assert(false && "miss source should match publisher");
    }
}

static void miss_handler(const ze_miss_t *miss, void *arg) {
    miss_ctx_t *ctx = (miss_ctx_t *)arg;

    int idx = atomic_load_explicit(&ctx->event_count, memory_order_relaxed);
    assert(idx < MAX_MISS_EVENTS);

    ctx->nbs[idx] = miss->nb;
    ctx->sources[idx] = miss->source;
    atomic_fetch_add_explicit(&ctx->total_nb, miss->nb, memory_order_relaxed);

    atomic_store_explicit(&ctx->event_count, idx + 1, memory_order_release);
}

static void test_advanced_sample_miss(void) {
    printf("test_advanced_sample_miss\n");

    const char *expr = "zenoh-pico/advanced-pubsub/test/sample_miss";

    tcp_proxy_t *tcp_proxy;
    z_owned_session_t s1, s2;
    setup_two_peers_with_proxy(&s1, &s2, &tcp_proxy, 9000);

    z_view_keyexpr_t k;
    ASSERT_OK(z_view_keyexpr_from_str(&k, expr));

    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    ASSERT_OK(z_fifo_channel_sample_new(&closure, &handler, 10));
    ASSERT_OK(ze_declare_advanced_subscriber(z_loan(s2), &sub, z_loan(k), z_move(closure), &sub_opts));

    ze_owned_sample_miss_listener_t miss_listener;
    ze_owned_closure_miss_t miss_closure;
    miss_ctx_t miss_ctx;
    miss_ctx_init(&miss_ctx);
    z_closure(&miss_closure, miss_handler, NULL, &miss_ctx);
    ASSERT_OK(ze_advanced_subscriber_declare_sample_miss_listener(z_loan(sub), &miss_listener, z_move(miss_closure)));
    z_sleep_ms(TEST_SLEEP_MS);

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    ze_advanced_publisher_sample_miss_detection_options_default(&pub_opts.sample_miss_detection);
    ASSERT_OK(ze_declare_advanced_publisher(z_loan(s1), &pub, z_loan(k), &pub_opts));

    put_str(z_loan(pub), "1");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "1");
    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, false);
    z_sleep_ms(TEST_SLEEP_MS);

    put_str(z_loan(pub), "2");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, true);
    z_sleep_ms(TEST_RECONNECT_MS);

    put_str(z_loan(pub), "3");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "3");
    expect_empty(z_loan(handler));

    z_entity_global_id_t pub_id = ze_advanced_publisher_id(z_loan(pub));
    miss_ctx_assert_single(&miss_ctx, &pub_id, 1);

    z_drop(z_move(miss_listener));
    z_drop(z_move(sub));
    z_drop(z_move(pub));
    z_drop(z_move(handler));

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s2)));

    z_drop(z_move(s1));
    z_drop(z_move(s2));

    tcp_proxy_stop(tcp_proxy);
    tcp_proxy_destroy(tcp_proxy);
}

static void test_advanced_retransmission_sample_miss(void) {
    printf("test_advanced_retransmission_sample_miss\n");

    const char *expr = "zenoh-pico/advanced-pubsub/test/retransmission_sample_miss";

    tcp_proxy_t *tcp_proxy;
    z_owned_session_t s1, s2;
    setup_two_peers_with_proxy(&s1, &s2, &tcp_proxy, 9000);

    z_view_keyexpr_t k;
    ASSERT_OK(z_view_keyexpr_from_str(&k, expr));

    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    ze_advanced_subscriber_recovery_options_default(&sub_opts.recovery);
    ze_advanced_subscriber_last_sample_miss_detection_options_default(&sub_opts.recovery.last_sample_miss_detection);
    sub_opts.recovery.last_sample_miss_detection.periodic_queries_period_ms = TEST_PERIODIC_QUERY_MS;
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    ASSERT_OK(z_fifo_channel_sample_new(&closure, &handler, 10));
    ASSERT_OK(ze_declare_advanced_subscriber(z_loan(s2), &sub, z_loan(k), z_move(closure), &sub_opts));

    ze_owned_sample_miss_listener_t miss_listener;
    ze_owned_closure_miss_t miss_closure;
    miss_ctx_t miss_ctx;
    miss_ctx_init(&miss_ctx);
    z_closure(&miss_closure, miss_handler, NULL, &miss_ctx);
    ASSERT_OK(ze_advanced_subscriber_declare_sample_miss_listener(z_loan(sub), &miss_listener, z_move(miss_closure)));
    z_sleep_ms(TEST_SLEEP_MS);

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    ze_advanced_publisher_cache_options_default(&pub_opts.cache);
    pub_opts.cache.max_samples = 1;
    ze_advanced_publisher_sample_miss_detection_options_default(&pub_opts.sample_miss_detection);
    ASSERT_OK(ze_declare_advanced_publisher(z_loan(s1), &pub, z_loan(k), &pub_opts));

    put_str(z_loan(pub), "1");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "1");
    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, false);
    z_sleep_ms(TEST_SLEEP_MS);

    put_str(z_loan(pub), "2");
    put_str(z_loan(pub), "3");
    put_str(z_loan(pub), "4");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, true);
    z_sleep_ms(TEST_RECONNECT_MS);

    expect_next(z_loan(handler), "4");
    expect_empty(z_loan(handler));

    z_entity_global_id_t pub_id = ze_advanced_publisher_id(z_loan(pub));
    miss_ctx_assert_single(&miss_ctx, &pub_id, 2);

    z_drop(z_move(miss_listener));
    z_drop(z_move(sub));
    z_drop(z_move(pub));
    z_drop(z_move(handler));

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s2)));

    z_drop(z_move(s1));
    z_drop(z_move(s2));

    tcp_proxy_stop(tcp_proxy);
    tcp_proxy_destroy(tcp_proxy);
}

/* TODO: Test disabled due to session reconnection issues
static void test_advanced_late_joiner(void) {
    printf("test_advanced_late_joiner\n");

    const char *expr = "zenoh-pico/advanced-pubsub/test/late_joiner";

    tcp_proxy_t *tcp_proxy;
    z_owned_session_t s1, s2;
    setup_two_peers_with_proxy(&s1, &s2, &tcp_proxy, 9000);

    z_view_keyexpr_t k;
    ASSERT_OK(z_view_keyexpr_from_str(&k, expr));

    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    ze_advanced_subscriber_history_options_default(&sub_opts.history);
    sub_opts.history.detect_late_publishers = true;
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    ASSERT_OK(z_fifo_channel_sample_new(&closure, &handler, 10));
    ASSERT_OK(ze_declare_advanced_subscriber(z_loan(s2), &sub, z_loan(k), z_move(closure), &sub_opts));
    z_sleep_ms(TEST_SLEEP_MS);

    tcp_proxy_set_enabled(tcp_proxy, false);
    z_sleep_ms(TEST_SLEEP_MS);

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    ze_advanced_publisher_cache_options_default(&pub_opts.cache);
    pub_opts.cache.max_samples = 10;
    pub_opts.publisher_detection = true;
    ASSERT_OK(ze_declare_advanced_publisher(z_loan(s1), &pub, z_loan(k), &pub_opts));

    put_str(z_loan(pub), "1");
    put_str(z_loan(pub), "2");
    put_str(z_loan(pub), "3");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_empty(z_loan(handler));

    tcp_proxy_set_enabled(tcp_proxy, true);
    z_sleep_ms(TEST_RECONNECT_MS);

    put_str(z_loan(pub), "4");
    z_sleep_ms(TEST_SLEEP_MS);

    expect_next(z_loan(handler), "1");
    expect_next(z_loan(handler), "2");
    expect_next(z_loan(handler), "3");
    expect_next(z_loan(handler), "4");
    expect_empty(z_loan(handler));

    z_drop(z_move(sub));
    z_drop(z_move(pub));
    z_drop(z_move(handler));

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s2)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s1)));
    ASSERT_OK(zp_stop_periodic_scheduler_task(z_loan_mut(s2)));

    z_drop(z_move(s1));
    z_drop(z_move(s2));

    tcp_proxy_stop(tcp_proxy);
    tcp_proxy_destroy(tcp_proxy);
}*/

#endif  // Z_ADVANCED_PUBSUB_TEST_USE_TCP_PROXY

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_advanced_history(false);
#if defined(ZENOH_LINUX)
    test_advanced_history(true);
#endif
#ifdef Z_ADVANCED_PUBSUB_TEST_USE_TCP_PROXY
    test_advanced_retransmission();
    test_advanced_retransmission_periodic();
    test_advanced_retransmission_heartbeat();
    test_advanced_sample_miss();
    test_advanced_retransmission_sample_miss();
    // test_advanced_late_joiner();
#endif

    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf(
        "Missing config token to build this test. This test requires: Z_FEATURE_ADVANCED_PUBLICATION "
        "and Z_FEATURE_ADVANCED_SUBSCRIPTION\n");
    return 0;
}

#endif
