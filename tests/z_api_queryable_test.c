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

#include "zenoh-pico.h"
#include "zenoh-pico/api/macros.h"

#undef NDEBUG
#include <assert.h>

#if Z_FEATURE_QUERYABLE

void test_queryable_keyexpr(void) {
    printf("Testing queryable key expressions...\n");
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");

    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    z_view_keyexpr_t ke1;
    z_view_keyexpr_from_str(&ke1, "test/queryable_keyexpr");

    z_owned_queryable_t q1;
    z_owned_closure_query_t cb1;
    z_owned_fifo_handler_query_t h1;
    z_fifo_channel_query_new(&cb1, &h1, 16);
    z_declare_queryable(z_loan(s), &q1, z_loan(ke1), z_move(cb1), NULL);

    z_view_string_t sv1;
    z_keyexpr_as_view_string(z_queryable_keyexpr(z_loan(q1)), &sv1);
    assert(strncmp("test/queryable_keyexpr", z_string_data(z_loan(sv1)), z_string_len(z_loan(sv1))) == 0);
    z_drop(z_move(q1));
    z_drop(z_move(h1));

    z_view_keyexpr_t ke2;
    z_owned_keyexpr_t ke2_declared;
    z_view_keyexpr_from_str(&ke2, "test/queryable_declared_keyexpr");
    z_declare_keyexpr(z_loan(s), &ke2_declared, z_loan(ke2));

    z_owned_queryable_t q2;
    z_owned_closure_query_t cb2;
    z_owned_fifo_handler_query_t h2;
    z_fifo_channel_query_new(&cb2, &h2, 16);
    z_declare_queryable(z_loan(s), &q2, z_loan(ke2_declared), z_move(cb2), NULL);

    z_view_string_t sv2;
    z_keyexpr_as_view_string(z_queryable_keyexpr(z_loan(q2)), &sv2);
    assert(strncmp("test/queryable_declared_keyexpr", z_string_data(z_loan(sv2)), z_string_len(z_loan(sv2))) == 0);
    z_drop(z_move(ke2_declared));
    z_drop(z_move(q2));
    z_drop(z_move(h2));

    z_drop(z_move(s));
}

void open_sessions(z_owned_session_t *s, z_owned_session_t *s2, bool local) {
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");

    if (z_open(s, z_move(config), NULL) < 0) {
        printf("Unable to open session!\n");
        exit(-1);
    }

    if (local) {
        *s2 = *s;
    } else {
        z_owned_config_t config2;
        z_config_default(&config2);
        zp_config_insert(z_loan_mut(config2), Z_CONFIG_MODE_KEY, "client");
        if (z_open(s2, z_move(config2), NULL) < 0) {
            printf("Unable to open second session for local queryable test!\n");
            z_drop(z_move(*s));
            exit(-1);
        }
    }
}

