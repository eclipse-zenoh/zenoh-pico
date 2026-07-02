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
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/common/socket_ops.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/udp_unicast.h"

#if Z_FEATURE_LINK_UDP_UNICAST == 1

typedef struct {
    _z_udp_socket_t _udp;
} _z_udp_unicast_link_state_t;

static _z_udp_unicast_link_state_t *_z_udp_unicast_link_state(_z_link_t *link) {
    return (_z_udp_unicast_link_state_t *)_z_link_state(link);
}

static const _z_udp_unicast_link_state_t *_z_udp_unicast_link_state_const(const _z_link_t *link) {
    return (const _z_udp_unicast_link_state_t *)_z_link_state_const(link);
}

static size_t _z_link_peer_read_udp_unicast(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr,
                                            size_t len);
static size_t _z_link_peer_write_udp_unicast(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr,
                                             size_t len);

static const _z_link_peer_ops_t _z_udp_unicast_peer_ops = {
    ._read_f = _z_link_peer_read_udp_unicast,
    ._write_f = _z_link_peer_write_udp_unicast,
    ._set_blocking_f = _z_link_socket_peer_set_blocking,
    ._get_endpoints_f = _z_link_socket_peer_get_endpoints,
    ._close_f = _z_link_socket_peer_close,
};

bool _z_endpoint_udp_unicast_matches(const _z_endpoint_t *endpoint) {
    _z_string_t udp_str = _z_string_alias_str(UDP_SCHEMA);
    return _z_string_equals(&endpoint->_locator._protocol, &udp_str);
}

z_result_t _z_endpoint_udp_unicast_valid(_z_endpoint_t *endpoint) {
    if (!_z_endpoint_udp_unicast_matches(endpoint)) {
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
    _z_udp_unicast_link_state_t *state = _z_udp_unicast_link_state(self);
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    z_result_t ret = _z_udp_unicast_open(&state->_udp._sock, state->_udp._rep, tout);
    if (ret == _Z_RES_OK) {
        ret = _z_link_socket_peer_from_socket(&self->_peer, state->_udp._sock, _z_udp_unicast_close,
                                              &_z_udp_unicast_peer_ops);
        if (ret != _Z_RES_OK) {
            _z_udp_unicast_close(&state->_udp._sock);
        }
    }
    return ret;
}

z_result_t _z_f_link_listen_udp_unicast(_z_link_t *self) {
    _z_udp_unicast_link_state_t *state = _z_udp_unicast_link_state(self);
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    z_result_t ret = _z_udp_unicast_listen(&state->_udp._sock, state->_udp._rep, tout);
    if (ret == _Z_RES_OK) {
        ret = _z_link_socket_peer_from_socket(&self->_peer, state->_udp._sock, _z_udp_unicast_close,
                                              &_z_udp_unicast_peer_ops);
        if (ret != _Z_RES_OK) {
            _z_udp_unicast_close(&state->_udp._sock);
        }
    }
    return ret;
}

void _z_f_link_close_udp_unicast(_z_link_t *self) { _z_link_peer_close(&self->_peer); }

void _z_f_link_free_udp_unicast(_z_link_t *self) {
    _z_udp_unicast_link_state_t *state = _z_udp_unicast_link_state(self);
    if (state != NULL) {
        _z_udp_unicast_endpoint_clear(&state->_udp._rep);
    }
}

size_t _z_f_link_write_udp_unicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_unicast_link_state_t *state = _z_udp_unicast_link_state_const(self);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return (socket == NULL) || (state == NULL) ? SIZE_MAX : _z_udp_unicast_write(*socket, ptr, len, state->_udp._rep);
}

size_t _z_f_link_write_all_udp_unicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_unicast_link_state_t *state = _z_udp_unicast_link_state_const(self);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return (socket == NULL) || (state == NULL) ? SIZE_MAX : _z_udp_unicast_write(*socket, ptr, len, state->_udp._rep);
}

size_t _z_f_link_read_udp_unicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return socket == NULL ? SIZE_MAX : _z_udp_unicast_read(*socket, ptr, len);
}

size_t _z_f_link_read_exact_udp_unicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    return socket == NULL ? SIZE_MAX : _z_udp_unicast_read_exact(*socket, ptr, len);
}

uint16_t _z_get_link_mtu_udp_unicast(void) {
    // @TODO: the return value should change depending on the target platform.
    return 1450;
}

static size_t _z_link_peer_read_udp_unicast(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr,
                                            size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    return socket == NULL ? SIZE_MAX : _z_udp_unicast_read(*socket, ptr, len);
}

static size_t _z_link_peer_write_udp_unicast(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr,
                                             size_t len) {
    const _z_udp_unicast_link_state_t *state = _z_udp_unicast_link_state_const(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    if ((state == NULL) || (socket == NULL)) {
        return SIZE_MAX;
    }
    return _z_udp_unicast_write(*socket, ptr, len, state->_udp._rep);
}

static z_result_t _z_f_link_peer_from_link_udp_unicast(const _z_link_t *link, _z_link_peer_t *peer) {
    if ((link == NULL) || (peer == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    *peer = _z_link_peer_clone(&link->_peer);
    return _z_link_peer_check(peer) ? _Z_RES_OK : _Z_ERR_GENERIC;
}

z_result_t _z_new_link_udp_unicast(_z_link_t *zl, _z_endpoint_t endpoint) {
    _Z_RETURN_IF_ERR(_z_endpoint_udp_unicast_valid(&endpoint));

    _z_udp_unicast_link_state_t *state = (_z_udp_unicast_link_state_t *)z_malloc(sizeof(_z_udp_unicast_link_state_t));
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(state, 0, sizeof(_z_udp_unicast_link_state_t));

    z_result_t ret = _z_udp_unicast_endpoint_init_from_address(&state->_udp._rep, &endpoint._locator._address);
    if (ret != _Z_RES_OK) {
        z_free(state);
        return ret;
    }

    zl->_state = state;
    zl->_drop_f = z_free;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    zl->_cap._is_reliable = false;

    zl->_mtu = _z_get_link_mtu_udp_unicast();

    zl->_endpoint = endpoint;

    zl->_open_f = _z_f_link_open_udp_unicast;
    zl->_listen_f = _z_f_link_listen_udp_unicast;
    zl->_close_f = _z_f_link_close_udp_unicast;
    zl->_free_f = _z_f_link_free_udp_unicast;

    zl->_write_f = _z_f_link_write_udp_unicast;
    zl->_write_all_f = _z_f_link_write_all_udp_unicast;
    zl->_read_f = _z_f_link_read_udp_unicast;
    zl->_read_exact_f = _z_f_link_read_exact_udp_unicast;
    zl->_wait_peers_readable_f = _z_link_socket_wait_peers_readable;
    zl->_peer_from_link_f = _z_f_link_peer_from_link_udp_unicast;

    return ret;
}
#endif
