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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/transport/common/lease.h"
#include "zenoh-pico/transport/common/read.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/multicast/tx.h"
#include "zenoh-pico/transport/raweth/tx.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1

int8_t _z_multicast_transport_create(_z_transport_t *zt, _z_link_t *zl,
                                     _z_transport_multicast_establish_param_t *param) {
    int8_t ret = _Z_RES_OK;
    // Transport specific information
    _z_transport_multicast_t *ztm = NULL;
    switch (zl->_cap._transport) {
        case Z_LINK_CAP_TRANSPORT_MULTICAST:
            zt->_type = _Z_TRANSPORT_MULTICAST_TYPE;
            ztm = &zt->_transport._multicast;
            ztm->_send_f = _z_multicast_send_t_msg;
            break;
        case Z_LINK_CAP_TRANSPORT_RAWETH:
            zt->_type = _Z_TRANSPORT_RAWETH_TYPE;
            ztm = &zt->_transport._raweth;
            ztm->_send_f = _z_raweth_send_t_msg;
            break;
        default:
            return _Z_ERR_GENERIC;
    }
#if Z_FEATURE_MULTI_THREAD == 1
    // Initialize the mutexes
    ret = _z_mutex_init(&ztm->_mutex_tx);
    if (ret == _Z_RES_OK) {
        ret = _z_mutex_init(&ztm->_mutex_rx);
        if (ret == _Z_RES_OK) {
            ret = _z_mutex_init(&ztm->_mutex_peer);
            if (ret != _Z_RES_OK) {
                _z_mutex_drop(&ztm->_mutex_tx);
                _z_mutex_drop(&ztm->_mutex_rx);
            }
        } else {
            _z_mutex_drop(&ztm->_mutex_tx);
        }
    }
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Initialize the read and write buffers
    if (ret == _Z_RES_OK) {
        uint16_t mtu = (zl->_mtu < Z_BATCH_MULTICAST_SIZE) ? zl->_mtu : Z_BATCH_MULTICAST_SIZE;
        ztm->_wbuf = _z_wbuf_make(mtu, false);
        ztm->_zbuf = _z_zbuf_make(Z_BATCH_MULTICAST_SIZE);

        // Clean up the buffers if one of them failed to be allocated
        if ((_z_wbuf_capacity(&ztm->_wbuf) != mtu) || (_z_zbuf_capacity(&ztm->_zbuf) != Z_BATCH_MULTICAST_SIZE)) {
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            _Z_ERROR("Not enough memory to allocate transport tx rx buffers!");

#if Z_FEATURE_MULTI_THREAD == 1
            _z_mutex_drop(&ztm->_mutex_tx);
            _z_mutex_drop(&ztm->_mutex_rx);
            _z_mutex_drop(&ztm->_mutex_peer);
#endif  // Z_FEATURE_MULTI_THREAD == 1

            _z_wbuf_clear(&ztm->_wbuf);
            _z_zbuf_clear(&ztm->_zbuf);
        }
    }

    if (ret == _Z_RES_OK) {
        // Set default SN resolution
        ztm->_sn_res = _z_sn_max(param->_seq_num_res);

        // The initial SN at TX side
        ztm->_sn_tx_reliable = param->_initial_sn_tx._val._plain._reliable;
        ztm->_sn_tx_best_effort = param->_initial_sn_tx._val._plain._best_effort;

        // Initialize peer list
        ztm->_peers = _z_transport_peer_entry_list_new();

#if Z_FEATURE_MULTI_THREAD == 1
        // Tasks
        ztm->_read_task_running = false;
        ztm->_read_task = NULL;
        ztm->_lease_task_running = false;
        ztm->_lease_task = NULL;
#endif  // Z_FEATURE_MULTI_THREAD == 1

        ztm->_lease = Z_TRANSPORT_LEASE;

        // Notifiers
        ztm->_transmitted = false;

        // Transport link for multicast
        ztm->_link = *zl;
    }
    return ret;
}

