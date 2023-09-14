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
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/system/link/ws.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_LINK_WS == 1

char *__z_parse_port_segment_ws(char *address) {
    char *ret = NULL;

    const char *p_start = strrchr(address, ':');
    if (p_start != NULL) {
        p_start = _z_cptr_char_offset(p_start, 1);

        const char *p_end = &address[strlen(address)];

        size_t len = _z_ptr_char_diff(p_end, p_start) + (size_t)1;
        ret = (char *)z_malloc(len);
        if (ret != NULL) {
            _z_str_n_copy(ret, p_start, len);
        }
    }

    return ret;
}

char *__z_parse_address_segment_ws(char *address) {
    char *ret = NULL;

    const char *p_start = &address[0];
    const char *p_end = strrchr(address, ':');

    // IPv6
    if ((p_start[0] == '[') && (p_end[-1] == ']')) {
        p_start = _z_cptr_char_offset(p_start, 1);
        p_end = _z_cptr_char_offset(p_end, -1);
        size_t len = _z_ptr_char_diff(p_end, p_start) + (size_t)1;
        ret = (char *)z_malloc(len);
        if (ret != NULL) {
            _z_str_n_copy(ret, p_start, len);
        }
    }
    // IPv4
    else {
        size_t len = _z_ptr_char_diff(p_end, p_start) + (size_t)1;
        ret = (char *)z_malloc(len);
        if (ret != NULL) {
            _z_str_n_copy(ret, p_start, len);
        }
    }

    return ret;
}

int8_t _z_endpoint_ws_valid(_z_endpoint_t *endpoint) {
    int8_t ret = _Z_RES_OK;

    if (_z_str_eq(endpoint->_locator._protocol, WS_SCHEMA) != true) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    if (ret == _Z_RES_OK) {
        char *s_addr = __z_parse_address_segment_ws(endpoint->_locator._address);
        if (s_addr == NULL) {
            ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
        } else {
            z_free(s_addr);
        }
    }

    if (ret == _Z_RES_OK) {
        char *s_port = __z_parse_port_segment_ws(endpoint->_locator._address);
        if (s_port == NULL) {
            ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
        } else {
            uint32_t port = strtoul(s_port, NULL, 10);
            if ((port < (uint32_t)1) || (port > (uint32_t)65355)) {  // Port numbers should range from 1 to 65355
                ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
            }
            z_free(s_port);
        }
    }

    return ret;
}

int8_t _z_f_link_open_ws(_z_link_t *zl) {
    int8_t ret = _Z_RES_OK;

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&zl->_endpoint._config, WS_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = strtoul(tout_as_str, NULL, 10);
    }

    ret = _z_open_ws(&zl->_socket._ws._sock, zl->_socket._ws._rep, tout);

    return ret;
}

int8_t _z_f_link_listen_ws(_z_link_t *zl) {
    int8_t ret = _Z_RES_OK;

    ret = _z_listen_ws(&zl->_socket._ws._sock, zl->_socket._ws._rep);

    return ret;
}

void _z_f_link_close_ws(_z_link_t *zl) { _z_close_ws(&zl->_socket._ws._sock); }

void _z_f_link_free_ws(_z_link_t *zl) { _z_free_endpoint_ws(&zl->_socket._ws._rep); }

size_t _z_f_link_write_ws(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    return _z_send_ws(zl->_socket._ws._sock, ptr, len);
}

size_t _z_f_link_write_all_ws(const _z_link_t *zl, const uint8_t *ptr, size_t len) {
    return _z_send_ws(zl->_socket._ws._sock, ptr, len);
}

size_t _z_f_link_read_ws(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    (void)(addr);
    return _z_read_ws(zl->_socket._ws._sock, ptr, len);
}

size_t _z_f_link_read_exact_ws(const _z_link_t *zl, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    (void)(addr);
    return _z_read_exact_ws(zl->_socket._ws._sock, ptr, len);
}

uint16_t _z_get_link_mtu_ws(void) {
    // Maximum MTU for TCP
    return 65535;
}

int8_t _z_new_link_ws(_z_link_t *zl, _z_endpoint_t *endpoint) {
    int8_t ret = _Z_RES_OK;

    zl->_capabilities = Z_LINK_CAPABILITY_RELIEABLE;
    zl->_mtu = _z_get_link_mtu_ws();

    zl->_endpoint = *endpoint;
    char *s_addr = __z_parse_address_segment_ws(endpoint->_locator._address);
    char *s_port = __z_parse_port_segment_ws(endpoint->_locator._address);
    ret = _z_create_endpoint_ws(&zl->_socket._ws._rep, s_addr, s_port);
    z_free(s_addr);
    z_free(s_port);

    zl->_open_f = _z_f_link_open_ws;
    zl->_listen_f = _z_f_link_listen_ws;
    zl->_close_f = _z_f_link_close_ws;
    zl->_free_f = _z_f_link_free_ws;

    zl->_write_f = _z_f_link_write_ws;
    zl->_write_all_f = _z_f_link_write_all_ws;
    zl->_read_f = _z_f_link_read_ws;
    zl->_read_exact_f = _z_f_link_read_exact_ws;

    return ret;
}
#endif
