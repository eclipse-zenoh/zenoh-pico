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

#include <string.h>
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/link/config/tcp.h"
#include "zenoh-pico/link/config/udp.h"

/*------------------ Locator ------------------*/
void _zn_locator_init(_zn_locator_t *locator)
{
    locator->protocol = NULL;
    locator->address = NULL;
    locator->metadata = _z_str_intmap_make();
}

void _zn_locator_clear(_zn_locator_t *lc)
{
    free((z_str_t)lc->protocol);
    free((z_str_t)lc->address);
    _z_str_intmap_clear(&lc->metadata);
}

void _zn_locator_free(_zn_locator_t **lc)
{
    _zn_locator_t *ptr = *lc;
    _zn_locator_clear(ptr);
    *lc = NULL;
}

void _zn_locator_copy(_zn_locator_t *dst, const _zn_locator_t *src)
{
    dst->protocol = _z_str_clone(src->protocol);
    dst->address = _z_str_clone(src->address);

    // @TODO: implement copy for metadata
    dst->metadata = _z_str_intmap_make();
}

int _zn_locator_eq(const _zn_locator_t *left, const _zn_locator_t *right)
{
    int res = 0;

    res = _z_str_eq(left->protocol, right->protocol);
    if (!res)
        return res;

    res = _z_str_eq(left->address, right->address);
    if (!res)
        return res;

    // @TODO: implement eq for metadata

    return res;
}

