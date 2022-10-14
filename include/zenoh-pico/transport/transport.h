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

#ifndef ZENOH_PICO_TRANSPORT_TYPES_H
#define ZENOH_PICO_TRANSPORT_TYPES_H

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"

typedef struct {
    // Defragmentation buffers
    _z_wbuf_t _dbuf_reliable;
    _z_wbuf_t _dbuf_best_effort;

    // SN numbers
    _z_zint_t _sn_resolution;
    _z_zint_t _sn_resolution_half;
    _z_conduit_sn_list_t _sn_rx_sns;

    _z_bytes_t _remote_pid;
    _z_bytes_t _remote_addr;

    volatile _z_zint_t _lease;
    volatile _z_zint_t _next_lease;
    volatile int _received;
} _z_transport_peer_entry_t;

size_t _z_transport_peer_entry_size(const _z_transport_peer_entry_t *src);
void _z_transport_peer_entry_clear(_z_transport_peer_entry_t *src);
void _z_transport_peer_entry_copy(_z_transport_peer_entry_t *dst, const _z_transport_peer_entry_t *src);
_Bool _z_transport_peer_entry_eq(const _z_transport_peer_entry_t *left, const _z_transport_peer_entry_t *right);
_Z_ELEM_DEFINE(_z_transport_peer_entry, _z_transport_peer_entry_t, _z_transport_peer_entry_size,
               _z_transport_peer_entry_clear, _z_transport_peer_entry_copy)
_Z_LIST_DEFINE(_z_transport_peer_entry, _z_transport_peer_entry_t)

typedef struct {
    // Session associated to the transport
    void *_session;

#if Z_MULTI_THREAD == 1
    // TX and RX mutexes
    _z_mutex_t _mutex_rx;
    _z_mutex_t _mutex_tx;
#endif  // Z_MULTI_THREAD == 1

    // Defragmentation buffers
    _z_wbuf_t _dbuf_reliable;
    _z_wbuf_t _dbuf_best_effort;

    // SN numbers
    _z_zint_t _sn_resolution;
    _z_zint_t _sn_resolution_half;
    _z_zint_t _sn_tx_reliable;
    _z_zint_t _sn_tx_best_effort;
    _z_zint_t _sn_rx_reliable;
    _z_zint_t _sn_rx_best_effort;

    _z_bytes_t _remote_pid;

    // ----------- Link related -----------
    // TX and RX buffers
    const _z_link_t *_link;
    _z_wbuf_t _wbuf;
    _z_zbuf_t _zbuf;

    volatile int _received;
    volatile int _transmitted;

#if Z_MULTI_THREAD == 1
    volatile int _read_task_running;
    _z_task_t *_read_task;
    volatile int _lease_task_running;
    _z_task_t *_lease_task;
#endif  // Z_MULTI_THREAD == 1

    volatile _z_zint_t _lease;
} _z_transport_unicast_t;

typedef struct {
    // Session associated to the transport
    void *_session;

#if Z_MULTI_THREAD == 1
    // TX and RX mutexes
    _z_mutex_t _mutex_rx;
    _z_mutex_t _mutex_tx;

    // Peer list mutex
    _z_mutex_t _mutex_peer;
#endif  // Z_MULTI_THREAD == 1

    // Known valid peers
    _z_transport_peer_entry_list_t *_peers;

    // SN initial numbers
    _z_zint_t _sn_resolution;
    _z_zint_t _sn_resolution_half;
    _z_zint_t _sn_tx_reliable;
    _z_zint_t _sn_tx_best_effort;

    // ----------- Link related -----------
    // TX and RX buffers
    const _z_link_t *_link;
    _z_wbuf_t _wbuf;
    _z_zbuf_t _zbuf;

    volatile int _transmitted;

#if Z_MULTI_THREAD == 1
    volatile int _read_task_running;
    _z_task_t *_read_task;
    volatile int _lease_task_running;
    _z_task_t *_lease_task;
#endif  // Z_MULTI_THREAD == 1

    volatile _z_zint_t _lease;
} _z_transport_multicast_t;

typedef struct {
    union {
        _z_transport_unicast_t _unicast;
        _z_transport_multicast_t _multicast;
    } _transport;

    enum {
        _Z_TRANSPORT_UNICAST_TYPE,
        _Z_TRANSPORT_MULTICAST_TYPE,
    } _type;
} _z_transport_t;

_Z_ELEM_DEFINE(_z_transport, _z_transport_t, _z_noop_size, _z_noop_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_transport, _z_transport_t)

_Z_RESULT_DECLARE(_z_transport_t, transport)
_Z_P_RESULT_DECLARE(_z_transport_t, transport)

typedef struct {
    _z_bytes_t _remote_pid;
    unsigned int _whatami;
    _z_zint_t _sn_resolution;
    _z_zint_t _initial_sn_rx;
    _z_zint_t _initial_sn_tx;
    uint8_t _is_qos;
    _z_zint_t _lease;
} _z_transport_unicast_establish_param_t;

_Z_RESULT_DECLARE(_z_transport_unicast_establish_param_t, transport_unicast_establish_param)

typedef struct {
    _z_zint_t _sn_resolution;
    _z_zint_t _initial_sn_tx;
    uint8_t _is_qos;
} _z_transport_multicast_establish_param_t;

_Z_RESULT_DECLARE(_z_transport_multicast_establish_param_t, transport_multicast_establish_param)

_z_transport_t *_z_transport_unicast_new(_z_link_t *link, _z_transport_unicast_establish_param_t param);
_z_transport_t *_z_transport_multicast_new(_z_link_t *link, _z_transport_multicast_establish_param_t param);

_z_transport_unicast_establish_param_result_t _z_transport_unicast_open_client(const _z_link_t *zl,
                                                                               const _z_bytes_t local_pid);
_z_transport_multicast_establish_param_result_t _z_transport_multicast_open_client(const _z_link_t *zl,
                                                                                   const _z_bytes_t local_pid);
_z_transport_unicast_establish_param_result_t _z_transport_unicast_open_peer(const _z_link_t *zl,
                                                                             const _z_bytes_t local_pid);
_z_transport_multicast_establish_param_result_t _z_transport_multicast_open_peer(const _z_link_t *zl,
                                                                                 const _z_bytes_t local_pid);

int _z_transport_close(_z_transport_t *zt, uint8_t reason);
int _z_transport_unicast_close(_z_transport_unicast_t *ztu, uint8_t reason);
int _z_transport_multicast_close(_z_transport_multicast_t *ztm, uint8_t reason);

void _z_transport_unicast_clear(_z_transport_unicast_t *ztu);
void _z_transport_multicast_clear(_z_transport_multicast_t *ztm);

void _z_transport_free(_z_transport_t **zt);
void _z_transport_unicast_free(_z_transport_unicast_t **ztu);
void _z_transport_multicast_free(_z_transport_multicast_t **ztm);

#endif /* ZENOH_PICO_TRANSPORT_TYPES_H */
