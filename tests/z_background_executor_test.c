//
// Copyright (c) 2026 ZettaScale Technology
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

#include <stddef.h>
#include <stdio.h>

#include "zenoh-pico/collections/background_executor.h"
#include "zenoh-pico/system/platform.h"

#undef NDEBUG
#include <assert.h>

#if Z_FEATURE_MULTI_THREAD == 1

// ─── Helpers ────────────────────────────────────────────────────────────────

// Shared state between the test thread and the background executor thread.
// All fields are protected by mutex + condvar except where noted.
typedef struct {
    _z_mutex_t mutex;
    _z_condvar_t condvar;
    int call_count;         // number of times fn body was entered
    bool destroyed;         // destroy_fn was called
    unsigned long wait_ms;  // wait for timed part of fn_reschedule_once
} test_arg_t;

static void test_arg_init(test_arg_t *a) {
    _z_mutex_init(&a->mutex);
    _z_condvar_init(&a->condvar);
    a->call_count = 0;
    a->destroyed = false;
    a->wait_ms = 0;
}

static void test_arg_clear(test_arg_t *a) {
    _z_condvar_drop(&a->condvar);
    _z_mutex_drop(&a->mutex);
}

// Block until call_count >= expected.
static void test_arg_wait_calls(test_arg_t *a, int expected) {
    _z_mutex_lock(&a->mutex);
    while (a->call_count < expected) {
        _z_condvar_wait(&a->condvar, &a->mutex);
    }
    _z_mutex_unlock(&a->mutex);
}

static int test_arg_get_calls(test_arg_t *a) {
    _z_mutex_lock(&a->mutex);
    int calls = a->call_count;
    _z_mutex_unlock(&a->mutex);
    return calls;
}

static int test_arg_get_destroyed(test_arg_t *a) {
    _z_mutex_lock(&a->mutex);
    bool destroyed = a->destroyed;
    _z_mutex_unlock(&a->mutex);
    return destroyed;
}

// Block until destroyed == true.
static void test_arg_wait_destroyed(test_arg_t *a) {
    _z_mutex_lock(&a->mutex);
    while (!a->destroyed) {
        _z_condvar_wait(&a->condvar, &a->mutex);
    }
    _z_mutex_unlock(&a->mutex);
}

// ── fut_fn helpers ────────────────────────────────────────────────────────────

// Increments call_count and finishes immediately.
static _z_fut_fn_result_t fn_finish(void *arg, _z_executor_t *ex) {
    (void)ex;
    test_arg_t *a = (test_arg_t *)arg;
    _z_mutex_lock(&a->mutex);
    a->call_count++;
    _z_condvar_signal_all(&a->condvar);
    _z_mutex_unlock(&a->mutex);
    return (_z_fut_fn_result_t){._status = _Z_FUT_STATUS_READY};
}

// Reschedules with immediate wake_up_time on first call, finishes on second.
static _z_fut_fn_result_t fn_reschedule_once(void *arg, _z_executor_t *ex) {
    (void)ex;
    test_arg_t *a = (test_arg_t *)arg;
    _z_mutex_lock(&a->mutex);
    int count = ++a->call_count;
    _z_condvar_signal_all(&a->condvar);
    _z_mutex_unlock(&a->mutex);
    z_clock_t wake = z_clock_now();
    z_clock_advance_ms(&wake, a->wait_ms);
    if (count == 1) {
        return (_z_fut_fn_result_t){
            ._status = _Z_FUT_STATUS_SLEEPING,
            ._wake_up_time = wake,
        };
    }
    return (_z_fut_fn_result_t){._status = _Z_FUT_STATUS_READY};
}

static void destroy_fn(void *arg) {
    test_arg_t *a = (test_arg_t *)arg;
    _z_mutex_lock(&a->mutex);
    a->destroyed = true;
    _z_condvar_signal_all(&a->condvar);
    _z_mutex_unlock(&a->mutex);
}

// ─── Tests ───────────────────────────────────────────────────────────────────

// _z_background_executor_init + _z_background_executor_destroy with no tasks.
static void test_new_destroy_no_tasks(void) {
    printf("Test: new and destroy with no tasks\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);
    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);
}

// A spawned future runs on the background thread; destroy_fn is called.
static void test_spawn_runs_task(void) {
    printf("Test: spawn runs task on background thread and calls destroy_fn\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);

    test_arg_t arg;
    test_arg_init(&arg);

    _z_fut_t fut = _z_fut_new(&arg, fn_finish, destroy_fn);
    assert(_z_background_executor_spawn(&be, &fut, NULL) == _Z_RES_OK);

    test_arg_wait_calls(&arg, 1);
    assert(arg.call_count == 1);

    test_arg_wait_destroyed(&arg);
    assert(arg.destroyed == true);

    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);
    test_arg_clear(&arg);
}

