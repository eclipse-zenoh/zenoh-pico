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

#include "zenoh-pico/link/types.h"

const char *_zn_get_protocol_segment(const char *locator)
{
    const char *p_init = &locator[0];
    if (p_init == NULL)
        return NULL;

    const char *p_end = strpbrk(locator, ENDPOINT_PROTO_SEPARATOR);
    if (p_end == NULL)
        return NULL;

    int len = p_end - p_init;
    char *segment = (char*)malloc((len + 1) * sizeof(char));
    strncpy(segment, p_init, len);
    segment[len] = '\0';

    return segment;
}

const char *_zn_get_address_segment(const char *locator)
{
    const char *p_init = strpbrk(locator, ENDPOINT_PROTO_SEPARATOR);
    if (p_init == NULL)
        return NULL;
    ++p_init;

    const char *p_end = strpbrk(locator, ENDPOINT_METADATA_SEPARATOR);
    if (p_end == NULL)
        p_end = strpbrk(locator, ENDPOINT_CONFIG_SEPARATOR);
        if (p_end == NULL)
            p_end = &locator[strlen(locator)];

    int len = p_end - p_init;
    char *segment = (char*)malloc((len + 1) * sizeof(char));
    strncpy(segment, p_init, len);
    segment[len] = '\0';

    return segment;
}

const char *_zn_get_metadata_segment(const char *locator)
{
    const char *p_init = strpbrk(locator, ENDPOINT_METADATA_SEPARATOR);
    if (p_init == NULL)
        return NULL;
    ++p_init;

    const char *p_end = strpbrk(locator, ENDPOINT_CONFIG_SEPARATOR);
    if (p_end == NULL)
        p_end = &locator[strlen(locator)];

    int len = p_end - p_init;
    char *segment = (char*)malloc((len + 1) * sizeof(char));
    strncpy(segment, p_init, len);
    segment[len] = '\0';

    return segment;
}

const char *_zn_get_config_segment(const char *locator)
{
    const char *p_init = strpbrk(locator, ENDPOINT_CONFIG_SEPARATOR);
    if (p_init == NULL)
        return NULL;
    ++p_init;

    const char *p_end = &locator[strlen(locator)];

    int len = p_end - p_init;
    char *segment = (char*)malloc((len + 1) * sizeof(char));
    strncpy(segment, p_init, len);
    segment[len] = '\0';

    return segment;
}

_zn_endpoint_t *_zn_endpoint_from_string(const char *s_locator)
{
    _zn_endpoint_t *zn = (_zn_endpoint_t *)malloc(sizeof(_zn_endpoint_t));

    zn->protocol = _zn_get_protocol_segment(s_locator);
    zn->address = _zn_get_address_segment(s_locator);
    zn->metadata = _zn_get_metadata_segment(s_locator);
    zn->config = _zn_get_config_segment(s_locator);

    return zn;
}

void _zn_endpoint_free(_zn_endpoint_t **zn)
{
    _zn_endpoint_t *ptr = *zn;

    free((char*)ptr->protocol);
    free((char*)ptr->address);
    free((char*)ptr->metadata);
    free((char*)ptr->config);

    *zn = NULL;
}
