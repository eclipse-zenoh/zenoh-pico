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
#include "zenoh-pico/transport/common/rx.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/unicast.h"
#include "zenoh-pico/transport/unicast/lease.h"
#include "zenoh-pico/transport/unicast/read.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/transport/unicast/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

z_result_t _z_unicast_transport_create(_z_transport_t *zt, _z_link_t *zl,
                                       _z_transport_unicast_establish_param_t *param) {
    z_result_t ret = _Z_RES_OK;

    zt->_type = _Z_TRANSPORT_UNICAST_TYPE;
    _z_transport_unicast_t *ztu = &zt->_transport._unicast;

// Initialize batching data
#if Z_FEATURE_BATCHING == 1
    ztu->_batch_state = _Z_BATCHING_IDLE;
    ztu->_batch = _z_network_message_vec_make(0);
#endif

#if Z_FEATURE_MULTI_THREAD == 1
    // Initialize the mutexes
    ret = _z_mutex_init(&ztu->_mutex_tx);
    if (ret == _Z_RES_OK) {
        ret = _z_mutex_init(&ztu->_mutex_rx);
        if (ret != _Z_RES_OK) {
            _z_mutex_drop(&ztu->_mutex_tx);
        }
    }
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Initialize the read and write buffers
    if (ret == _Z_RES_OK) {
        uint16_t mtu = (zl->_mtu < param->_batch_size) ? zl->_mtu : param->_batch_size;
        size_t wbuf_size = mtu;
        size_t zbuf_size = param->_batch_size;

        // Initialize tx rx buffers
        ztu->_wbuf = _z_wbuf_make(wbuf_size, false);
        ztu->_zbuf = _z_zbuf_make(zbuf_size);

        // Clean up the buffers if one of them failed to be allocated
        if ((_z_wbuf_capacity(&ztu->_wbuf) != wbuf_size) || (_z_zbuf_capacity(&ztu->_zbuf) != zbuf_size)) {
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            _Z_ERROR("Not enough memory to allocate transport tx rx buffers!");

#if Z_FEATURE_MULTI_THREAD == 1
            _z_mutex_drop(&ztu->_mutex_tx);
            _z_mutex_drop(&ztu->_mutex_rx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

            _z_wbuf_clear(&ztu->_wbuf);
            _z_zbuf_clear(&ztu->_zbuf);
        }

#if Z_FEATURE_FRAGMENTATION == 1
        // Initialize the defragmentation buffers
        ztu->_state_reliable = _Z_DBUF_STATE_NULL;
        ztu->_state_best_effort = _Z_DBUF_STATE_NULL;
        ztu->_dbuf_reliable = _z_wbuf_null();
        ztu->_dbuf_best_effort = _z_wbuf_null();
#endif  // Z_FEATURE_FRAGMENTATION == 1
    }

    if (ret == _Z_RES_OK) {
        // Set default SN resolution
        ztu->_sn_res = _z_sn_max(param->_seq_num_res);

        // The initial SN at TX side
        ztu->_sn_tx_reliable = param->_initial_sn_tx;
        ztu->_sn_tx_best_effort = param->_initial_sn_tx;

        // The initial SN at RX side
        _z_zint_t initial_sn_rx = _z_sn_decrement(ztu->_sn_res, param->_initial_sn_rx);
        ztu->_sn_rx_reliable = initial_sn_rx;
        ztu->_sn_rx_best_effort = initial_sn_rx;

#if Z_FEATURE_MULTI_THREAD == 1
        // Tasks
        ztu->_read_task_running = false;
        ztu->_read_task = NULL;
        ztu->_lease_task_running = false;
        ztu->_lease_task = NULL;
#endif  // Z_FEATURE_MULTI_THREAD == 1

        // Notifiers
        ztu->_received = 0;
        ztu->_transmitted = 0;

        // Transport lease
        ztu->_lease = param->_lease;

        // Transport link for unicast
        ztu->_link = *zl;

        // Remote peer PID
        ztu->_remote_zid = param->_remote_zid;
    } else {
        param->_remote_zid = _z_id_empty();
    }

    return ret;
}

z_result_t _z_unicast_open_client(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                  const _z_id_t *local_zid) {
    z_result_t ret = _Z_RES_OK;

    _z_id_t zid = *local_zid;
    _z_transport_message_t ism = _z_t_msg_make_init_syn(Z_WHATAMI_CLIENT, zid);
    param->_seq_num_res = ism._body._init._seq_num_res;  // The announced sn resolution
    param->_req_id_res = ism._body._init._req_id_res;    // The announced req id resolution
    param->_batch_size = ism._body._init._batch_size;    // The announced batch size

    // Encode and send the message
    _Z_DEBUG("Sending Z_INIT(Syn)");
    ret = _z_link_send_t_msg(zl, &ism);
    _z_t_msg_clear(&ism);
    if (ret == _Z_RES_OK) {
        _z_transport_message_t iam;
        ret = _z_link_recv_t_msg(&iam, zl);
        if (ret == _Z_RES_OK) {
            if ((_Z_MID(iam._header) == _Z_MID_T_INIT) && (_Z_HAS_FLAG(iam._header, _Z_FLAG_T_INIT_A) == true)) {
                _Z_DEBUG("Received Z_INIT(Ack)");

                // Any of the size parameters in the InitAck must be less or equal than the one in the InitSyn,
                // otherwise the InitAck message is considered invalid and it should be treated as a
                // CLOSE message with L==0 by the Initiating Peer -- the recipient of the InitAck message.
                if (iam._body._init._seq_num_res <= param->_seq_num_res) {
                    param->_seq_num_res = iam._body._init._seq_num_res;
                } else {
                    ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
                }

                if (iam._body._init._req_id_res <= param->_req_id_res) {
                    param->_req_id_res = iam._body._init._req_id_res;
                } else {
                    ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
                }

                if (iam._body._init._batch_size <= param->_batch_size) {
                    param->_batch_size = iam._body._init._batch_size;
                } else {
                    ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
                }

                if (ret == _Z_RES_OK) {
                    param->_key_id_res = 0x08 << param->_key_id_res;
                    param->_req_id_res = 0x08 << param->_req_id_res;

                    // The initial SN at TX side
                    z_random_fill(&param->_initial_sn_tx, sizeof(param->_initial_sn_tx));
                    param->_initial_sn_tx = param->_initial_sn_tx & !_z_sn_modulo_mask(param->_seq_num_res);

                    // Initialize the Local and Remote Peer IDs
                    param->_remote_zid = iam._body._init._zid;

                    // Create the OpenSyn message
                    _z_zint_t lease = Z_TRANSPORT_LEASE;
                    _z_zint_t initial_sn = param->_initial_sn_tx;
                    _z_slice_t cookie;
                    _z_slice_copy(&cookie, &iam._body._init._cookie);

                    _z_transport_message_t osm = _z_t_msg_make_open_syn(lease, initial_sn, cookie);

                    // Encode and send the message
                    _Z_DEBUG("Sending Z_OPEN(Syn)");
                    ret = _z_link_send_t_msg(zl, &osm);
                    if (ret == _Z_RES_OK) {
                        _z_transport_message_t oam;
                        ret = _z_link_recv_t_msg(&oam, zl);
                        if (ret == _Z_RES_OK) {
                            if ((_Z_MID(oam._header) == _Z_MID_T_OPEN) &&
                                (_Z_HAS_FLAG(oam._header, _Z_FLAG_T_OPEN_A) == true)) {
                                _Z_DEBUG("Received Z_OPEN(Ack)");
                                param->_lease = oam._body._open._lease;  // The session lease

                                // The initial SN at RX side. Initialize the session as we had already received
                                // a message with a SN equal to initial_sn - 1.
                                param->_initial_sn_rx = oam._body._open._initial_sn;
                            } else {
                                ret = _Z_ERR_MESSAGE_UNEXPECTED;
                            }
                            _z_t_msg_clear(&oam);
                        }
                    }
                    _z_t_msg_clear(&osm);
                }
            } else {
                ret = _Z_ERR_MESSAGE_UNEXPECTED;
            }
            _z_t_msg_clear(&iam);
        }
    }

    return ret;
}

z_result_t _z_unicast_open_peer(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                const _z_id_t *local_zid, bool is_open) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    z_result_t ret = _Z_ERR_CONFIG_UNSUPPORTED_PEER_UNICAST;
    // @TODO: not implemented
    return ret;
}

