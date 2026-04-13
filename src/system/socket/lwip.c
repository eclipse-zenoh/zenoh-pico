//
// Copyright (c) 2026 ZettaScale Technology
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

#include "zenoh-pico/link/transport/socket.h"

#if defined(ZP_PLATFORM_SOCKET_LWIP)

#if defined(ZP_PLATFORM_SOCKET_LINKS_ENABLED)
#include <unistd.h>

#include "lwip/sockets.h"
#include "zenoh-pico/link/transport/lwip_socket.h"
#endif

#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

static uintptr_t _z_socket_id_impl(const _z_sys_net_socket_t *sock) {
#if defined(ZP_PLATFORM_SOCKET_LINKS_ENABLED)
    return (uintptr_t)_z_lwip_socket_get(*sock);
#else
    _ZP_UNUSED(sock);
    return 0;
#endif
}

#if defined(ZP_PLATFORM_SOCKET_LINKS_ENABLED) && Z_FEATURE_LINK_TCP == 1
z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    int fd = _z_lwip_socket_get(*sock);
    int flags = lwip_fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (lwip_fcntl(fd, F_SETFL, blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK)) == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) { return _z_socket_set_blocking(sock, false); }

void _z_socket_close(_z_sys_net_socket_t *sock) {
    int fd = _z_lwip_socket_get(*sock);
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        lwip_close(fd);
        _z_lwip_socket_set(sock, -1);
    }
}

static z_result_t _z_socket_wait_readable_impl(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                               uint32_t timeout_ms) {
    fd_set read_fds;
    size_t i = 0;
    int max_fd = 0;

    FD_ZERO(&read_fds);
    if (count == 0) {
        return _Z_RES_OK;
    }

    for (i = 0; i < count; i++) {
        int fd = _z_lwip_socket_get(sockets[i]);
        ready[i] = 0;
        FD_SET(fd, &read_fds);
        if (fd > max_fd) {
            max_fd = fd;
        }
    }

    struct timeval timeout = {
        .tv_sec = (time_t)(timeout_ms / 1000U),
        .tv_usec = (suseconds_t)((timeout_ms % 1000U) * 1000U),
    };
    if (lwip_select(max_fd + 1, &read_fds, NULL, NULL, &timeout) <= 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    for (i = 0; i < count; i++) {
        if (FD_ISSET(_z_lwip_socket_get(sockets[i]), &read_fds)) {
            ready[i] = 1;
        }
    }

    return _Z_RES_OK;
}

#else
z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(blocking);
    _Z_ERROR("Function not yet supported on this system");
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) {
    _ZP_UNUSED(sock);
    _Z_ERROR("Function not yet supported on this system");
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

void _z_socket_close(_z_sys_net_socket_t *sock) { _ZP_UNUSED(sock); }

static z_result_t _z_socket_wait_readable_impl(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                               uint32_t timeout_ms) {
    _ZP_UNUSED(sockets);
    _ZP_UNUSED(count);
    _ZP_UNUSED(ready);
    _ZP_UNUSED(timeout_ms);
    _Z_ERROR("Function not yet supported on this system");
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}
#endif

uintptr_t _z_socket_id(const _z_sys_net_socket_t *sock) { return _z_socket_id_impl(sock); }

z_result_t _z_socket_wait_readable(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                   uint32_t timeout_ms) {
    return _z_socket_wait_readable_impl(sockets, count, ready, timeout_ms);
}

#endif /* defined(ZP_PLATFORM_SOCKET_LWIP) */
