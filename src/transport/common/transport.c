//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/transport/transport.h"

#include <stdbool.h>

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/result.h"

z_result_t _z_common_transport_new_peer(_z_transport_peer_entry_t **peer,
                                        const _z_transport_peer_param_t *peer_params) {
    _z_transport_peer_entry_t *entry = (_z_transport_peer_entry_t *)z_malloc(sizeof(_z_transport_peer_entry_t));
    if (entry == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    entry->_sn_res = _z_sn_max(peer_params->_seq_num_res);

    // If the new node has less representing capabilities then it is incompatible to communication
    if ((peer_params->_seq_num_res != Z_SN_RESOLUTION) || (peer_params->_req_id_res != Z_REQ_RESOLUTION) ||
        (peer_params->_batch_size != Z_BATCH_MULTICAST_SIZE)) {
        z_free(entry);
        return _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
    }
    entry->_remote_addr = _z_slice_duplicate(peer_params->_addr);
    entry->_remote_zid = *peer_params->_remote_zid;
    _z_conduit_sn_list_copy(&entry->_sn_rx_sns, &peer_params->_next_sn);
    _z_conduit_sn_list_decrement(entry->_sn_res, &entry->_sn_rx_sns);

#if Z_FEATURE_FRAGMENTATION == 1
    entry->_patch = peer_params->_patch < _Z_CURRENT_PATCH ? peer_params->_patch : _Z_CURRENT_PATCH;
    entry->_state_reliable = _Z_DBUF_STATE_NULL;
    entry->_state_best_effort = _Z_DBUF_STATE_NULL;
    entry->_dbuf_reliable = _z_wbuf_null();
    entry->_dbuf_best_effort = _z_wbuf_null();
#endif
    // Update lease time
    entry->_lease = peer_params->_lease;
    entry->_next_lease = entry->_lease;
    entry->_received = true;
    // Return peer
    *peer = entry;
    return _Z_RES_OK;
}

void _z_common_transport_clear(_z_transport_common_t *ztc, bool detach_tasks) {
#if Z_FEATURE_MULTI_THREAD == 1
    // Clean up tasks
    if (ztc->_read_task != NULL) {
        ztc->_read_task_running = false;
        if (detach_tasks) {
            _z_task_detach(ztc->_read_task);
        } else {
            _z_task_join(ztc->_read_task);
        }
        _z_task_free(&ztc->_read_task);
        ztc->_read_task = NULL;
    }
    if (ztc->_lease_task != NULL) {
        ztc->_lease_task_running = false;
        if (detach_tasks) {
            _z_task_detach(ztc->_lease_task);
        } else {
            _z_task_join(ztc->_lease_task);
        }
        _z_task_free(&ztc->_lease_task);
        ztc->_lease_task = NULL;
    }

    // Clean up the mutexes
    _z_mutex_drop(&ztc->_mutex_tx);
    _z_mutex_drop(&ztc->_mutex_rx);
#else
    _ZP_UNUSED(detach_tasks);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Clean up the buffers
    _z_wbuf_clear(&ztc->_wbuf);
    _z_zbuf_clear(&ztc->_zbuf);
    _z_arc_slice_svec_release(&ztc->_arc_pool);
    _z_network_message_svec_release(&ztc->_msg_pool);

    _z_link_clear(&ztc->_link);
}
