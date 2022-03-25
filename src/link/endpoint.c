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
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/endpoint.h"
#if Z_LINK_TCP == 1
#include "zenoh-pico/link/config/tcp.h"
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
#include "zenoh-pico/link/config/udp.h"
#endif
#if Z_LINK_BLUETOOTH == 1
#include "zenoh-pico/link/config/bt.h"
#endif

/*------------------ Locator ------------------*/
void _z_locator_init(_z_locator_t *locator)
{
    locator->protocol = NULL;
    locator->address = NULL;
    locator->metadata = _z_str_intmap_make();
}

void _z_locator_clear(_z_locator_t *lc)
{
    free((_z_str_t)lc->protocol);
    free((_z_str_t)lc->address);
    _z_str_intmap_clear(&lc->metadata);
}

void _z_locator_free(_z_locator_t **lc)
{
    _z_locator_t *ptr = *lc;
    _z_locator_clear(ptr);
    *lc = NULL;
}

void _z_locator_copy(_z_locator_t *dst, const _z_locator_t *src)
{
    dst->protocol = _z_str_clone(src->protocol);
    dst->address = _z_str_clone(src->address);

    // @TODO: implement copy for metadata
    dst->metadata = _z_str_intmap_make();
}

int _z_locator_eq(const _z_locator_t *left, const _z_locator_t *right)
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

_z_str_t _z_locator_protocol_from_str(const _z_str_t s)
{
    _z_str_t p_start = &s[0];
    if (p_start == NULL)
        goto ERR;

    _z_str_t p_end = strchr(s, LOCATOR_PROTOCOL_SEPARATOR);
    if (p_end == NULL)
        goto ERR;

    if (p_start == p_end)
        goto ERR;

    size_t p_len = p_end - p_start;
    _z_str_t protocol = (_z_str_t)malloc((p_len + 1) * sizeof(char));
    strncpy(protocol, p_start, p_len);
    protocol[p_len] = '\0';

    return protocol;

ERR:
    return NULL;
}

_z_str_t _z_locator_address_from_str(const _z_str_t s)
{
    _z_str_t p_start = strchr(s, LOCATOR_PROTOCOL_SEPARATOR);
    if (p_start == NULL)
        goto ERR;
    p_start++;

    _z_str_t p_end = strchr(s, LOCATOR_METADATA_SEPARATOR);
    if (p_end == NULL)
        p_end = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_end == NULL)
        p_end = &s[strlen(s)];

    if (p_start == p_end)
        goto ERR;

    size_t p_len = p_end - p_start;
    _z_str_t address = (_z_str_t)malloc((p_len + 1) * sizeof(char));
    strncpy(address, p_start, p_len);
    address[p_len] = '\0';

    return address;

ERR:
    return NULL;
}

_z_str_intmap_result_t _z_locator_metadata_from_str(const _z_str_t s)
{
    _z_str_intmap_result_t res;

    res.tag = _Z_RES_OK;
    res.value.str_intmap = _z_str_intmap_make();

    _z_str_t p_start = strchr(s, LOCATOR_METADATA_SEPARATOR);
    if (p_start == NULL)
        return res;
    p_start++;

    _z_str_t p_end = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_end == NULL)
        p_end = &s[strlen(s)];

    if (p_start == p_end)
        goto ERR;

    size_t p_len = p_end - p_start;

    // @TODO: define protocol-level metadata
    return _z_str_intmap_from_strn(p_start, 0, NULL, p_len);

ERR:
    res.tag = _Z_RES_ERR;
    res.value.error = _Z_ERR_PARSE_STRING;
    return res;
}

size_t _z_locator_metadata_strlen(const _z_str_intmap_t *s)
{
    // @TODO: define protocol-level metadata
    return _z_str_intmap_strlen(s, 0, NULL);
}

void _z_locator_metadata_onto_str(_z_str_t dst, const _z_str_intmap_t *s)
{
    // @TODO: define protocol-level metadata
    _z_str_intmap_onto_str(dst, s, 0, NULL);
}

_z_locator_result_t _z_locator_from_str(const _z_str_t s)
{
    _z_locator_result_t res;

    if (s == NULL)
        goto ERR;

    res.tag = _Z_RES_OK;
    _z_locator_init(&res.value.locator);

    // Parse protocol
    res.value.locator.protocol = _z_locator_protocol_from_str(s);
    if (res.value.locator.protocol == NULL)
        goto ERR;

    // Parse address
    res.value.locator.address = _z_locator_address_from_str(s);
    if (res.value.locator.address == NULL)
        goto ERR;

    // Parse metadata
    _z_str_intmap_result_t tmp_res;
    tmp_res = _z_locator_metadata_from_str(s);
    if (tmp_res.tag == _Z_RES_ERR)
        goto ERR;
    res.value.locator.metadata = tmp_res.value.str_intmap;

    return res;

ERR:
    _z_locator_clear(&res.value.locator);
    res.tag = _Z_RES_ERR;
    res.value.error = _Z_ERR_PARSE_STRING;
    return res;
}

size_t _z_locator_strlen(const _z_locator_t *l)
{
    if (l == NULL)
        goto ERR;

    // Calculate the string length to allocate
    size_t len = 0;

    len += strlen(l->protocol); // Locator protocol
    len += 1;                   // Locator protocol separator
    len += strlen(l->address);  // Locator address

    // @TODO: define protocol-level metadata
    size_t md_len = _z_locator_metadata_strlen(&l->metadata);
    if (md_len > 0)
    {
        len += 1;      // Locator metadata separator
        len += md_len; // Locator medatada content
    }

    return len;

ERR:
    return 0;
}

