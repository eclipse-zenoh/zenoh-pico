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

// THE POINT OF THIS FILE IS THAT IT DUPLICATES network.c.
//
// `unix` implements the ISO-TP link twice: once on the kernel's CAN_ISOTP
// socket (network.c) and once on the vendored `third_party/isotp-c`. That is
// deliberate duplication, and it is a TESTING ORACLE. Linux is the only
// platform where both exist, so it is the only place the vendored library can
// be judged against a reference implementation -- and every other platform
// this link will reach (FreeRTOS, ThreadX, NuttX, bare metal) has only the
// vendored one. A conformance bug found here is found on a laptop instead of
// on a board.
//
// Keep both. The day the vendored library regresses, the oracle is what finds
// it.

#include "zenoh-pico/config.h"

#if Z_FEATURE_LINK_ISOTP == 1 && Z_FEATURE_LINK_ISOTP_VENDORED == 1

#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "isotp.h"
#include "zenoh-pico/link/transport/isotp.h"
#include "zenoh-pico/utils/logging.h"

// The library holds no state of its own beyond the IsoTpLink the caller gives
// it, and the read hook is handed only a descriptor -- so the descriptor has to
// be enough to find everything else. A small fixed table is the whole
// mechanism; there is no allocator here for the same reason there is none in
// the library.
#ifndef _Z_ISOTP_VENDORED_MAX_LINKS
#define _Z_ISOTP_VENDORED_MAX_LINKS 4
#endif

typedef struct {
    _Bool _in_use;
    int _fd;
    _Bool _eff;
    uint32_t _tx_id;
    uint32_t _rx_id;
    IsoTpLink _link;
    pthread_mutex_t _lock;
    uint8_t _sendbuf[_Z_ISOTP_MTU_SIZE];
    uint8_t _recvbuf[_Z_ISOTP_MTU_SIZE];
} _z_isotp_slot_t;

static _z_isotp_slot_t _z_isotp_slots[_Z_ISOTP_VENDORED_MAX_LINKS];
static pthread_mutex_t _z_isotp_slots_lock = PTHREAD_MUTEX_INITIALIZER;

static _z_isotp_slot_t *__z_slot_for_fd(int fd) {
    for (size_t i = 0; i < _Z_ISOTP_VENDORED_MAX_LINKS; i++) {
        if (_z_isotp_slots[i]._in_use && (_z_isotp_slots[i]._fd == fd)) {
            return &_z_isotp_slots[i];
        }
    }
    return NULL;
}

// ---------------------------------------------------------------- user hooks

void isotp_user_debug(const char *message, ...) { _Z_DEBUG("ISO-TP(vendored): %s", message); }

uint32_t isotp_user_get_us(void) {
    struct timespec ts;
    // CLOCK_MONOTONIC, not the wall clock: STmin and the N_* timers must not
    // move when someone steps the system time.
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000000u + (uint64_t)(ts.tv_nsec / 1000));
}

int isotp_user_send_can(const uint32_t arbitration_id, const uint8_t *data, const uint8_t size, void *arg) {
    _z_isotp_slot_t *slot = (_z_isotp_slot_t *)arg;
    if ((slot == NULL) || !slot->_in_use) {
        return ISOTP_RET_ERROR;
    }

    struct can_frame frame;
    memset(&frame, 0, sizeof(frame));
    frame.can_id = slot->_eff ? (arbitration_id | CAN_EFF_FLAG) : arbitration_id;
    frame.can_dlc = size;
    memcpy(frame.data, data, size);

    ssize_t wb = write(slot->_fd, &frame, sizeof(frame));
    if (wb == (ssize_t)sizeof(frame)) {
        return ISOTP_RET_OK;
    }
    // ENOBUFS is the socket's transmit queue being full, which is a "try again"
    // and not a failure. The library retries on ISOTP_RET_NOSPACE.
    if ((wb < 0) && ((errno == ENOBUFS) || (errno == EAGAIN))) {
        return ISOTP_RET_NOSPACE;
    }
    _Z_ERROR("ISO-TP(vendored): write of frame 0x%x failed: %d", (unsigned)arbitration_id, errno);
    return ISOTP_RET_ERROR;
}

// ------------------------------------------------------------------ plumbing

