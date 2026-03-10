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

bool admin_space_query_reply_same_ke(const admin_space_query_reply_t *left, const admin_space_query_reply_t *right) {
    return z_keyexpr_equals(z_loan(left->ke), z_loan(right->ke));
}

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
_Z_ELEM_DEFINE(admin_space_query_reply, admin_space_query_reply_t, _z_noop_size, admin_space_query_reply_clear,
               _z_noop_copy, _z_noop_move, admin_space_query_reply_same_ke, _z_noop_cmp, _z_noop_hash)
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

                ASSERT_TRUE(admin_space_query_reply_list_find(results, admin_space_query_reply_same_ke, result) ==
                            NULL);

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

typedef struct admin_space_test_keyexprs_t {
    z_owned_keyexpr_t pico_ke;
    z_owned_keyexpr_t session_ke;
    z_owned_keyexpr_t transports_ke;
    z_owned_keyexpr_t transport_0_ke;
    z_owned_keyexpr_t peers_ke;
} admin_space_test_keyexprs_t;

typedef struct admin_space_test_sessions_t {
    z_owned_session_t s1;
    z_owned_session_t s2;
} admin_space_test_sessions_t;

static void admin_space_test_sessions_open(admin_space_test_sessions_t *ss) {
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    ASSERT_OK(z_open(&ss->s1, z_move(c1), NULL));
    ASSERT_OK(zp_start_admin_space(z_loan_mut(ss->s1)));
    ASSERT_OK(z_open(&ss->s2, z_move(c2), NULL));

    z_sleep_ms(250);
}

static void admin_space_test_sessions_close(admin_space_test_sessions_t *ss) {
    ASSERT_OK(zp_stop_admin_space(z_loan_mut(ss->s1)));
    z_drop(z_move(ss->s1));
    z_drop(z_move(ss->s2));
}

static void admin_space_test_keyexprs_clear(admin_space_test_keyexprs_t *kes) {
    z_drop(z_move(kes->peers_ke));
    z_drop(z_move(kes->transport_0_ke));
    z_drop(z_move(kes->transports_ke));
    z_drop(z_move(kes->session_ke));
    z_drop(z_move(kes->pico_ke));
}

static void assert_json_object(const z_loaned_string_t *s) {
    const char *p = z_string_data(s);
    size_t n = z_string_len(s);
    ASSERT_TRUE(n >= 2);
    ASSERT_TRUE(p[0] == '{');
    ASSERT_TRUE(p[n - 1] == '}');
}

static void assert_json_array(const z_loaned_string_t *s) {
    const char *p = z_string_data(s);
    size_t n = z_string_len(s);
    ASSERT_TRUE(n >= 2);
    ASSERT_TRUE(p[0] == '[');
    ASSERT_TRUE(p[n - 1] == ']');
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

static void assert_not_contains(const z_loaned_string_t *s, const char *needle) {
    const char *start = z_string_data(s);
    const char *end = start + z_string_len(s);
    if (_z_strstr(start, end, needle) != NULL) {
        fprintf(stderr, "Assertion failed: unexpected substring found.\nActual: %.*s\nNeedle: %s\n", (int)(end - start),
                start, needle);
    }
    ASSERT_TRUE(_z_strstr(start, end, needle) == NULL);
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

static const char *transport_type_to_str(int t) {
    switch (t) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            return "unicast";
        case _Z_TRANSPORT_MULTICAST_TYPE:
            return "multicast";
        case _Z_TRANSPORT_RAWETH_TYPE:
            return "raweth";
        default:
            return NULL;
    }
}

static const _z_link_t *admin_space_test_transport_link(const _z_session_t *session) {
    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            return session->_tp._transport._unicast._common._link;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            return session->_tp._transport._multicast._common._link;
        case _Z_TRANSPORT_RAWETH_TYPE:
            return session->_tp._transport._raweth._common._link;
        default:
            ASSERT_TRUE(false);
            return NULL;
    }
}

static void assert_contains_session_header(const z_loaned_string_t *payload, const z_id_t *zid, z_whatami_t whatami,
                                           bool wrapped) {
    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(zid, &zid_str));

    const char *whatami_str = whatami_to_str(whatami);
    ASSERT_NOT_NULL(whatami_str);

    char buf[256];
    int n;
    if (wrapped) {
        n = snprintf(buf, sizeof(buf), "\"session\":{\"zid\":\"%.*s\",\"whatami\":\"%s\"",
                     (int)z_string_len(z_loan(zid_str)), z_string_data(z_loan(zid_str)), whatami_str);
    } else {
        n = snprintf(buf, sizeof(buf), "{\"zid\":\"%.*s\",\"whatami\":\"%s\"", (int)z_string_len(z_loan(zid_str)),
                     z_string_data(z_loan(zid_str)), whatami_str);
    }

    ASSERT_TRUE(n > 0 && (size_t)n < sizeof(buf));
    assert_contains(payload, buf);
    z_drop(z_move(zid_str));
}

static void assert_contains_transport_header(const z_loaned_string_t *payload, const char *transport_type) {
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "{\"type\":\"%s\",\"link\":", transport_type);
    ASSERT_TRUE(n > 0 && (size_t)n < sizeof(buf));
    assert_contains(payload, buf);
}

