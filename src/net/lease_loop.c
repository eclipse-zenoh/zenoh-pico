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

#include "zenoh/net/session.h"
#include "zenoh/net/types.h"
#include "zenoh/net/private/session.h"
#include "zenoh/net/private/sync.h"

void *zn_lease_loop(zn_session_t *z)
{
    z->lease_loop_running = 1;

    z->received = 0;
    z->transmitted = 0;

    unsigned int next_lease = z->lease;
    unsigned int next_keep_alive = ZENOH_NET_KEEP_ALIVE_INTERVAL;
    while (z->lease_loop_running)
    {
        // Compute the target interval
        unsigned int interval;
        if (next_lease < next_keep_alive)
            interval = next_lease;
        else if (next_keep_alive < ZENOH_NET_KEEP_ALIVE_INTERVAL)
            interval = next_keep_alive;
        else
            interval = ZENOH_NET_KEEP_ALIVE_INTERVAL;

        // The keep alive and lease intervals are expressed in milliseconds
        _zn_sleep_ms(interval);

        // Decrement the interval
        next_lease -= interval;
        next_keep_alive -= interval;

        if (next_lease == 0)
        {
            // Check if received data
            if (z->received == 0)
            {
                _zn_session_free(z);
                return 0;
            }

            // Reset the lease parameters
            z->received = 0;
            next_lease = z->lease;
        }

        if (next_keep_alive == 0)
        {
            // Check if need to send a keep alive
            if (z->transmitted == 0)
            {
                zn_send_keep_alive(z);
            }

            // Reset the keep alive parameters
            z->transmitted = 0;
            next_keep_alive = ZENOH_NET_KEEP_ALIVE_INTERVAL;
        }
    }

    return 0;
}
