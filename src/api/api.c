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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/olv_macros.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/config.h"
#include "zenoh-pico/net/filtering.h"
#include "zenoh-pico/net/logger.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/system/platform-common.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/transport/unicast.h"
#include "zenoh-pico/utils/endianness.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/result.h"
#include "zenoh-pico/utils/uuid.h"

/********* Data Types Handlers *********/

int8_t z_view_string_from_str(z_view_string_t *str, const char *value) {
    str->_val = _z_string_alias_str((char *)value);
    return _Z_RES_OK;
}

const z_loaned_string_t *z_string_array_get(const z_loaned_string_array_t *a, size_t k) {
    return _z_string_svec_get(a, k);
}

size_t z_string_array_len(const z_loaned_string_array_t *a) { return _z_string_svec_len(a); }

_Bool z_string_array_is_empty(const z_loaned_string_array_t *a) { return _z_string_svec_is_empty(a); }

int8_t z_keyexpr_is_canon(const char *start, size_t len) { return _z_keyexpr_is_canon(start, len); }

int8_t z_keyexpr_canonize(char *start, size_t *len) { return _z_keyexpr_canonize(start, len); }

void z_view_keyexpr_from_str_unchecked(z_view_keyexpr_t *keyexpr, const char *name) { keyexpr->_val = _z_rname(name); }

z_result_t z_view_keyexpr_from_substr(z_view_keyexpr_t *keyexpr, const char *name, size_t len) {
    if (_z_keyexpr_is_canon(name, len) != Z_KEYEXPR_CANON_SUCCESS) {
        return Z_EINVAL;
    }
    keyexpr->_val = _z_keyexpr_from_substr(0, name, len);
    return _Z_RES_OK;
}

int8_t z_view_keyexpr_from_str(z_view_keyexpr_t *keyexpr, const char *name) {
    size_t name_len = strlen(name);
    return z_view_keyexpr_from_substr(keyexpr, name, name_len);
}

z_result_t z_view_keyexpr_from_substr_autocanonize(z_view_keyexpr_t *keyexpr, char *name, size_t *len) {
    _Z_RETURN_IF_ERR(z_keyexpr_canonize(name, len));
    keyexpr->_val = _z_keyexpr_from_substr(0, name, *len);
    return _Z_RES_OK;
}

int8_t z_view_keyexpr_from_str_autocanonize(z_view_keyexpr_t *keyexpr, char *name) {
    size_t name_len = strlen(name);
    return z_view_keyexpr_from_substr_autocanonize(keyexpr, name, &name_len);
}

void z_view_keyexpr_from_substr_unchecked(z_view_keyexpr_t *keyexpr, const char *name, size_t len) {
    keyexpr->_val = _z_keyexpr_from_substr(0, name, len);
}

int8_t z_keyexpr_as_view_string(const z_loaned_keyexpr_t *keyexpr, z_view_string_t *s) {
    s->_val = _z_string_alias(&keyexpr->_suffix);
    return _Z_RES_OK;
}

