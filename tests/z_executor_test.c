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

#include "zenoh-pico/collections/executor.h"
#include "zenoh-pico/system/platform.h"

#undef NDEBUG
#include <assert.h>

// ─── Helpers ────────────────────────────────────────────────────────────────

typedef struct {
    int call_count;
    bool destroyed;
} test_arg_t;

// Finishes immediately
static _z_fut_fn_result_t fn_finish(void *arg, _z_executor_t *ex) {
    (void)ex;
    test_arg_t *a = (test_arg_t *)arg;
    a->call_count++;
    return (_z_fut_fn_result_t){._ready = true};
}

// Reschedules with a wake_up_time, finishes on second call
static _z_fut_fn_result_t fn_reschedule_timed(void *arg, _z_executor_t *ex) {
    (void)ex;
    test_arg_t *a = (test_arg_t *)arg;
    a->call_count++;
    if (a->call_count == 1) {
        z_clock_t wake = z_clock_now();
        z_clock_advance_ms(&wake, 500);  // wake up after 500ms
        return (_z_fut_fn_result_t){
            ._ready = false,
            ._has_wake_up_time = true,
            ._wake_up_time = wake,
        };
    }
    return (_z_fut_fn_result_t){._ready = true};
}

// Reschedules without wake_up_time (goes back to regular deque), finishes on second call
static _z_fut_fn_result_t fn_reschedule_deque(void *arg, _z_executor_t *ex) {
    (void)ex;
    test_arg_t *a = (test_arg_t *)arg;
    a->call_count++;
    if (a->call_count == 1) {
        return (_z_fut_fn_result_t){._ready = false, ._has_wake_up_time = false};
    }
    return (_z_fut_fn_result_t){._ready = true};
}

// Spawns a child future into the executor, then finishes
static _z_fut_fn_result_t fn_spawn_child(void *arg, _z_executor_t *ex) {
    test_arg_t *child_arg = (test_arg_t *)arg;
    _z_fut_t child;
    _z_fut_new(&child, child_arg, fn_finish, NULL);
    _z_executor_spawn(ex, &child);
    return (_z_fut_fn_result_t){._ready = true};
}

static void destroy_fn(void *arg) {
    test_arg_t *a = (test_arg_t *)arg;
    a->destroyed = true;
}

// Drain the executor until NO_TASKS or max_spins reached; return number of spins
static int drain(_z_executor_t *ex, int max_spins) {
    int spins = 0;
    while (spins++ < max_spins) {
        _z_executor_spin_result_t r = _z_executor_spin(ex);
        if (r.status == _Z_EXECUTOR_SPIN_RESULT_NO_TASKS) break;
    }
    return spins;
}

// ─── Tests ───────────────────────────────────────────────────────────────────

// Spin on a freshly created executor returns NO_TASKS.
static void test_spin_empty(void) {
    printf("Test: spin on empty executor returns NO_TASKS\n");
    _z_executor_t ex = _z_executor_new();
    _z_executor_spin_result_t r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_NO_TASKS);
    _z_executor_destroy(&ex);
}

// A future without a handle (NULL handle): body runs, destroy_fn called.
static void test_spawn_no_handle(void) {
    printf("Test: future without handle runs and calls destroy_fn\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t arg = {0};

    _z_fut_t fut;
    _z_fut_new(&fut, &arg, fn_finish, destroy_fn);
    // No handle assigned — _handle stays null
    bool spawned = _z_executor_spawn(&ex, &fut);
    assert(spawned);

    _z_executor_spin_result_t r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_EXECUTED_TASK);
    assert(arg.call_count == 1);
    assert(arg.destroyed == true);

    r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_NO_TASKS);
    _z_executor_destroy(&ex);
}

