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

#ifndef ZENOH_PICO_LINK_CONFIG_TCP_H
#define ZENOH_PICO_LINK_CONFIG_TCP_H

#include "zenoh-pico/config.h"
#include "zenoh-pico/collections/intmap.h"
#include "zenoh-pico/collections/string.h"

#if ZN_LINK_TCP == 1

#define TCP_CONFIG_TOUT_KEY  0x01
#define TCP_CONFIG_TOUT_STR  "tout"

#define TCP_CONFIG_MAPPING_BUILD       \
    int argc = 1;                      \
    _z_str_intmapping_t args[argc];    \
    args[0].key = TCP_CONFIG_TOUT_KEY; \
    args[0].str = TCP_CONFIG_TOUT_STR;

size_t _zn_tcp_config_strlen(const _z_str_intmap_t *s);

void _zn_tcp_config_onto_str(z_str_t dst, const _z_str_intmap_t *s);
z_str_t _zn_tcp_config_to_str(const _z_str_intmap_t *s);

_z_str_intmap_result_t _zn_tcp_config_from_str(const z_str_t s);
_z_str_intmap_result_t _zn_tcp_config_from_strn(const z_str_t s, size_t n);

#endif

#endif /* ZENOH_PICO_LINK_CONFIG_TCP_H */