int8_t z_keyexpr_concat(z_owned_keyexpr_t *key, const z_loaned_keyexpr_t *left, const char *right, size_t len) {
    z_internal_keyexpr_null(key);
    if (len == 0) {
        return z_keyexpr_clone(key, left);
    } else if (right == NULL) {
        return _Z_ERR_INVALID;
    }
    size_t left_len = _z_string_len(&left->_suffix);
    if (left_len == 0) {
        return _Z_ERR_INVALID;
    }
    const char *left_data = _z_string_data(&left->_suffix);

    if (left_data[left_len - 1] == '*' && right[0] == '*') {
        return _Z_ERR_INVALID;
    }

    key->_val._suffix = _z_string_preallocate(left_len + len);
    if (!_z_keyexpr_has_suffix(&key->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Copy data
    uint8_t *curr_ptr = (uint8_t *)_z_string_data(&key->_val._suffix);
    memcpy(curr_ptr, _z_string_data(&left->_suffix), left_len);
    memcpy(curr_ptr + left_len, right, len);
    return _Z_RES_OK;
}

int8_t z_keyexpr_join(z_owned_keyexpr_t *key, const z_loaned_keyexpr_t *left, const z_loaned_keyexpr_t *right) {
    z_internal_keyexpr_null(key);

    size_t left_len = _z_string_len(&left->_suffix);
    size_t right_len = _z_string_len(&right->_suffix);

    key->_val._suffix = _z_string_preallocate(left_len + right_len + 1);
    if (!_z_keyexpr_has_suffix(&key->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Copy data
    uint8_t *curr_ptr = (uint8_t *)_z_string_data(&key->_val._suffix);
    memcpy(curr_ptr, _z_string_data(&left->_suffix), left_len);
    curr_ptr[left_len] = '/';
    memcpy(curr_ptr + left_len + 1, _z_string_data(&right->_suffix), right_len);
    _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_canonize((char *)curr_ptr, &key->_val._suffix._slice.len), z_free(curr_ptr));
    return _Z_RES_OK;
}

z_keyexpr_intersection_level_t z_keyexpr_relation_to(const z_loaned_keyexpr_t *left, const z_loaned_keyexpr_t *right) {
    if (z_keyexpr_equals(left, right)) {
        return Z_KEYEXPR_INTERSECTION_LEVEL_EQUALS;
    } else if (z_keyexpr_includes(left, right)) {
        return Z_KEYEXPR_INTERSECTION_LEVEL_INCLUDES;
    } else if (z_keyexpr_intersects(left, right)) {
        return Z_KEYEXPR_INTERSECTION_LEVEL_INTERSECTS;
    }
    return Z_KEYEXPR_INTERSECTION_LEVEL_DISJOINT;
}

_Bool z_keyexpr_includes(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    return _z_keyexpr_suffix_includes(l, r);
}

_Bool z_keyexpr_intersects(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    return _z_keyexpr_suffix_intersects(l, r);
}

_Bool z_keyexpr_equals(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    return _z_keyexpr_suffix_equals(l, r);
}

void z_config_new(z_owned_config_t *config) { config->_val = _z_config_empty(); }

int8_t z_config_default(z_owned_config_t *config) { return _z_config_default(&config->_val); }

void z_config_clone(z_owned_config_t *config, const z_loaned_config_t *src) { config->_val = _z_config_clone(src); }

int8_t z_config_client(z_owned_config_t *config, const char *locator) {
    return _z_config_client(&config->_val, locator);
}

int8_t z_config_peer(z_owned_config_t *config, const char *locator) {
    if (locator == NULL) {
        return _Z_ERR_INVALID;
    }
    _Z_RETURN_IF_ERR(z_config_default(config));
    _Z_CLEAN_RETURN_IF_ERR(zp_config_insert(&config->_val, Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER),
                           z_config_drop(z_config_move(config)));
    _Z_CLEAN_RETURN_IF_ERR(zp_config_insert(&config->_val, Z_CONFIG_CONNECT_KEY, locator),
                           z_config_drop(z_config_move(config)));
    return _Z_RES_OK;
}

const char *zp_config_get(const z_loaned_config_t *config, uint8_t key) { return _z_config_get(config, key); }

int8_t zp_config_insert(z_loaned_config_t *config, uint8_t key, const char *value) {
    return _zp_config_insert(config, key, value);
}

#if Z_FEATURE_ENCODING_VALUES == 1
#define ENCODING_SCHEMA_SEPARATOR ';'

const char *ENCODING_VALUES_ID_TO_STR[] = {
    "zenoh/bytes",
    "zenoh/int8",
    "zenoh/int16",
    "zenoh/int32",
    "zenoh/int64",
    "zenoh/int128",
    "zenoh/uint8",
    "zenoh/uint16",
    "zenoh/uint32",
    "zenoh/uint64",
    "zenoh/uint128",
    "zenoh/float32",
    "zenoh/float64",
    "zenoh/bool",
    "zenoh/string",
    "zenoh/error",
    "application/octet-stream",
    "text/plain",
    "application/json",
    "text/json",
    "application/cdr",
    "application/cbor",
    "application/yaml",
    "text/yaml",
    "text/json5",
    "application/python-serialized-object",
    "application/protobuf",
    "application/java-serialized-object",
    "application/openmetrics-text",
    "image/png",
    "image/jpeg",
    "image/gif",
    "image/bmp",
    "image/webp",
    "application/xml",
    "application/x-www-form-urlencoded",
    "text/html",
    "text/xml",
    "text/css",
    "text/javascript",
    "text/markdown",
    "text/csv",
    "application/sql",
    "application/coap-payload",
    "application/json-patch+json",
    "application/json-seq",
    "application/jsonpath",
    "application/jwt",
    "application/mp4",
    "application/soap+xml",
    "application/yang",
    "audio/aac",
    "audio/flac",
    "audio/mp4",
    "audio/ogg",
    "audio/vorbis",
    "video/h261",
    "video/h263",
    "video/h264",
    "video/h265",
    "video/h266",
    "video/mp4",
    "video/ogg",
    "video/raw",
    "video/vp8",
    "video/vp9",
};

static uint16_t _z_encoding_values_str_to_int(const char *schema, size_t len) {
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(ENCODING_VALUES_ID_TO_STR); i++) {
        if (strncmp(schema, ENCODING_VALUES_ID_TO_STR[i], len) == 0) {
            return (uint16_t)i;
        }
    }
    return UINT16_MAX;
}

static int8_t _z_encoding_convert_from_substr(z_owned_encoding_t *encoding, const char *s, size_t len) {
    size_t pos = 0;
    for (; pos < len; ++pos) {
        if (s[pos] == ENCODING_SCHEMA_SEPARATOR) break;
    }

    // Check id_end value + corner cases
    if ((pos != len) && (pos != 0)) {
        uint16_t id = _z_encoding_values_str_to_int(s, pos);
        // Check id
        if (id != UINT16_MAX) {
            const char *ptr = (pos + 1 == len) ? NULL : s + pos + 1;
            return _z_encoding_make(&encoding->_val, id, ptr, len - pos - 1);
        }
    }
    // By default store the string as schema
    return _z_encoding_make(&encoding->_val, _Z_ENCODING_ID_DEFAULT, s, len);
}

static int8_t _z_encoding_convert_into_string(const z_loaned_encoding_t *encoding, z_owned_string_t *s) {
    const char *prefix = NULL;
    size_t prefix_len = 0;
    // Convert id
    if (encoding->id < _ZP_ARRAY_SIZE(ENCODING_VALUES_ID_TO_STR)) {
        prefix = ENCODING_VALUES_ID_TO_STR[encoding->id];
        prefix_len = strlen(prefix);
    }
    _Bool has_schema = _z_string_len(&encoding->schema) > 0;
    // Size include null terminator
    size_t total_len = prefix_len + _z_string_len(&encoding->schema) + 1;
    // Check for schema separator
    if (has_schema) {
        total_len += 1;
    }
    // Allocate string
    char *value = (char *)z_malloc(sizeof(char) * total_len);
    memset(value, 0, total_len);
    if (value == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Copy prefix
    char sep = ENCODING_SCHEMA_SEPARATOR;
    (void)strncpy(value, prefix, prefix_len);
    // Copy schema and separator
    if (has_schema) {
        (void)strncat(value, &sep, 1);
        (void)strncat(value, _z_string_data(&encoding->schema), _z_string_len(&encoding->schema));
    }
    // Fill container
    s->_val = _z_string_alias_str(value);
    return _Z_RES_OK;
}

#else
static int8_t _z_encoding_convert_from_substr(z_owned_encoding_t *encoding, const char *s, size_t len) {
    return _z_encoding_make(encoding->_val, _Z_ENCODING_ID_DEFAULT, s, len);
}

static int8_t _z_encoding_convert_into_string(const z_loaned_encoding_t *encoding, z_owned_string_t *s) {
    _z_string_copy(s->_val, &encoding->schema);
    return _Z_RES_OK;
}

#endif

_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_encoding_t, encoding, _z_encoding_check, _z_encoding_null, _z_encoding_copy,
                              _z_encoding_clear)

int8_t z_encoding_from_str(z_owned_encoding_t *encoding, const char *s) {
    // Init owned encoding
    z_internal_encoding_null(encoding);
    // Convert string to encoding
    if (s != NULL) {
        return _z_encoding_convert_from_substr(encoding, s, strlen(s));
    }
    return _Z_RES_OK;
}

int8_t z_encoding_from_substr(z_owned_encoding_t *encoding, const char *s, size_t len) {
    // Init owned encoding
    z_internal_encoding_null(encoding);
    // Convert string to encoding
    if (s != NULL) {
        return _z_encoding_convert_from_substr(encoding, s, len);
    }
    return _Z_RES_OK;
}
int8_t z_encoding_set_schema_from_str(z_loaned_encoding_t *encoding, const char *schema) {
    return z_encoding_set_schema_from_substr(encoding, schema, strlen(schema));
}

int8_t z_encoding_set_schema_from_substr(z_loaned_encoding_t *encoding, const char *schema, size_t len) {
    _z_string_clear(&encoding->schema);
    if (schema == NULL && len > 0) {
        return _Z_ERR_INVALID;
    }
    encoding->schema = _z_string_copy_from_substr(schema, len);
    if (_z_string_len(&encoding->schema) != len) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

int8_t z_encoding_to_string(const z_loaned_encoding_t *encoding, z_owned_string_t *s) {
    // Init owned string
    z_internal_string_null(s);
    // Convert encoding to string
    _z_encoding_convert_into_string(encoding, s);
    return _Z_RES_OK;
}

int8_t z_slice_copy_from_buf(z_owned_slice_t *slice, const uint8_t *data, size_t len) {
    slice->_val = _z_slice_copy_from_buf(data, len);
    if (slice->_val.start == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    } else {
        return _Z_RES_OK;
    }
}

int8_t z_slice_from_buf(z_owned_slice_t *slice, uint8_t *data, size_t len, void (*deleter)(void *data, void *context),
                        void *context) {
    slice->_val = _z_slice_from_buf_custom_deleter(data, len, _z_delete_context_create(deleter, context));
    return _Z_RES_OK;
}

const uint8_t *z_slice_data(const z_loaned_slice_t *slice) { return slice->start; }

size_t z_slice_len(const z_loaned_slice_t *slice) { return slice->len; }

void z_slice_empty(z_owned_slice_t *slice) { slice->_val = _z_slice_empty(); }

bool z_slice_is_empty(const z_loaned_slice_t *slice) { return _z_slice_is_empty(slice); }

int8_t z_bytes_deserialize_into_int8(const z_loaned_bytes_t *bytes, int8_t *dst) {
    return _z_bytes_to_uint8(bytes, (uint8_t *)dst);
}

int8_t z_bytes_deserialize_into_int16(const z_loaned_bytes_t *bytes, int16_t *dst) {
    return _z_bytes_to_uint16(bytes, (uint16_t *)dst);
}

int8_t z_bytes_deserialize_into_int32(const z_loaned_bytes_t *bytes, int32_t *dst) {
    return _z_bytes_to_uint32(bytes, (uint32_t *)dst);
}

int8_t z_bytes_deserialize_into_int64(const z_loaned_bytes_t *bytes, int64_t *dst) {
    return _z_bytes_to_uint64(bytes, (uint64_t *)dst);
}

int8_t z_bytes_deserialize_into_uint8(const z_loaned_bytes_t *bytes, uint8_t *dst) {
    return _z_bytes_to_uint8(bytes, dst);
}

int8_t z_bytes_deserialize_into_uint16(const z_loaned_bytes_t *bytes, uint16_t *dst) {
    return _z_bytes_to_uint16(bytes, dst);
}

int8_t z_bytes_deserialize_into_uint32(const z_loaned_bytes_t *bytes, uint32_t *dst) {
    return _z_bytes_to_uint32(bytes, dst);
}

int8_t z_bytes_deserialize_into_uint64(const z_loaned_bytes_t *bytes, uint64_t *dst) {
    return _z_bytes_to_uint64(bytes, dst);
}

int8_t z_bytes_deserialize_into_float(const z_loaned_bytes_t *bytes, float *dst) {
    return _z_bytes_to_float(bytes, dst);
}

int8_t z_bytes_deserialize_into_double(const z_loaned_bytes_t *bytes, double *dst) {
    return _z_bytes_to_double(bytes, dst);
}

int8_t z_bytes_deserialize_into_slice(const z_loaned_bytes_t *bytes, z_owned_slice_t *dst) {
    dst->_val = _z_slice_empty();
    return _z_bytes_to_slice(bytes, &dst->_val);
}

int8_t z_bytes_deserialize_into_string(const z_loaned_bytes_t *bytes, z_owned_string_t *s) {
    // Init owned string
    z_internal_string_null(s);
    // Convert bytes to string
    size_t len = _z_bytes_len(bytes);
    s->_val = _z_string_preallocate(len);
    if (_z_string_len(&s->_val) != len) return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    _z_bytes_to_buf(bytes, (uint8_t *)_z_string_data(&s->_val), len);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_pair(const z_loaned_bytes_t *bytes, z_owned_bytes_t *first, z_owned_bytes_t *second) {
    // Init pair of owned bytes
    z_bytes_empty(first);
    z_bytes_empty(second);
    return _z_bytes_deserialize_into_pair(bytes, &first->_val, &second->_val);
}

int8_t z_bytes_serialize_from_int8(z_owned_bytes_t *bytes, int8_t val) {
    return z_bytes_serialize_from_uint8(bytes, (uint8_t)val);
}

int8_t z_bytes_serialize_from_int16(z_owned_bytes_t *bytes, int16_t val) {
    return z_bytes_serialize_from_uint16(bytes, (uint16_t)val);
}

int8_t z_bytes_serialize_from_int32(z_owned_bytes_t *bytes, int32_t val) {
    return z_bytes_serialize_from_uint32(bytes, (uint32_t)val);
}

int8_t z_bytes_serialize_from_int64(z_owned_bytes_t *bytes, int64_t val) {
    return z_bytes_serialize_from_uint64(bytes, (uint64_t)val);
}

int8_t z_bytes_serialize_from_uint8(z_owned_bytes_t *bytes, uint8_t val) {
    z_bytes_empty(bytes);
    return _z_bytes_from_uint8(&bytes->_val, val);
}

int8_t z_bytes_serialize_from_uint16(z_owned_bytes_t *bytes, uint16_t val) {
    z_bytes_empty(bytes);
    return _z_bytes_from_uint16(&bytes->_val, val);
}

int8_t z_bytes_serialize_from_uint32(z_owned_bytes_t *bytes, uint32_t val) {
    z_bytes_empty(bytes);
    return _z_bytes_from_uint32(&bytes->_val, val);
}

int8_t z_bytes_serialize_from_uint64(z_owned_bytes_t *bytes, uint64_t val) {
    z_bytes_empty(bytes);
    return _z_bytes_from_uint64(&bytes->_val, val);
}

int8_t z_bytes_serialize_from_float(z_owned_bytes_t *bytes, float val) {
    z_bytes_empty(bytes);
    return _z_bytes_from_float(&bytes->_val, val);
}

int8_t z_bytes_serialize_from_double(z_owned_bytes_t *bytes, double val) {
    z_bytes_empty(bytes);
    return _z_bytes_from_double(&bytes->_val, val);
}

int8_t z_bytes_from_slice(z_owned_bytes_t *bytes, z_moved_slice_t *slice) {
    z_bytes_empty(bytes);
    _z_slice_t s = _z_slice_steal(&slice->_this._val);
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_from_slice(&bytes->_val, s), _z_slice_clear(&s));
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_slice(z_owned_bytes_t *bytes, const z_loaned_slice_t *slice) {
    z_owned_slice_t s;
    _Z_RETURN_IF_ERR(z_slice_clone(&s, slice));
    return z_bytes_from_slice(bytes, z_slice_move(&s));
}

int8_t z_bytes_from_buf(z_owned_bytes_t *bytes, uint8_t *data, size_t len, void (*deleter)(void *data, void *context),
                        void *context) {
    z_owned_slice_t s;
    s._val = _z_slice_from_buf_custom_deleter(data, len, _z_delete_context_create(deleter, context));
    return z_bytes_from_slice(bytes, z_slice_move(&s));
}

int8_t z_bytes_from_static_buf(z_owned_bytes_t *bytes, const uint8_t *data, size_t len) {
    z_owned_slice_t s;
    s._val = _z_slice_alias_buf(data, len);
    return z_bytes_from_slice(bytes, z_slice_move(&s));
}

int8_t z_bytes_serialize_from_buf(z_owned_bytes_t *bytes, const uint8_t *data, size_t len) {
    z_owned_slice_t s;
    _Z_RETURN_IF_ERR(z_slice_copy_from_buf(&s, data, len));
    return z_bytes_from_slice(bytes, z_slice_move(&s));
}

int8_t z_bytes_from_string(z_owned_bytes_t *bytes, z_moved_string_t *s) {
    // TODO, verify that string is a valid UTF-8 ?
    z_owned_slice_t slice;
    size_t str_len = _z_string_len(&s->_this._val);
    slice._val = _z_slice_steal(&s->_this._val._slice);
    slice._val.len = str_len;
    return z_bytes_from_slice(bytes, z_slice_move(&slice));
}

int8_t z_bytes_serialize_from_string(z_owned_bytes_t *bytes, const z_loaned_string_t *s) {
    z_owned_string_t s_copy;
    _Z_RETURN_IF_ERR(z_string_clone(&s_copy, s));
    return z_bytes_from_string(bytes, z_string_move(&s_copy));
}

int8_t z_bytes_from_str(z_owned_bytes_t *bytes, char *value, void (*deleter)(void *value, void *context),
                        void *context) {
    z_owned_string_t s;
    s._val = _z_string_from_str_custom_deleter(value, _z_delete_context_create(deleter, context));
    return z_bytes_from_string(bytes, z_string_move(&s));
}

int8_t z_bytes_from_static_str(z_owned_bytes_t *bytes, const char *value) {
    z_owned_string_t s;
    s._val = _z_string_alias_str(value);
    return z_bytes_from_string(bytes, z_string_move(&s));
}

int8_t z_bytes_serialize_from_str(z_owned_bytes_t *bytes, const char *value) {
    z_owned_string_t s;
    _Z_RETURN_IF_ERR(z_string_copy_from_str(&s, value));
    return z_bytes_from_string(bytes, z_string_move(&s));
}

int8_t z_bytes_from_iter(z_owned_bytes_t *bytes, _Bool (*iterator_body)(z_owned_bytes_t *data, void *context),
                         void *context) {
    z_bytes_empty(bytes);
    z_owned_bytes_t data;
    _z_bytes_iterator_writer_t iter_writer = _z_bytes_get_iterator_writer(&bytes->_val, 0);
    while (iterator_body(&data, context)) {
        _Z_CLEAN_RETURN_IF_ERR(_z_bytes_iterator_writer_write(&iter_writer, &data._val),
                               z_bytes_drop(z_bytes_move(bytes)));
    }
    return _Z_RES_OK;
}

int8_t z_bytes_from_pair(z_owned_bytes_t *bytes, z_moved_bytes_t *first, z_moved_bytes_t *second) {
    z_bytes_empty(bytes);
    return _z_bytes_from_pair(&bytes->_val, &first->_this._val, &second->_this._val);
}

void z_bytes_empty(z_owned_bytes_t *bytes) { bytes->_val = _z_bytes_null(); }

size_t z_bytes_len(const z_loaned_bytes_t *bytes) { return _z_bytes_len(bytes); }

_Bool z_bytes_is_empty(const z_loaned_bytes_t *bytes) { return _z_bytes_is_empty(bytes); }

z_bytes_reader_t z_bytes_get_reader(const z_loaned_bytes_t *bytes) { return _z_bytes_get_iterator(bytes); }

size_t z_bytes_reader_read(z_bytes_reader_t *reader, uint8_t *dst, size_t len) {
    return _z_bytes_reader_read(&reader->_reader, dst, len);
}

int8_t z_bytes_reader_read_bounded(z_bytes_reader_t *reader, z_owned_bytes_t *dst) {
    return _z_bytes_iterator_next(reader, &dst->_val);
}

int8_t z_bytes_reader_seek(z_bytes_reader_t *reader, int64_t offset, int origin) {
    return _z_bytes_reader_seek(&reader->_reader, offset, origin);
}

int64_t z_bytes_reader_tell(z_bytes_reader_t *reader) { return _z_bytes_reader_tell(&reader->_reader); }

z_bytes_iterator_t z_bytes_get_iterator(const z_loaned_bytes_t *bytes) { return _z_bytes_get_iterator(bytes); }

_Bool z_bytes_iterator_next(z_bytes_iterator_t *iter, z_owned_bytes_t *bytes) {
    z_bytes_empty(bytes);
    if (_z_bytes_iterator_next(iter, &bytes->_val) != _Z_RES_OK) {
        z_bytes_drop(z_bytes_move(bytes));
        return false;
    }
    return true;
}

z_bytes_writer_t z_bytes_get_writer(z_loaned_bytes_t *bytes) {
    return _z_bytes_get_iterator_writer(bytes, Z_IOSLICE_SIZE);
}

int8_t z_bytes_writer_write_all(z_bytes_writer_t *writer, const uint8_t *src, size_t len) {
    return _z_bytes_writer_write_all(&writer->writer, src, len);
}

int8_t z_bytes_writer_append(z_bytes_writer_t *writer, z_moved_bytes_t *bytes) {
    return _z_bytes_writer_append(&writer->writer, &bytes->_this._val);
}

int8_t z_bytes_writer_append_bounded(z_bytes_writer_t *writer, z_moved_bytes_t *bytes) {
    return _z_bytes_iterator_writer_write(writer, &bytes->_this._val);
}

int8_t z_timestamp_new(z_timestamp_t *ts, const z_loaned_session_t *zs) {
    *ts = _z_timestamp_null();
    zp_time_since_epoch t;
    _Z_RETURN_IF_ERR(zp_get_time_since_epoch(&t));
    ts->time = _z_timestamp_ntp64_from_time(t.secs, t.nanos);
    ts->id = _Z_RC_IN_VAL(zs)->_local_zid;
    return _Z_RES_OK;
}

uint64_t z_timestamp_ntp64_time(const z_timestamp_t *ts) { return ts->time; }

z_id_t z_timestamp_id(const z_timestamp_t *ts) { return ts->id; }

_Bool z_timestamp_check(z_timestamp_t ts) { return _z_timestamp_check(&ts); }

z_query_target_t z_query_target_default(void) { return Z_QUERY_TARGET_DEFAULT; }

z_query_consolidation_t z_query_consolidation_auto(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_AUTO};
}

z_query_consolidation_t z_query_consolidation_latest(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_LATEST};
}

z_query_consolidation_t z_query_consolidation_monotonic(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_MONOTONIC};
}

z_query_consolidation_t z_query_consolidation_none(void) {
    return (z_query_consolidation_t){.mode = Z_CONSOLIDATION_MODE_NONE};
}

z_query_consolidation_t z_query_consolidation_default(void) { return z_query_consolidation_auto(); }

void z_query_parameters(const z_loaned_query_t *query, z_view_string_t *parameters) {
    parameters->_val = _z_string_alias_str(_Z_RC_IN_VAL(query)->_parameters);
}

const z_loaned_bytes_t *z_query_attachment(const z_loaned_query_t *query) { return &_Z_RC_IN_VAL(query)->attachment; }

const z_loaned_keyexpr_t *z_query_keyexpr(const z_loaned_query_t *query) { return &_Z_RC_IN_VAL(query)->_key; }

const z_loaned_bytes_t *z_query_payload(const z_loaned_query_t *query) { return &_Z_RC_IN_VAL(query)->_value.payload; }
const z_loaned_encoding_t *z_query_encoding(const z_loaned_query_t *query) {
    return &_Z_RC_IN_VAL(query)->_value.encoding;
}

void z_closure_sample_call(const z_loaned_closure_sample_t *closure, const z_loaned_sample_t *sample) {
    if (closure->call != NULL) {
        (closure->call)(sample, closure->context);
    }
}

void z_closure_query_call(const z_loaned_closure_query_t *closure, const z_loaned_query_t *query) {
    if (closure->call != NULL) {
        (closure->call)(query, closure->context);
    }
}

void z_closure_reply_call(const z_loaned_closure_reply_t *closure, const z_loaned_reply_t *reply) {
    if (closure->call != NULL) {
        (closure->call)(reply, closure->context);
    }
}

void z_closure_hello_call(const z_loaned_closure_hello_t *closure, const z_loaned_hello_t *hello) {
    if (closure->call != NULL) {
        (closure->call)(hello, closure->context);
    }
}

void z_closure_zid_call(const z_loaned_closure_zid_t *closure, const z_id_t *id) {
    if (closure->call != NULL) {
        (closure->call)(id, closure->context);
    }
}

_Bool _z_config_check(const _z_config_t *config) { return !_z_str_intmap_is_empty(config); }
_z_config_t _z_config_null(void) { return _z_str_intmap_make(); }
void _z_config_drop(_z_config_t *config) { _z_str_intmap_clear(config); }
_Z_OWNED_FUNCTIONS_VALUE_NO_COPY_IMPL(_z_config_t, config, _z_config_check, _z_config_null, _z_config_drop)

_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_string_t, string, _z_string_check, _z_string_null, _z_string_copy, _z_string_clear)