z_result_t _z_unicast_send_close(_z_transport_unicast_t *ztu, uint8_t reason, bool link_only) {
    z_result_t ret = _Z_RES_OK;
    // Send and clear message
    _z_transport_message_t cm = _z_t_msg_make_close(reason, link_only);
    ret = _z_unicast_send_t_msg(ztu, &cm);
    _z_t_msg_clear(&cm);
    return ret;
}

z_result_t _z_unicast_transport_close(_z_transport_unicast_t *ztu, uint8_t reason) {
    return _z_unicast_send_close(ztu, reason, false);
}

void _z_unicast_transport_clear(_z_transport_t *zt) {
    _z_transport_unicast_t *ztu = &zt->_transport._unicast;
#if Z_FEATURE_MULTI_THREAD == 1
    // Clean up tasks
    if (ztu->_read_task != NULL) {
        _z_task_join(ztu->_read_task);
        _z_task_free(&ztu->_read_task);
    }
    if (ztu->_lease_task != NULL) {
        _z_task_join(ztu->_lease_task);
        _z_task_free(&ztu->_lease_task);
    }

    // Clean up the mutexes
    _z_mutex_drop(&ztu->_mutex_tx);
    _z_mutex_drop(&ztu->_mutex_rx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

#if Z_FEATURE_BATCHING == 1
    _z_network_message_vec_clear(&ztu->_batch);
#endif

    // Clean up the buffers
    _z_wbuf_clear(&ztu->_wbuf);
    _z_zbuf_clear(&ztu->_zbuf);
#if Z_FEATURE_FRAGMENTATION == 1
    _z_wbuf_clear(&ztu->_dbuf_reliable);
    _z_wbuf_clear(&ztu->_dbuf_best_effort);
#endif

    // Clean up PIDs
    ztu->_remote_zid = _z_id_empty();
    _z_link_clear(&ztu->_link);
}

#else

z_result_t _z_unicast_transport_create(_z_transport_t *zt, _z_link_t *zl,
                                       _z_transport_unicast_establish_param_t *param) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(param);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_open_client(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                  const _z_id_t *local_zid) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_open_peer(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                const _z_id_t *local_zid, bool is_open) {
    _ZP_UNUSED(param);
    _ZP_UNUSED(zl);
    _ZP_UNUSED(local_zid);
    _ZP_UNUSED(is_open);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_send_close(_z_transport_unicast_t *ztu, uint8_t reason, bool link_only) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(reason);
    _ZP_UNUSED(link_only);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_unicast_transport_close(_z_transport_unicast_t *ztu, uint8_t reason) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(reason);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

void _z_unicast_transport_clear(_z_transport_t *zt) { _ZP_UNUSED(zt); }

#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1
