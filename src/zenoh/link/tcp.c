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

_zn_socket_result_t _zn_f_link_open_unicast_tcp(void *arg, const clock_t tout)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    _zn_socket_result_t r_sock = _zn_open_unicast_tcp(self->endpoint);
    return r_sock;
}

int _zn_f_link_close_unicast_tcp(void *arg)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_close_tcp(self->sock);
}

void _zn_f_link_release_unicast_tcp(void *arg)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    _zn_release_endpoint_tcp(self->endpoint);
}

size_t _zn_f_link_write_unicast_tcp(void *arg, const uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_send_tcp(self->sock, ptr, len);
}

size_t _zn_f_link_write_all_unicast_tcp(void *arg, const uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_send_tcp(self->sock, ptr, len);
}

size_t _zn_f_link_read_unicast_tcp(void *arg, uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_read_tcp(self->sock, ptr, len);
}

size_t _zn_f_link_read_exact_unicast_tcp(void *arg, uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_read_exact_tcp(self->sock, ptr, len);
}

size_t _zn_get_link_mtu_unicast_tcp()
{
    // TODO
    return -1;
}

_zn_link_t *_zn_new_link_unicast_tcp(const char *s_addr, const char *port)
{
    _zn_link_t *lt = (_zn_link_t *)malloc(sizeof(_zn_link_t));
    lt->is_reliable = 1;
    lt->is_streamed = 1;

    lt->endpoint = _zn_create_endpoint_tcp(s_addr, port);
    lt->mtu = _zn_get_link_mtu_unicast_tcp();

    lt->open_f = _zn_f_link_open_unicast_tcp;
    lt->close_f = _zn_f_link_close_unicast_tcp;
    lt->release_f = _zn_f_link_release_unicast_tcp;

    lt->write_f = _zn_f_link_write_unicast_tcp;
    lt->write_all_f = _zn_f_link_write_all_unicast_tcp;
    lt->read_f = _zn_f_link_read_unicast_tcp;
    lt->read_exact_f = _zn_f_link_read_exact_unicast_tcp;

    return lt;
}
