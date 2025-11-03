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
    _z_sync_group_notifier_t notifier;
    size_t* val;
} token_handler_arg_t;

z_result_t cancel_callback(void* arg) {
    token_handler_arg_t* typed = (token_handler_arg_t*)arg;
    _z_sync_group_notifier_drop(&typed->notifier);
    (*typed->val)++;
    return Z_OK;
}

void cancel_drop(void* arg) {
    token_handler_arg_t* typed = (token_handler_arg_t*)arg;
    _z_sync_group_notifier_drop(&typed->notifier);
}

void test_cancellation_token_cancel(void) {
    size_t val = 0;
    _z_cancellation_token_t ct;
    assert(_z_cancellation_token_create(&ct) == Z_OK);

    _z_sync_group_t g1, g2;
    assert(_z_sync_group_create(&g1) == Z_OK);
    assert(_z_sync_group_create(&g2) == Z_OK);

    token_handler_arg_t arg1, arg2;
    arg1.val = &val;
    assert(_z_sync_group_create_notifier(&g1, &arg1.notifier) == Z_OK);
    arg2.val = &val;
    assert(_z_sync_group_create_notifier(&g2, &arg2.notifier) == Z_OK);

    _z_cancellation_token_on_cancel_handler_t h1 = {
        ._arg = &arg1, ._on_cancel = cancel_callback, ._on_drop = cancel_drop, ._sync_group = g1};
    _z_cancellation_token_on_cancel_handler_t h2 = {
        ._arg = &arg2, ._on_cancel = cancel_callback, ._on_drop = cancel_drop, ._sync_group = g2};

    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h1) == Z_OK);
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h2) == Z_OK);

    assert(!_z_cancellation_token_is_cancelled(&ct));
    assert(_z_cancellation_token_cancel(&ct) == Z_OK);
    assert(_z_cancellation_token_is_cancelled(&ct));

    _z_cancellation_token_clear(&ct);

    assert(val == 2);
}

void test_cancellation_token_cancel_timeout(void) {
    size_t val = 0;
    _z_cancellation_token_t ct;
    assert(_z_cancellation_token_create(&ct) == Z_OK);

    _z_sync_group_t g1, g2, g3;
    assert(_z_sync_group_create(&g1) == Z_OK);
    assert(_z_sync_group_create(&g2) == Z_OK);
    assert(_z_sync_group_create(&g3) == Z_OK);

    token_handler_arg_t arg1, arg2, arg3;
    arg1.val = &val;
    assert(_z_sync_group_create_notifier(&g1, &arg1.notifier) == Z_OK);
    arg2.val = &val;
    assert(_z_sync_group_create_notifier(&g2, &arg2.notifier) == Z_OK);
    arg3.val = &val;
    assert(_z_sync_group_create_notifier(&g3, &arg3.notifier) == Z_OK);

    _z_cancellation_token_on_cancel_handler_t h1 = {
        ._arg = &arg1, ._on_cancel = cancel_callback, ._on_drop = cancel_drop, ._sync_group = g1};
    _z_cancellation_token_on_cancel_handler_t h2 = {
        ._arg = &arg2, ._on_cancel = cancel_callback, ._on_drop = cancel_drop, ._sync_group = g2};
    _z_cancellation_token_on_cancel_handler_t h3 = {
        ._arg = &arg3, ._on_cancel = NULL, ._on_drop = cancel_drop, ._sync_group = g3};

    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h1) == Z_OK);
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h2) == Z_OK);
    assert(_z_cancellation_token_add_on_cancel_handler(&ct, &h3) == Z_OK);

    assert(!_z_cancellation_token_is_cancelled(&ct));
    assert(_z_cancellation_token_cancel_with_timeout(&ct, 1000) == Z_ETIMEDOUT);
    assert(_z_cancellation_token_is_cancelled(&ct));

    _z_cancellation_token_clear(&ct);

    assert(val == 2);
}

int main(void) {
    test_cancellation_token_cancel();
    test_cancellation_token_cancel_timeout();
    return 0;
}
