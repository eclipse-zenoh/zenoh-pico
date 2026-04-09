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

#ifndef ZENOH_PICO_LINK_BACKEND_SERIAL_H
#define ZENOH_PICO_LINK_BACKEND_SERIAL_H

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

z_result_t _z_serial_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate);
z_result_t _z_serial_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate);
z_result_t _z_serial_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate);
z_result_t _z_serial_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate);
void _z_serial_close(_z_sys_net_socket_t *sock);
// flawfinder: ignore
size_t _z_serial_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t _z_serial_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_SERIAL_H */