// A future with a handle: status transitions PENDING → READY.
static void test_spawn_with_handle_status_transitions(void) {
    printf("Test: future with handle transitions PENDING -> READY\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t arg = {0};

    // Build a handle manually (mirrors what _z_executor_spawn does internally)
    _z_fut_t fut;
    _z_fut_new(&fut, &arg, fn_finish, destroy_fn);
    _z_fut_handle_rc_t h = _z_fut_get_handle(&fut);
    assert(!_Z_RC_IS_NULL(&h));
    assert(_z_fut_handle_status(_Z_RC_IN_VAL(&h)) == _Z_FUT_STATUS_PENDING);

    bool spawned = _z_executor_spawn(&ex, &fut);
    assert(spawned);

    drain(&ex, 10);
    assert(arg.call_count == 1);
    assert(arg.destroyed == true);
    assert(_z_fut_handle_status(_Z_RC_IN_VAL(&h)) == _Z_FUT_STATUS_READY);

    _z_fut_handle_rc_drop(&h);
    _z_executor_destroy(&ex);
}

// A future returning _ready=false + _has_wake_up_time=true is re-queued in the timed pqueue.
static void test_timed_reschedule(void) {
    printf("Test: timed reschedule re-queues in timed pqueue and eventually finishes\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t arg = {0};

    _z_fut_t fut;
    _z_fut_new(&fut, &arg, fn_reschedule_timed, destroy_fn);
    assert(_z_executor_spawn(&ex, &fut));

    // First spin: task runs once, reschedules with immediate wake_up_time
    _z_executor_spin_result_t r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_EXECUTED_TASK);
    assert(arg.call_count == 1);
    assert(arg.destroyed == false);

    r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_SHOULD_WAIT);
    z_clock_t now = z_clock_now();
    assert(zp_clock_elapsed_ms_since(&r.next_wake_up_time, &now) > 300);
    z_sleep_ms(100);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_SHOULD_WAIT);
    assert(zp_clock_elapsed_ms_since(&r.next_wake_up_time, &now) > 200);

    z_sleep_ms(600);
    r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_EXECUTED_TASK);

    assert(arg.call_count == 2);
    assert(arg.destroyed == true);

    r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_NO_TASKS);

    _z_executor_destroy(&ex);
}

// A future returning _ready=false + _has_wake_up_time=false is re-queued in the regular deque.
static void test_deque_reschedule(void) {
    printf("Test: deque reschedule re-queues at back of deque and finishes on next spin\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t arg = {0};

    _z_fut_t fut;
    _z_fut_new(&fut, &arg, fn_reschedule_deque, destroy_fn);
    assert(_z_executor_spawn(&ex, &fut));

    // First spin: task runs, returns not-ready, pushed back to deque
    _z_executor_spin_result_t r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_EXECUTED_TASK);
    assert(arg.call_count == 1);
    assert(arg.destroyed == false);

    // Second spin: task runs again and finishes
    r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_EXECUTED_TASK);
    assert(arg.call_count == 2);
    assert(arg.destroyed == true);

    r = _z_executor_spin(&ex);
    assert(r.status == _Z_EXECUTOR_SPIN_RESULT_NO_TASKS);
    _z_executor_destroy(&ex);
}

