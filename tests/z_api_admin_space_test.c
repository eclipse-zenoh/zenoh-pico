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

} admin_space_query_reply_t;

void admin_space_query_reply_clear(admin_space_query_reply_t *reply) {
    z_drop(z_move(reply->ke));
    z_drop(z_move(reply->payload));
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

static const char *link_type_to_str(int t) {
    switch (t) {
        case _Z_LINK_TYPE_TCP:
            return "tcp";
        case _Z_LINK_TYPE_UDP:
            return "udp";
        case _Z_LINK_TYPE_BT:
            return "bt";
        case _Z_LINK_TYPE_SERIAL:
            return "serial";
        case _Z_LINK_TYPE_WS:
            return "ws";
        case _Z_LINK_TYPE_TLS:
            return "tls";
        case _Z_LINK_TYPE_RAWETH:
            return "raweth";
        default:
            return NULL;
    }
}

static const char *cap_transport_to_str(int t) {
    switch (t) {
        case Z_LINK_CAP_TRANSPORT_UNICAST:
            return "unicast";
        case Z_LINK_CAP_TRANSPORT_MULTICAST:
            return "multicast";
        case Z_LINK_CAP_TRANSPORT_RAWETH:
            return "raweth";
        default:
            return NULL;
    }
}

static const char *cap_flow_to_str(int f) {
    switch (f) {
        case Z_LINK_CAP_FLOW_DATAGRAM:
            return "datagram";
        case Z_LINK_CAP_FLOW_STREAM:
            return "stream";
        default:
            return NULL;
    }
}

static void verify_transport_reply_json(const z_loaned_string_t *payload, z_whatami_t mode, const _z_link_t *link) {
    assert_json_object(payload);

    // top-level
    assert_contains(payload, "\"mode\"");
    assert_contains(payload, "\"link\"");

    // exact mode value
    const char *mode_s = whatami_to_str(mode);
    ASSERT_NOT_NULL(mode_s);
    assert_contains_quoted_value(payload, "mode", mode_s);

    // link.type
    const char *lt = link_type_to_str(link->_type);
    ASSERT_NOT_NULL(lt);
    assert_contains(payload, "\"type\"");
    assert_contains_quoted_value(payload, "type", lt);

    // endpoint structure
    assert_contains(payload, "\"endpoint\"");
    assert_contains(payload, "\"locator\"");
    assert_contains(payload, "\"metadata\":{");
    assert_contains(payload, "\"protocol\"");
    assert_contains(payload, "\"address\"");
    assert_contains(payload, "\"config\":{");

    // verify protocol and address values are present
    assert_contains_z_string(payload, &link->_endpoint._locator._protocol);
    assert_contains_z_string(payload, &link->_endpoint._locator._address);

    // capabilities
    assert_contains(payload, "\"capabilities\"");
    assert_contains(payload, "\"transport\"");
    assert_contains(payload, "\"flow\"");
    assert_contains(payload, "\"is_reliable\"");

    const char *ct = cap_transport_to_str(link->_cap._transport);
    const char *cf = cap_flow_to_str(link->_cap._flow);
    ASSERT_NOT_NULL(ct);
    ASSERT_NOT_NULL(cf);
    assert_contains_quoted_value(payload, "transport", ct);
    assert_contains_quoted_value(payload, "flow", cf);

    // verify is_reliable matches link capability
    assert_contains(payload, link->_cap._is_reliable ? "\"is_reliable\":true" : "\"is_reliable\":false");
}

static void verify_peer_common_json(const z_loaned_string_t *payload, const z_id_t *expected_zid,
                                    z_whatami_t expected_whatami) {
    assert_json_object(payload);
    assert_contains(payload, "\"zid\"");
    assert_contains(payload, "\"whatami\"");

    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(expected_zid, &zid_str));

    // Check zid string appears somewhere
    assert_contains_z_string(payload, z_loan(zid_str));

    z_drop(z_move(zid_str));

    const char *ws = whatami_to_str(expected_whatami);
    ASSERT_NOT_NULL(ws);
    assert_contains_quoted_value(payload, "whatami", ws);
}

static void build_transport_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *base_ke, const char *transport) {
    ASSERT_OK(z_keyexpr_clone(out, base_ke));
    ASSERT_OK(_z_keyexpr_append_str(out, transport));
}

static void build_peer_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *base_ke, const char *transport,
                          const z_id_t *peer_zid) {
    ASSERT_OK(z_keyexpr_clone(out, base_ke));
    ASSERT_OK(_z_keyexpr_append_str(out, transport));
    ASSERT_OK(_z_keyexpr_append_str(out, _Z_KEYEXPR_LINK));

    /* append peer zid */
    z_owned_string_t s;
    ASSERT_OK(z_id_to_string(peer_zid, &s));
    ASSERT_OK(_z_keyexpr_append_substr(out, z_string_data(z_string_loan(&s)), z_string_len(z_string_loan(&s))));
    z_string_drop(z_string_move(&s));
}

