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

#include <stdint.h>
#include "zenoh-pico/collections/array.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Locator ------------------*/
#define TCP_SCHEMA "tcp"
#define UDP_SCHEMA "udp"

#define LOCATOR_PROTOCOL_SEPARATOR '/'
#define LOCATOR_METADATA_SEPARATOR '?'
typedef struct
{
    z_str_t protocol;
    z_str_t address;
    _z_str_intmap_t metadata;
} _zn_locator_t;

_ZN_RESULT_DECLARE(_zn_locator_t, locator)

int _zn_locator_eq(const _zn_locator_t *left, const _zn_locator_t *right);

z_str_t _zn_locator_to_str(const _zn_locator_t *l);
_zn_locator_result_t _zn_locator_from_str(const z_str_t s);

size_t _zn_locator_size(_zn_locator_t *lc);
void _zn_locator_clear(_zn_locator_t *lc);
_Z_ELEM_DEFINE(_zn_locator, _zn_locator_t, _zn_locator_size, _zn_locator_clear, _zn_noop_copy)

/*------------------ Locator array ------------------*/
_Z_ARRAY_DEFINE(_zn_locator, _zn_locator_t)
_ZN_RESULT_DECLARE(_zn_locator_array_t, locator_array)

/*------------------ Endpoint ------------------*/
#define ENDPOINT_CONFIG_SEPARATOR '#'

typedef struct
{
    _zn_locator_t locator;
    _z_str_intmap_t config;
} _zn_endpoint_t;
_ZN_RESULT_DECLARE(_zn_endpoint_t, endpoint)

z_str_t _zn_endpoint_to_str(const _zn_endpoint_t *e);
_zn_endpoint_result_t _zn_endpoint_from_str(const z_str_t s);
void _zn_endpoint_clear(_zn_endpoint_t *ep);
void _zn_endpoint_free(_zn_endpoint_t **ep);

#endif /* ZENOH_PICO_LINK_ENDPOINT_H */
