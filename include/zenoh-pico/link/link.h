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

#ifndef ZENOH_PICO_LINK_H
#define ZENOH_PICO_LINK_H

#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/system/link/tcp.h"
#include "zenoh-pico/system/link/udp.h"
#include "zenoh-pico/system/link/bt.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Link ------------------*/
typedef int (*_zn_f_link_open)(void *arg);
typedef int (*_zn_f_link_listen)(void *arg);
typedef void (*_zn_f_link_close)(void *arg);
typedef size_t (*_zn_f_link_write)(const void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_write_all)(const void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_zn_f_link_read)(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr);
typedef size_t (*_zn_f_link_read_exact)(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr);
typedef void (*_zn_f_link_free)(void *arg);

typedef struct
{
    _zn_endpoint_t endpoint;

    union
    {
        _zn_tcp_socket_t tcp;
        _zn_udp_socket_t udp;
        _zn_bt_socket_t bt;
    } socket;

    _zn_f_link_open open_f;
    _zn_f_link_listen listen_f;
    _zn_f_link_close close_f;
    _zn_f_link_write write_f;
    _zn_f_link_write_all write_all_f;
    _zn_f_link_read read_f;
    _zn_f_link_read_exact read_exact_f;
    _zn_f_link_free free_f;

    uint16_t mtu;
    uint8_t is_reliable;
    uint8_t is_streamed;
    uint8_t is_multicast;
} _zn_link_t;

_ZN_RESULT_DECLARE(_zn_link_t, link)
_ZN_P_RESULT_DECLARE(_zn_link_t, link)

void _zn_link_free(_zn_link_t **zn);
_zn_link_p_result_t _zn_open_link(const z_str_t locator);
_zn_link_p_result_t _zn_listen_link(const z_str_t locator);

int _zn_link_send_wbuf(const _zn_link_t *link, const _z_wbuf_t *wbf);
int _zn_link_recv_zbuf(const _zn_link_t *link, _z_zbuf_t *zbf, z_bytes_t *addr);
int _zn_link_recv_exact_zbuf(const _zn_link_t *link, _z_zbuf_t *zbf, size_t len, z_bytes_t *addr);

#endif /* ZENOH_PICO_LINK_H */