/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include "zenoh-pico/transport/link/task/lease.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/utils/logging.h"

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
    _z_transport_unicast_t *ztu = (_z_transport_unicast_t *)arg;

    ztu->lease_task_running = 1;
    ztu->received = 0;
    ztu->transmitted = 0;

    _z_zint_t next_lease = ztu->lease;
    _z_zint_t next_keep_alive = ztu->lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR;
    while (ztu->lease_task_running)
    {
        if (next_lease <= 0)
        {
            // Check if received data
            if (ztu->received == 1)
            {
                // Reset the lease parameters
                ztu->received = 0;
            }
            else
            {
                _Z_INFO("Closing session because it has expired after %zums\n", ztu->lease);
                _z_transport_unicast_close(ztu, _Z_CLOSE_EXPIRED);
                return 0;
            }

            next_lease = ztu->lease;
        }

        if (next_keep_alive <= 0)
        {
            // Check if need to send a keep alive
            if (ztu->transmitted == 0)
                _zp_unicast_send_keep_alive(ztu);

            // Reset the keep alive parameters
            ztu->transmitted = 0;
            next_keep_alive = ztu->lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR;
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
        _z_sleep_ms(interval);

        next_lease -= interval;
        next_keep_alive -= interval;
    }

    return 0;
}