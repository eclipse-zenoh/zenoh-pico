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

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/config.h"
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

int _z_locator_eq(const _z_locator_t *left, const _z_locator_t *right) {
    int res = 0;

    res = _z_str_eq(left->_protocol, right->_protocol);
    if (!res) {
        return res;
    }

    res = _z_str_eq(left->_address, right->_address);
    if (!res) {
        return res;
    }

    // @TODO: implement eq for metadata

    return res;
}

char *_z_locator_protocol_from_str(const char *s) {
    const char *p_start = &s[0];
    if (p_start == NULL) {
        goto ERR;
    }

    const char *p_end = strchr(s, LOCATOR_PROTOCOL_SEPARATOR);
    if (p_end == NULL) {
        goto ERR;
    }

    if (p_start == p_end) {
        goto ERR;
    }

    size_t p_len = p_end - p_start;
    char *protocol = (char *)z_malloc((p_len + (size_t)1) * sizeof(char));
    (void)strncpy(protocol, p_start, p_len);
    protocol[p_len] = '\0';

    return protocol;

ERR:
    return NULL;
}

char *_z_locator_address_from_str(const char *s) {
    const char *p_start = strchr(s, LOCATOR_PROTOCOL_SEPARATOR);
    if (p_start == NULL) {
        goto ERR;
    }
    p_start++;

    const char *p_end = strchr(s, LOCATOR_METADATA_SEPARATOR);
    if (p_end == NULL) {
        p_end = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    }
    if (p_end == NULL) {
        p_end = &s[strlen(s)];
    }

    if (p_start == p_end) {
        goto ERR;
    }

    size_t p_len = p_end - p_start;
    char *address = (char *)z_malloc((p_len + (size_t)1) * sizeof(char));
    (void)strncpy(address, p_start, p_len);
    address[p_len] = '\0';

    return address;

ERR:
    return NULL;
}

_z_str_intmap_result_t _z_locator_metadata_from_str(const char *s) {
    _z_str_intmap_result_t res;

    res._tag = _Z_RES_OK;
    res._value._str_intmap = _z_str_intmap_make();

    const char *p_start = strchr(s, LOCATOR_METADATA_SEPARATOR);
    if (p_start == NULL) {
        return res;
    }
    p_start++;

    const char *p_end = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_end == NULL) {
        p_end = &s[strlen(s)];
    }

    if (p_start == p_end) {
        goto ERR;
    }

    size_t p_len = p_end - p_start;

    // @TODO: define protocol-level metadata
    return _z_str_intmap_from_strn(p_start, 0, NULL, p_len);

ERR:
    res._tag = _Z_RES_ERR;
    res._value._error = _Z_ERR_PARSE_STRING;
    return res;
}

size_t _z_locator_metadata_strlen(const _z_str_intmap_t *s) {
    // @TODO: define protocol-level metadata
    return _z_str_intmap_strlen(s, 0, NULL);
}

void _z_locator_metadata_onto_str(char *dst, const _z_str_intmap_t *s) {
    // @TODO: define protocol-level metadata
    _z_str_intmap_onto_str(dst, s, 0, NULL);
}

_z_locator_result_t _z_locator_from_str(const char *s) {
    _z_locator_result_t res;

    if (s == NULL) {
        goto ERR;
    }

    res._tag = _Z_RES_OK;
    _z_locator_init(&res._value._locator);

    // Parse protocol
    res._value._locator._protocol = _z_locator_protocol_from_str(s);
    if (res._value._locator._protocol == NULL) {
        goto ERR;
    }

    // Parse address
    res._value._locator._address = _z_locator_address_from_str(s);
    if (res._value._locator._address == NULL) {
        goto ERR;
    }

    // Parse metadata
    _z_str_intmap_result_t tmp_res;
    tmp_res = _z_locator_metadata_from_str(s);
    if (tmp_res._tag == _Z_RES_ERR) {
        goto ERR;
    }
    res._value._locator._metadata = tmp_res._value._str_intmap;

    return res;

ERR:
    _z_locator_clear(&res._value._locator);
    res._tag = _Z_RES_ERR;
    res._value._error = _Z_ERR_PARSE_STRING;
    return res;
}

size_t _z_locator_strlen(const _z_locator_t *l) {
    if (l == NULL) {
        goto ERR;
    }

    // Calculate the string length to allocate
    size_t len = 0;

    len += strlen(l->_protocol);  // Locator protocol
    len += (size_t)1;             // Locator protocol separator
    len += strlen(l->_address);   // Locator address

    // @TODO: define protocol-level metadata
    size_t md_len = _z_locator_metadata_strlen(&l->_metadata);
    if (md_len > 0) {
        len += (size_t)1;  // Locator metadata separator
        len += md_len;     // Locator medatada content
    }

    return len;

ERR:
    return 0;
}

