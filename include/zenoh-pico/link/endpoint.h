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
#include "zenoh-pico/utils/array.h"
#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/utils/result.h"

/*------------------ State  ------------------*/
#define ENDPOINT_STATE_KEYVALUE_SEPARATOR '='
#define ENDPOINT_STATE_LIST_SEPARATOR ';'

typedef z_i_map_t _zn_state_t;

typedef struct
{
    unsigned int key;
    z_str_t str;
} _zn_state_mapping_t;

_ZN_RESULT_DECLARE(_zn_state_t, state)

_zn_state_t _zn_state_make(void);
int _zn_state_init(_zn_state_t *ps);

int _zn_state_set(_zn_state_t *ps, unsigned int key, z_str_t value);
const z_str_t _zn_state_get(const _zn_state_t *ps, unsigned int key);
z_str_t _zn_state_del(_zn_state_t *ps, unsigned int key);

size_t _zn_state_len(const _zn_state_t *ps);
int _zn_state_is_empty(const _zn_state_t *ps);
void _zn_state_clear(_zn_state_t *ps);
void _zn_state_free(_zn_state_t **ps);

size_t _zn_state_strlen(const _zn_state_t *s, unsigned int argc, _zn_state_mapping_t argv[]);
void _zn_state_onto_str(z_str_t dst, const _zn_state_t *s, unsigned int argc, _zn_state_mapping_t argv[]);
z_str_t _zn_state_to_str(const _zn_state_t *s, unsigned int argc, _zn_state_mapping_t argv[]);
_zn_state_result_t _zn_state_from_str(const z_str_t s, unsigned int argc, _zn_state_mapping_t argv[]);
_zn_state_result_t _zn_state_from_strn(const z_str_t s, unsigned int argc, _zn_state_mapping_t argv[], size_t n);

/*------------------ Locator ------------------*/
#define TCP_SCHEMA "tcp"
#define UDP_SCHEMA "udp"

#define LOCATOR_PROTOCOL_SEPARATOR '/'
#define LOCATOR_METADATA_SEPARATOR '?'
typedef struct
{
    z_str_t protocol;
    z_str_t address;
    _zn_state_t metadata;
} _zn_locator_t;

_ZN_RESULT_DECLARE(_zn_locator_t, locator)

int _zn_locator_cmp(const _zn_locator_t *left, const _zn_locator_t *right);

z_str_t _zn_locator_to_str(const _zn_locator_t *l);
_zn_locator_result_t _zn_locator_from_str(const z_str_t s);

void _zn_locator_clear(_zn_locator_t *lc);
void _zn_locator_free(_zn_locator_t **lc);

/*------------------ Locator array ------------------*/
_ARRAY_DECLARE(_zn_locator_t, locator, _zn_)
_ZN_RESULT_DECLARE(_zn_locator_array_t, locator_array)

void _zn_locator_array_init(_zn_locator_array_t *la, size_t len);
_zn_locator_array_t _zn_locator_array_make(size_t len);
void _zn_locator_array_clear(_zn_locator_array_t *la);
void _zn_locator_array_free(_zn_locator_array_t **la);
void _zn_locator_array_move(_zn_locator_array_t *dst, _zn_locator_array_t *src);
void _zn_locator_array_copy(_zn_locator_array_t *dst, const _zn_locator_array_t *src);

/*------------------ Endpoint ------------------*/
#define ENDPOINT_CONFIG_SEPARATOR '#'

typedef struct
{
    _zn_locator_t locator;
    _zn_state_t config;
} _zn_endpoint_t;
_ZN_RESULT_DECLARE(_zn_endpoint_t, endpoint)

z_str_t _zn_endpoint_to_str(const _zn_endpoint_t *e);
_zn_endpoint_result_t _zn_endpoint_from_str(const z_str_t s);
void _zn_endpoint_clear(_zn_endpoint_t *ep);
void _zn_endpoint_free(_zn_endpoint_t **ep);

#endif /* ZENOH_PICO_LINK_ENDPOINT_H */