_Bool _z_value_check(const _z_value_t *value) {
    return _z_encoding_check(&value->encoding) || _z_bytes_check(&value->payload);
}
_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_value_t, reply_err, _z_value_check, _z_value_null, _z_value_copy, _z_value_clear)

_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_keyexpr_t, keyexpr, _z_keyexpr_check, _z_keyexpr_null, _z_keyexpr_copy,
                              _z_keyexpr_clear)
_Z_VIEW_FUNCTIONS_IMPL(_z_keyexpr_t, keyexpr, _z_keyexpr_check, _z_keyexpr_null)
_Z_VIEW_FUNCTIONS_IMPL(_z_string_t, string, _z_string_check, _z_string_null)

_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_hello_t, hello, _z_hello_check, _z_hello_null, _z_hello_copy, _z_hello_clear)

z_id_t z_hello_zid(const z_loaned_hello_t *hello) { return hello->_zid; }

z_whatami_t z_hello_whatami(const z_loaned_hello_t *hello) { return hello->_whatami; }

const z_loaned_string_array_t *z_hello_locators(const z_loaned_hello_t *hello) { return &hello->_locators; }

static const char *WHAT_AM_I_TO_STRING_MAP[8] = {
    "Other",              // 0
    "Router",             // 0b1
    "Peer",               // 0b01
    "Router|Peer",        // 0b11,
    "Client",             // 0b100
    "Router|Client",      // 0b101
    "Peer|Client",        // 0b110
    "Router|Peer|Client"  // 0b111
};

