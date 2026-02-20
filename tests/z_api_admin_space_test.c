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
#include <stdio.h>
#include <string.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"
#include "zenoh-pico/utils/string.h"

#if Z_FEATURE_ADMIN_SPACE == 1

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
        ASSERT_OK(zp_start_admin_space(z_loan_mut(s)));
        ASSERT_TRUE(_Z_RC_IN_VAL(z_loan(s))->_admin_space_queryable_id > 0);
        ASSERT_OK(zp_stop_admin_space(z_loan_mut(s)));
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

typedef struct admin_space_query_reply_t {
    z_owned_keyexpr_t ke;
    z_owned_string_t payload;
    z_owned_encoding_t encoding;

} admin_space_query_reply_t;

void admin_space_query_reply_clear(admin_space_query_reply_t *reply) {
    z_drop(z_move(reply->ke));
    z_drop(z_move(reply->payload));
    z_drop(z_move(reply->encoding));
}

bool admin_space_query_reply_eq(const admin_space_query_reply_t *left, const admin_space_query_reply_t *right) {
    return z_keyexpr_equals(z_loan(left->ke), z_loan(right->ke));
}

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
_Z_ELEM_DEFINE(admin_space_query_reply, admin_space_query_reply_t, _z_noop_size, admin_space_query_reply_clear,
               _z_noop_copy, _z_noop_move, admin_space_query_reply_eq, _z_noop_cmp, _z_noop_hash)
_Z_LIST_DEFINE(admin_space_query_reply, admin_space_query_reply_t)

static admin_space_query_reply_list_t *run_admin_space_query(const z_loaned_session_t *zs,
                                                             const z_loaned_keyexpr_t *query_ke) {
    admin_space_query_reply_list_t *results = admin_space_query_reply_list_new();

    z_owned_closure_reply_t closure;
    z_owned_fifo_handler_reply_t handler;
    ASSERT_OK(z_fifo_channel_reply_new(&closure, &handler, 10));

    z_get_options_t opts;
    z_get_options_default(&opts);
    opts.timeout_ms = 1000;
    ASSERT_OK(z_get(zs, query_ke, "", z_move(closure), &opts));

    // Wait for replies to arrive
    z_time_t start = z_time_now();
    for (unsigned long elapsed = z_time_elapsed_ms(&start);; elapsed = z_time_elapsed_ms(&start)) {
        /* Drain everything currently available */
        z_owned_reply_t reply;
        z_result_t res;
        for (res = z_try_recv(z_loan(handler), &reply); res == _Z_RES_OK; res = z_try_recv(z_loan(handler), &reply)) {
            if (z_reply_is_ok(z_loan(reply))) {
                admin_space_query_reply_t *result = z_malloc(sizeof(*result));
                ASSERT_NOT_NULL(result);

                const z_loaned_sample_t *sample = z_reply_ok(z_loan(reply));
                ASSERT_OK(z_bytes_to_string(z_sample_payload(sample), &result->payload));
                ASSERT_OK(z_keyexpr_clone(&result->ke, z_sample_keyexpr(sample)));
                const z_loaned_encoding_t *encoding = z_sample_encoding(sample);
                ASSERT_OK(z_encoding_clone(&result->encoding, encoding));

                admin_space_query_reply_list_t *old = results;
                admin_space_query_reply_list_t *tmp = admin_space_query_reply_list_push(old, result);
                ASSERT_TRUE(tmp != old);
                results = tmp;
            }

            z_drop(z_move(reply));
        }

        // If channel is closed, we're done regardless of timeout
        if (res == Z_CHANNEL_DISCONNECTED) {
            break;
        }

        // If nothing available right now, only stop once timeout elapsed
        if (res == Z_CHANNEL_NODATA) {
            if (elapsed >= opts.timeout_ms) {
                break;
            }
            z_sleep_ms(1);
            continue;
        }

        // Anything else is unexpected
        ASSERT_OK(res);
    }
    z_drop(z_move(handler));

    return results;
}

