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

#include "zenoh-pico/link/transport/serial.h"

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/common/socket_ops.h"
#include "zenoh-pico/link/driver.h"
#include "zenoh-pico/link/transport/serial_protocol.h"

#if Z_FEATURE_LINK_SERIAL == 1

typedef struct {
    _z_link_t _base;
    _z_serial_socket_t _serial;
} _z_serial_link_t;

static size_t _z_link_peer_read_serial(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len);
static size_t _z_link_peer_write_serial(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr,
                                        size_t len);

static const _z_link_peer_ops_t _z_serial_peer_ops = {
    ._read_f = _z_link_peer_read_serial,
    ._write_f = _z_link_peer_write_serial,
    ._set_blocking_f = _z_link_socket_peer_set_blocking,
    ._get_endpoints_f = _z_link_socket_peer_get_endpoints,
    ._close_f = _z_link_socket_peer_close,
};

z_result_t _z_endpoint_serial_valid(_z_endpoint_t *endpoint) { return _z_serial_endpoint_valid(endpoint); }

z_result_t _z_f_link_open_serial(_z_link_t *self) {
    _z_serial_link_t *link = (_z_serial_link_t *)self;
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _Z_RETURN_IF_ERR(_z_serial_protocol_open(&link->_serial, &self->_endpoint));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_link_socket_peer_from_socket(&self->_peer, link->_serial._sock, _z_serial_close, &_z_serial_peer_ops),
        _z_serial_protocol_close(&link->_serial));
    return _Z_RES_OK;
}

z_result_t _z_f_link_listen_serial(_z_link_t *self) {
    _z_serial_link_t *link = (_z_serial_link_t *)self;
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _Z_RETURN_IF_ERR(_z_serial_protocol_listen(&link->_serial, &self->_endpoint));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_link_socket_peer_from_socket(&self->_peer, link->_serial._sock, _z_serial_close, &_z_serial_peer_ops),
        _z_serial_protocol_close(&link->_serial));
    return _Z_RES_OK;
}

void _z_f_link_close_serial(_z_link_t *self) { _z_link_peer_close(&self->_peer); }

size_t _z_f_link_write_serial(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return socket == NULL ? SIZE_MAX : _z_send_serial(*socket, ptr, len);
}

size_t _z_f_link_write_all_serial(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return socket == NULL ? SIZE_MAX : _z_send_serial(*socket, ptr, len);
}

size_t _z_f_link_read_serial(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return socket == NULL ? SIZE_MAX : _z_read_serial(*socket, ptr, len);
}

size_t _z_f_link_read_exact_serial(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return socket == NULL ? SIZE_MAX : _z_read_exact_serial(*socket, ptr, len);
}

uint16_t _z_get_link_mtu_serial(void) { return _Z_SERIAL_MTU_SIZE; }

static size_t _z_link_peer_read_serial(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    return socket == NULL ? SIZE_MAX : _z_read_serial(*socket, ptr, len);
}

static size_t _z_link_peer_write_serial(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr,
                                        size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    return socket == NULL ? SIZE_MAX : _z_send_serial(*socket, ptr, len);
}

z_result_t _z_new_link_serial(_z_link_t **zl, _z_endpoint_t *endpoint) {
    if (zl == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    *zl = NULL;

    _z_serial_link_t *link = (_z_serial_link_t *)z_malloc(sizeof(_z_serial_link_t));
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(link, 0, sizeof(_z_serial_link_t));

    _z_link_t *base = &link->_base;
    base->_endpoint = *endpoint;
    *endpoint = (_z_endpoint_t){0};
    base->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    base->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    base->_cap._is_reliable = false;

    base->_mtu = _z_get_link_mtu_serial();

    base->_close_f = _z_f_link_close_serial;

    base->_write_f = _z_f_link_write_serial;
    base->_write_all_f = _z_f_link_write_all_serial;
    base->_read_f = _z_f_link_read_serial;
    base->_read_exact_f = _z_f_link_read_exact_serial;
    base->_wait_peers_readable_f = _z_link_socket_wait_peers_readable;
    base->_peer_from_link_f = _z_link_peer_from_default;

    *zl = base;

    return _Z_RES_OK;
}

static z_result_t _z_link_driver_serial_create(_z_link_t **link, _z_endpoint_t *endpoint,
                                               const _z_config_t *session_cfg) {
    _ZP_UNUSED(session_cfg);
    return _z_new_link_serial(link, endpoint);
}

const _z_link_driver_t _z_link_driver_serial = {
    ._validate_f = _z_endpoint_serial_valid,
    ._create_f = _z_link_driver_serial_create,
    ._open_f = _z_f_link_open_serial,
    ._listen_f = NULL,
};
#endif
