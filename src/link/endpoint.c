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
#if Z_FEATURE_LINK_TCP == 1
#include "zenoh-pico/link/config/tcp.h"
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
#include "zenoh-pico/link/config/udp.h"
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
#include "zenoh-pico/link/config/bt.h"
#endif
#if Z_FEATURE_LINK_SERIAL == 1
#include "zenoh-pico/link/config/serial.h"
#endif
#if Z_FEATURE_LINK_WS == 1
#include "zenoh-pico/link/config/ws.h"
#endif
#include "zenoh-pico/link/config/raweth.h"

/*------------------ Locator ------------------*/
void _z_locator_init(_z_locator_t *locator) {
    locator->_protocol = NULL;
    locator->_address = NULL;
    locator->_metadata = _z_str_intmap_make();
}

void _z_locator_clear(_z_locator_t *lc) {
    _z_str_free(&lc->_protocol);
    _z_str_free(&lc->_address);
    _z_str_intmap_clear(&lc->_metadata);
}

void _z_locator_free(_z_locator_t **lc) {
    _z_locator_t *ptr = *lc;

    if (ptr != NULL) {
        _z_locator_clear(ptr);

        z_free(ptr);
        *lc = NULL;
    }
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

    if (str != NULL) {
        const char *p_start = &str[0];
        const char *p_end = strchr(p_start, LOCATOR_PROTOCOL_SEPARATOR);
        if ((p_end != NULL) && (p_start != p_end)) {
            size_t p_len = _z_ptr_char_diff(p_end, p_start) + (size_t)1;
            ret = (char *)z_malloc(p_len);
            if (ret != NULL) {
                _z_str_n_copy(ret, p_start, p_len);
            }
        }
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
            size_t a_len = _z_ptr_char_diff(p_end, p_start) + (size_t)1;
            ret = (char *)z_malloc(a_len);
            if (ret != NULL) {
                _z_str_n_copy(ret, p_start, a_len);
            }
        }
    }

    return ret;
}

int8_t _z_locator_metadata_from_str(_z_str_intmap_t *strint, const char *str) {
    int8_t ret = _Z_RES_OK;
    *strint = _z_str_intmap_make();

    const char *p_start = strchr(str, LOCATOR_METADATA_SEPARATOR);
    if (p_start != NULL) {
        p_start = _z_cptr_char_offset(p_start, 1);

        const char *p_end = strchr(str, ENDPOINT_CONFIG_SEPARATOR);
        if (p_end == NULL) {
            p_end = &str[strlen(str)];
        }

        if (p_start != p_end) {
            size_t p_len = _z_ptr_char_diff(p_end, p_start);
            ret = _z_str_intmap_from_strn(strint, p_start, 0, NULL, p_len);
        }
    }

    return ret;
}

size_t _z_locator_metadata_strlen(const _z_str_intmap_t *s) {
    // @TODO: define protocol-level metadata
    return _z_str_intmap_strlen(s, 0, NULL);
}

void _z_locator_metadata_onto_str(char *dst, size_t dst_len, const _z_str_intmap_t *s) {
    // @TODO: define protocol-level metadata
    _z_str_intmap_onto_str(dst, dst_len, s, 0, NULL);
}

int8_t _z_locator_from_str(_z_locator_t *lc, const char *str) {
    int8_t ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;

    // Parse protocol
    lc->_protocol = _z_locator_protocol_from_str(str);
    if (lc->_protocol != NULL) {
        // Parse address
        lc->_address = _z_locator_address_from_str(str);
        if (lc->_address != NULL) {
            // Parse metadata
            ret = _z_locator_metadata_from_str(&lc->_metadata, str);
        }
    }

    if (ret != _Z_RES_OK) {
        _z_locator_clear(lc);
    }

    return ret;
}

