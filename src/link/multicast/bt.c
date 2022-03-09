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
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/config/bt.h"
#include "zenoh-pico/system/link/bt.h"

_zn_socket_result_t _zn_f_link_open_bt(void *arg, const clock_t tout)
{
    _zn_link_t *self = (_zn_link_t *)arg;
    _zn_socket_result_t r;
    r.tag = _z_res_t_OK;

    // FIXME: The listen and open procedures
    self->sock = _zn_open_bt(self->endpoint.locator.address, _z_str_intmap_get(&self->endpoint.config, BT_CONFIG_MODE_KEY), tout);
    if (self->sock < 0)
        goto ERR;

    r.value.socket = self->sock;
    return r;

ERR:
    r.tag = _z_res_t_ERR;
    r.value.error = _zn_err_t_OPEN_TRANSPORT_FAILED;
    return r;
}

_zn_socket_result_t _zn_f_link_listen_bt(void *arg, const clock_t tout)
{
    _zn_link_t *self = (_zn_link_t *)arg;
    _zn_socket_result_t r;
    r.tag = _z_res_t_OK;

    // FIXME: The listen and open procedures
    self->sock = _zn_listen_bt(self->endpoint.locator.address, _z_str_intmap_get(&self->endpoint.config, BT_CONFIG_MODE_KEY), tout);
    if (self->sock < 0)
        goto ERR;

    // FIXME: The listen and open procedures
    self->mcast_send_sock = _zn_open_bt(self->endpoint.locator.address, _z_str_intmap_get(&self->endpoint.config, BT_CONFIG_MODE_KEY), tout);
    if (self->mcast_send_sock < 0)
        goto ERR;

    r.value.socket = self->sock; // FIXME: we do not need to return it anymore
    return r;

ERR:
    r.tag = _z_res_t_ERR;
    r.value.error = _zn_err_t_OPEN_TRANSPORT_FAILED;
    return r;
}

void _zn_f_link_close_bt(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    _zn_close_bt(self->sock);
}

void _zn_f_link_free_bt(void *arg)
{
    _zn_link_t *self = (_zn_link_t *)arg;

    _zn_free_endpoint_bt(self->raddr);
    _zn_free_endpoint_bt(self->laddr);
}

size_t _zn_f_link_write_bt(const void *arg, const uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_send_bt(self->mcast_send_sock, ptr, len, self->raddr);
}

size_t _zn_f_link_write_all_bt(const void *arg, const uint8_t *ptr, size_t len)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    return _zn_send_bt(self->mcast_send_sock, ptr, len, self->raddr);
}

size_t _zn_f_link_read_bt(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    // FIXME: get the sender address

    return _zn_read_bt(self->sock, ptr, len);
}

size_t _zn_f_link_read_exact_bt(const void *arg, uint8_t *ptr, size_t len, z_bytes_t *addr)
{
    const _zn_link_t *self = (const _zn_link_t *)arg;

    // FIXME: get the sender address

    return _zn_read_exact_bt(self->sock, ptr, len);
}

uint16_t _zn_get_link_mtu_bt(void)
{
    // @TODO: the return value should change depending on the target platform.
    return 1450;
}

_zn_link_t *_zn_new_link_bt(_zn_endpoint_t endpoint)
{
    _zn_link_t *lt = (_zn_link_t *)malloc(sizeof(_zn_link_t));

    lt->is_reliable = 0;
    lt->is_streamed = 1;
    lt->is_multicast = 1;
    lt->mtu = _zn_get_link_mtu_bt();

    lt->raddr = NULL;
    lt->laddr = NULL;
    lt->endpoint = endpoint;

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