int8_t _z_multicast_open_peer(_z_transport_multicast_establish_param_t *param, const _z_link_t *zl,
                              const _z_id_t *local_zid) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t initial_sn_tx = 0;
    z_random_fill(&initial_sn_tx, sizeof(initial_sn_tx));
    initial_sn_tx = initial_sn_tx & !_z_sn_modulo_mask(Z_SN_RESOLUTION);

    _z_conduit_sn_list_t next_sn;
    next_sn._is_qos = false;
    next_sn._val._plain._best_effort = initial_sn_tx;
    next_sn._val._plain._reliable = initial_sn_tx;

    _z_id_t zid = *local_zid;
    _z_transport_message_t jsm = _z_t_msg_make_join(Z_WHATAMI_PEER, Z_TRANSPORT_LEASE, zid, next_sn);

    // Encode and send the message
    _Z_INFO("Sending Z_JOIN message");
    switch (zl->_cap._transport) {
        case Z_LINK_CAP_TRANSPORT_MULTICAST:
            ret = _z_link_send_t_msg(zl, &jsm);
            break;
        case Z_LINK_CAP_TRANSPORT_RAWETH:
            ret = _z_raweth_link_send_t_msg(zl, &jsm);
            break;
        default:
            return _Z_ERR_GENERIC;
    }
    _z_t_msg_clear(&jsm);

    if (ret == _Z_RES_OK) {
        param->_seq_num_res = jsm._body._join._seq_num_res;
        param->_initial_sn_tx = next_sn;
    }
    return ret;
}

int8_t _z_multicast_open_client(_z_transport_multicast_establish_param_t *param, const _z_link_t *zl,
                                const _z_id_t *local_zid) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    int8_t ret = _Z_ERR_CONFIG_UNSUPPORTED_CLIENT_MULTICAST;
    // @TODO: not implemented
    return ret;
}

int8_t _z_multicast_send_close(_z_transport_multicast_t *ztm, uint8_t reason, _Bool link_only) {
    int8_t ret = _Z_RES_OK;
    // Send and clear message
    _z_transport_message_t cm = _z_t_msg_make_close(reason, link_only);
    ret = ztm->_send_f(ztm, &cm);
    _z_t_msg_clear(&cm);
    return ret;
}

int8_t _z_multicast_transport_close(_z_transport_multicast_t *ztm, uint8_t reason) {
    return _z_multicast_send_close(ztm, reason, false);
}

void _z_multicast_transport_clear(_z_transport_t *zt) {
    _z_transport_multicast_t *ztm = &zt->_transport._multicast;
#if Z_FEATURE_MULTI_THREAD == 1
    // Clean up tasks
    if (ztm->_read_task != NULL) {
        _z_task_join(ztm->_read_task);
        _z_task_free(&ztm->_read_task);
    }
    if (ztm->_lease_task != NULL) {
        _z_task_join(ztm->_lease_task);
        _z_task_free(&ztm->_lease_task);
    }
    // Clean up the mutexes
    _z_mutex_drop(&ztm->_mutex_tx);
    _z_mutex_drop(&ztm->_mutex_rx);
    _z_mutex_drop(&ztm->_mutex_peer);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Clean up the buffers
    _z_wbuf_clear(&ztm->_wbuf);
    _z_zbuf_clear(&ztm->_zbuf);

    // Clean up peer list
    _z_transport_peer_entry_list_free(&ztm->_peers);
    _z_link_clear(&ztm->_link);
}

#else

int8_t _z_multicast_transport_create(_z_transport_t *zt, _z_link_t *zl,
                                     _z_transport_multicast_establish_param_t *param) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(param);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _z_multicast_open_peer(_z_transport_multicast_establish_param_t *param, const _z_link_t *zl,
                              const _z_id_t *local_zid) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _z_multicast_open_client(_z_transport_multicast_establish_param_t *param, const _z_link_t *zl,
                                const _z_id_t *local_zid) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _z_multicast_send_close(_z_transport_multicast_t *ztm, uint8_t reason, _Bool link_only) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(reason);
    _ZP_UNUSED(link_only);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _z_multicast_transport_close(_z_transport_multicast_t *ztm, uint8_t reason) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(reason);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    ;
}

void _z_multicast_transport_clear(_z_transport_t *zt) { _ZP_UNUSED(zt); }
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1
