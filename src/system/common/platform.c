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

#include "zenoh-pico/system/common/platform.h"

#include "zenoh-pico/api/olv_macros.h"

#if Z_FEATURE_MULTI_THREAD == 1

/*------------------ Thread ------------------*/
_Z_OWNED_FUNCTIONS_SYSTEM_IMPL(_z_task_t, task)

z_result_t z_task_init(z_owned_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    return _z_task_init(&task->_val, attr, fun, arg);
}

z_result_t z_task_join(z_moved_task_t *task) {
    _z_task_t *ptr = &task->_this._val;
    z_result_t ret = _z_task_join(ptr);
    return ret;
}

z_result_t z_task_detach(z_moved_task_t *task) {
    _z_task_t *ptr = &task->_this._val;
    z_result_t ret = _z_task_detach(ptr);
    return ret;
}

z_result_t z_task_drop(z_moved_task_t *task) { return z_task_detach(task); }

#endif  // Z_FEATURE_MULTI_THREAD == 1

// The owned/loaned/moved plumbing below (_Z_OWNED_FUNCTIONS_SYSTEM_IMPL) is
// generic pointer/struct bookkeeping that doesn't touch the underlying
// mutex/condvar type at all -- it works unchanged against the dummy void*
// _z_mutex_t/_z_condvar_t stub typedefs platform.h already provides for
// Z_FEATURE_MULTI_THREAD == 0 ("dummy types for correct macros work"). Only
// the actual lock/wait semantics below need a real vs. no-op split.

/*------------------ Mutex ------------------*/
_Z_OWNED_FUNCTIONS_SYSTEM_IMPL(_z_mutex_t, mutex)

/*------------------ CondVar ------------------*/
_Z_OWNED_FUNCTIONS_SYSTEM_IMPL(_z_condvar_t, condvar)

#if Z_FEATURE_MULTI_THREAD == 1

z_result_t z_mutex_init(z_owned_mutex_t *m) { return _z_mutex_init(&m->_val); }
z_result_t z_mutex_drop(z_moved_mutex_t *m) { return _z_mutex_drop(&m->_this._val); }

z_result_t z_mutex_lock(z_loaned_mutex_t *m) { return _z_mutex_lock(m); }
z_result_t z_mutex_try_lock(z_loaned_mutex_t *m) { return _z_mutex_try_lock(m); }
z_result_t z_mutex_unlock(z_loaned_mutex_t *m) { return _z_mutex_unlock(m); }

z_result_t z_condvar_init(z_owned_condvar_t *cv) { return _z_condvar_init(&cv->_val); }
z_result_t z_condvar_drop(z_moved_condvar_t *cv) { return _z_condvar_drop(&cv->_this._val); }

z_result_t z_condvar_signal(z_loaned_condvar_t *cv) { return _z_condvar_signal(cv); }
z_result_t z_condvar_wait(z_loaned_condvar_t *cv, z_loaned_mutex_t *m) { return _z_condvar_wait(cv, m); }
z_result_t z_condvar_wait_until(z_loaned_condvar_t *cv, z_loaned_mutex_t *m, const z_clock_t *abstime) {
    return _z_condvar_wait_until(cv, m, abstime);
}

#else  // Z_FEATURE_MULTI_THREAD == 0

// No real threads on this platform, so nothing ever runs concurrently with
// a mutex holder or a condvar waiter -- every one of these is a safe no-op.
// Callers that actually need to block waiting for incoming data (i.e.
// rmw_zenoh_pico's rmw_wait) don't rely on z_condvar_wait to do that
// anymore: they poll zp_read()/zp_send_keep_alive() directly instead, since
// that's the only place with a session handle to pump. A z_condvar_wait
// that just returned immediately without pumping would busy-loop its
// caller with no progress; anything still calling it here is only doing so
// to protect a data structure that's never actually contended.
z_result_t z_mutex_init(z_owned_mutex_t *m) {
    (void)m;
    return _Z_RES_OK;
}
z_result_t z_mutex_drop(z_moved_mutex_t *m) {
    (void)m;
    return _Z_RES_OK;
}

z_result_t z_mutex_lock(z_loaned_mutex_t *m) {
    (void)m;
    return _Z_RES_OK;
}
z_result_t z_mutex_try_lock(z_loaned_mutex_t *m) {
    (void)m;
    return _Z_RES_OK;
}
z_result_t z_mutex_unlock(z_loaned_mutex_t *m) {
    (void)m;
    return _Z_RES_OK;
}

z_result_t z_condvar_init(z_owned_condvar_t *cv) {
    (void)cv;
    return _Z_RES_OK;
}
z_result_t z_condvar_drop(z_moved_condvar_t *cv) {
    (void)cv;
    return _Z_RES_OK;
}

z_result_t z_condvar_signal(z_loaned_condvar_t *cv) {
    (void)cv;
    return _Z_RES_OK;
}
z_result_t z_condvar_wait(z_loaned_condvar_t *cv, z_loaned_mutex_t *m) {
    (void)cv;
    (void)m;
    return _Z_RES_OK;
}
z_result_t z_condvar_wait_until(z_loaned_condvar_t *cv, z_loaned_mutex_t *m, const z_clock_t *abstime) {
    (void)cv;
    (void)m;
    (void)abstime;
    return _Z_RES_OK;
}

#endif  // Z_FEATURE_MULTI_THREAD == 1
