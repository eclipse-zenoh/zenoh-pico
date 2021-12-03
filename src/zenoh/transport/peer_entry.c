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

#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/utils.h"

void _zn_transport_peer_entry_clear(_zn_transport_peer_entry_t *src)
{
    _z_wbuf_clear(&src->dbuf_reliable);
    _z_wbuf_clear(&src->dbuf_best_effort);

    _z_bytes_clear(&src->remote_pid);
    _z_bytes_clear(&src->remote_addr);
}

void _zn_transport_peer_entry_copy(_zn_transport_peer_entry_t *dst, const _zn_transport_peer_entry_t *src)
{
    _z_wbuf_copy(&dst->dbuf_reliable, &src->dbuf_reliable);
    _z_wbuf_copy(&dst->dbuf_best_effort, &src->dbuf_best_effort);

    dst->sn_resolution = src->sn_resolution;
    dst->sn_resolution_half = src->sn_resolution_half;
    _zn_conduit_sn_list_copy(&dst->sn_rx_sns, &src->sn_rx_sns);

    dst->lease = src->lease;
    dst->lease_expires = src->lease_expires;
    dst->received = src->received;

    _z_bytes_copy(&dst->remote_pid, &src->remote_pid);
    _z_bytes_copy(&dst->remote_addr, &src->remote_addr);
}

size_t _zn_transport_peer_entry_size(const _zn_transport_peer_entry_t *src)
{
    return sizeof(_zn_transport_peer_entry_size);
}

int _zn_transport_peer_entry_eq(const _zn_transport_peer_entry_t *left, const _zn_transport_peer_entry_t *right)
{
    if (left->remote_pid.len != right->remote_pid.len)
        return 0; // False

    if (memcmp(left->remote_pid.val, right->remote_pid.val, left->remote_pid.len) != 0)
        return 0; // False

    return 1; // True
}