static void assert_json_object(const z_loaned_string_t *s) {
    const char *p = z_string_data(s);
    size_t n = z_string_len(s);
    ASSERT_TRUE(n >= 2);
    ASSERT_TRUE(p[0] == '{');
    ASSERT_TRUE(p[n - 1] == '}');
}

static void assert_contains(const z_loaned_string_t *s, const char *needle) {
    const char *start = z_string_data(s);
    const char *end = start + z_string_len(s);
    if (_z_strstr(start, end, needle) == NULL) {
        fprintf(stderr, "Assertion failed: expected substring not found.\nActual: %.*s\nNeedle: %s\n",
                (int)(end - start), start, needle);
    }
    ASSERT_TRUE(_z_strstr(start, end, needle) != NULL);
}

static void assert_contains_z_string(const z_loaned_string_t *haystack, const z_loaned_string_t *needle) {
    const char *h_start = z_string_data(haystack);
    const char *h_end = h_start + z_string_len(haystack);

    size_t n = z_string_len(needle);
    ASSERT_TRUE(n < 256);

    char buf[256];
    // Flawfinder: ignore [CWE-120] - checked by assert above
    memcpy(buf, z_string_data(needle), n);
    buf[n] = '\0';

    ASSERT_TRUE(_z_strstr(h_start, h_end, buf) != NULL);
}

static void assert_contains_quoted_value(const z_loaned_string_t *s, const char *key, const char *val) {
    // e.g. key="mode" val="peer" -> "\"mode\":\"peer\""
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "\"%s\":\"%s\"", key, val);
    ASSERT_TRUE(n > 0 && (size_t)n < sizeof(buf));
    assert_contains(s, buf);
}

static const char *whatami_to_str(z_whatami_t w) {
    switch (w) {
        case Z_WHATAMI_ROUTER:
            return "router";
        case Z_WHATAMI_PEER:
            return "peer";
        case Z_WHATAMI_CLIENT:
            return "client";
        default:
            return NULL;
    }
}

static void verify_transport_reply_json(const z_loaned_string_t *payload, const z_id_t *expected_zid,
                                        z_whatami_t expected_whatami, bool is_multicast) {
    assert_json_object(payload);
    assert_contains(payload, "\"zid\"");
    assert_contains(payload, "\"whatami\"");
    assert_contains(payload, "\"is_qos\"");
    assert_contains(payload, "\"is_multicast\"");
    assert_contains(payload, "\"is_shm\"");

    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(expected_zid, &zid_str));
    assert_contains_z_string(payload, z_loan(zid_str));
    z_drop(z_move(zid_str));

    const char *ws = whatami_to_str(expected_whatami);
    ASSERT_NOT_NULL(ws);
    assert_contains_quoted_value(payload, "whatami", ws);
    assert_contains(payload, is_multicast ? "\"is_multicast\":true" : "\"is_multicast\":false");
}

static void verify_link_reply_json(const z_loaned_string_t *payload, const z_id_t *expected_zid) {
    assert_json_object(payload);
    assert_contains(payload, "\"zid\"");
    assert_contains(payload, "\"src\"");
    assert_contains(payload, "\"dst\"");
    assert_contains(payload, "\"mtu\"");
    assert_contains(payload, "\"is_streamed\"");
    assert_contains(payload, "\"is_reliable\"");

    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(expected_zid, &zid_str));
    assert_contains_z_string(payload, z_loan(zid_str));
    z_drop(z_move(zid_str));
}

static void append_zid_to_keyexpr(z_owned_keyexpr_t *out, const z_id_t *zid) {
    z_owned_string_t s;
    ASSERT_OK(z_id_to_string(zid, &s));
    ASSERT_OK(_z_keyexpr_append_substr(out, z_string_data(z_string_loan(&s)), z_string_len(z_string_loan(&s))));
    z_string_drop(z_string_move(&s));
}

static void build_transport_peer_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *base_ke, const char *transport,
                                    const z_id_t *peer_zid) {
    ASSERT_OK(z_keyexpr_clone(out, base_ke));
    ASSERT_OK(_z_keyexpr_append_str(out, transport));
    append_zid_to_keyexpr(out, peer_zid);
}

