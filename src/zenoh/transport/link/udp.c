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

#include <stdlib.h>
#include "zenoh-pico/system/common.h"
#include "zenoh-pico/transport/private/manager.h"
#include "zenoh-pico/utils/private/logging.h"

int _zn_f_link_udp_open(void *arg)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    _zn_socket_result_t r_sock = _zn_udp_open(self->s_addr, self->port);
    if (r_sock.tag != _z_res_t_OK)
        return -1;

    self->sock = r_sock.value.socket;
    return 0;
}

int _zn_f_link_udp_close(void *arg)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_udp_close(self->sock);
}

size_t _zn_f_link_udp_write(void *arg, const uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_udp_send(self->sock, ptr, len);
}

size_t _zn_f_link_udp_write_all(void *arg, const uint8_t *ptr, size_t len)
{
    _zn_link_t *self = (_zn_link_t*)arg;

    return _zn_udp_send(self->sock, ptr, len);
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

//FIXME: do proper return with _zn_*_result_t
_zn_link_t *_zn_new_udp_link(char* s_addr, int port)
{
    _zn_link_t *lt = (_zn_link_t *)malloc(sizeof(_zn_link_t));
    lt->is_reliable = 0;
    lt->is_streamed = 0;

    lt->s_addr = s_addr;
    lt->port = port;
    lt->mtu = _zn_get_link_udp_mtu();

    lt->o_func = _zn_f_link_udp_open;
    lt->c_func = _zn_f_link_udp_close;
    lt->w_func = _zn_f_link_udp_write;
    lt->wa_func = _zn_f_link_udp_write_all;
    lt->r_func = _zn_f_link_udp_read;
    lt->re_func = _zn_f_link_udp_read_exact;

    return lt;
}
