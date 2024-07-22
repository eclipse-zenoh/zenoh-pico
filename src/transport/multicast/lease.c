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

#include "zenoh-pico/transport/multicast/lease.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/lease.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1

int8_t _zp_multicast_send_join(_z_transport_multicast_t *ztm) {
    _z_conduit_sn_list_t next_sn;
    next_sn._is_qos = false;
    next_sn._val._plain._best_effort = ztm->_sn_tx_best_effort;
    next_sn._val._plain._reliable = ztm->_sn_tx_reliable;

    _z_id_t zid = _Z_RC_IN_VAL(ztm->_session)->_local_zid;
    _z_transport_message_t jsm = _z_t_msg_make_join(Z_WHATAMI_PEER, Z_TRANSPORT_LEASE, zid, next_sn);

    return ztm->_send_f(ztm, &jsm);
}

int8_t _zp_multicast_send_keep_alive(_z_transport_multicast_t *ztm) {
    _z_transport_message_t t_msg = _z_t_msg_make_keep_alive();
    return ztm->_send_f(ztm, &t_msg);
}

#else
int8_t _zp_multicast_send_join(_z_transport_multicast_t *ztm) {
    _ZP_UNUSED(ztm);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _zp_multicast_send_keep_alive(_z_transport_multicast_t *ztm) {
    _ZP_UNUSED(ztm);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1

#if Z_FEATURE_MULTI_THREAD == 1 && (Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1)

static _z_zint_t _z_get_minimum_lease(_z_transport_peer_entry_list_t *peers, _z_zint_t local_lease) {
    _z_zint_t ret = local_lease;

    _z_transport_peer_entry_list_t *it = peers;
    while (it != NULL) {
        _z_transport_peer_entry_t *val = _z_transport_peer_entry_list_head(it);
        _z_zint_t lease = val->_lease;
        if (lease < ret) {
            ret = lease;
        }

        it = _z_transport_peer_entry_list_tail(it);
    }

    return ret;
}

static _z_zint_t _z_get_next_lease(_z_transport_peer_entry_list_t *peers) {
    _z_zint_t ret = SIZE_MAX;

    _z_transport_peer_entry_list_t *it = peers;
    while (it != NULL) {
        _z_transport_peer_entry_t *val = _z_transport_peer_entry_list_head(it);
        _z_zint_t next_lease = val->_next_lease;
        if (next_lease < ret) {
            ret = next_lease;
        }

        it = _z_transport_peer_entry_list_tail(it);
    }

    return ret;
}

void *_zp_multicast_lease_task(void *ztm_arg) {
    _z_transport_multicast_t *ztm = (_z_transport_multicast_t *)ztm_arg;
    ztm->_transmitted = false;

    // From all peers, get the next lease time (minimum)
    int next_lease = (int)_z_get_minimum_lease(ztm->_peers, ztm->_lease);
    int next_keep_alive = (int)(next_lease / Z_TRANSPORT_LEASE_EXPIRE_FACTOR);
    int next_join = Z_JOIN_INTERVAL;

    _z_transport_peer_entry_list_t *it = NULL;
    while (ztm->_lease_task_running == true) {
        _z_mutex_lock(&ztm->_mutex_peer);

        if (next_lease <= 0) {
            it = ztm->_peers;
            while (it != NULL) {
                _z_transport_peer_entry_t *entry = _z_transport_peer_entry_list_head(it);
                if (entry->_received == true) {
                    // Reset the lease parameters
                    entry->_received = false;
                    entry->_next_lease = entry->_lease;
                    it = _z_transport_peer_entry_list_tail(it);
                } else {
                    _Z_INFO("Remove peer from know list because it has expired after %zums", entry->_lease);
                    ztm->_peers =
                        _z_transport_peer_entry_list_drop_filter(ztm->_peers, _z_transport_peer_entry_eq, entry);
                    it = ztm->_peers;
                }
            }
        }

        if (next_join <= 0) {
            _zp_multicast_send_join(ztm);
            ztm->_transmitted = true;

            // Reset the join parameters
            next_join = Z_JOIN_INTERVAL;
        }

        if (next_keep_alive <= 0) {
            // Check if need to send a keep alive
            if (ztm->_transmitted == false) {
                if (_zp_multicast_send_keep_alive(ztm) < 0) {
                    // TODO: Handle retransmission or error
                }
            }

            // Reset the keep alive parameters
            ztm->_transmitted = false;
            next_keep_alive = (int)(_z_get_minimum_lease(ztm->_peers, ztm->_lease) / Z_TRANSPORT_LEASE_EXPIRE_FACTOR);
        }

        // Compute the target interval to sleep
        int interval;
        if (next_lease > 0) {
            interval = next_lease;
            if (next_keep_alive < interval) {
                interval = next_keep_alive;
            }
            if (next_join < interval) {
                interval = next_join;
            }
        } else {
            interval = next_keep_alive;
            if (next_join < interval) {
                interval = next_join;
            }
        }

        _z_mutex_unlock(&ztm->_mutex_peer);

        // The keep alive and lease intervals are expressed in milliseconds
        z_sleep_ms((size_t)interval);

        // Decrement all intervals
        _z_mutex_lock(&ztm->_mutex_peer);

        it = ztm->_peers;
        while (it != NULL) {
            _z_transport_peer_entry_t *entry = _z_transport_peer_entry_list_head(it);
            int entry_next_lease = (int)entry->_next_lease - interval;
            if (entry_next_lease >= 0) {
                entry->_next_lease = (size_t)entry_next_lease;
            } else {
                _Z_ERROR("Negative next lease value");
                entry->_next_lease = 0;
            }
            it = _z_transport_peer_entry_list_tail(it);
        }
        next_lease = (int)_z_get_next_lease(ztm->_peers);
        next_keep_alive = next_keep_alive - interval;
        next_join = next_join - interval;

        _z_mutex_unlock(&ztm->_mutex_peer);
    }
    return 0;
}

int8_t _zp_multicast_start_lease_task(_z_transport_multicast_t *ztm, z_task_attr_t *attr, _z_task_t *task) {
    // Init memory
    (void)memset(task, 0, sizeof(_z_task_t));
    // Init task
    if (_z_task_init(task, attr, _zp_multicast_lease_task, ztm) != _Z_RES_OK) {
        return _Z_ERR_SYSTEM_TASK_FAILED;
    }
    // Attach task
    ztm->_lease_task = task;
    ztm->_lease_task_running = true;
    return _Z_RES_OK;
}

int8_t _zp_multicast_stop_lease_task(_z_transport_multicast_t *ztm) {
    ztm->_lease_task_running = false;
    return _Z_RES_OK;
}
#else

void *_zp_multicast_lease_task(void *ztm_arg) {
    _ZP_UNUSED(ztm_arg);
    return NULL;
}

int8_t _zp_multicast_start_lease_task(_z_transport_multicast_t *ztm, void *attr, void *task) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(attr);
    _ZP_UNUSED(task);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _zp_multicast_stop_lease_task(_z_transport_multicast_t *ztm) {
    _ZP_UNUSED(ztm);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_MULTI_THREAD == 1 && (Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1)
