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

#include <arpa/inet.h>
#include <emscripten/websocket.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_LINK_WS == 1

#define WS_LINK_SLEEP 1

/*------------------ TCP sockets ------------------*/
int8_t _z_create_endpoint_ws(_z_sys_net_endpoint_t *ep, const char *s_addr, const char *s_port) {
    int8_t ret = _Z_RES_OK;

    struct addrinfo hints;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_addr, s_port, &hints, &ep->_iptcp) < 0) {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

void _z_free_endpoint_ws(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

/*------------------ TCP sockets ------------------*/
int8_t _z_open_ws(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    int8_t ret = _Z_RES_OK;

    sock->_ws._fd = socket(rep._iptcp->ai_family, rep._iptcp->ai_socktype, rep._iptcp->ai_protocol);
    if (sock->_ws._fd != -1) {
        // WARNING: commented because setsockopt is not implemented in emscripten
        // if ((ret == _Z_RES_OK) && (setsockopt(sock->_ws._fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0))
        // {
        //     ret = _Z_ERR_GENERIC;
        // }
        sock->_ws._tout = tout;  // We are storing the timeout that we are going to use when sending and receiving

        struct addrinfo *it = NULL;
        for (it = rep._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(sock->_ws._fd, it->ai_addr, it->ai_addrlen) < 0)) {
                break;
                // WARNING: breaking here because connect returns -1 even if the
                // underlying socket is actually open.

                // if (it->ai_next == NULL) {
                //     ret = _Z_ERR_GENERIC;
                //     break;
                // }
            } else {
                break;
            }
        }

        // WARNING: workaround as connect returns before the websocket is
        // actually open.
        z_sleep_ms(100);

        if (ret != _Z_RES_OK) {
            close(sock->_ws._fd);
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t _z_listen_ws(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    int8_t ret = _Z_RES_OK;
    (void)sock;
    (void)lep;

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_ws(_z_sys_net_socket_t *sock) { close(sock->_ws._fd); }

size_t _z_read_ws(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    // WARNING: workaroud implementing here the timeout
    ssize_t rb = 0;
    z_time_t start = z_time_now();
    while ((z_time_elapsed_ms(&start) < sock._ws._tout) && (rb <= 0)) {
        z_sleep_ms(WS_LINK_SLEEP);  // WARNING: workaround need to give the hand to the emscripten threads
        rb = recv(sock._ws._fd, ptr, len, 0);
    }
    if (rb < 0) {
        rb = SIZE_MAX;
    }
    return rb;
}

size_t _z_read_exact_ws(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_read_ws(sock, pos, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, n);
    } while (n != len);

    return n;
}

size_t _z_send_ws(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    // WARNING: workaroud implementing here the timeout
    ssize_t sb = 0;
    z_time_t start = z_time_now();
    while ((z_time_elapsed_ms(&start) < sock._ws._tout) && (sb <= 0)) {
        z_sleep_ms(WS_LINK_SLEEP);  // WARNING: workaround need to give the hand to the emscripten threads
        sb = send(sock._ws._fd, ptr, len, 0);
    }
    // WARNING: workaround as the recv returns -1 not only in case of errors
    if (sb < 0) {
        sb = 0;
    }
    return sb;
}

#endif

#if Z_FEATURE_LINK_TCP == 1
#error "TCP not supported yet on Emscripten port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
#error "UDP not supported yet on Emscripten port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Emscripten port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
#error "Serial not supported yet on Emscripten port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on Emscripten port of Zenoh-Pico"
#endif
