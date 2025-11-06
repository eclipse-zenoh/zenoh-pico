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

#if Z_FEATURE_MULTI_THREAD == 1

typedef struct {
    _z_sync_group_notifier_t notifier1;
    _z_sync_group_notifier_t notifier2;
    size_t val;
} wait_task_arg_t;

void* sync_group_task(void* arg) {
    wait_task_arg_t* typed = (wait_task_arg_t*)arg;
    z_sleep_s(3);
    typed->val += 1;
    _z_sync_group_notifier_drop(&typed->notifier1);
    z_sleep_s(3);
    typed->val += 1;
    _z_sync_group_notifier_drop(&typed->notifier2);
    return NULL;
}

void test_sync_group_wait(void) {
    printf("test_sync_group_wait\n");
    _z_sync_group_t g;
    assert(_z_sync_group_create(&g) == _Z_RES_OK);
    wait_task_arg_t arg;
    arg.val = 0;
    assert(_z_sync_group_create_notifier(&g, &arg.notifier1) == _Z_RES_OK);
    assert(_z_sync_group_create_notifier(&g, &arg.notifier2) == _Z_RES_OK);
    _z_task_t task;
    _z_task_init(&task, NULL, sync_group_task, &arg);
    assert(_z_sync_group_wait(&g) == _Z_RES_OK);
    assert(arg.val == 2);
    _z_sync_group_drop(&g);
    _z_task_join(&task);
}

void test_sync_group_wait_deadline(void) {
    printf("test_sync_group_wait_deadline\n");
    _z_sync_group_t g;
    assert(_z_sync_group_create(&g) == Z_OK);
    wait_task_arg_t arg;
    arg.val = 0;
    assert(_z_sync_group_create_notifier(&g, &arg.notifier1) == Z_OK);
    assert(_z_sync_group_create_notifier(&g, &arg.notifier2) == Z_OK);

    _z_task_t task;
    _z_task_init(&task, NULL, sync_group_task, &arg);

    z_clock_t c = z_clock_now();
    z_clock_advance_s(&c, 2);
    assert(_z_sync_group_wait_deadline(&g, &c) == Z_ETIMEDOUT);
    z_clock_advance_s(&c, 5);
    assert(_z_sync_group_wait_deadline(&g, &c) == Z_OK);
    assert(arg.val == 2);
    _z_sync_group_drop(&g);
    _z_task_join(&task);
}

#endif
int main(void) {
#if Z_FEATURE_MULTI_THREAD == 1
    test_sync_group_wait();
    test_sync_group_wait_deadline();
#endif
    return 0;
}
