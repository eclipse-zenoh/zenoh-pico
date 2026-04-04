#include "zenoh-pico/link/backend/stream.h"

#if defined(ZP_PLATFORM_SOCKET_LWIP) && (Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1 || Z_FEATURE_LINK_WS == 1)

#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "zenoh-pico/link/backend/lwip_socket.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_tcp_lwip_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
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
    } else if (ep->_iptcp != NULL && ep->_iptcp->ai_addr != NULL) {
        ep->_iptcp->ai_addr->sa_family = ep->_iptcp->ai_family;
    }

    return ret;
}

static void _z_tcp_lwip_endpoint_clear(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

static z_result_t _z_tcp_lwip_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    _z_lwip_socket_set(sock,
                       socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol));
    if (_z_lwip_socket_get(*sock) != -1) {
        z_time_t tv;
        tv.tv_sec = tout / (uint32_t)1000;
        tv.tv_usec = (tout % (uint32_t)1000) * (uint32_t)1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        int flags = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#if Z_FEATURE_TCP_NODELAY == 1
        if ((ret == _Z_RES_OK) &&
            (setsockopt(_z_lwip_socket_get(*sock), IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#endif
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        for (struct addrinfo *it = endpoint._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(_z_lwip_socket_get(*sock), it->ai_addr, it->ai_addrlen) < 0)) {
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
            close(_z_lwip_socket_get(*sock));
            _z_lwip_socket_set(sock, -1);
        }
    } else {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static z_result_t _z_tcp_lwip_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    z_result_t ret = _Z_RES_OK;

    _z_lwip_socket_set(sock,
                       socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol));
    if (_z_lwip_socket_get(*sock) == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    int flags = 1;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }
#if Z_FEATURE_TCP_NODELAY == 1
    if ((ret == _Z_RES_OK) &&
        (setsockopt(_z_lwip_socket_get(*sock), IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }
#endif
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(_z_lwip_socket_get(*sock), SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    if (ret != _Z_RES_OK) {
        close(_z_lwip_socket_get(*sock));
        _z_lwip_socket_set(sock, -1);
        return ret;
    }

    for (struct addrinfo *it = endpoint._iptcp; it != NULL; it = it->ai_next) {
        if (bind(_z_lwip_socket_get(*sock), it->ai_addr, it->ai_addrlen) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
            break;
        }
        if (listen(_z_lwip_socket_get(*sock), Z_LISTEN_MAX_CONNECTION_NB) < 0) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
            break;
        }
    }

    if (ret != _Z_RES_OK) {
        close(_z_lwip_socket_get(*sock));
        _z_lwip_socket_set(sock, -1);
    }
    return ret;
}

static z_result_t _z_tcp_lwip_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    struct sockaddr naddr;
    socklen_t nlen = sizeof(naddr);
    _z_lwip_socket_set(sock_out, -1);
    int con_socket = lwip_accept(_z_lwip_socket_get(*sock_in), &naddr, &nlen);
    if (con_socket < 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    z_time_t tv;
    tv.tv_sec = Z_CONFIG_SOCKET_TIMEOUT / (uint32_t)1000;
    tv.tv_usec = (Z_CONFIG_SOCKET_TIMEOUT % (uint32_t)1000) * (uint32_t)1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0) {
        lwip_close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    int flags = 1;
    if (setsockopt(con_socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0) {
        lwip_close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#if Z_FEATURE_TCP_NODELAY == 1
    if (setsockopt(con_socket, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0) {
        lwip_close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
#endif
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if (setsockopt(con_socket, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0) {
        lwip_close(con_socket);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    _z_lwip_socket_set(sock_out, con_socket);
    return _Z_RES_OK;
}

static void _z_tcp_lwip_close(_z_sys_net_socket_t *sock) {
    if (_z_lwip_socket_get(*sock) >= 0) {
        shutdown(_z_lwip_socket_get(*sock), SHUT_RDWR);
        close(_z_lwip_socket_get(*sock));
        _z_lwip_socket_set(sock, -1);
    }
}

static size_t _z_tcp_lwip_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(_z_lwip_socket_get(sock), ptr, len, 0);
    if (rb < (ssize_t)0) {
        return SIZE_MAX;
    }

    return (size_t)rb;
}

static size_t _z_tcp_lwip_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_tcp_lwip_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n = n + rb;
        pos = _z_ptr_u8_offset(pos, rb);
    } while (n != len);

    return n;
}

static size_t _z_tcp_lwip_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return (size_t)send(_z_lwip_socket_get(sock), ptr, len, 0);
}

const _z_stream_ops_t _z_tcp_lwip_stream_ops = {
    .endpoint_init = _z_tcp_lwip_endpoint_init,
    .endpoint_clear = _z_tcp_lwip_endpoint_clear,
    .open = _z_tcp_lwip_open,
    .listen = _z_tcp_lwip_listen,
    .accept = _z_tcp_lwip_accept,
    .close = _z_tcp_lwip_close,
    .read = _z_tcp_lwip_read,
    .read_exact = _z_tcp_lwip_read_exact,
    .write = _z_tcp_lwip_write,
};

#endif /* defined(ZP_PLATFORM_SOCKET_LWIP) */
