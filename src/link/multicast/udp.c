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
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/driver.h"
#include "zenoh-pico/link/transport/udp_multicast.h"
#include "zenoh-pico/link/transport/udp_unicast.h"

#if Z_FEATURE_LINK_UDP_MULTICAST == 1

typedef struct {
    _z_udp_socket_t _udp;
} _z_udp_multicast_link_state_t;

static _z_udp_multicast_link_state_t *_z_udp_multicast_link_state(_z_link_t *link) {
    return (_z_udp_multicast_link_state_t *)_z_link_state(link);
}

static const _z_udp_multicast_link_state_t *_z_udp_multicast_link_state_const(const _z_link_t *link) {
    return (const _z_udp_multicast_link_state_t *)_z_link_state_const(link);
}

z_result_t _z_endpoint_udp_multicast_valid(_z_endpoint_t *endpoint) {
    _z_string_t udp_str = _z_string_alias_str(UDP_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &udp_str)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    z_result_t ret = _z_udp_unicast_address_valid(&endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    const char *iface = _z_str_intmap_get(&endpoint->_config, UDP_CONFIG_IFACE_KEY);
    if (iface == NULL) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return _Z_RES_OK;
}

z_result_t _z_f_link_open_udp_multicast(_z_link_t *self) {
    _z_udp_multicast_link_state_t *state = _z_udp_multicast_link_state(self);
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = (uint32_t)strtoul(tout_as_str, NULL, 10);
    }

    const char *iface = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_IFACE_KEY);
    return _z_udp_multicast_open(&state->_udp._sock, state->_udp._rep, &state->_udp._lep, tout, iface);
}

z_result_t _z_f_link_listen_udp_multicast(_z_link_t *self) {
    _z_udp_multicast_link_state_t *state = _z_udp_multicast_link_state(self);
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    z_result_t ret = _Z_RES_OK;

    const char *iface = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_IFACE_KEY);
    const char *join = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_JOIN_KEY);
    ret = _z_udp_multicast_listen(&state->_udp._sock, state->_udp._rep, Z_CONFIG_SOCKET_TIMEOUT, iface, join);
    ret |=
        _z_udp_multicast_open(&state->_udp._msock, state->_udp._rep, &state->_udp._lep, Z_CONFIG_SOCKET_TIMEOUT, iface);

    return ret;
}

void _z_f_link_close_udp_multicast(_z_link_t *self) {
    _z_udp_multicast_link_state_t *state = _z_udp_multicast_link_state(self);
    if (state != NULL) {
        _z_udp_multicast_close(&state->_udp._sock, &state->_udp._msock, state->_udp._rep, state->_udp._lep);
    }
}

static void _z_udp_multicast_link_state_drop(void *arg) {
    _z_udp_multicast_link_state_t *state = (_z_udp_multicast_link_state_t *)arg;
    if (state != NULL) {
        _z_udp_multicast_endpoint_clear(&state->_udp._lep);
        _z_udp_multicast_endpoint_clear(&state->_udp._rep);
        z_free(state);
    }
}

size_t _z_f_link_write_udp_multicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_multicast_link_state_t *state = _z_udp_multicast_link_state_const(self);
    return state == NULL ? SIZE_MAX : _z_udp_multicast_write(state->_udp._msock, ptr, len, state->_udp._rep);
}

size_t _z_f_link_write_all_udp_multicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_udp_multicast_link_state_t *state = _z_udp_multicast_link_state_const(self);
    return state == NULL ? SIZE_MAX : _z_udp_multicast_write(state->_udp._msock, ptr, len, state->_udp._rep);
}

size_t _z_f_link_read_udp_multicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    const _z_udp_multicast_link_state_t *state = _z_udp_multicast_link_state_const(self);
    return state == NULL ? SIZE_MAX : _z_udp_multicast_read(state->_udp._sock, ptr, len, state->_udp._lep, addr);
}

size_t _z_f_link_read_exact_udp_multicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    const _z_udp_multicast_link_state_t *state = _z_udp_multicast_link_state_const(self);
    return state == NULL ? SIZE_MAX : _z_udp_multicast_read_exact(state->_udp._sock, ptr, len, state->_udp._lep, addr);
}

uint16_t _z_get_link_mtu_udp_multicast(void) {
    // @TODO: the return value should change depending on the target platform.
    return 1450;
}

z_result_t _z_new_link_udp_multicast(_z_link_t *zl, _z_endpoint_t endpoint) {
    _z_udp_multicast_link_state_t *state =
        (_z_udp_multicast_link_state_t *)z_malloc(sizeof(_z_udp_multicast_link_state_t));
    if (state == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(state, 0, sizeof(_z_udp_multicast_link_state_t));

    _Z_CLEAN_RETURN_IF_ERR(_z_udp_multicast_endpoint_init_from_address(&state->_udp._rep, &endpoint._locator._address),
                           z_free(state));
    memset(&state->_udp._lep, 0, sizeof(state->_udp._lep));

    zl->_state = state;
    zl->_state_drop_f = _z_udp_multicast_link_state_drop;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_MULTICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    zl->_cap._is_reliable = false;

    zl->_mtu = _z_get_link_mtu_udp_multicast();

    zl->_endpoint = endpoint;

    zl->_close_f = _z_f_link_close_udp_multicast;

    zl->_write_f = _z_f_link_write_udp_multicast;
    zl->_write_all_f = _z_f_link_write_all_udp_multicast;
    zl->_read_f = _z_f_link_read_udp_multicast;
    zl->_read_exact_f = _z_f_link_read_exact_udp_multicast;
    zl->_wait_peers_readable_f = NULL;

    return _Z_RES_OK;
}

static z_result_t _z_link_driver_udp_multicast_create(_z_link_t *link, _z_endpoint_t *endpoint,
                                                      const _z_config_t *session_cfg) {
    _ZP_UNUSED(session_cfg);
    return _z_new_link_udp_multicast(link, *endpoint);
}

const _z_link_driver_t _z_link_driver_udp_multicast = {
    ._validate_f = _z_endpoint_udp_multicast_valid,
    ._create_f = _z_link_driver_udp_multicast_create,
    ._open_f = NULL,
    ._listen_f = _z_f_link_listen_udp_multicast,
};

#endif
