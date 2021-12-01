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

_zn_transport_peer_entry_t *_zn_transport_peer_entry_clone(const _zn_transport_peer_entry_t *src)
{
    _zn_transport_peer_entry_t *ret = (_zn_transport_peer_entry_t*)malloc(sizeof(_zn_transport_peer_entry_t));

    _z_wbuf_copy_into(&ret->dbuf_reliable, &src->dbuf_reliable, src->dbuf_reliable.capacity);
    _z_wbuf_copy_into(&ret->dbuf_best_effort, &src->dbuf_best_effort, src->dbuf_best_effort.capacity);

    ret->sn_resolution = src->sn_resolution;
    ret->sn_resolution_half = src->sn_resolution_half;
    _zn_conduit_sn_list_copy(&ret->sn_rx_sns, &src->sn_rx_sns);

    ret->lease = src->lease;
    ret->lease_expires = src->lease_expires;
    ret->received = src->received;

    _z_bytes_copy(&ret->remote_pid, &src->remote_pid);
    _z_bytes_copy(&ret->remote_addr, &src->remote_addr);
    
    return ret;
}

int _zn_transport_peer_entry_cmp(const _zn_transport_peer_entry_t *left, const _zn_transport_peer_entry_t *right)
{
    if (left->remote_pid.len != right->remote_pid.len)
        return 0; // False

    if (memcmp(left->remote_pid.val, right->remote_pid.val, left->remote_pid.len) != 0)
        return 0; // False

    return 1; // True
}