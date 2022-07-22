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

#ifndef ZENOH_PICO_LINK_H
#define ZENOH_PICO_LINK_H

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/system/platform.h"

#if Z_LINK_TCP == 1
#include "zenoh-pico/system/link/tcp.h"
#endif

#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
#include "zenoh-pico/system/link/udp.h"
#endif

#if Z_LINK_BLUETOOTH == 1
#include "zenoh-pico/system/link/bt.h"
#endif

#include "zenoh-pico/utils/result.h"

/*------------------ Link ------------------*/
typedef int (*_z_f_link_open)(void *arg);
typedef int (*_z_f_link_listen)(void *arg);
typedef void (*_z_f_link_close)(void *arg);
typedef size_t (*_z_f_link_write)(const void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_z_f_link_write_all)(const void *arg, const uint8_t *ptr, size_t len);
typedef size_t (*_z_f_link_read)(const void *arg, uint8_t *ptr, size_t len, _z_bytes_t *addr);
typedef size_t (*_z_f_link_read_exact)(const void *arg, uint8_t *ptr, size_t len, _z_bytes_t *addr);
typedef void (*_z_f_link_free)(void *arg);

typedef struct
{
    _z_endpoint_t _endpoint;

    union
    {
#if Z_LINK_TCP == 1
        _z_tcp_socket_t _tcp;
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
        _z_udp_socket_t _udp;
#endif
#if Z_LINK_BLUETOOTH == 1
        _z_bt_socket_t _bt;
#endif
    } _socket;

    _z_f_link_open _open_f;
    _z_f_link_listen _listen_f;
    _z_f_link_close _close_f;
    _z_f_link_write _write_f;
    _z_f_link_write_all _write_all_f;
    _z_f_link_read _read_f;
    _z_f_link_read_exact _read_exact_f;
    _z_f_link_free _free_f;

    uint16_t _mtu;
    uint8_t _is_reliable;
    uint8_t _is_streamed;
    uint8_t _is_multicast;
} _z_link_t;

_Z_RESULT_DECLARE(_z_link_t, link)
_Z_P_RESULT_DECLARE(_z_link_t, link)

void _z_link_free(_z_link_t **zn);
_z_link_p_result_t _z_open_link(const char *locator);
_z_link_p_result_t _z_listen_link(const char *locator);

int _z_link_send_wbuf(const _z_link_t *link, const _z_wbuf_t *wbf);
size_t _z_link_recv_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, _z_bytes_t *addr);
size_t _z_link_recv_exact_zbuf(const _z_link_t *link, _z_zbuf_t *zbf, size_t len, _z_bytes_t *addr);

#endif /* ZENOH_PICO_LINK_H */