int8_t z_whatami_to_view_string(z_whatami_t whatami, z_view_string_t *str_out) {
    uint8_t idx = (uint8_t)whatami;
    if (idx >= _ZP_ARRAY_SIZE(WHAT_AM_I_TO_STRING_MAP) || idx == 0) {
        z_view_string_from_str(str_out, WHAT_AM_I_TO_STRING_MAP[0]);
        return _Z_ERR_INVALID;
    } else {
        z_view_string_from_str(str_out, WHAT_AM_I_TO_STRING_MAP[idx]);
    }
    return _Z_RES_OK;
}

_Bool _z_string_array_check(const _z_string_svec_t *val) { return !_z_string_svec_is_empty(val); }
int8_t _z_string_array_copy(_z_string_svec_t *dst, const _z_string_svec_t *src) {
    _z_string_svec_copy(dst, src);
    return dst->_len == src->_len ? _Z_RES_OK : _Z_ERR_SYSTEM_OUT_OF_MEMORY;
}
_z_string_svec_t _z_string_array_null(void) { return _z_string_svec_make(0); }
_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_string_svec_t, string_array, _z_string_array_check, _z_string_array_null,
                              _z_string_array_copy, _z_string_svec_clear)
_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_slice_t, slice, _z_slice_check, _z_slice_empty, _z_slice_copy, _z_slice_clear)
_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_bytes_t, bytes, _z_bytes_check, _z_bytes_null, _z_bytes_copy, _z_bytes_drop)

#if Z_FEATURE_PUBLICATION == 1 || Z_FEATURE_QUERYABLE == 1 || Z_FEATURE_QUERY == 1
// Convert a user owned bytes payload to an internal bytes payload, returning an empty one if value invalid
static _z_bytes_t _z_bytes_from_owned_bytes(z_owned_bytes_t *bytes) {
    if ((bytes != NULL)) {
        return bytes->_val;
    } else {
        return _z_bytes_null();
    }
}
#endif

#if Z_FEATURE_QUERYABLE == 1 || Z_FEATURE_QUERY == 1
// Convert a user owned encoding to an internal encoding, return default encoding if value invalid
static _z_encoding_t _z_encoding_from_owned(const z_owned_encoding_t *encoding) {
    if (encoding == NULL) {
        return _z_encoding_null();
    }
    return encoding->_val;
}
#endif

_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_sample_t, sample, _z_sample_check, _z_sample_null, _z_sample_copy, _z_sample_clear)
_Z_OWNED_FUNCTIONS_RC_IMPL(session)

_Z_OWNED_FUNCTIONS_CLOSURE_IMPL(closure_sample, _z_data_handler_t, z_dropper_handler_t)
_Z_OWNED_FUNCTIONS_CLOSURE_IMPL(closure_query, _z_queryable_handler_t, z_dropper_handler_t)
_Z_OWNED_FUNCTIONS_CLOSURE_IMPL(closure_reply, _z_reply_handler_t, z_dropper_handler_t)
_Z_OWNED_FUNCTIONS_CLOSURE_IMPL(closure_hello, z_loaned_hello_handler_t, z_dropper_handler_t)
_Z_OWNED_FUNCTIONS_CLOSURE_IMPL(closure_zid, z_id_handler_t, z_dropper_handler_t)

