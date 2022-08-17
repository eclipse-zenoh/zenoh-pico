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

#if Z_LINK_TCP == 1

typedef struct {
    void *_sock;
    void *_raddr;
} _z_tcp_socket_t;

void *_z_create_endpoint_tcp(const char *s_addr, const char *port);
void _z_free_endpoint_tcp(void *addr_arg);

void *_z_open_tcp(void *raddr_arg, uint32_t tout);
void *_z_listen_tcp(void *raddr_arg);
void _z_close_tcp(void *sock_arg);
size_t _z_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len);
size_t _z_read_tcp(void *sock_arg, uint8_t *ptr, size_t len);
size_t _z_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len);
#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_TCP_H */
