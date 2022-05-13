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

#if ZN_LINK_TCP == 1

typedef struct
{
    void *sock;
    void *raddr;
} _zn_tcp_socket_t;

void *_zn_create_endpoint_tcp(const z_str_t s_addr, const z_str_t port);
void _zn_free_endpoint_tcp(void *addr_arg);

void *_zn_open_tcp(void *raddr_arg, const clock_t tout);
void *_zn_listen_tcp(void *raddr_arg);
void _zn_close_tcp(void *sock_arg);
size_t _zn_read_exact_tcp(void *sock_arg, uint8_t *ptr, size_t len);
size_t _zn_read_tcp(void *sock_arg, uint8_t *ptr, size_t len);
size_t _zn_send_tcp(void *sock_arg, const uint8_t *ptr, size_t len);
#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_TCP_H */
