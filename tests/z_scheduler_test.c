//
// Copyright (c) 2025 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

// Deterministic unit tests for _zp_periodic_scheduler_* (single-threaded)

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"

#if Z_FEATURE_PERIODIC_TASKS == 1 && defined Z_FEATURE_UNSTABLE_API

// ---------- Fake clock ----------
static uint64_t g_now_ms = 0;

static uint64_t fake_now(void *ctx) {
    (void)ctx;
    return g_now_ms;
}

static inline void test_set_now_ms(uint64_t t) { g_now_ms = t; }
static inline void test_tick_ms(uint64_t d) { g_now_ms += d; }

// ---------- Test instrumentation ----------
typedef struct {
    uint32_t id_seen[64];
    size_t count;
} call_trace_t;

typedef struct {
    call_trace_t *trace;
    _zp_periodic_scheduler_t *sched;
    uint32_t my_id;
    bool cancel_self;
    bool remove_other;
    uint32_t other_id;
    int *drops_counter;
} cb_ctx_t;

static void task_cb(void *arg) {
    cb_ctx_t *ctx = (cb_ctx_t *)arg;
    if (ctx->trace && ctx->trace->count < 64) {
        ctx->trace->id_seen[ctx->trace->count++] = ctx->my_id;
    }
    if (ctx->cancel_self) {
        (void)_zp_periodic_scheduler_remove(ctx->sched, ctx->my_id);
    }
    if (ctx->remove_other && ctx->other_id != 0u) {
        (void)_zp_periodic_scheduler_remove(ctx->sched, ctx->other_id);
        // make removal single-shot to avoid repeated attempts in later periods
        ctx->remove_other = false;
        ctx->other_id = 0;
    }
}

static void drop_cb(void *arg) {
    cb_ctx_t *ctx = (cb_ctx_t *)arg;
    if (ctx && ctx->drops_counter) {
        (*ctx->drops_counter)++;
    }
}

static _zp_closure_periodic_task_t mk_closure(_zp_closure_periodic_task_callback_t call, z_closure_drop_callback_t drop,
                                              void *ctx) {
    _zp_closure_periodic_task_t c = {.context = ctx, .call = call, .drop = drop};
    return c;
}

static void init_scheduler(_zp_periodic_scheduler_t *s) {
    ASSERT_OK(_zp_periodic_scheduler_init(s));
    // inject controllable time
    ASSERT_OK(_zp_periodic_scheduler_set_time_source(s, fake_now, NULL));
    test_set_now_ms(0);
}

// ---------- Tests ----------
static void test_init(void) {
    printf("test_init()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);
    ASSERT_EQ_U32(s._task_count, 0);
    ASSERT_EQ_U32(s._next_id, 1);
    ASSERT_FALSE(s._task_running);
    ASSERT_EQ_PTR(s._inflight, NULL);
    ASSERT_EQ_U32(s._inflight_id, 0);

    _zp_periodic_scheduler_clear(&s);
}

static void test_add_and_remove_basic(void) {
    printf("test_add_and_remove_basic()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);
    call_trace_t tr = {0};
    cb_ctx_t c1 = {.trace = &tr, .sched = &s};
    uint32_t id1 = 0;
    _zp_closure_periodic_task_t closure = mk_closure(task_cb, drop_cb, &c1);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &closure, 100, &id1));
    ASSERT_TRUE(id1 != 0);
    ASSERT_EQ_U32(s._task_count, 1);

    ASSERT_OK(_zp_periodic_scheduler_remove(&s, id1));
    ASSERT_EQ_U32(s._task_count, 0);

    _zp_periodic_scheduler_clear(&s);
}