// Cancelling a handle before spin: body never runs, destroy_fn still called.
static void test_cancel_before_spin(void) {
    printf("Test: cancel before spin prevents body; destroy_fn still called\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t arg = {0};

    _z_fut_t fut;
    _z_fut_new(&fut, &arg, fn_finish, destroy_fn);
    _z_fut_handle_rc_t h = _z_fut_get_handle(&fut);
    assert(_z_executor_spawn(&ex, &fut));

    // Cancel before any spin
    _z_fut_handle_cancel(_Z_RC_IN_VAL(&h));
    assert(_z_fut_handle_status(_Z_RC_IN_VAL(&h)) == _Z_FUT_STATUS_CANCELLED);

    drain(&ex, 10);
    assert(arg.call_count == 0);    // body never ran
    assert(arg.destroyed == true);  // destroy_fn still ran

    _z_fut_handle_rc_drop(&h);
    _z_executor_destroy(&ex);
}

// Cancelling after the task finishes is a safe no-op; status stays READY.
static void test_cancel_after_finish(void) {
    printf("Test: cancel on READY handle is a no-op\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t arg = {0};

    _z_fut_t fut;
    _z_fut_new(&fut, &arg, fn_finish, destroy_fn);
    _z_fut_handle_rc_t h = _z_fut_get_handle(&fut);
    assert(_z_executor_spawn(&ex, &fut));

    drain(&ex, 10);
    assert(_z_fut_handle_status(_Z_RC_IN_VAL(&h)) == _Z_FUT_STATUS_READY);

    // Must not crash or corrupt status
    _z_fut_handle_cancel(_Z_RC_IN_VAL(&h));
    assert(_z_fut_handle_status(_Z_RC_IN_VAL(&h)) == _Z_FUT_STATUS_READY);

    _z_fut_handle_rc_drop(&h);
    _z_executor_destroy(&ex);
}

// A task may spawn a child via the executor* passed to the fn.
static void test_task_spawns_child(void) {
    printf("Test: task can spawn a child future via executor pointer\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t child_arg = {0};

    _z_fut_t parent;
    _z_fut_new(&parent, &child_arg, fn_spawn_child, NULL);
    assert(_z_executor_spawn(&ex, &parent));

    // First spin: parent runs, spawns child into executor
    _z_executor_spin(&ex);

    // Remaining spins: child runs
    drain(&ex, 10);
    assert(child_arg.call_count == 1);

    _z_executor_destroy(&ex);
}

// N independent tasks all complete when drained.
static void test_multiple_tasks(void) {
    printf("Test: multiple futures all complete\n");
    _z_executor_t ex = _z_executor_new();
    const int N = 8;
    test_arg_t args[8];

    for (int i = 0; i < N; i++) {
        args[i] = (test_arg_t){0};
        _z_fut_t fut;
        _z_fut_new(&fut, &args[i], fn_finish, destroy_fn);
        assert(_z_executor_spawn(&ex, &fut));
    }

    drain(&ex, N * 4);

    for (int i = 0; i < N; i++) {
        assert(args[i].call_count == 1);
        assert(args[i].destroyed == true);
    }
    assert(_z_executor_spin(&ex).status == _Z_EXECUTOR_SPIN_RESULT_NO_TASKS);
    _z_executor_destroy(&ex);
}

// _z_executor_destroy calls destroy_fn on tasks that never ran.
static void test_destroy_drains_pending(void) {
    printf("Test: _z_executor_destroy calls destroy_fn on all pending futures\n");
    _z_executor_t ex = _z_executor_new();
    const int N = 4;
    test_arg_t args[4];

    for (int i = 0; i < N; i++) {
        args[i] = (test_arg_t){0};
        _z_fut_t fut;
        _z_fut_new(&fut, &args[i], fn_finish, destroy_fn);
        assert(_z_executor_spawn(&ex, &fut));
    }

    // Destroy without any spinning
    _z_executor_destroy(&ex);

    for (int i = 0; i < N; i++) {
        assert(args[i].call_count == 0);    // body never ran
        assert(args[i].destroyed == true);  // destroy_fn called by deque teardown
    }
}

// Dropping the last handle clone does not affect a still-pending task in the queue.
static void test_drop_handle_clone_task_still_runs(void) {
    printf("Test: dropping last handle clone does not cancel the task\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t arg = {0};

    _z_fut_t fut;
    _z_fut_new(&fut, &arg, fn_finish, destroy_fn);
    _z_fut_handle_rc_t h = _z_fut_get_handle(&fut);
    assert(_z_executor_spawn(&ex, &fut));

    // Drop our clone — executor still holds its own clone
    _z_fut_handle_rc_drop(&h);

    drain(&ex, 10);
    assert(arg.call_count == 1);
    assert(arg.destroyed == true);

    _z_executor_destroy(&ex);
}

// _z_fut_handle_status reports PENDING before spin, READY after.
static void test_handle_status_pending_then_ready(void) {
    printf("Test: handle status is PENDING before spin, READY after\n");
    _z_executor_t ex = _z_executor_new();
    test_arg_t arg = {0};

    _z_fut_t fut;
    _z_fut_new(&fut, &arg, fn_finish, NULL);
    _z_fut_handle_rc_t h = _z_fut_get_handle(&fut);
    assert(_z_executor_spawn(&ex, &fut));

    assert(_z_fut_handle_status(_Z_RC_IN_VAL(&h)) == _Z_FUT_STATUS_PENDING);
    drain(&ex, 10);
    assert(_z_fut_handle_status(_Z_RC_IN_VAL(&h)) == _Z_FUT_STATUS_READY);

    _z_fut_handle_rc_drop(&h);
    _z_executor_destroy(&ex);
}

// ─── main ────────────────────────────────────────────────────────────────────

int main(void) {
    test_spin_empty();
    test_spawn_no_handle();
    test_spawn_with_handle_status_transitions();
    test_timed_reschedule();
    test_deque_reschedule();
    test_cancel_before_spin();
    test_cancel_after_finish();
    test_task_spawns_child();
    test_multiple_tasks();
    test_destroy_drains_pending();
    test_drop_handle_clone_task_still_runs();
    test_handle_status_pending_then_ready();
    printf("All executor tests passed.\n");
    return 0;
}
