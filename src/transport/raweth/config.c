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

#include "zenoh-pico/transport/raweth/config.h"

#if Z_FEATURE_RAWETH_TRANSPORT == 1

// Should be generated
const uint16_t _ZP_RAWETH_CFG_ETHTYPE = 0x72e0;

// Should be generated
const char *_ZP_RAWETH_CFG_INTERFACE = "lo";

// Should be generated
const uint8_t _ZP_RAWETH_CFG_SMAC[_ZP_MAC_ADDR_LENGTH] = {0x30, 0x03, 0xc8, 0x37, 0x25, 0xa1};

// Should be generated
const _zp_raweth_cfg_entry _ZP_RAWETH_CFG_ARRAY[] = {
    {{0, {0}, ""}, 0x00, {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}, false},                // Default mac addr
    {{0, {0}, "some/key/expr"}, 0x8c, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}, true},    // entry1
    {{0, {0}, "another/keyexpr"}, 0x43, {0x01, 0x23, 0x45, 0x67, 0x89, 0xab}, true},  // entry2
};

// Should be generated
const _zp_raweth_cfg_whitelist_val _ZP_RAWETH_CFG_WHITELIST[] = {
    {{0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}},
    {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}},
};

// Don't modify
const size_t _ZP_RAWETH_CFG_SIZE = _ZP_ARRAY_SIZE(_ZP_RAWETH_CFG_ARRAY);
const size_t _ZP_RAWETH_CFG_WHITELIST_SIZE = _ZP_ARRAY_SIZE(_ZP_RAWETH_CFG_WHITELIST);

#endif  // Z_FEATURE_RAWETH_TRANSPORT == 1