z_str_t _zn_locator_protocol_from_str(const z_str_t s)
{
    z_str_t p_start = &s[0];
    if (p_start == NULL)
        goto ERR;

    z_str_t p_end = strchr(s, LOCATOR_PROTOCOL_SEPARATOR);
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
    z_str_t p_start = strchr(s, LOCATOR_PROTOCOL_SEPARATOR);
    if (p_start == NULL)
        goto ERR;
    p_start++;

    z_str_t p_end = strchr(s, LOCATOR_METADATA_SEPARATOR);
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

_z_str_intmap_result_t _zn_locator_metadata_from_str(const z_str_t s)
{
    _z_str_intmap_result_t res;

    res.tag = _z_res_t_OK;
    res.value.str_intmap = _z_str_intmap_make();

    z_str_t p_start = strchr(s, LOCATOR_METADATA_SEPARATOR);
    if (p_start == NULL)
        return res;
    p_start++;

    z_str_t p_end = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_end == NULL)
        p_end = &s[strlen(s)];

    if (p_start == p_end)
        goto ERR;

    size_t p_len = p_end - p_start;

    // @TODO: define protocol-level metadata
    return _z_str_intmap_from_strn(p_start, 0, NULL, p_len);

ERR:
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

size_t _zn_locator_metadata_strlen(const _z_str_intmap_t *s)
{
    // @TODO: define protocol-level metadata
    return _z_str_intmap_strlen(s, 0, NULL);
}

void _zn_locator_metadata_onto_str(z_str_t dst, const _z_str_intmap_t *s)
{
    // @TODO: define protocol-level metadata
    _z_str_intmap_onto_str(dst, s, 0, NULL);
}

_zn_locator_result_t _zn_locator_from_str(const z_str_t s)
{
    _zn_locator_result_t res;

    if (s == NULL)
        goto ERR;

    res.tag = _z_res_t_OK;
    _zn_locator_init(&res.value.locator);

    // Parse protocol
    res.value.locator.protocol = _zn_locator_protocol_from_str(s);
    if (res.value.locator.protocol == NULL)
        goto ERR;

    // Parse address
    res.value.locator.address = _zn_locator_address_from_str(s);
    if (res.value.locator.address == NULL)
        goto ERR;

    // Parse metadata
    _z_str_intmap_result_t tmp_res;
    tmp_res = _zn_locator_metadata_from_str(s);
    if (tmp_res.tag == _z_res_t_ERR)
        goto ERR;
    res.value.locator.metadata = tmp_res.value.str_intmap;

    return res;

ERR:
    _zn_locator_clear(&res.value.locator);
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

size_t _zn_locator_strlen(const _zn_locator_t *l)
{
    if (l == NULL)
        goto ERR;

    // Calculate the string length to allocate
    size_t len = 0;

    len += strlen(l->protocol); // Locator protocol
    len += 1;                   // Locator protocol separator
    len += strlen(l->address);  // Locator address

    // @TODO: define protocol-level metadata
    size_t md_len = _zn_locator_metadata_strlen(&l->metadata);
    if (md_len > 0)
    {
        len += 1;      // Locator metadata separator
        len += md_len; // Locator medatada content
    }

    return len;

ERR:
    return 0;
}

void _zn_locator_onto_str(z_str_t dst, const _zn_locator_t *l)
{
    dst[0] = '\0';

    const char psep = LOCATOR_PROTOCOL_SEPARATOR;
    const char msep = LOCATOR_METADATA_SEPARATOR;

    strcat(dst, l->protocol); // Locator protocol
    strncat(dst, &psep, 1);   // Locator protocol separator
    strcat(dst, l->address);  // Locator address

    // @TODO: define protocol-level metadata
    size_t md_len = _zn_locator_metadata_strlen(&l->metadata);
    if (md_len > 0)
    {
        strncat(dst, &msep, 1); // Locator metadata separator
        _zn_locator_metadata_onto_str(&dst[strlen(dst)], &l->metadata);
    }
}

z_str_t _zn_locator_to_str(const _zn_locator_t *l)
{
    size_t len = _zn_locator_strlen(l);
    z_str_t dst = (z_str_t)malloc(len + 1);
    _zn_locator_onto_str(dst, l);
    return dst;
}

/*------------------ Endpoint ------------------*/
void __zn_endpoint_init(_zn_endpoint_t *endpoint)
{
    _zn_locator_init(&endpoint->locator);
    endpoint->config = _z_str_intmap_make();
}

void _zn_endpoint_clear(_zn_endpoint_t *ep)
{
    _zn_locator_clear(&ep->locator);
    _z_str_intmap_clear(&ep->config);
}

void _zn_endpoint_free(_zn_endpoint_t **ep)
{
    _zn_endpoint_t *ptr = *ep;
    _zn_locator_clear(&ptr->locator);
    _z_str_intmap_clear(&ptr->config);
    free(ptr);
    *ep = NULL;
}

_z_str_intmap_result_t _zn_endpoint_config_from_str(const z_str_t s, const z_str_t proto)
{
    _z_str_intmap_result_t res;

    res.tag = _z_res_t_OK;
    res.value.str_intmap = _z_str_intmap_make();

    z_str_t p_start = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_start == NULL)
        return res;
    p_start++;

    // Call the right configuration parser depending on the protocol
    if (_z_str_eq(proto, TCP_SCHEMA))
        res = _zn_tcp_config_from_str(p_start);
    else if (_z_str_eq(proto, UDP_SCHEMA))
        res = _zn_udp_config_from_str(p_start);
    else
        goto ERR;

    return res;

ERR:
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

size_t _zn_endpoint_config_strlen(const _z_str_intmap_t *s, const z_str_t proto)
{
    size_t len;

    // Call the right configuration parser depending on the protocol
    if (_z_str_eq(proto, TCP_SCHEMA))
        len = _zn_tcp_config_strlen(s);
    else if (_z_str_eq(proto, UDP_SCHEMA))
        len = _zn_udp_config_strlen(s);
    else
        goto ERR;

    return len;

ERR:
    len = 0;
    return len;
}

z_str_t _zn_endpoint_config_to_str(const _z_str_intmap_t *s, const z_str_t proto)
{
    z_str_t res;

    // Call the right configuration parser depending on the protocol
    if (_z_str_eq(proto, TCP_SCHEMA))
        res = _zn_tcp_config_to_str(s);
    else if (_z_str_eq(proto, UDP_SCHEMA))
        res = _zn_udp_config_to_str(s);
    else
        goto ERR;

    return res;

ERR:
    res = NULL;
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

    _z_str_intmap_result_t conf_res = _zn_endpoint_config_from_str(s, res.value.endpoint.locator.protocol);
    if (conf_res.tag == _z_res_t_ERR)
        goto ERR;
    res.value.endpoint.config = conf_res.value.str_intmap;

    return res;

ERR:
    _zn_endpoint_clear(&res.value.endpoint);
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}

z_str_t _zn_endpoint_to_str(const _zn_endpoint_t *endpoint)
{
    z_str_t res;

    z_str_t locator = _zn_locator_to_str(&endpoint->locator);
    if (locator == NULL)
        goto ERR;

    size_t len = strlen(locator);

    z_str_t config = _zn_endpoint_config_to_str(&endpoint->config, endpoint->locator.protocol);
    if (config != NULL)
    {
        len += 1;              // Config separator
        len += strlen(config); // Config content
    }

    z_str_t s = (z_str_t)malloc(len + 1);

    strcat(s, locator);
ERR:
    res = NULL;
    return res;
}