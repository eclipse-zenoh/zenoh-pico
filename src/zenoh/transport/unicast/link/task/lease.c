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
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

int _znp_unicast_send_keep_alive(_zn_transport_unicast_t *ztu)
{
    _zn_transport_message_t t_msg = _zn_transport_message_init(_ZN_MID_KEEP_ALIVE);

    return _zn_unicast_send_t_msg(ztu, &t_msg);
}

void *_znp_unicast_lease_task(void *arg)
{
    _zn_transport_unicast_t *ztu = (_zn_transport_unicast_t *)arg;

    ztu->lease_task_running = 1;
    ztu->received = 0;
    ztu->transmitted = 0;

    unsigned int next_lease = ztu->lease;
    unsigned int next_keep_alive = ZN_KEEP_ALIVE_INTERVAL;
    while (ztu->lease_task_running)
    {
        // Compute the target interval
        unsigned int interval;
        if (ztu->lease > 0)
        {
            if (next_lease < next_keep_alive)
                interval = next_lease;
            else if (next_keep_alive < ZN_KEEP_ALIVE_INTERVAL)
                interval = next_keep_alive;
            else
                interval = ZN_KEEP_ALIVE_INTERVAL;
        }
        else
        {
            interval = ZN_KEEP_ALIVE_INTERVAL;
        }

        // The keep alive and lease intervals are expressed in milliseconds
        z_sleep_ms(interval);

        // Decrement the interval
        if (ztu->lease > 0)
        {
            next_lease -= interval;
        }
        next_keep_alive -= interval;

        if (ztu->lease > 0 && next_lease == 0)
        {
            // Check if received data
            if (ztu->received == 0)
            {
                _Z_DEBUG_VA("Closing session because it has expired after %zums", ztu->lease);
                _zn_transport_unicast_close(ztu, _ZN_CLOSE_EXPIRED);
                return 0;
            }

            // Reset the lease parameters
            ztu->received = 0;
            next_lease = ztu->lease;
        }

        if (next_keep_alive == 0)
        {
            // Check if need to send a keep alive
            if (ztu->transmitted == 0)
            {
                _znp_unicast_send_keep_alive(ztu);
            }

            // Reset the keep alive parameters
            ztu->transmitted = 0;
            next_keep_alive = ZN_KEEP_ALIVE_INTERVAL;
        }
    }

    return 0;
}