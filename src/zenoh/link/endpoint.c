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

/*------------------ Locator ------------------*/
void __zn_locator_init(_zn_locator_t *locator)
{
    locator->protocol = NULL;
    locator->address = NULL;
    locator->metadata = zn_properties_make();
}

void _zn_locator_clear(_zn_locator_t *lc)
{
    free((z_str_t)lc->protocol);
    free((z_str_t)lc->address);
    zn_properties_clear(&lc->metadata);
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

zn_properties_result_t _zn_locator_metadata_from_str(const z_str_t s)
{
    zn_properties_result_t res;

    res.tag = _z_res_t_OK;
    res.value.properties = zn_properties_make();

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
    res = zn_properties_from_str(metadata, 0, NULL);
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
    zn_properties_result_t tmp_res;
    tmp_res = _zn_locator_metadata_from_str(s);
    if (tmp_res.tag == _z_res_t_ERR)
        goto ERR;
    res.value.locator.metadata = tmp_res.value.properties;

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
    endpoint->config = zn_properties_make();
}

// @TODO: remove
z_str_t _zn_endpoint_property_from_key(const z_str_t str, const z_str_t key)
{
    return NULL;
}

void _zn_endpoint_clear(_zn_endpoint_t *ep)
{
    _zn_locator_clear(&ep->locator);
    zn_properties_clear(&ep->config);
}

void _zn_endpoint_free(_zn_endpoint_t **ep)
{
    _zn_endpoint_t *ptr = *ep;
    _zn_locator_clear(&ptr->locator);
    zn_properties_clear(&ptr->config);
    free(ptr);
    *ep = NULL;
}

zn_properties_result_t _zn_endpoint_config_from_str(const z_str_t s, const z_str_t proto)
{
    zn_properties_result_t res;

    res.tag = _z_res_t_OK;
    res.value.properties = zn_properties_make();

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

    zn_properties_result_t conf_res = _zn_endpoint_config_from_str(s, res.value.endpoint.locator.protocol);
    if (conf_res.tag == _z_res_t_ERR)
        goto ERR;
    res.value.endpoint.config = conf_res.value.properties;

    return res;

ERR:
    _zn_endpoint_clear(&res.value.endpoint);
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}