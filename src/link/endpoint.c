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

#include "zenoh-pico/link/endpoint.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/utils/pointers.h"
#if Z_LINK_TCP == 1
#include "zenoh-pico/link/config/tcp.h"
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
#include "zenoh-pico/link/config/udp.h"
#endif
#if Z_LINK_BLUETOOTH == 1
#include "zenoh-pico/link/config/bt.h"
#endif
#if Z_LINK_SERIAL == 1
#include "zenoh-pico/link/config/serial.h"
#endif
/*------------------ Locator ------------------*/
void _z_locator_init(_z_locator_t *locator) {
    locator->_protocol = NULL;
    locator->_address = NULL;
    locator->_metadata = _z_str_intmap_make();
}

void _z_locator_clear(_z_locator_t *lc) {
    _z_str_clear(lc->_protocol);
    _z_str_clear(lc->_address);
    _z_str_intmap_clear(&lc->_metadata);
}

void _z_locator_free(_z_locator_t **lc) {
    _z_locator_t *ptr = *lc;
    _z_locator_clear(ptr);

    z_free(ptr);
    *lc = NULL;
}

void _z_locator_copy(_z_locator_t *dst, const _z_locator_t *src) {
    dst->_protocol = _z_str_clone(src->_protocol);
    dst->_address = _z_str_clone(src->_address);

    // @TODO: implement copy for metadata
    dst->_metadata = _z_str_intmap_make();
}

_Bool _z_locator_eq(const _z_locator_t *left, const _z_locator_t *right) {
    _Bool res = false;

    res = _z_str_eq(left->_protocol, right->_protocol);
    if (res == true) {
        res = _z_str_eq(left->_address, right->_address);
        // if (res == true) {
        //     // @TODO: implement eq for metadata
        // }
    }

    return res;
}

char *_z_locator_protocol_from_str(const char *str) {
    char *ret = NULL;

    const char *p_start = &str[0];
    const char *p_end = strchr(p_start, LOCATOR_PROTOCOL_SEPARATOR);
    if ((p_start != NULL) && (p_end != NULL) && (p_start != p_end)) {
        size_t p_len = _z_ptr_char_diff(p_end, p_start);
        ret = (char *)z_malloc(p_len + (size_t)1);
        (void)strncpy(ret, p_start, p_len);
        ret[p_len] = '\0';
    }

    return ret;
}

char *_z_locator_address_from_str(const char *str) {
    char *ret = NULL;

    const char *p_start = strchr(str, LOCATOR_PROTOCOL_SEPARATOR);
    if (p_start != NULL) {
        p_start = _z_cptr_char_offset(p_start, 1);  // Skip protocol separator character

        const char *p_end = strchr(p_start, LOCATOR_METADATA_SEPARATOR);
        if (p_end == NULL) {  // There is no metadata separator, then look for config separator
            p_end = strchr(p_start, ENDPOINT_CONFIG_SEPARATOR);
        }
        if (p_end == NULL) {  // There is no config separator, then address goes to the end of string
            p_end = &str[strlen(str)];
        }

        if (p_start != p_end) {
            size_t a_len = _z_ptr_char_diff(p_end, p_start);
            ret = (char *)z_malloc(a_len + (size_t)1);
            (void)strncpy(ret, p_start, a_len);
            ret[a_len] = '\0';
        }
    }

    return ret;
}

_z_str_intmap_result_t _z_locator_metadata_from_str(const char *str) {
    _z_str_intmap_result_t ret;
    ret._tag = _Z_ERR_PARSE_STRING;
    ret._value = _z_str_intmap_make();

    const char *p_start = strchr(str, LOCATOR_METADATA_SEPARATOR);
    if (p_start != NULL) {
        p_start++;

        const char *p_end = strchr(str, ENDPOINT_CONFIG_SEPARATOR);
        if (p_end == NULL) {
            p_end = &str[strlen(str)];
        }

        if (p_start != p_end) {
            size_t p_len = _z_ptr_char_diff(p_end, p_start);
            ret = _z_str_intmap_from_strn(p_start, 0, NULL, p_len);
        }
    } else {
        ret._tag = _Z_RES_OK;  // Metadata is optional in locator, if its absense still means successful parsing
    }

    return ret;
}

size_t _z_locator_metadata_strlen(const _z_str_intmap_t *s) {
    // @TODO: define protocol-level metadata
    return _z_str_intmap_strlen(s, 0, NULL);
}

