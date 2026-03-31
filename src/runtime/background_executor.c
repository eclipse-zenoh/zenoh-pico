#include "zenoh-pico/runtime/background_executor.h"

#if Z_FEATURE_MULTI_THREAD == 1
typedef enum {
    _Z_BACKGROUND_EXECUTOR_STATE_STOPPED,
    _Z_BACKGROUND_EXECUTOR_STATE_STARTING,
    _Z_BACKGROUND_EXECUTOR_STATE_RUNNING,
    _Z_BACKGROUND_EXECUTOR_STATE_STOPPING
} _z_background_executor_state_t;

typedef struct _z_background_executor_inner_t {
    _z_executor_t _executor;
    _z_mutex_t _mutex;
    _z_condvar_t _condvar;
    _z_atomic_size_t _waiters;
    bool _stop_requested;
    _z_atomic_size_t _state;
    _z_task_t _task;
} _z_background_executor_inner_t;

z_result_t _z_background_executor_inner_suspend_and_lock(_z_background_executor_inner_t *be) {
    _z_atomic_size_fetch_add(&be->_waiters, 1, _z_memory_order_acq_rel);
    return _z_mutex_lock(&be->_mutex);
}

z_result_t _z_background_executor_inner_suspend(_z_background_executor_inner_t *be) {
    _Z_RETURN_IF_ERR(_z_background_executor_inner_suspend_and_lock(be));
    return _z_mutex_unlock(&be->_mutex);
}

z_result_t _z_background_executor_inner_unlock_and_resume(_z_background_executor_inner_t *be) {
    _z_atomic_size_fetch_sub(&be->_waiters, 1, _z_memory_order_acq_rel);
    _Z_CLEAN_RETURN_IF_ERR(_z_condvar_signal_all(&be->_condvar), _z_mutex_unlock(&be->_mutex));
    return _z_mutex_unlock(&be->_mutex);
}

z_result_t _z_background_executor_inner_resume(_z_background_executor_inner_t *be) {
    _Z_RETURN_IF_ERR(_z_mutex_lock(&be->_mutex));
    return _z_background_executor_inner_unlock_and_resume(be);
}

z_result_t _z_background_executor_inner_run_forever(_z_background_executor_inner_t *be) {
    _Z_RETURN_IF_ERR(_z_mutex_lock(&be->_mutex));
    while (true) {
        while (_z_atomic_size_load(&be->_waiters, _z_memory_order_acquire) > 0) {
            if (be->_stop_requested) {  // extra check to allow to stop executor thread even if there are waiters
                break;                  // stop requested, exit the loop and end the thread
            }
            // if there are waiters, sleep until they are resumed
            _Z_CLEAN_RETURN_IF_ERR(_z_condvar_wait(&be->_condvar, &be->_mutex), _z_mutex_unlock(&be->_mutex));
        }
        if (be->_stop_requested) {
            break;  // stop requested, exit the loop and end the thread
        }
        _z_executor_spin_result_t res = _z_executor_spin(&be->_executor);
        if (res.status == _Z_EXECUTOR_SPIN_RESULT_NO_TASKS) {  // no pending tasks, sleep until next task is added
            _Z_CLEAN_RETURN_IF_ERR(_z_condvar_wait(&be->_condvar, &be->_mutex), _z_mutex_unlock(&be->_mutex));
        } else if (res.status == _Z_EXECUTOR_SPIN_RESULT_SHOULD_WAIT) {  // we have pending timed tasks but they are not
                                                                         // ready yet, sleep until the next one is ready
            z_clock_t now = z_clock_now();
            if (zp_clock_elapsed_ms_since(&res.next_wake_up_time, &now) > 1) {  // sleep until next task is ready
                z_result_t wait_result = _z_condvar_wait_until(&be->_condvar, &be->_mutex, &res.next_wake_up_time);
                if (wait_result != Z_ETIMEDOUT && wait_result != _Z_RES_OK) {
                    return _z_mutex_unlock(&be->_mutex);
                }
            }
        }
    }
    // wake up all waiters so that they can see that executor is no longer running and exit
    _z_condvar_signal_all(&be->_condvar);
    return _z_mutex_unlock(&be->_mutex);
}

