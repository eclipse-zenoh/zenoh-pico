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
#include "zenoh-pico/system/platform/unix.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_RAWETH_TRANSPORT == 1

int8_t _z_get_smac_raweth(_z_raweth_socket_t *resock) {
    int8_t ret = _Z_RES_OK;
    struct ifaddrs *sys_ifa, *sys_ifa_root;
    struct sockaddr *sa;

    // Retrieve all interfaces data
    if (getifaddrs(&sys_ifa_root) == -1) {
        switch (errno) {
            case ENOMEM:
            case ENOBUFS:
                ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
                break;
            case EACCES:
            default:
                ret = _Z_ERR_GENERIC;
                break;
        }
    } else {
        // Parse the interfaces
        for (sys_ifa = sys_ifa_root; sys_ifa != NULL && ret == 0; sys_ifa = sys_ifa->ifa_next) {
            // Skip interfaces without an address or not up or loopback
            if (sys_ifa->ifa_addr == NULL /*|| (sys_ifa->ifa_flags & IFF_UP) == 0 ||
                (sys_ifa->ifa_flags & IFF_LOOPBACK) != 0*/) {
                continue;
            }
            // Interface is ethernet
            if (sys_ifa->ifa_addr->sa_family == AF_PACKET) {
                // Copy data
                memcpy(resock->_smac, sys_ifa->ifa_addr->sa_data, _ZP_MAC_ADDR_LENGTH);
                break;
            }
        }
        freeifaddrs(sys_ifa_root);
    }
    return ret;
}

int8_t _z_open_raweth(_z_sys_net_socket_t *sock) {
    int8_t ret = _Z_RES_OK;
    // Open a raw network socket in promiscuous mode
    sock->_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock->_fd == -1) {
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

size_t _z_receive_raweth(const _z_sys_net_socket_t *sock, void *buff, size_t buff_len, _z_bytes_t *addr) {
    size_t bytesRead = recvfrom(sock->_fd, buff, buff_len, 0, NULL, NULL);
    if (bytesRead < 0) {
        return SIZE_MAX;
    }
    // Soft Filtering ?

    // Copy sender mac if needed
    if ((addr != NULL) && (bytesRead > 2 * ETH_ALEN)) {
        *addr = _z_bytes_make(sizeof(ETH_ALEN));
        (void)memcpy((uint8_t *)addr->start, (buff + ETH_ALEN), sizeof(ETH_ALEN));
    }
    return bytesRead;
}

#endif