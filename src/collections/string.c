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

#include "zenoh-pico/collections/string.h"

#include <stddef.h>
#include <string.h>

/*-------- string --------*/
_z_string_t _z_string_null(void) {
    _z_string_t s = {.len = 0, .val = NULL};
    return s;
}

_Bool _z_string_check(const _z_string_t *value) { return value->val != NULL; }

_z_string_t _z_string_make(const char *value) {
    _z_string_t s;
    s.val = _z_str_clone(value);
    if (s.val == NULL) {
        s.len = 0;
    } else {
        s.len = strlen(value);
    }
    return s;
}

_z_string_t _z_string_n_make(const char *value, size_t len) {
    _z_string_t s;
    s.val = _z_str_n_clone(value, len);
    if (s.val == NULL) {
        s.len = 0;
    } else {
        s.len = len;
    }
    return s;
}

_z_string_t _z_string_wrap(char *value) {
    _z_string_t s;
    s.val = value;
    s.len = strlen(value);
    return s;
}

_z_string_t *_z_string_make_as_ptr(const char *value) {
    _z_string_t *s = (_z_string_t *)z_malloc(sizeof(_z_string_t));
    s->val = _z_str_clone(value);
    s->len = strlen(value);
    return s;
}

size_t _z_string_size(const _z_string_t *s) { return s->len; }

int8_t _z_string_copy(_z_string_t *dst, const _z_string_t *src) {
    if (src->val != NULL) {
        dst->val = _z_str_clone(src->val);
        if (dst->val == NULL) {
            dst->len = 0;
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
    } else {
        dst->val = NULL;
    }
    dst->len = src->len;
    return _Z_RES_OK;
}

void _z_string_move(_z_string_t *dst, _z_string_t *src) {
    dst->val = src->val;
    dst->len = src->len;

    src->val = NULL;
    src->len = 0;
}

_z_string_t _z_string_steal(_z_string_t *str) {
    _z_string_t ret = {
        .val = str->val,
        .len = str->len,
    };
    str->val = NULL;
    str->len = 0;
    return ret;
}

void _z_string_move_str(_z_string_t *dst, char *src) {
    dst->val = src;
    dst->len = strlen(src);
}

void _z_string_reset(_z_string_t *str) {
    str->val = NULL;
    str->len = 0;
}

void _z_string_clear(_z_string_t *str) {
    z_free(str->val);
    _z_string_reset(str);
}

void _z_string_free(_z_string_t **str) {
    _z_string_t *ptr = *str;
    if (ptr != NULL) {
        _z_string_clear(ptr);

        z_free(ptr);
        *str = NULL;
    }
}

_z_string_t _z_string_convert_bytes(const _z_slice_t *bs) {
    _z_string_t s = _z_string_null();
    size_t len = bs->len * (size_t)2;
    char *s_val = (char *)z_malloc((len + (size_t)1) * sizeof(char));

    if (s_val != NULL) {
        const char c[] = "0123456789ABCDEF";
        for (size_t i = 0; i < bs->len; i++) {
            s_val[i * (size_t)2] = c[(bs->start[i] & (uint8_t)0xF0) >> (uint8_t)4];
            s_val[(i * (size_t)2) + (size_t)1] = c[bs->start[i] & (uint8_t)0x0F];
        }
        s_val[len] = '\0';
    } else {
        len = 0;
    }

    s.val = s_val;
    s.len = len;

    return s;
}

_z_string_t _z_string_from_bytes(const _z_slice_t *bs) {
    _z_string_t s = _z_string_preallocate(bs->len);
    if (s.val == NULL) {
        return s;
    }
    // Recopy data
    memcpy(s.val, bs->start, bs->len);
    return s;
}

_z_string_t _z_string_preallocate(size_t len) {
    _z_string_t s = _z_string_null();
    // Allocate string
    s.len = len;
    char *str_val = (char *)z_malloc((s.len + (size_t)1) * sizeof(char));  // bytes data + null terminator
    if (str_val == NULL) {
        s.len = 0;
        return s;
    }
    s.val = str_val;
    s.val[len] = '\0';
    return s;
}

/*-------- str --------*/
size_t _z_str_size(const char *src) { return strlen(src) + (size_t)1; }

void _z_str_clear(char *src) { z_free(src); }

void _z_str_free(char **src) {
    char *ptr = *src;
    if (ptr != NULL) {
        _z_str_clear(ptr);

        *src = NULL;
    }
}

void _z_str_copy(char *dst, const char *src) {
    size_t size = _z_str_size(src);
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';  // No matter what, strings are always null-terminated upon copy
}

void _z_str_n_copy(char *dst, const char *src, size_t size) {
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';  // No matter what, strings are always null-terminated upon copy
}

char *_z_str_clone(const char *src) {
    size_t len = _z_str_size(src);
    char *dst = (char *)z_malloc(len);
    if (dst != NULL) {
        _z_str_n_copy(dst, src, len);
    }

    return dst;
}

char *_z_str_n_clone(const char *src, size_t len) {
    char *dst = (char *)z_malloc(len + 1);
    if (dst != NULL) {
        _z_str_n_copy(dst, src, len + 1);
    }

    return dst;
}

_Bool _z_str_eq(const char *left, const char *right) { return strcmp(left, right) == 0; }
