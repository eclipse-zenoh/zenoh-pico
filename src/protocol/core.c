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

#include "zenoh-pico/protocol/core.h"

uint8_t _z_intres_to_nbytes(_z_int_res_t intres) {
    uint8_t n_bytes = 0;

    switch (intres) {
        case 0: {
            n_bytes = 1;
        } break;

        case 1: {
            n_bytes = 2;
        } break;

        case 2: {
            n_bytes = 4;
        } break;

        case 3: {
            n_bytes = 8;
        } break;

        default: {
            n_bytes = 0;
        }
    }

    return n_bytes;
}