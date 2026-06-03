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

#include "zenoh-pico/link/config/tcp.h"

#include <stdlib.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/tcp.h"

#if Z_FEATURE_LINK_TCP == 1

z_result_t _z_endpoint_tcp_valid(_z_endpoint_t *endpoint) {
    _z_string_t tcp_str = _z_string_alias_str(TCP_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &tcp_str)) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_tcp_address_valid(&endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
    }
    return ret;
}

z_result_t _z_f_link_open_tcp(_z_link_t *zl) {
    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&zl->_endpoint._config, TCP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    return _z_tcp_open(&zl->_socket._tcp._sock, zl->_socket._tcp._rep, tout);
}

z_result_t _z_f_link_listen_tcp(_z_link_t *zl) { return _z_tcp_listen(&zl->_socket._tcp._sock, zl->_socket._tcp._rep); }

void _z_f_link_close_tcp(_z_link_t *zl) { _z_tcp_close(&zl->_socket._tcp._sock); }

void _z_f_link_free_tcp(_z_link_t *zl) { _z_tcp_endpoint_clear(&zl->_socket._tcp._rep); }

size_t _z_f_link_write_tcp(const _z_link_t *zl, const uint8_t *ptr, size_t len, _z_sys_net_socket_t *socket) {
    if (socket != NULL) {
        return _z_tcp_write(*socket, ptr, len);
    } else {
        return _z_tcp_write(zl->_socket._tcp._sock, ptr, len);
    }
}

size_t _z_f_link_write_all_tcp(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    return _z_tcp_write(zl->_socket._tcp._sock, ptr, len);
}

size_t _z_f_link_read_tcp(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    return _z_tcp_read(zl->_socket._tcp._sock, ptr, len);
}

size_t _z_f_link_read_exact_tcp(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_slice_t *addr,
                                _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(addr);
    if (socket != NULL) {
        return _z_tcp_read_exact(*socket, ptr, len);
    } else {
        return _z_tcp_read_exact(zl->_socket._tcp._sock, ptr, len);
    }
}

size_t _z_f_link_tcp_read_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    return _z_tcp_read(socket, ptr, len);
}

uint16_t _z_get_link_mtu_tcp(void) {
    // Maximum MTU for TCP
    return 65535;
}

z_result_t _z_new_peer_tcp(_z_endpoint_t *endpoint, _z_sys_net_socket_t *socket) {
    _z_sys_net_endpoint_t sys_endpoint = {0};
    z_result_t ret = _z_tcp_endpoint_init_from_address(&sys_endpoint, &endpoint->_locator._address);

    if (ret != _Z_RES_OK) {
        _z_tcp_endpoint_clear(&sys_endpoint);
        return ret;
    }

    ret = _z_tcp_open(socket, sys_endpoint, Z_CONFIG_SOCKET_TIMEOUT);
    _z_tcp_endpoint_clear(&sys_endpoint);
    return ret;
}

z_result_t _z_new_link_tcp(_z_link_t *zl, _z_endpoint_t *endpoint) {
    zl->_type = _Z_LINK_TYPE_TCP;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_STREAM;
    zl->_cap._is_reliable = true;

    zl->_mtu = _z_get_link_mtu_tcp();

    zl->_endpoint = *endpoint;
    z_result_t ret = _z_tcp_endpoint_init_from_address(&zl->_socket._tcp._rep, &endpoint->_locator._address);

    zl->_open_f = _z_f_link_open_tcp;
    zl->_listen_f = _z_f_link_listen_tcp;
    zl->_close_f = _z_f_link_close_tcp;
    zl->_free_f = _z_f_link_free_tcp;

    zl->_write_f = _z_f_link_write_tcp;
    zl->_write_all_f = _z_f_link_write_all_tcp;
    zl->_read_f = _z_f_link_read_tcp;
    zl->_read_exact_f = _z_f_link_read_exact_tcp;
    zl->_read_socket_f = _z_f_link_tcp_read_socket;

    return ret;
}
#else
z_result_t _z_endpoint_tcp_valid(_z_endpoint_t *endpoint) {
    _ZP_UNUSED(endpoint);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}

z_result_t _z_new_peer_tcp(_z_endpoint_t *endpoint, _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(endpoint);
    _ZP_UNUSED(socket);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}

z_result_t _z_new_link_tcp(_z_link_t *zl, _z_endpoint_t *endpoint) {
    _ZP_UNUSED(zl);
    _ZP_UNUSED(endpoint);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}
#endif
