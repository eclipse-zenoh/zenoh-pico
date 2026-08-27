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

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

// After the zenoh includes, so the Z_FEATURE_LINK_*
// macros the guards below test are already defined.
#if Z_FEATURE_LINK_CAN == 1
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#endif
#if Z_FEATURE_LINK_CAN == 1
#include "zenoh-pico/link/transport/can.h"
#endif
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
    fd_set read_fds;
    int max_fd = 0;
    bool has_sockets = false;

    FD_ZERO(&read_fds);

    _z_socket_wait_iter_reset(iter);
    while (_z_socket_wait_iter_next(iter)) {
        const _z_sys_net_socket_t *sock = _z_socket_wait_iter_get_socket(iter);
        _z_socket_wait_iter_set_ready(iter, false);
        FD_SET(sock->_fd, &read_fds);
        if (sock->_fd > max_fd) {
            max_fd = sock->_fd;
        }
        has_sockets = true;
    }

    if (!has_sockets) {
        return _Z_RES_OK;
    }

    struct timeval timeout = {
        .tv_sec = (time_t)(timeout_ms / 1000U),
        .tv_usec = (suseconds_t)((timeout_ms % 1000U) * 1000U),
    };
    int result = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result < 0) {
        _Z_DEBUG("Errno: %d\n", errno);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    bool has_data = false;
    _z_socket_wait_iter_reset(iter);
    while (_z_socket_wait_iter_next(iter)) {
        const _z_sys_net_socket_t *sock = _z_socket_wait_iter_get_socket(iter);
        bool is_ready = FD_ISSET(sock->_fd, &read_fds);
        _z_socket_wait_iter_set_ready(iter, is_ready);
        has_data |= is_ready;
    }

    return has_data ? _Z_RES_OK : _Z_NO_DATA_PROCESSED;
}

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on Unix port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_CAN == 1

static z_result_t __z_can_bind(_z_can_socket_t *sock, const char *dev, uint32_t dbitrate, uint32_t id, uint32_t match,
                               uint32_t mask) {
    // Bit rates are set out of band on Linux (`ip link set can0 type can
    // bitrate ...`), and a virtual interface has none at all. Honour the
    // contract in system/link/can.h: do not fail on a rate we cannot apply.
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        _Z_ERROR("CAN: socket(PF_CAN) failed: %d", errno);
        return _Z_ERR_GENERIC;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    (void)strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name) - 1);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        _Z_ERROR("CAN: no such interface '%s': %d", dev, errno);
        close(fd);
        return _Z_ERR_GENERIC;
    }

    // Admit the whole band this bus segment uses for zenoh; the read then drops
    // our own frames. A mask of 0 matches everything, which is the default for
    // a bus carrying nothing else.
    struct can_filter filter;
    filter.can_id = match;
    filter.can_mask = mask;
    if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
        _Z_ERROR("CAN: CAN_RAW_FILTER failed: %d", errno);
        close(fd);
        return _Z_ERR_GENERIC;
    }

    // Ask for CAN FD. If the interface does not support it we fall back to
    // classic framing rather than failing, and report the mode we got so the
    // link can size its MTU from reality.
    _Bool fd_mode = false;
    if (dbitrate != 0u) {
        int enable = 1;
        if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable, sizeof(enable)) == 0) {
            fd_mode = true;
        } else {
            _Z_DEBUG("CAN: interface '%s' has no CAN FD, using classic frames", dev);
        }
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        _Z_ERROR("CAN: bind('%s') failed: %d", dev, errno);
        close(fd);
        return _Z_ERR_GENERIC;
    }

    sock->_sock._fd = fd;
    sock->_id = id;
    sock->_match = match;
    sock->_mask = mask;
    sock->_fd_mode = fd_mode;
    sock->_mtu = fd_mode ? _Z_CAN_FD_MTU_SIZE : _Z_CAN_CLASSIC_MTU_SIZE;
    return _Z_RES_OK;
}

z_result_t _z_open_can(_z_can_socket_t *sock, const char *dev, uint32_t bitrate, uint32_t dbitrate, uint32_t id,
                       uint32_t match, uint32_t mask) {
    _ZP_UNUSED(bitrate);
    return __z_can_bind(sock, dev, dbitrate, id, match, mask);
}

