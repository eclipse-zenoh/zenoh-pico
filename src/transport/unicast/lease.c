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

#include "zenoh-pico/transport/unicast/transport.h"
#include "zenoh-pico/transport/unicast/tx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

int8_t _zp_unicast_send_keep_alive(_z_transport_unicast_t *ztu) {
    int8_t ret = _Z_RES_OK;

    _z_transport_message_t t_msg = _z_t_msg_make_keep_alive();
    ret = _z_unicast_send_t_msg(ztu, &t_msg);

    return ret;
}
#else

int8_t _zp_unicast_send_keep_alive(_z_transport_unicast_t *ztu) {
    _ZP_UNUSED(ztu);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1

#if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_UNICAST_TRANSPORT == 1

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
                    // TODO: Handle retransmission or error
                }
            }

            // Reset the keep alive parameters
            ztu->_transmitted = false;
            next_keep_alive = (int)(ztu->_lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR);
        }

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

int8_t _zp_unicast_start_lease_task(_z_transport_t *zt, z_task_attr_t *attr, _z_task_t *task) {
    // Init memory
    (void)memset(task, 0, sizeof(_z_task_t));
    // Init task
    if (_z_task_init(task, attr, _zp_unicast_lease_task, &zt->_transport._unicast) != _Z_RES_OK) {
        return _Z_ERR_SYSTEM_TASK_FAILED;
    }
    // Attach task
    zt->_transport._unicast._lease_task = task;
    zt->_transport._unicast._lease_task_running = true;
    return _Z_RES_OK;
}

int8_t _zp_unicast_stop_lease_task(_z_transport_t *zt) {
    zt->_transport._unicast._lease_task_running = false;
    return _Z_RES_OK;
}
#else

void *_zp_unicast_lease_task(void *ztu_arg) {
    _ZP_UNUSED(ztu_arg);
    return NULL;
}

int8_t _zp_unicast_start_lease_task(_z_transport_t *zt, void *attr, void *task) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(attr);
    _ZP_UNUSED(task);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _zp_unicast_stop_lease_task(_z_transport_t *zt) {
    _ZP_UNUSED(zt);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif
