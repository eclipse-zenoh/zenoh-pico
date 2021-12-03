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

z_zint_t _zn_get_minimum_lease(_zn_transport_peer_entry_list_t *peers)
{
    z_zint_t ret = -1; // As ret is size_t, -1 sets it to the highest value

    _zn_transport_peer_entry_list_t *it = peers;
    while (it != NULL)
    {
        z_zint_t lease_expires = ((_zn_transport_peer_entry_t *)it->val)->lease_expires;
        if (lease_expires < ret)
            ret = lease_expires;

        it = it->tail;
    }

    return ret;
}

int _znp_multicast_send_keep_alive(_zn_transport_multicast_t *ztm)
{
    _zn_transport_message_t t_msg = _zn_t_msg_make_keep_alive(((zn_session_t *)ztm->session)->tp_manager->local_pid);

    return _zn_multicast_send_t_msg(ztm, &t_msg);
}

void *_znp_multicast_lease_task(void *arg)
{
    _zn_transport_multicast_t *ztm = (_zn_transport_multicast_t *)arg;

    ztm->lease_task_running = 1;
    ztm->transmitted = 0;

    // From all peers, get the next lease time (minimum)
    z_zint_t next_lease = _zn_get_minimum_lease(ztm->peers);
    z_zint_t next_keep_alive = ztm->keep_alive;

    _zn_transport_peer_entry_list_t *it = NULL;
    while (ztm->lease_task_running)
    {
        // Compute the target interval to sleep
        z_zint_t interval;
        if (next_lease > 0)
        {
            if (next_lease < next_keep_alive)
                interval = next_lease;
            else
                interval = next_keep_alive;
        }
        else
            interval = next_keep_alive;

        // The keep alive and lease intervals are expressed in milliseconds
        z_sleep_ms(interval);

        // Get the peer lock
        z_mutex_lock(&ztm->mutex_peer);

        // Decrement all intervals
        it = ztm->peers;
        while (it != NULL)
        {
            ((_zn_transport_peer_entry_t *)it->val)->lease_expires -= interval;
            it = it->tail;
        }

        next_lease -= interval;
        next_keep_alive -= interval;

        if (next_lease <= 0)
        {
            it = ztm->peers;
            while (it != NULL)
            {
                _zn_transport_peer_entry_t *entry = it->val;
                if (entry->lease_expires == 0 && entry->received == 0)
                {
                    _Z_DEBUG_VA("Remove peer from know list because it has expired after %zums", ztu->lease);
                    ztm->peers = _zn_transport_peer_entry_list_drop_filter(ztm->peers, _zn_transport_peer_entry_eq, entry);
                    it = ztm->peers;
                }
                else if (entry->received == 1)
                {
                    // Reset the lease parameters
                    entry->received = 0;
                    entry->lease_expires = entry->lease * ZN_LEASE_EXPIRE_FACTOR; // FIXME: might overflow
                    it = it->tail;
                } else
                    it = it->tail;
            }
        }
        else if (next_keep_alive == 0)
        {
            // Check if need to send a keep alive
            if (ztm->transmitted == 0)
            {
                _znp_multicast_send_keep_alive(ztm);
            }

            // Reset the keep alive parameters
            ztm->transmitted = 0;
            next_keep_alive = ztm->keep_alive;
        }

        next_lease = _zn_get_minimum_lease(ztm->peers);

        z_mutex_unlock(&ztm->mutex_peer);
    }

    return 0;
}