z_result_t _z_listen_can(_z_can_socket_t *sock, const char *dev, uint32_t bitrate, uint32_t dbitrate, uint32_t id,
                         uint32_t match, uint32_t mask) {
    _ZP_UNUSED(bitrate);
    // Multicast peers all listen; a bus has no connection setup.
    return __z_can_bind(sock, dev, dbitrate, id, match, mask);
}

void _z_close_can(_z_can_socket_t *sock) {
    if (sock->_sock._fd >= 0) {
        close(sock->_sock._fd);
        sock->_sock._fd = -1;
    }
}

size_t _z_send_can(const _z_can_socket_t *sock, const uint8_t *ptr, size_t len) {
    if (len > sock->_mtu) {
        _Z_ERROR("CAN: datagram %zu exceeds link MTU %u", len, (unsigned)sock->_mtu);
        return SIZE_MAX;
    }

    struct canfd_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.can_id = sock->_id;
    // Byte 0 carries the true length; CAN FD DLCs are quantised so the frame
    // may be longer than the datagram (system/link/can.h).
    frame.data[0] = (uint8_t)len;
    if (len > 0) {
        memcpy(&frame.data[1], ptr, len);
    }

    size_t payload = len + _Z_CAN_LEN_PREFIX;
    size_t frame_len = payload;
    if (sock->_fd_mode && (payload > 8u)) {
        // Round up to the next representable CAN FD length.
        static const uint8_t steps[] = {12, 16, 20, 24, 32, 48, 64};
        for (size_t i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++) {
            if (payload <= steps[i]) {
                frame_len = steps[i];
                break;
            }
        }
        frame.flags = CANFD_BRS;  // use the fast data phase when configured
    } else if (sock->_fd_mode) {
        frame.flags = CANFD_BRS;
    }
    frame.len = (uint8_t)frame_len;

    size_t wire = sock->_fd_mode ? CANFD_MTU : CAN_MTU;
    ssize_t wb = write(sock->_sock._fd, &frame, wire);
    if (wb < 0 || (size_t)wb != wire) {
        _Z_ERROR("CAN: write failed: %d", errno);
        return SIZE_MAX;
    }

    return len;
}

size_t _z_read_can(const _z_can_socket_t *sock, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    struct canfd_frame frame;

    // Loop until a frame arrives that is not ours, mirroring
    // `_z_read_udp_multicast`: on a bus every peer hears everything, including
    // its own transmissions on a loopback-enabled interface.
    for (;;) {
        ssize_t rb = read(sock->_sock._fd, &frame, sizeof(frame));
        if (rb < 0) {
            if (errno == EINTR) {
                continue;
            }
            return SIZE_MAX;
        }
        if ((rb != (ssize_t)CANFD_MTU) && (rb != (ssize_t)CAN_MTU)) {
            continue;  // runt or error frame
        }

        uint32_t sender = frame.can_id & CAN_EFF_MASK;
        if (sender == sock->_id) {
            continue;  // our own frame
        }
        if ((sock->_mask != 0u) && ((sender & sock->_mask) != sock->_match)) {
            continue;  // outside the band this bus reserves for zenoh
        }
        if (frame.len < _Z_CAN_LEN_PREFIX) {
            continue;  // no length byte
        }

        size_t dlen = frame.data[0];
        if ((dlen > (size_t)(frame.len - _Z_CAN_LEN_PREFIX)) || (dlen > len)) {
            // Length byte disagrees with the frame, or the caller's buffer is
            // too small. Either way this datagram is unusable -- drop it rather
            // than hand back a truncated one that would deserialize as garbage.
            _Z_ERROR("CAN: bad datagram length %zu (frame %u, buffer %zu)", dlen, (unsigned)frame.len, len);
            continue;
        }

        if (addr != NULL) {
            assert(addr->len >= _Z_CAN_ADDR_SIZE);
            addr->len = _Z_CAN_ADDR_SIZE;
            uint8_t *dst = (uint8_t *)addr->start;
            dst[0] = (uint8_t)(sender & 0xFFu);
            dst[1] = (uint8_t)((sender >> 8) & 0xFFu);
            dst[2] = (uint8_t)((sender >> 16) & 0xFFu);
            dst[3] = (uint8_t)((sender >> 24) & 0xFFu);
        }

        if (dlen > 0) {
            memcpy(ptr, &frame.data[1], dlen);
        }
        return dlen;
    }
}

#endif  // Z_FEATURE_LINK_CAN == 1
