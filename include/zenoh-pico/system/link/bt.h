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

#ifndef ZENOH_PICO_SYSTEM_LINK_BT_H
#define ZENOH_PICO_SYSTEM_LINK_BT_H

#include <stdint.h>
#include "zenoh-pico/collections/string.h"

void *_zn_create_endpoint_bt(const z_str_t s_addr);
void _zn_free_endpoint_bt(void *arg);

// Unicast
int _zn_open_bt(void *arg, z_str_t mode, const clock_t tout);
int _zn_listen_bt(void *arg, z_str_t mode, const clock_t tout);
void _zn_close_bt(int sock);
size_t _zn_read_exact_bt(int sock, uint8_t *ptr, size_t len);
size_t _zn_read_bt(int sock, uint8_t *ptr, size_t len);
size_t _zn_send_bt(int sock, const uint8_t *ptr, size_t len, void *arg);

#endif /* ZENOH_PICO_SYSTEM_LINK_BT_H */
