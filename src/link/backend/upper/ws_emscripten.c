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

#if defined(ZENOH_EMSCRIPTEN) && Z_FEATURE_LINK_WS == 1

#include <arpa/inet.h>
#include <emscripten/websocket.h>
#include <netdb.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "zenoh-pico/link/backend/ws.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#define WS_LINK_SLEEP 1

static z_result_t _z_ws_emscripten_create_endpoint(_z_sys_net_endpoint_t *ep, const char *s_addr, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_addr, s_port, &hints, &ep->_iptcp) < 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static void _z_ws_emscripten_free_endpoint(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

static z_result_t _z_ws_emscripten_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

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
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_ws_emscripten_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t lep) {
    z_result_t ret = _Z_RES_OK;
    _ZP_UNUSED(sock);
    _ZP_UNUSED(lep);

    // @TODO: To be implemented
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    ret = _Z_ERR_GENERIC;

    return ret;
}

static void _z_ws_emscripten_close(_z_sys_net_socket_t *sock) { close(sock->_ws._fd); }

static size_t _z_ws_emscripten_read(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    // WARNING: workaroud implementing here the timeout
    ssize_t rb = 0;
    z_time_t start = z_time_now();
    while ((z_time_elapsed_ms(&start) < sock._ws._tout) && (rb <= 0)) {
        z_sleep_ms(WS_LINK_SLEEP);
        rb = recv(sock._ws._fd, ptr, len, 0);
    }
    if (rb < 0) {
        rb = SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_ws_emscripten_read_exact(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_ws_emscripten_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_ws_emscripten_write(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    // WARNING: workaroud implementing here the timeout
    ssize_t sb = 0;
    z_time_t start = z_time_now();
    while ((z_time_elapsed_ms(&start) < sock._ws._tout) && (sb <= 0)) {
        z_sleep_ms(WS_LINK_SLEEP);
        sb = send(sock._ws._fd, ptr, len, 0);
    }
    // WARNING: workaround as the recv returns -1 not only in case of errors
    if (sb < 0) {
        sb = 0;
    }
    return (size_t)sb;
}

z_result_t _z_ws_endpoint_init(_z_sys_net_endpoint_t *ep, const _z_string_t *address) {
    z_result_t ret = _Z_RES_OK;
    char *host = _z_endpoint_parse_host(address);
    char *port = _z_endpoint_parse_port(address);

    if ((host == NULL) || (port == NULL)) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    } else {
        ret = _z_ws_emscripten_create_endpoint(ep, host, port);
    }

    z_free(host);
    z_free(port);
    return ret;
}

void _z_ws_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_ws_emscripten_free_endpoint(ep); }

z_result_t _z_ws_transport_open(_z_ws_socket_t *sock, uint32_t tout) {
    return _z_ws_emscripten_open(&sock->_sock, sock->_rep, tout);
}

z_result_t _z_ws_transport_listen(_z_ws_socket_t *sock) { return _z_ws_emscripten_listen(&sock->_sock, sock->_rep); }

void _z_ws_transport_close(_z_ws_socket_t *sock) { _z_ws_emscripten_close(&sock->_sock); }

size_t _z_ws_transport_read(const _z_ws_socket_t *sock, uint8_t *ptr, size_t len) {
    return _z_ws_emscripten_read(sock->_sock, ptr, len);
}

size_t _z_ws_transport_read_exact(const _z_ws_socket_t *sock, uint8_t *ptr, size_t len) {
    return _z_ws_emscripten_read_exact(sock->_sock, ptr, len);
}

size_t _z_ws_transport_write(const _z_ws_socket_t *sock, const uint8_t *ptr, size_t len) {
    return _z_ws_emscripten_write(sock->_sock, ptr, len);
}

size_t _z_ws_transport_read_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    return _z_ws_emscripten_read(socket, ptr, len);
}

#else
typedef int _zp_ws_emscripten_backend_disabled_t;
#endif
