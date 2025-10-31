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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/system/platform.h"

#undef NDEBUG
#include <assert.h>

#if Z_FEATURE_MULTI_THREAD == 1
typedef struct {
    z_loaned_mutex_t* m;
    z_loaned_condvar_t* cv;
} task_arg_t;

void* task_fn(void* arg) {
    task_arg_t* typed = (task_arg_t*)arg;
    z_mutex_lock(typed->m);
    z_sleep_s(2);
    z_condvar_signal(typed->cv);
    z_mutex_unlock(typed->m);
    return NULL;
}
#endif

int main(void) {
#if Z_FEATURE_MULTI_THREAD == 1
    z_owned_condvar_t cv;
    z_owned_mutex_t m;

    z_mutex_init(&m);
    z_condvar_init(&cv);

    assert(z_mutex_lock(z_mutex_loan_mut(&m)) == Z_OK);
    for (unsigned i = 1; i <= 5; i++) {
        printf("Check timedout wait: %d seconds\n", i);
        z_clock_t c = z_clock_now();
        z_clock_t deadline = c;
        z_clock_advance_s(&deadline, i);
        assert(z_condvar_wait_until(z_condvar_loan_mut(&cv), z_mutex_loan_mut(&m), &deadline) == Z_ETIMEDOUT);
        ;
        unsigned long elapsed = z_clock_elapsed_ms(&c);
        assert(elapsed > (i * 1000 - 500) && elapsed < (i * 1000 + 500));
    }

    printf("Check successful wait\n");
    z_clock_t c = z_clock_now();
    z_clock_t deadline = c;
    z_clock_advance_s(&deadline, 10);
    z_owned_task_t task;
    task_arg_t arg;
    arg.m = z_mutex_loan_mut(&m);
    arg.cv = z_condvar_loan_mut(&cv);

    assert(z_task_init(&task, NULL, task_fn, &arg) == Z_OK);

    assert(z_condvar_wait_until(z_condvar_loan_mut(&cv), z_mutex_loan_mut(&m), &deadline) == Z_OK);
    unsigned long elapsed = z_clock_elapsed_ms(&c);
    assert(elapsed < 5000);

    z_task_join(z_task_move(&task));
    z_mutex_unlock(z_mutex_loan_mut(&m));
    z_condvar_drop(z_condvar_move(&cv));
    z_mutex_drop(z_mutex_move(&m));
#endif
}
