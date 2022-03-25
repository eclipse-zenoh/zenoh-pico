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
#include "zenoh-pico/link/config/tcp.h"

#if Z_LINK_TCP == 1

size_t _z_tcp_config_strlen(const _z_str_intmap_t *s)
{
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_strlen(s, argc, args);
}

void _z_tcp_config_onto_str(_z_str_t dst, const _z_str_intmap_t *s)
{
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_onto_str(dst, s, argc, args);
}

_z_str_t _z_tcp_config_to_str(const _z_str_intmap_t *s)
{
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_to_str(s, argc, args);
}

_z_str_intmap_result_t _z_tcp_config_from_strn(const _z_str_t s, size_t n)
{
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_from_strn(s, argc, args, n);
}

_z_str_intmap_result_t _z_tcp_config_from_str(const _z_str_t s)
{
    return _z_tcp_config_from_strn(s, strlen(s));
}
#endif