// A future with a handle: cancel before it runs — body never called, destroy_fn called.
static void test_cancel_before_execution(void) {
    printf("Test: cancel before execution prevents body; destroy_fn still called\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);

    test_arg_t arg;
    test_arg_init(&arg);

    // Suspend so the task cannot be picked up before we cancel it
    assert(_z_background_executor_suspend(&be) == _Z_RES_OK);

    _z_fut_t fut = _z_fut_new(&arg, fn_finish, destroy_fn);
    _z_fut_handle_t h;
    assert(_z_background_executor_spawn(&be, &fut, &h) == _Z_RES_OK);
    assert(h.is_valid);

    // Cancel while executor is suspended
    assert(_z_background_executor_cancel_fut(&be, &h) == _Z_RES_OK);
    _z_fut_status_t status;
    assert(_z_background_executor_get_fut_status(&be, &h, &status) == _Z_RES_OK);
    assert(status == _Z_FUT_STATUS_READY);  // cancelled tasks are considered ready (not pending, running, or sleeping);

    assert(_z_background_executor_resume(&be) == _Z_RES_OK);

    // destroy_fn must be called even though the task was cancelled
    test_arg_wait_destroyed(&arg);
    assert(arg.destroyed == true);
    assert(arg.call_count == 0);  // body never ran

    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);
    test_arg_clear(&arg);
}

// A future with timed reschedule runs exactly twice.
static void test_timed_reschedule_runs_twice(void) {
    printf("Test: timed reschedule runs task body exactly twice\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);

    test_arg_t arg;
    test_arg_init(&arg);
    arg.wait_ms = 500;

    _z_fut_t fut = _z_fut_new(&arg, fn_reschedule_once, destroy_fn);
    assert(_z_background_executor_spawn(&be, &fut, NULL) == _Z_RES_OK);

    z_sleep_ms(100);                        // give the background thread a chance to run the first call and reschedule
    assert(test_arg_get_calls(&arg) == 1);  // first call must have happened, but not the second yet
    assert(test_arg_get_destroyed(&arg) == false);  // destroy_fn must not have been called yet
    z_sleep_ms(500);                                // wait for the rescheduled call to become ready and run
    assert(test_arg_get_calls(&arg) == 2);
    assert(test_arg_get_destroyed(&arg) == true);

    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);
    test_arg_clear(&arg);
}

// Tasks do not run while the executor is suspended; they run after resume.
static void test_suspend_blocks_execution(void) {
    printf("Test: tasks do not run while suspended; run after resume\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);

    test_arg_t arg;
    test_arg_init(&arg);

    assert(_z_background_executor_suspend(&be) == _Z_RES_OK);

    _z_fut_t fut = _z_fut_new(&arg, fn_finish, destroy_fn);
    assert(_z_background_executor_spawn(&be, &fut, NULL) == _Z_RES_OK);

    // Give the background thread ample opportunity to (incorrectly) run the task
    z_sleep_ms(100);
    _z_mutex_lock(&arg.mutex);
    int calls_while_suspended = arg.call_count;
    _z_mutex_unlock(&arg.mutex);
    assert(calls_while_suspended == 0);  // must not have run yet

    assert(_z_background_executor_resume(&be) == _Z_RES_OK);

    test_arg_wait_calls(&arg, 1);
    assert(arg.call_count == 1);

    test_arg_wait_destroyed(&arg);
    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);
    test_arg_clear(&arg);
}

// Nested suspend/resume: execution resumes only after all suspenders have resumed.
static void test_nested_suspend_resume(void) {
    printf("Test: nested suspend/resume — task runs only after all resumes\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);

    test_arg_t arg;
    test_arg_init(&arg);

    // Two independent suspenders
    assert(_z_background_executor_suspend(&be) == _Z_RES_OK);
    assert(_z_background_executor_suspend(&be) == _Z_RES_OK);

    _z_fut_t fut = _z_fut_new(&arg, fn_finish, destroy_fn);
    assert(_z_background_executor_spawn(&be, &fut, NULL) == _Z_RES_OK);

    z_sleep_ms(100);
    _z_mutex_lock(&arg.mutex);
    assert(arg.call_count == 0);
    _z_mutex_unlock(&arg.mutex);

    // First resume — still one suspender outstanding
    assert(_z_background_executor_resume(&be) == _Z_RES_OK);
    z_sleep_ms(100);
    _z_mutex_lock(&arg.mutex);
    assert(arg.call_count == 0);
    _z_mutex_unlock(&arg.mutex);

    // Second resume — now fully unblocked
    assert(_z_background_executor_resume(&be) == _Z_RES_OK);
    test_arg_wait_calls(&arg, 1);
    assert(arg.call_count == 1);

    test_arg_wait_destroyed(&arg);
    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);
    test_arg_clear(&arg);
}

