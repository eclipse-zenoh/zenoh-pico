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

#ifndef ZENOH_PICO_LINK_ENDPOINT_H
#define ZENOH_PICO_LINK_ENDPOINT_H

#include "zenoh-pico/collections/array.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Locator ------------------*/
#if Z_FEATURE_LINK_TCP == 1
#define TCP_SCHEMA "tcp"
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
#define UDP_SCHEMA "udp"
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
#define BT_SCHEMA "bt"
#endif
#if Z_FEATURE_LINK_SERIAL == 1
#define SERIAL_SCHEMA "serial"
#endif
#if Z_FEATURE_LINK_WS == 1
#define WS_SCHEMA "ws"
#endif

#define LOCATOR_PROTOCOL_SEPARATOR '/'
#define LOCATOR_METADATA_SEPARATOR '?'
typedef struct {
    _z_str_intmap_t _metadata;
    char *_protocol;
    char *_address;
} _z_locator_t;

_Bool _z_locator_eq(const _z_locator_t *left, const _z_locator_t *right);

void _z_locator_init(_z_locator_t *locator);
_z_string_t _z_locator_to_string(const _z_locator_t *loc);
int8_t _z_locator_from_str(_z_locator_t *lc, const char *s);

size_t _z_locator_size(_z_locator_t *lc);
void _z_locator_clear(_z_locator_t *lc);
_Z_ELEM_DEFINE(_z_locator, _z_locator_t, _z_locator_size, _z_locator_clear, _z_noop_copy)

/*------------------ Locator array ------------------*/
_Z_ARRAY_DEFINE(_z_locator, _z_locator_t)

/*------------------ Endpoint ------------------*/
#define ENDPOINT_CONFIG_SEPARATOR '#'

typedef struct {
    _z_locator_t _locator;
    _z_str_intmap_t _config;
} _z_endpoint_t;

char *_z_endpoint_to_str(const _z_endpoint_t *e);
int8_t _z_endpoint_from_str(_z_endpoint_t *ep, const char *s);
void _z_endpoint_clear(_z_endpoint_t *ep);
void _z_endpoint_free(_z_endpoint_t **ep);

#endif /* ZENOH_PICO_LINK_ENDPOINT_H */