static void build_peer_link_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *base_ke, const char *transport,
                               const z_id_t *peer_zid, const z_id_t *link_id) {
    ASSERT_OK(z_keyexpr_clone(out, base_ke));
    ASSERT_OK(_z_keyexpr_append_str(out, transport));
    append_zid_to_keyexpr(out, peer_zid);
    ASSERT_OK(_z_keyexpr_append_str(out, _Z_KEYEXPR_LINK));
    append_zid_to_keyexpr(out, link_id);
}

static void verify_admin_space_query_unicast(const admin_space_query_reply_list_t *results,
                                             const z_loaned_keyexpr_t *base_ke, const _z_transport_unicast_t *unicast) {
    for (_z_transport_peer_unicast_slist_t *it = unicast->_peers; it != NULL;
         it = _z_transport_peer_unicast_slist_next(it)) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(it);

        admin_space_query_reply_t transport_expected;
        z_internal_keyexpr_null(&transport_expected.ke);
        z_internal_string_null(&transport_expected.payload);
        z_internal_encoding_null(&transport_expected.encoding);
        build_transport_peer_ke(&transport_expected.ke, base_ke, _Z_KEYEXPR_TRANSPORT_UNICAST,
                                &peer->common._remote_zid);

        admin_space_query_reply_list_t *entry =
            admin_space_query_reply_list_find(results, admin_space_query_reply_eq, &transport_expected);
        ASSERT_NOT_NULL(entry);
        const admin_space_query_reply_t *reply = admin_space_query_reply_list_value(entry);
        ASSERT_NOT_NULL(reply);
        admin_space_query_reply_elem_clear(&transport_expected);
        ASSERT_TRUE(z_encoding_equals(z_loan(reply->encoding), z_encoding_application_json()));
        verify_transport_reply_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                    peer->common._remote_whatami, false);

        admin_space_query_reply_t link_expected;
        z_internal_keyexpr_null(&link_expected.ke);
        z_internal_string_null(&link_expected.payload);
        z_internal_encoding_null(&link_expected.encoding);
        build_peer_link_ke(&link_expected.ke, base_ke, _Z_KEYEXPR_TRANSPORT_UNICAST, &peer->common._remote_zid,
                           &peer->common._remote_zid);

        entry = admin_space_query_reply_list_find(results, admin_space_query_reply_eq, &link_expected);
        ASSERT_NOT_NULL(entry);
        reply = admin_space_query_reply_list_value(entry);
        ASSERT_NOT_NULL(reply);
        admin_space_query_reply_elem_clear(&link_expected);
        ASSERT_TRUE(z_encoding_equals(z_loan(reply->encoding), z_encoding_application_json()));
        verify_link_reply_json(z_string_loan(&reply->payload), &peer->common._remote_zid);
    }
}

