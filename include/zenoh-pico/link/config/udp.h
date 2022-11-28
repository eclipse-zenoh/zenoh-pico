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

#ifndef ZENOH_PICO_LINK_CONFIG_UDP_H
#define ZENOH_PICO_LINK_CONFIG_UDP_H

#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1

#define UDP_CONFIG_IFACE_KEY 0x01
#define UDP_CONFIG_IFACE_STR "iface"

#define UDP_CONFIG_TOUT_KEY 0x02
#define UDP_CONFIG_TOUT_STR "tout"

#define UDP_CONFIG_MAPPING_BUILD         \
    uint8_t argc = 2;                    \
    _z_str_intmapping_t args[argc];      \
    args[0]._key = UDP_CONFIG_IFACE_KEY; \
    args[0]._str = UDP_CONFIG_IFACE_STR; \
    args[1]._key = UDP_CONFIG_TOUT_KEY;  \
    args[1]._str = UDP_CONFIG_TOUT_STR;

size_t _z_udp_config_strlen(const _z_str_intmap_t *s);

void _z_udp_config_onto_str(char *dst, size_t dst_len, const _z_str_intmap_t *s);
char *_z_udp_config_to_str(const _z_str_intmap_t *s);

int8_t _z_udp_config_from_str(_z_str_intmap_t *strint, const char *s);
int8_t _z_udp_config_from_strn(_z_str_intmap_t *strint, const char *s, size_t n);
#endif

#endif /* ZENOH_PICO_LINK_CONFIG_UDP_H */