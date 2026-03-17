#include "zenoh-pico/link/backend/stream.h"

#if defined(ZENOH_LINUX) || defined(ZENOH_MACOS) || defined(ZENOH_BSD)

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static z_result_t _z_tcp_posix_endpoint_init(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {
    z_result_t ret = _Z_RES_OK;

    struct addrinfo hints;
    (void)memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(s_address, s_port, &hints, &ep->_iptcp) < 0) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

static void _z_tcp_posix_endpoint_clear(_z_sys_net_endpoint_t *ep) { freeaddrinfo(ep->_iptcp); }

static z_result_t _z_tcp_posix_open(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    z_result_t ret = _Z_RES_OK;

    sock->_fd = socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol);
    if (sock->_fd != -1) {
        z_time_t tv;
        tv.tv_sec = (time_t)(tout / (uint32_t)1000);
        tv.tv_usec = (suseconds_t)((tout % (uint32_t)1000) * (uint32_t)1000);
        if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

        int flags = 1;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#if Z_FEATURE_TCP_NODELAY == 1
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }
#endif
        struct linger ling;
        ling.l_onoff = 1;
        ling.l_linger = Z_TRANSPORT_LEASE / 1000;
        if ((ret == _Z_RES_OK) &&
            (setsockopt(sock->_fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
            _Z_ERROR_LOG(_Z_ERR_GENERIC);
            ret = _Z_ERR_GENERIC;
        }

#if defined(ZENOH_MACOS) || defined(ZENOH_BSD)
        int nosigpipe_val = 1;
        setsockopt(sock->_fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&nosigpipe_val, sizeof(int));
#endif
        for (struct addrinfo *it = endpoint._iptcp; it != NULL; it = it->ai_next) {
            if ((ret == _Z_RES_OK) && (connect(sock->_fd, it->ai_addr, it->ai_addrlen) < 0)) {
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

static z_result_t _z_tcp_posix_listen(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint) {
    z_result_t ret = _Z_RES_OK;

    sock->_fd = socket(endpoint._iptcp->ai_family, endpoint._iptcp->ai_socktype, endpoint._iptcp->ai_protocol);
    if (sock->_fd == -1) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    int value = true;
    if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }
    int flags = 1;
    if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }
#if Z_FEATURE_TCP_NODELAY == 1
    if ((ret == _Z_RES_OK) && (setsockopt(sock->_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }
#endif
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = Z_TRANSPORT_LEASE / 1000;
    if ((ret == _Z_RES_OK) &&
        (setsockopt(sock->_fd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(struct linger)) < 0)) {
        _Z_ERROR_LOG(_Z_ERR_GENERIC);
        ret = _Z_ERR_GENERIC;
    }
#if defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    int nosigpipe_val = 1;
    setsockopt(sock->_fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&nosigpipe_val, sizeof(int));
#endif
    if (ret != _Z_RES_OK) {
        close(sock->_fd);
        return ret;
    }

    int addr_count = 0;
    for (struct addrinfo *it = endpoint._iptcp; it != NULL; it = it->ai_next) {
        addr_count++;
        char addr_str[INET6_ADDRSTRLEN];
        const char *family_str = (it->ai_family == AF_INET) ? "IPv4" : (it->ai_family == AF_INET6) ? "IPv6" : "Unknown";

        if (it->ai_family == AF_INET) {
            inet_ntop(AF_INET, &((struct sockaddr_in *)it->ai_addr)->sin_addr, addr_str, INET_ADDRSTRLEN);
        } else if (it->ai_family == AF_INET6) {
            inet_ntop(AF_INET6, &((struct sockaddr_in6 *)it->ai_addr)->sin6_addr, addr_str, INET6_ADDRSTRLEN);
        } else {
            snprintf(addr_str, sizeof(addr_str), "%s", "unknown");
        }

        _Z_DEBUG("Trying address %d: %s (%s), family=%d", addr_count, addr_str, family_str, it->ai_family);
        if (bind(sock->_fd, it->ai_addr, it->ai_addrlen) < 0) {
            _Z_DEBUG("bind() failed for address %s: %s", addr_str, strerror(errno));
            if (it->ai_next == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
                break;
            }
            continue;
        }
        _Z_DEBUG("bind() successful for address %s", addr_str);

        if (listen(sock->_fd, Z_LISTEN_MAX_CONNECTION_NB) < 0) {
            _Z_DEBUG("listen() failed for address %s: %s", addr_str, strerror(errno));
            if (it->ai_next == NULL) {
                _Z_ERROR_LOG(_Z_ERR_GENERIC);
                ret = _Z_ERR_GENERIC;
                break;
            }
            continue;
        }
        _Z_DEBUG("listen() successful for address %s", addr_str);
        break;
    }
    if (ret != _Z_RES_OK) {
        close(sock->_fd);
        sock->_fd = -1;
    }
    return ret;
}

static void _z_tcp_posix_close(_z_sys_net_socket_t *sock) {
    if (sock->_fd >= 0) {
        shutdown(sock->_fd, SHUT_RDWR);
        close(sock->_fd);
        sock->_fd = -1;
    }
}

static size_t _z_tcp_posix_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    ssize_t rb = recv(sock._fd, ptr, len, 0);
    if (rb < (ssize_t)0) {
        if (errno != EAGAIN) {
            _Z_DEBUG("Errno: %d\n", errno);
        }
        return SIZE_MAX;
    }

    return (size_t)rb;
}

static size_t _z_tcp_posix_read_exact(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;
    uint8_t *pos = &ptr[0];

    do {
        size_t rb = _z_tcp_posix_read(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n += rb;
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t)n);
    } while (n != len);

    return n;
}

static size_t _z_tcp_posix_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
#if defined(ZENOH_LINUX)
    return (size_t)send(sock._fd, ptr, len, MSG_NOSIGNAL);
#else
    return (size_t)send(sock._fd, ptr, len, 0);
#endif
}

const _z_stream_ops_t _z_tcp_posix_stream_ops = {
    .endpoint_init = _z_tcp_posix_endpoint_init,
    .endpoint_clear = _z_tcp_posix_endpoint_clear,
    .open = _z_tcp_posix_open,
    .listen = _z_tcp_posix_listen,
    .close = _z_tcp_posix_close,
    .read = _z_tcp_posix_read,
    .read_exact = _z_tcp_posix_read_exact,
    .write = _z_tcp_posix_write,
};

#endif /* defined(ZENOH_LINUX) || defined(ZENOH_MACOS) || defined(ZENOH_BSD) */
