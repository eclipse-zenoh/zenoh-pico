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
#include "zenoh-pico/link/config/tcp.h"

size_t _zn_tcp_config_strlen(const _z_str_intmap_t *s)
{
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_strlen(s, argc, args);
}

void _zn_tcp_config_onto_str(z_str_t dst, const _z_str_intmap_t *s)
{
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_onto_str(dst, s, argc, args);
}

z_str_t _zn_tcp_config_to_str(const _z_str_intmap_t *s)
{
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_to_str(s, argc, args);
}

_z_str_intmap_result_t _zn_tcp_config_from_strn(const z_str_t s, size_t n)
{
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_from_strn(s, argc, args, n);
}

_z_str_intmap_result_t _zn_tcp_config_from_str(const z_str_t s)
{
    TCP_CONFIG_MAPPING_BUILD

    return _zn_tcp_config_from_strn(s, strlen(s));
}