size_t _z_locator_strlen(const _z_locator_t *l) {
    size_t ret = 0;

    if (l != NULL) {
        // Calculate the string length to allocate
        ret = ret + strlen(l->_protocol);  // Locator protocol
        ret = ret + (size_t)1;             // Locator protocol separator
        ret = ret + strlen(l->_address);   // Locator address

        // @TODO: define protocol-level metadata
        size_t md_len = _z_locator_metadata_strlen(&l->_metadata);
        if (md_len > (size_t)0) {
            ret = ret + (size_t)1;  // Locator metadata separator
            ret = ret + md_len;     // Locator metadata content
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
void __z_locator_onto_str(char *dst, size_t dst_len, const _z_locator_t *loc) {
    size_t len = dst_len;
    const char psep = LOCATOR_PROTOCOL_SEPARATOR;
    const char msep = LOCATOR_METADATA_SEPARATOR;

    dst[0] = '\0';
    len = len - (size_t)1;

    if (len > (size_t)0) {
        (void)strncat(dst, loc->_protocol, dst_len);  // Locator protocol
        len = len - strlen(loc->_protocol);
    }

    if (len > (size_t)0) {
        (void)strncat(dst, &psep, 1);  // Locator protocol separator
        len = len - (size_t)1;
    }

    if (len > (size_t)0) {
        (void)strncat(dst, loc->_address, len);  // Locator address
        len = len - strlen(loc->_address);
    }

    // @TODO: define protocol-level metadata
    size_t md_len = _z_locator_metadata_strlen(&loc->_metadata);
    if (md_len > (size_t)0) {
        if (len > (size_t)0) {
            (void)strncat(dst, &msep, 1);  // Locator metadata separator
            len = len - (size_t)1;
        }

        if (len > (size_t)0) {
            _z_locator_metadata_onto_str(&dst[strlen(dst)], len, &loc->_metadata);
            len = len - md_len;
        }
    }
}

/**
 * Converts a :c:type:`_z_locator_t` into its _z_string format.
 *
 * Parameters:
 *   loc: :c:type:`_z_locator_t` to be converted into its _z_string format.
 *
 * Returns:
 *   The z_stringified :c:type:`_z_locator_t`.
 */
_z_string_t _z_locator_to_string(const _z_locator_t *loc) {
    _z_string_t s;
    s.len = _z_locator_strlen(loc);
    s.val = (char *)z_malloc(s.len + 1);
    if (s.val == NULL) {
        s.len = 0;
        return s;
    }
    __z_locator_onto_str(s.val, s.len + 1, loc);
    return s;
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

    if (ptr != NULL) {
        _z_locator_clear(&ptr->_locator);
        _z_str_intmap_clear(&ptr->_config);

        z_free(ptr);
        *ep = NULL;
    }
}

int8_t _z_endpoint_config_from_str(_z_str_intmap_t *strint, const char *str, const char *proto) {
    int8_t ret = _Z_RES_OK;

    char *p_start = strchr(str, ENDPOINT_CONFIG_SEPARATOR);
    if (p_start != NULL) {
        p_start = _z_ptr_char_offset(p_start, 1);

        // Call the right configuration parser depending on the protocol
#if Z_FEATURE_LINK_TCP == 1
        if (_z_str_eq(proto, TCP_SCHEMA) == true) {
            ret = _z_tcp_config_from_str(strint, p_start);
        } else
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
            if (_z_str_eq(proto, UDP_SCHEMA) == true) {
            ret = _z_udp_config_from_str(strint, p_start);
        } else
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
            if (_z_str_eq(proto, BT_SCHEMA) == true) {
            ret = _z_bt_config_from_str(strint, p_start);
        } else
#endif
#if Z_FEATURE_LINK_SERIAL == 1
            if (_z_str_eq(proto, SERIAL_SCHEMA) == true) {
            ret = _z_serial_config_from_str(strint, p_start);
        } else
#endif
#if Z_FEATURE_LINK_WS == 1
            if (_z_str_eq(proto, WS_SCHEMA) == true) {
            ret = _z_ws_config_from_str(strint, p_start);
        } else
#endif
            if (_z_str_eq(proto, RAWETH_SCHEMA) == true) {
            _z_raweth_config_from_str(strint, p_start);
        } else {
            ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
        }
    }

    return ret;
}

size_t _z_endpoint_config_strlen(const _z_str_intmap_t *s, const char *proto) {
    size_t len = 0;

    // Call the right configuration parser depending on the protocol
#if Z_FEATURE_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA) == true) {
        len = _z_tcp_config_strlen(s);
    } else
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
        if (_z_str_eq(proto, UDP_SCHEMA) == true) {
        len = _z_udp_config_strlen(s);
    } else
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
        if (_z_str_eq(proto, BT_SCHEMA) == true) {
        len = _z_bt_config_strlen(s);
    } else
#endif
#if Z_FEATURE_LINK_SERIAL == 1
        if (_z_str_eq(proto, SERIAL_SCHEMA) == true) {
        len = _z_serial_config_strlen(s);
    } else
#endif
#if Z_FEATURE_LINK_WS == 1
        if (_z_str_eq(proto, WS_SCHEMA) == true) {
        len = _z_ws_config_strlen(s);
    } else
#endif
        if (_z_str_eq(proto, RAWETH_SCHEMA) == true) {
        len = _z_raweth_config_strlen(s);
    }
    return len;
}

char *_z_endpoint_config_to_str(const _z_str_intmap_t *s, const char *proto) {
    char *res = NULL;

    // Call the right configuration parser depending on the protocol
#if Z_FEATURE_LINK_TCP == 1
    if (_z_str_eq(proto, TCP_SCHEMA) == true) {
        res = _z_tcp_config_to_str(s);
    } else
#endif
#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
        if (_z_str_eq(proto, UDP_SCHEMA) == true) {
        res = _z_udp_config_to_str(s);
    } else
#endif
#if Z_FEATURE_LINK_BLUETOOTH == 1
        if (_z_str_eq(proto, BT_SCHEMA) == true) {
        res = _z_bt_config_to_str(s);
    } else
#endif
#if Z_FEATURE_LINK_SERIAL == 1
        if (_z_str_eq(proto, SERIAL_SCHEMA) == true) {
        res = _z_serial_config_to_str(s);
    } else
#endif
#if Z_FEATURE_LINK_WS == 1
        if (_z_str_eq(proto, WS_SCHEMA) == true) {
        res = _z_ws_config_to_str(s);
    } else
#endif
        if (_z_str_eq(proto, RAWETH_SCHEMA) == true) {
        _z_raweth_config_to_str(s);
    }
    return res;
}

int8_t _z_endpoint_from_str(_z_endpoint_t *ep, const char *str) {
    int8_t ret = _Z_RES_OK;
    _z_endpoint_init(ep);

    ret = _z_locator_from_str(&ep->_locator, str);
    if (ret == _Z_RES_OK) {
        ret = _z_endpoint_config_from_str(&ep->_config, str, ep->_locator._protocol);
    }

    if (ret != _Z_RES_OK) {
        _z_endpoint_clear(ep);
    }

    return ret;
}

char *_z_endpoint_to_str(const _z_endpoint_t *endpoint) {
    char *ret = NULL;
    // Retrieve locator
    _z_string_t locator = _z_locator_to_string(&endpoint->_locator);
    if (locator.val == NULL) {
        return NULL;
    }
    size_t curr_len = locator.len;
    // Retrieve config
    char *config = _z_endpoint_config_to_str(&endpoint->_config, endpoint->_locator._protocol);
    if (config != NULL) {
        curr_len += strlen(config) + (size_t)1;  // Content + separator;
    }
    // Reconstruct the endpoint as a string
    ret = (char *)z_malloc(curr_len);
    if (ret == NULL) {
        return NULL;
    }
    ret[0] = '\0';
    curr_len -= (size_t)1;
    // Copy locator
    if (curr_len > (size_t)0) {
        (void)strncat(ret, locator.val, curr_len);
        curr_len -= locator.len;
    }
    // Copy config
    if (config != NULL) {
        if (curr_len > (size_t)0) {
            (void)strncat(ret, config, curr_len);
            curr_len -= strlen(config);
        }
    }
    // Clean up
    _z_string_clear(&locator);
    return ret;
}
