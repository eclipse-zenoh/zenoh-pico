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
    _z_string_t s = {._slice = _z_slice_empty()};
    return s;
}

_Bool _z_string_check(const _z_string_t *value) { return !_z_slice_is_empty(&value->_slice); }

_z_string_t _z_string_make(const char *value) {
    _z_string_t s;
    s._slice = _z_slice_wrap_copy((uint8_t *)value, strlen(value) + 1);
    return s;
}

_z_string_t _z_string_n_make(const char *value, size_t len) {
    _z_string_t s;
    char *c = _z_str_n_clone(value, len);

    if (c == NULL) {
        return _z_string_null();
    } else {
        s._slice = _z_slice_wrap_custom_deleter((const uint8_t *)c, len + 1, _z_delete_context_default());
        return s;
    }
}

_z_string_t _z_string_wrap(const char *value) {
    _z_string_t s;
    s._slice = _z_slice_wrap((const uint8_t *)(value), strlen(value) + 1);
    return s;
}

_z_string_t _z_string_wrap_custom_deleter(char *value, _z_delete_context_t c) {
    _z_string_t s;
    s._slice = _z_slice_wrap_custom_deleter((const uint8_t *)(value), strlen(value) + 1, c);
    return s;
}

_z_string_t *_z_string_make_as_ptr(const char *value) {
    _z_string_t *s = (_z_string_t *)z_malloc(sizeof(_z_string_t));
    *s = _z_string_make(value);
    if (_z_slice_is_empty(&s->_slice) && value != NULL) {
        z_free(s);
        return NULL;
    }
    return s;
}

size_t _z_string_len(const _z_string_t *s) { return s->_slice.len == 0 ? 0 : s->_slice.len - 1; }

int8_t _z_string_copy(_z_string_t *dst, const _z_string_t *src) { return _z_slice_copy(&dst->_slice, &src->_slice); }

void _z_string_move(_z_string_t *dst, _z_string_t *src) { *dst = _z_string_steal(src); }

_z_string_t _z_string_steal(_z_string_t *str) {
    _z_string_t ret;
    ret._slice = _z_slice_steal(&str->_slice);
    return ret;
}

void _z_string_move_str(_z_string_t *dst, char *src) { *dst = _z_string_wrap(src); }

void _z_string_reset(_z_string_t *str) { _z_slice_reset(&str->_slice); }

void _z_string_clear(_z_string_t *str) { _z_slice_clear(&str->_slice); }

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
    if (s_val == NULL) {
        return s;
    }

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
    s._slice = _z_slice_wrap_custom_deleter((const uint8_t *)s_val, len + 1, _z_delete_context_default());

    return s;
}

_z_string_t _z_string_preallocate(size_t len) {
    _z_string_t s = _z_string_null();
    _z_slice_init(&s._slice, len + 1);
    if (_z_slice_is_empty(&s._slice)) {
        return _z_string_null();
    }
    char *ss = (char *)s._slice.start;
    ss[len] = '\0';
    return s;
}
const char *_z_string_data(const _z_string_t *s) { return (const char *)s->_slice.start; }

_Bool _z_string_is_empty(const _z_string_t *s) { return s->_slice.len <= 1; }
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
