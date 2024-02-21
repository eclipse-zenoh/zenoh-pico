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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/config/raweth.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/system/link/raweth.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/raweth/config.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_RAWETH_TRANSPORT == 1

#define RAWETH_CONFIG_ARGC 4

#define RAWETH_CONFIG_IFACE_KEY 0x01
#define RAWETH_CONFIG_IFACE_STR "iface"

#define RAWETH_CONFIG_ETHTYPE_KEY 0x02
#define RAWETH_CONFIG_ETHTYPE_STR "ethtype"

#define RAWETH_CONFIG_MAPPING_KEY 0x03
#define RAWETH_CONFIG_MAPPING_STR "mapping"

#define RAWETH_CONFIG_WHITELIST_KEY 0x04
#define RAWETH_CONFIG_WHITELIST_STR "whitelist"

#define RAWETH_CONFIG_MAPPING_BUILD               \
    _z_str_intmapping_t args[RAWETH_CONFIG_ARGC]; \
    args[0]._key = RAWETH_CONFIG_IFACE_KEY;       \
    args[0]._str = RAWETH_CONFIG_IFACE_STR;       \
    args[1]._key = RAWETH_CONFIG_ETHTYPE_KEY;     \
    args[1]._str = RAWETH_CONFIG_ETHTYPE_STR;     \
    args[2]._key = RAWETH_CONFIG_MAPPING_KEY;     \
    args[2]._str = RAWETH_CONFIG_MAPPING_STR;     \
    args[3]._key = RAWETH_CONFIG_WHITELIST_KEY;   \
    args[3]._str = RAWETH_CONFIG_WHITELIST_STR;

static _Bool _z_valid_iface_raweth(_z_str_intmap_t *config) {
    const char *iface = _z_str_intmap_get(config, RAWETH_CONFIG_IFACE_KEY);
    return (iface != NULL);
}

static const char *_z_get_iface_raweth(_z_str_intmap_t *config) {
    return _z_str_intmap_get(config, RAWETH_CONFIG_IFACE_KEY);
}

static _Bool _z_valid_ethtype_raweth(_z_str_intmap_t *config) {
    const char *s_ethtype = _z_str_intmap_get(config, RAWETH_CONFIG_ETHTYPE_KEY);
    if (s_ethtype == NULL) {
        return false;
    }
    int ethtype = atoi(s_ethtype);
    return ((ethtype & 0xff) > 0x6);  // Ethtype must be above 0x600 in network order
}

static int _z_get_ethtype_raweth(_z_str_intmap_t *config) {
    const char *s_ethtype = _z_str_intmap_get(config, RAWETH_CONFIG_ETHTYPE_KEY);
    return atoi(s_ethtype);
}

static _Bool _z_valid_address_raweth(const char *address) {
    // Check if the string has the correct length
    size_t len = strlen(address);
    if (len != 17) {  // 6 pairs of hexadecimal digits and 5 colons
        return false;
    }
    // Check if the colons are at the correct positions
    for (int i = 2; i < len; i += 3) {
        if (address[i] != ':') {
            return false;
        }
    }
    // Check if each character is a valid hexadecimal digit
    for (int i = 0; i < len; ++i) {
        if (i % 3 != 2) {
            char c = address[i];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                return false;
            }
        }
    }
    return true;
}

static uint8_t *_z_parse_address_raweth(const char *address) {
    size_t len = strlen(address);
    // Allocate data
    uint8_t *ret = (uint8_t *)zp_malloc(_ZP_MAC_ADDR_LENGTH);
    if (ret == NULL) {
        return ret;
    }
    for (int i = 0; i < _ZP_MAC_ADDR_LENGTH; ++i) {
        // Extract a pair of hexadecimal digits from the MAC address string
        char byteString[3];
        strncpy(byteString, address + i * 3, 2);
        byteString[2] = '\0';
        // Convert the hexadecimal string to an integer
        ret[i] = (unsigned char)strtol(byteString, NULL, 16);
    }
    return ret;
}