static void test_ordering_by_next_due_then_id(void) {
    printf("test_ordering_by_next_due_then_id()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    call_trace_t tr = {0};
    cb_ctx_t c1 = {.trace = &tr, .sched = &s};  // period 200
    cb_ctx_t c2 = {.trace = &tr, .sched = &s};  // period 100

    uint32_t id1 = 0, id2 = 0;
    _zp_closure_periodic_task_t closure1 = mk_closure(task_cb, drop_cb, &c1);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &closure1, 200, &id1));
    c1.my_id = id1;

    _zp_closure_periodic_task_t closure2 = mk_closure(task_cb, drop_cb, &c2);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &closure2, 100, &id2));
    c2.my_id = id2;

    // t = 100: only task2 is due
    test_tick_ms(100);
    (void)_zp_periodic_scheduler_process_tasks(&s);
    ASSERT_EQ_U32(tr.count, 1);
    ASSERT_EQ_U32(tr.id_seen[0], id2);

    // t = 200: task1 (due=200) and task2 (rescheduled to 200) are both due
    // The scheduler processes BOTH in one call, ordering by (_next_due_ms, then _id)
    test_tick_ms(100);
    (void)_zp_periodic_scheduler_process_tasks(&s);
    ASSERT_EQ_U32(tr.count, 3);
    ASSERT_EQ_U32(tr.id_seen[1], id1);  // lower id first at the tie
    ASSERT_EQ_U32(tr.id_seen[2], id2);

    _zp_periodic_scheduler_clear(&s);
}

static void test_periodic_reschedules(void) {
    printf("test_periodic_reschedules()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    call_trace_t tr = {0};
    cb_ctx_t c = {.trace = &tr, .sched = &s};
    uint32_t id = 0;
    _zp_closure_periodic_task_t closure = mk_closure(task_cb, drop_cb, &c);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &closure, 50, &id));
    c.my_id = id;

    // Fire 5 times at 50ms,100ms,150ms,200ms,250ms
    for (int i = 0; i < 5; i++) {
        test_tick_ms(50);
        (void)_zp_periodic_scheduler_process_tasks(&s);
        ASSERT_EQ_U32(tr.count, (size_t)(i + 1));
        ASSERT_EQ_U32(tr.id_seen[i], id);
    }

    _zp_periodic_scheduler_clear(&s);
}

static void test_self_cancel_inside_callback(void) {
    printf("test_self_cancel_inside_callback()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    call_trace_t tr = {0};
    cb_ctx_t c = {.trace = &tr, .sched = &s, .cancel_self = true};
    uint32_t id = 0;
    _zp_closure_periodic_task_t closure = mk_closure(task_cb, drop_cb, &c);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &closure, 30, &id));
    c.my_id = id;

    test_tick_ms(30);
    (void)_zp_periodic_scheduler_process_tasks(&s);
    ASSERT_EQ_U32(tr.count, 1);

    // Advance another period -> should NOT fire again
    test_tick_ms(30);
    (void)_zp_periodic_scheduler_process_tasks(&s);
    ASSERT_EQ_U32(tr.count, 1);  // unchanged

    _zp_periodic_scheduler_clear(&s);
}

static void test_remove_other_inside_callback(void) {
    printf("test_remove_other_inside_callback()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    call_trace_t tr = {0};
    int drops = 0;

    // Two tasks with the same period; the one added first (lower id) will fire first at the tie
    cb_ctx_t c1 = {.trace = &tr, .sched = &s, .remove_other = true};  // will remove c2 inside its callback
    cb_ctx_t c2 = {.trace = &tr, .sched = &s, .drops_counter = &drops};

    uint32_t id1 = 0, id2 = 0;
    _zp_closure_periodic_task_t cl1 = mk_closure(task_cb, drop_cb, &c1);
    _zp_closure_periodic_task_t cl2 = mk_closure(task_cb, drop_cb, &c2);

    ASSERT_OK(_zp_periodic_scheduler_add(&s, &cl1, 100, &id1));
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &cl2, 100, &id2));
    c1.my_id = id1;
    c1.other_id = id2;
    c2.my_id = id2;

    // At t=100 both are due; c1 (lower id) runs first and removes c2
    test_tick_ms(100);
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));

    // Only c1 should have fired; c2 was removed pre-exec and its drop() called once
    ASSERT_EQ_U32(tr.count, 1);
    ASSERT_EQ_U32(tr.id_seen[0], id1);
    ASSERT_EQ_I32(drops, 1);
    ASSERT_EQ_U32(s._task_count, 1);  // only c1 remains scheduled

    // Next period, c1 fires again; c2 is gone
    test_tick_ms(100);
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));
    ASSERT_EQ_U32(tr.count, 2);
    ASSERT_EQ_U32(tr.id_seen[1], id1);

    _zp_periodic_scheduler_clear(&s);
}