static void verify_admin_space_query_multicast(const z_loaned_session_t *zs,
                                               const admin_space_query_reply_list_t *results,
                                               const z_loaned_keyexpr_t *base_ke,
                                               const _z_transport_multicast_t *multicast) {
    const _z_session_t *session = _Z_RC_IN_VAL(zs);
    const char *transport =
        (session->_tp._type == _Z_TRANSPORT_RAWETH_TYPE) ? _Z_KEYEXPR_TRANSPORT_RAWETH : _Z_KEYEXPR_TRANSPORT_MULTICAST;

    for (_z_transport_peer_multicast_slist_t *it = multicast->_peers; it != NULL;
         it = _z_transport_peer_multicast_slist_next(it)) {
        _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(it);

        admin_space_query_reply_t transport_expected;
        z_internal_keyexpr_null(&transport_expected.ke);
        z_internal_string_null(&transport_expected.payload);
        z_internal_encoding_null(&transport_expected.encoding);
        build_transport_peer_ke(&transport_expected.ke, base_ke, transport, &peer->common._remote_zid);

        admin_space_query_reply_list_t *entry =
            admin_space_query_reply_list_find(results, admin_space_query_reply_eq, &transport_expected);
        ASSERT_NOT_NULL(entry);
        const admin_space_query_reply_t *reply = admin_space_query_reply_list_value(entry);
        ASSERT_NOT_NULL(reply);
        admin_space_query_reply_elem_clear(&transport_expected);
        ASSERT_TRUE(z_encoding_equals(z_loan(reply->encoding), z_encoding_application_json()));
        verify_transport_reply_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                    peer->common._remote_whatami, true);

        admin_space_query_reply_t link_expected;
        z_internal_keyexpr_null(&link_expected.ke);
        z_internal_string_null(&link_expected.payload);
        z_internal_encoding_null(&link_expected.encoding);
        build_peer_link_ke(&link_expected.ke, base_ke, transport, &peer->common._remote_zid, &peer->common._remote_zid);

        entry = admin_space_query_reply_list_find(results, admin_space_query_reply_eq, &link_expected);
        ASSERT_NOT_NULL(entry);
        reply = admin_space_query_reply_list_value(entry);
        ASSERT_NOT_NULL(reply);
        admin_space_query_reply_elem_clear(&link_expected);
        ASSERT_TRUE(z_encoding_equals(z_loan(reply->encoding), z_encoding_application_json()));
        verify_link_reply_json(z_string_loan(&reply->payload), &peer->common._remote_zid);
    }
}

static void verify_admin_space_query(const z_loaned_session_t *zs, const admin_space_query_reply_list_t *results,
                                     const z_loaned_keyexpr_t *base_ke) {
    _z_session_t *session = _Z_RC_IN_VAL(zs);

    size_t expected_replies = 0;
    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            expected_replies = 2 * _z_transport_peer_unicast_slist_len(session->_tp._transport._unicast._peers);
            verify_admin_space_query_unicast(results, base_ke, &session->_tp._transport._unicast);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            expected_replies = 2 * _z_transport_peer_multicast_slist_len(session->_tp._transport._multicast._peers);
            verify_admin_space_query_multicast(zs, results, base_ke, &session->_tp._transport._multicast);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            expected_replies = 2 * _z_transport_peer_multicast_slist_len(session->_tp._transport._raweth._peers);
            verify_admin_space_query_multicast(zs, results, base_ke, &session->_tp._transport._raweth);
            break;
        default:
            break;
    }
    ASSERT_TRUE(admin_space_query_reply_list_len(results) == expected_replies);
}

void test_admin_space_query_succeeds(void) {
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    ASSERT_OK(zp_start_admin_space(z_loan_mut(s1)));

    ASSERT_OK(z_open(&s2, z_move(c2), NULL));

    // Wait for sessions to connect
    z_sleep_ms(250);

    // Build base query keyexpr: @/<zid>/pico/session
    z_id_t zid = z_info_zid(z_loan(s1));
    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(&zid, &zid_str));

    z_owned_keyexpr_t base_ke;
    z_internal_keyexpr_null(&base_ke);

    ASSERT_OK(_z_keyexpr_append_str(&base_ke, _Z_KEYEXPR_AT));
    ASSERT_OK(_z_keyexpr_append_substr(&base_ke, z_string_data(z_loan(zid_str)), z_string_len(z_loan(zid_str))));
    z_drop(z_move(zid_str));
    ASSERT_OK(_z_keyexpr_append_str(&base_ke, _Z_KEYEXPR_PICO));
    ASSERT_OK(_z_keyexpr_append_str(&base_ke, _Z_KEYEXPR_SESSION));

    // Derive query keyexpr: @/<zid>/pico/session/**
    z_owned_keyexpr_t query_ke;
    z_keyexpr_clone(&query_ke, z_loan(base_ke));
    ASSERT_OK(_z_keyexpr_append_str(&query_ke, _Z_KEYEXPR_STARSTAR));

    admin_space_query_reply_list_t *results = run_admin_space_query(z_loan(s2), z_loan(query_ke));
    z_drop(z_move(query_ke));
    verify_admin_space_query(z_loan(s1), results, z_loan(base_ke));
    z_drop(z_move(base_ke));

    ASSERT_OK(zp_stop_admin_space(z_loan_mut(s1)));

    admin_space_query_reply_list_free(&results);
    z_drop(z_move(s1));
    z_drop(z_move(s2));
}

