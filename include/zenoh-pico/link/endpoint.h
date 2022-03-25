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

#ifndef ZENOH_PICO_LINK_ENDPOINT_H
#define ZENOH_PICO_LINK_ENDPOINT_H

#include "zenoh-pico/collections/array.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Locator ------------------*/
#if Z_LINK_TCP == 1
#define TCP_SCHEMA "tcp"
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
#define UDP_SCHEMA "udp"
#endif
#if Z_LINK_BLUETOOTH == 1
#define BT_SCHEMA "bt"
#endif

#define LOCATOR_PROTOCOL_SEPARATOR '/'
#define LOCATOR_METADATA_SEPARATOR '?'
typedef struct
{
    _z_str_t protocol;
    _z_str_t address;
    _z_str_intmap_t metadata;
} _z_locator_t;

_Z_RESULT_DECLARE(_z_locator_t, locator)

int _z_locator_eq(const _z_locator_t *left, const _z_locator_t *right);

_z_str_t _z_locator_to_str(const _z_locator_t *l);
_z_locator_result_t _z_locator_from_str(const _z_str_t s);

size_t _z_locator_size(_z_locator_t *lc);
void _z_locator_clear(_z_locator_t *lc);
_Z_ELEM_DEFINE(_z_locator, _z_locator_t, _z_locator_size, _z_locator_clear, _z_noop_copy)

/*------------------ Locator array ------------------*/
_Z_ARRAY_DEFINE(_z_locator, _z_locator_t)
_Z_RESULT_DECLARE(_z_locator_array_t, locator_array)

/*------------------ Endpoint ------------------*/
#define ENDPOINT_CONFIG_SEPARATOR '#'

typedef struct
{
    _z_locator_t locator;
    _z_str_intmap_t config;
} _z_endpoint_t;
_Z_RESULT_DECLARE(_z_endpoint_t, endpoint)

_z_str_t _z_endpoint_to_str(const _z_endpoint_t *e);
_z_endpoint_result_t _z_endpoint_from_str(const _z_str_t s);
void _z_endpoint_clear(_z_endpoint_t *ep);
void _z_endpoint_free(_z_endpoint_t **ep);

#endif /* ZENOH_PICO_LINK_ENDPOINT_H */
