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

#ifndef ZENOH_PICO_TRANSPORT_TYPES_H
#define ZENOH_PICO_TRANSPORT_TYPES_H

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"

typedef struct
{
    // Session associated to the transport
    void *session;

    // TX and RX mutexes
    z_mutex_t mutex_rx;
    z_mutex_t mutex_tx;

    // Defragmentation buffers
    _z_wbuf_t dbuf_reliable;
    _z_wbuf_t dbuf_best_effort;

    // SN numbers
    z_zint_t sn_resolution;
    z_zint_t sn_resolution_half;
    z_zint_t sn_tx_reliable;
    z_zint_t sn_tx_best_effort;
    z_zint_t sn_rx_reliable;
    z_zint_t sn_rx_best_effort;

    z_bytes_t remote_pid;

    // ----------- Link related -----------
    // TX and RX buffers
    const _zn_link_t *link;
    _z_wbuf_t wbuf;
    _z_zbuf_t zbuf;

    volatile int received;
    volatile int transmitted;
    volatile int read_task_running;
    z_task_t *read_task;

    volatile int lease_task_running;
    z_task_t *lease_task;
    volatile z_zint_t lease;
} _zn_transport_unicast_t;

typedef struct
{
    // Session associated to the transport
    void *session;

    // TX and RX mutexes
    z_mutex_t mutex_rx;
    z_mutex_t mutex_tx;

    // Defragmentation buffers
    _z_vec_t/*<_z_wbuf_t>*/ dbuf_reliable_peers;
    _z_vec_t/*<_z_wbuf_t>*/ dbuf_best_effort_peers;
    _z_wbuf_t dbuf_reliable; // TODO: to delete
    _z_wbuf_t dbuf_best_effort; // TODO: to delete

    // SN initial numbers
    z_zint_t sn_resolution;
    z_zint_t sn_resolution_half;
    z_zint_t sn_rx_reliable;
    z_zint_t sn_rx_best_effort;
    z_zint_t sn_tx_reliable;
    z_zint_t sn_tx_best_effort;

    // SN numbers
    _z_vec_t/*<z_zint_t>*/ sn_resolution_peers;
    _z_vec_t/*<z_zint_t>*/ sn_resolution_half_peers;
    _z_vec_t/*<z_zint_t>*/ sn_rx_reliable_peers;
    _z_vec_t/*<z_zint_t>*/ sn_rx_best_effort_peers;

    _z_vec_t/*<z_bytes_t>*/ remote_pid;

    // ----------- Link related -----------
    // TX and RX buffers
    const _zn_link_t *link;
    _z_wbuf_t wbuf;
    _z_zbuf_t zbuf;

    volatile int received;
    volatile int transmitted;

    volatile int join_task_running;
    z_task_t *join_task;

    volatile int read_task_running;
    z_task_t *read_task;

    volatile int lease_task_running;
    z_task_t *lease_task;
    volatile z_zint_t lease;
} _zn_transport_multicast_t;

typedef struct
{
    union
    {
        _zn_transport_unicast_t unicast;
        _zn_transport_multicast_t multicast;
    } transport;

    enum
    {
        _ZN_TRANSPORT_UNICAST_TYPE,
        _ZN_TRANSPORT_MULTICAST_TYPE,
    } type;
} _zn_transport_t;

_ZN_RESULT_DECLARE(_zn_transport_t, transport)
_ZN_P_RESULT_DECLARE(_zn_transport_t, transport)

typedef struct {
    z_bytes_t remote_pid;
    unsigned int whatami;
    z_zint_t sn_resolution;
    z_zint_t initial_sn_rx;
    z_zint_t initial_sn_tx;
    uint8_t is_qos;
    z_zint_t lease;
} _zn_transport_unicast_establish_param_t;

_ZN_RESULT_DECLARE(_zn_transport_unicast_establish_param_t, transport_unicast_establish_param)

typedef struct {
    z_zint_t sn_resolution;
    z_zint_t initial_sn_rx;
    uint8_t is_qos;
    z_zint_t lease;
} _zn_transport_multicast_establish_param_t;

_ZN_RESULT_DECLARE(_zn_transport_multicast_establish_param_t, transport_multicast_establish_param)

_zn_transport_t *_zn_transport_unicast_init();
_zn_transport_t *_zn_transport_multicast_init();

_zn_transport_unicast_establish_param_result_t _zn_transport_unicast_open_client(const _zn_link_t *zl, const z_bytes_t local_pid);
_zn_transport_multicast_establish_param_result_t _zn_transport_multicast_open_client(const _zn_link_t *zl, const z_bytes_t local_pid);
_zn_transport_unicast_establish_param_result_t _zn_transport_unicast_open_peer(const _zn_link_t *zl, const z_bytes_t local_pid);
_zn_transport_multicast_establish_param_result_t _zn_transport_multicast_open_peer(const _zn_link_t *zl, const z_bytes_t local_pid);

int _zn_transport_close(_zn_transport_t *zt, uint8_t reason);
int _zn_transport_unicast_close(_zn_transport_unicast_t *ztu, uint8_t reason);
int _zn_transport_multicast_close(_zn_transport_multicast_t *ztm, uint8_t reason);

void _zn_transport_unicast_clear(_zn_transport_unicast_t *ztu);
void _zn_transport_multicast_clear(_zn_transport_multicast_t *ztm);

void _zn_transport_free(_zn_transport_t **zt);
void _zn_transport_unicast_free(_zn_transport_unicast_t **ztu);
void _zn_transport_multicast_free(_zn_transport_multicast_t **ztm);

#endif /* ZENOH_PICO_TRANSPORT_TYPES_H */
