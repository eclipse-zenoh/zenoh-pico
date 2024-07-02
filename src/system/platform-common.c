//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/system/platform-common.h"

#include "zenoh-pico/api/olv_macros.h"

#if Z_FEATURE_MULTI_THREAD == 1

/*------------------ Thread ------------------*/
_Z_OWNED_FUNCTIONS_SYSTEM_IMPL(_z_task_t, task)

int8_t z_task_init(z_owned_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    return _z_task_init(&task->_val, attr, fun, arg);
}

int8_t z_task_join(z_owned_task_t *task) {
    _z_task_t *ptr = &task->_val;
    int8_t ret = _z_task_join(ptr);
    _z_task_free(&ptr);
    return ret;
}

/*------------------ Mutex ------------------*/
_Z_OWNED_FUNCTIONS_SYSTEM_IMPL(_z_mutex_t, mutex)

int8_t z_mutex_init(z_owned_mutex_t *m) { return _z_mutex_init(&m->_val); }
int8_t z_mutex_drop(z_owned_mutex_t *m) { return _z_mutex_drop(&m->_val); }

int8_t z_mutex_lock(z_loaned_mutex_t *m) { return _z_mutex_lock(m); }
int8_t z_mutex_try_lock(z_loaned_mutex_t *m) { return _z_mutex_try_lock(m); }
int8_t z_mutex_unlock(z_loaned_mutex_t *m) { return _z_mutex_unlock(m); }

/*------------------ CondVar ------------------*/
_Z_OWNED_FUNCTIONS_SYSTEM_IMPL(_z_condvar_t, condvar)

int8_t z_condvar_init(z_owned_condvar_t *cv) { return _z_condvar_init(&cv->_val); }
int8_t z_condvar_drop(z_owned_condvar_t *cv) { return _z_condvar_drop(&cv->_val); }

int8_t z_condvar_signal(z_loaned_condvar_t *cv) { return _z_condvar_signal(cv); }
int8_t z_condvar_wait(z_loaned_condvar_t *cv, z_loaned_mutex_t *m) { return _z_condvar_wait(cv, m); }

#endif  // Z_FEATURE_MULTI_THREAD == 1
