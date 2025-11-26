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

#include "zenoh-pico.h"

#undef NDEBUG
#include <assert.h>

#if Z_FEATURE_QUERY == 1 && Z_FEATURE_QUERYABLE == 1 && Z_FEATURE_LOCAL_QUERYABLE == 1 && Z_FEATURE_MULTI_THREAD == 1

static const char *QUERYABLE_EXPR = "zenoh-pico/locality/query";

void test_queryable(z_locality_t get_allowed_destination, z_locality_t q1_allowed_origin,
                    z_locality_t q2_allowed_origin) {
    printf("Testing: %d %d %d\n", get_allowed_destination, q1_allowed_origin, q2_allowed_origin);
    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, QUERYABLE_EXPR);

    assert(z_open(&s1, z_config_move(&c1), NULL) == Z_OK);
    assert(z_open(&s2, z_config_move(&c2), NULL) == Z_OK);

    assert(zp_start_read_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_read_task(z_loan_mut(s2), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s2), NULL) == Z_OK);

    z_owned_queryable_t queryable1, queryable2;
    z_queryable_options_t opts1, opts2;
    z_queryable_options_default(&opts1);
    z_queryable_options_default(&opts2);
    opts1.allowed_origin = q1_allowed_origin;
    opts2.allowed_origin = q2_allowed_origin;

    z_owned_closure_query_t query_callback1, query_callback2;
    z_owned_fifo_handler_query_t query_handler1, query_handler2;
    z_fifo_channel_query_new(&query_callback1, &query_handler1, 16);
    z_declare_queryable(z_session_loan(&s1), &queryable1, z_view_keyexpr_loan(&ke),
                        z_closure_query_move(&query_callback1), &opts1);
    z_fifo_channel_query_new(&query_callback2, &query_handler2, 16);
    z_declare_queryable(z_session_loan(&s2), &queryable2, z_view_keyexpr_loan(&ke),
                        z_closure_query_move(&query_callback2), &opts2);

    z_owned_closure_reply_t reply_callback;
    z_owned_fifo_handler_reply_t reply_handler;
    z_fifo_channel_reply_new(&reply_callback, &reply_handler, 16);

    z_get_options_t opts;
    z_get_options_default(&opts);
    opts.allowed_destination = get_allowed_destination;
    opts.consolidation = z_query_consolidation_none();

    z_sleep_s(1);

    z_get(z_session_loan(&s1), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&reply_callback), &opts);

    z_sleep_s(1);

    z_owned_query_t q1, q2;

    z_result_t ret1 = z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler1), &q1);
    z_result_t ret2 = z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler2), &q2);
    bool expect_1 = false;
    bool expect_2 = false;
    size_t expect_responses = 0;
    if (_z_locality_allows_local(get_allowed_destination) && _z_locality_allows_local(q1_allowed_origin)) {
        assert(ret1 == Z_OK);
        z_owned_bytes_t payload1;
        z_bytes_copy_from_str(&payload1, "1");
        z_query_reply(z_query_loan(&q1), z_view_keyexpr_loan(&ke), z_bytes_move(&payload1), NULL);
        z_query_drop(z_query_move(&q1));
        expect_1 = true;
        expect_responses++;
    } else {
        assert(ret1 != Z_OK);
    }
    if (_z_locality_allows_remote(get_allowed_destination) && _z_locality_allows_remote(q2_allowed_origin)) {
        assert(ret2 == Z_OK);
        z_owned_bytes_t payload2;
        z_bytes_copy_from_str(&payload2, "2");
        z_query_reply(z_query_loan(&q2), z_view_keyexpr_loan(&ke), z_bytes_move(&payload2), NULL);
        z_query_drop(z_query_move(&q2));
        expect_2 = true;
        expect_responses++;
    } else {
        assert(ret2 != Z_OK);
    }

    bool found_1 = false;
    bool found_2 = false;
    size_t reply_count = 0;

    z_sleep_s(1);

    z_owned_reply_t reply;
    for (z_result_t res = z_fifo_handler_reply_recv(z_fifo_handler_reply_loan(&reply_handler), &reply);
         res != Z_CHANNEL_DISCONNECTED;
         res = z_fifo_handler_reply_recv(z_fifo_handler_reply_loan(&reply_handler), &reply)) {
        assert(z_reply_is_ok(z_loan(reply)));
        const z_loaned_sample_t *sample = z_reply_ok(z_loan(reply));
        z_view_string_t keystr;
        z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
        z_owned_string_t replystr;
        z_bytes_to_string(z_sample_payload(sample), &replystr);
        if (strncmp(z_string_data(z_loan(replystr)), "1", 1) == 0) {
            found_1 = true;
        }
        if (strncmp(z_string_data(z_loan(replystr)), "2", 1) == 0) {
            found_2 = true;
        }
        reply_count++;
        assert(strncmp(z_string_data(z_loan(keystr)), QUERYABLE_EXPR, strlen(QUERYABLE_EXPR)) == 0);
        z_reply_drop(z_reply_move(&reply));
        z_string_drop(z_string_move(&replystr));
    }

    assert(reply_count == expect_responses);
    assert(found_1 == expect_1);
    assert(found_2 == expect_2);

    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&reply_handler));
    z_fifo_handler_query_drop(z_fifo_handler_query_move(&query_handler1));
    z_fifo_handler_query_drop(z_fifo_handler_query_move(&query_handler2));

    zp_stop_read_task(z_loan_mut(s1));
    zp_stop_read_task(z_loan_mut(s2));
    zp_stop_lease_task(z_loan_mut(s1));
    zp_stop_lease_task(z_loan_mut(s2));

    z_queryable_drop(z_queryable_move(&queryable1));
    z_queryable_drop(z_queryable_move(&queryable2));
    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    z_locality_t localities[] = {Z_LOCALITY_ANY, Z_LOCALITY_REMOTE, Z_LOCALITY_SESSION_LOCAL};
    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            for (size_t k = 0; k < 3; k++) {
                test_queryable(localities[i], localities[j], localities[k]);
            }
        }
    }
}

#else
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
}
#endif
