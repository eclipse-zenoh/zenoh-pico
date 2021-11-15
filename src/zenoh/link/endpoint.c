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

#include <stdio.h>
#include <string.h>

#include "zenoh-pico/link/config/tcp.h"
#include "zenoh-pico/link/config/udp.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/utils/collections.h"

/*------------------ State ------------------*/
_zn_state_t _zn_state_make()
{
    return z_i_map_make(_Z_DEFAULT_I_MAP_CAPACITY);
}

int _zn_state_init(_zn_state_t *ps)
{
    return z_i_map_start(ps, _Z_DEFAULT_I_MAP_CAPACITY);
}

int _zn_state_insert(_zn_state_t *ps, unsigned int key, z_str_t value)
{
    return z_i_map_set(ps, key, value);
}

const z_str_t _zn_state_get(const _zn_state_t *ps, unsigned int key)
{
    return (const z_str_t)z_i_map_get(ps, key);
}

size_t _zn_state_len(const _zn_state_t *ps)
{
    return z_i_map_len(ps);
}

int _zn_state_is_empty(const _zn_state_t *ps)
{
    return z_i_map_is_empty(ps);
}

void _zn_state_clear(_zn_state_t *ps)
{
    z_i_map_clear(ps);
}

void _zn_state_free(_zn_state_t **ps)
{
    z_i_map_free(ps);
}

_zn_state_result_t _zn_state_from_str(const z_str_t s, unsigned int argc, _zn_state_mapping_t argv[])
{
    _zn_state_result_t res;

    res.tag = _z_res_t_OK;
    res.value.state = _zn_state_make();

    // Check the string contains only the right
    z_str_t start = s;
    z_str_t end = &s[strlen(s)];
    while (start < end)
    {
        z_str_t p_key_start = start;
        z_str_t p_key_end = strchr(s, ENDPOINT_STATE_KEYVALUE_SEPARATOR);

        if (p_key_end == NULL)
            goto ERR;

        // Verify the key is valid based on the provided mapping
        size_t p_key_len = p_key_end - p_key_start;
        int found = 0;
        unsigned int key;
        for (unsigned int i = 0; i < argc; i++)
        {
            if (p_key_len != strlen(argv[i].str))
                continue;
            if (strncmp(p_key_start, argv[i].str, p_key_len) != 0)
                continue;

            found = 1;
            key = argv[i].key;
            break;
        }

        if (!found)
            goto ERR;

        // Read and populate the value
        z_str_t p_value_start = p_key_end + 1;
        z_str_t p_value_end = strchr(p_key_end, ENDPOINT_STATE_LIST_SEPARATOR);
        if (p_value_end == NULL)
            p_value_end = end;

        size_t p_value_len = p_value_end - p_value_start;
        z_str_t p_value = (z_str_t)malloc((p_value_len + 1) * sizeof(char));
        strncpy(p_value, p_value_start, p_value_len);
        p_value[p_value_len] = '\0';

        _zn_state_insert(&res.value.state, key, p_value);

        // Process next key value
        start = p_value_end + 1;
    }

    return res;

ERR:
    _zn_state_clear(&res.value.state);
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

/*------------------ Locator ------------------*/
void __zn_locator_init(_zn_locator_t *locator)
{
    locator->protocol = NULL;
    locator->address = NULL;
    locator->metadata = _zn_state_make();
}

void _zn_locator_clear(_zn_locator_t *lc)
{
    free((z_str_t)lc->protocol);
    free((z_str_t)lc->address);
    _zn_state_clear(&lc->metadata);
}

void _zn_locator_free(_zn_locator_t **lc)
{
    _zn_locator_t *ptr = *lc;
    _zn_locator_clear(ptr);
    *lc = NULL;
}

z_str_t _zn_locator_protocol_from_str(const z_str_t s)
{
    z_str_t p_start = &s[0];
    if (p_start == NULL)
        goto ERR;

    z_str_t p_end = strchr(s, ENDPOINT_PROTO_SEPARATOR);
    if (p_end == NULL)
        goto ERR;

    if (p_start == p_end)
        goto ERR;

    size_t p_len = p_end - p_start;
    z_str_t protocol = (z_str_t)malloc((p_len + 1) * sizeof(char));
    strncpy(protocol, p_start, p_len);
    protocol[p_len] = '\0';

    return protocol;

ERR:
    return NULL;
}

z_str_t _zn_locator_address_from_str(const z_str_t s)
{
    z_str_t p_start = strchr(s, ENDPOINT_PROTO_SEPARATOR);
    if (p_start == NULL)
        goto ERR;
    p_start++;

    z_str_t p_end = strchr(s, ENDPOINT_METADATA_SEPARATOR);
    if (p_end == NULL)
        p_end = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_end == NULL)
        p_end = &s[strlen(s)];

    if (p_start == p_end)
        goto ERR;

    size_t p_len = p_end - p_start;
    z_str_t address = (z_str_t)malloc((p_len + 1) * sizeof(char));
    strncpy(address, p_start, p_len);
    address[p_len] = '\0';

    return address;

ERR:
    return NULL;
}