/************* Primitives **************/
typedef struct __z_hello_handler_wrapper_t {
    z_loaned_hello_handler_t user_call;
    void *ctx;
} __z_hello_handler_wrapper_t;

void __z_hello_handler(_z_hello_t *hello, __z_hello_handler_wrapper_t *wrapped_ctx) {
    wrapped_ctx->user_call(hello, wrapped_ctx->ctx);
}

int8_t z_scout(z_moved_config_t *config, z_moved_closure_hello_t *callback, const z_scout_options_t *options) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->_this._val.context;
    callback->_this._val.context = NULL;

    // TODO[API-NET]: When API and NET are a single layer, there is no wrap the user callback and args
    //                to enclose the z_reply_t into a z_owned_reply_t.
    __z_hello_handler_wrapper_t *wrapped_ctx =
        (__z_hello_handler_wrapper_t *)z_malloc(sizeof(__z_hello_handler_wrapper_t));
    if (wrapped_ctx != NULL) {
        wrapped_ctx->user_call = callback->_this._val.call;
        wrapped_ctx->ctx = ctx;

        z_what_t what;
        if (options != NULL) {
            what = options->what;
        } else {
            char *opt_as_str = _z_config_get(&config->_this._val, Z_CONFIG_SCOUTING_WHAT_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = (char *)Z_CONFIG_SCOUTING_WHAT_DEFAULT;
            }
            what = strtol(opt_as_str, NULL, 10);
        }

        char *opt_as_str = _z_config_get(&config->_this._val, Z_CONFIG_MULTICAST_LOCATOR_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = (char *)Z_CONFIG_MULTICAST_LOCATOR_DEFAULT;
        }
        _z_string_t mcast_locator = _z_string_alias_str(opt_as_str);

        uint32_t timeout;
        if (options != NULL) {
            timeout = options->timeout_ms;
        } else {
            opt_as_str = _z_config_get(&config->_this._val, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = (char *)Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
            }
            timeout = (uint32_t)strtoul(opt_as_str, NULL, 10);
        }

        _z_id_t zid = _z_id_empty();
        char *zid_str = _z_config_get(&config->_this._val, Z_CONFIG_SESSION_ZID_KEY);
        if (zid_str != NULL) {
            _z_uuid_to_bytes(zid.id, zid_str);
        }

        _z_scout(what, zid, &mcast_locator, timeout, __z_hello_handler, wrapped_ctx, callback->_this._val.drop, ctx);

        z_free(wrapped_ctx);
        z_config_drop(config);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    z_internal_closure_hello_null(&callback->_this);

    return ret;
}

