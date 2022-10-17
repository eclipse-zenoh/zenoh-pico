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
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/system/link/udp.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_LINK_UDP_UNICAST == 1

char *_z_parse_port_segment_udp_unicast(char *address) {
    char *ret = NULL;

    const char *p_start = strrchr(address, ':');
    if (p_start != NULL) {
        p_start = _z_cptr_char_offset(p_start, 1);

        const char *p_end = &address[strlen(address)];

        size_t len = _z_ptr_char_diff(p_end, p_start);
        ret = (char *)z_malloc(len + (size_t)1);
        (void)strncpy(ret, p_start, len);
        ret[len] = '\0';
    }

    return ret;
}

char *_z_parse_address_segment_udp_unicast(char *address) {
    char *ret = NULL;

    const char *p_start = &address[0];
    const char *p_end = strrchr(address, ':');

    // IPv6
    if ((p_start[0] == '[') && (p_end[-1] == ']')) {
        p_start = _z_cptr_char_offset(p_start, 1);
        p_end = _z_cptr_char_offset(p_end, -1);
        size_t len = _z_ptr_char_diff(p_end, p_start);
        ret = (char *)z_malloc(len + (size_t)1);
        (void)strncpy(ret, p_start, len);
        ret[len] = '\0';
    }
    // IPv4
    else {
        size_t len = _z_ptr_char_diff(p_end, p_start);
        ret = (char *)z_malloc(len + (size_t)1);
        (void)strncpy(ret, p_start, len);
        ret[len] = '\0';
    }

    return ret;
}

int8_t _z_f_link_open_udp_unicast(_z_link_t *self) {
    int8_t ret = _Z_RES_OK;

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = strtoul(tout_as_str, NULL, 10);
    }

    self->_socket._udp._sock = _z_open_udp_unicast(self->_socket._udp._rep, tout);
    if (self->_socket._udp._sock._err == true) {
        ret = -1;
    }

    return ret;
}

int8_t _z_f_link_listen_udp_unicast(_z_link_t *self) {
    int8_t ret = _Z_RES_OK;

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = strtoul(tout_as_str, NULL, 10);
    }

    self->_socket._udp._sock = _z_listen_udp_unicast(self->_socket._udp._rep, tout);
    if (self->_socket._udp._sock._err == true) {
        ret = -1;
    }

    return ret;
}

void _z_f_link_close_udp_unicast(_z_link_t *self) { _z_close_udp_unicast(self->_socket._udp._sock); }

void _z_f_link_free_udp_unicast(_z_link_t *self) { _z_free_endpoint_udp(self->_socket._udp._rep); }

size_t _z_f_link_write_udp_unicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_udp_unicast(self->_socket._udp._sock, ptr, len, self->_socket._udp._rep);
}

size_t _z_f_link_write_all_udp_unicast(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_udp_unicast(self->_socket._udp._sock, ptr, len, self->_socket._udp._rep);
}

size_t _z_f_link_read_udp_unicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    (void)(addr);
    return _z_read_udp_unicast(self->_socket._udp._sock, ptr, len);
}

size_t _z_f_link_read_exact_udp_unicast(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    (void)(addr);
    return _z_read_exact_udp_unicast(self->_socket._udp._sock, ptr, len);
}

uint16_t _z_get_link_mtu_udp_unicast(void) {
    // @TODO: the return value should change depending on the target platform.
    return 1450;
}

_z_link_t *_z_new_link_udp_unicast(_z_endpoint_t endpoint) {
    _z_link_t *lt = (_z_link_t *)z_malloc(sizeof(_z_link_t));

    lt->_capabilities = Z_LINK_CAPABILITY_NONE;
    lt->_mtu = _z_get_link_mtu_udp_unicast();

    lt->_endpoint = endpoint;

    lt->_socket._udp._sock._err = true;
    lt->_socket._udp._msock._err = true;
    char *s_addr = _z_parse_address_segment_udp_unicast(endpoint._locator._address);
    char *s_port = _z_parse_port_segment_udp_unicast(endpoint._locator._address);
    lt->_socket._udp._rep = _z_create_endpoint_udp(s_addr, s_port);
    lt->_socket._udp._lep._err = true;
    z_free(s_addr);
    z_free(s_port);

    lt->_open_f = _z_f_link_open_udp_unicast;
    lt->_listen_f = _z_f_link_listen_udp_unicast;
    lt->_close_f = _z_f_link_close_udp_unicast;
    lt->_free_f = _z_f_link_free_udp_unicast;

    lt->_write_f = _z_f_link_write_udp_unicast;
    lt->_write_all_f = _z_f_link_write_all_udp_unicast;
    lt->_read_f = _z_f_link_read_udp_unicast;
    lt->_read_exact_f = _z_f_link_read_exact_udp_unicast;

    return lt;
}
#endif