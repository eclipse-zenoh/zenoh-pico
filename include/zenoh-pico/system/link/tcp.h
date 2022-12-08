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

#ifndef ZENOH_PICO_SYSTEM_LINK_TCP_H
#define ZENOH_PICO_SYSTEM_LINK_TCP_H

#include <stdint.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/system/platform.h"

#if Z_LINK_TCP == 1

typedef struct {
    _z_sys_net_socket_t _sock;
    _z_sys_net_endpoint_t _rep;
} _z_tcp_socket_t;

int8_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_addr, const char *s_port);
void _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep);

int8_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout);
int8_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep);
void _z_close_tcp(_z_sys_net_socket_t *sock);
size_t _z_read_exact_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len);
#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_TCP_H */
