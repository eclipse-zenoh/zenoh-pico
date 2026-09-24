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

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

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

void _z_socket_close(_z_sys_net_socket_t *sock) {
    if (sock->_fd >= 0) {
        shutdown(sock->_fd, SHUT_RDWR);
        close(sock->_fd);
        sock->_fd = -1;
    }
}

z_result_t _z_socket_wait_readable(_z_socket_wait_iter_t *iter, uint32_t timeout_ms) {
    // Count sockets to determine poll array size
    size_t sock_count = 0;
    _z_socket_wait_iter_reset(iter);
    while (_z_socket_wait_iter_next(iter)) {
        sock_count++;
    }

    if (sock_count == 0) {
        return _Z_RES_OK;
    }

    // Build pollfd array on the stack (zenoh-pico typically has very few sockets)
    struct pollfd fds[sock_count];
    size_t idx = 0;

    _z_socket_wait_iter_reset(iter);
    while (_z_socket_wait_iter_next(iter)) {
        const _z_sys_net_socket_t *sock = _z_socket_wait_iter_get_socket(iter);
        _z_socket_wait_iter_set_ready(iter, false);
        fds[idx].fd = sock->_fd;
        fds[idx].events = POLLIN;
        fds[idx].revents = 0;
        idx++;
    }

    int result = poll(fds, (nfds_t)sock_count, (int)timeout_ms);
    if (result < 0) {
        _Z_DEBUG("Errno: %d\n", errno);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    bool has_data = false;
    idx = 0;
    _z_socket_wait_iter_reset(iter);
    while (_z_socket_wait_iter_next(iter)) {
        bool is_ready = (fds[idx].revents & POLLIN) != 0;
        _z_socket_wait_iter_set_ready(iter, is_ready);
        has_data |= is_ready;
        idx++;
    }

    return has_data ? _Z_RES_OK : _Z_NO_DATA_PROCESSED;
}

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported on WASI"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
#error "Serial not supported on WASI"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet not supported on WASI"
#endif
