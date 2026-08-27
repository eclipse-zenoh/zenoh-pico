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
#if defined(CONFIG_NET_SOCKETS)
#include <netdb.h>
#endif
#include <stdbool.h>
#include <stddef.h>
#if defined(CONFIG_NET_SOCKETS)
#include <sys/socket.h>
#endif
#include <unistd.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/select.h>

#include "zenoh-pico/collections/string.h"

// After the zenoh includes, so Z_FEATURE_LINK_CAN is defined.
#if Z_FEATURE_LINK_CAN == 1
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#endif
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
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    } else if (result == 0) {
        return _Z_NO_DATA_PROCESSED;
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
#error "Bluetooth not supported yet on Zephyr port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on Zephyr port of Zenoh-Pico"
#endif

#endif /* defined(ZENOH_ZEPHYR) */

#if Z_FEATURE_LINK_CAN == 1

// Per-link receive queues, and per-device start refcounts.
//
// Both are pools rather than single globals because several zenoh sessions can
// share one CAN controller. A shared queue would let one link dequeue and drop
// a frame belonging to another; a shared start/stop would let one link's close
// stop the controller under the others.
#ifndef Z_CAN_RX_QUEUE_DEPTH
#define Z_CAN_RX_QUEUE_DEPTH 16
#endif

// Links per image. Each costs a queue of Z_CAN_RX_QUEUE_DEPTH frames.
#ifndef Z_CAN_MAX_LINKS
#define Z_CAN_MAX_LINKS 2
#endif

static struct k_msgq _z_can_msgq_pool[Z_CAN_MAX_LINKS];
static char _z_can_msgq_buf[Z_CAN_MAX_LINKS][Z_CAN_RX_QUEUE_DEPTH * sizeof(struct can_frame)];
static bool _z_can_msgq_taken[Z_CAN_MAX_LINKS];

// Devices this image has started, with a refcount each.
static const struct device *_z_can_started_dev[Z_CAN_MAX_LINKS];
static uint8_t _z_can_started_ref[Z_CAN_MAX_LINKS];

// One mutex guards both tables. Open and close are rare and off the data path,
// so a single lock costs nothing and avoids a torn refcount when two sessions
// come up concurrently.
K_MUTEX_DEFINE(_z_can_pool_mutex);

static struct k_msgq *__z_can_msgq_take(void) {
    struct k_msgq *out = NULL;
    k_mutex_lock(&_z_can_pool_mutex, K_FOREVER);
    for (size_t i = 0; i < Z_CAN_MAX_LINKS; i++) {
        if (!_z_can_msgq_taken[i]) {
            _z_can_msgq_taken[i] = true;
            k_msgq_init(&_z_can_msgq_pool[i], _z_can_msgq_buf[i], sizeof(struct can_frame), Z_CAN_RX_QUEUE_DEPTH);
            out = &_z_can_msgq_pool[i];
            break;
        }
    }
    k_mutex_unlock(&_z_can_pool_mutex);
    if (out == NULL) {
        _Z_ERROR("CAN: no free receive queue (Z_CAN_MAX_LINKS=%d)", (int)Z_CAN_MAX_LINKS);
    }
    return out;
}

static void __z_can_msgq_give(struct k_msgq *q) {
    if (q == NULL) {
        return;
    }
    k_mutex_lock(&_z_can_pool_mutex, K_FOREVER);
    for (size_t i = 0; i < Z_CAN_MAX_LINKS; i++) {
        if (&_z_can_msgq_pool[i] == q) {
            k_msgq_purge(q);
            _z_can_msgq_taken[i] = false;
            break;
        }
    }
    k_mutex_unlock(&_z_can_pool_mutex);
}

// Start the controller on the first link to claim it; later links just take a
// reference. Returns false if it could not be started.
static bool __z_can_dev_acquire(const struct device *dev) {
    bool ok = true;
    k_mutex_lock(&_z_can_pool_mutex, K_FOREVER);
    for (size_t i = 0; i < Z_CAN_MAX_LINKS; i++) {
        if (_z_can_started_dev[i] == dev) {
            _z_can_started_ref[i]++;
            k_mutex_unlock(&_z_can_pool_mutex);
            return true;  // already running, do not restart it under the others
        }
    }
    for (size_t i = 0; i < Z_CAN_MAX_LINKS; i++) {
        if (_z_can_started_dev[i] == NULL) {
            if (can_start(dev) < 0) {
                ok = false;
            } else {
                _z_can_started_dev[i] = dev;
                _z_can_started_ref[i] = 1;
            }
            k_mutex_unlock(&_z_can_pool_mutex);
            return ok;
        }
    }
    k_mutex_unlock(&_z_can_pool_mutex);
    _Z_ERROR("CAN: no free device slot (Z_CAN_MAX_LINKS=%d)", (int)Z_CAN_MAX_LINKS);
    return false;
}

// Stop only when the last link on this device goes away.
static void __z_can_dev_release(const struct device *dev) {
    k_mutex_lock(&_z_can_pool_mutex, K_FOREVER);
    for (size_t i = 0; i < Z_CAN_MAX_LINKS; i++) {
        if (_z_can_started_dev[i] == dev) {
            if (_z_can_started_ref[i] > 0) {
                _z_can_started_ref[i]--;
            }
            if (_z_can_started_ref[i] == 0) {
                (void)can_stop(dev);
                _z_can_started_dev[i] = NULL;
            }
            break;
        }
    }
    k_mutex_unlock(&_z_can_pool_mutex);
}

// True when some link has already started this controller, in which case its
// mode and bit rates must not be touched.
static bool __z_can_dev_in_use(const struct device *dev) {
    bool in_use = false;
    k_mutex_lock(&_z_can_pool_mutex, K_FOREVER);
    for (size_t i = 0; i < Z_CAN_MAX_LINKS; i++) {
        if ((_z_can_started_dev[i] == dev) && (_z_can_started_ref[i] > 0)) {
            in_use = true;
            break;
        }
    }
    k_mutex_unlock(&_z_can_pool_mutex);
    return in_use;
}

static z_result_t __z_can_setup(_z_can_socket_t *sock, const char *dev_name, uint32_t bitrate, uint32_t dbitrate,
                                uint32_t id, uint32_t match, uint32_t mask) {
    const struct device *dev = device_get_binding(dev_name);
    if (dev == NULL) {
        _Z_ERROR("CAN: no device '%s'", dev_name);
        return _Z_ERR_GENERIC;
    }
    if (!device_is_ready(dev)) {
        _Z_ERROR("CAN: device '%s' not ready", dev_name);
        return _Z_ERR_GENERIC;
    }

    bool shared = __z_can_dev_in_use(dev);
    _Bool fd_mode = false;

    if (shared) {
        // Another link already configured and started this controller. Read the
        // mode back rather than reconfiguring, which would disrupt it.
#ifdef CONFIG_CAN_FD_MODE
        can_mode_t mode = 0;
        if (can_get_capabilities(dev, &mode) == 0) {
            fd_mode = ((mode & CAN_MODE_FD) != 0);
        }
#endif
        _Z_DEBUG("CAN: '%s' already in use, inheriting its configuration", dev_name);
    } else {
        // A controller already started cannot be reconfigured; stopping first is
        // harmless if it was never started.
        (void)can_stop(dev);

        // CONFIG_CAN_FD_MODE gates more than performance: without it
        // `z_impl_can_set_bitrate_data` is not compiled at all, and
        // `struct can_frame.data` is 8 bytes rather than 64. Everything FD has
        // to be behind this, not behind a runtime flag.
#ifdef CONFIG_CAN_FD_MODE
        if (dbitrate != 0u) {
            can_mode_t caps = 0;
            if ((can_get_capabilities(dev, &caps) == 0) && ((caps & CAN_MODE_FD) != 0)) {
                if (can_set_mode(dev, CAN_MODE_FD) == 0) {
                    fd_mode = true;
                }
            }
            if (!fd_mode) {
                _Z_DEBUG("CAN: '%s' has no CAN FD, using classic frames", dev_name);
            }
        }
#else
        if (dbitrate != 0u) {
            _Z_DEBUG("CAN: built without CONFIG_CAN_FD_MODE, using classic frames");
        }
#endif

        // Bit rates may be fixed by devicetree. Honour the contract in
        // system/link/can.h: a rate we cannot apply is not a failure.
        if (bitrate != 0u) {
            (void)can_set_bitrate(dev, bitrate);
        }
#ifdef CONFIG_CAN_FD_MODE
        if (fd_mode && (dbitrate != 0u)) {
            (void)can_set_bitrate_data(dev, dbitrate);
        }
#endif
    }

    struct k_msgq *rx = __z_can_msgq_take();
    if (rx == NULL) {
        return _Z_ERR_GENERIC;
    }

    // Admit the whole zenoh band; the read drops our own frames. A mask of 0
    // matches every identifier, which is the default for a dedicated bus.
    const struct can_filter filter = {
        .id = match,
        .mask = mask,
        .flags = 0,
    };
    int filter_id = can_add_rx_filter_msgq(dev, rx, &filter);
    if (filter_id < 0) {
        _Z_ERROR("CAN: could not add rx filter (match 0x%x mask 0x%x)", (unsigned)match, (unsigned)mask);
        __z_can_msgq_give(rx);
        return _Z_ERR_GENERIC;
    }

    if (!__z_can_dev_acquire(dev)) {
        _Z_ERROR("CAN: could not start '%s'", dev_name);
        can_remove_rx_filter(dev, filter_id);
        __z_can_msgq_give(rx);
        return _Z_ERR_GENERIC;
    }

    sock->_sock._can_dev = dev;
    sock->_sock._can_rx_msgq = rx;
    sock->_sock._can_filter_id = filter_id;
    sock->_id = id;
    sock->_match = match;
    sock->_mask = mask;
    sock->_fd_mode = fd_mode;
    sock->_mtu = fd_mode ? _Z_CAN_FD_MTU_SIZE : _Z_CAN_CLASSIC_MTU_SIZE;
    return _Z_RES_OK;
}

z_result_t _z_open_can(_z_can_socket_t *sock, const char *dev, uint32_t bitrate, uint32_t dbitrate, uint32_t id,
                       uint32_t match, uint32_t mask) {
    return __z_can_setup(sock, dev, bitrate, dbitrate, id, match, mask);
}

z_result_t _z_listen_can(_z_can_socket_t *sock, const char *dev, uint32_t bitrate, uint32_t dbitrate, uint32_t id,
                         uint32_t match, uint32_t mask) {
    // Multicast peers all listen; a bus has no connection setup.
    return __z_can_setup(sock, dev, bitrate, dbitrate, id, match, mask);
}

void _z_close_can(_z_can_socket_t *sock) {
    const struct device *dev = (const struct device *)sock->_sock._can_dev;
    if (dev == NULL) {
        return;
    }
    // Give back the controller's filter slot; there are only a handful, and
    // leaking one per open/close cycle exhausts them.
    if (sock->_sock._can_filter_id >= 0) {
        can_remove_rx_filter(dev, sock->_sock._can_filter_id);
        sock->_sock._can_filter_id = -1;
    }
    __z_can_msgq_give(sock->_sock._can_rx_msgq);
    sock->_sock._can_rx_msgq = NULL;
    // Stops the controller only if this was the last link using it.
    __z_can_dev_release(dev);
    sock->_sock._can_dev = NULL;
}

size_t _z_send_can(const _z_can_socket_t *sock, const uint8_t *ptr, size_t len) {
    if (len > sock->_mtu) {
        _Z_ERROR("CAN: datagram %zu exceeds link MTU %u", len, (unsigned)sock->_mtu);
        return SIZE_MAX;
    }

    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.id = sock->_id;
    // Byte 0 is the true length; the DLC may describe a longer frame because
    // CAN FD lengths are quantised (system/link/can.h).
    frame.data[0] = (uint8_t)len;
    if (len > 0) {
        memcpy(&frame.data[1], ptr, len);
    }
    frame.dlc = can_bytes_to_dlc((uint8_t)(len + _Z_CAN_LEN_PREFIX));
#ifdef CONFIG_CAN_FD_MODE
    if (sock->_fd_mode) {
        frame.flags = CAN_FRAME_FDF | CAN_FRAME_BRS;
    }
#endif

    if (can_send((const struct device *)sock->_sock._can_dev, &frame, K_MSEC(100), NULL, NULL) != 0) {
        return SIZE_MAX;
    }
    return len;
}

size_t _z_read_can(const _z_can_socket_t *sock, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    struct can_frame frame;

    // Loop until a frame arrives that is not ours, mirroring
    // `_z_read_udp_multicast`: on a bus every peer hears everything.
    for (;;) {
        if (k_msgq_get(sock->_sock._can_rx_msgq, &frame, K_FOREVER) != 0) {
            return SIZE_MAX;
        }

        uint32_t sender = frame.id & CAN_EXT_ID_MASK;
        if (sender == sock->_id) {
            continue;  // our own frame
        }
        if ((sock->_mask != 0u) && ((sender & sock->_mask) != sock->_match)) {
            continue;  // outside the zenoh band
        }

        uint8_t frame_len = can_dlc_to_bytes(frame.dlc);
        if (frame_len < _Z_CAN_LEN_PREFIX) {
            continue;
        }

        size_t dlen = frame.data[0];
        if ((dlen > (size_t)(frame_len - _Z_CAN_LEN_PREFIX)) || (dlen > len)) {
            // Length byte disagrees with the frame, or the caller's buffer is
            // too small. Drop rather than deliver a truncated datagram.
            _Z_ERROR("CAN: bad datagram length %zu (frame %u, buffer %zu)", dlen, (unsigned)frame_len, len);
            continue;
        }

        if (addr != NULL) {
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