// Multiple concurrent tasks all complete.
static void test_multiple_tasks_all_complete(void) {
    printf("Test: multiple concurrent tasks all complete\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);

    const int N = 8;
    test_arg_t args[8];
    z_clock_t start = z_clock_now();
    for (int i = 0; i < N; i++) {
        test_arg_init(&args[i]);
        args[i].wait_ms = (unsigned long)(300 * (i + 1));  // stagger the wait times so tasks finish in order
        _z_fut_t fut = _z_fut_new(&args[i], fn_reschedule_once, destroy_fn);
        assert(_z_background_executor_spawn(&be, &fut, NULL) == _Z_RES_OK);
    }

    for (int i = 0; i < N; i++) {
        test_arg_wait_calls(&args[i], 2);
        test_arg_wait_destroyed(&args[i]);
        z_clock_t now = z_clock_now();
        unsigned long elapsed_ms = zp_clock_elapsed_ms_since(&now, &start);
        assert(args[i].call_count == 2);
        assert(args[i].destroyed == true);
        assert(elapsed_ms >=
               args[i].wait_ms);  // each task must have taken at least as long as its wait time, indicating it
        assert(elapsed_ms <= args[i].wait_ms + 300);  // but not too much longer
    }

    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);
    for (int i = 0; i < N; i++) {
        test_arg_clear(&args[i]);
    }
}

// destroy while tasks are pending: destroy_fn is called for each.
static void test_destroy_with_pending_tasks(void) {
    printf("Test: destroy calls destroy_fn on all pending tasks\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);

    const int N = 4;
    test_arg_t args[4];

    // Queue tasks while suspended so none run before destroy
    assert(_z_background_executor_suspend(&be) == _Z_RES_OK);
    for (int i = 0; i < N; i++) {
        test_arg_init(&args[i]);
        _z_fut_t fut = _z_fut_new(&args[i], fn_finish, destroy_fn);
        assert(_z_background_executor_spawn(&be, &fut, NULL) == _Z_RES_OK);
    }
    // Resume so the background thread can process cancellations on destroy
    assert(_z_background_executor_resume(&be) == _Z_RES_OK);

    // Destroy immediately — some or all tasks may not have run yet
    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);

    // Every destroy_fn must have been called by now (destroy is synchronous)
    for (int i = 0; i < N; i++) {
        assert(args[i].destroyed == true);
        test_arg_clear(&args[i]);
    }
}

// Handle status is PENDING before execution, READY after.
static void test_handle_status_transitions(void) {
    printf("Test: handle status transitions PENDING -> READY\n");
    _z_background_executor_t be;
    assert(_z_background_executor_init(&be) == _Z_RES_OK);

    test_arg_t arg;
    test_arg_init(&arg);

    assert(_z_background_executor_suspend(&be) == _Z_RES_OK);

    _z_fut_t fut = _z_fut_new(&arg, fn_finish, destroy_fn);
    _z_fut_handle_t h;
    assert(_z_background_executor_spawn(&be, &fut, &h) == _Z_RES_OK);
    _z_fut_status_t status;
    assert(_z_background_executor_get_fut_status(&be, &h, &status) == _Z_RES_OK);
    assert(status == _Z_FUT_STATUS_RUNNING);

    assert(_z_background_executor_resume(&be) == _Z_RES_OK);

    test_arg_wait_calls(&arg, 1);
    assert(_z_background_executor_get_fut_status(&be, &h, &status) == _Z_RES_OK);
    assert(status == _Z_FUT_STATUS_READY);

    assert(_z_background_executor_destroy(&be) == _Z_RES_OK);
    test_arg_clear(&arg);
}

// ─── main ────────────────────────────────────────────────────────────────────

int main(void) {
    test_new_destroy_no_tasks();
    test_spawn_runs_task();
    test_cancel_before_execution();
    test_timed_reschedule_runs_twice();
    test_suspend_blocks_execution();
    test_nested_suspend_resume();
    test_multiple_tasks_all_complete();
    test_destroy_with_pending_tasks();
    test_handle_status_transitions();
    printf("All background executor tests passed.\n");
    return 0;
}

#else

int main(void) {
    printf("Skipping background executor tests (Z_FEATURE_MULTI_THREAD disabled)\n");
    return 0;
}

#endif
