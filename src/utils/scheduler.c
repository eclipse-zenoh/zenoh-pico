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

#include "zenoh-pico/utils/scheduler.h"

#include <string.h>

#ifdef Z_FEATURE_UNSTABLE_API
#if Z_FEATURE_PERIODIC_TASKS == 1

#define _ZP_PERIODIC_SCHEDULER_DEFAULT_WAIT_MS 1000u

// Default time provider: use this scheduler's epoch and the platform clock
static uint64_t _zp_periodic_scheduler_default_now_ms(void *ctx) {
    _zp_periodic_scheduler_t *scheduler = (_zp_periodic_scheduler_t *)ctx;
    return (uint64_t)z_clock_elapsed_ms(&scheduler->_epoch);
}

static inline uint64_t __unsafe_zp_periodic_scheduler_now_ms(const _zp_periodic_scheduler_t *scheduler) {
    if (scheduler->_time.now_ms) {
        return scheduler->_time.now_ms(scheduler->_time.ctx);
    }
    // Fallback if not initialized (should not happen after init)
    return (uint64_t)z_clock_elapsed_ms((z_clock_t *)&scheduler->_epoch);
}

static inline uint32_t __unsafe_zp_periodic_scheduler_next_id(_zp_periodic_scheduler_t *scheduler) {
    uint32_t id = scheduler->_next_id++;
    if (id == _ZP_PERIODIC_SCHEDULER_INVALID_ID) {
        id = scheduler->_next_id++;
    }
    return id;
}

static inline uint64_t __unsafe_zp_periodic_scheduler_compute_wait_ms(_zp_periodic_scheduler_t *scheduler) {
    uint64_t wait_ms = _ZP_PERIODIC_SCHEDULER_DEFAULT_WAIT_MS;
    if (!_zp_periodic_task_list_is_empty(scheduler->_tasks)) {
        _zp_periodic_task_t *head = _zp_periodic_task_list_value(scheduler->_tasks);
        uint64_t now = __unsafe_zp_periodic_scheduler_now_ms(scheduler);
        wait_ms = (head->_next_due_ms > now) ? (head->_next_due_ms - now) : 0u;
    }
    return wait_ms;
}