static inline bool _is_called_from_executor(const _z_background_executor_inner_t *be) {
    if (_z_atomic_size_load((_z_atomic_size_t *)&be->_state, _z_memory_order_acquire) !=
        _Z_BACKGROUND_EXECUTOR_STATE_RUNNING) {
        return false;  // if executor hasn't started yet, we can't be called from it
    }
    _z_task_id_t current_task_id = _z_task_current_id();
    _z_task_id_t executor_task_id = _z_task_get_id(&be->_task);
    return _z_task_id_equal(&current_task_id, &executor_task_id);
}

z_result_t _z_background_executor_inner_spawn(_z_background_executor_inner_t *be, _z_fut_t *fut,
                                              _z_fut_handle_t *handle) {
    if (_is_called_from_executor(be)) {
        *handle = _z_executor_spawn(&be->_executor, fut);
    } else {
        _Z_RETURN_IF_ERR(_z_background_executor_inner_suspend_and_lock(be));
        *handle = _z_executor_spawn(&be->_executor, fut);
        _z_background_executor_inner_unlock_and_resume(be);
    }
    return _z_fut_handle_is_null(*handle) ? _Z_ERR_SYSTEM_OUT_OF_MEMORY : _Z_RES_OK;
}

z_result_t _z_background_executor_inner_get_fut_status(_z_background_executor_inner_t *be,
                                                       const _z_fut_handle_t *handle, _z_fut_status_t *status_out) {
    if (_is_called_from_executor(be)) {
        *status_out = _z_executor_get_fut_status(&be->_executor, handle);
        return _Z_RES_OK;
    } else {
        _Z_RETURN_IF_ERR(_z_background_executor_inner_suspend_and_lock(be));
        *status_out = _z_executor_get_fut_status(&be->_executor, handle);
        return _z_background_executor_inner_unlock_and_resume(be);
    }
}

z_result_t _z_background_executor_inner_cancel_fut(_z_background_executor_inner_t *be, const _z_fut_handle_t *handle) {
    if (_is_called_from_executor(be)) {
        return _z_executor_cancel_fut(&be->_executor, handle) ? _Z_RES_OK : _Z_ERR_INVALID;
    } else {
        _Z_RETURN_IF_ERR(_z_background_executor_inner_suspend_and_lock(be));
        _z_executor_cancel_fut(&be->_executor, handle);
        return _z_background_executor_inner_unlock_and_resume(be);
    }
}

z_result_t _z_background_executor_inner_stop(_z_background_executor_inner_t *be) {
    _Z_RETURN_IF_ERR(_z_background_executor_inner_suspend_and_lock(be));
    // executor is now suspended, so no API can be called from its thread
    // in addition we are holding the mutex, so no other thread can request to stop or start the executor
    size_t state = _z_atomic_size_load(&be->_state, _z_memory_order_acquire);
    if (state != _Z_BACKGROUND_EXECUTOR_STATE_RUNNING) {
        _Z_RETURN_IF_ERR(_z_background_executor_inner_unlock_and_resume(be));
        if (state == _Z_BACKGROUND_EXECUTOR_STATE_STOPPING) {
            // other thread requested executor to stop, it means that the executor thread is
            // proceeding to its end, and thus it will not execute any more tasks, unless restarted,
            // so it is safe to return success
            return _Z_RES_OK;
        } else if (state == _Z_BACKGROUND_EXECUTOR_STATE_STOPPED) {
            // already stopped
            return _Z_RES_OK;
        } else {  // _Z_BACKGROUND_EXECUTOR_STATE_STARTING
            // this state is unreacheable because the mutex is always held while the state is STARTING
            return _Z_ERR_GENERIC;
        }
    }
    be->_stop_requested = true;
    _z_atomic_size_store(&be->_state, _Z_BACKGROUND_EXECUTOR_STATE_STOPPING, _z_memory_order_release);
    z_result_t ret = _z_background_executor_inner_unlock_and_resume(be);
    // after resume the executor thread will proceed to stop directly without trying to execute any tasks.
    _Z_SET_IF_OK(ret, _z_task_join(&be->_task));
    be->_stop_requested = false;
    _z_atomic_size_store(&be->_state, _Z_BACKGROUND_EXECUTOR_STATE_STOPPED, _z_memory_order_release);
    return ret;
}

