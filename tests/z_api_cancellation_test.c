//
// Copyright (c) 2024 ZettaScale Technology
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
#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/liveliness.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#undef NDEBUG
#include <assert.h>

#if Z_FEATURE_QUERY == 1 && Z_FEATURE_MULTI_THREAD == 1 && defined(Z_FEATURE_UNSTABLE_API)

void on_reply(z_loaned_reply_t* reply, void* arg) {
    uint32_t* t = (uint32_t*)arg;
    z_sleep_s(3);
    (*t) += 1;
    _ZP_UNUSED(reply);
}

void on_drop(void* arg) {
    uint32_t* t = (uint32_t*)arg;
    z_sleep_s(3);
    (*t) += 2;
}

void test_cancel_get(void) {
    printf("test_cancel_get\n");
    const char* query_expr = "zenoh-pico/query/cancellation/test";

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, query_expr);

    assert(z_open(&s1, z_config_move(&c1), NULL) == Z_OK);
    assert(z_open(&s2, z_config_move(&c2), NULL) == Z_OK);

    assert(zp_start_read_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_read_task(z_loan_mut(s2), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s2), NULL) == Z_OK);

    z_owned_queryable_t queryable;
    z_owned_closure_query_t query_callback;
    z_owned_fifo_handler_query_t query_handler;
    z_fifo_channel_query_new(&query_callback, &query_handler, 16);
    z_declare_queryable(z_session_loan(&s1), &queryable, z_view_keyexpr_loan(&ke),
                        z_closure_query_move(&query_callback), NULL);
    z_sleep_s(2);

    z_owned_closure_reply_t reply_callback;
    z_owned_fifo_handler_reply_t reply_handler;
    z_fifo_channel_reply_new(&reply_callback, &reply_handler, 16);

    printf("Check cancel removes callback\n");
    z_owned_cancellation_token_t ct, ct_clone;
    assert(z_cancellation_token_new(&ct) == Z_OK);
    assert(!z_cancellation_token_is_cancelled(z_cancellation_token_loan(&ct)));
    assert(z_cancellation_token_clone(&ct_clone, z_cancellation_token_loan(&ct)) == Z_OK);
    z_get_options_t opts;
    z_get_options_default(&opts);
    opts.cancellation_token = z_cancellation_token_move(&ct_clone);
    z_get(z_session_loan(&s2), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&reply_callback), &opts);
    z_sleep_s(1);
    assert(z_cancellation_token_cancel(z_cancellation_token_loan_mut(&ct)) == Z_OK);
    assert(z_cancellation_token_is_cancelled(z_cancellation_token_loan(&ct)));
    z_cancellation_token_drop(z_cancellation_token_move(&ct));

    z_owned_reply_t reply;
    assert(z_fifo_handler_reply_try_recv(z_fifo_handler_reply_loan(&reply_handler), &reply) == Z_CHANNEL_DISCONNECTED);
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&reply_handler));

    z_owned_query_t q;
    assert(z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler), &q) == Z_OK);
    z_query_drop(z_query_move(&q));
    z_sleep_s(1);

    printf("Check cancel blocks until callback is finished\n");
    uint32_t t = 0;
    assert(z_cancellation_token_new(&ct) == Z_OK);
    assert(z_cancellation_token_clone(&ct_clone, z_cancellation_token_loan(&ct)) == Z_OK);
    z_get_options_default(&opts);
    opts.cancellation_token = z_cancellation_token_move(&ct_clone);
    z_closure_reply(&reply_callback, on_reply, on_drop, (void*)&t);

    z_get(z_session_loan(&s2), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&reply_callback), &opts);
    z_sleep_s(1);
    assert(z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler), &q) == Z_OK);

    z_owned_bytes_t b;
    z_bytes_from_static_str(&b, "ok");
    z_query_reply(z_query_loan(&q), z_query_keyexpr(z_query_loan(&q)), z_bytes_move(&b), NULL);
    z_query_drop(z_query_move(&q));
    z_sleep_s(1);
    assert(z_cancellation_token_cancel(z_cancellation_token_loan_mut(&ct)) == Z_OK);
    assert(t == 3);

    printf("Check cancelled token does not send a query\n");
    z_fifo_channel_reply_new(&reply_callback, &reply_handler, 16);
    assert(z_cancellation_token_clone(&ct_clone, z_cancellation_token_loan(&ct)) == Z_OK);
    z_get_options_default(&opts);
    opts.cancellation_token = z_cancellation_token_move(&ct_clone);
    z_get(z_session_loan(&s2), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&reply_callback), &opts);
    assert(z_fifo_handler_reply_try_recv(z_fifo_handler_reply_loan(&reply_handler), &reply) == Z_CHANNEL_DISCONNECTED);
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&reply_handler));

    z_sleep_s(1);
    assert(z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler), &q) == Z_CHANNEL_NODATA);

    z_fifo_handler_query_drop(z_fifo_handler_query_move(&query_handler));

    z_cancellation_token_drop(z_cancellation_token_move(&ct));
    z_queryable_drop(z_queryable_move(&queryable));
    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

