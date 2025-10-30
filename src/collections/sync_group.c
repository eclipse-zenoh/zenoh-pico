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

#include "zenoh-pico/collections/sync_group.h"

#include "assert.h"

z_result_t _z_sync_group_state_create(_z_sync_group_state_t* state) {
    state->counter = 0;
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_init(&state->counter_mutex));
    _Z_CLEAN_RETURN_IF_ERR(_z_condvar_init(&state->counter_condvar), _z_mutex_drop(&state->counter_mutex));
#endif
    return Z_OK;
}

void _z_sync_group_state_clear(_z_sync_group_state_t* sync_group_state) {
    if (sync_group_state != NULL) {
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_drop(&sync_group_state->counter_mutex);
        _z_condvar_drop(&sync_group_state->counter_condvar);
#endif
    }
}

z_result_t _z_sync_group_state_increase_counter(_z_sync_group_state_t* sync_group_state) {
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_lock(&sync_group_state->counter_mutex));
#endif
    sync_group_state->counter++;
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_unlock(&sync_group_state->counter_mutex));
#endif
    return _Z_RES_OK;
}

z_result_t _z_sync_group_state_decrease_counter(_z_sync_group_state_t* sync_group_state) {
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_lock(&sync_group_state->counter_mutex));
#endif
    assert(sync_group_state->counter > 0);
    sync_group_state->counter--;
#if Z_FEATURE_MULTI_THREAD == 1
    _z_condvar_signal(&sync_group_state->counter_condvar);
    _Z_RETURN_IF_ERR(_z_mutex_unlock(&sync_group_state->counter_mutex));
#endif
    return _Z_RES_OK;
}

z_result_t _z_sync_group_state_wait(_z_sync_group_state_t* sync_group_state) {
#if Z_FEATURE_MULTI_THREAD == 1
    _Z_RETURN_IF_ERR(_z_mutex_lock(&sync_group_state->counter_mutex));
    while (sync_group_state->counter != 0) {
        _Z_RETURN_IF_ERR(_z_condvar_wait(&sync_group_state->counter_condvar, &sync_group_state->counter_mutex));
    }
    _Z_RETURN_IF_ERR(_z_mutex_unlock(&sync_group_state->counter_mutex));
    return _Z_RES_OK;
#else
    return sync_group_state->counter == 0 ? _Z_RES_OK : _Z_ERR_GENERIC;
#endif
}

z_result_t _z_sync_group_state_wait_deadline(_z_sync_group_state_t* sync_group_state, const z_clock_t* deadline) {
#if Z_FEATURE_MULTI_THREAD == 1
    z_result_t ret = _Z_RES_OK;
    _Z_RETURN_IF_ERR(_z_mutex_lock(&sync_group_state->counter_mutex));
    while (sync_group_state->counter != 0) {
        ret = _z_condvar_wait_until(&sync_group_state->counter_condvar, &sync_group_state->counter_mutex, deadline);
        if (ret == Z_ETIMEDOUT) {
            break;
        } else if (ret != _Z_RES_OK) {
            return ret;
        }
    }
    _Z_RETURN_IF_ERR(_z_mutex_unlock(&sync_group_state->counter_mutex));
    return ret;
#else
    _ZP_UNUSED(deadline);
    return sync_group_state->counter == 0 ? _Z_RES_OK : Z_ETIMEDOUT;
#endif
}

z_result_t _z_sync_group_create(_z_sync_group_t* sync_group) {
    _z_sync_group_state_t* state = (_z_sync_group_state_t*)z_malloc(sizeof(_z_sync_group_state_t));
    if (state == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    _Z_CLEAN_RETURN_IF_ERR(_z_sync_group_state_create(state), z_free(state));
    sync_group->_state = _z_sync_group_state_rc_new(state);
    if (_Z_RC_IS_NULL(&sync_group->_state)) {
        _z_sync_group_state_clear(state);
        z_free(state);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

z_result_t _z_sync_group_wait(_z_sync_group_t* sync_group) {
    if (_Z_RC_IS_NULL(&sync_group->_state)) {
        return _Z_ERR_NULL;
    }
    return _z_sync_group_state_wait(_Z_RC_IN_VAL(&sync_group->_state));
}

z_result_t _z_sync_group_wait_deadline(_z_sync_group_t* sync_group, const z_clock_t* deadline) {
    if (_Z_RC_IS_NULL(&sync_group->_state)) {
        return _Z_ERR_NULL;
    }
    return _z_sync_group_state_wait_deadline(_Z_RC_IN_VAL(&sync_group->_state), deadline);
}

void _z_sync_group_drop(_z_sync_group_t* sync_group) { _z_sync_group_state_rc_drop(&sync_group->_state); }

z_result_t _z_sync_group_create_notifier(_z_sync_group_t* sync_group, _z_sync_group_notifier_t* notifier) {
    if (_Z_RC_IS_NULL(&sync_group->_state)) {
        return _Z_ERR_NULL;
    }
    _Z_RETURN_IF_ERR(_z_sync_group_state_increase_counter(_Z_RC_IN_VAL(&sync_group->_state)));
    notifier->_state = _z_sync_group_state_rc_clone_as_weak(&sync_group->_state);
    return _Z_RES_OK;
}

void _z_sync_group_notifier_drop(_z_sync_group_notifier_t* notifier) {
    if (_Z_RC_IS_NULL(&notifier->_state)) {
        return;
    }
    _z_sync_group_state_rc_t state_rc = _z_sync_group_state_weak_upgrade(&notifier->_state);
    if (!_Z_RC_IS_NULL(&state_rc)) {
        _z_sync_group_state_decrease_counter(_Z_RC_IN_VAL(&state_rc));
        _z_sync_group_state_rc_drop(&state_rc);
    }
    _z_sync_group_state_weak_drop(&notifier->_state);
}
