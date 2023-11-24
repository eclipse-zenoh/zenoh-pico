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

#ifndef ZENOH_PICO_SYSTEM_LINK_RAWETH_H
#define ZENOH_PICO_SYSTEM_LINK_RAWETH_H

#include <stdint.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/system/platform.h"

#if Z_FEATURE_RAWETH_TRANSPORT == 1

// Ethernet types
#define _ZP_ETH_TYPE_VLAN 0x8100   // 2048  (0x0800) IPv4
#define _ZP_ETH_TYPE_CUTOFF 0x600  // Values above are ethertype

// Address Sizes
#define _ZP_MAC_ADDR_LENGTH 6

// Max frame size
#define _ZP_MAX_ETH_FRAME_SIZE 1500

typedef struct {
    uint16_t tpid;      // Tag ethertype
    uint16_t pcp : 3;   // Priority code point
    uint16_t dei : 1;   // Drop eligible indicator
    uint16_t vid : 12;  // VLAN id
} _zp_vlan_tag_t;

// Ethernet header structure type
typedef struct {
    uint8_t dmac[_ZP_MAC_ADDR_LENGTH];  // Destination mac address
    uint8_t smac[_ZP_MAC_ADDR_LENGTH];  // Source mac address
    uint16_t length;                    // Size of frame
} _zp_eth_header_t;

typedef struct {
    uint8_t dmac[_ZP_MAC_ADDR_LENGTH];  // Destination mac address
    uint8_t smac[_ZP_MAC_ADDR_LENGTH];  // Source mac address
    _zp_vlan_tag_t tag;
    uint16_t length;  // Size of frame
} _zp_eth_vlan_header_t;

typedef struct {
    void *_config;  // Pointer to config data
    _z_sys_net_socket_t _sock;
    uint16_t _vlan;
    uint8_t _dmac[_ZP_MAC_ADDR_LENGTH];
    uint8_t _smac[_ZP_MAC_ADDR_LENGTH];
    _Bool _has_vlan;
} _z_raweth_socket_t;

int8_t _z_get_smac_raweth(_z_raweth_socket_t *resock);
int8_t _z_open_raweth(_z_sys_net_socket_t *sock);
size_t _z_send_raweth(const _z_sys_net_socket_t *sock, const void *buff, size_t buff_len);
size_t _z_receive_raweth(const _z_sys_net_socket_t *sock, void *buff, size_t buff_len, _z_bytes_t *addr);

#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_RAWETH_H */
