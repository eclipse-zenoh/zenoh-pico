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

#include "zenoh-pico/link/config/serial.h"

#include <string.h>

#if Z_LINK_SERIAL == 1

size_t _z_serial_config_strlen(const _z_str_intmap_t *s) {
    SERIAL_CONFIG_MAPPING_BUILD

    return _z_str_intmap_strlen(s, argc, args);
}

void _z_serial_config_onto_str(char *dst, size_t dst_len, const _z_str_intmap_t *s) {
    SERIAL_CONFIG_MAPPING_BUILD

    return _z_str_intmap_onto_str(dst, dst_len, s, argc, args);
}

char *_z_serial_config_to_str(const _z_str_intmap_t *s) {
    SERIAL_CONFIG_MAPPING_BUILD

    return _z_str_intmap_to_str(s, argc, args);
}

int8_t _z_serial_config_from_strn(_z_str_intmap_t *strint, const char *s, size_t n) {
    SERIAL_CONFIG_MAPPING_BUILD

    return _z_str_intmap_from_strn(strint, s, argc, args, n);
}

int8_t _z_serial_config_from_str(_z_str_intmap_t *strint, const char *s) {
    SERIAL_CONFIG_MAPPING_BUILD

    return _z_str_intmap_from_str(strint, s, argc, args);
}
#endif