void test_admin_space_query_fails_when_not_running(void) {
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    ASSERT_OK(z_open(&s2, z_move(c2), NULL));

    // Wait for sessions to connect
    z_sleep_ms(250);

    // Build query keyexpr: @/<zid>/pico/session/**
    z_id_t zid = z_info_zid(z_loan(s1));
    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(&zid, &zid_str));

    z_owned_keyexpr_t ke;
    z_internal_keyexpr_null(&ke);

    ASSERT_OK(_z_keyexpr_append_str(&ke, _Z_KEYEXPR_AT));
    ASSERT_OK(_z_keyexpr_append_substr(&ke, z_string_data(z_loan(zid_str)), z_string_len(z_loan(zid_str))));
    z_drop(z_move(zid_str));
    ASSERT_OK(_z_keyexpr_append_str(&ke, _Z_KEYEXPR_PICO));
    ASSERT_OK(_z_keyexpr_append_str(&ke, _Z_KEYEXPR_SESSION));
    ASSERT_OK(_z_keyexpr_append_str(&ke, _Z_KEYEXPR_STARSTAR));

    // 1) Not running - expect no replies
    admin_space_query_reply_list_t *results = run_admin_space_query(z_loan(s2), z_keyexpr_loan(&ke));
    ASSERT_TRUE(admin_space_query_reply_list_len(results) == 0);
    admin_space_query_reply_list_free(&results);

    // 2) Start admin space - expect replies
    ASSERT_OK(zp_start_admin_space(z_loan_mut(s1)));
    z_sleep_ms(250);
    results = run_admin_space_query(z_loan(s2), z_keyexpr_loan(&ke));
    ASSERT_TRUE(admin_space_query_reply_list_len(results) > 0);
    admin_space_query_reply_list_free(&results);

    // 3) Stop admin space - expect no replies again
    ASSERT_OK(zp_stop_admin_space(z_loan_mut(s1)));
    results = run_admin_space_query(z_loan(s2), z_keyexpr_loan(&ke));
    ASSERT_TRUE(admin_space_query_reply_list_len(results) == 0);
    admin_space_query_reply_list_free(&results);

    z_keyexpr_drop(z_keyexpr_move(&ke));
    z_drop(z_move(s1));
    z_drop(z_move(s2));
}

#if Z_FEATURE_CONNECTIVITY == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_LINK_TCP == 1 && \
    Z_FEATURE_MULTI_THREAD == 1
static void open_listener_session(z_owned_session_t *session, const char *listen_locator) {
    z_owned_config_t cfg;
    z_config_default(&cfg);
    ASSERT_OK(zp_config_insert(z_config_loan_mut(&cfg), Z_CONFIG_MODE_KEY, "peer"));
    ASSERT_OK(zp_config_insert(z_config_loan_mut(&cfg), Z_CONFIG_LISTEN_KEY, listen_locator));
    ASSERT_OK(z_open(session, z_config_move(&cfg), NULL));
    ASSERT_OK(zp_start_read_task(z_session_loan_mut(session), NULL));
    ASSERT_OK(zp_start_lease_task(z_session_loan_mut(session), NULL));
}

static void open_connector_session(z_owned_session_t *session, const char *connect_locator) {
    z_owned_config_t cfg;
    z_config_default(&cfg);
    ASSERT_OK(zp_config_insert(z_config_loan_mut(&cfg), Z_CONFIG_MODE_KEY, "peer"));
    ASSERT_OK(zp_config_insert(z_config_loan_mut(&cfg), Z_CONFIG_CONNECT_KEY, connect_locator));
    ASSERT_OK(z_open(session, z_config_move(&cfg), NULL));
    ASSERT_OK(zp_start_read_task(z_session_loan_mut(session), NULL));
    ASSERT_OK(zp_start_lease_task(z_session_loan_mut(session), NULL));
}

