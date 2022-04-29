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

#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/transport/link/task/join.h"
#include "zenoh-pico/transport/link/task/lease.h"
#include "zenoh-pico/utils/logging.h"

_z_zint_t _z_get_minimum_lease(_z_transport_peer_entry_list_t *peers, _z_zint_t local_lease)
{
    _z_zint_t ret = local_lease;

    _z_transport_peer_entry_list_t *it = peers;
    while (it != NULL)
    {
        _z_transport_peer_entry_t *val = _z_transport_peer_entry_list_head(it);
        _z_zint_t lease = val->_lease;
        if (lease < ret)
            ret = lease;

        it = _z_transport_peer_entry_list_tail(it);
    }

    return ret;
}

_z_zint_t _z_get_next_lease(_z_transport_peer_entry_list_t *peers)
{
    _z_zint_t ret = SIZE_MAX;

    _z_transport_peer_entry_list_t *it = peers;
    while (it != NULL)
    {
        _z_transport_peer_entry_t *val = _z_transport_peer_entry_list_head(it);
        _z_zint_t next_lease = val->_next_lease;
        if (next_lease < ret)
            ret = next_lease;

        it = _z_transport_peer_entry_list_tail(it);
    }

    return ret;
}

int _zp_multicast_send_keep_alive(_z_transport_multicast_t *ztm)
{
    _z_bytes_t pid = _z_bytes_wrap(((_z_session_t *)ztm->_session)->_tp_manager->_local_pid.val, ((_z_session_t *)ztm->_session)->_tp_manager->_local_pid.len);
    _z_transport_message_t t_msg = _z_t_msg_make_keep_alive(pid);

    return _z_multicast_send_t_msg(ztm, &t_msg);
}

void *_zp_multicast_lease_task(void *arg)
{
    _z_transport_multicast_t *ztm = (_z_transport_multicast_t *)arg;

    ztm->_lease_task_running = 1;
    ztm->_transmitted = 0;

    // From all peers, get the next lease time (minimum)
    _z_zint_t next_lease = _z_get_minimum_lease(ztm->_peers, ztm->_lease);
    _z_zint_t next_keep_alive = next_lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR;
    _z_zint_t next_join = Z_JOIN_INTERVAL;

    _z_transport_peer_entry_list_t *it = NULL;
    while (ztm->_lease_task_running)
    {
        _z_mutex_lock(&ztm->_mutex_peer);
        if (next_lease <= 0)
        {
            it = ztm->_peers;
            while (it != NULL)
            {
                _z_transport_peer_entry_t *entry = _z_transport_peer_entry_list_head(it);
                if (entry->_received == 1)
                {
                    // Reset the lease parameters
                    entry->_received = 0;
                    entry->_next_lease = entry->_lease;
                    it = _z_transport_peer_entry_list_tail(it);
                }
                else
                {
                    _Z_INFO("Remove peer from know list because it has expired after %zums\n", entry->_lease);
                    ztm->_peers = _z_transport_peer_entry_list_drop_filter(ztm->_peers, _z_transport_peer_entry_eq, entry);
                    it = ztm->_peers;
                }
            }
        }

        if (next_join <= 0)
        {
            _zp_multicast_send_join(ztm);
            ztm->_transmitted = 1;

            // Reset the join parameters
            next_join = Z_JOIN_INTERVAL;
        }

        if (next_keep_alive <= 0)
        {
            // Check if need to send a keep alive
            if (ztm->_transmitted == 0)
                _zp_multicast_send_keep_alive(ztm);

            // Reset the keep alive parameters
            ztm->_transmitted = 0;
            next_keep_alive = _z_get_minimum_lease(ztm->_peers, ztm->_lease) / Z_TRANSPORT_LEASE_EXPIRE_FACTOR;
        }

        // Compute the target interval to sleep
        _z_zint_t interval;
        if (next_lease > 0)
        {
            interval = next_lease;
            if (next_keep_alive < interval)
                interval = next_keep_alive;
            if (next_join < interval)
                interval = next_join;
        }
        else
        {
            interval = next_keep_alive;
            if (next_join < interval)
                interval = next_join;
        }

        _z_mutex_unlock(&ztm->_mutex_peer);

        // The keep alive and lease intervals are expressed in milliseconds
        _z_sleep_ms(interval);

        // Decrement all intervals
        _z_mutex_lock(&ztm->_mutex_peer);
        it = ztm->_peers;
        while (it != NULL)
        {
            _z_transport_peer_entry_t *entry = _z_transport_peer_entry_list_head(it);
            entry->_next_lease -= interval;
            it = _z_transport_peer_entry_list_tail(it);
        }
        next_lease = _z_get_next_lease(ztm->_peers);
        next_keep_alive -= interval;
        next_join -= interval;
        _z_mutex_unlock(&ztm->_mutex_peer);
    }

    return 0;
}