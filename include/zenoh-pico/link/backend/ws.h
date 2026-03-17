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

#ifndef ZENOH_PICO_LINK_BACKEND_WS_H
#define ZENOH_PICO_LINK_BACKEND_WS_H

#include <stdint.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LINK_WS == 1

typedef struct {
    _z_sys_net_socket_t _sock;
    _z_sys_net_endpoint_t _rep;
} _z_ws_socket_t;

z_result_t _z_ws_endpoint_init(_z_sys_net_endpoint_t *ep, const _z_string_t *address);
void _z_ws_endpoint_clear(_z_sys_net_endpoint_t *ep);
z_result_t _z_ws_transport_open(_z_ws_socket_t *sock, uint32_t tout);
z_result_t _z_ws_transport_listen(_z_ws_socket_t *sock);
void _z_ws_transport_close(_z_ws_socket_t *sock);
size_t _z_ws_transport_read(const _z_ws_socket_t *sock, uint8_t *ptr, size_t len);
size_t _z_ws_transport_read_exact(const _z_ws_socket_t *sock, uint8_t *ptr, size_t len);
size_t _z_ws_transport_write(const _z_ws_socket_t *sock, const uint8_t *ptr, size_t len);
size_t _z_ws_transport_read_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_WS_H */
