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
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/system/link/raweth.h"
#include "zenoh-pico/system/platform.h"
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

const uint16_t _ZP_RAWETH_DEFAULT_ETHTYPE = 0x72e0;
const char *_ZP_RAWETH_DEFAULT_INTERFACE = "lo";
const uint8_t _ZP_RAWETH_DEFAULT_SMAC[_ZP_MAC_ADDR_LENGTH] = {0x30, 0x03, 0xc8, 0x37, 0x25, 0xa1};
// const _zp_raweth_mapping_entry_t _ZP_RAWETH_DEFAULT_MAPPING[] = {
//     {{0, {0}, ""}, 0x0000, {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}, false},                            // Default mac
//     addr
//     {{0, {0}, "some/key/expr"}, 0x8c00, {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}, true},                // entry1
//     {{0, {0}, "demo/example/zenoh-pico-pub"}, 0xab00, {0x41, 0x55, 0xa8, 0x00, 0x9d, 0xc0}, true},  // entry2
//     {{0, {0}, "another/keyexpr"}, 0x4300, {0x01, 0x23, 0x45, 0x67, 0x89, 0xab}, true},              // entry3
// };
// some/key/expr 01:23:45:67:89:ab 12,another/ke aa:bb:cc:dd:ee:ff

static _Bool _z_valid_iface_raweth(_z_str_intmap_t *config);
static const char *_z_get_iface_raweth(_z_str_intmap_t *config);
static _Bool _z_valid_ethtype_raweth(_z_str_intmap_t *config);
static int _z_get_ethtype_raweth(_z_str_intmap_t *config);
static size_t _z_valid_mapping_raweth(_z_str_intmap_t *config);
static int8_t _z_get_mapping_raweth(_z_str_intmap_t *config, _zp_raweth_mapping_array_t *array, size_t size);
static size_t _z_valid_whitelist_raweth(_z_str_intmap_t *config);
static int8_t _z_get_whitelist_raweth(_z_str_intmap_t *config, _zp_raweth_whitelist_array_t *array, size_t size);
static _Bool _z_valid_address_raweth(const char *address);
static uint8_t *_z_parse_address_raweth(const char *address);
static int8_t _z_f_link_open_raweth(_z_link_t *self);
static int8_t _z_f_link_listen_raweth(_z_link_t *self);
static void _z_f_link_close_raweth(_z_link_t *self);
static void _z_f_link_free_raweth(_z_link_t *self);
static size_t _z_f_link_write_raweth(const _z_link_t *self, const uint8_t *ptr, size_t len);
static size_t _z_f_link_write_all_raweth(const _z_link_t *self, const uint8_t *ptr, size_t len);
static size_t _z_f_link_read_raweth(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr);
static size_t _z_f_link_read_exact_raweth(const _z_link_t *self, uint8_t *ptr, size_t len, _z_bytes_t *addr);
static uint16_t _z_get_link_mtu_raweth(void);

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

static size_t _z_valid_mapping_raweth(_z_str_intmap_t *config) {
    // Retrieve list
    const char *cfg_str = _z_str_intmap_get(config, RAWETH_CONFIG_MAPPING_KEY);
    if (cfg_str == NULL) {
        return 0;
    }
    char *s_mapping = zp_malloc(strlen(cfg_str));
    if (s_mapping == NULL) {
        return 0;
    }
    size_t size = 0;
    strcpy(s_mapping, cfg_str);
    // Parse list
    char delim[] = ",";
    char *entry = strtok(s_mapping, delim);
    while (entry != NULL) {
        // Check entry
        size += 1;
        entry = strtok(s_mapping, delim);
    }
    // Clean up
    zp_free(s_mapping);
    return size;
}

static int8_t _z_get_mapping_raweth(_z_str_intmap_t *config, _zp_raweth_mapping_array_t *array, size_t size) {
    // Retrieve data
    const char *cfg_str = _z_str_intmap_get(config, RAWETH_CONFIG_MAPPING_KEY);
    // Copy data
    // Allocate array
    *array = _zp_raweth_mapping_array_make(size);
    // Store size
    // Copy data
    return _Z_RES_OK;
}