static void close_session_with_tasks(z_owned_session_t *session) {
    ASSERT_OK(zp_stop_read_task(z_session_loan_mut(session)));
    ASSERT_OK(zp_stop_lease_task(z_session_loan_mut(session)));
    z_session_drop(z_session_move(session));
}

static void wait_for_sample_kind(z_owned_fifo_handler_sample_t *handler, z_sample_kind_t expected_kind,
                                 z_owned_string_t *payload_out) {
    if (payload_out != NULL) {
        z_internal_string_null(payload_out);
    }

    for (unsigned i = 0; i < 50; ++i) {
        z_owned_sample_t sample;
        z_result_t res = z_fifo_handler_sample_try_recv(z_fifo_handler_sample_loan(handler), &sample);
        if (res == Z_CHANNEL_NODATA) {
            z_sleep_ms(100);
            continue;
        }

        ASSERT_OK(res);
        if (z_sample_kind(z_loan(sample)) == expected_kind) {
            if (payload_out != NULL && expected_kind == Z_SAMPLE_KIND_PUT) {
                ASSERT_OK(z_bytes_to_string(z_sample_payload(z_loan(sample)), payload_out));
            }
            z_drop(z_move(sample));
            return;
        }
        z_drop(z_move(sample));
    }

    ASSERT_TRUE(false);
}

static void wait_admin_space_ready(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *probe_ke) {
    bool ready = false;
    for (unsigned i = 0; i < 50; ++i) {
        admin_space_query_reply_list_t *results = run_admin_space_query(zs, probe_ke);
        ready = admin_space_query_reply_list_len(results) > 0;
        admin_space_query_reply_list_free(&results);
        if (ready) {
            break;
        }
        z_sleep_ms(100);
    }
    ASSERT_TRUE(ready);
}

