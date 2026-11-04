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

#include "zenoh-pico/link/transport/tcp.h"

#if defined(ZP_PLATFORM_SOCKET_ESP32)

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stddef.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_tcp_esp32_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    ep->_iptcp = NULL;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) != 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static void _z_tcp_esp32_endpoint_clear(_z_sys_net_endpoint_t *ep) {
    if ((ep == NULL) || (ep->_iptcp == NULL)) {
        return;
    }

    freeaddrinfo(ep->_iptcp);
    ep->_iptcp = NULL;
}

// Apply the link-layer socket timeout like tcp_lwip.c and the other ports do. Without it recv()
// blocks forever and a silent peer parks the executor task that runs read, lease and keep-alive.
// SO_RCVTIMEO = tout (Z_CONFIG_SOCKET_TIMEOUT), SO_SNDTIMEO = 1000 ms, and
// connect() is non-blocking with a select() bounded by `tout`.
#define _Z_TCP_ESP32_SEND_TIMEOUT_MS 1000u

static void _z_tcp_esp32_ms_to_timeval(uint32_t ms, struct timeval *tv) {
    tv->tv_sec = (time_t)(ms / (uint32_t)1000);
    tv->tv_usec = (suseconds_t)((ms % (uint32_t)1000) * (uint32_t)1000);
}

static int _z_tcp_esp32_connect_tout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint32_t tout) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }

    int rc = connect(fd, addr, addrlen);
    if (rc < 0) {
        if (errno != EINPROGRESS) {
            (void)fcntl(fd, F_SETFL, flags);
            return -1;
        }
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(fd, &wfds);
        struct timeval tv;
        _z_tcp_esp32_ms_to_timeval(tout, &tv);
        rc = select(fd + 1, NULL, &wfds, NULL, &tv);
        if (rc <= 0) {
            if (rc == 0) {
                errno = ETIMEDOUT;
            }
            (void)fcntl(fd, F_SETFL, flags);
            return -1;
        }
        int so_error = 0;
        socklen_t so_len = sizeof(so_error);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_len) < 0) {
            (void)fcntl(fd, F_SETFL, flags);
            return -1;
        }
        if (so_error != 0) {
            errno = so_error;
            (void)fcntl(fd, F_SETFL, flags);
            return -1;
        }
    }

    // back to blocking: reads/writes are bounded by SO_RCVTIMEO/SO_SNDTIMEO instead
    if (fcntl(fd, F_SETFL, flags) < 0) {
        return -1;
    }
    return 0;
}

static z_result_t _z_tcp_esp32_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_fd = socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol);
    if (sock->_fd != -1) {
        struct timeval rcv_tv;
        _z_tcp_esp32_ms_to_timeval(tout, &rcv_tv);
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&rcv_tv, sizeof(rcv_tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        struct timeval snd_tv;
        _z_tcp_esp32_ms_to_timeval(_Z_TCP_ESP32_SEND_TIMEOUT_MS, &snd_tv);
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&snd_tv, sizeof(snd_tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        int optflag = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&optflag, sizeof(optflag)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

#if Z_FEATURE_TCP_NODELAY == 1
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&optflag, sizeof(optflag)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#endif

#if LWIP_SO_LINGER == 1
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#endif

        for (struct addrinfo *it = endpoint._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (_z_tcp_esp32_connect_tout(sock->_fd, it->ai_addr, it->ai_addrlen, tout) < 0)) {
                if (it->ai_next == NULL) {
                    _Z_ERROR_LOG(_Z_ERR_GENERIC);
                    ret = _Z_ERR_GENERIC;
                    break;
                }
            } else {
                break;
            }
        }

        if (ret != _Z_RES_OK) {
            close(sock->_fd);
            sock->_fd = -1;
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_tcp_esp32_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    z_result_t ret = _Z_RES_OK;

    sock->_fd = socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol);
    if (sock->_fd == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

#if Z_FEATURE_TCP_NODELAY == 1
    int optflag = 1;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&optflag, sizeof(optflag)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }
#endif
    if (ret != _Z_RES_OK) {
        close(sock->_fd);
        sock->_fd = -1;
        return ret;
    }

    for (struct addrinfo *it = endpoint._iptcp; it != NULL; it = it->ai_next) {
        if (bind(sock->_fd, it->ai_addr, it->ai_addrlen) < 0) {
            if (it->ai_next == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
                break;
            }
        }
        if (listen(sock->_fd, Z_LISTEN_MAX_CONNECTION_NB) < 0) {
            if (it->ai_next == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
                break;
            }
        }
    }

    if (ret != _Z_RES_OK) {
        close(sock->_fd);
        sock->_fd = -1;
    }

    return ret;
}

static z_result_t _z_tcp_esp32_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct sockaddr naddr;
    socklen_t nlen = sizeof(naddr);
    sock_out->_fd = -1;
    int con_socket = accept(sock_in->_fd, &naddr, &nlen);
    if (con_socket < 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    struct timeval rcv_tv;
    _z_tcp_esp32_ms_to_timeval(Z_CONFIG_SOCKET_TIMEOUT, &rcv_tv);
    if (setsockopt(con_socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&rcv_tv, sizeof(rcv_tv)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    struct timeval snd_tv;
    _z_tcp_esp32_ms_to_timeval(_Z_TCP_ESP32_SEND_TIMEOUT_MS, &snd_tv);
    if (setsockopt(con_socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&snd_tv, sizeof(snd_tv)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    int optflag = 1;
    if (setsockopt(con_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&optflag, sizeof(optflag)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#if Z_FEATURE_TCP_NODELAY == 1
    if (setsockopt(con_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&optflag, sizeof(optflag)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#endif
#if LWIP_SO_LINGER == 1
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#endif

    sock_out->_fd = con_socket;
    return _Z_RES_OK;
}

static void _z_tcp_esp32_close(_z_sys_net_socket_t *sock) {
    if (sock->_fd >= 0) {
        shutdown(sock->_fd, SHUT_RDWR);
        close(sock->_fd);
        sock->_fd = -1;
    }
}

static size_t _z_tcp_esp32_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(sock._fd, ptr, len, 0);
    if (rb < (ssize_t)0) {
        return SIZE_MAX;
    }

    return (size_t)rb;
}

static size_t _z_tcp_esp32_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_tcp_esp32_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_tcp_esp32_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    ssize_t wb = send(sock._fd, ptr, len, 0);
    if (wb < (ssize_t)0) {
        return SIZE_MAX;
    }

    return (size_t)wb;
}

z_result_t _z_tcp_endpoint_init(_z_sys_net_endpoint_t *ep, const char *address, const char *port) {
    return _z_tcp_esp32_endpoint_init(ep, address, port);
}

void _z_tcp_endpoint_clear(_z_sys_net_endpoint_t *ep) { _z_tcp_esp32_endpoint_clear(ep); }

z_result_t _z_tcp_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    return _z_tcp_esp32_open(sock, endpoint, tout);
}

z_result_t _z_tcp_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    return _z_tcp_esp32_listen(sock, endpoint);
}

z_result_t _z_tcp_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    return _z_tcp_esp32_accept(sock_in, sock_out);
}

void _z_tcp_close(_z_sys_net_socket_t *sock) { _z_tcp_esp32_close(sock); }

size_t _z_tcp_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) { return _z_tcp_esp32_read(sock, ptr, len); }

size_t _z_tcp_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_tcp_esp32_read_exact(sock, ptr, len);
}

size_t _z_tcp_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return _z_tcp_esp32_write(sock, ptr, len);
}

#endif /* defined(ZP_PLATFORM_SOCKET_ESP32) */
