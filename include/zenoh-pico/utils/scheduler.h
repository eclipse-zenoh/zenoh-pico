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

#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/system/platform.h"

#ifndef ZENOH_PICO_UTILS_SCHEDULER_H
#define ZENOH_PICO_UTILS_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1

#define _ZP_PERIODIC_SCHEDULER_INVALID_ID 0u

typedef void (*_zp_closure_periodic_task_callback_t)(void *arg);
typedef void (*z_closure_drop_callback_t)(void *arg);  // Forward declaration to avoid cyclical include

typedef struct {
    void *context;
    _zp_closure_periodic_task_callback_t call;
    z_closure_drop_callback_t drop;
} _zp_closure_periodic_task_t;

typedef struct {
    uint32_t _id;
    uint64_t _period_ms;
    uint64_t _next_due_ms;  // ms since scheduler epoch
    _zp_closure_periodic_task_t _closure;
    volatile bool _cancelled;
} _zp_periodic_task_t;

static inline void _zp_periodic_task_clear(_zp_periodic_task_t *task) {
    z_closure_drop_callback_t drop = task->_closure.drop;
    void *context = task->_closure.context;
    *task = (_zp_periodic_task_t){0};
    if (drop != NULL) {
        drop(context);
    };
}

static inline bool _zp_periodic_task_eq(const _zp_periodic_task_t *left, const _zp_periodic_task_t *right) {
    return left->_id == right->_id;
}

static inline int _zp_periodic_task_cmp(const _zp_periodic_task_t *left, const _zp_periodic_task_t *right) {
    if (left->_next_due_ms != right->_next_due_ms) {
        return left->_next_due_ms < right->_next_due_ms ? -1 : 1;
    } else if (left->_id != right->_id) {
        return left->_id < right->_id ? -1 : 1;
    } else {
        return 0;
    }
}

_Z_ELEM_DEFINE(_zp_periodic_task, _zp_periodic_task_t, _z_noop_size, _zp_periodic_task_clear, _z_noop_copy,
               _z_noop_move, _zp_periodic_task_eq, _zp_periodic_task_cmp, _z_noop_hash)
_Z_LIST_DEFINE(_zp_periodic_task, _zp_periodic_task_t)

// Time source used by the scheduler (to allow tests to inject a fake clock)
typedef struct {
    uint64_t (*now_ms)(void *ctx);
    void *ctx;
} _zp_periodic_scheduler_time_source_t;

typedef struct {
    bool _initialized;
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_t _mutex;
    _z_condvar_t _condvar;
#endif
    _zp_periodic_task_list_t *_tasks;
    z_clock_t _epoch;
    size_t _task_count;
    uint32_t _next_id;
    volatile bool _task_running;
    _zp_periodic_scheduler_time_source_t _time;
    _zp_periodic_task_t *_inflight;
    uint32_t _inflight_id;
} _zp_periodic_scheduler_t;

static inline bool _zp_periodic_scheduler_check(const _zp_periodic_scheduler_t *scheduler) {
    return scheduler != NULL && scheduler->_initialized;
}
z_result_t _zp_periodic_scheduler_init(_zp_periodic_scheduler_t *scheduler);
void _zp_periodic_scheduler_clear(_zp_periodic_scheduler_t *scheduler);
z_result_t _zp_periodic_scheduler_add(_zp_periodic_scheduler_t *scheduler, const _zp_closure_periodic_task_t *closure,
                                      uint64_t period_ms, uint32_t *id);
z_result_t _zp_periodic_scheduler_remove(_zp_periodic_scheduler_t *scheduler, uint32_t id);

z_result_t _zp_periodic_scheduler_start_task(_zp_periodic_scheduler_t *scheduler, z_task_attr_t *attr, _z_task_t *task);
z_result_t _zp_periodic_scheduler_stop_task(_zp_periodic_scheduler_t *scheduler);
z_result_t _zp_periodic_scheduler_process_tasks(_zp_periodic_scheduler_t *scheduler);

// Set/override the time source used by the scheduler. Passing NULL resets to default
z_result_t _zp_periodic_scheduler_set_time_source(_zp_periodic_scheduler_t *scheduler, uint64_t (*now_ms)(void *ctx),
                                                  void *ctx);

#endif  // Z_FEATURE_PERIODIC_TASKS == 1
#endif  // Z_FEATURE_UNSTABLE_API

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_UTILS_SCHEDULER_H */
