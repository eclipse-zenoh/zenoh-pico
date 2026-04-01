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

#include "zenoh-pico/config.h"

#if defined(ZENOH_ZEPHYR)

#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/select.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/backend/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static uintptr_t _z_socket_id_impl(const _z_sys_net_socket_t *sock) { return (uintptr_t)sock->_fd; }

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    int flags = fcntl(sock->_fd, F_GETFL, 0);
    if (flags == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (fcntl(sock->_fd, F_SETFL, blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK)) == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) { return _z_socket_set_blocking(sock, false); }

void _z_socket_close(_z_sys_net_socket_t *sock) {
    if (sock->_fd >= 0) {
        close(sock->_fd);
        sock->_fd = -1;
    }
}

#if Z_FEATURE_MULTI_THREAD == 1
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
        ready[i] = 0;
        FD_SET(sockets[i]._fd, &read_fds);
        if (sockets[i]._fd > max_fd) {
            max_fd = sockets[i]._fd;
        }
    }

    struct timeval timeout = {
        .tv_sec = (time_t)(timeout_ms / 1000U),
        .tv_usec = (suseconds_t)((timeout_ms % 1000U) * 1000U),
    };
    int result = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result <= 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    for (i = 0; i < count; i++) {
        if (FD_ISSET(sockets[i]._fd, &read_fds)) {
            ready[i] = 1;
        }
    }

    return _Z_RES_OK;
}
#endif

const _z_socket_ops_t _z_zephyr_socket_ops = {
    .id = _z_socket_id_impl,
    .set_non_blocking = _z_socket_set_non_blocking,
#if Z_FEATURE_MULTI_THREAD == 1
    .wait_readable = _z_socket_wait_readable_impl,
#else
    .wait_readable = NULL,
#endif
    .close = _z_socket_close,
};

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Zephyr port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on Zephyr port of Zenoh-Pico"
#endif

#endif /* defined(ZENOH_ZEPHYR) */
