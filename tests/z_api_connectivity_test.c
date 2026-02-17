//
// Copyright (c) 2026 ZettaScale Technology
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

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "zenoh-pico.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#if defined(Z_FEATURE_UNSTABLE_API) && Z_FEATURE_CONNECTIVITY == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1 && \
    Z_FEATURE_LINK_TCP == 1 && Z_FEATURE_MULTI_THREAD == 1

#undef NDEBUG
#include <assert.h>

#define assert_ok(x)                            \
    {                                           \
        int ret = (int)x;                       \
        if (ret != Z_OK) {                      \
            printf("%s failed: %d\n", #x, ret); \
            assert(false);                      \
        }                                       \
    }

typedef struct events_ctx_t {
    atomic_uint transport_put;
    atomic_uint transport_delete;
    atomic_uint link_put;
    atomic_uint link_delete;
} events_ctx_t;

typedef struct transport_events_ctx_t {
    atomic_uint put;
    atomic_uint delete_;
    atomic_bool put_zid_set;
    atomic_bool delete_zid_set;
    atomic_bool put_snapshot_set;
    z_id_t put_zid;
    z_id_t delete_zid;
    z_whatami_t put_whatami;
    bool put_is_qos;
    bool put_is_multicast;
    bool put_is_shm;
} transport_events_ctx_t;

typedef struct link_events_ctx_t {
    atomic_uint put;
    atomic_uint delete_;
    atomic_bool put_zid_set;
    atomic_bool delete_zid_set;
    atomic_bool put_snapshot_set;
    z_id_t put_zid;
    z_id_t delete_zid;
    bool put_src_non_empty;
    bool put_dst_non_empty;
    uint16_t put_mtu;
    bool put_is_streamed;
    bool put_is_reliable;
} link_events_ctx_t;

typedef struct transport_capture_t {
    z_owned_transport_t transport;
    unsigned count;
} transport_capture_t;

typedef struct transport_info_capture_t {
    unsigned count;
    z_id_t first_zid;
    bool has_first;
} transport_info_capture_t;

typedef struct link_info_capture_t {
    unsigned count;
    z_id_t first_zid;
    bool has_first;
    bool first_src_non_empty;
    bool first_dst_non_empty;
} link_info_capture_t;

static void events_ctx_reset(events_ctx_t *ctx) {
    atomic_store_explicit(&ctx->transport_put, 0, memory_order_relaxed);
    atomic_store_explicit(&ctx->transport_delete, 0, memory_order_relaxed);
    atomic_store_explicit(&ctx->link_put, 0, memory_order_relaxed);
    atomic_store_explicit(&ctx->link_delete, 0, memory_order_relaxed);
}

static void transport_events_ctx_reset(transport_events_ctx_t *ctx) {
    atomic_store_explicit(&ctx->put, 0, memory_order_relaxed);
    atomic_store_explicit(&ctx->delete_, 0, memory_order_relaxed);
    atomic_store_explicit(&ctx->put_zid_set, false, memory_order_relaxed);
    atomic_store_explicit(&ctx->delete_zid_set, false, memory_order_relaxed);
    atomic_store_explicit(&ctx->put_snapshot_set, false, memory_order_relaxed);
    ctx->put_whatami = Z_WHATAMI_CLIENT;
    ctx->put_is_qos = false;
    ctx->put_is_multicast = false;
    ctx->put_is_shm = false;
}

static void link_events_ctx_reset(link_events_ctx_t *ctx) {
    atomic_store_explicit(&ctx->put, 0, memory_order_relaxed);
    atomic_store_explicit(&ctx->delete_, 0, memory_order_relaxed);
    atomic_store_explicit(&ctx->put_zid_set, false, memory_order_relaxed);
    atomic_store_explicit(&ctx->delete_zid_set, false, memory_order_relaxed);
    atomic_store_explicit(&ctx->put_snapshot_set, false, memory_order_relaxed);
    ctx->put_src_non_empty = false;
    ctx->put_dst_non_empty = false;
    ctx->put_mtu = 0;
    ctx->put_is_streamed = false;
    ctx->put_is_reliable = false;
}

static bool wait_counter_at_least(atomic_uint *counter, unsigned expected) {
    for (unsigned i = 0; i < 50; ++i) {
        if (atomic_load_explicit(counter, memory_order_acquire) >= expected) {
            return true;
        }
        z_sleep_ms(100);
    }
    return false;
}

static void open_listener_session(z_owned_session_t *session, const char *listen_locator) {
    z_owned_config_t cfg;
    z_config_default(&cfg);
    assert_ok(zp_config_insert(z_config_loan_mut(&cfg), Z_CONFIG_MODE_KEY, "peer"));
    assert_ok(zp_config_insert(z_config_loan_mut(&cfg), Z_CONFIG_LISTEN_KEY, listen_locator));
    assert_ok(z_open(session, z_config_move(&cfg), NULL));
    assert_ok(zp_start_read_task(z_session_loan_mut(session), NULL));
    assert_ok(zp_start_lease_task(z_session_loan_mut(session), NULL));
}

static void open_connector_session(z_owned_session_t *session, const char *connect_locator) {
    z_owned_config_t cfg;
    z_config_default(&cfg);
    assert_ok(zp_config_insert(z_config_loan_mut(&cfg), Z_CONFIG_MODE_KEY, "peer"));
    assert_ok(zp_config_insert(z_config_loan_mut(&cfg), Z_CONFIG_CONNECT_KEY, connect_locator));
    assert_ok(z_open(session, z_config_move(&cfg), NULL));
    assert_ok(zp_start_read_task(z_session_loan_mut(session), NULL));
    assert_ok(zp_start_lease_task(z_session_loan_mut(session), NULL));
}

static void close_session_with_tasks(z_owned_session_t *session) {
    assert_ok(zp_stop_read_task(z_session_loan_mut(session)));
    assert_ok(zp_stop_lease_task(z_session_loan_mut(session)));
    z_session_drop(z_session_move(session));
}

static void on_transport_event(z_loaned_transport_event_t *event, void *arg) {
    transport_events_ctx_t *ctx = (transport_events_ctx_t *)arg;
    const z_loaned_transport_t *transport = z_transport_event_transport(event);
    z_id_t zid = z_transport_zid(transport);
    switch (z_transport_event_kind(event)) {
        case Z_SAMPLE_KIND_PUT:
            if (!atomic_load_explicit(&ctx->put_zid_set, memory_order_relaxed)) {
                ctx->put_zid = zid;
                atomic_store_explicit(&ctx->put_zid_set, true, memory_order_release);
            }
            if (!atomic_load_explicit(&ctx->put_snapshot_set, memory_order_relaxed)) {
                ctx->put_whatami = z_transport_whatami(transport);
                ctx->put_is_qos = z_transport_is_qos(transport);
                ctx->put_is_multicast = z_transport_is_multicast(transport);
                ctx->put_is_shm = z_transport_is_shm(transport);
                atomic_store_explicit(&ctx->put_snapshot_set, true, memory_order_release);
            }
            atomic_fetch_add_explicit(&ctx->put, 1, memory_order_release);
            break;
        case Z_SAMPLE_KIND_DELETE:
            if (!atomic_load_explicit(&ctx->delete_zid_set, memory_order_relaxed)) {
                ctx->delete_zid = zid;
                atomic_store_explicit(&ctx->delete_zid_set, true, memory_order_release);
            }
            atomic_fetch_add_explicit(&ctx->delete_, 1, memory_order_release);
            break;
        default:
            break;
    }
}

static void on_link_event(z_loaned_link_event_t *event, void *arg) {
    link_events_ctx_t *ctx = (link_events_ctx_t *)arg;
    const z_loaned_link_t *link = z_link_event_link(event);
    z_id_t zid = z_link_zid(link);
    switch (z_link_event_kind(event)) {
        case Z_SAMPLE_KIND_PUT:
            if (!atomic_load_explicit(&ctx->put_zid_set, memory_order_relaxed)) {
                ctx->put_zid = zid;
                atomic_store_explicit(&ctx->put_zid_set, true, memory_order_release);
            }
            if (!atomic_load_explicit(&ctx->put_snapshot_set, memory_order_relaxed)) {
                z_owned_string_t src;
                z_owned_string_t dst;
                z_internal_string_null(&src);
                z_internal_string_null(&dst);
                assert_ok(z_link_src(link, &src));
                assert_ok(z_link_dst(link, &dst));
                ctx->put_src_non_empty = z_string_len(z_loan(src)) > 0;
                ctx->put_dst_non_empty = z_string_len(z_loan(dst)) > 0;
                z_string_drop(z_string_move(&src));
                z_string_drop(z_string_move(&dst));
                ctx->put_mtu = z_link_mtu(link);
                ctx->put_is_streamed = z_link_is_streamed(link);
                ctx->put_is_reliable = z_link_is_reliable(link);
                atomic_store_explicit(&ctx->put_snapshot_set, true, memory_order_release);
            }
            atomic_fetch_add_explicit(&ctx->put, 1, memory_order_release);
            break;
        case Z_SAMPLE_KIND_DELETE:
            if (!atomic_load_explicit(&ctx->delete_zid_set, memory_order_relaxed)) {
                ctx->delete_zid = zid;
                atomic_store_explicit(&ctx->delete_zid_set, true, memory_order_release);
            }
            atomic_fetch_add_explicit(&ctx->delete_, 1, memory_order_release);
            break;
        default:
            break;
    }
}

static void on_link_event_count_only(z_loaned_link_event_t *event, void *arg) {
    events_ctx_t *ctx = (events_ctx_t *)arg;
    switch (z_link_event_kind(event)) {
        case Z_SAMPLE_KIND_PUT:
            atomic_fetch_add_explicit(&ctx->link_put, 1, memory_order_relaxed);
            break;
        case Z_SAMPLE_KIND_DELETE:
            atomic_fetch_add_explicit(&ctx->link_delete, 1, memory_order_relaxed);
            break;
        default:
            break;
    }
}

static void capture_transport(z_loaned_transport_t *transport, void *arg) {
    transport_capture_t *ctx = (transport_capture_t *)arg;
    ctx->count++;
    if (ctx->count == 1) {
        assert_ok(z_transport_clone(&ctx->transport, transport));
    }
}

static void capture_transport_info(z_loaned_transport_t *transport, void *arg) {
    transport_info_capture_t *ctx = (transport_info_capture_t *)arg;
    ctx->count++;
    if (!ctx->has_first) {
        ctx->first_zid = z_transport_zid(transport);
        ctx->has_first = true;
    }
}

static void capture_link_info(z_loaned_link_t *link, void *arg) {
    link_info_capture_t *ctx = (link_info_capture_t *)arg;
    ctx->count++;
    if (!ctx->has_first) {
        z_owned_string_t src;
        z_owned_string_t dst;
        z_internal_string_null(&src);
        z_internal_string_null(&dst);
        assert_ok(z_link_src(link, &src));
        assert_ok(z_link_dst(link, &dst));

        ctx->first_zid = z_link_zid(link);
        ctx->has_first = true;
        ctx->first_src_non_empty = z_string_len(z_loan(src)) > 0;
        ctx->first_dst_non_empty = z_string_len(z_loan(dst)) > 0;

        z_string_drop(z_string_move(&src));
        z_string_drop(z_string_move(&dst));
    }
}

static unsigned capture_single_transport(const z_loaned_session_t *session, z_owned_transport_t *out) {
    transport_capture_t ctx;
    z_internal_transport_null(&ctx.transport);
    ctx.count = 0;

    z_owned_closure_transport_t callback;
    assert_ok(z_closure_transport(&callback, capture_transport, NULL, &ctx));
    assert_ok(z_info_transports(session, z_closure_transport_move(&callback)));

    z_internal_transport_null(out);
    if (ctx.count > 0) {
        z_transport_take(out, z_transport_move(&ctx.transport));
    }
    return ctx.count;
}

static void test_info_transports_and_links(void) {
    printf("test_info_transports_and_links\n");

    z_owned_session_t s1, s2;
    open_listener_session(&s1, "tcp/127.0.0.1:7452");
    open_connector_session(&s2, "tcp/127.0.0.1:7452");
    z_sleep_s(1);

    z_id_t s2_zid = z_info_zid(z_session_loan(&s2));

    transport_info_capture_t transport_ctx = {0};
    z_owned_closure_transport_t transport_callback;
    assert_ok(z_closure_transport(&transport_callback, capture_transport_info, NULL, &transport_ctx));
    assert_ok(z_info_transports(z_session_loan(&s1), z_closure_transport_move(&transport_callback)));
    assert(transport_ctx.count == 1);
    assert(transport_ctx.has_first);
    assert(memcmp(&transport_ctx.first_zid, &s2_zid, sizeof(z_id_t)) == 0);

    link_info_capture_t link_ctx = {0};
    z_owned_closure_link_t link_callback;
    assert_ok(z_closure_link(&link_callback, capture_link_info, NULL, &link_ctx));
    assert_ok(z_info_links(z_session_loan(&s1), z_closure_link_move(&link_callback), NULL));
    assert(link_ctx.count == 1);
    assert(link_ctx.has_first);
    assert(memcmp(&link_ctx.first_zid, &s2_zid, sizeof(z_id_t)) == 0);
    assert(link_ctx.first_src_non_empty);
    assert(link_ctx.first_dst_non_empty);

    close_session_with_tasks(&s2);
    close_session_with_tasks(&s1);
}

static void test_info_links_filtered(void) {
    printf("test_info_links_filtered\n");

    z_owned_session_t s1, s2;
    open_listener_session(&s1, "tcp/127.0.0.1:7453");
    open_connector_session(&s2, "tcp/127.0.0.1:7453");
    z_sleep_s(1);

    z_id_t s2_zid = z_info_zid(z_session_loan(&s2));

    z_owned_transport_t s1_transport;
    z_owned_transport_t s2_transport;
    z_internal_transport_null(&s1_transport);
    z_internal_transport_null(&s2_transport);
    assert(capture_single_transport(z_session_loan(&s1), &s1_transport) == 1);
    assert(capture_single_transport(z_session_loan(&s2), &s2_transport) == 1);
    assert(z_internal_transport_check(&s1_transport));
    assert(z_internal_transport_check(&s2_transport));

    link_info_capture_t match_ctx = {0};
    z_owned_closure_link_t match_callback;
    assert_ok(z_closure_link(&match_callback, capture_link_info, NULL, &match_ctx));
    z_info_links_options_t match_opts;
    z_info_links_options_default(&match_opts);
    match_opts.transport = z_transport_move(&s1_transport);
    assert_ok(z_info_links(z_session_loan(&s1), z_closure_link_move(&match_callback), &match_opts));
    assert(match_ctx.count == 1);
    assert(match_ctx.has_first);
    assert(memcmp(&match_ctx.first_zid, &s2_zid, sizeof(z_id_t)) == 0);
    assert(match_ctx.first_src_non_empty);
    assert(match_ctx.first_dst_non_empty);

    link_info_capture_t miss_ctx = {0};
    z_owned_closure_link_t miss_callback;
    assert_ok(z_closure_link(&miss_callback, capture_link_info, NULL, &miss_ctx));
    z_info_links_options_t miss_opts;
    z_info_links_options_default(&miss_opts);
    miss_opts.transport = z_transport_move(&s2_transport);
    assert_ok(z_info_links(z_session_loan(&s1), z_closure_link_move(&miss_callback), &miss_opts));
    assert(miss_ctx.count == 0);
    assert(!miss_ctx.has_first);

    z_transport_drop(z_transport_move(&s1_transport));
    z_transport_drop(z_transport_move(&s2_transport));

    close_session_with_tasks(&s2);
    close_session_with_tasks(&s1);
}

static void test_info_transports_and_links_empty(void) {
    printf("test_info_transports_and_links_empty\n");

    z_owned_session_t s1;
    open_listener_session(&s1, "tcp/127.0.0.1:7460");
    z_sleep_ms(250);

    transport_info_capture_t transport_ctx = {0};
    z_owned_closure_transport_t transport_callback;
    assert_ok(z_closure_transport(&transport_callback, capture_transport_info, NULL, &transport_ctx));
    assert_ok(z_info_transports(z_session_loan(&s1), z_closure_transport_move(&transport_callback)));
    assert(transport_ctx.count == 0);
    assert(!transport_ctx.has_first);

    link_info_capture_t link_ctx = {0};
    z_owned_closure_link_t link_callback;
    assert_ok(z_closure_link(&link_callback, capture_link_info, NULL, &link_ctx));
    assert_ok(z_info_links(z_session_loan(&s1), z_closure_link_move(&link_callback), NULL));
    assert(link_ctx.count == 0);
    assert(!link_ctx.has_first);

    close_session_with_tasks(&s1);
}

static void run_transport_events_test(const char *locator, bool history, bool background) {
    z_owned_session_t s1, s2;
    transport_events_ctx_t ctx;
    transport_events_ctx_reset(&ctx);

    open_listener_session(&s1, locator);
    if (history) {
        open_connector_session(&s2, locator);
        z_sleep_s(1);
    }

    z_owned_closure_transport_event_t callback;
    assert_ok(z_closure_transport_event(&callback, on_transport_event, NULL, &ctx));

    z_transport_events_listener_options_t opts;
    z_transport_events_listener_options_default(&opts);
    opts.history = history;

    z_owned_transport_events_listener_t listener;
    if (background) {
        assert_ok(z_declare_background_transport_events_listener(z_session_loan(&s1),
                                                                 z_closure_transport_event_move(&callback), &opts));
    } else {
        assert_ok(z_declare_transport_events_listener(z_session_loan(&s1), &listener,
                                                      z_closure_transport_event_move(&callback), &opts));
    }

    if (!history) {
        assert(atomic_load_explicit(&ctx.put, memory_order_acquire) == 0);
        open_connector_session(&s2, locator);
    }

    z_id_t s2_zid = z_info_zid(z_session_loan(&s2));
    assert(wait_counter_at_least(&ctx.put, 1));
    z_sleep_ms(250);
    assert(atomic_load_explicit(&ctx.put, memory_order_acquire) == 1);
    assert(atomic_load_explicit(&ctx.put_zid_set, memory_order_acquire));
    assert(atomic_load_explicit(&ctx.put_snapshot_set, memory_order_acquire));
    assert(memcmp(&ctx.put_zid, &s2_zid, sizeof(z_id_t)) == 0);
    assert(ctx.put_whatami == Z_WHATAMI_PEER);
    assert(!ctx.put_is_qos);
    assert(!ctx.put_is_multicast);
    assert(!ctx.put_is_shm);

    close_session_with_tasks(&s2);

    assert(wait_counter_at_least(&ctx.delete_, 1));
    z_sleep_ms(250);
    assert(atomic_load_explicit(&ctx.delete_, memory_order_acquire) == 1);
    assert(atomic_load_explicit(&ctx.delete_zid_set, memory_order_acquire));
    assert(memcmp(&ctx.delete_zid, &s2_zid, sizeof(z_id_t)) == 0);

    if (!background) {
        assert_ok(z_undeclare_transport_events_listener(z_transport_events_listener_move(&listener)));
    }
    close_session_with_tasks(&s1);
}

static void run_link_events_test(const char *locator, bool history, bool background) {
    z_owned_session_t s1, s2;
    link_events_ctx_t ctx;
    link_events_ctx_reset(&ctx);

    open_listener_session(&s1, locator);
    if (history) {
        open_connector_session(&s2, locator);
        z_sleep_s(1);
    }

    z_owned_closure_link_event_t callback;
    assert_ok(z_closure_link_event(&callback, on_link_event, NULL, &ctx));

    z_link_events_listener_options_t opts;
    z_link_events_listener_options_default(&opts);
    opts.history = history;

    z_owned_link_events_listener_t listener;
    if (background) {
        assert_ok(z_declare_background_link_events_listener(z_session_loan(&s1), z_closure_link_event_move(&callback),
                                                            &opts));
    } else {
        assert_ok(z_declare_link_events_listener(z_session_loan(&s1), &listener, z_closure_link_event_move(&callback),
                                                 &opts));
    }

    if (!history) {
        assert(atomic_load_explicit(&ctx.put, memory_order_acquire) == 0);
        open_connector_session(&s2, locator);
    }

    z_id_t s2_zid = z_info_zid(z_session_loan(&s2));
    assert(wait_counter_at_least(&ctx.put, 1));
    z_sleep_ms(250);
    assert(atomic_load_explicit(&ctx.put, memory_order_acquire) == 1);
    assert(atomic_load_explicit(&ctx.put_zid_set, memory_order_acquire));
    assert(atomic_load_explicit(&ctx.put_snapshot_set, memory_order_acquire));
    assert(memcmp(&ctx.put_zid, &s2_zid, sizeof(z_id_t)) == 0);
    assert(ctx.put_src_non_empty);
    assert(ctx.put_dst_non_empty);
    assert(ctx.put_mtu > 0);
    assert(ctx.put_is_streamed);
    assert(ctx.put_is_reliable);

    close_session_with_tasks(&s2);

    assert(wait_counter_at_least(&ctx.delete_, 1));
    z_sleep_ms(250);
    assert(atomic_load_explicit(&ctx.delete_, memory_order_acquire) == 1);
    assert(atomic_load_explicit(&ctx.delete_zid_set, memory_order_acquire));
    assert(memcmp(&ctx.delete_zid, &s2_zid, sizeof(z_id_t)) == 0);

    if (!background) {
        assert_ok(z_undeclare_link_events_listener(z_link_events_listener_move(&listener)));
    }
    close_session_with_tasks(&s1);
}

static void test_transport_events_no_history(void) {
    printf("test_transport_events_no_history\n");
    run_transport_events_test("tcp/127.0.0.1:7448", false, false);
}

static void test_transport_events_history(void) {
    printf("test_transport_events_history\n");
    run_transport_events_test("tcp/127.0.0.1:7449", true, false);
}

static void test_transport_events_background(void) {
    printf("test_transport_events_background\n");
    run_transport_events_test("tcp/127.0.0.1:7450", false, true);
}

static void test_transport_events_background_history(void) {
    printf("test_transport_events_background_history\n");
    run_transport_events_test("tcp/127.0.0.1:7461", true, true);
}

static void test_link_events_no_history(void) {
    printf("test_link_events_no_history\n");
    run_link_events_test("tcp/127.0.0.1:7451", false, false);
}

static void test_link_events_history(void) {
    printf("test_link_events_history\n");
    run_link_events_test("tcp/127.0.0.1:7454", true, false);
}

static void test_link_events_background(void) {
    printf("test_link_events_background\n");
    run_link_events_test("tcp/127.0.0.1:7455", false, true);
}

static void test_link_events_background_history(void) {
    printf("test_link_events_background_history\n");
    run_link_events_test("tcp/127.0.0.1:7462", true, true);
}

static void test_transport_events_undeclare(void) {
    printf("test_transport_events_undeclare\n");

    z_owned_session_t s1, s2, s3;
    transport_events_ctx_t ctx;
    transport_events_ctx_reset(&ctx);

    open_listener_session(&s1, "tcp/127.0.0.1:7463");
    z_owned_closure_transport_event_t callback;
    assert_ok(z_closure_transport_event(&callback, on_transport_event, NULL, &ctx));
    z_owned_transport_events_listener_t listener;
    assert_ok(z_declare_transport_events_listener(z_session_loan(&s1), &listener,
                                                  z_closure_transport_event_move(&callback), NULL));

    open_connector_session(&s2, "tcp/127.0.0.1:7463");
    assert(wait_counter_at_least(&ctx.put, 1));
    z_sleep_ms(250);
    assert(atomic_load_explicit(&ctx.put, memory_order_acquire) == 1);

    assert_ok(z_undeclare_transport_events_listener(z_transport_events_listener_move(&listener)));

    close_session_with_tasks(&s2);
    z_sleep_ms(500);
    assert(atomic_load_explicit(&ctx.delete_, memory_order_acquire) == 0);

    open_connector_session(&s3, "tcp/127.0.0.1:7463");
    z_sleep_ms(500);
    assert(atomic_load_explicit(&ctx.put, memory_order_acquire) == 1);
    assert(atomic_load_explicit(&ctx.delete_, memory_order_acquire) == 0);

    close_session_with_tasks(&s3);
    close_session_with_tasks(&s1);
}

static void test_link_events_undeclare(void) {
    printf("test_link_events_undeclare\n");

    z_owned_session_t s1, s2, s3;
    link_events_ctx_t ctx;
    link_events_ctx_reset(&ctx);

    open_listener_session(&s1, "tcp/127.0.0.1:7464");
    z_owned_closure_link_event_t callback;
    assert_ok(z_closure_link_event(&callback, on_link_event, NULL, &ctx));
    z_owned_link_events_listener_t listener;
    assert_ok(
        z_declare_link_events_listener(z_session_loan(&s1), &listener, z_closure_link_event_move(&callback), NULL));

    open_connector_session(&s2, "tcp/127.0.0.1:7464");
    assert(wait_counter_at_least(&ctx.put, 1));
    z_sleep_ms(250);
    assert(atomic_load_explicit(&ctx.put, memory_order_acquire) == 1);

    assert_ok(z_undeclare_link_events_listener(z_link_events_listener_move(&listener)));

    close_session_with_tasks(&s2);
    z_sleep_ms(500);
    assert(atomic_load_explicit(&ctx.delete_, memory_order_acquire) == 0);

    open_connector_session(&s3, "tcp/127.0.0.1:7464");
    z_sleep_ms(500);
    assert(atomic_load_explicit(&ctx.put, memory_order_acquire) == 1);
    assert(atomic_load_explicit(&ctx.delete_, memory_order_acquire) == 0);

    close_session_with_tasks(&s3);
    close_session_with_tasks(&s1);
}

static void test_link_events_filtered(void) {
    printf("test_link_events_filtered\n");

    z_owned_session_t s1, s2;
    open_listener_session(&s1, "tcp/127.0.0.1:7456");
    open_connector_session(&s2, "tcp/127.0.0.1:7456");
    z_sleep_s(1);

    z_owned_transport_t s1_transport;
    z_owned_transport_t s2_transport;
    z_internal_transport_null(&s1_transport);
    z_internal_transport_null(&s2_transport);

    assert(capture_single_transport(z_session_loan(&s1), &s1_transport) == 1);
    assert(capture_single_transport(z_session_loan(&s2), &s2_transport) == 1);
    assert(z_internal_transport_check(&s1_transport));
    assert(z_internal_transport_check(&s2_transport));

    events_ctx_t match_ctx;
    events_ctx_reset(&match_ctx);

    z_owned_closure_link_event_t match_callback;
    assert_ok(z_closure_link_event(&match_callback, on_link_event_count_only, NULL, &match_ctx));

    z_link_events_listener_options_t match_opts;
    z_link_events_listener_options_default(&match_opts);
    match_opts.history = true;
    match_opts.transport = z_transport_move(&s1_transport);

    z_owned_link_events_listener_t match_listener;
    assert_ok(z_declare_link_events_listener(z_session_loan(&s1), &match_listener,
                                             z_closure_link_event_move(&match_callback), &match_opts));

    assert(wait_counter_at_least(&match_ctx.link_put, 1));
    z_sleep_ms(250);
    assert(atomic_load_explicit(&match_ctx.link_put, memory_order_acquire) == 1);
    assert_ok(z_undeclare_link_events_listener(z_link_events_listener_move(&match_listener)));

    events_ctx_t miss_ctx;
    events_ctx_reset(&miss_ctx);

    z_owned_closure_link_event_t miss_callback;
    assert_ok(z_closure_link_event(&miss_callback, on_link_event_count_only, NULL, &miss_ctx));

    z_link_events_listener_options_t miss_opts;
    z_link_events_listener_options_default(&miss_opts);
    miss_opts.history = true;
    miss_opts.transport = z_transport_move(&s2_transport);

    z_owned_link_events_listener_t miss_listener;
    assert_ok(z_declare_link_events_listener(z_session_loan(&s1), &miss_listener,
                                             z_closure_link_event_move(&miss_callback), &miss_opts));

    z_sleep_ms(500);
    assert(atomic_load_explicit(&miss_ctx.link_put, memory_order_acquire) == 0);
    assert(atomic_load_explicit(&miss_ctx.link_delete, memory_order_acquire) == 0);
    assert_ok(z_undeclare_link_events_listener(z_link_events_listener_move(&miss_listener)));

    z_transport_drop(z_transport_move(&s1_transport));
    z_transport_drop(z_transport_move(&s2_transport));

    close_session_with_tasks(&s2);
    close_session_with_tasks(&s1);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    test_info_transports_and_links();
    test_info_links_filtered();
    test_info_transports_and_links_empty();
    test_transport_events_no_history();
    test_transport_events_history();
    test_transport_events_background();
    test_transport_events_background_history();
    test_transport_events_undeclare();
    test_link_events_no_history();
    test_link_events_history();
    test_link_events_background();
    test_link_events_background_history();
    test_link_events_undeclare();
    test_link_events_filtered();
    return 0;
}

#else
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}
#endif