static void assert_contains_link_header(const z_loaned_string_t *payload, const char *link_type) {
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "\"link\":{\"type\":\"%s\",\"endpoint\":", link_type);
    ASSERT_TRUE(n > 0 && (size_t)n < sizeof(buf));
    assert_contains(payload, buf);
}

static void assert_contains_peer_header(const z_loaned_string_t *payload, const z_id_t *zid, z_whatami_t whatami,
                                        bool expect_remote_addr) {
    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(zid, &zid_str));

    const char *whatami_str = whatami_to_str(whatami);
    ASSERT_NOT_NULL(whatami_str);

    char buf[256];
    int n;
    if (expect_remote_addr) {
        n = snprintf(buf, sizeof(buf),
                     "{\"zid\":\"%.*s\",\"whatami\":\"%s\",\"remote_addr\":", (int)z_string_len(z_loan(zid_str)),
                     z_string_data(z_loan(zid_str)), whatami_str);
    } else {
        n = snprintf(buf, sizeof(buf), "{\"zid\":\"%.*s\",\"whatami\":\"%s\"", (int)z_string_len(z_loan(zid_str)),
                     z_string_data(z_loan(zid_str)), whatami_str);
    }

    ASSERT_TRUE(n > 0 && (size_t)n < sizeof(buf));
    assert_contains(payload, buf);
    z_drop(z_move(zid_str));
}

static void build_pico_ke(z_owned_keyexpr_t *out, const z_id_t *zid) {
    z_owned_string_t s;
    ASSERT_OK(z_id_to_string(zid, &s));

    z_internal_keyexpr_null(out);
    ASSERT_OK(_z_keyexpr_append_str(out, _Z_KEYEXPR_AT));
    ASSERT_OK(_z_keyexpr_append_substr(out, z_string_data(z_loan(s)), z_string_len(z_loan(s))));
    ASSERT_OK(_z_keyexpr_append_str(out, _Z_KEYEXPR_PICO));

    z_drop(z_move(s));
}

static void build_pico_session_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *pico_ke) {
    ASSERT_OK(z_keyexpr_clone(out, pico_ke));
    ASSERT_OK(_z_keyexpr_append_str(out, _Z_KEYEXPR_SESSION));
}

static void build_pico_transports_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *session_ke) {
    ASSERT_OK(z_keyexpr_clone(out, session_ke));
    ASSERT_OK(_z_keyexpr_append_str(out, _Z_KEYEXPR_TRANSPORTS));
}

static void build_pico_transport_0_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *transports_ke) {
    ASSERT_OK(z_keyexpr_clone(out, transports_ke));
    ASSERT_OK(_z_keyexpr_append_str(out, "0"));
}

static void build_pico_transport_0_peers_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *transport_0_ke) {
    ASSERT_OK(z_keyexpr_clone(out, transport_0_ke));
    ASSERT_OK(_z_keyexpr_append_str(out, _Z_KEYEXPR_PEERS));
}

static void build_pico_transport_0_peer_ke(z_owned_keyexpr_t *out, const z_loaned_keyexpr_t *peers_ke,
                                           const z_id_t *peer_zid) {
    z_owned_string_t s;
    ASSERT_OK(z_id_to_string(peer_zid, &s));

    ASSERT_OK(z_keyexpr_clone(out, peers_ke));
    ASSERT_OK(_z_keyexpr_append_substr(out, z_string_data(z_loan(s)), z_string_len(z_loan(s))));

    z_drop(z_move(s));
}

static const admin_space_query_reply_t *find_expected_reply(const admin_space_query_reply_list_t *results,
                                                            const z_loaned_keyexpr_t *expected_ke) {
    admin_space_query_reply_t expected;
    z_internal_keyexpr_null(&expected.ke);
    z_internal_string_null(&expected.payload);
    z_internal_encoding_null(&expected.encoding);
    ASSERT_OK(z_keyexpr_clone(&expected.ke, expected_ke));

    admin_space_query_reply_list_t *entry =
        admin_space_query_reply_list_find(results, admin_space_query_reply_same_ke, &expected);
    admin_space_query_reply_elem_clear(&expected);

    ASSERT_NOT_NULL(entry);
    const admin_space_query_reply_t *reply = admin_space_query_reply_list_value(entry);
    ASSERT_NOT_NULL(reply);
    ASSERT_TRUE(z_encoding_equals(z_loan(reply->encoding), z_encoding_application_json()));
    return reply;
}

static const admin_space_query_reply_t *run_exact_admin_space_query(const z_loaned_session_t *zs,
                                                                    const z_loaned_keyexpr_t *ke,
                                                                    admin_space_query_reply_list_t **results_out) {
    admin_space_query_reply_list_t *results = run_admin_space_query(zs, ke);
    ASSERT_TRUE(admin_space_query_reply_list_len(results) == 1);

    const admin_space_query_reply_t *reply = find_expected_reply(results, ke);
    ASSERT_NOT_NULL(reply);

    *results_out = results;
    return reply;
}

