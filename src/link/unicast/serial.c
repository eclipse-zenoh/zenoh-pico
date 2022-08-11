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

#include <string.h>
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/config/serial.h"
#include "zenoh-pico/system/link/serial.h"

#if Z_LINK_SERIAL == 1

#define SPP_MAXIMUM_PAYLOAD 128

int _z_f_link_open_serial(void *arg)
{
    _z_link_t *self = (_z_link_t *)arg;

    char* p_dot = strchr(self->_endpoint._locator._address, '.');
    self->_socket._serial._sock = _z_open_serial(strtoul(self->_endpoint._locator._address, &p_dot, 10),
                                                 strtoul(p_dot + 1, NULL, 10),
                                                 strtoul(_z_str_intmap_get(&self->_endpoint._config, SERIAL_CONFIG_BAUDRATE_KEY), NULL, 10));
    if (self->_socket._serial._sock == NULL)
        goto ERR;

    return 0;

ERR:
    return -1;
}

int _z_f_link_listen_serial(void *arg)
{
    _z_link_t *self = (_z_link_t *)arg;

    self->_socket._serial._sock = _z_listen_serial(0, 0, 0);
    if (self->_socket._serial._sock == NULL)
        goto ERR;

    return 0;

ERR:
    return -1;
}

void _z_f_link_close_serial(void *arg)
{
    _z_link_t *self = (_z_link_t *)arg;

    _z_close_serial(self->_socket._serial._sock);
}

void _z_f_link_free_serial(void *arg)
{
    _z_link_t *self = (_z_link_t *)arg;
    (void) (self);
}

size_t _z_f_link_write_serial(const void *arg, const uint8_t *ptr, size_t len)
{
    const _z_link_t *self = (const _z_link_t *)arg;

    return _z_send_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_write_all_serial(const void *arg, const uint8_t *ptr, size_t len)
{
    const _z_link_t *self = (const _z_link_t *)arg;

    return _z_send_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_read_serial(const void *arg, uint8_t *ptr, size_t len, _z_bytes_t *addr)
{
    (void) (addr);
    const _z_link_t *self = (const _z_link_t *)arg;

    return _z_read_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_read_exact_serial(const void *arg, uint8_t *ptr, size_t len, _z_bytes_t *addr)
{
    (void) (addr);
    const _z_link_t *self = (const _z_link_t *)arg;

    return _z_read_exact_serial(self->_socket._serial._sock, ptr, len);
}

uint16_t _z_get_link_mtu_serial(void)
{
    return _Z_SERIAL_MTU_SIZE;
}

_z_link_t *_z_new_link_serial(_z_endpoint_t endpoint)
{
    _z_link_t *lt = (_z_link_t *)z_malloc(sizeof(_z_link_t));

    lt->_capabilities = Z_LINK_CAPABILITY_NONE;
    lt->_mtu = _z_get_link_mtu_serial();

    lt->_endpoint = endpoint;

    lt->_socket._serial._sock = NULL;

    lt->_open_f = _z_f_link_open_serial;
    lt->_listen_f = _z_f_link_listen_serial;
    lt->_close_f = _z_f_link_close_serial;
    lt->_free_f = _z_f_link_free_serial;

    lt->_write_f = _z_f_link_write_serial;
    lt->_write_all_f = _z_f_link_write_all_serial;
    lt->_read_f = _z_f_link_read_serial;
    lt->_read_exact_f = _z_f_link_read_exact_serial;

    return lt;
}
#endif