// Drain whatever the socket has and hand it to the library. The caller holds
// no lock; this takes the slot's.
static void __z_pump_rx(_z_isotp_slot_t *slot) {
    for (;;) {
        struct can_frame frame;
        ssize_t rb = recv(slot->_fd, &frame, sizeof(frame), MSG_DONTWAIT);
        if (rb != (ssize_t)sizeof(frame)) {
            break;
        }
        pthread_mutex_lock(&slot->_lock);
        isotp_on_can_message(&slot->_link, frame.data, frame.can_dlc);
        pthread_mutex_unlock(&slot->_lock);
    }
    pthread_mutex_lock(&slot->_lock);
    isotp_poll(&slot->_link);
    pthread_mutex_unlock(&slot->_lock);
}

// ------------------------------------------------------------------ the link

z_result_t _z_open_isotp(_z_isotp_socket_t *sock, const char *dev, uint32_t tx_id, uint32_t rx_id, _Bool eff,
                         uint8_t stmin, uint8_t bs) {
    // The vendored library takes its flow control values from compile-time
    // constants (ISO_TP_DEFAULT_BLOCK_SIZE, ISO_TP_DEFAULT_ST_MIN_US) rather
    // than per link, so it cannot honour these. Refusing beats accepting the
    // endpoint and pacing nothing: a peer that asked to be protected from a
    // burst would not find out until it dropped frames on a real bus.
    if ((stmin != 0u) || (bs != 0u)) {
        _Z_ERROR("ISO-TP(vendored): stmin and bs need the kernel backend; this build sets them at compile time");
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    pthread_mutex_lock(&_z_isotp_slots_lock);
    _z_isotp_slot_t *slot = NULL;
    for (size_t i = 0; i < _Z_ISOTP_VENDORED_MAX_LINKS; i++) {
        if (!_z_isotp_slots[i]._in_use) {
            slot = &_z_isotp_slots[i];
            break;
        }
    }
    if (slot == NULL) {
        pthread_mutex_unlock(&_z_isotp_slots_lock);
        _Z_ERROR("ISO-TP(vendored): no free link slot (max %d)", _Z_ISOTP_VENDORED_MAX_LINKS);
        return _Z_ERR_GENERIC;
    }
    slot->_in_use = true;  // claim it before releasing, or two opens race for it
    pthread_mutex_unlock(&_z_isotp_slots_lock);

    // A RAW socket, not CAN_ISOTP: the whole point is that the protocol runs in
    // userspace here, exactly as it will on a platform with no kernel support.
    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        slot->_in_use = false;
        _Z_ERROR("ISO-TP(vendored): socket(PF_CAN, CAN_RAW) failed: %d", errno);
        return _Z_ERR_GENERIC;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    (void)strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name) - 1);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        _Z_ERROR("ISO-TP(vendored): no such interface '%s': %d", dev, errno);
        close(fd);
        slot->_in_use = false;
        return _Z_ERR_GENERIC;
    }

    // Only our own receive identifier. Without this the library would be fed
    // every frame on the bus and would reassemble noise.
    struct can_filter filter;
    filter.can_id = eff ? (rx_id | CAN_EFF_FLAG) : rx_id;
    filter.can_mask = eff ? (CAN_EFF_MASK | CAN_EFF_FLAG) : CAN_SFF_MASK;
    if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
        _Z_ERROR("ISO-TP(vendored): CAN_RAW_FILTER failed: %d", errno);
        close(fd);
        slot->_in_use = false;
        return _Z_ERR_GENERIC;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        _Z_ERROR("ISO-TP(vendored): bind('%s') failed: %d", dev, errno);
        close(fd);
        slot->_in_use = false;
        return _Z_ERR_GENERIC;
    }

    slot->_fd = fd;
    slot->_eff = eff;
    slot->_tx_id = tx_id;
    slot->_rx_id = rx_id;
    (void)pthread_mutex_init(&slot->_lock, NULL);
    isotp_init_link(&slot->_link, eff ? (tx_id | CAN_EFF_FLAG) : tx_id, slot->_sendbuf, sizeof(slot->_sendbuf),
                    slot->_recvbuf, sizeof(slot->_recvbuf));
    slot->_link.user_send_can_arg = slot;

    sock->_sock._fd = fd;
    sock->_tx_id = tx_id;
    sock->_rx_id = rx_id;
    sock->_eff = eff;
    return _Z_RES_OK;
}

