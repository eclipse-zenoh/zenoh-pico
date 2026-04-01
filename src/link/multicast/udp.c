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

#include "zenoh-pico/link/config/udp.h"

#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/backend/default_ops.h"
#include "zenoh-pico/link/backend/udp_multicast.h"
#include "zenoh-pico/link/backend/udp_unicast.h"
#include "zenoh-pico/link/manager.h"

#if Z_FEATURE_LINK_UDP_MULTICAST == 1

static const _z_udp_multicast_ops_t *_z_require_default_udp_multicast_ops(void) {
    const _z_udp_multicast_ops_t *ops = _z_default_udp_multicast_ops();
    if (ops == NULL) {
        _Z_ERROR_LOG(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
    }
    return ops;
}

z_result_t _z_endpoint_udp_multicast_valid(_z_endpoint_t *endpoint) {
    if (_z_require_default_udp_multicast_ops() == NULL) {
        return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    _z_string_t udp_str = _z_string_alias_str(UDP_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &udp_str)) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_udp_unicast_address_valid(&endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return ret;
    }

    const char *iface = _z_str_intmap_get(&endpoint->_config, UDP_CONFIG_IFACE_KEY);
    if (iface == NULL) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return _Z_RES_OK;
}

z_result_t _z_f_link_open_udp_multicast(_z_link_t *self) {
    const _z_udp_multicast_ops_t *ops = _z_require_default_udp_multicast_ops();
    if (ops == NULL) {
        return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }
    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    const char *iface = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_IFACE_KEY);
    return _z_udp_multicast_open(ops, &self->_socket._udp._sock, self->_socket._udp._rep, &self->_socket._udp._lep,
                                 tout, iface);
}

z_result_t _z_f_link_listen_udp_multicast(_z_link_t *self) {
    const _z_udp_multicast_ops_t *ops = _z_require_default_udp_multicast_ops();
    if (ops == NULL) {
        return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }
    z_result_t ret = _Z_RES_OK;

    const char *iface = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_IFACE_KEY);
    const char *join = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_JOIN_KEY);
    ret = _z_udp_multicast_listen(ops, &self->_socket._udp._sock, self->_socket._udp._rep, Z_CONFIG_SOCKET_TIMEOUT,
                                  iface, join);
    ret |= _z_udp_multicast_open(ops, &self->_socket._udp._msock, self->_socket._udp._rep, &self->_socket._udp._lep,
                                 Z_CONFIG_SOCKET_TIMEOUT, iface);

    return ret;
}

void _z_f_link_close_udp_multicast(_z_link_t *self) {
    const _z_udp_multicast_ops_t *ops = _z_default_udp_multicast_ops();
    if (ops != NULL) {
        _z_udp_multicast_close(ops, &self->_socket._udp._sock, &self->_socket._udp._msock, self->_socket._udp._rep,
                               self->_socket._udp._lep);
    }
}

void _z_f_link_free_udp_multicast(_z_link_t *self) {
    const _z_udp_multicast_ops_t *ops = _z_default_udp_multicast_ops();
    if (ops != NULL) {
        _z_udp_multicast_endpoint_clear(ops, &self->_socket._udp._lep);
        _z_udp_multicast_endpoint_clear(ops, &self->_socket._udp._rep);
    }
}

size_t _z_f_link_write_udp_multicast(const _z_link_t *self, const uint8_t *ptr, size_t len,
                                     _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(socket);
    const _z_udp_multicast_ops_t *ops = _z_default_udp_multicast_ops();
    return ops == NULL ? 0 : _z_udp_multicast_write(ops, self->_socket._udp._msock, ptr, len, self->_socket._udp._rep);
}

size_t _z_f_link_write_all_udp_multicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_multicast_ops_t *ops = _z_default_udp_multicast_ops();
    return ops == NULL ? 0 : _z_udp_multicast_write(ops, self->_socket._udp._msock, ptr, len, self->_socket._udp._rep);
}

size_t _z_f_link_read_udp_multicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    const _z_udp_multicast_ops_t *ops = _z_default_udp_multicast_ops();
    return ops == NULL ? 0
                       : _z_udp_multicast_read(ops, self->_socket._udp._sock, ptr, len, self->_socket._udp._lep, addr);
}

size_t _z_f_link_read_exact_udp_multicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr,
                                          _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(socket);
    const _z_udp_multicast_ops_t *ops = _z_default_udp_multicast_ops();
    return ops == NULL
               ? 0
               : _z_udp_multicast_read_exact(ops, self->_socket._udp._sock, ptr, len, self->_socket._udp._lep, addr);
}

uint16_t _z_get_link_mtu_udp_multicast(void) {
    // @TODO: the return value should change depending on the target platform.
    return 1450;
}

z_result_t _z_new_link_udp_multicast(_z_link_t *zl, _z_endpoint_t endpoint) {
    const _z_udp_multicast_ops_t *ops = _z_require_default_udp_multicast_ops();
    if (ops == NULL) {
        return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }
    zl->_type = _Z_LINK_TYPE_UDP;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_MULTICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    zl->_cap._is_reliable = false;

    zl->_mtu = _z_get_link_mtu_udp_multicast();

    zl->_endpoint = endpoint;
    z_result_t ret =
        _z_udp_multicast_endpoint_init_from_address(ops, &zl->_socket._udp._rep, &endpoint._locator._address);
    memset(&zl->_socket._udp._lep, 0, sizeof(zl->_socket._udp._lep));

    zl->_open_f = _z_f_link_open_udp_multicast;
    zl->_listen_f = _z_f_link_listen_udp_multicast;
    zl->_close_f = _z_f_link_close_udp_multicast;
    zl->_free_f = _z_f_link_free_udp_multicast;

    zl->_write_f = _z_f_link_write_udp_multicast;
    zl->_write_all_f = _z_f_link_write_all_udp_multicast;
    zl->_read_f = _z_f_link_read_udp_multicast;
    zl->_read_exact_f = _z_f_link_read_exact_udp_multicast;
    zl->_read_socket_f = _z_noop_link_read_socket;

    return ret;
}

#endif