void test_cancel_querier_get(void) {
    printf("test_cancel_querier_get\n");
    const char* query_expr = "zenoh-pico/querier/cancellation/test";

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, query_expr);

    assert(z_open(&s1, z_config_move(&c1), NULL) == Z_OK);
    assert(z_open(&s2, z_config_move(&c2), NULL) == Z_OK);

    assert(zp_start_read_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_read_task(z_loan_mut(s2), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s2), NULL) == Z_OK);

    z_owned_queryable_t queryable;
    z_owned_closure_query_t query_callback;
    z_owned_fifo_handler_query_t query_handler;
    z_fifo_channel_query_new(&query_callback, &query_handler, 16);
    z_declare_queryable(z_session_loan(&s1), &queryable, z_view_keyexpr_loan(&ke),
                        z_closure_query_move(&query_callback), NULL);

    z_owned_querier_t querier;
    z_declare_querier(z_session_loan(&s2), &querier, z_view_keyexpr_loan(&ke), NULL);
    z_sleep_s(2);

    z_owned_closure_reply_t reply_callback;
    z_owned_fifo_handler_reply_t reply_handler;
    z_fifo_channel_reply_new(&reply_callback, &reply_handler, 16);

    printf("Check cancel removes callback\n");
    z_owned_cancellation_token_t ct, ct_clone;
    assert(z_cancellation_token_new(&ct) == Z_OK);
    assert(!z_cancellation_token_is_cancelled(z_cancellation_token_loan(&ct)));
    assert(z_cancellation_token_clone(&ct_clone, z_cancellation_token_loan(&ct)) == Z_OK);
    z_querier_get_options_t opts;
    z_querier_get_options_default(&opts);
    opts.cancellation_token = z_cancellation_token_move(&ct_clone);
    z_querier_get(z_querier_loan(&querier), "", z_closure_reply_move(&reply_callback), &opts);
    z_sleep_s(1);

    assert(z_cancellation_token_cancel(z_cancellation_token_loan_mut(&ct)) == Z_OK);
    assert(z_cancellation_token_is_cancelled(z_cancellation_token_loan(&ct)));
    z_cancellation_token_drop(z_cancellation_token_move(&ct));

    z_owned_reply_t reply;
    assert(z_fifo_handler_reply_try_recv(z_fifo_handler_reply_loan(&reply_handler), &reply) == Z_CHANNEL_DISCONNECTED);
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&reply_handler));

    z_owned_query_t q;
    assert(z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler), &q) == Z_OK);
    z_query_drop(z_query_move(&q));
    z_sleep_s(1);

    printf("Check cancel blocks until callback is finished\n");
    uint32_t t = 0;
    assert(z_cancellation_token_new(&ct) == Z_OK);
    assert(z_cancellation_token_clone(&ct_clone, z_cancellation_token_loan(&ct)) == Z_OK);
    z_querier_get_options_default(&opts);
    opts.cancellation_token = z_cancellation_token_move(&ct_clone);
    z_closure_reply(&reply_callback, on_reply, on_drop, (void*)&t);

    z_querier_get(z_querier_loan(&querier), "", z_closure_reply_move(&reply_callback), &opts);
    z_sleep_s(1);
    assert(z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler), &q) == Z_OK);

    z_owned_bytes_t b;
    z_bytes_from_static_str(&b, "ok");
    z_query_reply(z_query_loan(&q), z_query_keyexpr(z_query_loan(&q)), z_bytes_move(&b), NULL);
    z_query_drop(z_query_move(&q));
    z_sleep_s(1);
    assert(z_cancellation_token_cancel(z_cancellation_token_loan_mut(&ct)) == Z_OK);
    assert(t == 3);

    printf("Check cancelled token does not send a query\n");
    z_fifo_channel_reply_new(&reply_callback, &reply_handler, 16);
    assert(z_cancellation_token_clone(&ct_clone, z_cancellation_token_loan(&ct)) == Z_OK);
    z_querier_get_options_default(&opts);
    opts.cancellation_token = z_cancellation_token_move(&ct_clone);
    z_querier_get(z_querier_loan(&querier), "", z_closure_reply_move(&reply_callback), &opts);
    assert(z_fifo_handler_reply_try_recv(z_fifo_handler_reply_loan(&reply_handler), &reply) == Z_CHANNEL_DISCONNECTED);
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&reply_handler));

    z_sleep_s(1);
    assert(z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler), &q) == Z_CHANNEL_NODATA);

    z_fifo_handler_query_drop(z_fifo_handler_query_move(&query_handler));

    z_cancellation_token_drop(z_cancellation_token_move(&ct));
    z_queryable_drop(z_queryable_move(&queryable));
    z_querier_drop(z_querier_move(&querier));
    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