void _z_background_executor_inner_clear(_z_background_executor_inner_t *be) {
    _z_background_executor_inner_stop(be);
    _z_executor_destroy(&be->_executor);
    _z_condvar_drop(&be->_condvar);
    _z_mutex_drop(&be->_mutex);
}

void *_z_background_executor_inner_task_fn(void *arg) {
    _z_background_executor_inner_t *be = (_z_background_executor_inner_t *)arg;
    _z_atomic_size_store(&be->_state, _Z_BACKGROUND_EXECUTOR_STATE_RUNNING, _z_memory_order_release);
    _z_background_executor_inner_run_forever(be);
    return NULL;
}

z_result_t _z_background_executor_inner_init_deferred(_z_background_executor_inner_t *be) {
    _Z_RETURN_IF_ERR(_z_mutex_init(&be->_mutex));
    _Z_CLEAN_RETURN_IF_ERR(_z_condvar_init(&be->_condvar), _z_mutex_drop(&be->_mutex));
    _z_executor_init(&be->_executor);
    _z_atomic_size_init(&be->_waiters, 0);
    be->_stop_requested = false;
    _z_atomic_size_init(&be->_state, _Z_BACKGROUND_EXECUTOR_STATE_STOPPED);
    return _Z_RES_OK;
}

z_result_t _z_background_executor_inner_start(_z_background_executor_inner_t *be, z_task_attr_t *task_attr) {
    _Z_RETURN_IF_ERR(_z_mutex_lock(&be->_mutex));
    size_t expected = _Z_BACKGROUND_EXECUTOR_STATE_STOPPED;
    while (!_z_atomic_size_compare_exchange_strong(&be->_state, &expected, _Z_BACKGROUND_EXECUTOR_STATE_STARTING,
                                                   _z_memory_order_acq_rel, _z_memory_order_acquire)) {
        _z_mutex_unlock(&be->_mutex);
        if (expected == _Z_BACKGROUND_EXECUTOR_STATE_STARTING) {
            // this state is unreacheable because the mutex is always held while the state is STARTING
            return _Z_ERR_GENERIC;
        } else if (expected == _Z_BACKGROUND_EXECUTOR_STATE_STOPPING) {
            // other thread requested executor to stop, it means that the executor thread is proceeding to its end.
            // Let's just wait for it
            z_sleep_us(10);
        } else {  // _Z_BACKGROUND_EXECUTOR_STATE_RUNNING
            // already running
            return _Z_RES_OK;
        }
    }
    z_result_t ret = _z_task_init(&be->_task, task_attr, _z_background_executor_inner_task_fn, be);
    if (ret != _Z_RES_OK) {
        _z_atomic_size_store(&be->_state, _Z_BACKGROUND_EXECUTOR_STATE_STOPPED, _z_memory_order_release);
    } else {
        while (_z_atomic_size_load(&be->_state, _z_memory_order_acquire) == _Z_BACKGROUND_EXECUTOR_STATE_STARTING) {
            z_sleep_us(10);  // wait until the executor thread sets the state to RUNNING or any other transition happens
        }
    }
    _z_mutex_unlock(&be->_mutex);
    return ret;
}

