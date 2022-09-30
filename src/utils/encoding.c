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
    uint8_t *codep = output;
    output++;
    uint8_t code = 1;

    for (const uint8_t *byte = input; input_len != (size_t)0; byte++) {
        if (*byte != (uint8_t)0x00) {
            *output = *byte;
            output++;
            code++;
        }

        if (!*byte || (code == (uint8_t)0xFF)) {
            *codep = code;
            code = 1;
            codep = output;
            if (!*byte || input_len) {
                ++output;
            }
        }

        input_len--;
    }
    *codep = code;

    return output - output_initial_ptr;
}

size_t _z_cobs_decode(const uint8_t *input, size_t input_len, uint8_t *output) {
    const uint8_t *byte = input;
    uint8_t *output_initial_ptr = output;

    uint8_t code = (uint8_t)0xFF;
    for (uint8_t block = (uint8_t)0x00; byte < (input + input_len); block--) {
        if (block != (uint8_t)0x00) {
            *output = *byte;
            output++;
            byte++;
        } else {
            if (code != (uint8_t)0xFF) {
                *output = (uint8_t)0x00;
                output++;
            }
            code = *byte;
            block = *byte;
            byte++;
            if (code == (uint8_t)0x00) {
                break;
            }
        }
    }

    return output - output_initial_ptr;
}
