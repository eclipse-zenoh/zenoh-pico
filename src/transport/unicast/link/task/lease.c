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

#include "zenoh-pico/transport/link/task/lease.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/utils/logging.h"

#if Z_UNICAST_TRANSPORT == 1

int _zp_unicast_send_keep_alive(_z_transport_unicast_t *ztu)
{
    // Do not send the PID on unicast links
    _z_bytes_t pid;
    _z_bytes_reset(&pid);

    _z_transport_message_t t_msg = _z_t_msg_make_keep_alive(pid);

    return _z_unicast_send_t_msg(ztu, &t_msg);
}

void *_zp_unicast_lease_task(void *arg)
{
    (void) (arg);
#if Z_MULTI_THREAD == 1
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)arg;

    ztu->_lease_task_running = 1;
    ztu->_received = 0;
    ztu->_transmitted = 0;

    _z_zint_t next_lease = ztu->_lease;
    _z_zint_t next_keep_alive = ztu->_lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR;
    while (ztu->_lease_task_running)
    {
        if (next_lease <= 0)
        {
            // Check if received data
            if (ztu->_received == 1)
            {
                // Reset the lease parameters
                ztu->_received = 0;
            }
            else
            {
                _Z_INFO("Closing session because it has expired after %zums\n", ztu->_lease);
                _z_transport_unicast_close(ztu, _Z_CLOSE_EXPIRED);
                return 0;
            }

            next_lease = ztu->_lease;
        }

        if (next_keep_alive <= 0)
        {
            // Check if need to send a keep alive
            if (ztu->_transmitted == 0)
                _zp_unicast_send_keep_alive(ztu);

            // Reset the keep alive parameters
            ztu->_transmitted = 0;
            next_keep_alive = ztu->_lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR;
        }

        // Compute the target interval
        _z_zint_t interval;
        if (next_lease > 0)
        {
            interval = next_lease;
            if (next_keep_alive < interval)
                interval = next_keep_alive;
        }
        else
            interval = next_keep_alive;

        // The keep alive and lease intervals are expressed in milliseconds
        z_sleep_ms(interval);

        next_lease -= interval;
        next_keep_alive -= interval;
    }
#endif // Z_MULTI_THREAD == 1

    return 0;
}

#endif // Z_UNICAST_TRANSPORT == 1
