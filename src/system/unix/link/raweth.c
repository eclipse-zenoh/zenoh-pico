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

#include "zenoh-pico/system/link/raweth.h"

#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/system/link/raweth.h"
#include "zenoh-pico/system/platform/unix.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_RAWETH_TRANSPORT == 1

#if !defined(__linux)
#error "Raweth transport only supported on linux systems"
#else
#include <linux/if_packet.h>

void _z_raweth_clear_mapping_entry(_zp_raweth_mapping_entry_t *entry) { _z_keyexpr_clear(&entry->_keyexpr); }

int8_t _z_open_raweth(_z_sys_net_socket_t *sock, const char *interface) {
    int8_t ret = _Z_RES_OK;
    // Open a raw network socket in promiscuous mode
    sock->_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock->_fd == -1) {
        return _Z_ERR_GENERIC;
    }
    // Get the index of the interface to send on
    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, interface, strlen(interface));
    if (ioctl(sock->_fd, SIOCGIFINDEX, &if_idx) < 0) {
        return _Z_ERR_GENERIC;
    }
    // Bind the socket
    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = if_idx.ifr_ifindex;
    addr.sll_pkttype = PACKET_HOST | PACKET_BROADCAST | PACKET_MULTICAST;

    if (bind(sock->_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(sock->_fd);
        ret = _Z_ERR_GENERIC;
    }
    return ret;
}

int8_t _z_close_raweth(_z_sys_net_socket_t *sock) {
    int8_t ret = _Z_RES_OK;
    if (close(sock->_fd) != 0) {
        ret = _Z_ERR_GENERIC;
    }
    return ret;
}

size_t _z_send_raweth(const _z_sys_net_socket_t *sock, const void *buff, size_t buff_len) {
    // Send data
    ssize_t wb = write(sock->_fd, buff, buff_len);
    if (wb < 0) {
        return SIZE_MAX;
    }
    return (size_t)wb;
}

size_t _z_receive_raweth(const _z_sys_net_socket_t *sock, void *buff, size_t buff_len, _z_slice_t *addr,
                         const _zp_raweth_whitelist_array_t *whitelist) {
    // Read from socket
    ssize_t bytesRead = recvfrom(sock->_fd, buff, buff_len, 0, NULL, NULL);
    if ((bytesRead < 0) || (bytesRead < (ssize_t)sizeof(_zp_eth_header_t))) {
        return SIZE_MAX;
    }
    _Bool is_valid = true;
    // Address filtering (only if there is a whitelist)
    if (_zp_raweth_whitelist_array_len(whitelist) > 0) {
        is_valid = false;
        const _zp_eth_header_t *header = (_zp_eth_header_t *)buff;
        for (size_t i = 0; i < _zp_raweth_whitelist_array_len(whitelist); i++) {
            const _zp_raweth_whitelist_entry_t *entry = _zp_raweth_whitelist_array_get(whitelist, i);
            if (memcmp(&header->smac, entry->_mac, _ZP_MAC_ADDR_LENGTH) == 0) {
                is_valid = true;
                break;
            }
        }
    }
    // Ignore packet from unknown sources
    if (!is_valid) {
        return SIZE_MAX;
    }
    // Copy sender mac if needed
    if (addr != NULL) {
        uint8_t *header_addr = (uint8_t *)buff;
        *addr = _z_slice_make(sizeof(ETH_ALEN));
        (void)memcpy((uint8_t *)addr->start, (header_addr + ETH_ALEN), sizeof(ETH_ALEN));
    }
    return (size_t)bytesRead;
}

uint16_t _z_raweth_ntohs(uint16_t val) { return ntohs(val); }

uint16_t _z_raweth_htons(uint16_t val) { return htons(val); }

#endif  // defined(__linux)
#endif  // Z_FEATURE_RAWETH_TRANSPORT == 1
