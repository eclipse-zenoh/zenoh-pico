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

// Ethernet types (big endian)
#define _ZP_ETH_TYPE_VLAN 0x0081

// Address Sizes
#define _ZP_MAC_ADDR_LENGTH 6

// Max frame size
#define _ZP_MAX_ETH_FRAME_SIZE 1500

// Ethernet header structure type
typedef struct {
    uint8_t dmac[_ZP_MAC_ADDR_LENGTH];  // Destination mac address
    uint8_t smac[_ZP_MAC_ADDR_LENGTH];  // Source mac address
    uint16_t ethtype;                   // Ethertype of frame
} _zp_eth_header_t;

typedef struct {
    uint8_t dmac[_ZP_MAC_ADDR_LENGTH];  // Destination mac address
    uint8_t smac[_ZP_MAC_ADDR_LENGTH];  // Source mac address
    uint16_t vlan_type;                 // Vlan ethtype
    uint16_t tag;                       // Vlan tag
    uint16_t ethtype;                   // Ethertype of frame
} _zp_eth_vlan_header_t;

typedef struct {
    const char *_interface;
    _z_sys_net_socket_t _sock;
    uint16_t _vlan;
    uint16_t ethtype;
    uint8_t _dmac[_ZP_MAC_ADDR_LENGTH];
    uint8_t _smac[_ZP_MAC_ADDR_LENGTH];
    _Bool _has_vlan;
} _z_raweth_socket_t;

int8_t _z_get_smac_raweth(_z_raweth_socket_t *resock);
int8_t _z_open_raweth(_z_sys_net_socket_t *sock, const char *interface);
size_t _z_send_raweth(const _z_sys_net_socket_t *sock, const void *buff, size_t buff_len);
size_t _z_receive_raweth(const _z_sys_net_socket_t *sock, void *buff, size_t buff_len, _z_bytes_t *addr);
int8_t _z_close_raweth(_z_sys_net_socket_t *sock);

#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_RAWETH_H */
