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

#include "zenoh-pico/utils/uuid.h"

#include <stdint.h>
#include <stdlib.h>

#include "zenoh-pico/utils/pointers.h"

#define UUID_SIZE 16

void _z_uuid_to_bytes(uint8_t *bytes, const char *uuid_str) {
    uint8_t n_dash = 0;
    for (uint8_t i = 0; i < 32; i += 2) {
        if (i == 8 || i == 12 || i == 16 || i == 18) {
            n_dash += 1;
        }
        char val[5] = {'0', 'x', uuid_str[i + n_dash], uuid_str[i + 1 + n_dash], '\0'};
        *bytes = (uint8_t)strtoul(val, NULL, 0);
        bytes = _z_ptr_u8_offset(bytes, 1);
    }
}