static void test_drop_called_on_remove_and_clear(void) {
    printf("test_drop_called_on_remove_and_clear()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);
    int drops = 0;
    cb_ctx_t c1 = {.drops_counter = &drops, .sched = &s};
    uint32_t id = 0;
    _zp_closure_periodic_task_t closure1 = mk_closure(task_cb, drop_cb, &c1);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &closure1, 100, &id));
    ASSERT_EQ_I32(drops, 0);

    // Remove -> drop should be called once
    ASSERT_OK(_zp_periodic_scheduler_remove(&s, id));
    ASSERT_EQ_I32(drops, 1);

    // Add two more, then clear -> drop for each remaining
    cb_ctx_t c2 = {.drops_counter = &drops, .sched = &s};
    cb_ctx_t c3 = {.drops_counter = &drops, .sched = &s};
    uint32_t id2 = 0, id3 = 0;
    _zp_closure_periodic_task_t closure2 = mk_closure(task_cb, drop_cb, &c2);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &closure2, 100, &id2));
    _zp_closure_periodic_task_t closure3 = mk_closure(task_cb, drop_cb, &c3);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &closure3, 100, &id3));
    _zp_periodic_scheduler_clear(&s);
    ASSERT_EQ_I32(drops, 1 /*remove*/ + 2 /*clear*/);
}

