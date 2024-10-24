//
// Copyright (c) 2022 ZettaScale Technology
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

#include "zenoh-pico/transport/unicast/lease.h"

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/transport/unicast/tx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

z_result_t _zp_unicast_send_keep_alive(_z_transport_unicast_t *ztu) {
    z_result_t ret = _Z_RES_OK;

    _z_transport_message_t t_msg = _z_t_msg_make_keep_alive();
    ret = _z_unicast_send_t_msg(ztu, &t_msg);

    return ret;
}
#else

z_result_t _zp_unicast_send_keep_alive(_z_transport_unicast_t *ztu) {
    _ZP_UNUSED(ztu);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1

#if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1

bool _z_pending_query_timeout(const _z_pending_query_t *foo, const _z_pending_query_t *pq) {
    _ZP_UNUSED(foo);
    bool result = z_clock_elapsed_ms((z_clock_t *)&pq->_start_time) >= pq->_timeout;
    if (result) {
        _Z_INFO("Dropping query because of timeout");
    }
    return result;
}

static void _z_process_query_timeout(_z_transport_unicast_t *ztu) {
#if Z_FEATURE_QUERY == 1
    _z_session_t *zn = _Z_RC_IN_VAL(ztu->_session);
    _z_session_mutex_lock(zn);
    // Drop all queries with timeout elapsed
    zn->_pending_queries = _z_pending_query_list_drop_filter(zn->_pending_queries, _z_pending_query_timeout, NULL);
    _z_session_mutex_unlock(zn);
#else
    _ZP_UNUSED(ztu);
    return;
#endif
}

void *_zp_unicast_lease_task(void *ztu_arg) {
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)ztu_arg;

    ztu->_received = false;
    ztu->_transmitted = false;

    int next_lease = (int)ztu->_lease;
    int next_keep_alive = (int)(ztu->_lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR);
    while (ztu->_lease_task_running == true) {
        // Next lease process
        if (next_lease <= 0) {
            // Check if received data
            if (ztu->_received == true) {
                // Reset the lease parameters
                ztu->_received = false;
            } else {
                _Z_INFO("Closing session because it has expired after %zums", ztu->_lease);
                ztu->_lease_task_running = false;
                _z_unicast_transport_close(ztu, _Z_CLOSE_EXPIRED);
                break;
            }
            next_lease = (int)ztu->_lease;
        }
        // Next keep alive process
        if (next_keep_alive <= 0) {
            // Check if need to send a keep alive
            if (ztu->_transmitted == false) {
                if (_zp_unicast_send_keep_alive(ztu) < 0) {
                    _Z_INFO("Send keep alive failed.");
                }
            }
            // Reset the keep alive parameters
            ztu->_transmitted = false;
            next_keep_alive = (int)(ztu->_lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR);
        }
        // Query timeout process
        _z_process_query_timeout(ztu);

        // Compute the target interval
        int interval;
        if (next_lease == 0) {
            interval = next_keep_alive;
        } else {
            interval = next_lease;
            if (next_keep_alive < interval) {
                interval = next_keep_alive;
            }
        }

        // The keep alive and lease intervals are expressed in milliseconds
        z_sleep_ms((size_t)interval);

        next_lease = next_lease - interval;
        next_keep_alive = next_keep_alive - interval;
    }
    return 0;
}

z_result_t _zp_unicast_start_lease_task(_z_transport_t *zt, z_task_attr_t *attr, _z_task_t *task) {
    // Init memory
    (void)memset(task, 0, sizeof(_z_task_t));
    zt->_transport._unicast._lease_task_running = true;  // Init before z_task_init for concurrency issue
    // Init task
    if (_z_task_init(task, attr, _zp_unicast_lease_task, &zt->_transport._unicast) != _Z_RES_OK) {
        return _Z_ERR_SYSTEM_TASK_FAILED;
    }
    // Attach task
    zt->_transport._unicast._lease_task = task;
    return _Z_RES_OK;
}

z_result_t _zp_unicast_stop_lease_task(_z_transport_t *zt) {
    zt->_transport._unicast._lease_task_running = false;
    return _Z_RES_OK;
}
#else

void *_zp_unicast_lease_task(void *ztu_arg) {
    _ZP_UNUSED(ztu_arg);
    return NULL;
}

z_result_t _zp_unicast_start_lease_task(_z_transport_t *zt, void *attr, void *task) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(attr);
    _ZP_UNUSED(task);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _zp_unicast_stop_lease_task(_z_transport_t *zt) {
    _ZP_UNUSED(zt);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif
