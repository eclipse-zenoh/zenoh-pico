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
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/system/link/udp.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_LINK_UDP_UNICAST == 1

char *__z_parse_port_segment_udp_unicast(char *address) {
    char *ret = NULL;

    const char *p_start = strrchr(address, ':');
    if (p_start != NULL) {
        p_start = _z_cptr_char_offset(p_start, 1);

        const char *p_end = &address[strlen(address)];

        size_t len = _z_ptr_char_diff(p_end, p_start);
        ret = (char *)z_malloc(len + (size_t)1);
        if (ret != NULL) {
            (void)strncpy(ret, p_start, len);
            ret[len] = '\0';
        }
    }

    return ret;
}

char *__z_parse_address_segment_udp_unicast(char *address) {
    char *ret = NULL;

    const char *p_start = &address[0];
    const char *p_end = strrchr(address, ':');

    // IPv6
    if ((p_start[0] == '[') && (p_end[-1] == ']')) {
        p_start = _z_cptr_char_offset(p_start, 1);
        p_end = _z_cptr_char_offset(p_end, -1);
        size_t len = _z_ptr_char_diff(p_end, p_start);
        ret = (char *)z_malloc(len + (size_t)1);
        if (ret != NULL) {
            (void)strncpy(ret, p_start, len);
            ret[len] = '\0';
        }
    }
    // IPv4
    else {
        size_t len = _z_ptr_char_diff(p_end, p_start);
        ret = (char *)z_malloc(len + (size_t)1);
        if (ret != NULL) {
            (void)strncpy(ret, p_start, len);
            ret[len] = '\0';
        }
    }

    return ret;
}

int8_t _z_endpoint_udp_unicast_valid(_z_endpoint_t *endpoint) {
    int8_t ret = _Z_RES_OK;

    if (_z_str_eq(endpoint->_locator._protocol, UDP_SCHEMA) != true) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    char *s_addr = __z_parse_address_segment_udp_unicast(endpoint->_locator._address);
    if (s_addr == NULL) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    } else {
        z_free(s_addr);
    }

    char *s_port = __z_parse_port_segment_udp_unicast(endpoint->_locator._address);
    if (s_port == NULL) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    } else {
        uint32_t port = strtoul(s_port, NULL, 10);
        if ((port < (uint32_t)1) || (port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
            ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
        }
        z_free(s_port);
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

    ret = _z_open_udp_unicast(&self->_socket._udp._sock, self->_socket._udp._rep, tout);

    return ret;
}

int8_t _z_f_link_listen_udp_unicast(_z_link_t *self) {
    int8_t ret = _Z_RES_OK;

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = strtoul(tout_as_str, NULL, 10);
    }

    ret = _z_listen_udp_unicast(&self->_socket._udp._sock, self->_socket._udp._rep, tout);

    return ret;
}

void _z_f_link_close_udp_unicast(_z_link_t *self) { _z_close_udp_unicast(&self->_socket._udp._sock); }

void _z_f_link_free_udp_unicast(_z_link_t *self) { _z_free_endpoint_udp(&self->_socket._udp._rep); }

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

int8_t _z_new_link_udp_unicast(_z_link_t *zl, _z_endpoint_t endpoint) {
    int8_t ret = _Z_RES_OK;

    zl->_capabilities = Z_LINK_CAPABILITY_NONE;
    zl->_mtu = _z_get_link_mtu_udp_unicast();

    zl->_endpoint = endpoint;
    char *s_addr = __z_parse_address_segment_udp_unicast(endpoint._locator._address);
    char *s_port = __z_parse_port_segment_udp_unicast(endpoint._locator._address);
    ret = _z_create_endpoint_udp(&zl->_socket._udp._rep, s_addr, s_port);
    z_free(s_addr);
    z_free(s_port);

    zl->_open_f = _z_f_link_open_udp_unicast;
    zl->_listen_f = _z_f_link_listen_udp_unicast;
    zl->_close_f = _z_f_link_close_udp_unicast;
    zl->_free_f = _z_f_link_free_udp_unicast;

    zl->_write_f = _z_f_link_write_udp_unicast;
    zl->_write_all_f = _z_f_link_write_all_udp_unicast;
    zl->_read_f = _z_f_link_read_udp_unicast;
    zl->_read_exact_f = _z_f_link_read_exact_udp_unicast;

    return ret;
}
#endif