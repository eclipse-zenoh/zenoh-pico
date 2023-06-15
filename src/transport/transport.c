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

#include "zenoh-pico/transport/transport.h"

#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/transport/link/tx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_UNICAST_TRANSPORT == 1
int8_t _z_unicast_send_close(_z_transport_unicast_t *ztu, uint8_t reason, _Bool link_only) {
    int8_t ret = _Z_RES_OK;

    _z_bytes_t zid = _z_bytes_wrap(((_z_session_t *)ztu->_session)->_local_zid.start,
                                   ((_z_session_t *)ztu->_session)->_local_zid.len);
    _z_transport_message_t cm = _z_t_msg_make_close(reason, zid, link_only);

    ret = _z_unicast_send_t_msg(ztu, &cm);
    _z_t_msg_clear(&cm);

    return ret;
}
#endif  // Z_UNICAST_TRANSPORT == 1

#if Z_MULTICAST_TRANSPORT == 1
int8_t _z_multicast_send_close(_z_transport_multicast_t *ztm, uint8_t reason, _Bool link_only) {
    int8_t ret = _Z_RES_OK;

    _z_bytes_t zid = _z_bytes_wrap(((_z_session_t *)ztm->_session)->_local_zid.start,
                                   ((_z_session_t *)ztm->_session)->_local_zid.len);
    _z_transport_message_t cm = _z_t_msg_make_close(reason, zid, link_only);

    ret = _z_multicast_send_t_msg(ztm, &cm);
    _z_t_msg_clear(&cm);

    return ret;
}
#endif  // Z_MULTICAST_TRANSPORT == 1