void test_get_accept_replies(bool local) {
    printf("Testing get accept_replies options local = %d...\n", local);
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");

    z_owned_session_t s, s2;
    open_sessions(&s, &s2, local);

    // Declare a queryable on a wildcard key expression
    z_view_keyexpr_t queryable_ke;
    z_view_keyexpr_from_str(&queryable_ke, "test/accept_replies/**");

    z_owned_queryable_t q;
    z_owned_closure_query_t cb;
    z_owned_fifo_handler_query_t h;
    z_fifo_channel_query_new(&cb, &h, 16);
    z_declare_queryable(z_loan(s), &q, z_loan(queryable_ke), z_move(cb), NULL);
    assert(z_internal_check(q));
    z_sleep_s(1);

    // --- Test 1: Z_REPLY_KEYEXPR_ANY ---
    // With accept_replies = Z_REPLY_KEYEXPR_ANY, the queryable should be able
    // to reply on any key expression, even one not matching the query key
    {
        z_owned_fifo_handler_reply_t reply_handler;
        z_owned_closure_reply_t reply_cb;
        z_fifo_channel_reply_new(&reply_cb, &reply_handler, 16);

        z_view_keyexpr_t get_ke;
        z_view_keyexpr_from_str(&get_ke, "test/accept_replies/a");

        z_get_options_t get_opts;
        z_get_options_default(&get_opts);
        get_opts.accept_replies = Z_REPLY_KEYEXPR_ANY;

        z_get(z_loan(s2), z_loan(get_ke), "", z_move(reply_cb), &get_opts);
        z_sleep_s(1);

        // Receive the incoming query
        z_owned_query_t query;
        assert(z_recv(z_loan(h), &query) == _Z_RES_OK);
        assert(z_query_accepts_replies(z_loan(query)) == Z_REPLY_KEYEXPR_ANY);

        // Reply on a different key expression (not intersecting with query key)
        z_view_keyexpr_t reply_ke;
        z_view_keyexpr_from_str(&reply_ke, "test/accept_replies/b");

        z_owned_bytes_t payload;
        z_bytes_from_static_str(&payload, "reply_any");

        z_query_reply_options_t reply_opts;
        z_query_reply_options_default(&reply_opts);
        z_result_t ret = z_query_reply(z_loan(query), z_loan(reply_ke), z_move(payload), &reply_opts);
        assert(ret == _Z_RES_OK);
        z_drop(z_move(query));
        z_sleep_s(1);
        // The reply should arrive and carry the replied key expression
        z_owned_reply_t reply;
        assert(z_recv(z_loan(reply_handler), &reply) == _Z_RES_OK);
        assert(z_reply_is_ok(z_loan(reply)));

        z_view_string_t reply_key_str;
        z_keyexpr_as_view_string(z_sample_keyexpr(z_reply_ok(z_loan(reply))), &reply_key_str);
        assert(strncmp("test/accept_replies/b", z_string_data(z_loan(reply_key_str)),
                       z_string_len(z_loan(reply_key_str))) == 0);

        z_drop(z_move(reply));
        z_drop(z_move(reply_handler));
    }

    // --- Test 2: Z_REPLY_KEYEXPR_MATCHING_QUERY (default) ---
    // With accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY, the queryable must
    // reply on a key expression that intersects with the query key expression.
    // A reply on a matching key should be accepted; one on a non-matching key should be rejected.
    {
        // 2a: reply on a key expression matching the query key — should succeed
        z_owned_fifo_handler_reply_t reply_handler;
        z_owned_closure_reply_t reply_cb;
        z_fifo_channel_reply_new(&reply_cb, &reply_handler, 16);

        z_view_keyexpr_t get_ke;
        z_view_keyexpr_from_str(&get_ke, "test/accept_replies/c");

        z_get_options_t get_opts;
        z_get_options_default(&get_opts);
        get_opts.accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY;

        z_get(z_loan(s2), z_loan(get_ke), "", z_move(reply_cb), &get_opts);
        z_sleep_s(1);
        z_owned_query_t query;
        assert(z_recv(z_loan(h), &query) == _Z_RES_OK);
        assert(z_query_accepts_replies(z_loan(query)) == Z_REPLY_KEYEXPR_MATCHING_QUERY);

        z_view_keyexpr_t reply_ke_matching;
        z_view_keyexpr_from_str(&reply_ke_matching, "test/accept_replies/c");

        z_owned_bytes_t payload_matching;
        z_bytes_from_static_str(&payload_matching, "reply_matching");

        z_query_reply_options_t reply_opts;
        z_query_reply_options_default(&reply_opts);
        z_result_t ret = z_query_reply(z_loan(query), z_loan(reply_ke_matching), z_move(payload_matching), &reply_opts);
        assert(ret == _Z_RES_OK);
        z_drop(z_move(query));

        z_sleep_s(1);
        z_owned_reply_t reply;
        assert(z_recv(z_loan(reply_handler), &reply) == _Z_RES_OK);
        assert(z_reply_is_ok(z_loan(reply)));

        z_view_string_t reply_key_str;
        z_keyexpr_as_view_string(z_sample_keyexpr(z_reply_ok(z_loan(reply))), &reply_key_str);
        assert(strncmp("test/accept_replies/c", z_string_data(z_loan(reply_key_str)),
                       z_string_len(z_loan(reply_key_str))) == 0);

        z_drop(z_move(reply));
        z_drop(z_move(reply_handler));

        // 2b: reply on a key expression NOT matching the query key — should be rejected
        z_owned_fifo_handler_reply_t reply_handler2;
        z_owned_closure_reply_t reply_cb2;
        z_fifo_channel_reply_new(&reply_cb2, &reply_handler2, 16);

        z_view_keyexpr_t get_ke2;
        z_view_keyexpr_from_str(&get_ke2, "test/accept_replies/d");

        z_get_options_t get_opts2;
        z_get_options_default(&get_opts2);
        get_opts2.accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY;

        z_get(z_loan(s2), z_loan(get_ke2), "", z_move(reply_cb2), &get_opts2);
        z_sleep_s(1);

        z_owned_query_t query2;
        assert(z_recv(z_loan(h), &query2) == _Z_RES_OK);
        assert(z_query_accepts_replies(z_loan(query2)) == Z_REPLY_KEYEXPR_MATCHING_QUERY);

        z_view_keyexpr_t reply_ke_nonmatching;
        z_view_keyexpr_from_str(&reply_ke_nonmatching, "test/other/key");

        z_owned_bytes_t payload_nonmatching;
        z_bytes_from_static_str(&payload_nonmatching, "reply_nonmatching");

        ret = z_query_reply(z_loan(query2), z_loan(reply_ke_nonmatching), z_move(payload_nonmatching), &reply_opts);
        assert(ret != _Z_RES_OK);

        z_drop(z_move(query2));
        z_drop(z_move(reply_handler2));
    }

    z_drop(z_move(q));
    z_drop(z_move(h));
    z_drop(z_move(s));
    if (!local) {
        z_drop(z_move(s2));
    }
}