static int8_t _z_f_link_open_raweth(_z_link_t *self) {
    // Init socket smac
    if (_z_valid_address_raweth(self->_endpoint._locator._address)) {
        uint8_t *addr = _z_parse_address_raweth(self->_endpoint._locator._address);
        memcpy(&self->_socket._raweth._smac, addr, _ZP_MAC_ADDR_LENGTH);
        zp_free(addr);
    } else {
        memcpy(&self->_socket._raweth._smac, _ZP_RAWETH_CFG_SMAC, _ZP_MAC_ADDR_LENGTH);
    }
    // Init socket interface
    if (_z_valid_iface_raweth(&self->_endpoint._config)) {
        self->_socket._raweth._interface = _z_get_iface_raweth(&self->_endpoint._config);
    } else {
        self->_socket._raweth._interface = _ZP_RAWETH_CFG_INTERFACE;
    }
    // Init socket ethtype
    if (_z_valid_ethtype_raweth(&self->_endpoint._config)) {
        self->_socket._raweth._ethtype = _z_get_ethtype_raweth(&self->_endpoint._config);
    } else {
        self->_socket._raweth._ethtype = _ZP_RAWETH_CFG_ETHTYPE;
    }
    // Open raweth link
    return _z_open_raweth(&self->_socket._raweth._sock, self->_socket._raweth._interface);
}

static int8_t _z_f_link_listen_raweth(_z_link_t *self) { return _z_f_link_open_raweth(self); }

static void _z_f_link_close_raweth(_z_link_t *self) { _z_close_raweth(&self->_socket._raweth._sock); }

static void _z_f_link_free_raweth(_z_link_t *self) { _ZP_UNUSED(self); }

static size_t _z_f_link_write_raweth(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    _ZP_UNUSED(self);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    return SIZE_MAX;
}

static size_t _z_f_link_write_all_raweth(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    _ZP_UNUSED(self);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    return SIZE_MAX;
}

static size_t _z_f_link_read_raweth(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    _ZP_UNUSED(self);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    _ZP_UNUSED(addr);
    return SIZE_MAX;
}

static size_t _z_f_link_read_exact_raweth(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr) {
    _ZP_UNUSED(self);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    _ZP_UNUSED(addr);
    return SIZE_MAX;
}

static uint16_t _z_get_link_mtu_raweth(void) { return _ZP_MAX_ETH_FRAME_SIZE; }

int8_t _z_endpoint_raweth_valid(_z_endpoint_t *endpoint) {
    int8_t ret = _Z_RES_OK;

    // Check the root
    if (!_z_str_eq(endpoint->_locator._protocol, RAWETH_SCHEMA)) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }
    return ret;
}

int8_t _z_new_link_raweth(_z_link_t *zl, _z_endpoint_t endpoint) {
    int8_t ret = _Z_RES_OK;

    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_RAWETH;
    zl->_cap._is_reliable = false;
    zl->_mtu = _z_get_link_mtu_raweth();

    zl->_endpoint = endpoint;

    // Init socket
    memset(&zl->_socket._raweth, 0, sizeof(zl->_socket._raweth));

    zl->_open_f = _z_f_link_open_raweth;
    zl->_listen_f = _z_f_link_listen_raweth;
    zl->_close_f = _z_f_link_close_raweth;
    zl->_free_f = _z_f_link_free_raweth;

    zl->_write_f = _z_f_link_write_raweth;
    zl->_write_all_f = _z_f_link_write_all_raweth;
    zl->_read_f = _z_f_link_read_raweth;
    zl->_read_exact_f = _z_f_link_read_exact_raweth;

    return ret;
}

size_t _z_raweth_config_strlen(const _z_str_intmap_t *s) {
    RAWETH_CONFIG_MAPPING_BUILD
    return _z_str_intmap_strlen(s, RAWETH_CONFIG_ARGC, args);
}
char *_z_raweth_config_to_str(const _z_str_intmap_t *s) {
    RAWETH_CONFIG_MAPPING_BUILD
    return _z_str_intmap_to_str(s, RAWETH_CONFIG_ARGC, args);
}

int8_t _z_raweth_config_from_str(_z_str_intmap_t *strint, const char *s) {
    RAWETH_CONFIG_MAPPING_BUILD
    return _z_str_intmap_from_strn(strint, s, RAWETH_CONFIG_ARGC, args, strlen(s));
}

#else
int8_t _z_endpoint_raweth_valid(_z_endpoint_t *endpoint) {
    _ZP_UNUSED(endpoint);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _z_new_link_raweth(_z_link_t *zl, _z_endpoint_t endpoint) {
    _ZP_UNUSED(zl);
    _ZP_UNUSED(endpoint);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

size_t _z_raweth_config_strlen(const _z_str_intmap_t *s) {
    _ZP_UNUSED(s);
    return 0;
}
char *_z_raweth_config_to_str(const _z_str_intmap_t *s) {
    _ZP_UNUSED(s);
    return NULL;
}

int8_t _z_raweth_config_from_str(_z_str_intmap_t *strint, const char *s) {
    _ZP_UNUSED(strint);
    _ZP_UNUSED(s);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif
