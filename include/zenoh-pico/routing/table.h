/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http: *www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https: *www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_PICO_ROUTING_TABLE_H
#define ZENOH_PICO_ROUTING_TABLE_H

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/transport/transport.h"

typedef struct
{
    zn_reskey_t mapping;
    _zn_transport_t transport;
} _z_next_hop_t;

// @TODO: define size, clear, and copy functions for _z_routing_next_hop_t
_Z_ELEM_DEFINE(_z_next_hop, _z_next_hop_t, _zn_noop_size, _zn_noop_clear, _zn_noop_copy)
_Z_LIST_DEFINE(_z_next_hop, _z_next_hop_t)

typedef struct
{
    z_str_t key;
    _z_next_hop_list_t next;
} _z_routing_entry_t;

_Z_ELEM_DEFINE(_z_routing_entry, _z_routing_entry_t, _zn_noop_size, _zn_noop_clear, _zn_noop_copy)
_Z_LIST_DEFINE(_z_routing_entry, _z_routing_entry_t)

typedef struct
{
    _z_routing_entry_list_t entries;
} _z_routing_table_t;

#endif /* ZENOH_PICO_ROUTING_TABLE_H */