int8_t z_open(z_owned_session_t *zs, z_moved_config_t *config) {
    z_internal_session_null(zs);
    _z_session_t *s = z_malloc(sizeof(_z_session_t));
    if (s == NULL) {
        z_config_drop(config);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    memset(s, 0, sizeof(_z_session_t));
    // Create rc
    _z_session_rc_t zsrc = _z_session_rc_new(s);
    if (zsrc._cnt == NULL) {
        z_free(s);
        z_config_drop(config);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    zs->_rc = zsrc;
    // Open session
    int8_t ret = _z_open(&zs->_rc, &config->_this._val);
    if (ret != _Z_RES_OK) {
        _z_session_rc_decr(&zs->_rc);
        z_internal_session_null(zs);
        z_config_drop(config);
        return ret;
    }
    // Clean up
    z_config_drop(config);
    return _Z_RES_OK;
}

int8_t z_close(z_moved_session_t *zs) {
    if (zs == NULL || !z_internal_session_check(&zs->_this)) {
        return _Z_RES_OK;
    }
    z_session_drop(zs);
    return _Z_RES_OK;
}

int8_t z_info_peers_zid(const z_loaned_session_t *zs, z_moved_closure_zid_t *callback) {
    // Call transport function
    switch (_Z_RC_IN_VAL(zs)->_tp._type) {
        case _Z_TRANSPORT_MULTICAST_TYPE:
        case _Z_TRANSPORT_RAWETH_TYPE:
            _zp_multicast_fetch_zid(&(_Z_RC_IN_VAL(zs)->_tp), &callback->_this._val);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->_this._val.context;
    callback->_this._val.context = NULL;
    // Drop if needed
    if (callback->_this._val.drop != NULL) {
        callback->_this._val.drop(ctx);
    }
    z_internal_closure_zid_null(&callback->_this);
    return 0;
}

int8_t z_info_routers_zid(const z_loaned_session_t *zs, z_moved_closure_zid_t *callback) {
    // Call transport function
    switch (_Z_RC_IN_VAL(zs)->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _zp_unicast_fetch_zid(&(_Z_RC_IN_VAL(zs)->_tp), &callback->_this._val);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->_this._val.context;
    callback->_this._val.context = NULL;
    // Drop if needed
    if (callback->_this._val.drop != NULL) {
        callback->_this._val.drop(ctx);
    }
    z_internal_closure_zid_null(&callback->_this);
    return 0;
}

z_id_t z_info_zid(const z_loaned_session_t *zs) { return _Z_RC_IN_VAL(zs)->_local_zid; }

z_result_t z_id_to_string(z_owned_string_t *str, z_id_t *id) {
    _z_slice_t buf = _z_slice_alias_buf(id->id, sizeof(id->id));
    str->_val = _z_string_convert_bytes(&buf);
    if (!_z_string_check(&str->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

const z_loaned_keyexpr_t *z_sample_keyexpr(const z_loaned_sample_t *sample) { return &sample->keyexpr; }
z_sample_kind_t z_sample_kind(const z_loaned_sample_t *sample) { return sample->kind; }
const z_loaned_bytes_t *z_sample_payload(const z_loaned_sample_t *sample) { return &sample->payload; }
const z_timestamp_t *z_sample_timestamp(const z_loaned_sample_t *sample) {
    if (_z_timestamp_check(&sample->timestamp)) {
        return &sample->timestamp;
    } else {
        return NULL;
    }
}
const z_loaned_encoding_t *z_sample_encoding(const z_loaned_sample_t *sample) { return &sample->encoding; }
const z_loaned_bytes_t *z_sample_attachment(const z_loaned_sample_t *sample) { return &sample->attachment; }
z_congestion_control_t z_sample_congestion_control(const z_loaned_sample_t *sample) {
    return _z_n_qos_get_congestion_control(sample->qos);
}
bool z_sample_express(const z_loaned_sample_t *sample) { return _z_n_qos_get_express(sample->qos); }
z_priority_t z_sample_priority(const z_loaned_sample_t *sample) { return _z_n_qos_get_priority(sample->qos); }

const z_loaned_bytes_t *z_reply_err_payload(const z_loaned_reply_err_t *reply_err) { return &reply_err->payload; }
const z_loaned_encoding_t *z_reply_err_encoding(const z_loaned_reply_err_t *reply_err) { return &reply_err->encoding; }

const char *z_string_data(const z_loaned_string_t *str) { return _z_string_data(str); }
size_t z_string_len(const z_loaned_string_t *str) { return _z_string_len(str); }

int8_t z_string_copy_from_str(z_owned_string_t *str, const char *value) {
    str->_val = _z_string_copy_from_str(value);
    if (str->_val._slice.start == NULL && value != NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

int8_t z_string_from_str(z_owned_string_t *str, char *value, void (*deleter)(void *value, void *context),
                         void *context) {
    str->_val = _z_string_from_str_custom_deleter(value, _z_delete_context_create(deleter, context));
    return _Z_RES_OK;
}

void z_string_empty(z_owned_string_t *str) { str->_val = _z_string_null(); }

int8_t z_string_copy_from_substr(z_owned_string_t *str, const char *value, size_t len) {
    str->_val = _z_string_copy_from_substr(value, len);
    return _Z_RES_OK;
}

bool z_string_is_empty(const z_loaned_string_t *str) { return _z_string_is_empty(str); }

#if Z_FEATURE_PUBLICATION == 1
int8_t _z_undeclare_and_clear_publisher(_z_publisher_t *pub) {
    int8_t ret = _Z_RES_OK;
    ret = _z_undeclare_publisher(pub);
    _z_publisher_clear(pub);
    return ret;
}

void _z_publisher_drop(_z_publisher_t *pub) { _z_undeclare_and_clear_publisher(pub); }

_Z_OWNED_FUNCTIONS_VALUE_NO_COPY_IMPL(_z_publisher_t, publisher, _z_publisher_check, _z_publisher_null,
                                      _z_publisher_drop)

void z_put_options_default(z_put_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
    options->encoding = NULL;
    options->is_express = false;
    options->timestamp = NULL;
    options->attachment = NULL;
}

void z_delete_options_default(z_delete_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->is_express = false;
    options->timestamp = NULL;
    options->priority = Z_PRIORITY_DEFAULT;
}

int8_t z_put(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, z_moved_bytes_t *payload,
             const z_put_options_t *options) {
    int8_t ret = 0;

    z_put_options_t opt;
    z_put_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.encoding = options->encoding;
        opt.priority = options->priority;
        opt.is_express = options->is_express;
        opt.timestamp = options->timestamp;
        opt.attachment = options->attachment;
    }

    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);
    ret = _z_write(_Z_RC_IN_VAL(zs), keyexpr_aliased, _z_bytes_from_owned_bytes(&payload->_this),
                   opt.encoding == NULL ? NULL : &opt.encoding->_this._val, Z_SAMPLE_KIND_PUT, opt.congestion_control,
                   opt.priority, opt.is_express, opt.timestamp, _z_bytes_from_owned_bytes(&opt.attachment->_this));

    // Trigger local subscriptions
    _z_trigger_local_subscriptions(
        _Z_RC_IN_VAL(zs), keyexpr_aliased, _z_bytes_from_owned_bytes(&payload->_this),
        opt.encoding == NULL ? NULL : &opt.encoding->_this._val,
        _z_n_qos_make(opt.is_express, opt.congestion_control == Z_CONGESTION_CONTROL_BLOCK, opt.priority),
        opt.timestamp, _z_bytes_from_owned_bytes(&opt.attachment->_this));
    // Clean-up
    z_encoding_drop(opt.encoding);
    z_bytes_drop(opt.attachment);
    z_bytes_drop(payload);
    return ret;
}

int8_t z_delete(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const z_delete_options_t *options) {
    int8_t ret = 0;

    z_delete_options_t opt;
    z_delete_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
        opt.is_express = options->is_express;
        opt.timestamp = options->timestamp;
    }
    ret = _z_write(_Z_RC_IN_VAL(zs), *keyexpr, _z_bytes_null(), NULL, Z_SAMPLE_KIND_DELETE, opt.congestion_control,
                   opt.priority, opt.is_express, opt.timestamp, _z_bytes_null());

    return ret;
}

void z_publisher_options_default(z_publisher_options_t *options) {
    options->encoding = NULL;
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
    options->is_express = false;
}

int8_t z_declare_publisher(z_owned_publisher_t *pub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                           const z_publisher_options_t *options) {
    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);
    _z_keyexpr_t key = keyexpr_aliased;

    pub->_val = _z_publisher_null();
    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(_Z_RC_IN_VAL(zs), &keyexpr_aliased);
        if (r == NULL) {
            uint16_t id = _z_declare_resource(_Z_RC_IN_VAL(zs), keyexpr_aliased);
            key = _z_keyexpr_from_string(id, &keyexpr_aliased._suffix);
        }
    }
    // Set options
    z_publisher_options_t opt;
    z_publisher_options_default(&opt);
    if (options != NULL) {
        opt = *options;
    }
    // Set publisher
    _z_publisher_t int_pub = _z_declare_publisher(zs, key, opt.encoding == NULL ? NULL : &opt.encoding->_this._val,
                                                  opt.congestion_control, opt.priority, opt.is_express);
    // Create write filter
    int8_t res = _z_write_filter_create(&int_pub);
    if (res != _Z_RES_OK) {
        if (key._id != Z_RESOURCE_ID_NONE) {
            _z_undeclare_resource(_Z_RC_IN_VAL(zs), key._id);
        }
        return res;
    }
    pub->_val = int_pub;
    return _Z_RES_OK;
}

int8_t z_undeclare_publisher(z_moved_publisher_t *pub) { return _z_undeclare_and_clear_publisher(&pub->_this._val); }

void z_publisher_put_options_default(z_publisher_put_options_t *options) {
    options->encoding = NULL;
    options->attachment = NULL;
    options->timestamp = NULL;
}

void z_publisher_delete_options_default(z_publisher_delete_options_t *options) { options->timestamp = NULL; }

int8_t z_publisher_put(const z_loaned_publisher_t *pub, z_moved_bytes_t *payload,
                       const z_publisher_put_options_t *options) {
    int8_t ret = 0;
    // Build options
    z_publisher_put_options_t opt;
    z_publisher_put_options_default(&opt);
    if (options != NULL) {
        opt.encoding = options->encoding;
        opt.timestamp = options->timestamp;
        opt.attachment = options->attachment;
    }
    _z_encoding_t encoding;
    if (opt.encoding == NULL) {
        _Z_RETURN_IF_ERR(_z_encoding_copy(&encoding, &pub->_encoding));
    } else {
        encoding = _z_encoding_steal(&opt.encoding->_this._val);
    }
    // Remove potentially redundant ke suffix
    _z_keyexpr_t pub_keyexpr = _z_keyexpr_alias_from_user_defined(pub->_key, true);

    // Check if write filter is active before writing
    if (!_z_write_filter_active(pub)) {
        // Write value
        ret = _z_write(_Z_RC_IN_VAL(&pub->_zn), pub_keyexpr, _z_bytes_from_owned_bytes(&payload->_this), &encoding,
                       Z_SAMPLE_KIND_PUT, pub->_congestion_control, pub->_priority, pub->_is_express, opt.timestamp,
                       _z_bytes_from_owned_bytes(&opt.attachment->_this));
    }
    // Trigger local subscriptions
    _z_trigger_local_subscriptions(
        _Z_RC_IN_VAL(&pub->_zn), pub_keyexpr, _z_bytes_from_owned_bytes(&payload->_this), &encoding,
        _z_n_qos_make(pub->_is_express, pub->_congestion_control == Z_CONGESTION_CONTROL_BLOCK, pub->_priority),
        opt.timestamp, _z_bytes_from_owned_bytes(&opt.attachment->_this));
    // Clean-up
    _z_encoding_clear(&encoding);
    z_bytes_drop(opt.attachment);
    z_bytes_drop(payload);
    return ret;
}

int8_t z_publisher_delete(const z_loaned_publisher_t *pub, const z_publisher_delete_options_t *options) {
    // Build options
    z_publisher_delete_options_t opt;
    z_publisher_delete_options_default(&opt);
    if (options != NULL) {
        opt.timestamp = options->timestamp;
    }
    // Remove potentially redundant ke suffix
    _z_keyexpr_t pub_keyexpr = _z_keyexpr_alias_from_user_defined(pub->_key, true);

    return _z_write(_Z_RC_IN_VAL(&pub->_zn), pub_keyexpr, _z_bytes_null(), NULL, Z_SAMPLE_KIND_DELETE,
                    pub->_congestion_control, pub->_priority, pub->_is_express, opt.timestamp, _z_bytes_null());
}

const z_loaned_keyexpr_t *z_publisher_keyexpr(const z_loaned_publisher_t *publisher) {
    return (const z_loaned_keyexpr_t *)&publisher->_key;
}
#endif

#if Z_FEATURE_QUERY == 1
_Bool _z_reply_check(const _z_reply_t *reply) {
    if (reply->data._tag == _Z_REPLY_TAG_DATA) {
        return _z_sample_check(&reply->data._result.sample);
    } else if (reply->data._tag == _Z_REPLY_TAG_ERROR) {
        return _z_value_check(&reply->data._result.error);
    }
    return false;
}
_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_reply_t, reply, _z_reply_check, _z_reply_null, _z_reply_copy, _z_reply_clear)

void z_get_options_default(z_get_options_t *options) {
    options->target = z_query_target_default();
    options->consolidation = z_query_consolidation_default();
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
    options->is_express = false;
    options->encoding = NULL;
    options->payload = NULL;
    options->attachment = NULL;
    options->timeout_ms = Z_GET_TIMEOUT_DEFAULT;
}

int8_t z_get(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const char *parameters,
             z_moved_closure_reply_t *callback, z_get_options_t *options) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->_this._val.context;
    callback->_this._val.context = NULL;

    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);

    z_get_options_t opt;
    z_get_options_default(&opt);
    if (options != NULL) {
        opt.consolidation = options->consolidation;
        opt.target = options->target;
        opt.encoding = options->encoding;
        opt.payload = options->payload;
        opt.attachment = options->attachment;
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
        opt.is_express = options->is_express;
    }

    if (opt.consolidation.mode == Z_CONSOLIDATION_MODE_AUTO) {
        const char *lp = (parameters == NULL) ? "" : parameters;
        if (strstr(lp, Z_SELECTOR_TIME) != NULL) {
            opt.consolidation.mode = Z_CONSOLIDATION_MODE_NONE;
        } else {
            opt.consolidation.mode = Z_CONSOLIDATION_MODE_LATEST;
        }
    }
    // Set value
    _z_value_t value = {.payload = _z_bytes_from_owned_bytes(&opt.payload->_this),
                        .encoding = _z_encoding_from_owned(&opt.encoding->_this)};

    ret = _z_query(_Z_RC_IN_VAL(zs), keyexpr_aliased, parameters, opt.target, opt.consolidation.mode, value,
                   callback->_this._val.call, callback->_this._val.drop, ctx, opt.timeout_ms,
                   _z_bytes_from_owned_bytes(&opt.attachment->_this), opt.congestion_control, opt.priority,
                   opt.is_express);
    // Clean-up
    z_bytes_drop(opt.payload);
    z_encoding_drop(opt.encoding);
    z_bytes_drop(opt.attachment);
    z_internal_closure_reply_null(
        &callback->_this);  // call and drop passed to _z_query, so we nullify the closure here
    return ret;
}

_Bool z_reply_is_ok(const z_loaned_reply_t *reply) { return reply->data._tag != _Z_REPLY_TAG_ERROR; }

const z_loaned_sample_t *z_reply_ok(const z_loaned_reply_t *reply) { return &reply->data._result.sample; }

const z_loaned_reply_err_t *z_reply_err(const z_loaned_reply_t *reply) { return &reply->data._result.error; }

_Bool z_reply_replier_id(const z_loaned_reply_t *reply, z_id_t *out_id) {
    if (_z_id_check(reply->data.replier_id)) {
        *out_id = reply->data.replier_id;
        return true;
    }
    return false;
}
#endif

#if Z_FEATURE_QUERYABLE == 1
_Z_OWNED_FUNCTIONS_RC_IMPL(query)

int8_t _z_undeclare_and_clear_queryable(_z_queryable_t *queryable) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_queryable(queryable);
    _z_queryable_clear(queryable);
    return ret;
}

