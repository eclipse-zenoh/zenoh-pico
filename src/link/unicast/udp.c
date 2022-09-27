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

#if Z_LINK_UDP_UNICAST == 1

char *_z_parse_port_segment_udp_unicast(char *address) {
    char *p_start = strrchr(address, ':');
    if (p_start == NULL) return NULL;
    p_start++;

    char *p_end = &address[strlen(address)];

    int len = p_end - p_start;
    char *port = (char *)z_malloc((len + 1) * (int)sizeof(char));
    strncpy(port, p_start, len);
    port[len] = '\0';

    return port;
}

char *_z_parse_address_segment_udp_unicast(char *address) {
    char *p_start = &address[0];
    char *p_end = strrchr(address, ':');

    if (*p_start == '[' && *(p_end - 1) == ']') {
        p_start++;
        p_end--;
        int len = p_end - p_start;
        char *ip6_addr = (char *)z_malloc((len + 1) * (int)sizeof(char));
        strncpy(ip6_addr, p_start, len);
        ip6_addr[len] = '\0';

        return ip6_addr;
    } else {
        int len = p_end - p_start;
        char *ip4_addr_or_domain = (char *)z_malloc((len + 1) * (int)sizeof(char));
        strncpy(ip4_addr_or_domain, p_start, len);
        ip4_addr_or_domain[len] = '\0';

        return ip4_addr_or_domain;
    }

    return NULL;
}

int _z_f_link_open_udp_unicast(void *arg) {
    _z_link_t *self = (_z_link_t *)arg;

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) tout = strtoul(tout_as_str, NULL, 10);

    self->_socket._udp._sock = _z_open_udp_unicast(self->_socket._udp._raddr, tout);
    if (self->_socket._udp._sock == NULL) goto ERR;

    return 0;

ERR:
    return -1;
}

int _z_f_link_listen_udp_unicast(void *arg) {
    _z_link_t *self = (_z_link_t *)arg;

    uint32_t tout = Z_CONFIG_SOCKET_TIMEOUT;
    char *tout_as_str = _z_str_intmap_get(&self->_endpoint._config, UDP_CONFIG_TOUT_KEY);
    if (tout_as_str != NULL) tout = strtoul(tout_as_str, NULL, 10);

    self->_socket._udp._sock = _z_listen_udp_unicast(self->_socket._udp._raddr, tout);
    if (self->_socket._udp._sock == NULL) goto ERR;

    return 0;

ERR:
    return -1;
}

void _z_f_link_close_udp_unicast(void *arg) {
    _z_link_t *self = (_z_link_t *)arg;

    _z_close_udp_unicast(self->_socket._udp._sock);
}

void _z_f_link_free_udp_unicast(void *arg) {
    _z_link_t *self = (_z_link_t *)arg;

    _z_free_endpoint_udp(self->_socket._udp._raddr);
}

size_t _z_f_link_write_udp_unicast(const void *arg, const uint8_t *ptr, size_t len) {
    const _z_link_t *self = (const _z_link_t *)arg;

    return _z_send_udp_unicast(self->_socket._udp._sock, ptr, len, self->_socket._udp._raddr);
}

size_t _z_f_link_write_all_udp_unicast(const void *arg, const uint8_t *ptr, size_t len) {
    const _z_link_t *self = (const _z_link_t *)arg;

    return _z_send_udp_unicast(self->_socket._udp._sock, ptr, len, self->_socket._udp._raddr);
}

size_t _z_f_link_read_udp_unicast(const void *arg, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    (void)(addr);
    const _z_link_t *self = (const _z_link_t *)arg;

    return _z_read_udp_unicast(self->_socket._udp._sock, ptr, len);
}

size_t _z_f_link_read_exact_udp_unicast(const void *arg, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    (void)(addr);
    const _z_link_t *self = (const _z_link_t *)arg;

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

    lt->_socket._udp._sock = NULL;
    lt->_socket._udp._msock = NULL;
    char *s_addr = _z_parse_address_segment_udp_unicast(endpoint._locator._address);
    char *s_port = _z_parse_port_segment_udp_unicast(endpoint._locator._address);
    lt->_socket._udp._raddr = _z_create_endpoint_udp(s_addr, s_port);
    lt->_socket._udp._laddr = NULL;
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