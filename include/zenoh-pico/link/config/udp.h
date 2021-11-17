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

#ifndef ZENOH_PICO_LINK_CONFIG_UDP_H
#define ZENOH_PICO_LINK_CONFIG_UDP_H

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/endpoint.h"

#define UDP_CONFIG_MULTICAST_IFACE_KEY 0x01
#define UDP_CONFIG_MULTICAST_IFACE_STR "iface"

#define UDP_CONFIG_MAPPING_BUILD                  \
    int argc = 1;                                 \
    zn_int_str_mapping_t args[argc];              \
    args[0].key = UDP_CONFIG_MULTICAST_IFACE_KEY; \
    args[0].str = UDP_CONFIG_MULTICAST_IFACE_STR;

size_t _zn_udp_config_strlen(const zn_int_str_map_t *s);

void _zn_udp_config_onto_str(z_str_t dst, const zn_int_str_map_t *s);
z_str_t _zn_udp_config_to_str(const zn_int_str_map_t *s);

zn_int_str_map_result_t _zn_udp_config_from_str(const z_str_t s);
zn_int_str_map_result_t _zn_udp_config_from_strn(const z_str_t s, size_t n);

#endif /* ZENOH_PICO_LINK_CONFIG_UDP_H */