int8_t _z_send_close(_z_transport_t *zt, uint8_t reason, _Bool link_only) {
    int8_t ret = _Z_RES_OK;

#if Z_UNICAST_TRANSPORT == 1
    if (zt->_type == _Z_TRANSPORT_UNICAST_TYPE) {
        ret = _z_unicast_send_close(&zt->_transport._unicast, reason, link_only);
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (zt->_type == _Z_TRANSPORT_MULTICAST_TYPE) {
        ret = _z_multicast_send_close(&zt->_transport._multicast, reason, link_only);
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
    {
        ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    return ret;
}

#if Z_UNICAST_TRANSPORT == 1
int8_t _z_transport_unicast(_z_transport_t *zt, _z_link_t *zl, _z_transport_unicast_establish_param_t *param) {
    int8_t ret = _Z_RES_OK;

    zt->_type = _Z_TRANSPORT_UNICAST_TYPE;

#if Z_MULTI_THREAD == 1
    // Initialize the mutexes
    ret = _z_mutex_init(&zt->_transport._unicast._mutex_tx);
    if (ret == _Z_RES_OK) {
        ret = _z_mutex_init(&zt->_transport._unicast._mutex_rx);
        if (ret != _Z_RES_OK) {
            _z_mutex_free(&zt->_transport._unicast._mutex_tx);
        }
    }
#endif  // Z_MULTI_THREAD == 1

    // Initialize the read and write buffers
    if (ret == _Z_RES_OK) {
        uint16_t mtu = (zl->_mtu < Z_BATCH_SIZE_TX) ? zl->_mtu : Z_BATCH_SIZE_TX;
        zt->_transport._unicast._wbuf = _z_wbuf_make(mtu, false);
        zt->_transport._unicast._zbuf = _z_zbuf_make(Z_BATCH_SIZE_RX);

        // Initialize the defragmentation buffers
        size_t dbuf_size = 0;
        _Bool dbuf_expandable = true;
#if Z_DYNAMIC_MEMORY_ALLOCATION == 0
        dbuf_size = Z_FRAG_MAX_SIZE;
        dbuf_expandable = false;
#endif
        zt->_transport._unicast._dbuf_reliable = _z_wbuf_make(dbuf_size, dbuf_expandable);
        zt->_transport._unicast._dbuf_best_effort = _z_wbuf_make(dbuf_size, dbuf_expandable);

        // Clean up the buffers if one of them failed to be allocated
        if ((_z_wbuf_capacity(&zt->_transport._unicast._wbuf) != mtu) ||
            (_z_zbuf_capacity(&zt->_transport._unicast._zbuf) != Z_BATCH_SIZE_RX) ||
#if Z_DYNAMIC_MEMORY_ALLOCATION == 0
            (_z_wbuf_capacity(&zt->_transport._unicast._dbuf_reliable) != dbuf_size) ||
            (_z_wbuf_capacity(&zt->_transport._unicast._dbuf_best_effort) != dbuf_size)) {
#else
            (_z_wbuf_capacity(&zt->_transport._unicast._dbuf_reliable) != Z_IOSLICE_SIZE) ||
            (_z_wbuf_capacity(&zt->_transport._unicast._dbuf_best_effort) != Z_IOSLICE_SIZE)) {
#endif
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;

#if Z_MULTI_THREAD == 1
            _z_mutex_free(&zt->_transport._unicast._mutex_tx);
            _z_mutex_free(&zt->_transport._unicast._mutex_rx);
#endif  // Z_MULTI_THREAD == 1

            _z_wbuf_clear(&zt->_transport._unicast._wbuf);
            _z_zbuf_clear(&zt->_transport._unicast._zbuf);
            _z_wbuf_clear(&zt->_transport._unicast._dbuf_reliable);
            _z_wbuf_clear(&zt->_transport._unicast._dbuf_best_effort);
        }
    }

    if (ret == _Z_RES_OK) {
        // Set default SN resolution
        zt->_transport._unicast._sn_resolution = param->_sn_resolution;
        zt->_transport._unicast._sn_resolution_half = param->_sn_resolution / 2;

        // The initial SN at TX side
        zt->_transport._unicast._sn_tx_reliable = param->_initial_sn_tx;
        zt->_transport._unicast._sn_tx_best_effort = param->_initial_sn_tx;

        // The initial SN at RX side
        zt->_transport._unicast._sn_rx_reliable = param->_initial_sn_rx;
        zt->_transport._unicast._sn_rx_best_effort = param->_initial_sn_rx;

#if Z_MULTI_THREAD == 1
        // Tasks
        zt->_transport._unicast._read_task_running = false;
        zt->_transport._unicast._read_task = NULL;
        zt->_transport._unicast._lease_task_running = false;
        zt->_transport._unicast._lease_task = NULL;
#endif  // Z_MULTI_THREAD == 1

        // Notifiers
        zt->_transport._unicast._received = 0;
        zt->_transport._unicast._transmitted = 0;

        // Transport lease
        zt->_transport._unicast._lease = param->_lease;

        // Transport link for unicast
        zt->_transport._unicast._link = *zl;

        // Remote peer PID
        _z_bytes_move(&zt->_transport._unicast._remote_zid, &param->_remote_zid);
    } else {
        _z_bytes_clear(&param->_remote_zid);
    }

    return ret;
}
#endif  // Z_UNICAST_TRANSPORT == 1

#if Z_MULTICAST_TRANSPORT == 1
int8_t _z_transport_multicast(_z_transport_t *zt, _z_link_t *zl, _z_transport_multicast_establish_param_t *param) {
    int8_t ret = _Z_RES_OK;

    zt->_type = _Z_TRANSPORT_MULTICAST_TYPE;

#if Z_MULTI_THREAD == 1
    // Initialize the mutexes
    ret = _z_mutex_init(&zt->_transport._multicast._mutex_tx);
    if (ret == _Z_RES_OK) {
        ret = _z_mutex_init(&zt->_transport._multicast._mutex_rx);
        if (ret == _Z_RES_OK) {
            ret = _z_mutex_init(&zt->_transport._multicast._mutex_peer);
            if (ret != _Z_RES_OK) {
                _z_mutex_free(&zt->_transport._multicast._mutex_tx);
                _z_mutex_free(&zt->_transport._multicast._mutex_rx);
            }
        } else {
            _z_mutex_free(&zt->_transport._multicast._mutex_tx);
        }
    }
#endif  // Z_MULTI_THREAD == 1

    // Initialize the read and write buffers
    if (ret == _Z_RES_OK) {
        uint16_t mtu = (zl->_mtu < Z_BATCH_SIZE_TX) ? zl->_mtu : Z_BATCH_SIZE_TX;
        zt->_transport._multicast._wbuf = _z_wbuf_make(mtu, false);
        zt->_transport._multicast._zbuf = _z_zbuf_make(Z_BATCH_SIZE_RX);

        // Clean up the buffers if one of them failed to be allocated
        if ((_z_wbuf_capacity(&zt->_transport._multicast._wbuf) != mtu) ||
            (_z_zbuf_capacity(&zt->_transport._multicast._zbuf) != Z_BATCH_SIZE_RX)) {
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;

#if Z_MULTI_THREAD == 1
            _z_mutex_free(&zt->_transport._multicast._mutex_tx);
            _z_mutex_free(&zt->_transport._multicast._mutex_rx);
            _z_mutex_free(&zt->_transport._multicast._mutex_peer);
#endif  // Z_MULTI_THREAD == 1

            _z_wbuf_clear(&zt->_transport._multicast._wbuf);
            _z_zbuf_clear(&zt->_transport._multicast._zbuf);
        }
    }

    if (ret == _Z_RES_OK) {
        // Set default SN resolution
        zt->_transport._multicast._sn_resolution = param->_sn_resolution;
        zt->_transport._multicast._sn_resolution_half = param->_sn_resolution / 2;

        // The initial SN at TX side
        zt->_transport._multicast._sn_tx_reliable = param->_initial_sn_tx;
        zt->_transport._multicast._sn_tx_best_effort = param->_initial_sn_tx;

        // Initialize peer list
        zt->_transport._multicast._peers = _z_transport_peer_entry_list_new();

#if Z_MULTI_THREAD == 1
        // Tasks
        zt->_transport._multicast._read_task_running = false;
        zt->_transport._multicast._read_task = NULL;
        zt->_transport._multicast._lease_task_running = false;
        zt->_transport._multicast._lease_task = NULL;
#endif  // Z_MULTI_THREAD == 1

        zt->_transport._multicast._lease = Z_TRANSPORT_LEASE;

        // Notifiers
        zt->_transport._multicast._transmitted = false;

        // Transport link for multicast
        zt->_transport._multicast._link = *zl;
    }

    return ret;
}
#endif  // Z_MULTICAST_TRANSPORT == 1

#if Z_UNICAST_TRANSPORT == 1
int8_t _z_transport_unicast_open_client(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                        const _z_bytes_t *local_zid) {
    int8_t ret = _Z_RES_OK;

    // Build the open message
    uint8_t version = Z_PROTO_VERSION;
    z_whatami_t whatami = Z_WHATAMI_CLIENT;
    _z_zint_t sn_resolution = Z_SN_RESOLUTION;
    _Bool is_qos = false;

    _z_bytes_t zid = _z_bytes_wrap(local_zid->start, local_zid->len);
    _z_transport_message_t ism = _z_t_msg_make_init_syn(version, whatami, sn_resolution, zid, is_qos);
    param->_sn_resolution = ism._body._init._sn_resolution;  // The announced sn resolution

    // Encode and send the message
    _Z_INFO("Sending Z_INIT(Syn)\n");
    ret = _z_link_send_t_msg(zl, &ism);
    _z_t_msg_clear(&ism);
    if (ret == _Z_RES_OK) {
        _z_transport_message_t iam;
        ret = _z_link_recv_t_msg(&iam, zl);
        if (ret == _Z_RES_OK) {
            if ((_Z_MID(iam._header) == _Z_MID_INIT) && (_Z_HAS_FLAG(iam._header, _Z_FLAG_T_A) == true)) {
                _Z_INFO("Received Z_INIT(Ack)\n");

                // Handle SN resolution option if present
                if (_Z_HAS_FLAG(iam._header, _Z_FLAG_T_S) == true) {
                    // The resolution in the InitAck must be less or equal than the resolution in the InitSyn,
                    // otherwise the InitAck message is considered invalid and it should be treated as a
                    // CLOSE message with L==0 by the Initiating Peer -- the recipient of the InitAck message.
                    if (iam._body._init._sn_resolution <= param->_sn_resolution) {
                        param->_sn_resolution = iam._body._init._sn_resolution;
                    } else {
                        ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
                    }
                }

                if (ret == _Z_RES_OK) {
                    // The initial SN at TX side
                    z_random_fill(&param->_initial_sn_tx, sizeof(param->_initial_sn_tx));
                    param->_initial_sn_tx = param->_initial_sn_tx % param->_sn_resolution;

                    // Initialize the Local and Remote Peer IDs
                    _z_bytes_copy(&param->_remote_zid, &iam._body._init._zid);

                    // Create the OpenSyn message
                    _z_zint_t lease = Z_TRANSPORT_LEASE;
                    _z_zint_t initial_sn = param->_initial_sn_tx;
                    _z_bytes_t cookie;
                    _z_bytes_copy(&cookie, &iam._body._init._cookie);

                    _z_transport_message_t osm = _z_t_msg_make_open_syn(lease, initial_sn, cookie);

                    // Encode and send the message
                    _Z_INFO("Sending Z_OPEN(Syn)\n");
                    ret = _z_link_send_t_msg(zl, &osm);
                    if (ret == _Z_RES_OK) {
                        _z_transport_message_t oam;
                        ret = _z_link_recv_t_msg(&oam, zl);
                        if (ret == _Z_RES_OK) {
                            if ((_Z_MID(oam._header) == _Z_MID_OPEN) &&
                                (_Z_HAS_FLAG(oam._header, _Z_FLAG_T_A) == true)) {
                                _Z_INFO("Received Z_OPEN(Ack)\n");
                                param->_lease = oam._body._open._lease;  // The session lease

                                // The initial SN at RX side. Initialize the session as we had already received
                                // a message with a SN equal to initial_sn - 1.
                                param->_initial_sn_rx =
                                    _z_sn_decrement(param->_sn_resolution, oam._body._open._initial_sn);
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

#endif  // Z_UNICAST_TRANSPORT == 1

#if Z_MULTICAST_TRANSPORT == 1
int8_t _z_transport_multicast_open_client(_z_transport_multicast_establish_param_t *param, const _z_link_t *zl,
                                          const _z_bytes_t *local_zid) {
    (void)(param);
    (void)(zl);
    (void)(local_zid);
    int8_t ret = _Z_ERR_CONFIG_UNSUPPORTED_CLIENT_MULTICAST;

    // @TODO: not implemented

    return ret;
}
#endif  // Z_MULTICAST_TRANSPORT == 1

#if Z_UNICAST_TRANSPORT == 1
int8_t _z_transport_unicast_open_peer(_z_transport_unicast_establish_param_t *param, const _z_link_t *zl,
                                      const _z_bytes_t *local_zid) {
    (void)(param);
    (void)(zl);
    (void)(local_zid);
    int8_t ret = _Z_ERR_CONFIG_UNSUPPORTED_PEER_UNICAST;

    // @TODO: not implemented

    return ret;
}
#endif  // Z_UNICAST_TRANSPORT == 1

#if Z_MULTICAST_TRANSPORT == 1
int8_t _z_transport_multicast_open_peer(_z_transport_multicast_establish_param_t *param, const _z_link_t *zl,
                                        const _z_bytes_t *local_zid) {
    int8_t ret = _Z_RES_OK;

    param->_is_qos = false;  // FIXME: make transport aware of qos configuration
    param->_initial_sn_tx = 0;
    param->_sn_resolution = Z_SN_RESOLUTION;

    // Explicitly send a JOIN message upon startup
    // FIXME: make transport aware of qos configuration
    _z_conduit_sn_list_t next_sns;
    next_sns._is_qos = false;
    next_sns._val._plain._best_effort = param->_initial_sn_tx;
    next_sns._val._plain._reliable = param->_initial_sn_tx;

    _z_bytes_t zid = _z_bytes_wrap(local_zid->start, local_zid->len);
    _z_transport_message_t jsm =
        _z_t_msg_make_join(Z_PROTO_VERSION, Z_WHATAMI_PEER, Z_TRANSPORT_LEASE, param->_sn_resolution, zid, next_sns);

    // Encode and send the message
    _Z_INFO("Sending Z_JOIN message\n");
    ret = _z_link_send_t_msg(zl, &jsm);
    _z_t_msg_clear(&jsm);

    return ret;
}
#endif  // Z_MULTICAST_TRANSPORT == 1

#if Z_UNICAST_TRANSPORT == 1
int8_t _z_transport_unicast_close(_z_transport_unicast_t *ztu, uint8_t reason) {
    return _z_unicast_send_close(ztu, reason, false);
}
#endif  // Z_UNICAST_TRANSPORT == 1

#if Z_MULTICAST_TRANSPORT == 1
int8_t _z_transport_multicast_close(_z_transport_multicast_t *ztm, uint8_t reason) {
    return _z_multicast_send_close(ztm, reason, false);
}
#endif  // Z_MULTICAST_TRANSPORT == 1

int8_t _z_transport_close(_z_transport_t *zt, uint8_t reason) { return _z_send_close(zt, reason, false); }

#if Z_UNICAST_TRANSPORT == 1
void _z_transport_unicast_clear(_z_transport_unicast_t *ztu) {
#if Z_MULTI_THREAD == 1
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
    _z_mutex_free(&ztu->_mutex_tx);
    _z_mutex_free(&ztu->_mutex_rx);
#endif  // Z_MULTI_THREAD == 1

    // Clean up the buffers
    _z_wbuf_clear(&ztu->_wbuf);
    _z_zbuf_clear(&ztu->_zbuf);
    _z_wbuf_clear(&ztu->_dbuf_reliable);
    _z_wbuf_clear(&ztu->_dbuf_best_effort);

    // Clean up PIDs
    _z_bytes_clear(&ztu->_remote_zid);
    _z_link_clear(&ztu->_link);
}
#endif  // Z_UNICAST_TRANSPORT == 1

#if Z_MULTICAST_TRANSPORT == 1
void _z_transport_multicast_clear(_z_transport_multicast_t *ztm) {
#if Z_MULTI_THREAD == 1
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
    _z_mutex_free(&ztm->_mutex_tx);
    _z_mutex_free(&ztm->_mutex_rx);
    _z_mutex_free(&ztm->_mutex_peer);
#endif  // Z_MULTI_THREAD == 1

    // Clean up the buffers
    _z_wbuf_clear(&ztm->_wbuf);
    _z_zbuf_clear(&ztm->_zbuf);

    // Clean up peer list
    _z_transport_peer_entry_list_free(&ztm->_peers);
    _z_link_clear(&ztm->_link);
}
#endif  // Z_MULTICAST_TRANSPORT == 1

void _z_transport_clear(_z_transport_t *zt) {
#if Z_UNICAST_TRANSPORT == 1
    if (zt->_type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_transport_unicast_clear(&zt->_transport._unicast);
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (zt->_type == _Z_TRANSPORT_MULTICAST_TYPE) {
        _z_transport_multicast_clear(&zt->_transport._multicast);
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
    {
        __asm__("nop");
    }
}

void _z_transport_free(_z_transport_t **zt) {
    _z_transport_t *ptr = *zt;

    if (ptr != NULL) {
        _z_transport_clear(ptr);

        z_free(ptr);
        *zt = NULL;
    }
}
