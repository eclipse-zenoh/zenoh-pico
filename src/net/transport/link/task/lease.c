/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#include "zenoh-pico/net/private/internal.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/private/logging.h"
#include "zenoh-pico/private/system.h"

void *_znp_lease_task(void *arg)
{
    zn_session_t *zn = (zn_session_t *)arg;
    zn->lease_task_running = 1;

    zn->received = 0;
    zn->transmitted = 0;

    unsigned int next_lease = zn->lease;
    unsigned int next_keep_alive = ZN_KEEP_ALIVE_INTERVAL;
    while (zn->lease_task_running)
    {
        // Compute the target interval
        unsigned int interval;
        if (zn->lease > 0)
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
        _z_sleep_ms(interval);

        // Decrement the interval
        if (zn->lease > 0)
        {
            next_lease -= interval;
        }
        next_keep_alive -= interval;

        if (zn->lease > 0 && next_lease == 0)
        {
            // Check if received data
            if (zn->received == 0)
            {
                _Z_DEBUG_VA("Closing session because it has expired after %zums", zn->lease);
                _zn_session_close(zn, _ZN_CLOSE_EXPIRED);
                return 0;
            }

            // Reset the lease parameters
            zn->received = 0;
            next_lease = zn->lease;
        }

        if (next_keep_alive == 0)
        {
            // Check if need to send a keep alive
            if (zn->transmitted == 0)
            {
                znp_send_keep_alive(zn);
            }

            // Reset the keep alive parameters
            zn->transmitted = 0;
            next_keep_alive = ZN_KEEP_ALIVE_INTERVAL;
        }
    }

    return 0;
}

int znp_start_lease_task(zn_session_t *zn)
{
    _z_task_t *task = (_z_task_t *)malloc(sizeof(_z_task_t));
    memset(task, 0, sizeof(pthread_t));
    zn->lease_task = task;
    if (_z_task_init(task, NULL, _znp_lease_task, zn) != 0)
    {
        return -1;
    }
    return 0;
}

int znp_stop_lease_task(zn_session_t *zn)
{
    zn->lease_task_running = 0;
    return 0;
}
