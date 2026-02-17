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
#include "zenoh-pico/utils/logging.h"

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

/*------------------ Mutex ------------------*/
_Z_OWNED_FUNCTIONS_SYSTEM_IMPL(_z_mutex_t, mutex)

z_result_t z_mutex_init(z_owned_mutex_t *m) { return _z_mutex_init(&m->_val); }
z_result_t z_mutex_drop(z_moved_mutex_t *m) { return _z_mutex_drop(&m->_this._val); }

z_result_t z_mutex_lock(z_loaned_mutex_t *m) { return _z_mutex_lock(m); }
z_result_t z_mutex_try_lock(z_loaned_mutex_t *m) { return _z_mutex_try_lock(m); }
z_result_t z_mutex_unlock(z_loaned_mutex_t *m) { return _z_mutex_unlock(m); }

/*------------------ CondVar ------------------*/
_Z_OWNED_FUNCTIONS_SYSTEM_IMPL(_z_condvar_t, condvar)

z_result_t z_condvar_init(z_owned_condvar_t *cv) { return _z_condvar_init(&cv->_val); }
z_result_t z_condvar_drop(z_moved_condvar_t *cv) { return _z_condvar_drop(&cv->_this._val); }

z_result_t z_condvar_signal(z_loaned_condvar_t *cv) { return _z_condvar_signal(cv); }
z_result_t z_condvar_wait(z_loaned_condvar_t *cv, z_loaned_mutex_t *m) { return _z_condvar_wait(cv, m); }
z_result_t z_condvar_wait_until(z_loaned_condvar_t *cv, z_loaned_mutex_t *m, const z_clock_t *abstime) {
    return _z_condvar_wait_until(cv, m, abstime);
}

#endif  // Z_FEATURE_MULTI_THREAD == 1

#if !defined(ZENOH_WINDOWS) && !defined(ZENOH_LINUX) && !defined(ZENOH_MACOS) && !defined(ZENOH_BSD) && \
    !defined(ZENOH_ZEPHYR)
z_result_t _z_ip_port_to_endpoint(const uint8_t *address, size_t address_len, uint16_t port, char *dst,
                                  size_t dst_len) {
    _ZP_UNUSED(address);
    _ZP_UNUSED(address_len);
    _ZP_UNUSED(port);
    _ZP_UNUSED(dst);
    _ZP_UNUSED(dst_len);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}
z_result_t _z_socket_get_endpoints(const _z_sys_net_socket_t *sock, char *local, size_t local_len, char *remote,
                                   size_t remote_len) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(local);
    _ZP_UNUSED(local_len);
    _ZP_UNUSED(remote);
    _ZP_UNUSED(remote_len);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}
#endif