void test_admin_space_connectivity_events(void) {
    z_owned_session_t s1, s2, s3;
    open_listener_session(&s1, "tcp/127.0.0.1:7457");
    ASSERT_OK(zp_start_admin_space(z_session_loan_mut(&s1)));

    z_id_t s1_zid = z_info_zid(z_session_loan(&s1));
    z_owned_string_t s1_zid_str;
    ASSERT_OK(z_id_to_string(&s1_zid, &s1_zid_str));

    char transport_sub_ke_str[256];
    int transport_sub_ke_len =
        snprintf(transport_sub_ke_str, sizeof(transport_sub_ke_str), "@/%.*s/%s/%s/%s/*",
                 (int)z_string_len(z_string_loan(&s1_zid_str)), z_string_data(z_string_loan(&s1_zid_str)),
                 _Z_KEYEXPR_PICO, _Z_KEYEXPR_SESSION, _Z_KEYEXPR_TRANSPORT_UNICAST);
    ASSERT_TRUE(transport_sub_ke_len > 0 && (size_t)transport_sub_ke_len < sizeof(transport_sub_ke_str));

    char link_sub_ke_str[320];
    int link_sub_ke_len =
        snprintf(link_sub_ke_str, sizeof(link_sub_ke_str), "@/%.*s/%s/%s/%s/*/%s/*",
                 (int)z_string_len(z_string_loan(&s1_zid_str)), z_string_data(z_string_loan(&s1_zid_str)),
                 _Z_KEYEXPR_PICO, _Z_KEYEXPR_SESSION, _Z_KEYEXPR_TRANSPORT_UNICAST, _Z_KEYEXPR_LINK);
    ASSERT_TRUE(link_sub_ke_len > 0 && (size_t)link_sub_ke_len < sizeof(link_sub_ke_str));

    z_drop(z_move(s1_zid_str));

    z_view_keyexpr_t transport_sub_ke;
    z_view_keyexpr_t link_sub_ke;
    ASSERT_OK(z_view_keyexpr_from_str(&transport_sub_ke, transport_sub_ke_str));
    ASSERT_OK(z_view_keyexpr_from_str(&link_sub_ke, link_sub_ke_str));

    open_connector_session(&s2, "tcp/127.0.0.1:7457");

    z_owned_closure_sample_t transport_sub_closure;
    z_owned_fifo_handler_sample_t transport_sub_handler;
    ASSERT_OK(z_fifo_channel_sample_new(&transport_sub_closure, &transport_sub_handler, 8));
    z_owned_subscriber_t transport_sub;
    ASSERT_OK(z_declare_subscriber(z_session_loan(&s2), &transport_sub, z_view_keyexpr_loan(&transport_sub_ke),
                                   z_closure_sample_move(&transport_sub_closure), NULL));

    z_owned_closure_sample_t link_sub_closure;
    z_owned_fifo_handler_sample_t link_sub_handler;
    ASSERT_OK(z_fifo_channel_sample_new(&link_sub_closure, &link_sub_handler, 8));
    z_owned_subscriber_t link_sub;
    ASSERT_OK(z_declare_subscriber(z_session_loan(&s2), &link_sub, z_view_keyexpr_loan(&link_sub_ke),
                                   z_closure_sample_move(&link_sub_closure), NULL));
    wait_admin_space_ready(z_session_loan(&s2), z_view_keyexpr_loan(&transport_sub_ke));
    wait_admin_space_ready(z_session_loan(&s2), z_view_keyexpr_loan(&link_sub_ke));
    z_sleep_ms(500);
    open_connector_session(&s3, "tcp/127.0.0.1:7457");

    z_owned_string_t transport_put_payload;
    z_owned_string_t link_put_payload;
    wait_for_sample_kind(&transport_sub_handler, Z_SAMPLE_KIND_PUT, &transport_put_payload);
    wait_for_sample_kind(&link_sub_handler, Z_SAMPLE_KIND_PUT, &link_put_payload);

    assert_contains(z_string_loan(&transport_put_payload), "\"zid\"");
    assert_contains(z_string_loan(&transport_put_payload), "\"whatami\"");
    assert_contains(z_string_loan(&transport_put_payload), "\"is_qos\"");
    assert_contains(z_string_loan(&transport_put_payload), "\"is_multicast\"");
    assert_contains(z_string_loan(&transport_put_payload), "\"is_shm\"");

    assert_contains(z_string_loan(&link_put_payload), "\"zid\"");
    assert_contains(z_string_loan(&link_put_payload), "\"src\"");
    assert_contains(z_string_loan(&link_put_payload), "\"dst\"");
    assert_contains(z_string_loan(&link_put_payload), "\"mtu\"");
    assert_contains(z_string_loan(&link_put_payload), "\"is_streamed\"");
    assert_contains(z_string_loan(&link_put_payload), "\"is_reliable\"");

    z_drop(z_move(transport_put_payload));
    z_drop(z_move(link_put_payload));

    close_session_with_tasks(&s3);

    wait_for_sample_kind(&link_sub_handler, Z_SAMPLE_KIND_DELETE, NULL);
    wait_for_sample_kind(&transport_sub_handler, Z_SAMPLE_KIND_DELETE, NULL);

    ASSERT_OK(z_undeclare_subscriber(z_subscriber_move(&transport_sub)));
    ASSERT_OK(z_undeclare_subscriber(z_subscriber_move(&link_sub)));
    z_drop(z_move(transport_sub_handler));
    z_drop(z_move(link_sub_handler));

    close_session_with_tasks(&s2);
    ASSERT_OK(zp_stop_admin_space(z_session_loan_mut(&s1)));
    close_session_with_tasks(&s1);
}
#endif

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_start_stop_admin_space();
    test_auto_start_admin_space();
    test_admin_space_query_succeeds();
    test_admin_space_query_fails_when_not_running();
#if Z_FEATURE_CONNECTIVITY == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_LINK_TCP == 1 && \
    Z_FEATURE_MULTI_THREAD == 1
    test_admin_space_connectivity_events();
#endif
    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("Missing config token to build this test. This test requires: Z_FEATURE_ADMIN_SPACE\n");
    return 0;
}

#endif  // Z_FEATURE_ADMIN_SPACE == 1
