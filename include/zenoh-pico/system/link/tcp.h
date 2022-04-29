/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_PICO_SYSTEM_LINK_TCP_H
#define ZENOH_PICO_SYSTEM_LINK_TCP_H

#include <stdint.h>
#include "zenoh-pico/collections/string.h"

#if Z_LINK_TCP == 1

typedef struct
{
    int _sock;
    void *_raddr;
} _z_tcp_socket_t;

void *_z_create_endpoint_tcp(const _z_str_t s_addr, const _z_str_t port);
void _z_free_endpoint_tcp(void *arg);

int _z_open_tcp(void *arg, const clock_t tout);
int _z_listen_tcp(void *arg);
void _z_close_tcp(int sock);
size_t _z_read_exact_tcp(int sock, uint8_t *ptr, size_t len);
size_t _z_read_tcp(int sock, uint8_t *ptr, size_t len);
size_t _z_send_tcp(int sock, const uint8_t *ptr, size_t len);
#endif

#endif /* ZENOH_PICO_SYSTEM_LINK_TCP_H */