static void admin_space_test_keyexprs_init(admin_space_test_keyexprs_t *kes, const z_id_t *zid) {
    z_internal_keyexpr_null(&kes->pico_ke);
    z_internal_keyexpr_null(&kes->session_ke);
    z_internal_keyexpr_null(&kes->transports_ke);
    z_internal_keyexpr_null(&kes->transport_0_ke);
    z_internal_keyexpr_null(&kes->peers_ke);

    build_pico_ke(&kes->pico_ke, zid);
    build_pico_session_ke(&kes->session_ke, z_loan(kes->pico_ke));
    build_pico_transports_ke(&kes->transports_ke, z_loan(kes->session_ke));
    build_pico_transport_0_ke(&kes->transport_0_ke, z_loan(kes->transports_ke));
    build_pico_transport_0_peers_ke(&kes->peers_ke, z_loan(kes->transport_0_ke));
}

static size_t expected_admin_space_reply_count(const _z_session_t *session) {
    size_t count = 5;  // pico, session, transports, transports/0, transports/0/peers

    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            count += _z_transport_peer_unicast_slist_len(session->_tp._transport._unicast._peers);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            count += _z_transport_peer_multicast_slist_len(session->_tp._transport._multicast._peers);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            count += _z_transport_peer_multicast_slist_len(session->_tp._transport._raweth._peers);
            break;
        default:
            break;
    }

    return count;
}

static void verify_peer_json(const z_loaned_string_t *payload, const z_id_t *expected_zid, z_whatami_t expected_whatami,
                             bool expect_remote_addr) {
    assert_json_object(payload);
    assert_contains_peer_header(payload, expected_zid, expected_whatami, expect_remote_addr);
}

static void verify_peers_array_json_unicast(const z_loaned_string_t *payload, const _z_transport_unicast_t *tp) {
    assert_json_array(payload);

    for (_z_transport_peer_unicast_slist_t *it = tp->_peers; it != NULL;
         it = _z_transport_peer_unicast_slist_next(it)) {
        const _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(it);
        assert_contains_peer_header(payload, &peer->common._remote_zid, peer->common._remote_whatami, false);
    }
}

static void verify_peers_array_json_multicast(const z_loaned_string_t *payload, const _z_transport_multicast_t *tp) {
    assert_json_array(payload);

    for (_z_transport_peer_multicast_slist_t *it = tp->_peers; it != NULL;
         it = _z_transport_peer_multicast_slist_next(it)) {
        const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(it);
        assert_contains_peer_header(payload, &peer->common._remote_zid, peer->common._remote_whatami, true);
    }
}

static void verify_transport_json(const z_loaned_string_t *payload, int transport_type, const _z_link_t *link) {
    assert_contains(payload, "\"link\"");
    assert_contains(payload, "\"peers\"");

    const char *tt = transport_type_to_str(transport_type);
    ASSERT_NOT_NULL(tt);
    assert_contains_transport_header(payload, tt);

    const char *lt = link_type_to_str(link->_type);
    ASSERT_NOT_NULL(lt);
    assert_contains_link_header(payload, lt);

    assert_contains(payload, "\"endpoint\"");
    assert_contains(payload, "\"locator\"");
    assert_contains(payload, "\"metadata\":{");
    assert_contains(payload, "\"protocol\"");
    assert_contains(payload, "\"address\"");
    assert_contains(payload, "\"config\":{");

    assert_contains_z_string(payload, &link->_endpoint._locator._protocol);
    assert_contains_z_string(payload, &link->_endpoint._locator._address);

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
    assert_contains(payload, link->_cap._is_reliable ? "\"is_reliable\":true" : "\"is_reliable\":false");
}

static void verify_session_json(const z_loaned_string_t *payload, const _z_session_t *session) {
    assert_json_object(payload);
    assert_contains(payload, "\"transports\"");
    assert_contains_session_header(payload, &session->_local_zid, session->_mode, false);
}

static void verify_pico_json(const z_loaned_string_t *payload, const _z_session_t *session) {
    assert_json_object(payload);
    assert_contains(payload, "\"session\"");
    assert_contains(payload, "\"transports\"");
    assert_contains_session_header(payload, &session->_local_zid, session->_mode, true);
}

