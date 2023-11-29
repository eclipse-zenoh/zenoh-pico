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

#ifndef ZENOH_PICO_RAWETH_CONFIG_H
#define ZENOH_PICO_RAWETH_CONFIG_H

#include <stdbool.h>

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/system/link/raweth.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_RAWETH_TRANSPORT == 1

typedef struct {
    _z_keyexpr_t _keyexpr;
    uint16_t _vlan;  // vlan tag (pcp + dei + id)
    uint8_t _dmac[_ZP_MAC_ADDR_LENGTH];
    _Bool _has_vlan;
} _zp_raweth_cfg_entry;

typedef struct {
    uint8_t _mac[_ZP_MAC_ADDR_LENGTH];
} _zp_raweth_cfg_whitelist_val;

// Ethertype to use in frame
extern const uint16_t _ZP_RAWETH_CFG_ETHTYPE;

// Source mac address
extern const uint8_t _ZP_RAWETH_CFG_SMAC[_ZP_MAC_ADDR_LENGTH];

// Main config array
extern const _zp_raweth_cfg_entry _ZP_RAWETH_CFG_ARRAY[];

// Mac address rx whitelist array
extern const _zp_raweth_cfg_whitelist_val _ZP_RAWETH_CFG_WHITELIST[];

// Array size
extern const size_t _ZP_RAWETH_CFG_SIZE;
extern const size_t _ZP_RAWETH_CFG_WHITELIST_SIZE;

#endif  // Z_FEATURE_RAWETH_TRANSPORT == 1
#endif  // ZENOH_PICO_RAWETH_CONFIG_H