z_result_t _z_background_executor_init_deferred(_z_background_executor_t *be) {
    be->_inner = _z_background_executor_inner_rc_null();
    _z_background_executor_inner_t *inner =
        (_z_background_executor_inner_t *)z_malloc(sizeof(_z_background_executor_inner_t));
    if (!inner) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    if (_z_background_executor_inner_init_deferred(inner) != _Z_RES_OK) {
        z_free(inner);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    be->_inner = _z_background_executor_inner_rc_new(inner);
    if (_Z_RC_IS_NULL(&be->_inner)) {
        _z_background_executor_inner_clear(inner);
        z_free(inner);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

z_result_t _z_background_executor_start(_z_background_executor_t *be, z_task_attr_t *task_attr) {
    if (_Z_RC_IS_NULL(&be->_inner)) {
        return _Z_ERR_INVALID;
    }
    return _z_background_executor_inner_start(_Z_RC_IN_VAL(&be->_inner), task_attr);
}

z_result_t _z_background_executor_stop(_z_background_executor_t *be) {
    if (_Z_RC_IS_NULL(&be->_inner)) {
        return _Z_ERR_INVALID;
    }
    return _z_background_executor_inner_stop(_Z_RC_IN_VAL(&be->_inner));
}

z_result_t _z_background_executor_init(_z_background_executor_t *be, z_task_attr_t *task_attr) {
    _Z_RETURN_IF_ERR(_z_background_executor_init_deferred(be));
    _Z_CLEAN_RETURN_IF_ERR(_z_background_executor_start(be, task_attr), _z_background_executor_destroy(be));
    return _Z_RES_OK;
}

z_result_t _z_background_executor_spawn(_z_background_executor_t *be, _z_fut_t *fut, _z_fut_handle_t *handle_out) {
    _z_fut_handle_t dummy_handle;
    if (handle_out == NULL) {
        handle_out = &dummy_handle;
    }
    *handle_out = _z_fut_handle_null();
    if (_Z_RC_IS_NULL(&be->_inner)) {
        return _Z_ERR_INVALID;
    }
    return _z_background_executor_inner_spawn(_Z_RC_IN_VAL(&be->_inner), fut, handle_out);
}

z_result_t _z_background_executor_suspend(_z_background_executor_t *be) {
    if (_Z_RC_IS_NULL(&be->_inner)) {
        return _Z_ERR_INVALID;
    }
    return _z_background_executor_inner_suspend(_Z_RC_IN_VAL(&be->_inner));
}

z_result_t _z_background_executor_resume(_z_background_executor_t *be) {
    if (_Z_RC_IS_NULL(&be->_inner)) {
        return _Z_ERR_INVALID;
    }
    return _z_background_executor_inner_resume(_Z_RC_IN_VAL(&be->_inner));
}

void _z_background_executor_destroy(_z_background_executor_t *be) { _z_background_executor_inner_rc_drop(&be->_inner); }

z_result_t _z_background_executor_get_fut_status(_z_background_executor_t *be, const _z_fut_handle_t *handle,
                                                 _z_fut_status_t *status_out) {
    if (_Z_RC_IS_NULL(&be->_inner)) {
        return _Z_ERR_INVALID;
    }
    return _z_background_executor_inner_get_fut_status(_Z_RC_IN_VAL(&be->_inner), handle, status_out);
}

z_result_t _z_background_executor_cancel_fut(_z_background_executor_t *be, const _z_fut_handle_t *handle) {
    if (_Z_RC_IS_NULL(&be->_inner)) {
        return _Z_ERR_INVALID;
    }
    return _z_background_executor_inner_cancel_fut(_Z_RC_IN_VAL(&be->_inner), handle);
}

z_result_t _z_background_executor_clone(_z_background_executor_t *dst, const _z_background_executor_t *src) {
    if (_Z_RC_IS_NULL(&src->_inner)) {
        dst->_inner = _z_background_executor_inner_rc_null();
        return _Z_RES_OK;
    }
    dst->_inner = _z_background_executor_inner_rc_clone(&src->_inner);
    if (_Z_RC_IS_NULL(&dst->_inner)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

bool _z_background_executor_is_running(const _z_background_executor_t *be) {
    if (_Z_RC_IS_NULL(&be->_inner)) {
        return false;
    }
    size_t state = _z_atomic_size_load((_z_atomic_size_t *)&_Z_RC_IN_VAL(&be->_inner)->_state, _z_memory_order_acquire);
    return state != _Z_BACKGROUND_EXECUTOR_STATE_STOPPED;
}
#else
// to prevent "empty compilation unit" warning when multi-threading is disabled
typedef int _z_background_executor_inner_t;
#endif