static void verify_admin_space_query(const z_loaned_session_t *zs, const admin_space_query_reply_list_t *results) {
    const _z_session_t *session = _Z_RC_IN_VAL(zs);
    const admin_space_query_reply_t *reply;
    const z_loaned_string_t *payload;

    z_owned_keyexpr_t pico_ke, session_ke, transports_ke, transport_0_ke, peers_ke;
    z_internal_keyexpr_null(&pico_ke);
    z_internal_keyexpr_null(&session_ke);
    z_internal_keyexpr_null(&transports_ke);
    z_internal_keyexpr_null(&transport_0_ke);
    z_internal_keyexpr_null(&peers_ke);

    build_pico_ke(&pico_ke, &session->_local_zid);
    build_pico_session_ke(&session_ke, z_loan(pico_ke));
    build_pico_transports_ke(&transports_ke, z_loan(session_ke));
    build_pico_transport_0_ke(&transport_0_ke, z_loan(transports_ke));
    build_pico_transport_0_peers_ke(&peers_ke, z_loan(transport_0_ke));

    ASSERT_TRUE(admin_space_query_reply_list_len(results) == expected_admin_space_reply_count(session));

    reply = find_expected_reply(results, z_loan(pico_ke));
    verify_pico_json(z_string_loan(&reply->payload), session);

    reply = find_expected_reply(results, z_loan(session_ke));
    verify_session_json(z_string_loan(&reply->payload), session);

    reply = find_expected_reply(results, z_loan(transports_ke));
    payload = z_string_loan(&reply->payload);
    assert_json_array(payload);
    verify_transport_json(payload, session->_tp._type, admin_space_test_transport_link(session));

    reply = find_expected_reply(results, z_loan(transport_0_ke));
    payload = z_string_loan(&reply->payload);
    assert_json_object(payload);
    verify_transport_json(payload, session->_tp._type, admin_space_test_transport_link(session));

    reply = find_expected_reply(results, z_loan(peers_ke));
    payload = z_string_loan(&reply->payload);

    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            verify_peers_array_json_unicast(payload, &session->_tp._transport._unicast);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            verify_peers_array_json_multicast(payload, &session->_tp._transport._multicast);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            verify_peers_array_json_multicast(payload, &session->_tp._transport._raweth);
            break;
        default:
            ASSERT_TRUE(false);
    }

    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE: {
            const _z_transport_unicast_t *tp = &session->_tp._transport._unicast;
            for (_z_transport_peer_unicast_slist_t *it = tp->_peers; it != NULL;
                 it = _z_transport_peer_unicast_slist_next(it)) {
                const _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(it);

                z_owned_keyexpr_t peer_ke;
                z_internal_keyexpr_null(&peer_ke);
                build_pico_transport_0_peer_ke(&peer_ke, z_loan(peers_ke), &peer->common._remote_zid);

                reply = find_expected_reply(results, z_loan(peer_ke));
                verify_peer_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                 peer->common._remote_whatami, false);

                z_drop(z_move(peer_ke));
            }
            break;
        }

        case _Z_TRANSPORT_MULTICAST_TYPE: {
            const _z_transport_multicast_t *tp = &session->_tp._transport._multicast;
            for (_z_transport_peer_multicast_slist_t *it = tp->_peers; it != NULL;
                 it = _z_transport_peer_multicast_slist_next(it)) {
                const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(it);

                z_owned_keyexpr_t peer_ke;
                z_internal_keyexpr_null(&peer_ke);
                build_pico_transport_0_peer_ke(&peer_ke, z_loan(peers_ke), &peer->common._remote_zid);

                reply = find_expected_reply(results, z_loan(peer_ke));
                verify_peer_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                 peer->common._remote_whatami, true);

                z_drop(z_move(peer_ke));
            }
            break;
        }

        case _Z_TRANSPORT_RAWETH_TYPE: {
            const _z_transport_multicast_t *tp = &session->_tp._transport._raweth;
            for (_z_transport_peer_multicast_slist_t *it = tp->_peers; it != NULL;
                 it = _z_transport_peer_multicast_slist_next(it)) {
                const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(it);

                z_owned_keyexpr_t peer_ke;
                z_internal_keyexpr_null(&peer_ke);
                build_pico_transport_0_peer_ke(&peer_ke, z_loan(peers_ke), &peer->common._remote_zid);

                reply = find_expected_reply(results, z_loan(peer_ke));
                verify_peer_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                 peer->common._remote_whatami, true);

                z_drop(z_move(peer_ke));
            }
            break;
        }

        default:
            ASSERT_TRUE(false);
    }

    z_drop(z_move(peers_ke));
    z_drop(z_move(transport_0_ke));
    z_drop(z_move(transports_ke));
    z_drop(z_move(session_ke));
    z_drop(z_move(pico_ke));
}

