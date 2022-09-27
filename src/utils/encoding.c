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

#include "zenoh-pico/utils/encoding.h"

size_t _z_cobs_encode(const uint8_t *input, size_t input_len, uint8_t *output) {
    uint8_t *output_initial_ptr = output;
    uint8_t *codep = output++;
    uint8_t code = 1;

    for (const uint8_t *byte = input; input_len--; ++byte) {
        if (*byte) {
            *output = *byte;
            output++;
            code++;
        }

        if (!*byte || code == (uint8_t)0xFF) {
            *codep = code;
            code = 1;
            codep = output;
            if (!*byte || input_len) {
                ++output;
            }
        }
    }
    *codep = code;

    return (size_t)(output - output_initial_ptr);
}

size_t _z_cobs_decode(const uint8_t *input, size_t input_len, uint8_t *output) {
    const uint8_t *byte = input;
    uint8_t *output_initial_ptr = output;

    for (uint8_t code = 0xFF, block = 0; byte < input + input_len; --block) {
        if (block) {
            *output = *byte;
            output++;
            byte++;
        } else {
            if (code != (uint8_t)0xff) {
                *output++ = 0;
            }
            block = code = *byte++;
            if (code == (uint8_t)0x00) {
                break;
            }
        }
    }

    return (size_t)(output - output_initial_ptr);
}
