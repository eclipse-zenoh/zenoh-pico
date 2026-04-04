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

#ifndef ZENOH_PICO_LINK_BACKEND_SERIAL_PROTOCOL_H
#define ZENOH_PICO_LINK_BACKEND_SERIAL_PROTOCOL_H

#include <stdint.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LINK_SERIAL == 1

#define _Z_SERIAL_MTU_SIZE 1500
#define _Z_SERIAL_MFS_SIZE _Z_SERIAL_MTU_SIZE + 1 + 2 + 4  // MTU + Header + Serial Len + Serial CRC32
#define _Z_SERIAL_MAX_COBS_BUF_SIZE \
    1516  // Max On-the-wire length for an MFS/MTU of 1510/1500 (MFS + Overhead Byte (OHB) + End of packet (EOP))

typedef struct {
    _z_sys_net_socket_t _sock;
} _z_serial_socket_t;

z_result_t _z_serial_endpoint_valid(const _z_endpoint_t *endpoint);
z_result_t _z_serial_open(_z_serial_socket_t *sock, const _z_endpoint_t *endpoint);
z_result_t _z_serial_listen(_z_serial_socket_t *sock, const _z_endpoint_t *endpoint);
void _z_serial_close(_z_serial_socket_t *sock);
z_result_t _z_connect_serial(const _z_sys_net_socket_t sock);
size_t _z_read_serial(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t _z_send_serial(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len);
size_t _z_read_exact_serial(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_SERIAL_PROTOCOL_H */