void test_start_stop_admin_space(void) {
    printf("test_start_stop_admin_space\n");
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

void test_auto_start_admin_space(void) {
    printf("test_auto_start_admin_space\n");
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

void test_admin_space_query_fails_when_not_running(void) {
    printf("test_admin_space_query_fails_when_not_running\n");
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    ASSERT_OK(z_open(&s2, z_move(c2), NULL));

    // Wait for sessions to connect
    z_sleep_ms(250);

    // Build query keyexpr: @/<zid>/pico/**
    z_id_t zid = z_info_zid(z_loan(s1));
    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(&zid, &zid_str));

    z_owned_keyexpr_t ke;
    z_internal_keyexpr_null(&ke);

    ASSERT_OK(_z_keyexpr_append_str(&ke, _Z_KEYEXPR_AT));
    ASSERT_OK(_z_keyexpr_append_substr(&ke, z_string_data(z_loan(zid_str)), z_string_len(z_loan(zid_str))));
    z_drop(z_move(zid_str));
    ASSERT_OK(_z_keyexpr_append_str(&ke, _Z_KEYEXPR_PICO));
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

void test_admin_space_general_query_succeeds(void) {
    printf("test_admin_space_general_query_succeeds\n");
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    ASSERT_OK(z_open(&s1, z_move(c1), NULL));
    ASSERT_OK(zp_start_admin_space(z_loan_mut(s1)));
    ASSERT_OK(z_open(&s2, z_move(c2), NULL));

    z_sleep_ms(250);

    z_id_t zid = z_info_zid(z_loan(s1));
    z_owned_string_t zid_str;
    ASSERT_OK(z_id_to_string(&zid, &zid_str));

    z_owned_keyexpr_t query_ke;
    z_internal_keyexpr_null(&query_ke);
    ASSERT_OK(_z_keyexpr_append_str(&query_ke, _Z_KEYEXPR_AT));
    ASSERT_OK(_z_keyexpr_append_substr(&query_ke, z_string_data(z_loan(zid_str)), z_string_len(z_loan(zid_str))));
    z_drop(z_move(zid_str));
    ASSERT_OK(_z_keyexpr_append_str(&query_ke, _Z_KEYEXPR_PICO));
    ASSERT_OK(_z_keyexpr_append_str(&query_ke, _Z_KEYEXPR_STARSTAR));

    admin_space_query_reply_list_t *results = run_admin_space_query(z_loan(s2), z_loan(query_ke));
    verify_admin_space_query(z_loan(s1), results);

    admin_space_query_reply_list_free(&results);
    z_drop(z_move(query_ke));

    ASSERT_OK(zp_stop_admin_space(z_loan_mut(s1)));
    z_drop(z_move(s1));
    z_drop(z_move(s2));
}

void test_admin_space_pico_endpoint_succeeds(void) {
    printf("test_admin_space_pico_endpoint_succeeds\n");

    admin_space_test_sessions_t ss;
    admin_space_test_sessions_open(&ss);

    const _z_session_t *session = _Z_RC_IN_VAL(z_loan(ss.s1));

    admin_space_test_keyexprs_t kes;
    admin_space_test_keyexprs_init(&kes, &session->_local_zid);

    admin_space_query_reply_list_t *results;
    const admin_space_query_reply_t *reply = run_exact_admin_space_query(z_loan(ss.s2), z_loan(kes.pico_ke), &results);

    verify_pico_json(z_string_loan(&reply->payload), session);

    admin_space_query_reply_list_free(&results);
    admin_space_test_keyexprs_clear(&kes);
    admin_space_test_sessions_close(&ss);
}

void test_admin_space_session_endpoint_succeeds(void) {
    printf("test_admin_space_session_endpoint_succeeds\n");

    admin_space_test_sessions_t ss;
    admin_space_test_sessions_open(&ss);

    const _z_session_t *session = _Z_RC_IN_VAL(z_loan(ss.s1));

    admin_space_test_keyexprs_t kes;
    admin_space_test_keyexprs_init(&kes, &session->_local_zid);

    admin_space_query_reply_list_t *results;
    const admin_space_query_reply_t *reply =
        run_exact_admin_space_query(z_loan(ss.s2), z_loan(kes.session_ke), &results);

    verify_session_json(z_string_loan(&reply->payload), session);

    admin_space_query_reply_list_free(&results);
    admin_space_test_keyexprs_clear(&kes);
    admin_space_test_sessions_close(&ss);
}

void test_admin_space_transports_endpoint_succeeds(void) {
    printf("test_admin_space_transports_endpoint_succeeds\n");

    admin_space_test_sessions_t ss;
    admin_space_test_sessions_open(&ss);

    const _z_session_t *session = _Z_RC_IN_VAL(z_loan(ss.s1));

    admin_space_test_keyexprs_t kes;
    admin_space_test_keyexprs_init(&kes, &session->_local_zid);

    admin_space_query_reply_list_t *results;
    const admin_space_query_reply_t *reply =
        run_exact_admin_space_query(z_loan(ss.s2), z_loan(kes.transports_ke), &results);

    const z_loaned_string_t *payload = z_string_loan(&reply->payload);
    assert_json_array(payload);
    verify_transport_json(payload, session->_tp._type, admin_space_test_transport_link(session));

    admin_space_query_reply_list_free(&results);
    admin_space_test_keyexprs_clear(&kes);
    admin_space_test_sessions_close(&ss);
}

void test_admin_space_transport_0_endpoint_succeeds(void) {
    printf("test_admin_space_transport_0_endpoint_succeeds\n");

    admin_space_test_sessions_t ss;
    admin_space_test_sessions_open(&ss);

    const _z_session_t *session = _Z_RC_IN_VAL(z_loan(ss.s1));

    admin_space_test_keyexprs_t kes;
    admin_space_test_keyexprs_init(&kes, &session->_local_zid);

    admin_space_query_reply_list_t *results;
    const admin_space_query_reply_t *reply =
        run_exact_admin_space_query(z_loan(ss.s2), z_loan(kes.transport_0_ke), &results);

    const z_loaned_string_t *payload = z_string_loan(&reply->payload);
    assert_json_object(payload);
    verify_transport_json(payload, session->_tp._type, admin_space_test_transport_link(session));

    admin_space_query_reply_list_free(&results);
    admin_space_test_keyexprs_clear(&kes);
    admin_space_test_sessions_close(&ss);
}

void test_admin_space_transport_0_peers_endpoint_succeeds(void) {
    printf("test_admin_space_transport_0_peers_endpoint_succeeds\n");

    admin_space_test_sessions_t ss;
    admin_space_test_sessions_open(&ss);

    const _z_session_t *session = _Z_RC_IN_VAL(z_loan(ss.s1));

    admin_space_test_keyexprs_t kes;
    admin_space_test_keyexprs_init(&kes, &session->_local_zid);

    admin_space_query_reply_list_t *results;
    const admin_space_query_reply_t *reply = run_exact_admin_space_query(z_loan(ss.s2), z_loan(kes.peers_ke), &results);

    const z_loaned_string_t *payload = z_string_loan(&reply->payload);
    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            verify_peers_array_json_unicast(payload, &session->_tp._transport._unicast);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            verify_peers_array_json_multicast(payload, &session->_tp._transport._multicast);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            verify_peers_array_json_multicast(payload, &session->_tp._transport._raweth);
            break;
        default:
            ASSERT_TRUE(false);
    }

    admin_space_query_reply_list_free(&results);
    admin_space_test_keyexprs_clear(&kes);
    admin_space_test_sessions_close(&ss);
}

void test_admin_space_transport_0_peer_endpoints_succeeds(void) {
    printf("test_admin_space_transport_0_peer_endpoints_succeeds\n");

    admin_space_test_sessions_t ss;
    admin_space_test_sessions_open(&ss);

    const _z_session_t *session = _Z_RC_IN_VAL(z_loan(ss.s1));

    admin_space_test_keyexprs_t kes;
    admin_space_test_keyexprs_init(&kes, &session->_local_zid);

    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE: {
            const _z_transport_unicast_t *tp = &session->_tp._transport._unicast;
            for (_z_transport_peer_unicast_slist_t *it = tp->_peers; it != NULL;
                 it = _z_transport_peer_unicast_slist_next(it)) {
                const _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(it);

                z_owned_keyexpr_t peer_ke;
                z_internal_keyexpr_null(&peer_ke);
                build_pico_transport_0_peer_ke(&peer_ke, z_loan(kes.peers_ke), &peer->common._remote_zid);

                admin_space_query_reply_list_t *results;
                const admin_space_query_reply_t *reply =
                    run_exact_admin_space_query(z_loan(ss.s2), z_loan(peer_ke), &results);

                verify_peer_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                 peer->common._remote_whatami, false);

                admin_space_query_reply_list_free(&results);
                z_drop(z_move(peer_ke));
            }
            break;
        }

        case _Z_TRANSPORT_MULTICAST_TYPE: {
            const _z_transport_multicast_t *tp = &session->_tp._transport._multicast;
            for (_z_transport_peer_multicast_slist_t *it = tp->_peers; it != NULL;
                 it = _z_transport_peer_multicast_slist_next(it)) {
                const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(it);

                z_owned_keyexpr_t peer_ke;
                z_internal_keyexpr_null(&peer_ke);
                build_pico_transport_0_peer_ke(&peer_ke, z_loan(kes.peers_ke), &peer->common._remote_zid);

                admin_space_query_reply_list_t *results;
                const admin_space_query_reply_t *reply =
                    run_exact_admin_space_query(z_loan(ss.s2), z_loan(peer_ke), &results);

                verify_peer_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                 peer->common._remote_whatami, true);

                admin_space_query_reply_list_free(&results);
                z_drop(z_move(peer_ke));
            }
            break;
        }

        case _Z_TRANSPORT_RAWETH_TYPE: {
            const _z_transport_multicast_t *tp = &session->_tp._transport._raweth;
            for (_z_transport_peer_multicast_slist_t *it = tp->_peers; it != NULL;
                 it = _z_transport_peer_multicast_slist_next(it)) {
                const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(it);

                z_owned_keyexpr_t peer_ke;
                z_internal_keyexpr_null(&peer_ke);
                build_pico_transport_0_peer_ke(&peer_ke, z_loan(kes.peers_ke), &peer->common._remote_zid);

                admin_space_query_reply_list_t *results;
                const admin_space_query_reply_t *reply =
                    run_exact_admin_space_query(z_loan(ss.s2), z_loan(peer_ke), &results);

                verify_peer_json(z_string_loan(&reply->payload), &peer->common._remote_zid,
                                 peer->common._remote_whatami, true);

                admin_space_query_reply_list_free(&results);
                z_drop(z_move(peer_ke));
            }
            break;
        }

        default:
            ASSERT_TRUE(false);
    }

    admin_space_test_keyexprs_clear(&kes);
    admin_space_test_sessions_close(&ss);
}

