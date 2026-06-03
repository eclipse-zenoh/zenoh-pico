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

#include "zenoh-pico/link/config/ws.h"

#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/tcp.h"
#include "zenoh-pico/link/transport/ws.h"

#if Z_FEATURE_LINK_WS == 1

static z_result_t _z_ws_address_valid(const _z_string_t *address) { return _z_tcp_address_valid(address); }

z_result_t _z_endpoint_ws_valid(_z_endpoint_t *endpoint) {
    _z_string_t str = _z_string_alias_str(WS_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &str)) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_ws_address_valid(&endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
    }
    return ret;
}

z_result_t _z_f_link_open_ws(_z_link_t *zl) {
    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&zl->_endpoint._config, WS_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    return _z_ws_transport_open(&zl->_socket._ws, tout);
}

z_result_t _z_f_link_listen_ws(_z_link_t *zl) { return _z_ws_transport_listen(&zl->_socket._ws); }

void _z_f_link_close_ws(_z_link_t *zl) { _z_ws_transport_close(&zl->_socket._ws); }

void _z_f_link_free_ws(_z_link_t *zl) { _z_ws_endpoint_clear(&zl->_socket._ws._rep); }

size_t _z_f_link_write_ws(const _z_link_t *zl, const uint8_t *ptr, size_t len, _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(socket);
    return _z_ws_transport_write(&zl->_socket._ws, ptr, len);
}

size_t _z_f_link_write_all_ws(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    return _z_ws_transport_write(&zl->_socket._ws, ptr, len);
}

size_t _z_f_link_read_ws(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    return _z_ws_transport_read(&zl->_socket._ws, ptr, len);
}

size_t _z_f_link_read_exact_ws(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr,
                               _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(addr);
    _ZP_UNUSED(socket);
    return _z_ws_transport_read_exact(&zl->_socket._ws, ptr, len);
}

size_t _z_f_link_ws_read_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    return _z_ws_transport_read_socket(socket, ptr, len);
}

uint16_t _z_get_link_mtu_ws(void) {
    // Maximum MTU for TCP
    return 65535;
}

z_result_t _z_new_link_ws(_z_link_t *zl, _z_endpoint_t *endpoint) {
    zl->_type = _Z_LINK_TYPE_WS;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    zl->_cap._is_reliable = true;

    zl->_mtu = _z_get_link_mtu_ws();

    zl->_endpoint = *endpoint;
    z_result_t ret = _z_ws_endpoint_init(&zl->_socket._ws._rep, &endpoint->_locator._address);

    zl->_open_f = _z_f_link_open_ws;
    zl->_listen_f = _z_f_link_listen_ws;
    zl->_close_f = _z_f_link_close_ws;
    zl->_free_f = _z_f_link_free_ws;

    zl->_write_f = _z_f_link_write_ws;
    zl->_write_all_f = _z_f_link_write_all_ws;
    zl->_read_f = _z_f_link_read_ws;
    zl->_read_exact_f = _z_f_link_read_exact_ws;
    zl->_read_socket_f = _z_f_link_ws_read_socket;

    return ret;
}
#endif