static void verify_admin_space_query_unicast(const z_loaned_session_t *zs,
                                             const admin_space_query_reply_list_t *results,
                                             const z_loaned_keyexpr_t *base_ke, const _z_transport_unicast_t *unicast) {
    // Transport reply
    admin_space_query_reply_t expected;
    z_internal_keyexpr_null(&expected.ke);
    z_internal_string_null(&expected.payload);
    build_transport_ke(&expected.ke, base_ke, _Z_KEYEXPR_TRANSPORT_UNICAST);

    admin_space_query_reply_list_t *entry =
        admin_space_query_reply_list_find(results, admin_space_query_reply_eq, &expected);
    ASSERT_NOT_NULL(entry);
    const admin_space_query_reply_t *reply = admin_space_query_reply_list_value(entry);
    ASSERT_NOT_NULL(reply);
    admin_space_query_reply_elem_clear(&expected);

    verify_transport_reply_json(z_string_loan(&reply->payload), _Z_RC_IN_VAL(zs)->_mode, unicast->_common._link);

    // Peer replies
    for (_z_transport_peer_unicast_slist_t *it = unicast->_peers; it != NULL;
         it = _z_transport_peer_unicast_slist_next(it)) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(it);

        admin_space_query_reply_t peer_expected;
        z_internal_keyexpr_null(&peer_expected.ke);
        z_internal_string_null(&peer_expected.payload);
        build_peer_ke(&peer_expected.ke, base_ke, _Z_KEYEXPR_TRANSPORT_UNICAST, &peer->common._remote_zid);

        entry = admin_space_query_reply_list_find(results, admin_space_query_reply_eq, &peer_expected);
        ASSERT_NOT_NULL(entry);
        reply = admin_space_query_reply_list_value(entry);
        ASSERT_NOT_NULL(reply);
        admin_space_query_reply_elem_clear(&peer_expected);

        verify_peer_common_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                peer->common._remote_whatami);
    }
}

static void verify_admin_space_query_multicast(const z_loaned_session_t *zs,
                                               const admin_space_query_reply_list_t *results,
                                               const z_loaned_keyexpr_t *base_ke,
                                               const _z_transport_multicast_t *multicast) {
    const _z_session_t *session = _Z_RC_IN_VAL(zs);
    const char *transport =
        (session->_tp._type == _Z_TRANSPORT_RAWETH_TYPE) ? _Z_KEYEXPR_TRANSPORT_RAWETH : _Z_KEYEXPR_TRANSPORT_MULTICAST;

    // Transport reply
    admin_space_query_reply_t expected;
    z_internal_keyexpr_null(&expected.ke);
    z_internal_string_null(&expected.payload);
    build_transport_ke(&expected.ke, base_ke, transport);

    admin_space_query_reply_list_t *entry =
        admin_space_query_reply_list_find(results, admin_space_query_reply_eq, &expected);
    ASSERT_NOT_NULL(entry);
    const admin_space_query_reply_t *reply = admin_space_query_reply_list_value(entry);
    ASSERT_NOT_NULL(reply);
    admin_space_query_reply_elem_clear(&expected);

    verify_transport_reply_json(z_string_loan(&reply->payload), session->_mode, multicast->_common._link);

    // Peer replies
    for (_z_transport_peer_multicast_slist_t *it = multicast->_peers; it != NULL;
         it = _z_transport_peer_multicast_slist_next(it)) {
        _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(it);

        admin_space_query_reply_t peer_expected;
        z_internal_keyexpr_null(&peer_expected.ke);
        z_internal_string_null(&peer_expected.payload);
        build_peer_ke(&peer_expected.ke, base_ke, transport, &peer->common._remote_zid);

        entry = admin_space_query_reply_list_find(results, admin_space_query_reply_eq, &peer_expected);
        ASSERT_NOT_NULL(entry);
        reply = admin_space_query_reply_list_value(entry);
        ASSERT_NOT_NULL(reply);
        admin_space_query_reply_elem_clear(&peer_expected);

        verify_peer_common_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                peer->common._remote_whatami);
        assert_contains(z_string_loan(&reply->payload), "\"remote_addr\"");
    }
}

static void verify_admin_space_query(const z_loaned_session_t *zs, const admin_space_query_reply_list_t *results,
                                     const z_loaned_keyexpr_t *base_ke) {
    _z_session_t *session = _Z_RC_IN_VAL(zs);

    size_t expected_replies = 1;
    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            expected_replies += _z_transport_peer_unicast_slist_len(session->_tp._transport._unicast._peers);
            verify_admin_space_query_unicast(zs, results, base_ke, &session->_tp._transport._unicast);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            expected_replies += _z_transport_peer_multicast_slist_len(session->_tp._transport._multicast._peers);
            verify_admin_space_query_multicast(zs, results, base_ke, &session->_tp._transport._multicast);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            expected_replies += _z_transport_peer_multicast_slist_len(session->_tp._transport._raweth._peers);
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

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_start_stop_admin_space();
    test_auto_start_admin_space();
    test_admin_space_query_succeeds();
    test_admin_space_query_fails_when_not_running();
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