void test_querier_accept_replies(bool local) {
    printf("Testing querier accept_replies options local = %d...\n", local);
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");

    z_owned_session_t s, s2;
    open_sessions(&s, &s2, local);

    // Declare a queryable on a wildcard key expression to handle incoming queries
    z_view_keyexpr_t queryable_ke;
    z_view_keyexpr_from_str(&queryable_ke, "test/querier_accept/**");

    z_owned_queryable_t q;
    z_owned_closure_query_t cb;
    z_owned_fifo_handler_query_t h;
    z_fifo_channel_query_new(&cb, &h, 16);
    z_declare_queryable(z_loan(s), &q, z_loan(queryable_ke), z_move(cb), NULL);
    assert(z_internal_check(q));
    z_sleep_s(1);

    // --- Test 1: Z_REPLY_KEYEXPR_ANY ---
    // With accept_replies = Z_REPLY_KEYEXPR_ANY set on the querier, the queryable
    // should be able to reply on any key expression, even one not matching the query key
    {
        z_view_keyexpr_t querier_ke;
        z_view_keyexpr_from_str(&querier_ke, "test/querier_accept/a");

        z_querier_options_t querier_opts;
        z_querier_options_default(&querier_opts);
        querier_opts.accept_replies = Z_REPLY_KEYEXPR_ANY;

        z_owned_querier_t querier;
        z_declare_querier(z_loan(s2), &querier, z_loan(querier_ke), &querier_opts);
        assert(z_internal_check(querier));
        z_sleep_s(1);

        z_owned_fifo_handler_reply_t reply_handler;
        z_owned_closure_reply_t reply_cb;
        z_fifo_channel_reply_new(&reply_cb, &reply_handler, 16);

        z_querier_get_options_t get_opts;
        z_querier_get_options_default(&get_opts);
        z_querier_get(z_loan(querier), "", z_move(reply_cb), &get_opts);
        z_sleep_s(1);

        // Receive the incoming query on the queryable side
        z_owned_query_t query;
        assert(z_recv(z_loan(h), &query) == _Z_RES_OK);
        assert(z_query_accepts_replies(z_loan(query)) == Z_REPLY_KEYEXPR_ANY);

        // Reply on a different key expression (not intersecting with the query key)
        z_view_keyexpr_t reply_ke;
        z_view_keyexpr_from_str(&reply_ke, "test/querier_accept/b");

        z_owned_bytes_t payload;
        z_bytes_from_static_str(&payload, "reply_any");

        z_query_reply_options_t reply_opts;
        z_query_reply_options_default(&reply_opts);
        z_result_t ret = z_query_reply(z_loan(query), z_loan(reply_ke), z_move(payload), &reply_opts);
        assert(ret == _Z_RES_OK);
        z_drop(z_move(query));
        z_sleep_s(1);

        // The reply should arrive and carry the replied key expression
        z_owned_reply_t reply;
        assert(z_recv(z_loan(reply_handler), &reply) == _Z_RES_OK);
        assert(z_reply_is_ok(z_loan(reply)));

        z_view_string_t reply_key_str;
        z_keyexpr_as_view_string(z_sample_keyexpr(z_reply_ok(z_loan(reply))), &reply_key_str);
        assert(strncmp("test/querier_accept/b", z_string_data(z_loan(reply_key_str)),
                       z_string_len(z_loan(reply_key_str))) == 0);

        z_drop(z_move(reply));
        z_drop(z_move(reply_handler));
        z_drop(z_move(querier));
    }

    // --- Test 2: Z_REPLY_KEYEXPR_MATCHING_QUERY (default) ---
    // With accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY set on the querier,
    // the queryable must reply on a key expression that intersects with the query key.
    // A reply on a matching key should be accepted; one on a non-matching key should be rejected.
    {
        // 2a: reply on a key expression matching the querier key — should succeed
        z_view_keyexpr_t querier_ke;
        z_view_keyexpr_from_str(&querier_ke, "test/querier_accept/c");

        z_querier_options_t querier_opts;
        z_querier_options_default(&querier_opts);
        querier_opts.accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY;

        z_owned_querier_t querier;
        z_declare_querier(z_loan(s2), &querier, z_loan(querier_ke), &querier_opts);
        assert(z_internal_check(querier));
        z_sleep_s(1);
        z_owned_fifo_handler_reply_t reply_handler;
        z_owned_closure_reply_t reply_cb;
        z_fifo_channel_reply_new(&reply_cb, &reply_handler, 16);

        z_querier_get_options_t get_opts;
        z_querier_get_options_default(&get_opts);
        z_querier_get(z_loan(querier), "", z_move(reply_cb), &get_opts);
        z_sleep_s(1);

        z_owned_query_t query;
        assert(z_recv(z_loan(h), &query) == _Z_RES_OK);
        assert(z_query_accepts_replies(z_loan(query)) == Z_REPLY_KEYEXPR_MATCHING_QUERY);

        // Reply on a key matching the querier key expression
        z_view_keyexpr_t reply_ke_matching;
        z_view_keyexpr_from_str(&reply_ke_matching, "test/querier_accept/c");

        z_owned_bytes_t payload_matching;
        z_bytes_from_static_str(&payload_matching, "reply_matching");

        z_query_reply_options_t reply_opts;
        z_query_reply_options_default(&reply_opts);
        z_result_t ret = z_query_reply(z_loan(query), z_loan(reply_ke_matching), z_move(payload_matching), &reply_opts);
        assert(ret == _Z_RES_OK);
        z_drop(z_move(query));

        z_sleep_s(1);
        z_owned_reply_t reply;
        assert(z_recv(z_loan(reply_handler), &reply) == _Z_RES_OK);
        assert(z_reply_is_ok(z_loan(reply)));

        z_view_string_t reply_key_str;
        z_keyexpr_as_view_string(z_sample_keyexpr(z_reply_ok(z_loan(reply))), &reply_key_str);
        assert(strncmp("test/querier_accept/c", z_string_data(z_loan(reply_key_str)),
                       z_string_len(z_loan(reply_key_str))) == 0);

        z_drop(z_move(reply));
        z_drop(z_move(reply_handler));
        z_drop(z_move(querier));

        // 2b: reply on a key expression NOT matching the querier key — should be rejected
        z_view_keyexpr_t querier_ke2;
        z_view_keyexpr_from_str(&querier_ke2, "test/querier_accept/d");

        z_querier_options_t querier_opts2;
        z_querier_options_default(&querier_opts2);
        querier_opts2.accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY;

        z_owned_querier_t querier2;
        z_declare_querier(z_loan(s), &querier2, z_loan(querier_ke2), &querier_opts2);
        assert(z_internal_check(querier2));
        z_sleep_s(1);

        z_owned_fifo_handler_reply_t reply_handler2;
        z_owned_closure_reply_t reply_cb2;
        z_fifo_channel_reply_new(&reply_cb2, &reply_handler2, 16);

        z_querier_get_options_t get_opts2;
        z_querier_get_options_default(&get_opts2);
        z_querier_get(z_loan(querier2), "", z_move(reply_cb2), &get_opts2);
        z_sleep_s(1);

        z_owned_query_t query2;
        assert(z_recv(z_loan(h), &query2) == _Z_RES_OK);
        assert(z_query_accepts_replies(z_loan(query2)) == Z_REPLY_KEYEXPR_MATCHING_QUERY);

        // Reply on a key expression not intersecting with the querier key
        z_view_keyexpr_t reply_ke_nonmatching;
        z_view_keyexpr_from_str(&reply_ke_nonmatching, "test/other/key");

        z_owned_bytes_t payload_nonmatching;
        z_bytes_from_static_str(&payload_nonmatching, "reply_nonmatching");

        ret = z_query_reply(z_loan(query2), z_loan(reply_ke_nonmatching), z_move(payload_nonmatching), &reply_opts);
        assert(ret != _Z_RES_OK);

        z_drop(z_move(query2));
        z_drop(z_move(reply_handler2));
        z_drop(z_move(querier2));
    }

    z_drop(z_move(q));
    z_drop(z_move(h));
    z_drop(z_move(s));
    if (!local) {
        z_drop(z_move(s2));
    }
}

int main(void) {
    test_queryable_keyexpr();
    test_get_accept_replies(false);
    test_querier_accept_replies(false);
#if Z_FEATURE_LOCAL_QUERYABLE == 1
    test_get_accept_replies(true);
    test_querier_accept_replies(true);
#endif
    return 0;
}
#else
int main(void) {}
#endif
