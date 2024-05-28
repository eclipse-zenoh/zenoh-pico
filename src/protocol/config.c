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

int8_t _z_config_init(_z_config_t *ps) {
    _z_str_intmap_init(ps);
    return 0;
}

int8_t _zp_config_insert(_z_config_t *ps, uint8_t key, const char *value) {
    int8_t ret = _Z_RES_OK;

    char *res = _z_str_intmap_insert(ps, key, _z_str_clone(value));
    if (strcmp(res, value) != 0) {
        ret = _Z_ERR_CONFIG_FAILED_INSERT;
    }

    return ret;
}

char *_z_config_get(const _z_config_t *ps, uint8_t key) { return _z_str_intmap_get(ps, key); }

/*------------------ int-string map ------------------*/
int8_t _z_str_intmap_from_strn(_z_str_intmap_t *strint, const char *s, uint8_t argc, _z_str_intmapping_t argv[],
                               size_t n) {
    int8_t ret = _Z_RES_OK;
    *strint = _z_str_intmap_make();

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
            uint8_t key = 0;
            for (uint8_t i = 0; i < argc; i++) {
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

            if (found == false) {
                break;
            }

            // Read and populate the value
            const char *p_value_start = _z_cptr_char_offset(p_key_end, 1);
            const char *p_value_end = strchr(p_key_end, INT_STR_MAP_LIST_SEPARATOR);
            if (p_value_end == NULL) {
                p_value_end = end;
            }

            size_t p_value_len = _z_ptr_char_diff(p_value_end, p_value_start) + (size_t)1;
            char *p_value = (char *)z_malloc(p_value_len);
            if (p_value != NULL) {
                _z_str_n_copy(p_value, p_value_start, p_value_len);

                _z_str_intmap_insert(strint, key, p_value);

                // Process next key value
                start = _z_cptr_char_offset(p_value_end, 1);
            } else {
                ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }
        }
    }

    return ret;
}

int8_t _z_str_intmap_from_str(_z_str_intmap_t *strint, const char *s, uint8_t argc, _z_str_intmapping_t argv[]) {
    return _z_str_intmap_from_strn(strint, s, argc, argv, strlen(s));
}

size_t _z_str_intmap_strlen(const _z_str_intmap_t *s, uint8_t argc, _z_str_intmapping_t argv[]) {
    // Calculate the string length to allocate
    size_t len = 0;
    for (size_t i = 0; i < argc; i++) {
        char *v = _z_str_intmap_get(s, argv[i]._key);
        if (v != NULL) {
            if (len != (size_t)0) {
                len = len + (size_t)1;  // List separator
            }
            len = len + strlen(argv[i]._str);  // Key
            len = len + (size_t)1;             // KeyValue separator
            len = len + strlen(v);             // Value
        }
    }

    return len;
}

void _z_str_intmap_onto_str(char *dst, size_t dst_len, const _z_str_intmap_t *s, uint8_t argc,
                            _z_str_intmapping_t argv[]) {
    size_t len = dst_len;
    const char lsep = INT_STR_MAP_LIST_SEPARATOR;
    const char ksep = INT_STR_MAP_KEYVALUE_SEPARATOR;

    dst[0] = '\0';
    for (size_t i = 0; i < argc; i++) {
        char *v = _z_str_intmap_get(s, argv[i]._key);
        if (v != NULL) {
            if (len > (size_t)0) {
                (void)strncat(dst, &lsep, 1);  // List separator
                len = len - (size_t)1;
            }

            if (len > (size_t)0) {
                (void)strncat(dst, argv[i]._str, len);  // Key
                len = len - strlen(argv[i]._str);
            }
            if (len > (size_t)0) {
                (void)strncat(dst, &ksep, 1);  // KeyValue separator
                len = len - (size_t)1;
            }
            if (len > (size_t)0) {
                (void)strncat(dst, v, len);  // Value
                len = len - strlen(v);
            }
        }
    }
}

char *_z_str_intmap_to_str(const _z_str_intmap_t *s, uint8_t argc, _z_str_intmapping_t argv[]) {
    // Calculate the string length to allocate
    size_t len = _z_str_intmap_strlen(s, argc, argv) + (size_t)1;
    // Build the string
    char *dst = (char *)z_malloc(len);
    if (dst != NULL) {
        _z_str_intmap_onto_str(dst, len, s, argc, argv);
    }
    return dst;
}
