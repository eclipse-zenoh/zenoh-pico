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

#include "zenoh-pico/link/config/tcp.h"

#include <string.h>

#include "zenoh-pico/config.h"

#if Z_LINK_TCP == 1

size_t _z_tcp_config_strlen(const _z_str_intmap_t *s) {
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_strlen(s, argc, args);
}

void _z_tcp_config_onto_str(char *dst, size_t dst_len, const _z_str_intmap_t *s) {
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_onto_str(dst, dst_len, s, argc, args);
}

char *_z_tcp_config_to_str(const _z_str_intmap_t *s) {
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_to_str(s, argc, args);
}

int8_t _z_tcp_config_from_strn(_z_str_intmap_t *strint, const char *s, size_t n) {
    TCP_CONFIG_MAPPING_BUILD

    return _z_str_intmap_from_strn(strint, s, argc, args, n);
}

int8_t _z_tcp_config_from_str(_z_str_intmap_t *strint, const char *s) {
    return _z_tcp_config_from_strn(strint, s, strlen(s));
}
#endif