void _z_close_isotp(_z_isotp_socket_t *sock) {
    if (sock->_sock._fd < 0) {
        return;
    }
    _z_isotp_slot_t *slot = __z_slot_for_fd(sock->_sock._fd);
    if (slot != NULL) {
        pthread_mutex_lock(&slot->_lock);
        isotp_destroy_link(&slot->_link);
        pthread_mutex_unlock(&slot->_lock);
        (void)pthread_mutex_destroy(&slot->_lock);
        slot->_in_use = false;
    }
    close(sock->_sock._fd);
    sock->_sock._fd = -1;
}

size_t _z_read_isotp_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    _z_isotp_slot_t *slot = __z_slot_for_fd(socket._fd);
    if (slot == NULL) {
        _Z_ERROR("ISO-TP(vendored): read on a descriptor with no link");
        return SIZE_MAX;
    }

    for (;;) {
        pthread_mutex_lock(&slot->_lock);
        uint32_t out = 0;
        int ret = isotp_receive(&slot->_link, ptr, (uint32_t)len, &out);
        pthread_mutex_unlock(&slot->_lock);
        if (ret == ISOTP_RET_OK) {
            return (size_t)out;
        }
        if ((ret != ISOTP_RET_NO_DATA) && (ret != ISOTP_RET_INPROGRESS)) {
            _Z_ERROR("ISO-TP(vendored): receive failed: %d", ret);
            return SIZE_MAX;
        }

        // Poll OUTSIDE the lock. Holding it across the wait would block every
        // send for the whole timeout, and the send path is what answers the
        // peer's flow control.
        struct pollfd pfd = {.fd = slot->_fd, .events = POLLIN, .revents = 0};
        int pr = poll(&pfd, 1, 50);
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            _Z_ERROR("ISO-TP(vendored): poll failed: %d", errno);
            return SIZE_MAX;
        }
        __z_pump_rx(slot);  // also runs isotp_poll, so the timers advance on a timeout
    }
}

size_t _z_read_isotp(const _z_isotp_socket_t *sock, uint8_t *ptr, size_t len) {
    return _z_read_isotp_socket(sock->_sock, ptr, len);
}

size_t _z_send_isotp(const _z_isotp_socket_t *sock, const uint8_t *ptr, size_t len) {
    if (len > _Z_ISOTP_MTU_SIZE) {
        _Z_ERROR("ISO-TP(vendored): PDU %zu exceeds the 12-bit FF_DL limit of %d", len, _Z_ISOTP_MTU_SIZE);
        return SIZE_MAX;
    }
    _z_isotp_slot_t *slot = __z_slot_for_fd(sock->_sock._fd);
    if (slot == NULL) {
        _Z_ERROR("ISO-TP(vendored): send on a descriptor with no link");
        return SIZE_MAX;
    }

    pthread_mutex_lock(&slot->_lock);
    int ret = isotp_send(&slot->_link, ptr, (uint32_t)len);
    pthread_mutex_unlock(&slot->_lock);
    if (ret != ISOTP_RET_OK) {
        _Z_ERROR("ISO-TP(vendored): send of %zu bytes rejected: %d", len, ret);
        return SIZE_MAX;
    }

    // Drive it to completion. A multi-frame send does not finish until the peer
    // has sent flow control and we have clocked out every consecutive frame, and
    // that flow control arrives on OUR receive identifier -- so the send loop
    // has to pump the receive side too, or it waits forever for a frame nobody
    // is reading.
    for (;;) {
        pthread_mutex_lock(&slot->_lock);
        uint8_t status = slot->_link.send_status;
        pthread_mutex_unlock(&slot->_lock);

        if (status == ISOTP_SEND_STATUS_IDLE) {
            return len;
        }
        if (status == ISOTP_SEND_STATUS_ERROR) {
            _Z_ERROR("ISO-TP(vendored): send of %zu bytes failed in flight", len);
            return SIZE_MAX;
        }

        struct pollfd pfd = {.fd = slot->_fd, .events = POLLIN, .revents = 0};
        int pr = poll(&pfd, 1, 5);
        if ((pr < 0) && (errno != EINTR)) {
            _Z_ERROR("ISO-TP(vendored): poll during send failed: %d", errno);
            return SIZE_MAX;
        }
        __z_pump_rx(slot);
    }
}

#endif  // Z_FEATURE_LINK_ISOTP == 1 && Z_FEATURE_LINK_ISOTP_VENDORED == 1