static const size_t _z_valid_whitelist_raweth(_z_str_intmap_t *config) {
    // Retrieve data
    const char *cfg_str = _z_str_intmap_get(config, RAWETH_CONFIG_WHITELIST_KEY);
    if (cfg_str == NULL) {
        return 0;
    }
    // Copy data
    char *s_whitelist = zp_malloc(strlen(cfg_str));
    if (s_whitelist == NULL) {
        return 0;
    }
    strcpy(s_whitelist, cfg_str);
    // Parse list
    size_t size = 0;
    char delim[] = ",";
    char *entry = strtok(s_whitelist, delim);
    while (entry != NULL) {
        // Check entry
        if (!_z_valid_address_raweth(entry)) {
            zp_free(s_whitelist);
            return 0;
        }
        size += 1;
        entry = strtok(s_whitelist, delim);
    }
    // Clean up
    zp_free(s_whitelist);
    return size;
}

static int8_t _z_get_whitelist_raweth(_z_str_intmap_t *config, _zp_raweth_whitelist_array_t *array, size_t size) {
    // Retrieve data
    const char *cfg_str = _z_str_intmap_get(config, RAWETH_CONFIG_WHITELIST_KEY);
    if (cfg_str == NULL) {
        return _Z_ERR_GENERIC;
    }
    // Copy data
    char *s_whitelist = zp_malloc(strlen(cfg_str));
    if (s_whitelist == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    strcpy(s_whitelist, cfg_str);
    // Allocate array
    *array = _zp_raweth_whitelist_array_make(size);
    if (array->_len == 0) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    size_t idx = 0;
    // Parse list
    char delim[] = ",";
    char *entry = strtok(s_whitelist, delim);
    while ((entry != NULL) && (idx < array->_len)) {
        // Convert address from string to int array
        uint8_t *addr = _z_parse_address_raweth(entry);
        if (addr == NULL) {
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
        // Copy address to entry
        _zp_raweth_whitelist_entry_t *elem = _zp_raweth_whitelist_array_get(array, idx);
        memcpy(elem->_mac, addr, _ZP_MAC_ADDR_LENGTH);
        zp_free(addr);
        // Next iteration
        idx += 1;
        entry = strtok(s_whitelist, delim);
    }
    // Clean up
    zp_free(s_whitelist);
    return _Z_RES_OK;
}

static _Bool _z_valid_mapping_entry(const char *entry) {
    // some/key/expr 01:23:45:67:89:ab 12,another/ke aa:bb:cc:dd:ee:ff
    size_t len = strlen(entry);
    size_t idx = 0;
    const char *p_start = &entry[0];
    const char *p_end = NULL;
    char c = '\0';
    do {
        c = entry[idx];
        idx += 1;
    } while ((idx < len) && (c != ' '));
    return false;
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
    // Init arrays
    self->_socket._raweth._mapping = _zp_raweth_mapping_array_empty();
    self->_socket._raweth._whitelist = _zp_raweth_whitelist_array_empty();
    // Init socket smac
    if (_z_valid_address_raweth(self->_endpoint._locator._address)) {
        uint8_t *addr = _z_parse_address_raweth(self->_endpoint._locator._address);
        memcpy(&self->_socket._raweth._smac, addr, _ZP_MAC_ADDR_LENGTH);
        zp_free(addr);
    } else {
        memcpy(&self->_socket._raweth._smac, _ZP_RAWETH_DEFAULT_SMAC, _ZP_MAC_ADDR_LENGTH);
    }
    // Init socket interface
    if (_z_valid_iface_raweth(&self->_endpoint._config)) {
        self->_socket._raweth._interface = _z_get_iface_raweth(&self->_endpoint._config);
    } else {
        self->_socket._raweth._interface = _ZP_RAWETH_DEFAULT_INTERFACE;
    }
    // Init socket ethtype
    if (_z_valid_ethtype_raweth(&self->_endpoint._config)) {
        self->_socket._raweth._ethtype = _z_get_ethtype_raweth(&self->_endpoint._config);
    } else {
        self->_socket._raweth._ethtype = _ZP_RAWETH_DEFAULT_ETHTYPE;
    }
    // Init socket mapping
    size_t size = _z_valid_mapping_raweth(&self->_endpoint._config);
    if (size != 0) {
        _Z_RETURN_IF_ERR(_z_get_mapping_raweth(&self->_endpoint._config, &self->_socket._raweth._mapping, size));
    } else {
        // self->_socket._raweth._mapping = _ZP_RAWETH_DEFAULT_MAPPING;
    }
    // Init socket whitelist
    size = _z_valid_whitelist_raweth(&self->_endpoint._config);
    if (size != 0) {
        _Z_RETURN_IF_ERR(_z_get_whitelist_raweth(&self->_endpoint._config, &self->_socket._raweth._whitelist, size));
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
