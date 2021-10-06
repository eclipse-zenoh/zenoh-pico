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

#include "zenoh-pico/system/common.h"
#include "zenoh-pico/link/private/manager.h"

_zn_socket_result_t _zn_f_link_udp_open(void *arg, const clock_t tout)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    _zn_socket_result_t r_sock = _zn_udp_open(self->endpoint, tout);
    return r_sock;
}

int _zn_f_link_udp_close(void *arg)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_udp_close(self->sock);
}

void _zn_f_link_udp_release(void *arg)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    _zn_release_udp_endpoint(self->endpoint);
}

size_t _zn_f_link_udp_write(void *arg, const uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_udp_send(self->sock, ptr, len, self->endpoint);
}

size_t _zn_f_link_udp_write_all(void *arg, const uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_udp_send(self->sock, ptr, len, self->endpoint);
}

size_t _zn_f_link_udp_read(void *arg, uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_udp_read(self->sock, ptr, len);
}

size_t _zn_f_link_udp_read_exact(void *arg, uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_udp_read_exact(self->sock, ptr, len);
}

size_t _zn_get_link_udp_mtu()
{
    // TODO
    return -1;
}

_zn_link_t *_zn_new_udp_link(const char *s_addr, const char *port)
{
    _zn_link_t *lt = (_zn_link_t *)malloc(sizeof(_zn_link_t));
    lt->is_reliable = 0;
    lt->is_streamed = 0;

    lt->endpoint = _zn_create_udp_endpoint(s_addr, port);
    lt->mtu = _zn_get_link_udp_mtu();

    lt->open_f = _zn_f_link_udp_open;
    lt->close_f = _zn_f_link_udp_close;
    lt->release_f = _zn_f_link_udp_release;

    lt->write_f = _zn_f_link_udp_write;
    lt->write_all_f = _zn_f_link_udp_write_all;
    lt->read_f = _zn_f_link_udp_read;
    lt->read_exact_f = _zn_f_link_udp_read_exact;

    return lt;
}
