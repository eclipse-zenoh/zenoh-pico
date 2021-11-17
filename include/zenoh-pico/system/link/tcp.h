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

void *_zn_create_endpoint_tcp(const z_str_t s_addr, const z_str_t port);
void _zn_free_endpoint_tcp(void *arg);

int _zn_open_tcp(void *arg);
int _zn_listen_tcp(void *arg);
void _zn_close_tcp(int sock);
size_t _zn_read_exact_tcp(int sock, uint8_t *ptr, size_t len);
size_t _zn_read_tcp(int sock, uint8_t *ptr, size_t len);
size_t _zn_send_tcp(int sock, const uint8_t *ptr, size_t len);

#endif /* ZENOH_PICO_SYSTEM_LINK_TCP_H */
