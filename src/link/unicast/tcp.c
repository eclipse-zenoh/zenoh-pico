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

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/system/link/tcp.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_LINK_TCP == 1

char *_z_parse_port_segment_tcp(char *address) {
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

char *_z_parse_address_segment_tcp(char *address) {
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

int8_t _z_f_link_open_tcp(_z_link_t *self) {
    int8_t ret = 0;

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, TCP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) {
        tout = strtoul(tout_as_str, NULL, 10);
    }

    self->_socket._tcp._sock = _z_open_tcp(self->_socket._tcp._rep, tout);
    if (self->_socket._tcp._sock._err == true) {
        ret = -1;
    }

    return ret;
}

int8_t _z_f_link_listen_tcp(_z_link_t *self) {
    int8_t ret = 0;

    self->_socket._tcp._sock = _z_listen_tcp(self->_socket._tcp._rep);
    if (self->_socket._tcp._sock._err == true) {
        ret = -1;
    }

    return ret;
}

void _z_f_link_close_tcp(_z_link_t *self) { _z_close_tcp(self->_socket._tcp._sock); }

void _z_f_link_free_tcp(_z_link_t *self) { _z_free_endpoint_tcp(self->_socket._tcp._rep); }

size_t _z_f_link_write_tcp(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_tcp(self->_socket._tcp._sock, ptr, len);
}

size_t _z_f_link_write_all_tcp(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_tcp(self->_socket._tcp._sock, ptr, len);
}

size_t _z_f_link_read_tcp(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    (void)(addr);
    return _z_read_tcp(self->_socket._tcp._sock, ptr, len);
}

size_t _z_f_link_read_exact_tcp(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    (void)(addr);
    return _z_read_exact_tcp(self->_socket._tcp._sock, ptr, len);
}

uint16_t _z_get_link_mtu_tcp(void) {
    // Maximum MTU for TCP
    return 65535;
}

_z_link_t *_z_new_link_tcp(_z_endpoint_t endpoint) {
    _z_link_t *lt = (_z_link_t *)z_malloc(sizeof(_z_link_t));

    lt->_capabilities = Z_LINK_CAPABILITY_RELIEABLE | Z_LINK_CAPABILITY_STREAMED;
    lt->_mtu = _z_get_link_mtu_tcp();

    lt->_endpoint = endpoint;

    lt->_socket._tcp._sock._err = true;
    char *s_addr = _z_parse_address_segment_tcp(endpoint._locator._address);
    char *s_port = _z_parse_port_segment_tcp(endpoint._locator._address);
    lt->_socket._tcp._rep = _z_create_endpoint_tcp(s_addr, s_port);
    z_free(s_addr);
    z_free(s_port);

    lt->_open_f = _z_f_link_open_tcp;
    lt->_listen_f = _z_f_link_listen_tcp;
    lt->_close_f = _z_f_link_close_tcp;
    lt->_free_f = _z_f_link_free_tcp;

    lt->_write_f = _z_f_link_write_tcp;
    lt->_write_all_f = _z_f_link_write_all_tcp;
    lt->_read_f = _z_f_link_read_tcp;
    lt->_read_exact_f = _z_f_link_read_exact_tcp;

    return lt;
}
#endif
