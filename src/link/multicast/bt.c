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

#include <string.h>
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/config/bt.h"
#include "zenoh-pico/system/link/bt.h"

#if ZN_LINK_BLUETOOTH == 1

#define SPP_MAXIMUM_PAYLOAD 128

int _zn_f_link_open_bt(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    self->socket.bt.sock = _zn_open_bt(strcmp(_z_str_intmap_get(&self->endpoint.config, BT_CONFIG_MODE_KEY), "master") == 0 ? _ZN_BT_MODE_MASTER : _ZN_BT_MODE_SLAVE,
                                       _z_str_intmap_get(&self->endpoint.config, BT_CONFIG_LNAME_KEY),
                                       _z_str_intmap_get(&self->endpoint.config, BT_CONFIG_RNAME_KEY),
                                       strcmp(_z_str_intmap_get(&self->endpoint.config, BT_CONFIG_PROFILE_KEY), "spp") == 0 ? _ZN_BT_PROFILE_SPP : _ZN_BT_PROFILE_UNSUPPORTED);
    if (self->socket.bt.sock == NULL)
        goto ERR;

    self->socket.bt.lname = _z_str_clone(_z_str_intmap_get(&self->endpoint.config, BT_CONFIG_LNAME_KEY));
    self->socket.bt.rname = _z_str_clone(_z_str_intmap_get(&self->endpoint.config, BT_CONFIG_RNAME_KEY));

    return 0;

ERR:
    return -1;
}

int _zn_f_link_listen_bt(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    self->socket.bt.sock = _zn_listen_bt(strcmp(_z_str_intmap_get(&self->endpoint.config, BT_CONFIG_MODE_KEY), "master") == 0 ? _ZN_BT_MODE_MASTER : _ZN_BT_MODE_SLAVE,
                                         _z_str_intmap_get(&self->endpoint.config, BT_CONFIG_LNAME_KEY),
                                         _z_str_intmap_get(&self->endpoint.config, BT_CONFIG_RNAME_KEY),
                                         strcmp(_z_str_intmap_get(&self->endpoint.config, BT_CONFIG_PROFILE_KEY), "spp") == 0 ? _ZN_BT_PROFILE_SPP : _ZN_BT_PROFILE_UNSUPPORTED);
    if (self->socket.bt.sock == NULL)
        goto ERR;

    self->socket.bt.lname = _z_str_clone(_z_str_intmap_get(&self->endpoint.config, BT_CONFIG_LNAME_KEY));
    self->socket.bt.rname = _z_str_clone(_z_str_intmap_get(&self->endpoint.config, BT_CONFIG_RNAME_KEY));

    return 0;

ERR:
    return -1;
}

void _zn_f_link_close_bt(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    _zn_close_bt(self->socket.bt.sock);
}

void _zn_f_link_free_bt(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;
    _z_str_free(&self->socket.bt.lname);
    _z_str_free(&self->socket.bt.rname);
}

size_t _zn_f_link_write_bt(const void *arg, const uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_send_bt(self->socket.bt.sock, ptr, len);
}

size_t _zn_f_link_write_all_bt(const void *arg, const uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_send_bt(self->socket.bt.sock, ptr, len);
}

size_t _zn_f_link_read_bt(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    size_t rb  = _zn_read_bt(self->socket.bt.sock, ptr, len);
    if (rb > 0 && addr != NULL)
    {
        *addr = _z_bytes_make(strlen(self->socket.bt.rname));
        memcpy((void *)addr->val, self->socket.bt.rname, addr->len);
    }

    return rb;
}

size_t _zn_f_link_read_exact_bt(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    size_t rb  = _zn_read_exact_bt(self->socket.bt.sock, ptr, len);
    if (rb == len && addr != NULL)
    {
        *addr = _z_bytes_make(strlen(self->socket.bt.rname));
        memcpy((void *)addr->val, self->socket.bt.rname, addr->len);
    }

    return rb;
}

uint16_t _zn_get_link_mtu_bt(void)
{
    return SPP_MAXIMUM_PAYLOAD;
}

_zn_link_t *_zn_new_link_bt(_zn_endpoint_t endpoint)
{
    _zn_link_t *lt = (_zn_link_t *)malloc(sizeof(_zn_link_t));

    lt->is_reliable = 0;
    lt->is_streamed = 1;
    lt->is_multicast = 1;
    lt->mtu = _zn_get_link_mtu_bt();

    lt->endpoint = endpoint;

    lt->socket.bt.sock = NULL;

    lt->open_f = _zn_f_link_open_bt;
    lt->listen_f = _zn_f_link_listen_bt;
    lt->close_f = _zn_f_link_close_bt;
    lt->free_f = _zn_f_link_free_bt;

    lt->write_f = _zn_f_link_write_bt;
    lt->write_all_f = _zn_f_link_write_all_bt;
    lt->read_f = _zn_f_link_read_bt;
    lt->read_exact_f = _zn_f_link_read_exact_bt;

    return lt;
}
#endif
