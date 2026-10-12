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

#ifndef ZENOH_PICO_SYSTEM_LINK_ISOTP_H
#define ZENOH_PICO_SYSTEM_LINK_ISOTP_H

#include <stdint.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LINK_ISOTP == 1

// A CAN frame names a message, not a destination: there is no source or
// destination field and every node hears every frame. ISO 15765-2 builds
// point-to-point addressing on top of that from a DIRECTED IDENTIFIER PAIR plus
// flow control -- the receiver must answer a first frame and then paces the
// sender, so exactly one peer may own the other end of a pair. That pairing is
// the address.
//
// Two things follow, and they are why this link exists alongside the multicast
// multicast CAN link:
//
//   * segmentation moves below zenoh, so the MTU is 4095 bytes rather than
//     seven, which is what makes CLASSIC CAN usable -- and classic CAN is most
//     of the hardware in the field;
//   * it is UNICAST, and zenoh routes queries and liveliness only to unicast
//     faces, so this link can carry ROS services, actions, parameters and graph
//     introspection where the multicast one cannot.
//
// ISO-TP itself is a platform capability, never implemented here: Linux has it
// in the kernel, Zephyr in `subsys/canbus/isotp`, and other platforms use a
// vendored portable library.

// The classic first frame carries a 12-bit length, so a PDU is at most 4095
// bytes. The 2016 revision adds a 32-bit escape, but 4095 is what every
// implementation supports, and a larger MTU is a larger unit of loss: one
// dropped consecutive frame destroys the whole PDU.
#define _Z_ISOTP_MTU_SIZE 4095

typedef struct {
    _z_sys_net_socket_t _sock;
    uint32_t _tx_id;  // this peer transmits its PDUs on this identifier
    uint32_t _rx_id;  // and receives on this one, where it also sends flow control
    _Bool _eff;       // 29-bit extended identifiers rather than 11-bit
} _z_isotp_socket_t;

/**
 * Open an ISO-TP channel.
 *
 * `tx_id` and `rx_id` are a directed pair: this peer's `tx_id` must be the other
 * peer's `rx_id`. Only ISO-TP NORMAL addressing is used -- extended and mixed
 * addressing are a deliberate non-goal, because no portable implementation
 * provides them and normal addressing is the interoperable common denominator.
 */
z_result_t _z_open_isotp(_z_isotp_socket_t *sock, const char *dev, uint32_t tx_id, uint32_t rx_id, _Bool eff,
                         uint8_t stmin, uint8_t bs);

void _z_close_isotp(_z_isotp_socket_t *sock);

/** Read one reassembled PDU. Returns its length, or `SIZE_MAX` on error. */
size_t _z_read_isotp(const _z_isotp_socket_t *sock, uint8_t *ptr, size_t len);

/**
 * Read one reassembled PDU given only the descriptor.
 *
 * The unicast transport's `_read_socket_f` hands back a bare
 * `_z_sys_net_socket_t`, not the ISO-TP socket, so the identifier pair is not
 * available here -- and it is not needed: the pair is bound into the socket and
 * the kernel has already used it to reassemble. `_z_read_isotp` delegates here.
 */
size_t _z_read_isotp_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len);

/** Write one PDU; the platform segments it. Returns bytes written, or
 *  `SIZE_MAX` on error. */
size_t _z_send_isotp(const _z_isotp_socket_t *sock, const uint8_t *ptr, size_t len);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_ISOTP_H */