static void test_add_invalid_period_zero(void) {
    printf("test_add_invalid_period_zero()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    uint32_t id = 0;
    _zp_closure_periodic_task_t cl = {.context = NULL, .call = NULL, .drop = NULL};
    ASSERT_ERR(_zp_periodic_scheduler_add(&s, &cl, 0 /* invalid */, &id), _Z_ERR_INVALID);

    _zp_periodic_scheduler_clear(&s);
}

static void test_remove_nonexistent(void) {
    printf("test_remove_nonexistent()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    ASSERT_ERR(_zp_periodic_scheduler_remove(&s, 123456u), _Z_ERR_INVALID);
    ASSERT_EQ_U32(s._task_count, 0);

    _zp_periodic_scheduler_clear(&s);
}

static void test_id_wrap_skips_zero(void) {
    printf("test_id_wrap_skips_zero()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    // Force next_id to wrap: next -> UINT32_MAX, then (would be 0) must skip to 1
    s._next_id = UINT32_MAX;

    cb_ctx_t cA = {.sched = &s}, cB = {.sched = &s};
    uint32_t idA = 0, idB = 0;
    _zp_closure_periodic_task_t clA = {.context = &cA, .call = task_cb, .drop = drop_cb};
    _zp_closure_periodic_task_t clB = {.context = &cB, .call = task_cb, .drop = drop_cb};

    ASSERT_OK(_zp_periodic_scheduler_add(&s, &clA, 100, &idA));
    ASSERT_EQ_U32(idA, UINT32_MAX);

    ASSERT_OK(_zp_periodic_scheduler_add(&s, &clB, 100, &idB));
    ASSERT_TRUE(idB != 0);  // MUST NOT be 0
    ASSERT_EQ_U32(idB, 1);  // Should skip 0 to 1

    _zp_periodic_scheduler_clear(&s);
}

static void test_capacity_limit(void) {
#ifdef ZP_PERIODIC_SCHEDULER_MAX_TASKS
    printf("test_capacity_limit()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    const size_t N = (size_t)ZP_PERIODIC_SCHEDULER_MAX_TASKS;
    cb_ctx_t ctxs[ZP_PERIODIC_SCHEDULER_MAX_TASKS] = {0};  // zero-init to keep drops_counter NULL
    uint32_t ids[ZP_PERIODIC_SCHEDULER_MAX_TASKS];
    int drops_total = 0;

    for (size_t i = 0; i < N; i++) {
        ctxs[i].sched = &s;
        ctxs[i].drops_counter = &drops_total;  // count drops for all tasks
        _zp_closure_periodic_task_t cl = {.context = &ctxs[i], .call = task_cb, .drop = drop_cb};
        ASSERT_OK(_zp_periodic_scheduler_add(&s, &cl, 100, &ids[i]));
    }
    ASSERT_EQ_U32(s._task_count, (uint32_t)N);

    // One more should fail with _Z_ERR_GENERIC (per implementation)
    cb_ctx_t extra = {.sched = &s};
    uint32_t id_extra = 0;
    _zp_closure_periodic_task_t clx = {.context = &extra, .call = task_cb, .drop = drop_cb};
    ASSERT_ERR(_zp_periodic_scheduler_add(&s, &clx, 100, &id_extra), _Z_ERR_GENERIC);
    ASSERT_EQ_U32(s._task_count, (uint32_t)N);

    _zp_periodic_scheduler_clear(&s);
    ASSERT_EQ_I32(drops_total, (int)N);  // every capacity task should be dropped exactly once
#else
    (void)printf("test_capacity_limit() skipped: ZP_PERIODIC_SCHEDULER_MAX_TASKS not defined\n");
#endif
}

// --- Helper that adds a task during its callback --------------------------
typedef struct {
    _zp_periodic_scheduler_t *sched;
    call_trace_t *trace;  // pass trace so the child can record hits
    uint32_t *out_id;     // return the new task's id
} adder_ctx_t;

static void adder_drop(void *arg) { (void)arg; }

static void adder_cb(void *arg) {
    adder_ctx_t *ac = (adder_ctx_t *)arg;

    // Child task that will record its own executions
    static cb_ctx_t child;  // static: lifetime outlives the scheduler
    memset(&child, 0, sizeof child);
    child.sched = ac->sched;
    child.trace = ac->trace;

    _zp_closure_periodic_task_t cl = {.context = &child, .call = task_cb, .drop = drop_cb};

    // Add child with 30ms period
    ASSERT_OK(_zp_periodic_scheduler_add(ac->sched, &cl, 30, ac->out_id));
    child.my_id = *ac->out_id;  // set after we know the id
}
// -------------------------------------------------------------------------

static void test_add_during_callback(void) {
    printf("test_add_during_callback()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    call_trace_t tr = {0};
    uint32_t new_id = 0;

    adder_ctx_t ac = {.sched = &s, .trace = &tr, .out_id = &new_id};
    _zp_closure_periodic_task_t addA = {.context = &ac, .call = adder_cb, .drop = adder_drop};

    uint32_t idA = 0;
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &addA, 50, &idA));

    // t=50: A runs, which adds B (period 30, due at t=80)
    test_tick_ms(50);
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));
    ASSERT_TRUE(new_id != 0);

    // t=80: B fires and records itself
    test_tick_ms(30);
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));
    ASSERT_EQ_U32(tr.count, 1);
    ASSERT_EQ_U32(tr.id_seen[0], new_id);

    _zp_periodic_scheduler_clear(&s);
}

