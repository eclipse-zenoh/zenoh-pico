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

#include <stdint.h>
#include <stdlib.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/backend/default_ops.h"
#include "zenoh-pico/link/backend/udp_unicast.h"
#include "zenoh-pico/link/manager.h"

#if Z_FEATURE_LINK_UDP_UNICAST == 1

z_result_t _z_endpoint_udp_unicast_valid(_z_endpoint_t *endpoint) {
    _z_string_t udp_str = _z_string_alias_str(UDP_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &udp_str)) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_udp_unicast_address_valid(&endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
    }
    return ret;
}

z_result_t _z_f_link_open_udp_unicast(_z_link_t *self) {
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    if (ops == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
    }

    return _z_udp_unicast_open(ops, &self->_socket._udp._sock, self->_socket._udp._rep, tout);
}

z_result_t _z_f_link_listen_udp_unicast(_z_link_t *self) {
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    if (ops == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
    }

    return _z_udp_unicast_listen(ops, &self->_socket._udp._sock, self->_socket._udp._rep, tout);
}

void _z_f_link_close_udp_unicast(_z_link_t *self) {
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();
    if (ops != NULL) {
        _z_udp_unicast_close(ops, &self->_socket._udp._sock);
    }
}

void _z_f_link_free_udp_unicast(_z_link_t *self) {
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();
    if (ops != NULL) {
        _z_udp_unicast_endpoint_clear(ops, &self->_socket._udp._rep);
    }
}

size_t _z_f_link_write_udp_unicast(const _z_link_t *self, const uint8_t *ptr, size_t len, _z_sys_net_socket_t *socket) {
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();
    if (ops == NULL) {
        return SIZE_MAX;
    }
    if (socket != NULL) {
        return _z_udp_unicast_write(ops, *socket, ptr, len, self->_socket._udp._rep);
    } else {
        return _z_udp_unicast_write(ops, self->_socket._udp._sock, ptr, len, self->_socket._udp._rep);
    }
}

size_t _z_f_link_write_all_udp_unicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();
    if (ops == NULL) {
        return SIZE_MAX;
    }
    return _z_udp_unicast_write(ops, self->_socket._udp._sock, ptr, len, self->_socket._udp._rep);
}

size_t _z_f_link_read_udp_unicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();
    if (ops == NULL) {
        return SIZE_MAX;
    }
    return _z_udp_unicast_read(ops, self->_socket._udp._sock, ptr, len);
}

size_t _z_f_link_read_exact_udp_unicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr,
                                        _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(addr);
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();
    if (ops == NULL) {
        return SIZE_MAX;
    }
    if (socket != NULL) {
        return _z_udp_unicast_read_exact(ops, *socket, ptr, len);
    } else {
        return _z_udp_unicast_read_exact(ops, self->_socket._udp._sock, ptr, len);
    }
}

size_t _z_f_link_udp_read_socket(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();
    if (ops == NULL) {
        return SIZE_MAX;
    }
    return _z_udp_unicast_read(ops, socket, ptr, len);
}

uint16_t _z_get_link_mtu_udp_unicast(void) {
    // @TODO: the return value should change depending on the target platform.
    return 1450;
}

z_result_t _z_new_link_udp_unicast(_z_link_t *zl, _z_endpoint_t endpoint) {
    const _z_udp_unicast_ops_t *ops = _z_default_udp_unicast_ops();
    zl->_type = _Z_LINK_TYPE_UDP;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    zl->_cap._is_reliable = false;

    zl->_mtu = _z_get_link_mtu_udp_unicast();

    zl->_endpoint = endpoint;
    z_result_t ret =
        _z_udp_unicast_endpoint_init_from_address(ops, &zl->_socket._udp._rep, &endpoint._locator._address);

    zl->_open_f = _z_f_link_open_udp_unicast;
    zl->_listen_f = _z_f_link_listen_udp_unicast;
    zl->_close_f = _z_f_link_close_udp_unicast;
    zl->_free_f = _z_f_link_free_udp_unicast;

    zl->_write_f = _z_f_link_write_udp_unicast;
    zl->_write_all_f = _z_f_link_write_all_udp_unicast;
    zl->_read_f = _z_f_link_read_udp_unicast;
    zl->_read_exact_f = _z_f_link_read_exact_udp_unicast;
    zl->_read_socket_f = _z_f_link_udp_read_socket;

    return ret;
}
#endif