_zn_state_result_t _zn_locator_metadata_from_str(const z_str_t s)
{
    _zn_state_result_t res;

    res.tag = _z_res_t_OK;
    res.value.state = _zn_state_make();

    z_str_t p_start = strchr(s, ENDPOINT_METADATA_SEPARATOR);
    if (p_start == NULL)
        return res;
    p_start++;

    z_str_t p_end = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_end == NULL)
        p_end = &s[strlen(s)];

    if (p_start == p_end)
        goto ERR;

    size_t p_len = p_end - p_start;
    z_str_t metadata = (z_str_t)malloc((p_len + 1) * sizeof(char));
    strncpy(metadata, p_start, p_len);
    metadata[p_len] = '\0';

    // @TODO: define protocol-level metadata
    res = _zn_state_from_str(metadata, 0, NULL);
    free(metadata);

    return res;

ERR:
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

_zn_locator_result_t _zn_locator_from_str(const z_str_t s)
{
    _zn_locator_result_t res;

    if (s == NULL)
        goto ERR;

    res.tag = _z_res_t_OK;
    __zn_locator_init(&res.value.locator);

    // Parse protocol
    res.value.locator.protocol = _zn_locator_protocol_from_str(s);
    if (res.value.locator.protocol == NULL)
        goto ERR;

    // Parse address
    res.value.locator.address = _zn_locator_address_from_str(s);
    if (res.value.locator.address == NULL)
        goto ERR;

    // Parse metadata
    _zn_state_result_t tmp_res;
    tmp_res = _zn_locator_metadata_from_str(s);
    if (tmp_res.tag == _z_res_t_ERR)
        goto ERR;
    res.value.locator.metadata = tmp_res.value.state;

    return res;

ERR:
    _zn_locator_clear(&res.value.locator);
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

/*------------------ Endpoint ------------------*/
void __zn_endpoint_init(_zn_endpoint_t *endpoint)
{
    __zn_locator_init(&endpoint->locator);
    endpoint->config = _zn_state_make();
}

void _zn_endpoint_clear(_zn_endpoint_t *ep)
{
    _zn_locator_clear(&ep->locator);
    _zn_state_clear(&ep->config);
}

void _zn_endpoint_free(_zn_endpoint_t **ep)
{
    _zn_endpoint_t *ptr = *ep;
    _zn_locator_clear(&ptr->locator);
    _zn_state_clear(&ptr->config);
    free(ptr);
    *ep = NULL;
}

_zn_state_result_t _zn_endpoint_config_from_str(const z_str_t s, const z_str_t proto)
{
    _zn_state_result_t res;

    res.tag = _z_res_t_OK;
    res.value.state = _zn_state_make();

    z_str_t p_start = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_start == NULL)
        return res;
    p_start++;

    // Call the right configuration parser depending on the protocol
    if (strcmp(proto, TCP_SCHEMA) == 0)
    {
        res = _zn_tcp_config_from_str(p_start);
    }
    else if (strcmp(proto, UDP_SCHEMA) == 0)
    {
        res = _zn_udp_config_from_str(p_start);
    }
    else
    {
        goto ERR;
    }

    return res;

ERR:
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

_zn_endpoint_result_t _zn_endpoint_from_str(const z_str_t s)
{
    _zn_endpoint_result_t res;

    res.tag = _z_res_t_OK;
    __zn_endpoint_init(&res.value.endpoint);

    _zn_locator_result_t loc_res = _zn_locator_from_str(s);
    if (loc_res.tag == _z_res_t_ERR)
        goto ERR;
    res.value.endpoint.locator = loc_res.value.locator;

    _zn_state_result_t conf_res = _zn_endpoint_config_from_str(s, res.value.endpoint.locator.protocol);
    if (conf_res.tag == _z_res_t_ERR)
        goto ERR;
    res.value.endpoint.config = conf_res.value.state;

    return res;

ERR:
    _zn_endpoint_clear(&res.value.endpoint);
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}