void _z_queryable_drop(_z_queryable_t *queryable) { _z_undeclare_and_clear_queryable(queryable); }

_Z_OWNED_FUNCTIONS_VALUE_NO_COPY_IMPL(_z_queryable_t, queryable, _z_queryable_check, _z_queryable_null,
                                      _z_queryable_drop)

void z_queryable_options_default(z_queryable_options_t *options) { options->complete = _Z_QUERYABLE_COMPLETE_DEFAULT; }

int8_t z_declare_queryable(z_owned_queryable_t *queryable, const z_loaned_session_t *zs,
                           const z_loaned_keyexpr_t *keyexpr, z_moved_closure_query_t *callback,
                           const z_queryable_options_t *options) {
    void *ctx = callback->_this._val.context;
    callback->_this._val.context = NULL;

    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);
    _z_keyexpr_t key = keyexpr_aliased;

    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(_Z_RC_IN_VAL(zs), &keyexpr_aliased);
        if (r == NULL) {
            uint16_t id = _z_declare_resource(_Z_RC_IN_VAL(zs), keyexpr_aliased);
            key = _z_rid_with_suffix(id, NULL);
        }
    }

    z_queryable_options_t opt;
    z_queryable_options_default(&opt);
    if (options != NULL) {
        opt.complete = options->complete;
    }

    queryable->_val =
        _z_declare_queryable(zs, key, opt.complete, callback->_this._val.call, callback->_this._val.drop, ctx);

    z_internal_closure_query_null(&callback->_this);
    return _Z_RES_OK;
}

int8_t z_undeclare_queryable(z_moved_queryable_t *queryable) {
    return _z_undeclare_and_clear_queryable(&queryable->_this._val);
}

void z_query_reply_options_default(z_query_reply_options_t *options) {
    options->encoding = NULL;
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
    options->timestamp = NULL;
    options->is_express = false;
    options->attachment = NULL;
}

int8_t z_query_reply(const z_loaned_query_t *query, const z_loaned_keyexpr_t *keyexpr, z_moved_bytes_t *payload,
                     const z_query_reply_options_t *options) {
    // Try upgrading session weak to rc
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(&_Z_RC_IN_VAL(query)->_zn);
    if (_Z_RC_IS_NULL(&sess_rc)) {
        return _Z_ERR_CONNECTION_CLOSED;
    }
    // Set options
    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);
    z_query_reply_options_t opts;
    if (options == NULL) {
        z_query_reply_options_default(&opts);
    } else {
        opts = *options;
    }
    // Set value
    _z_value_t value = {.payload = _z_bytes_from_owned_bytes(&payload->_this),
                        .encoding = _z_encoding_from_owned(&opts.encoding->_this)};

    int8_t ret = _z_send_reply(_Z_RC_IN_VAL(query), &sess_rc, keyexpr_aliased, value, Z_SAMPLE_KIND_PUT,
                               opts.congestion_control, opts.priority, opts.is_express, opts.timestamp,
                               _z_bytes_from_owned_bytes(&opts.attachment->_this));
    z_bytes_drop(payload);
    // Clean-up
    _z_session_rc_drop(&sess_rc);
    z_encoding_drop(opts.encoding);
    z_bytes_drop(opts.attachment);
    return ret;
}

void z_query_reply_del_options_default(z_query_reply_del_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
    options->timestamp = NULL;
    options->is_express = false;
    options->attachment = NULL;
}

int8_t z_query_reply_del(const z_loaned_query_t *query, const z_loaned_keyexpr_t *keyexpr,
                         const z_query_reply_del_options_t *options) {
    // Try upgrading session weak to rc
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(&_Z_RC_IN_VAL(query)->_zn);
    if (_Z_RC_IS_NULL(&sess_rc)) {
        return _Z_ERR_CONNECTION_CLOSED;
    }
    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);
    z_query_reply_del_options_t opts;
    if (options == NULL) {
        z_query_reply_del_options_default(&opts);
    } else {
        opts = *options;
    }

    _z_value_t value = {.payload = _z_bytes_null(), .encoding = _z_encoding_null()};

    int8_t ret = _z_send_reply(_Z_RC_IN_VAL(query), &sess_rc, keyexpr_aliased, value, Z_SAMPLE_KIND_DELETE,
                               opts.congestion_control, opts.priority, opts.is_express, opts.timestamp,
                               _z_bytes_from_owned_bytes(&opts.attachment->_this));
    // Clean-up
    _z_session_rc_drop(&sess_rc);
    z_bytes_drop(opts.attachment);
    return ret;
}

void z_query_reply_err_options_default(z_query_reply_err_options_t *options) { options->encoding = NULL; }

int8_t z_query_reply_err(const z_loaned_query_t *query, z_moved_bytes_t *payload,
                         const z_query_reply_err_options_t *options) {
    // Try upgrading session weak to rc
    _z_session_rc_t sess_rc = _z_session_weak_upgrade(&_Z_RC_IN_VAL(query)->_zn);
    if (_Z_RC_IS_NULL(&sess_rc)) {
        return _Z_ERR_CONNECTION_CLOSED;
    }
    z_query_reply_err_options_t opts;
    if (options == NULL) {
        z_query_reply_err_options_default(&opts);
    } else {
        opts = *options;
    }
    // Set value
    _z_value_t value = {.payload = _z_bytes_from_owned_bytes(&payload->_this),
                        .encoding = _z_encoding_from_owned(&opts.encoding->_this)};

    int8_t ret = _z_send_reply_err(_Z_RC_IN_VAL(query), &sess_rc, value);
    z_bytes_drop(payload);
    // Clean-up
    z_encoding_drop(opts.encoding);
    return ret;
}
#endif

int8_t z_keyexpr_from_str_autocanonize(z_owned_keyexpr_t *key, const char *name) {
    size_t len = strlen(name);
    return z_keyexpr_from_substr_autocanonize(key, name, &len);
}

