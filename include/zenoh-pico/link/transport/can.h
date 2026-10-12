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

#ifndef ZENOH_PICO_SYSTEM_LINK_CAN_H
#define ZENOH_PICO_SYSTEM_LINK_CAN_H

#include <stdint.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LINK_CAN == 1

// One CAN frame carries one zenoh datagram.
//
// CAN FD payload lengths are quantised -- the DLC encodes 0..8, 12, 16, 20, 24,
// 32, 48, 64 and nothing between -- so a 40-byte datagram travels in a 48-byte
// frame and the receiver cannot recover the true length from the frame alone.
// Byte 0 of every payload is therefore the datagram length and bytes 1..N are
// the datagram, which costs one byte and keeps the link from having to parse
// zenoh's own headers to find a boundary.
#define _Z_CAN_LEN_PREFIX 1u

#define _Z_CAN_FD_FRAME_LEN 64u
#define _Z_CAN_CLASSIC_FRAME_LEN 8u

#define _Z_CAN_FD_MTU_SIZE (_Z_CAN_FD_FRAME_LEN - _Z_CAN_LEN_PREFIX)            // 63
#define _Z_CAN_CLASSIC_MTU_SIZE (_Z_CAN_CLASSIC_FRAME_LEN - _Z_CAN_LEN_PREFIX)  // 7

// Anything larger is fragmented by zenoh's transport before it reaches the
// link (`Z_FEATURE_FRAGMENTATION`, and `tx.c` clamping to `min(mtu, batch)`),
// so the link never sees a write it cannot place in one frame.
#define _Z_CAN_MTU_SIZE _Z_CAN_FD_MTU_SIZE

// A CAN bus is a broadcast medium: every node hears every frame and filters by
// identifier. That is a MULTICAST link, not a unicast one, and modelling it as
// unicast is the mistake to avoid here -- zenoh's unicast
// listen path goes through `_zp_unicast_accept_task`, which needs a socket and
// an accept() that no datagram medium has.
//
// So each peer owns one identifier, transmits on it, and accepts frames from
// every other identifier the mask admits. The sender's identifier is that
// peer's address, which is what the multicast transport compares in
// `_remote_addr` to tell peers apart. UDP multicast is the reference: its read
// loops until it sees a datagram that is not its own, then reports the sender.
typedef struct {
    _z_sys_net_socket_t _sock;
    uint32_t _id;     // this peer's identifier -- its address on the bus
    uint32_t _match;  // accept frames whose (id & _mask) == _match
    uint32_t _mask;   // 0 accepts every identifier
    uint16_t _mtu;    // _Z_CAN_FD_MTU_SIZE or _Z_CAN_CLASSIC_MTU_SIZE
    _Bool _fd_mode;
} _z_can_socket_t;

// The sender identifier reported through the `addr` slice. The multicast
// transport gives the link a 32-byte buffer (_Z_MULTICAST_ADDR_BUFF_SIZE), so
// four bytes is comfortable.
#define _Z_CAN_ADDR_SIZE 4u

/**
 * Open a CAN link.
 *
 * `dbitrate` of 0 selects classic CAN; any other value selects CAN FD with
 * that data-phase rate. On platforms whose interface is preconfigured (a
 * Linux `ip link set`, or a Zephyr devicetree `bitrate` property) the rate
 * arguments may be advisory -- the call must still succeed rather than fail
 * on a rate it cannot apply, and must report the mode it actually got in
 * `sock->_fd_mode`.
 */
z_result_t _z_open_can(_z_can_socket_t *sock, const char *dev, uint32_t bitrate, uint32_t dbitrate, uint32_t id,
                       uint32_t match, uint32_t mask);

/** Listen side. A CAN bus has no connection setup and multicast peers all
 *  listen, so this is identical to `_z_open_can`; both exist because the link
 *  table wants both callbacks. */
z_result_t _z_listen_can(_z_can_socket_t *sock, const char *dev, uint32_t bitrate, uint32_t dbitrate, uint32_t id,
                         uint32_t match, uint32_t mask);

void _z_close_can(_z_can_socket_t *sock);

/** Read one datagram and report who sent it.
 *
 * Skips this peer's own frames and any whose identifier the mask excludes, so a
 * shared bus does not deliver foreign traffic into the transport. When `addr`
 * is non-NULL the sender's identifier is written to it as `_Z_CAN_ADDR_SIZE`
 * little-endian bytes; the multicast transport uses that to tell peers apart.
 *
 * Returns the datagram length, or `SIZE_MAX` on error. */
size_t _z_read_can(const _z_can_socket_t *sock, uint8_t *ptr, size_t len, _z_slice_t *addr);

/** Write one datagram, which must be <= the socket's MTU. Returns the number
 *  of bytes written, or `SIZE_MAX` on error. */
size_t _z_send_can(const _z_can_socket_t *sock, const uint8_t *ptr, size_t len);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_CAN_H */
