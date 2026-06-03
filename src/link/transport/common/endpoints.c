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

#include "zenoh-pico/config.h"

#if defined(ZENOH_WINDOWS)

#include <winsock2.h>
#include <ws2tcpip.h>

#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

static z_result_t _z_sockaddr_to_endpoint(const SOCKADDR *addr, char *dst, size_t dst_len) {
    if (addr->sa_family == AF_INET) {
        const SOCKADDR_IN *addr4 = (const SOCKADDR_IN *)addr;
        const uint8_t *bytes = (const uint8_t *)&addr4->sin_addr;
        return _z_ip_port_to_endpoint(bytes, sizeof(addr4->sin_addr), ntohs(addr4->sin_port), dst, dst_len);
    } else if (addr->sa_family == AF_INET6) {
        const SOCKADDR_IN6 *addr6 = (const SOCKADDR_IN6 *)addr;
        const uint8_t *bytes = (const uint8_t *)&addr6->sin6_addr;
        return _z_ip_port_to_endpoint(bytes, sizeof(addr6->sin6_addr), ntohs(addr6->sin6_port), dst, dst_len);
    } else {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
}

z_result_t _z_socket_get_endpoints(const _z_sys_net_socket_t *sock, char *local, size_t local_len, char *remote,
                                   size_t remote_len) {
    SOCKADDR_STORAGE local_addr = {0};
    SOCKADDR_STORAGE remote_addr = {0};
    int local_addr_len = sizeof(local_addr);
    int remote_addr_len = sizeof(remote_addr);
    SOCKET fd;

    if (sock == NULL || local == NULL || remote == NULL || local_len == 0 || remote_len == 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    fd = sock->_sock._fd;
    if (fd == INVALID_SOCKET) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    if (getsockname(fd, (SOCKADDR *)&local_addr, &local_addr_len) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (getpeername(fd, (SOCKADDR *)&remote_addr, &remote_addr_len) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    _Z_RETURN_IF_ERR(_z_sockaddr_to_endpoint((const SOCKADDR *)&local_addr, local, local_len));
    _Z_RETURN_IF_ERR(_z_sockaddr_to_endpoint((const SOCKADDR *)&remote_addr, remote, remote_len));
    return _Z_RES_OK;
}

#elif defined(ZENOH_LINUX) || defined(ZENOH_MACOS) || defined(ZENOH_BSD) || defined(ZENOH_ZEPHYR)

#include <stddef.h>

#if defined(ZENOH_ZEPHYR)
#include <netdb.h>
#include <sys/socket.h>
#include <zephyr/net/socket.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

static z_result_t _z_sockaddr_to_endpoint(const struct sockaddr *addr, char *dst, size_t dst_len) {
    if (addr->sa_family == AF_INET) {
        const struct sockaddr_in *addr4 = (const struct sockaddr_in *)addr;
        const uint8_t *bytes = (const uint8_t *)&addr4->sin_addr;
        return _z_ip_port_to_endpoint(bytes, sizeof(addr4->sin_addr), ntohs(addr4->sin_port), dst, dst_len);
    } else if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *)addr;
        const uint8_t *bytes = (const uint8_t *)&addr6->sin6_addr;
        return _z_ip_port_to_endpoint(bytes, sizeof(addr6->sin6_addr), ntohs(addr6->sin6_port), dst, dst_len);
    } else {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
}

z_result_t _z_socket_get_endpoints(const _z_sys_net_socket_t *sock, char *local, size_t local_len, char *remote,
                                   size_t remote_len) {
    struct sockaddr_storage local_addr = {0};
    struct sockaddr_storage remote_addr = {0};
    socklen_t local_addr_len = sizeof(local_addr);
    socklen_t remote_addr_len = sizeof(remote_addr);

    if (sock == NULL || local == NULL || remote == NULL || local_len == 0 || remote_len == 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    if (sock->_fd < 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    if (getsockname(sock->_fd, (struct sockaddr *)&local_addr, &local_addr_len) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (getpeername(sock->_fd, (struct sockaddr *)&remote_addr, &remote_addr_len) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    _Z_RETURN_IF_ERR(_z_sockaddr_to_endpoint((const struct sockaddr *)&local_addr, local, local_len));
    _Z_RETURN_IF_ERR(_z_sockaddr_to_endpoint((const struct sockaddr *)&remote_addr, remote, remote_len));
    return _Z_RES_OK;
}

#else

#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/utils/logging.h"

z_result_t _z_socket_get_endpoints(const _z_sys_net_socket_t *sock, char *local, size_t local_len, char *remote,
                                   size_t remote_len) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(local);
    _ZP_UNUSED(local_len);
    _ZP_UNUSED(remote);
    _ZP_UNUSED(remote_len);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}

#endif