z_result_t _zp_periodic_scheduler_init(_zp_periodic_scheduler_t *scheduler) {
    if (scheduler == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    *scheduler = (_zp_periodic_scheduler_t){0};
#if Z_FEATURE_MULTI_THREAD == 1
    z_result_t res = _z_mutex_init(&scheduler->_mutex);
    if (res != _Z_RES_OK) {
        _Z_ERROR_RETURN(res);
    }
    res = _z_condvar_init(&scheduler->_condvar);
    if (res != _Z_RES_OK) {
        _z_mutex_drop(&scheduler->_mutex);
        _Z_ERROR_RETURN(res);
    }
#endif
    scheduler->_tasks = _zp_periodic_task_list_new();
    scheduler->_epoch = z_clock_now();
    scheduler->_time.now_ms = _zp_periodic_scheduler_default_now_ms;
    scheduler->_time.ctx = scheduler;
    scheduler->_next_id = 1;
    scheduler->_inflight_id = _ZP_PERIODIC_SCHEDULER_INVALID_ID;
    return _Z_RES_OK;
}

void _zp_periodic_scheduler_clear(_zp_periodic_scheduler_t *scheduler) {
    if (scheduler == NULL) {
        return;
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_lock(&scheduler->_mutex);
#endif
    scheduler->_task_running = false;
#if Z_FEATURE_MULTI_THREAD == 1
    _z_condvar_signal(&scheduler->_condvar);
    if (scheduler->_inflight != NULL) {
        scheduler->_inflight->_cancelled = true;
        do {
            _z_condvar_wait(&scheduler->_condvar, &scheduler->_mutex);
        } while (scheduler->_inflight != NULL);
    }
#endif

    if (scheduler->_tasks != NULL) {
        _zp_periodic_task_list_free(&scheduler->_tasks);
    }
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&scheduler->_mutex);
    _z_condvar_drop(&scheduler->_condvar);
    _z_mutex_drop(&scheduler->_mutex);
#endif
    *scheduler = (_zp_periodic_scheduler_t){0};
}

z_result_t _zp_periodic_scheduler_add(_zp_periodic_scheduler_t *scheduler, const _zp_closure_periodic_task_t *closure,
                                      uint64_t period_ms, uint32_t *id) {
    if (scheduler == NULL || closure == NULL || id == NULL || period_ms == 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    z_result_t res = _Z_RES_OK;
#if Z_FEATURE_MULTI_THREAD == 1
    res = _z_mutex_lock(&scheduler->_mutex);
    if (res != _Z_RES_OK) {
        _Z_ERROR_RETURN(res);
    }
#endif

    if (scheduler->_task_count >= ZP_PERIODIC_SCHEDULER_MAX_TASKS) {
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_unlock(&scheduler->_mutex);
#endif
        _Z_ERROR("Periodic scheduler task limit reached: %zu >= %u", scheduler->_task_count,
                 ZP_PERIODIC_SCHEDULER_MAX_TASKS);
        return _Z_ERR_GENERIC;
    }

    uint64_t now_ms = __unsafe_zp_periodic_scheduler_now_ms(scheduler);

    _zp_periodic_task_t *task = (_zp_periodic_task_t *)z_malloc(sizeof(_zp_periodic_task_t));
    if (task == NULL) {
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_unlock(&scheduler->_mutex);
#endif
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    *task =
        (_zp_periodic_task_t){._id = __unsafe_zp_periodic_scheduler_next_id(scheduler),
                              ._period_ms = period_ms,
                              ._next_due_ms = now_ms + period_ms,
                              ._closure = {.context = closure->context, .call = closure->call, .drop = closure->drop},
                              ._cancelled = false};

    size_t expected_len = (scheduler->_inflight != NULL) ? scheduler->_task_count : scheduler->_task_count + 1;
    scheduler->_tasks = _zp_periodic_task_list_push_sorted(scheduler->_tasks, task);
    size_t new_len = _zp_periodic_task_list_len(scheduler->_tasks);

    if (new_len == expected_len) {
        scheduler->_task_count++;
        *id = task->_id;
#if Z_FEATURE_MULTI_THREAD == 1
        _zp_periodic_task_t *head = _zp_periodic_task_list_value(scheduler->_tasks);
        if (_zp_periodic_task_eq(head, task)) {
            _z_condvar_signal(&scheduler->_condvar);
        }
#endif
    } else {
        _Z_ERROR("Failed to add periodic task");
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_unlock(&scheduler->_mutex);
#endif
        _zp_periodic_task_clear(task);
        z_free(task);
        return _Z_ERR_GENERIC;
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&scheduler->_mutex);
#endif
    return res;
}

z_result_t _zp_periodic_scheduler_remove(_zp_periodic_scheduler_t *scheduler, uint32_t id) {
    if (scheduler == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    z_result_t res = _Z_RES_OK;
#if Z_FEATURE_MULTI_THREAD == 1
    res = _z_mutex_lock(&scheduler->_mutex);
    if (res != _Z_RES_OK) {
        _Z_ERROR_RETURN(res);
    }
#endif

    bool removed = false;
    if (!_zp_periodic_task_list_is_empty(scheduler->_tasks)) {
        size_t expected_len = (scheduler->_inflight != NULL) ? scheduler->_task_count - 2 : scheduler->_task_count - 1;
        scheduler->_tasks = _zp_periodic_task_list_drop_filter(scheduler->_tasks, _zp_periodic_task_eq,
                                                               &(const _zp_periodic_task_t){._id = id});
        size_t new_len = _zp_periodic_task_list_len(scheduler->_tasks);
        if (new_len == expected_len) {
            scheduler->_task_count--;
            removed = true;
        }
    }

    if (!removed) {
        if (scheduler->_inflight != NULL && scheduler->_inflight->_id == id) {
            scheduler->_inflight->_cancelled = true;
            removed = true;
        } else {
            _Z_WARN("Failed to remove periodic task with id %u", id);
        }
    }

#if Z_FEATURE_MULTI_THREAD == 1
    if (removed) {
        _z_condvar_signal(&scheduler->_condvar);
    }
#endif

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&scheduler->_mutex);
#endif
    res = removed ? _Z_RES_OK : _Z_ERR_INVALID;
    return res;
}

z_result_t _zp_periodic_scheduler_process_tasks(_zp_periodic_scheduler_t *scheduler) {
    if (scheduler == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    z_result_t res = _Z_RES_OK;
#if Z_FEATURE_MULTI_THREAD == 1
    res = _z_mutex_lock(&scheduler->_mutex);
    if (res != _Z_RES_OK) {
        _Z_ERROR_RETURN(res);
    }
#endif

    while (!_zp_periodic_task_list_is_empty(scheduler->_tasks)) {
        _zp_periodic_task_t *head = _zp_periodic_task_list_value(scheduler->_tasks);
        uint64_t now_ms = __unsafe_zp_periodic_scheduler_now_ms(scheduler);
        if (head->_next_due_ms > now_ms) {
            break;
        }

        _zp_periodic_task_t *task = NULL;
        scheduler->_tasks = _zp_periodic_task_list_pop(scheduler->_tasks, &task);
        task->_cancelled = false;
        scheduler->_inflight = task;
        scheduler->_inflight_id = task->_id;

#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_unlock(&scheduler->_mutex);
#endif

        if (task->_closure.call != NULL) {
            task->_closure.call(task->_closure.context);
        }

#if Z_FEATURE_MULTI_THREAD == 1
        res = _z_mutex_lock(&scheduler->_mutex);
        if (res != _Z_RES_OK) {
            _Z_ERROR("Failed to relock mutex for tick periodic scheduler - dropping task: %u", task->_id);
            _zp_periodic_task_clear(task);
            z_free(task);
            _Z_ERROR_RETURN(res);
        }
#endif
        scheduler->_inflight = NULL;
        scheduler->_inflight_id = _ZP_PERIODIC_SCHEDULER_INVALID_ID;
#if Z_FEATURE_MULTI_THREAD == 1
        _z_condvar_signal(&scheduler->_condvar);
#endif

        if (task->_cancelled) {
            scheduler->_task_count--;
            _zp_periodic_task_clear(task);
            z_free(task);
            continue;
        }

        // Recalculate next due time
        now_ms = __unsafe_zp_periodic_scheduler_now_ms(scheduler);
        uint64_t delta = now_ms - task->_next_due_ms;
        uint64_t periods = (delta / task->_period_ms) + 1ULL;
        task->_next_due_ms += periods * task->_period_ms;

        // Add the task back to the list
        scheduler->_tasks = _zp_periodic_task_list_push_sorted(scheduler->_tasks, task);
        size_t new_len = _zp_periodic_task_list_len(scheduler->_tasks);

        if (new_len != scheduler->_task_count) {
            _Z_ERROR("Failed to re-add periodic task with id %u", task->_id);
            scheduler->_task_count--;
            _zp_periodic_task_clear(task);
            z_free(task);
            continue;
        }
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&scheduler->_mutex);
#endif

    return res;
}

z_result_t _zp_periodic_scheduler_set_time_source(_zp_periodic_scheduler_t *scheduler, uint64_t (*now_ms)(void *ctx),
                                                  void *ctx) {
    if (scheduler == NULL) {
        return _Z_ERR_INVALID;
    }
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_lock(&scheduler->_mutex));
#endif
    scheduler->_time.now_ms = now_ms ? now_ms : _zp_periodic_scheduler_default_now_ms;
    scheduler->_time.ctx = now_ms ? ctx : scheduler;
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&scheduler->_mutex);
#endif
    return _Z_RES_OK;
}

#if Z_FEATURE_MULTI_THREAD == 1

void *_zp_periodic_scheduler_task(void *arg) {
    _zp_periodic_scheduler_t *scheduler = (_zp_periodic_scheduler_t *)arg;
    while (scheduler->_task_running) {
        z_result_t res = _z_mutex_lock(&scheduler->_mutex);
        if (res != _Z_RES_OK) {
            _Z_ERROR("Failed to lock mutex for periodic scheduler task");
            continue;
        }
        if (!scheduler->_task_running) {
            _z_mutex_unlock(&scheduler->_mutex);
            break;
        }

        uint64_t wait_ms = __unsafe_zp_periodic_scheduler_compute_wait_ms(scheduler);
        if (wait_ms > 0) {
            z_clock_t wait_abs = z_clock_now();
            z_clock_advance_ms(&wait_abs, wait_ms);
            _z_condvar_wait_until(&scheduler->_condvar, &scheduler->_mutex, &wait_abs);
        }

        if (!scheduler->_task_running) {
            _z_mutex_unlock(&scheduler->_mutex);
            break;
        }

        _z_mutex_unlock(&scheduler->_mutex);
        res = _zp_periodic_scheduler_process_tasks(scheduler);
        if (res != _Z_RES_OK) {
            _Z_ERROR("Periodic scheduler processjobs failed: %d", res);
        }
    }

    return NULL;
}

z_result_t _zp_periodic_scheduler_start_task(_zp_periodic_scheduler_t *scheduler, z_task_attr_t *attr,
                                             _z_task_t *task) {
    (void)memset(task, 0, sizeof(_z_task_t));

    z_result_t res = _z_mutex_lock(&scheduler->_mutex);
    if (res != _Z_RES_OK) {
        _Z_ERROR_RETURN(res);
    }
    if (scheduler->_task_running) {
        _Z_ERROR("Periodic scheduler task already running");
        _z_mutex_unlock(&scheduler->_mutex);
        return _Z_ERR_GENERIC;
    }

    scheduler->_task_running = true;
    if (_z_task_init(task, attr, _zp_periodic_scheduler_task, scheduler) != _Z_RES_OK) {
        scheduler->_task_running = false;
        _z_mutex_unlock(&scheduler->_mutex);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_TASK_FAILED);
    }
    _z_mutex_unlock(&scheduler->_mutex);
    return _Z_RES_OK;
}

z_result_t _zp_periodic_scheduler_stop_task(_zp_periodic_scheduler_t *scheduler) {
    if (scheduler == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    z_result_t res = _z_mutex_lock(&scheduler->_mutex);
    if (res != _Z_RES_OK) {
        _Z_ERROR("Failed to lock mutex for stopping periodic scheduler task");
        return _Z_ERR_GENERIC;
    }
    scheduler->_task_running = false;
    _z_condvar_signal(&scheduler->_condvar);
    _z_mutex_unlock(&scheduler->_mutex);
    return _Z_RES_OK;
}
#else

void *_zp_periodic_scheduler_task(void *scheduler) {
    _ZP_UNUSED(scheduler);
    return NULL;
}

z_result_t _zp_periodic_scheduler_start_task(_zp_periodic_scheduler_t *scheduler, z_task_attr_t *attr,
                                             _z_task_t *task) {
    _ZP_UNUSED(scheduler);
    _ZP_UNUSED(attr);
    _ZP_UNUSED(task);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

z_result_t _zp_periodic_scheduler_stop_task(_zp_periodic_scheduler_t *scheduler) {
    _ZP_UNUSED(scheduler);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}
#endif  // Z_FEATURE_MULTI_THREAD == 1
#endif  // Z_FEATURE_PERIODIC_TASKS == 1
#endif  // Z_FEATURE_UNSTABLE_API