void _z_locator_metadata_onto_str(char *dst, const _z_str_intmap_t *s) {
    // @TODO: define protocol-level metadata
    _z_str_intmap_onto_str(dst, s, 0, NULL);
}

_z_locator_result_t _z_locator_from_str(const char *str) {
    _z_locator_result_t ret;
    ret._tag = _Z_ERR_PARSE_STRING;
    _z_locator_init(&ret._value);

    // Parse protocol
    ret._value._protocol = _z_locator_protocol_from_str(str);
    if (ret._value._protocol != NULL) {
        // Parse address
        ret._value._address = _z_locator_address_from_str(str);
        if (ret._value._address != NULL) {
            // Parse metadata
            _z_str_intmap_result_t r_md = _z_locator_metadata_from_str(str);
            if (r_md._tag == _Z_RES_OK) {
                ret._tag = _Z_RES_OK;
                ret._value._metadata = r_md._value;
            }
        }
    }

    if (ret._tag < _Z_RES_OK) {
        _z_locator_clear(&ret._value);
    }

    return ret;
}

size_t _z_locator_strlen(const _z_locator_t *l) {
    size_t ret = 0;

    if (l != NULL) {
        // Calculate the string length to allocate
        ret += strlen(l->_protocol);  // Locator protocol
        ret += (size_t)1;             // Locator protocol separator
        ret += strlen(l->_address);   // Locator address

        // @TODO: define protocol-level metadata
        size_t md_len = _z_locator_metadata_strlen(&l->_metadata);
        if (md_len > (size_t)0) {
            ret += (size_t)1;  // Locator metadata separator
            ret += md_len;     // Locator medatada content
        }
    }
    return ret;
}

/**
 * Converts a :c:type:`_z_locator_t` into its string format.
 *
 * Parameters:
 *   dst: Pointer to the destination string. It MUST be already allocated with enough space to store the locator in
 * its string format. loc: :c:type:`_z_locator_t` to be converted into its string format.
 */
void __z_locator_onto_str(char *dst, const _z_locator_t *loc) {
    dst[0] = '\0';

    const char psep = LOCATOR_PROTOCOL_SEPARATOR;
    const char msep = LOCATOR_METADATA_SEPARATOR;

    (void)strncat(dst, loc->_protocol, strlen(loc->_protocol));  // Locator protocol
    (void)strncat(dst, &psep, 1);                                // Locator protocol separator
    (void)strncat(dst, loc->_address, strlen(loc->_address));    // Locator address

    // @TODO: define protocol-level metadata
    size_t md_len = _z_locator_metadata_strlen(&loc->_metadata);
    if (md_len > (size_t)0) {
        (void)strncat(dst, &msep, 1);  // Locator metadata separator
        _z_locator_metadata_onto_str(&dst[strlen(dst)], &loc->_metadata);
    }
}

/**
 * Converts a :c:type:`_z_locator_t` into its string format.
 *
 * Parameters:
 *   loc: :c:type:`_z_locator_t` to be converted into its string format.
 *
 * Returns:
 *   The pointer to the stringified :c:type:`_z_locator_t`.
 */
char *_z_locator_to_str(const _z_locator_t *l) {
    size_t loc_len = _z_locator_strlen(l) + (size_t)1;
    char *dst = (char *)z_malloc(loc_len);
    __z_locator_onto_str(dst, l);
    return dst;
}

/*------------------ Endpoint ------------------*/
void _z_endpoint_init(_z_endpoint_t *endpoint) {
    _z_locator_init(&endpoint->_locator);
    endpoint->_config = _z_str_intmap_make();
}

void _z_endpoint_clear(_z_endpoint_t *ep) {
    _z_locator_clear(&ep->_locator);
    _z_str_intmap_clear(&ep->_config);
}

void _z_endpoint_free(_z_endpoint_t **ep) {
    _z_endpoint_t *ptr = *ep;
    _z_locator_clear(&ptr->_locator);
    _z_str_intmap_clear(&ptr->_config);
    z_free(ptr);
    *ep = NULL;
}