void _z_locator_onto_str(_z_str_t dst, const _z_locator_t *l)
{
    dst[0] = '\0';

    const char psep = LOCATOR_PROTOCOL_SEPARATOR;
    const char msep = LOCATOR_METADATA_SEPARATOR;

    strcat(dst, l->protocol); // Locator protocol
    strncat(dst, &psep, 1);   // Locator protocol separator
    strcat(dst, l->address);  // Locator address

    // @TODO: define protocol-level metadata
    size_t md_len = _z_locator_metadata_strlen(&l->metadata);
    if (md_len > 0)
    {
        strncat(dst, &msep, 1); // Locator metadata separator
        _z_locator_metadata_onto_str(&dst[strlen(dst)], &l->metadata);
    }
}

_z_str_t _z_locator_to_str(const _z_locator_t *l)
{
    size_t len = _z_locator_strlen(l);
    _z_str_t dst = (_z_str_t)malloc(len + 1);
    _z_locator_onto_str(dst, l);
    return dst;
}

/*------------------ Endpoint ------------------*/
void _z_endpoint_init(_z_endpoint_t *endpoint)
{
    _z_locator_init(&endpoint->locator);
    endpoint->config = _z_str_intmap_make();
}

void _z_endpoint_clear(_z_endpoint_t *ep)
{
    _z_locator_clear(&ep->locator);
    _z_str_intmap_clear(&ep->config);
}

void _z_endpoint_free(_z_endpoint_t **ep)
{
    _z_endpoint_t *ptr = *ep;
    _z_locator_clear(&ptr->locator);
    _z_str_intmap_clear(&ptr->config);
    free(ptr);
    *ep = NULL;
}

_z_str_intmap_result_t _z_endpoint_config_from_str(const _z_str_t s, const _z_str_t proto)
{
    _z_str_intmap_result_t res;

    res.tag = _Z_RES_OK;
    res.value.str_intmap = _z_str_intmap_make();

    _z_str_t p_start = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_start == NULL)
        return res;
    p_start++;

    // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA))
        res = _z_tcp_config_from_str(p_start);
    else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
    if (_z_str_eq(proto, UDP_SCHEMA))
        res = _z_udp_config_from_str(p_start);
    else
#endif
#if Z_LINK_BLUETOOTH == 1
    if (_z_str_eq(proto, BT_SCHEMA))
        res = _z_bt_config_from_str(p_start);
    else
#endif
        goto ERR;

    return res;

ERR:
    res.tag = _Z_RES_ERR;
    res.value.error = _Z_ERR_PARSE_STRING;
    return res;
}

size_t _z_endpoint_config_strlen(const _z_str_intmap_t *s, const _z_str_t proto)
{
    size_t len;

    // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA))
        len = _z_tcp_config_strlen(s);
    else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
    if (_z_str_eq(proto, UDP_SCHEMA))
        len = _z_udp_config_strlen(s);
    else
#endif
#if Z_LINK_BLUETOOTH == 1
    if (_z_str_eq(proto, BT_SCHEMA))
        len = _z_bt_config_strlen(s);
    else
#endif
        goto ERR;

    return len;

ERR:
    len = 0;
    return len;
}

_z_str_t _z_endpoint_config_to_str(const _z_str_intmap_t *s, const _z_str_t proto)
{
    _z_str_t res;

    // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA))
        res = _z_tcp_config_to_str(s);
    else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
    if (_z_str_eq(proto, UDP_SCHEMA))
        res = _z_udp_config_to_str(s);
    else
#endif
#if Z_LINK_BLUETOOTH == 1
    if (_z_str_eq(proto, BT_SCHEMA))
        res = _z_bt_config_to_str(s);
    else
#endif
        goto ERR;

    return res;

ERR:
    res = NULL;
    return res;
}

_z_endpoint_result_t _z_endpoint_from_str(const _z_str_t s)
{
    _z_endpoint_result_t res;

    res.tag = _Z_RES_OK;
    _z_endpoint_init(&res.value.endpoint);

    _z_locator_result_t loc_res = _z_locator_from_str(s);
    if (loc_res.tag == _Z_RES_ERR)
        goto ERR;
    res.value.endpoint.locator = loc_res.value.locator;

    _z_str_intmap_result_t conf_res = _z_endpoint_config_from_str(s, res.value.endpoint.locator.protocol);
    if (conf_res.tag == _Z_RES_ERR)
        goto ERR;
    res.value.endpoint.config = conf_res.value.str_intmap;

    return res;

ERR:
    _z_endpoint_clear(&res.value.endpoint);
    res.tag = _Z_RES_ERR;
    res.value.error = _Z_ERR_PARSE_STRING;
    return res;
}

_z_str_t _z_endpoint_to_str(const _z_endpoint_t *endpoint)
{
    _z_str_t res;

    _z_str_t locator = _z_locator_to_str(&endpoint->locator);
    if (locator == NULL)
        goto ERR;

    size_t len = strlen(locator);

    _z_str_t config = _z_endpoint_config_to_str(&endpoint->config, endpoint->locator.protocol);
    if (config != NULL)
    {
        len += 1;              // Config separator
        len += strlen(config); // Config content
    }

    _z_str_t s = (_z_str_t)malloc(len + 1);

    strcat(s, locator);
ERR:
    res = NULL;
    return res;
}