#if Z_FEATURE_LIVELINESS == 1
void test_liveliness_get(void) {
    printf("test_cancel_liveliness_get\n");
    const char* query_expr = "zenoh-pico/liveliness_query/cancellation/test";

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, query_expr);

    assert(z_open(&s1, z_config_move(&c1), NULL) == Z_OK);
    assert(z_open(&s2, z_config_move(&c2), NULL) == Z_OK);

    assert(zp_start_read_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_read_task(z_loan_mut(s2), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s1), NULL) == Z_OK);
    assert(zp_start_lease_task(z_loan_mut(s2), NULL) == Z_OK);

    z_owned_liveliness_token_t token;
    z_liveliness_declare_token(z_session_loan(&s1), &token, z_view_keyexpr_loan(&ke), NULL);
    z_sleep_s(2);

    z_owned_closure_reply_t reply_callback;

    printf("Check cancel blocks until callback is finished\n");
    z_owned_cancellation_token_t ct, ct_clone;
    uint32_t t = 0;
    assert(z_cancellation_token_new(&ct) == Z_OK);
    assert(z_cancellation_token_clone(&ct_clone, z_cancellation_token_loan(&ct)) == Z_OK);
    z_liveliness_get_options_t opts;
    z_liveliness_get_options_default(&opts);
    opts.cancellation_token = z_cancellation_token_move(&ct_clone);
    z_closure_reply(&reply_callback, on_reply, on_drop, (void*)&t);
    z_liveliness_get(z_session_loan(&s2), z_view_keyexpr_loan(&ke), z_closure_reply_move(&reply_callback), &opts);
    z_sleep_s(1);
    assert(z_cancellation_token_cancel(z_cancellation_token_loan_mut(&ct)) == Z_OK);
    assert(t == 3);

    printf("Check cancelled token does not send a query\n");
    z_owned_fifo_handler_reply_t reply_handler;
    z_fifo_channel_reply_new(&reply_callback, &reply_handler, 16);
    assert(z_cancellation_token_clone(&ct_clone, z_cancellation_token_loan(&ct)) == Z_OK);
    z_liveliness_get_options_default(&opts);
    opts.cancellation_token = z_cancellation_token_move(&ct_clone);
    z_liveliness_get(z_session_loan(&s2), z_view_keyexpr_loan(&ke), z_closure_reply_move(&reply_callback), &opts);
    z_owned_reply_t reply;
    assert(z_fifo_handler_reply_try_recv(z_fifo_handler_reply_loan(&reply_handler), &reply) == Z_CHANNEL_DISCONNECTED);
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&reply_handler));
    z_cancellation_token_drop(z_cancellation_token_move(&ct));
    z_liveliness_token_drop(z_liveliness_token_move(&token));
    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}
#endif

#endif

int main(void) {
#if Z_FEATURE_QUERY == 1 && Z_FEATURE_MULTI_THREAD == 1 && defined(Z_FEATURE_UNSTABLE_API)
    test_cancel_get();
    test_cancel_querier_get();
#if Z_FEATURE_LIVELINESS == 1
    test_liveliness_get();
#endif
#endif
    return 0;
}
