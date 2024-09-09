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

#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/liveliness.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#undef NDEBUG
#include <assert.h>
typedef struct context_t {
    bool token1_put;
    bool token2_put;
    bool token1_drop;
    bool token2_drop;
} context_t;

const char* token1_expr = "zenoh-pico/liveliness/test/1";
const char* token2_expr = "zenoh-pico/liveliness/test/2";

void on_receive(const z_loaned_sample_t* s, void* context) {
    context_t* c = (context_t*)context;
    const z_loaned_keyexpr_t* k = z_sample_keyexpr(s);
    z_view_string_t ks;
    z_keyexpr_as_view_string(k, &ks);

    if (z_sample_kind(s) == Z_SAMPLE_KIND_PUT) {
        if (strncmp(token1_expr, z_string_data(z_view_string_loan(&ks)), z_string_len(z_view_string_loan(&ks))) == 0) {
            c->token1_put = true;
        } else if (strncmp(token2_expr, z_string_data(z_view_string_loan(&ks)),
                           z_string_len(z_view_string_loan(&ks))) == 0) {
            c->token2_put = true;
        }
    } else if (z_sample_kind(s) == Z_SAMPLE_KIND_DELETE) {
        if (strncmp(token1_expr, z_string_data(z_view_string_loan(&ks)), z_string_len(z_view_string_loan(&ks))) == 0) {
            c->token1_drop = true;
        } else if (strncmp(token2_expr, z_string_data(z_view_string_loan(&ks)),
                           z_string_len(z_view_string_loan(&ks))) == 0) {
            c->token2_drop = true;
        }
    }
}

void test_liveliness_sub(void) {
    const char* expr = "zenoh-pico/liveliness/test/*";

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k, k1, k2;
    z_view_keyexpr_from_str(&k, expr);
    z_view_keyexpr_from_str(&k1, token1_expr);
    z_view_keyexpr_from_str(&k2, token2_expr);

    z_open(&s1, z_config_move(&c1));
    z_open(&s2, z_config_move(&c2));

    z_owned_closure_sample_t closure;
    context_t context = {false, false, false, false};
    z_closure_sample(&closure, on_receive, NULL, (void*)(&context));

    z_owned_subscriber_t sub;
    z_liveliness_declare_subscriber(&sub, z_session_loan(&s2), z_view_keyexpr_loan(&k), z_closure_sample_move(&closure),
                                    NULL);

    z_sleep_s(1);
    z_owned_liveliness_token_t t1, t2;
    z_liveliness_declare_token(&t1, z_session_loan(&s1), z_view_keyexpr_loan(&k1), NULL);
    z_liveliness_declare_token(&t2, z_session_loan(&s1), z_view_keyexpr_loan(&k2), NULL);

    z_sleep_s(1);

    assert(context.token1_put);
    assert(context.token2_put);

    z_liveliness_undeclare_token(z_liveliness_token_move(&t1));
    z_sleep_s(1);

    assert(context.token1_drop);
    assert(!context.token2_drop);

    z_liveliness_undeclare_token(z_liveliness_token_move(&t2));
    z_sleep_s(1);
    assert(context.token2_drop);
}

void test_liveliness_get(void) {
    const char* expr = "zenoh-pico/liveliness/test/*";

    z_owned_session_t s1, s2;
    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);
    z_view_keyexpr_t k, k1;
    z_view_keyexpr_from_str(&k, expr);
    z_view_keyexpr_from_str(&k1, token1_expr);

    z_open(&s1, z_config_move(&c1));
    z_open(&s2, z_config_move(&c2));

    z_sleep_s(1);
    z_owned_liveliness_token_t t1;
    z_liveliness_declare_token(&t1, z_session_loan(&s1), z_view_keyexpr_loan(&k1), NULL);
    z_sleep_s(1);

    z_owned_fifo_handler_reply_t handler;
    z_owned_closure_reply_t cb;
    z_fifo_channel_reply_new(&cb, &handler, 3);

    z_liveliness_get(z_session_loan(&s2), z_view_keyexpr_loan(&k), z_closure_reply_move(&cb), NULL);
    z_owned_reply_t reply;
    assert(z_fifo_handler_reply_recv(z_fifo_handler_reply_loan(&handler), &reply) == Z_OK);
    assert(z_reply_is_ok(z_reply_loan(&reply)));
    const z_loaned_keyexpr_t* reply_keyexpr = z_sample_keyexpr(z_reply_ok(z_reply_loan(&reply)));
    z_view_string_t reply_keyexpr_s;
    z_keyexpr_as_view_string(reply_keyexpr, &reply_keyexpr_s);
    assert(strncmp(token1_expr, z_string_data(z_view_string_loan(&reply_keyexpr_s)),
                   z_string_len(z_view_string_loan(&reply_keyexpr_s))) == 0);

    z_reply_drop(z_reply_move(&reply));
    assert(z_fifo_handler_reply_recv(z_fifo_handler_reply_loan(&handler), &reply) == Z_CHANNEL_DISCONNECTED);

    z_liveliness_token_drop(z_liveliness_token_move(&t1));
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&handler));
    z_sleep_s(1);
    z_fifo_channel_reply_new(&cb, &handler, 3);

    z_liveliness_get(z_session_loan(&s2), z_view_keyexpr_loan(&k), z_closure_reply_move(&cb), NULL);
    assert(z_fifo_handler_reply_recv(z_fifo_handler_reply_loan(&handler), &reply) == Z_CHANNEL_DISCONNECTED);

    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&handler));
    z_session_drop(z_session_move(&s1));
    z_session_drop(z_session_move(&s2));
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    test_liveliness_sub();
    test_liveliness_get();
}
