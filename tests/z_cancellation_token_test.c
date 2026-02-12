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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"

#undef NDEBUG
#include <assert.h>

typedef struct {
    size_t called;
    size_t dropped;
} token_handler_arg_t;

z_result_t cancel_callback(void* arg) {
    token_handler_arg_t* typed = (token_handler_arg_t*)arg;
    (typed->called)++;
    return Z_OK;
}

void cancel_drop(void* arg) {
    token_handler_arg_t* typed = (token_handler_arg_t*)arg;
    (typed->dropped)++;
}

void test_cancellation_token_cancel(void) {
    _z_cancellation_token_t ct;
    assert(_z_cancellation_token_create(&ct) == Z_OK);

    token_handler_arg_t arg1 = {0};
    token_handler_arg_t arg2 = {0};
    _z_cancellation_token_on_cancel_handler_t h1 = {
        ._arg = &arg1, ._on_cancel = cancel_callback, ._on_drop = cancel_drop};
    _z_cancellation_token_on_cancel_handler_t h2 = {
        ._arg = &arg2, ._on_cancel = cancel_callback, ._on_drop = cancel_drop};

    _z_sync_group_notifier_t notifier;
    assert(_z_cancellation_token_get_notifier(&ct, &notifier) == _Z_RES_OK);
    size_t id1, id2;
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h1, &id1) == Z_OK);
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h2, &id2) == Z_OK);
    _z_sync_group_notifier_drop(&notifier);
    assert(!_z_cancellation_token_is_cancelled(&ct));
    assert(_z_cancellation_token_cancel(&ct) == Z_OK);
    assert(_z_cancellation_token_is_cancelled(&ct));
    assert(arg1.called == 1);
    assert(arg1.dropped == 1);
    assert(arg2.called == 1);
    assert(arg2.dropped == 1);
    _z_cancellation_token_clear(&ct);
}

void test_cancellation_token_cancel_timeout(void) {
    _z_cancellation_token_t ct;
    assert(_z_cancellation_token_create(&ct) == Z_OK);

    token_handler_arg_t arg1 = {0};
    token_handler_arg_t arg2 = {0};
    _z_cancellation_token_on_cancel_handler_t h1 = {
        ._arg = &arg1, ._on_cancel = cancel_callback, ._on_drop = cancel_drop};
    _z_cancellation_token_on_cancel_handler_t h2 = {
        ._arg = &arg2, ._on_cancel = cancel_callback, ._on_drop = cancel_drop};

    _z_sync_group_notifier_t notifier;
    assert(_z_cancellation_token_get_notifier(&ct, &notifier) == _Z_RES_OK);
    size_t id1, id2;
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h1, &id1) == Z_OK);
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h2, &id2) == Z_OK);

    assert(!_z_cancellation_token_is_cancelled(&ct));
    assert(_z_cancellation_token_cancel_with_timeout(&ct, 1000) == Z_ETIMEDOUT);
    assert(!_z_cancellation_token_is_cancelled(&ct));

    _z_sync_group_notifier_drop(&notifier);
    assert(_z_cancellation_token_cancel_with_timeout(&ct, 1000) == _Z_RES_OK);
    assert(_z_cancellation_token_is_cancelled(&ct));
    _z_cancellation_token_clear(&ct);
}

void test_cancellation_token_remove_handler(void) {
    _z_cancellation_token_t ct;
    assert(_z_cancellation_token_create(&ct) == Z_OK);

    token_handler_arg_t arg1 = {0};
    token_handler_arg_t arg2 = {0};
    _z_cancellation_token_on_cancel_handler_t h1 = {
        ._arg = &arg1, ._on_cancel = cancel_callback, ._on_drop = cancel_drop};
    _z_cancellation_token_on_cancel_handler_t h2 = {
        ._arg = &arg2, ._on_cancel = cancel_callback, ._on_drop = cancel_drop};

    _z_sync_group_notifier_t notifier;
    assert(_z_cancellation_token_get_notifier(&ct, &notifier) == _Z_RES_OK);
    size_t id1, id2;
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h1, &id1) == Z_OK);
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h2, &id2) == Z_OK);
    assert(_z_cancellation_token_remove_on_cancel_handler(&ct, id2) == Z_OK);
    assert(arg2.dropped == 1);

    _z_sync_group_notifier_drop(&notifier);
    assert(!_z_cancellation_token_is_cancelled(&ct));
    assert(_z_cancellation_token_cancel(&ct) == Z_OK);
    assert(_z_cancellation_token_is_cancelled(&ct));
    assert(arg1.called == 1);
    assert(arg1.dropped == 1);
    assert(arg2.called == 0);

    _z_cancellation_token_clear(&ct);
}

int main(void) {
    test_cancellation_token_cancel();
    test_cancellation_token_cancel_timeout();
    test_cancellation_token_remove_handler();
    return 0;
}
