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

#include "zenoh-pico/utils/config.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/utils/pointers.h"

int _z_config_init(_z_config_t *ps) {
    _z_str_intmap_init(ps);
    return 0;
}

int8_t _zp_config_insert(_z_config_t *ps, unsigned int key, _z_string_t value) {
    char *res = _z_str_intmap_insert(ps, key, value.val);
    if (res != value.val) {
        return -1;
    }

    return 0;
}

char *_z_config_get(const _z_config_t *ps, unsigned int key) { return _z_str_intmap_get(ps, key); }

/*------------------ int-string map ------------------*/
_z_str_intmap_result_t _z_str_intmap_from_strn(const char *s, unsigned int argc, _z_str_intmapping_t argv[], size_t n) {
    _z_str_intmap_result_t res;

    res._tag = _Z_RES_OK;
    res._value = _z_str_intmap_make();

    // Check the string contains only the right
    const char *start = s;
    const char *end = &s[n];
    while (start < end) {
        const char *p_key_start = start;
        const char *p_key_end = strchr(p_key_start, INT_STR_MAP_KEYVALUE_SEPARATOR);

        if (p_key_end != NULL) {
            // Verify the key is valid based on the provided mapping
            size_t p_key_len = _z_ptr_char_diff(p_key_end, p_key_start);
            _Bool found = false;
            unsigned int key;
            for (unsigned int i = 0; i < argc; i++) {
                if (p_key_len != strlen(argv[i]._str)) {
                    continue;
                }
                if (strncmp(p_key_start, argv[i]._str, p_key_len) != 0) {
                    continue;
                }

                found = true;
                key = argv[i]._key;
                break;
            }

            if (found == true) {
                // Read and populate the value
                const char *p_value_start = _z_cptr_char_offset(p_key_end, 1);
                const char *p_value_end = strchr(p_key_end, INT_STR_MAP_LIST_SEPARATOR);
                if (p_value_end == NULL) {
                    p_value_end = end;
                }

                size_t p_value_len = _z_ptr_char_diff(p_value_end, p_value_start);
                char *p_value = (char *)z_malloc(p_value_len + (size_t)1);
                (void)strncpy(p_value, p_value_start, p_value_len);
                p_value[p_value_len] = '\0';

                _z_str_intmap_insert(&res._value, key, p_value);

                // Process next key value
                start = _z_cptr_char_offset(p_value_end, 1);
            }
        }
    }

    return res;
}

_z_str_intmap_result_t _z_str_intmap_from_str(const char *s, unsigned int argc, _z_str_intmapping_t argv[]) {
    return _z_str_intmap_from_strn(s, argc, argv, strlen(s));
}

size_t _z_str_intmap_strlen(const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]) {
    // Calculate the string length to allocate
    size_t len = 0;
    for (size_t i = 0; i < argc; i++) {
        char *v = _z_str_intmap_get(s, argv[i]._key);
        if (v != NULL) {
            if (len != (size_t)0) {
                len += (size_t)1;  // List separator
            }
            len += strlen(argv[i]._str);  // Key
            len += (size_t)1;             // KeyValue separator
            len += strlen(v);             // Value
        }
    }

    return len;
}

void _z_str_intmap_onto_str(char *dst, const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]) {
    // Build the string
    dst[0] = '\0';

    const char lsep = INT_STR_MAP_LIST_SEPARATOR;
    const char ksep = INT_STR_MAP_KEYVALUE_SEPARATOR;
    for (size_t i = 0; i < argc; i++) {
        char *v = _z_str_intmap_get(s, argv[i]._key);
        if (v != NULL) {
            if (strlen(dst) != (size_t)0) {
                (void)strncat(dst, &lsep, 1);  // List separator
            }
            (void)strncat(dst, argv[i]._str, strlen(argv[i]._str));  // Key
            (void)strncat(dst, &ksep, 1);                            // KeyValue separator
            (void)strncat(dst, v, strlen(v));                        // Value
        }
    }
}

char *_z_str_intmap_to_str(const _z_str_intmap_t *s, unsigned int argc, _z_str_intmapping_t argv[]) {
    // Calculate the string length to allocate
    size_t len = _z_str_intmap_strlen(s, argc, argv) + (size_t)1;
    // Build the string
    char *dst = (char *)z_malloc(len);
    _z_str_intmap_onto_str(dst, s, argc, argv);
    return dst;
}
