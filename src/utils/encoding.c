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

#include "zenoh-pico/utils/pointers.h"

size_t _z_cobs_encode(const uint8_t *input, size_t input_len, uint8_t *output) {
    uint8_t *output_initial_ptr = output;
    uint8_t *codep = output;
    uint8_t code = 1;

    output = _z_ptr_u8_offset(output, 1);
    for (const uint8_t *byte = input; input_len != (size_t)0; byte++) {
        if (*byte != (uint8_t)0x00) {
            *output = *byte;
            output = _z_ptr_u8_offset(output, 1);
            code++;
        }

        if (!*byte || (code == (uint8_t)0xFF)) {
            *codep = code;
            code = 1;
            codep = output;
            if (!*byte || input_len) {
                output = _z_ptr_u8_offset(output, 1);
            }
        }

        input_len--;
    }
    *codep = code;

    return _z_ptr_u8_diff(output, output_initial_ptr);
}

size_t _z_cobs_decode(const uint8_t *input, size_t input_len, uint8_t *output) {
    const uint8_t *byte = input;
    uint8_t *output_initial_ptr = output;

    uint8_t code = (uint8_t)0xFF;
    const uint8_t *input_end_ptr = _z_cptr_u8_offset(input, input_len);
    for (uint8_t block = (uint8_t)0x00; byte < input_end_ptr; block--) {
        if (block != (uint8_t)0x00) {
            *output = *byte;
            output = _z_ptr_u8_offset(output, 1);
            byte = _z_cptr_u8_offset(byte, 1);
        } else {
            if (code != (uint8_t)0xFF) {
                *output = (uint8_t)0x00;
                output = _z_ptr_u8_offset(output, 1);
            }
            code = *byte;
            block = *byte;
            byte = _z_cptr_u8_offset(byte, 1);
            if (code == (uint8_t)0x00) {
                break;
            }
        }
    }

    return _z_ptr_u8_diff(output, output_initial_ptr);
}