int8_t z_keyexpr_from_substr_autocanonize(z_owned_keyexpr_t *key, const char *name, size_t *len) {
    z_internal_keyexpr_null(key);

    // Copy the suffix
    key->_val._suffix = _z_string_preallocate(*len);
    if (!_z_keyexpr_has_suffix(&key->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    memcpy((char *)_z_string_data(&key->_val._suffix), name, _z_string_len(&key->_val._suffix));
    // Canonize the suffix
    _Z_CLEAN_RETURN_IF_ERR(
        z_keyexpr_canonize((char *)_z_string_data(&key->_val._suffix), &key->_val._suffix._slice.len),
        _z_keyexpr_clear(&key->_val));
    *len = _z_string_len(&key->_val._suffix);
    return _Z_RES_OK;
}

int8_t z_keyexpr_from_str(z_owned_keyexpr_t *key, const char *name) {
    return z_keyexpr_from_substr(key, name, strlen(name));
}

int8_t z_keyexpr_from_substr(z_owned_keyexpr_t *key, const char *name, size_t len) {
    z_internal_keyexpr_null(key);
    key->_val._suffix = _z_string_preallocate(len);
    if (!_z_keyexpr_has_suffix(&key->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    memcpy((char *)_z_string_data(&key->_val._suffix), name, _z_string_len(&key->_val._suffix));
    return _Z_RES_OK;
}

int8_t z_declare_keyexpr(z_owned_keyexpr_t *key, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr) {
    _z_keyexpr_t k = _z_keyexpr_alias_from_user_defined(*keyexpr, false);
    uint16_t id = _z_declare_resource(_Z_RC_IN_VAL(zs), k);
    key->_val = _z_rid_with_suffix(id, NULL);
    // we still need to store the original suffix, for user needs
    // (for example to compare keys or perform other operations on their string representation).
    // Generally this breaks internal keyexpr representation, but is ok for user-defined keyexprs
    // since they consist of 2 disjoint sets: either they have a non-nul suffix or non-trivial id/mapping.
    // The resulting keyexpr can be separated later into valid internal keys using _z_keyexpr_alias_from_user_defined.
    if (_z_keyexpr_has_suffix(keyexpr)) {
        _Z_RETURN_IF_ERR(_z_string_copy(&key->_val._suffix, &keyexpr->_suffix));
    }
    return _Z_RES_OK;
}

int8_t z_undeclare_keyexpr(z_moved_keyexpr_t *keyexpr, const z_loaned_session_t *zs) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_resource(_Z_RC_IN_VAL(zs), keyexpr->_this._val._id);
    z_keyexpr_drop(keyexpr);

    return ret;
}

#if Z_FEATURE_SUBSCRIPTION == 1
int8_t _z_undeclare_and_clear_subscriber(_z_subscriber_t *sub) {
    int8_t ret = _Z_RES_OK;
    ret = _z_undeclare_subscriber(sub);
    _z_subscriber_clear(sub);
    return ret;
}

void _z_subscriber_drop(_z_subscriber_t *sub) { _z_undeclare_and_clear_subscriber(sub); }

_Z_OWNED_FUNCTIONS_VALUE_NO_COPY_IMPL(_z_subscriber_t, subscriber, _z_subscriber_check, _z_subscriber_null,
                                      _z_subscriber_drop)

void z_subscriber_options_default(z_subscriber_options_t *options) {
#if Z_FEATURE_UNSTABLE_API == 1
    options->reliability = Z_RELIABILITY_DEFAULT;
#else
    options->__dummy = 0;
#endif
}

int8_t z_declare_subscriber(z_owned_subscriber_t *sub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                            z_moved_closure_sample_t *callback, const z_subscriber_options_t *options) {
    void *ctx = callback->_this._val.context;
    callback->_this._val.context = NULL;

    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);
    _z_keyexpr_t key = _z_keyexpr_alias(keyexpr_aliased);

    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(_Z_RC_IN_VAL(zs), &keyexpr_aliased);
        if (r == NULL) {
            _Bool do_keydecl = true;
            _z_keyexpr_t resource_key = _z_keyexpr_alias(keyexpr_aliased);
            // Remove wild
            char *wild = _z_string_pbrk(&keyexpr_aliased._suffix, "*$");
            if ((wild != NULL) && _z_keyexpr_has_suffix(&keyexpr_aliased)) {
                wild = _z_ptr_char_offset(wild, -1);
                size_t len = _z_ptr_char_diff(wild, _z_string_data(&keyexpr_aliased._suffix));
                resource_key._suffix = _z_string_preallocate(len);

                if (!_z_keyexpr_has_suffix(&resource_key)) {
                    return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
                }
                memcpy((char *)_z_string_data(&resource_key._suffix), _z_string_data(&keyexpr_aliased._suffix), len);
            }
            // Declare resource
            if (do_keydecl) {
                uint16_t id = _z_declare_resource(_Z_RC_IN_VAL(zs), resource_key);
                key = _z_rid_with_suffix(id, wild);
            }
            _z_keyexpr_clear(&resource_key);
        }
    }

    _z_subinfo_t subinfo = _z_subinfo_default();
    if (options != NULL) {
#if Z_FEATURE_UNSTABLE_API == 1
        subinfo.reliability = options->reliability;
#else
        subinfo.reliability = Z_RELIABILITY_DEFAULT;
#endif
    }
    _z_subscriber_t int_sub =
        _z_declare_subscriber(zs, key, subinfo, callback->_this._val.call, callback->_this._val.drop, ctx);

    z_internal_closure_sample_null(&callback->_this);
    sub->_val = int_sub;

    if (!_z_subscriber_check(&sub->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    } else {
        return _Z_RES_OK;
    }
}

int8_t z_undeclare_subscriber(z_moved_subscriber_t *sub) { return _z_undeclare_and_clear_subscriber(&sub->_this._val); }

const z_loaned_keyexpr_t *z_subscriber_keyexpr(const z_loaned_subscriber_t *sub) {
    // Retrieve keyexpr from session
    uint32_t lookup = sub->_entity_id;
    _z_subscription_rc_list_t *tail = _Z_RC_IN_VAL(&sub->_zn)->_local_subscriptions;
    while (tail != NULL) {
        _z_subscription_rc_t *head = _z_subscription_rc_list_head(tail);
        if (_Z_RC_IN_VAL(head)->_id == lookup) {
            return (const z_loaned_keyexpr_t *)&_Z_RC_IN_VAL(head)->_key;
        }
        tail = _z_subscription_rc_list_tail(tail);
    }
    return NULL;
}
#endif

/**************** Tasks ****************/
void zp_task_read_options_default(zp_task_read_options_t *options) {
#if Z_FEATURE_MULTI_THREAD == 1
    options->task_attributes = NULL;
#else
    options->__dummy = 0;
#endif
}

int8_t zp_start_read_task(z_loaned_session_t *zs, const zp_task_read_options_t *options) {
    (void)(options);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_task_read_options_t opt;
    zp_task_read_options_default(&opt);
    if (options != NULL) {
        opt.task_attributes = options->task_attributes;
    }
    return _zp_start_read_task(_Z_RC_IN_VAL(zs), opt.task_attributes);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_read_task(z_loaned_session_t *zs) {
#if Z_FEATURE_MULTI_THREAD == 1
    return _zp_stop_read_task(_Z_RC_IN_VAL(zs));
#else
    (void)(zs);
    return -1;
#endif
}

void zp_task_lease_options_default(zp_task_lease_options_t *options) {
#if Z_FEATURE_MULTI_THREAD == 1
    options->task_attributes = NULL;
#else
    options->__dummy = 0;
#endif
}

int8_t zp_start_lease_task(z_loaned_session_t *zs, const zp_task_lease_options_t *options) {
    (void)(options);
#if Z_FEATURE_MULTI_THREAD == 1
    zp_task_lease_options_t opt;
    zp_task_lease_options_default(&opt);
    if (options != NULL) {
        opt.task_attributes = options->task_attributes;
    }
    return _zp_start_lease_task(_Z_RC_IN_VAL(zs), opt.task_attributes);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_lease_task(z_loaned_session_t *zs) {
#if Z_FEATURE_MULTI_THREAD == 1
    return _zp_stop_lease_task(_Z_RC_IN_VAL(zs));
#else
    (void)(zs);
    return -1;
#endif
}

void zp_read_options_default(zp_read_options_t *options) { options->__dummy = 0; }

int8_t zp_read(const z_loaned_session_t *zs, const zp_read_options_t *options) {
    (void)(options);
    return _zp_read(_Z_RC_IN_VAL(zs));
}

void zp_send_keep_alive_options_default(zp_send_keep_alive_options_t *options) { options->__dummy = 0; }

int8_t zp_send_keep_alive(const z_loaned_session_t *zs, const zp_send_keep_alive_options_t *options) {
    (void)(options);
    return _zp_send_keep_alive(_Z_RC_IN_VAL(zs));
}

void zp_send_join_options_default(zp_send_join_options_t *options) { options->__dummy = 0; }

int8_t zp_send_join(const z_loaned_session_t *zs, const zp_send_join_options_t *options) {
    (void)(options);
    return _zp_send_join(_Z_RC_IN_VAL(zs));
}
