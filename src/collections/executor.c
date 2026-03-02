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

#include "zenoh-pico/collections/executor.h"

_z_fut_handle_t _z_executor_spawn(_z_executor_t *executor, _z_fut_t *fut) {
    _z_fut_data_t fut_data;
    _z_fut_move(&fut_data._fut, fut);
    fut_data._status = _Z_FUT_STATUS_RUNNING;
    _z_fut_data_hmap_node_t *fut_node_ptr =
        _z_fut_data_hmap_insert(&executor->_tasks, &executor->_next_fut_id, &fut_data);
    _z_fut_handle_t handle;
    if (fut_node_ptr == NULL) {
        handle.is_valid = false;
        return handle;
    }
    handle._id = executor->_next_fut_id;
    handle.is_valid = true;
    executor->_next_fut_id++;
    _z_fut_data_hmap_node_ptr_deque_push_back(
        &executor->_ready_tasks,
        &fut_node_ptr);  // can't fail since we have enough capacity for all tasks in the hashmap
    return handle;
}

_z_executor_spin_result_t _z_executor_get_next_fut(_z_executor_t *executor, _z_fut_data_hmap_node_t **task) {
    _z_executor_spin_result_t result;
    result.status = _Z_EXECUTOR_SPIN_RESULT_NO_TASKS;
    _z_sleeping_fut_data_ptr_t *sleeping_task_ptr = _z_sleeping_fut_data_ptr_pqueue_peek(&executor->_sleeping_tasks);
    if (sleeping_task_ptr != NULL) {
        z_clock_t now = z_clock_now();
        z_clock_t wake_up_time = executor->_epoch;
        z_clock_advance_ms(&wake_up_time, sleeping_task_ptr->_wake_up_time_ms);
        if (zp_clock_elapsed_ms_since(&now, &wake_up_time) > 0) {
            // The sleeping task is ready to execute
            _z_sleeping_fut_data_ptr_t t;
            _z_sleeping_fut_data_ptr_pqueue_pop(&executor->_sleeping_tasks, &t);
            if (_z_fut_data_hmap_node_ptr_deque_pop_front(&executor->_ready_tasks, task)) {
                // We have a non-sleeping task to execute, we should re-enqueue the ready sleeping task as non-sleeping
                // one and execute the non-sleeping task first.
                _z_fut_data_hmap_node_ptr_deque_push_back(&executor->_ready_tasks, &t._fut);
                t._fut->val._status = _Z_FUT_STATUS_READY;  // Mark the sleeping task as ready since it's now
                                                            // re-enqueued to the ready task queue and can be executed.
            } else {
                *task = t._fut;  // No non-sleeping task, execute the ready sleeping task directly.
            }
            result.status = _Z_EXECUTOR_SPIN_RESULT_EXECUTED_TASK;
        } else if (_z_fut_data_hmap_node_ptr_deque_pop_front(&executor->_ready_tasks, task)) {
            // We have a non-sleeping task to execute
            result.status = _Z_EXECUTOR_SPIN_RESULT_EXECUTED_TASK;
        } else {
            // No non-sleeping task, we should wait for the sleeping task to be ready.
            result.status = _Z_EXECUTOR_SPIN_RESULT_SHOULD_WAIT;
            result.next_wake_up_time = wake_up_time;
        }
    } else if (_z_fut_data_hmap_node_ptr_deque_pop_front(&executor->_ready_tasks, task)) {
        // We have a non-sleeping task to execute
        result.status = _Z_EXECUTOR_SPIN_RESULT_EXECUTED_TASK;
    }
    return result;
}

_z_executor_spin_result_t _z_executor_spin(_z_executor_t *executor) {
    _z_fut_data_hmap_node_t *fut;
    _z_executor_spin_result_t result;
    while (true) {  // Loop until we find non-null task to execute
        result = _z_executor_get_next_fut(executor, &fut);
        if (result.status == _Z_EXECUTOR_SPIN_RESULT_NO_TASKS ||
            result.status == _Z_EXECUTOR_SPIN_RESULT_SHOULD_WAIT) {  // No tasks to execute
            return result;
        }
        if (fut->val._fut._fut_fn == NULL) {  // idle task, just skip it and check the next task.
            _z_fut_data_hmap_remove(&executor->_tasks, &fut->key, NULL);  // Remove the idle task from the task pool
            continue;
        }
        break;
    }
    _z_fut_t *f = &fut->val._fut;
    _z_fut_fn_result_t fn_result = f->_fut_fn(f->_fut_arg, executor);
    if (fn_result._status == _Z_FUT_STATUS_RUNNING) {
        // The task is still running, we should re-enqueue it to the executor.
        fut->val._status = _Z_FUT_STATUS_RUNNING;  // Mark the task as running since it's still running and can be
                                                   // executed in the next spin.
        _z_fut_data_hmap_node_ptr_deque_push_back(
            &executor->_ready_tasks, &fut);  // can't fail since we have enough capacity for all tasks in the hashmap
    } else if (fn_result._status == _Z_FUT_STATUS_SLEEPING) {
        // The task is sleeping, we should move it to the sleeping task queue with the wake-up time.
        fut->val._status = _Z_FUT_STATUS_SLEEPING;  // Mark the task as sleeping since it's now sleeping and can only be
                                                    // executed after the wake-up time.
        _z_sleeping_fut_data_ptr_t sleeping_task;
        sleeping_task._fut = fut;
        sleeping_task._wake_up_time_ms = zp_clock_elapsed_ms_since(&fn_result._wake_up_time, &executor->_epoch);
        _z_sleeping_fut_data_ptr_pqueue_push(
            &executor->_sleeping_tasks,
            &sleeping_task);  // can't fail since we have enough capacity for all sleeping tasks in the hashmap
    } else {
        // The task is ready, we should destroy it to free the resource.
        _z_fut_data_hmap_remove(&executor->_tasks, &fut->key, NULL);
    }
    return result;
}

_z_fut_status_t _z_executor_get_fut_status(const _z_executor_t *executor, const _z_fut_handle_t *handle) {
    if (!handle->is_valid) {
        return _Z_FUT_STATUS_READY;  // Invalid handle is considered as ready (i.e., not running or sleeping)
    }
    _z_fut_data_t *fut_data = _z_fut_data_hmap_get((_z_fut_data_hmap_t *)&executor->_tasks, &handle->_id);
    if (fut_data == NULL) {
        return _Z_FUT_STATUS_READY;  // If the task is not found in the task pool, it means it's already completed and
                                     // removed, we consider it as ready (i.e., not running or sleeping)
    }
    return fut_data->_status;
}

bool _z_executor_cancel_fut(_z_executor_t *executor, const _z_fut_handle_t *handle) {
    if (!handle->is_valid) {
        return false;
    }
    _z_fut_data_t *fut = _z_fut_data_hmap_get(&executor->_tasks, &handle->_id);
    if (fut == NULL) {
        return false;
    }
    _z_fut_data_destroy(fut);

    return true;
}