static void test_remove_twice_drop_once(void) {
    printf("test_remove_twice_drop_once()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);
    int drops = 0;
    cb_ctx_t c = {.drops_counter = &drops, .sched = &s};
    uint32_t id = 0;
    _zp_closure_periodic_task_t cl = {.context = &c, .call = task_cb, .drop = drop_cb};

    ASSERT_OK(_zp_periodic_scheduler_add(&s, &cl, 100, &id));
    ASSERT_EQ_I32(drops, 0);

    ASSERT_OK(_zp_periodic_scheduler_remove(&s, id));
    ASSERT_EQ_I32(drops, 1);

    ASSERT_ERR(_zp_periodic_scheduler_remove(&s, id), _Z_ERR_INVALID);
    ASSERT_EQ_I32(drops, 1);  // no extra drop

    _zp_periodic_scheduler_clear(&s);
}

static void test_time_regression_safe(void) {
    printf("test_time_regression_safe()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);
    test_set_now_ms(100);

    call_trace_t tr = {0};
    cb_ctx_t c = {.trace = &tr, .sched = &s};
    uint32_t id = 0;
    _zp_closure_periodic_task_t cl = {.context = &c, .call = task_cb, .drop = drop_cb};
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &cl, 50, &id));  // due at t=150
    c.my_id = id;

    // Go backwards in time
    test_set_now_ms(0);
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));  // nothing due
    ASSERT_EQ_U32(tr.count, 0);

    // Now forward to due time
    test_set_now_ms(150);
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));  // should run once
    ASSERT_EQ_U32(tr.count, 1);
    ASSERT_EQ_U32(tr.id_seen[0], id);

    _zp_periodic_scheduler_clear(&s);
}

// When time jumps forward by multiple periods without processing,
// only a single callback should run; the task is rescheduled past 'now'
static void test_skip_missed_periods(void) {
    printf("test_skip_missed_periods()\n");

    _zp_periodic_scheduler_t s;
    init_scheduler(&s);

    call_trace_t tr = {0};
    cb_ctx_t c = {.trace = &tr, .sched = &s};
    uint32_t id = 0;
    _zp_closure_periodic_task_t cl = mk_closure(task_cb, drop_cb, &c);
    ASSERT_OK(_zp_periodic_scheduler_add(&s, &cl, 50, &id));
    c.my_id = id;

    // Jump ahead 5 periods without processing: now = 250ms, due was 50ms
    test_set_now_ms(250);
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));
    ASSERT_EQ_U32(tr.count, 1);  // fired exactly once
    ASSERT_EQ_U32(tr.id_seen[0], id);

    // Another immediate process shouldn't fire again (rescheduled to 300ms)
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));
    ASSERT_EQ_U32(tr.count, 1);

    // Advance to the next scheduled time; should fire once more
    test_set_now_ms(300);
    ASSERT_OK(_zp_periodic_scheduler_process_tasks(&s));
    ASSERT_EQ_U32(tr.count, 2);

    _zp_periodic_scheduler_clear(&s);
}

// With single-threading, start/stop APIs are stubbed and should return errors
static void test_start_stop_when_single_thread_disabled(void) {
#if Z_FEATURE_MULTI_THREAD == 0
    printf("test_start_stop_when_single_thread_disabled()\n");
    _zp_periodic_scheduler_t s;
    init_scheduler(&s);
    _z_task_t task;
    z_task_attr_t attr = (z_task_attr_t){0};
    ASSERT_ERR(_zp_periodic_scheduler_start_task(&s, &attr, &task), _Z_ERR_GENERIC);
    ASSERT_ERR(_zp_periodic_scheduler_stop_task(&s), _Z_ERR_GENERIC);
    _zp_periodic_scheduler_clear(&s);
#else
    (void)printf("test_start_stop_when_single_thread_disabled() skipped: MT enabled\n");
#endif
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_init();
    test_add_and_remove_basic();
    test_ordering_by_next_due_then_id();
    test_periodic_reschedules();
    test_self_cancel_inside_callback();
    test_remove_other_inside_callback();
    test_drop_called_on_remove_and_clear();
    test_add_invalid_period_zero();
    test_remove_nonexistent();
    test_id_wrap_skips_zero();
    test_capacity_limit();
    test_add_during_callback();
    test_remove_twice_drop_once();
    test_time_regression_safe();
    test_skip_missed_periods();
    test_start_stop_when_single_thread_disabled();

    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf(
        "Missing config token to build this test. This test requires: Z_FEATURE_PERIODIC_TASKS and"
        " Z_FEATURE_UNSTABLE_API\n");
    return 0;
}

#endif