#if Z_FEATURE_CONNECTIVITY == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_LINK_TCP == 1 && \
    Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_PUBLICATION == 1
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

static void assert_no_sample(z_owned_fifo_handler_sample_t *handler) {
    for (unsigned i = 0; i < 5; ++i) {
        z_owned_sample_t sample;
        z_result_t res = z_fifo_handler_sample_try_recv(z_fifo_handler_sample_loan(handler), &sample);
        if (res == Z_CHANNEL_NODATA) {
            z_sleep_ms(100);
            continue;
        }

        if (res == _Z_RES_OK) {
            z_drop(z_move(sample));
        }
        ASSERT_TRUE(false);
    }
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

void test_admin_space_rfc_connectivity_query_and_events(void) {
    z_owned_session_t s1, s2, s3;
    open_listener_session(&s1, "tcp/127.0.0.1:7448");
    ASSERT_OK(zp_start_admin_space(z_session_loan_mut(&s1)));
    open_connector_session(&s2, "tcp/127.0.0.1:7448");

    z_id_t s1_zid = z_info_zid(z_session_loan(&s1));
    z_owned_string_t s1_zid_str;
    ASSERT_OK(z_id_to_string(&s1_zid, &s1_zid_str));

    char session_query_ke_str[256];
    int session_query_ke_len = snprintf(session_query_ke_str, sizeof(session_query_ke_str), "@/%.*s/%s/**",
                                        (int)z_string_len(z_string_loan(&s1_zid_str)),
                                        z_string_data(z_string_loan(&s1_zid_str)), _Z_KEYEXPR_SESSION);
    ASSERT_TRUE(session_query_ke_len > 0 && (size_t)session_query_ke_len < sizeof(session_query_ke_str));

    z_view_keyexpr_t session_query_ke;
    ASSERT_OK(z_view_keyexpr_from_str(&session_query_ke, session_query_ke_str));
    wait_admin_space_ready(z_session_loan(&s1), z_view_keyexpr_loan(&session_query_ke));

    admin_space_query_reply_list_t *results =
        run_admin_space_query(z_session_loan(&s2), z_view_keyexpr_loan(&session_query_ke));
    ASSERT_TRUE(admin_space_query_reply_list_len(results) == 0);
    admin_space_query_reply_list_free(&results);

    results = run_admin_space_query(z_session_loan(&s1), z_view_keyexpr_loan(&session_query_ke));
    ASSERT_TRUE(admin_space_query_reply_list_len(results) >= 2);
    bool saw_transport = false;
    bool saw_link = false;
    for (admin_space_query_reply_list_t *it = results; it != NULL; it = admin_space_query_reply_list_next(it)) {
        const admin_space_query_reply_t *reply = admin_space_query_reply_list_value(it);
        z_view_string_t key_view;
        ASSERT_OK(z_keyexpr_as_view_string(z_keyexpr_loan(&reply->ke), &key_view));
        const z_loaned_string_t *key = z_view_string_loan(&key_view);
        const char *k_start = z_string_data(key);
        const char *k_end = k_start + z_string_len(key);
        if (_z_strstr(k_start, k_end, "/session/transport/unicast/") != NULL &&
            _z_strstr(k_start, k_end, "/link/") == NULL) {
            saw_transport = true;
            assert_contains(z_string_loan(&reply->payload), "\"zid\"");
            assert_contains(z_string_loan(&reply->payload), "\"whatami\"");
            assert_contains(z_string_loan(&reply->payload), "\"is_qos\"");
            assert_contains(z_string_loan(&reply->payload), "\"is_shm\"");
        }
        if (_z_strstr(k_start, k_end, "/session/transport/unicast/") != NULL &&
            _z_strstr(k_start, k_end, "/link/") != NULL) {
            saw_link = true;
            assert_contains(z_string_loan(&reply->payload), "\"src\"");
            assert_contains(z_string_loan(&reply->payload), "\"dst\"");
            assert_contains(z_string_loan(&reply->payload), "\"group\":null");
            assert_contains(z_string_loan(&reply->payload), "\"mtu\"");
            assert_contains(z_string_loan(&reply->payload), "\"is_reliable\"");
            assert_contains(z_string_loan(&reply->payload), "\"is_streamed\"");
            assert_not_contains(z_string_loan(&reply->payload), "\"zid\"");
        }
    }
    ASSERT_TRUE(saw_transport);
    ASSERT_TRUE(saw_link);
    admin_space_query_reply_list_free(&results);

    char transport_sub_ke_str[256];
    int transport_sub_ke_len =
        snprintf(transport_sub_ke_str, sizeof(transport_sub_ke_str), "@/%.*s/%s/%s/*",
                 (int)z_string_len(z_string_loan(&s1_zid_str)), z_string_data(z_string_loan(&s1_zid_str)),
                 _Z_KEYEXPR_SESSION, _Z_KEYEXPR_TRANSPORT_UNICAST);
    ASSERT_TRUE(transport_sub_ke_len > 0 && (size_t)transport_sub_ke_len < sizeof(transport_sub_ke_str));

    char link_sub_ke_str[320];
    int link_sub_ke_len = snprintf(
        link_sub_ke_str, sizeof(link_sub_ke_str), "@/%.*s/%s/%s/*/%s/*", (int)z_string_len(z_string_loan(&s1_zid_str)),
        z_string_data(z_string_loan(&s1_zid_str)), _Z_KEYEXPR_SESSION, _Z_KEYEXPR_TRANSPORT_UNICAST, _Z_KEYEXPR_LINK);
    ASSERT_TRUE(link_sub_ke_len > 0 && (size_t)link_sub_ke_len < sizeof(link_sub_ke_str));
    z_drop(z_move(s1_zid_str));

    z_view_keyexpr_t transport_sub_ke;
    z_view_keyexpr_t link_sub_ke;
    ASSERT_OK(z_view_keyexpr_from_str(&transport_sub_ke, transport_sub_ke_str));
    ASSERT_OK(z_view_keyexpr_from_str(&link_sub_ke, link_sub_ke_str));

    z_owned_closure_sample_t transport_sub_closure;
    z_owned_fifo_handler_sample_t transport_sub_handler;
    ASSERT_OK(z_fifo_channel_sample_new(&transport_sub_closure, &transport_sub_handler, 8));
    z_owned_subscriber_t transport_sub;
    ASSERT_OK(z_declare_subscriber(z_session_loan(&s1), &transport_sub, z_view_keyexpr_loan(&transport_sub_ke),
                                   z_closure_sample_move(&transport_sub_closure), NULL));

    z_owned_closure_sample_t link_sub_closure;
    z_owned_fifo_handler_sample_t link_sub_handler;
    ASSERT_OK(z_fifo_channel_sample_new(&link_sub_closure, &link_sub_handler, 8));
    z_owned_subscriber_t link_sub;
    ASSERT_OK(z_declare_subscriber(z_session_loan(&s1), &link_sub, z_view_keyexpr_loan(&link_sub_ke),
                                   z_closure_sample_move(&link_sub_closure), NULL));

    z_owned_closure_sample_t remote_transport_sub_closure;
    z_owned_fifo_handler_sample_t remote_transport_sub_handler;
    ASSERT_OK(z_fifo_channel_sample_new(&remote_transport_sub_closure, &remote_transport_sub_handler, 8));
    z_owned_subscriber_t remote_transport_sub;
    ASSERT_OK(z_declare_subscriber(z_session_loan(&s2), &remote_transport_sub, z_view_keyexpr_loan(&transport_sub_ke),
                                   z_closure_sample_move(&remote_transport_sub_closure), NULL));

    z_owned_closure_sample_t remote_link_sub_closure;
    z_owned_fifo_handler_sample_t remote_link_sub_handler;
    ASSERT_OK(z_fifo_channel_sample_new(&remote_link_sub_closure, &remote_link_sub_handler, 8));
    z_owned_subscriber_t remote_link_sub;
    ASSERT_OK(z_declare_subscriber(z_session_loan(&s2), &remote_link_sub, z_view_keyexpr_loan(&link_sub_ke),
                                   z_closure_sample_move(&remote_link_sub_closure), NULL));

    open_connector_session(&s3, "tcp/127.0.0.1:7448");

    z_owned_string_t transport_put_payload;
    z_owned_string_t link_put_payload;
    wait_for_sample_kind(&transport_sub_handler, Z_SAMPLE_KIND_PUT, &transport_put_payload);
    wait_for_sample_kind(&link_sub_handler, Z_SAMPLE_KIND_PUT, &link_put_payload);
    assert_contains(z_string_loan(&transport_put_payload), "\"is_qos\"");
    assert_contains(z_string_loan(&transport_put_payload), "\"is_shm\"");
    assert_contains(z_string_loan(&link_put_payload), "\"group\":null");
    assert_not_contains(z_string_loan(&link_put_payload), "\"zid\"");
    z_drop(z_move(transport_put_payload));
    z_drop(z_move(link_put_payload));
    assert_no_sample(&remote_transport_sub_handler);
    assert_no_sample(&remote_link_sub_handler);

    close_session_with_tasks(&s3);
    wait_for_sample_kind(&link_sub_handler, Z_SAMPLE_KIND_DELETE, NULL);
    wait_for_sample_kind(&transport_sub_handler, Z_SAMPLE_KIND_DELETE, NULL);
    assert_no_sample(&remote_transport_sub_handler);
    assert_no_sample(&remote_link_sub_handler);

    ASSERT_OK(z_undeclare_subscriber(z_subscriber_move(&transport_sub)));
    ASSERT_OK(z_undeclare_subscriber(z_subscriber_move(&link_sub)));
    ASSERT_OK(z_undeclare_subscriber(z_subscriber_move(&remote_transport_sub)));
    ASSERT_OK(z_undeclare_subscriber(z_subscriber_move(&remote_link_sub)));
    z_drop(z_move(transport_sub_handler));
    z_drop(z_move(link_sub_handler));
    z_drop(z_move(remote_transport_sub_handler));
    z_drop(z_move(remote_link_sub_handler));

    ASSERT_OK(zp_stop_admin_space(z_session_loan_mut(&s1)));
    close_session_with_tasks(&s2);
    close_session_with_tasks(&s1);
}
#endif

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_start_stop_admin_space();
    test_auto_start_admin_space();
    test_admin_space_query_fails_when_not_running();
    test_admin_space_general_query_succeeds();
    test_admin_space_pico_endpoint_succeeds();
    test_admin_space_session_endpoint_succeeds();
    test_admin_space_transports_endpoint_succeeds();
    test_admin_space_transport_0_endpoint_succeeds();
    test_admin_space_transport_0_peers_endpoint_succeeds();
    test_admin_space_transport_0_peer_endpoints_succeeds();
#if Z_FEATURE_CONNECTIVITY == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_LINK_TCP == 1 &&  \
    Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_LOCAL_QUERYABLE == 1 && \
    Z_FEATURE_LOCAL_SUBSCRIBER == 1
    test_admin_space_rfc_connectivity_query_and_events();
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