/**
 * Converts a :c:type:`_z_locator_t` into its string format.
 *
 * Parameters:
 *   dst: Pointer to the destination string. It MUST be already allocated with enough space to store the locator in its
 * string format.
 *   loc: :c:type:`_z_locator_t` to be converted into its string format.
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
    if (md_len > 0) {
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

_z_str_intmap_result_t _z_endpoint_config_from_str(const char *s, const char *proto) {
    _z_str_intmap_result_t res;

    res._tag = _Z_RES_OK;
    res._value._str_intmap = _z_str_intmap_make();

    char *p_start = strchr(s, ENDPOINT_CONFIG_SEPARATOR);
    if (p_start == NULL) {
        return res;
    }
    p_start++;

    // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA)) {
        res = _z_tcp_config_from_str(p_start);
    } else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
        if (_z_str_eq(proto, UDP_SCHEMA)) {
        res = _z_udp_config_from_str(p_start);
    } else
#endif
#if Z_LINK_BLUETOOTH == 1
        if (_z_str_eq(proto, BT_SCHEMA)) {
        res = _z_bt_config_from_str(p_start);
    } else
#endif
#if Z_LINK_SERIAL == 1
        if (_z_str_eq(proto, SERIAL_SCHEMA)) {
        res = _z_serial_config_from_str(p_start);
    } else
#endif
    {
        goto ERR;
    }

    return res;

ERR:
    res._tag = _Z_RES_ERR;
    res._value._error = _Z_ERR_PARSE_STRING;
    return res;
}

size_t _z_endpoint_config_strlen(const _z_str_intmap_t *s, const char *proto) {
    size_t len;

    // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA)) {
        len = _z_tcp_config_strlen(s);
    } else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
        if (_z_str_eq(proto, UDP_SCHEMA)) {
        len = _z_udp_config_strlen(s);
    } else
#endif
#if Z_LINK_BLUETOOTH == 1
        if (_z_str_eq(proto, BT_SCHEMA)) {
        len = _z_bt_config_strlen(s);
    } else
#endif
#if Z_LINK_SERIAL == 1
        if (_z_str_eq(proto, SERIAL_SCHEMA)) {
        len = _z_serial_config_strlen(s);
    } else
#endif
    {
        goto ERR;
    }

    return len;

ERR:
    len = 0;
    return len;
}

char *_z_endpoint_config_to_str(const _z_str_intmap_t *s, const char *proto) {
    char *res;

    // Call the right configuration parser depending on the protocol
#if Z_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA)) {
        res = _z_tcp_config_to_str(s);
    } else
#endif
#if Z_LINK_UDP_UNICAST == 1 || Z_LINK_UDP_MULTICAST == 1
        if (_z_str_eq(proto, UDP_SCHEMA)) {
        res = _z_udp_config_to_str(s);
    } else
#endif
#if Z_LINK_BLUETOOTH == 1
        if (_z_str_eq(proto, BT_SCHEMA)) {
        res = _z_bt_config_to_str(s);
    } else
#endif
#if Z_LINK_SERIAL == 1
        if (_z_str_eq(proto, SERIAL_SCHEMA)) {
        res = _z_serial_config_to_str(s);
    } else
#endif
    {
        goto ERR;
    }

    return res;

ERR:
    res = NULL;
    return res;
}

_z_endpoint_result_t _z_endpoint_from_str(const char *s) {
    _z_endpoint_result_t res;

    res._tag = _Z_RES_OK;
    _z_endpoint_init(&res._value._endpoint);

    _z_locator_result_t loc_res = _z_locator_from_str(s);
    if (loc_res._tag == _Z_RES_ERR) {
        goto ERR;
    }
    res._value._endpoint._locator = loc_res._value._locator;

    _z_str_intmap_result_t conf_res = _z_endpoint_config_from_str(s, res._value._endpoint._locator._protocol);
    if (conf_res._tag == _Z_RES_ERR) {
        goto ERR;
    }
    res._value._endpoint._config = conf_res._value._str_intmap;

    return res;

ERR:
    _z_endpoint_clear(&res._value._endpoint);
    res._tag = _Z_RES_ERR;
    res._value._error = _Z_ERR_PARSE_STRING;
    return res;
}

char *_z_endpoint_to_str(const _z_endpoint_t *endpoint) {
    char *locator = _z_locator_to_str(&endpoint->_locator);
    if (locator == NULL) {
        goto ERR;
    }

    size_t loc_len = 1;  // Start with space for the null-terminator
    loc_len += strlen(locator);

    char *config = _z_endpoint_config_to_str(&endpoint->_config, endpoint->_locator._protocol);
    if (config != NULL) {
        loc_len += (size_t)1;       // Config separator
        loc_len += strlen(config);  // Config content
    }

    char *s = (char *)z_malloc(loc_len);
    s[0] = '\0';

    (void)strncat(s, locator, strlen(locator));
    if (config != NULL) {
        (void)strncat(s, config, strlen(config));
    }

    return s;

ERR:
    return NULL;
}
