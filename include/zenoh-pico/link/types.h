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

#ifndef _ZENOH_PICO_TRANSPORT_PRIVATE_LINK_H
#define _ZENOH_PICO_TRANSPORT_PRIVATE_LINK_H

#include <stdint.h>

#include "zenoh-pico/system/types.h"
#include "zenoh-pico/system/result.h"

#define TCP_SCHEMA "tcp"
#define UDP_SCHEMA "udp"

typedef _zn_socket_result_t (*_zn_f_link_open)(void *arg, clock_t tout);
typedef int (*_zn_f_link_close)(void *arg);
typedef void (*_zn_f_link_release)(void *arg);
typedef size_t (*_zn_f_link_write)(void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_write_all)(void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_read)(void *arg, uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_read_exact)(void *arg, uint8_t *ptr, size_t len);

typedef struct {
    _zn_socket_t sock;

    uint8_t is_reliable;
    uint8_t is_streamed;

    void* endpoint;
    uint16_t mtu;

    // Function pointers
    _zn_f_link_open open_f;
    _zn_f_link_close close_f;
    _zn_f_link_release release_f;
    _zn_f_link_write write_f;
    _zn_f_link_write_all write_all_f;
    _zn_f_link_read read_f;
    _zn_f_link_read_exact read_exact_f;
} _zn_link_t;

#endif /* _ZENOH_PICO_TRANSPORT_PRIVATE_LINK_H */