_z_str_intmap_result_t _z_endpoint_config_from_str(const char *str, const char *proto) {
    _z_str_intmap_result_t res;
    res._tag = _Z_RES_OK;
    res._value = _z_str_intmap_make();

    char *p_start = strchr(str, ENDPOINT_CONFIG_SEPARATOR);
    if (p_start != NULL) {
        p_start++;

        // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
        if (_z_str_eq(proto, TCP_SCHEMA) == true) {
            res._tag = _Z_RES_OK;
            res = _z_tcp_config_from_str(p_start);
        } else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
            if (_z_str_eq(proto, UDP_SCHEMA) == true) {
            res._tag = _Z_RES_OK;
            res = _z_udp_config_from_str(p_start);
        } else
#endif
#if Z_LINK_BLUETOOTH == 1
            if (_z_str_eq(proto, BT_SCHEMA) == true) {
            res._tag = _Z_RES_OK;
            res = _z_bt_config_from_str(p_start);
        } else
#endif
#if Z_LINK_SERIAL == 1
            if (_z_str_eq(proto, SERIAL_SCHEMA) == true) {
            res._tag = _Z_RES_OK;
            res = _z_serial_config_from_str(p_start);
        } else
#endif
        {
            res._tag = _Z_ERR_UNKNOWN_SCHEMA;
        }
    } else {
        res._tag = _Z_ERR_PARSE_STRING;
    }

    return res;
}

size_t _z_endpoint_config_strlen(const _z_str_intmap_t *s, const char *proto) {
    size_t len = 0;

    // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA) == true) {
        len = _z_tcp_config_strlen(s);
    } else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
        if (_z_str_eq(proto, UDP_SCHEMA) == true) {
        len = _z_udp_config_strlen(s);
    } else
#endif
#if Z_LINK_BLUETOOTH == 1
        if (_z_str_eq(proto, BT_SCHEMA) == true) {
        len = _z_bt_config_strlen(s);
    } else
#endif
#if Z_LINK_SERIAL == 1
        if (_z_str_eq(proto, SERIAL_SCHEMA) == true) {
        len = _z_serial_config_strlen(s);
    } else
#endif
    {
        __asm__("nop");
    }

    return len;
}

char *_z_endpoint_config_to_str(const _z_str_intmap_t *s, const char *proto) {
    char *res = NULL;

    // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA) == true) {
        res = _z_tcp_config_to_str(s);
    } else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
        if (_z_str_eq(proto, UDP_SCHEMA) == true) {
        res = _z_udp_config_to_str(s);
    } else
#endif
#if Z_LINK_BLUETOOTH == 1
        if (_z_str_eq(proto, BT_SCHEMA) == true) {
        res = _z_bt_config_to_str(s);
    } else
#endif
#if Z_LINK_SERIAL == 1
        if (_z_str_eq(proto, SERIAL_SCHEMA) == true) {
        res = _z_serial_config_to_str(s);
    } else
#endif
    {
        __asm__("nop");
    }

    return res;
}

_z_endpoint_result_t _z_endpoint_from_str(const char *str) {
    _z_endpoint_result_t ret;
    ret._tag = _Z_ERR_PARSE_STRING;
    _z_endpoint_init(&ret._value);

    _z_locator_result_t r_loc = _z_locator_from_str(str);
    if (r_loc._tag == _Z_RES_OK) {
        ret._tag = _Z_RES_OK;
        ret._value._locator = r_loc._value;

        _z_str_intmap_result_t r_conf = _z_endpoint_config_from_str(str, ret._value._locator._protocol);
        if (r_conf._tag == _Z_RES_OK) {
            ret._value._config = r_conf._value;
        }  // Config is optional in locator, if its absense still means successful parsing
    }

    if (ret._tag < _Z_RES_OK) {
        _z_endpoint_clear(&ret._value);
    }

    return ret;
}

char *_z_endpoint_to_str(const _z_endpoint_t *endpoint) {
    char *ret = NULL;

    char *locator = _z_locator_to_str(&endpoint->_locator);
    if (locator != NULL) {
        size_t loc_len = 1;  // Start with space for the null-terminator
        loc_len += strlen(locator);

        char *config = _z_endpoint_config_to_str(&endpoint->_config, endpoint->_locator._protocol);
        if (config != NULL) {
            loc_len += (size_t)1;       // Config separator
            loc_len += strlen(config);  // Config content
        }

        ret = (char *)z_malloc(loc_len);
        ret[0] = '\0';

        (void)strncat(ret, locator, strlen(locator));
        if (config != NULL) {
            (void)strncat(ret, config, strlen(config));
        }
    }

    return ret;
}
