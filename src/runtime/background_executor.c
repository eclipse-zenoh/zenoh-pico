#include "zenoh-pico/runtime/background_executor.h"

#if Z_FEATURE_MULTI_THREAD == 1

typedef struct _z_background_executor_inner_t {
    _z_executor_t _executor;
    _z_mutex_t _mutex;
    _z_condvar_t _condvar;
    _z_atomic_size_t _waiters;
    bool _stop_requested;
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
    while (!be->_stop_requested) {
        while (_z_atomic_size_load(&be->_waiters, _z_memory_order_acquire) >
               0) {  // if there are waiters, sleep until they are resumed
            _Z_CLEAN_RETURN_IF_ERR(_z_condvar_wait(&be->_condvar, &be->_mutex), _z_mutex_unlock(&be->_mutex));
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

z_result_t _z_background_executor_inner_signal_stop(_z_background_executor_inner_t *be) {
    _Z_RETURN_IF_ERR(_z_background_executor_inner_suspend_and_lock(be));
    be->_stop_requested = true;
    return _z_background_executor_inner_unlock_and_resume(be);
}

static inline bool _is_called_from_executor(const _z_background_executor_inner_t *be) {
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

void _z_background_executor_inner_clear(_z_background_executor_inner_t *be) {
    if (_z_background_executor_inner_signal_stop(be) != _Z_RES_OK) {
        // if we failed to signal the background task to stop, we are in an undefined state and there's not much we can
        // do about it
        return;
    }
    _z_task_join(&be->_task);
    _z_executor_destroy(&be->_executor);
    _z_condvar_drop(&be->_condvar);
    _z_mutex_drop(&be->_mutex);
}

void *_z_background_executor_inner_task_fn(void *arg) {
    _z_background_executor_inner_t *be = (_z_background_executor_inner_t *)arg;
    _z_background_executor_inner_run_forever(be);
    return NULL;
}

z_result_t _z_background_executor_inner_init(_z_background_executor_inner_t *be, z_task_attr_t *task_attr) {
    _Z_RETURN_IF_ERR(_z_mutex_init(&be->_mutex));
    _Z_CLEAN_RETURN_IF_ERR(_z_condvar_init(&be->_condvar), _z_mutex_drop(&be->_mutex));
    _z_executor_init(&be->_executor);
    _z_atomic_size_init(&be->_waiters, 0);
    be->_stop_requested = false;
    _Z_CLEAN_RETURN_IF_ERR(_z_task_init(&be->_task, task_attr, _z_background_executor_inner_task_fn, be),
                           _z_executor_destroy(&be->_executor);
                           _z_condvar_drop(&be->_condvar); _z_mutex_drop(&be->_mutex));
    return _Z_RES_OK;
}

z_result_t _z_background_executor_init(_z_background_executor_t *be, z_task_attr_t *task_attr) {
    be->_inner = _z_background_executor_inner_rc_null();
    _z_background_executor_inner_t *inner =
        (_z_background_executor_inner_t *)z_malloc(sizeof(_z_background_executor_inner_t));
    if (!inner) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    if (_z_background_executor_inner_init(inner, task_attr) != _Z_RES_OK) {
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
#else
// to prevent "empty compilation unit" warning when multi-threading is disabled
typedef int _z_background_executor_inner_t;
#endif
