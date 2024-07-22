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
#include "zenoh-pico/utils/result.h"
#include "zenoh-pico/utils/uuid.h"

/********* Data Types Handlers *********/

int8_t z_view_string_empty(z_view_string_t *str) {
    str->_val.val = NULL;
    str->_val.len = 0;
    return _Z_RES_OK;
}

int8_t z_view_string_wrap(z_view_string_t *str, const char *value) {
    str->_val = _z_string_wrap((char *)value);
    return _Z_RES_OK;
}

int8_t z_view_string_from_substring(z_view_string_t *str, const char *value, size_t len) {
    str->_val.val = (char *)value;
    str->_val.len = len;
    return _Z_RES_OK;
}

const z_loaned_string_t *z_string_array_get(const z_loaned_string_array_t *a, size_t k) {
    return _z_string_svec_get(a, k);
}

size_t z_string_array_len(const z_loaned_string_array_t *a) { return _z_string_svec_len(a); }

_Bool z_string_array_is_empty(const z_loaned_string_array_t *a) { return _z_string_svec_is_empty(a); }

int8_t zp_keyexpr_canonize_null_terminated(char *start) {
    zp_keyexpr_canon_status_t ret = Z_KEYEXPR_CANON_SUCCESS;

    size_t len = strlen(start);
    size_t newlen = len;
    ret = _z_keyexpr_canonize(start, &newlen);
    if (newlen < len) {
        start[newlen] = '\0';
    }

    return ret;
}

int8_t z_view_keyexpr_from_str(z_view_keyexpr_t *keyexpr, const char *name) {
    keyexpr->_val = _z_rname(name);
    return _Z_RES_OK;
}

int8_t z_view_keyexpr_from_str_autocanonize(z_view_keyexpr_t *keyexpr, char *name) {
    _Z_RETURN_IF_ERR(zp_keyexpr_canonize_null_terminated(name));
    keyexpr->_val = _z_rname(name);
    return _Z_RES_OK;
}

int8_t z_view_keyexpr_from_str_unchecked(z_view_keyexpr_t *keyexpr, const char *name) {
    keyexpr->_val = _z_rname(name);
    return _Z_RES_OK;
}

int8_t z_keyexpr_as_view_string(const z_loaned_keyexpr_t *keyexpr, z_view_string_t *s) {
    s->_val = _z_string_wrap(keyexpr->_suffix);
    return _Z_RES_OK;
}

int8_t z_keyexpr_concat(z_owned_keyexpr_t *key, const z_loaned_keyexpr_t *left, const char *right, size_t len) {
    z_keyexpr_null(key);
    if (len == 0) {
        return z_keyexpr_clone(key, left);
    } else if (right == NULL) {
        return _Z_ERR_INVALID;
    }
    size_t left_len = strlen(left->_suffix);
    if (left_len == 0) {
        return _Z_ERR_INVALID;
    }
    if (left->_suffix[left_len - 1] == '*' && right[0] == '*') {
        return _Z_ERR_INVALID;
    }

    char *s = z_malloc(left_len + len + 1);
    if (s == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    s[left_len + len] = '\0';

    memcpy(s, left->_suffix, left_len);
    memcpy(s + left_len, right, len);

    key->_val = _z_rname(s);
    _z_keyexpr_set_owns_suffix(&key->_val, true);
    return _Z_RES_OK;
}

int8_t z_keyexpr_join(z_owned_keyexpr_t *key, const z_loaned_keyexpr_t *left, const z_loaned_keyexpr_t *right) {
    z_keyexpr_null(key);

    size_t left_len = strlen(left->_suffix);
    size_t right_len = strlen(right->_suffix);

    char *s = z_malloc(left_len + right_len + 2);
    if (s == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    s[left_len + right_len + 1] = '\0';
    s[left_len] = '/';
    memcpy(s, left->_suffix, left_len);
    memcpy(s + left_len + 1, right->_suffix, right_len);

    _Z_CLEAN_RETURN_IF_ERR(zp_keyexpr_canonize_null_terminated(s), z_free(s));
    key->_val = _z_rname(s);
    _z_keyexpr_set_owns_suffix(&key->_val, true);
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

int8_t z_keyexpr_is_canon(const char *start, size_t len) { return _z_keyexpr_is_canon(start, len); }

int8_t zp_keyexpr_is_canon_null_terminated(const char *start) { return _z_keyexpr_is_canon(start, strlen(start)); }

int8_t z_keyexpr_canonize(char *start, size_t *len) { return _z_keyexpr_canonize(start, len); }

_Bool z_keyexpr_includes(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    return zp_keyexpr_includes_null_terminated(l->_suffix, r->_suffix);
}

_Bool zp_keyexpr_includes_null_terminated(const char *l, const char *r) {
    if (l != NULL && r != NULL) {
        return _z_keyexpr_includes(l, strlen(l), r, strlen(r));
    }
    return false;
}

_Bool z_keyexpr_intersects(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    return zp_keyexpr_intersect_null_terminated(l->_suffix, r->_suffix);
}

_Bool zp_keyexpr_intersect_null_terminated(const char *l, const char *r) {
    if (l != NULL && r != NULL) {
        return _z_keyexpr_intersects(l, strlen(l), r, strlen(r));
    }
    return false;
}

_Bool z_keyexpr_equals(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    return zp_keyexpr_equals_null_terminated(l->_suffix, r->_suffix);
}

_Bool zp_keyexpr_equals_null_terminated(const char *l, const char *r) {
    if (l != NULL && r != NULL) {
        size_t llen = strlen(l);
        if (llen == strlen(r)) {
            if (strncmp(l, r, llen) == 0) {
                return true;
            }
        }
    }
    return false;
}

void z_config_new(z_owned_config_t *config) { config->_val = _z_config_empty(); }

int8_t z_config_default(z_owned_config_t *config) { return _z_config_default(&config->_val); }

int8_t z_config_client(z_owned_config_t *config, const char *locator) {
    return _z_config_client(&config->_val, locator);
}

int8_t z_config_peer(z_owned_config_t *config, const char *locator) {
    if (locator == NULL) {
        return _Z_ERR_INVALID;
    }
    _Z_RETURN_IF_ERR(z_config_default(config));
    _Z_CLEAN_RETURN_IF_ERR(zp_config_insert(&config->_val, Z_CONFIG_MODE_KEY, Z_CONFIG_MODE_PEER),
                           z_config_drop(config));
    _Z_CLEAN_RETURN_IF_ERR(zp_config_insert(&config->_val, Z_CONFIG_CONNECT_KEY, locator), z_config_drop(config));
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
    _Bool has_schema = encoding->schema.len > 0;
    // Size include null terminator
    size_t total_len = prefix_len + encoding->schema.len + 1;
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
        (void)strncat(value, encoding->schema.val, encoding->schema.len);
    }
    // Fill container
    s->_val = _z_string_wrap(value);
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
    z_encoding_null(encoding);
    // Convert string to encoding
    if (s != NULL) {
        return _z_encoding_convert_from_substr(encoding, s, strlen(s));
    }
    return _Z_RES_OK;
}

int8_t z_encoding_from_substr(z_owned_encoding_t *encoding, const char *s, size_t len) {
    // Init owned encoding
    z_encoding_null(encoding);
    // Convert string to encoding
    if (s != NULL) {
        return _z_encoding_convert_from_substr(encoding, s, len);
    }
    return _Z_RES_OK;
}

int8_t z_encoding_to_string(const z_loaned_encoding_t *encoding, z_owned_string_t *s) {
    // Init owned string
    z_string_null(s);
    // Convert encoding to string
    _z_encoding_convert_into_string(encoding, s);
    return _Z_RES_OK;
}

const uint8_t *z_slice_data(const z_loaned_slice_t *slice) { return slice->start; }

size_t z_slice_len(const z_loaned_slice_t *slice) { return slice->len; }

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
    z_string_null(s);
    // Convert bytes to string
    size_t len = _z_bytes_len(bytes);
    s->_val = _z_string_preallocate(len);
    if (s->_val.len != len) return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    _z_bytes_to_buf(bytes, (uint8_t *)s->_val.val, len);
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

int8_t z_bytes_serialize_from_slice(z_owned_bytes_t *bytes, const uint8_t *data, size_t len) {
    z_bytes_empty(bytes);
    _z_slice_t s = _z_slice_wrap((uint8_t *)data, len);
    return _z_bytes_from_slice(&bytes->_val, s);
}

int8_t z_bytes_serialize_from_slice_copy(z_owned_bytes_t *bytes, const uint8_t *data, size_t len) {
    // Allocate bytes
    _z_slice_t s = _z_slice_wrap_copy((uint8_t *)data, len);
    if (!_z_slice_check(&s) && len > 0) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    z_bytes_empty(bytes);
    _Z_CLEAN_RETURN_IF_ERR(_z_bytes_from_slice(&bytes->_val, s), _z_slice_clear(&s));
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_str(z_owned_bytes_t *bytes, const char *s) {
    // Encode string without null terminator
    size_t len = strlen(s);
    return z_bytes_serialize_from_slice(bytes, (uint8_t *)s, len);
}

int8_t z_bytes_serialize_from_str_copy(z_owned_bytes_t *bytes, const char *s) {
    // Encode string without null terminator
    size_t len = strlen(s);
    return z_bytes_serialize_from_slice_copy(bytes, (uint8_t *)s, len);
}

int8_t z_bytes_serialize_from_iter(z_owned_bytes_t *bytes, _Bool (*iterator_body)(z_owned_bytes_t *data, void *context),
                                   void *context) {
    z_bytes_empty(bytes);
    z_owned_bytes_t data;
    _z_bytes_iterator_writer_t iter_writer = _z_bytes_get_iterator_writer(&bytes->_val);
    while (iterator_body(&data, context)) {
        _Z_CLEAN_RETURN_IF_ERR(_z_bytes_iterator_writer_write(&iter_writer, &data._val), z_bytes_drop(bytes));
    }
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_pair(z_owned_bytes_t *bytes, z_owned_bytes_t *first, z_owned_bytes_t *second) {
    z_bytes_empty(bytes);
    return _z_bytes_serialize_from_pair(&bytes->_val, &first->_val, &second->_val);
}

void z_bytes_empty(z_owned_bytes_t *bytes) { bytes->_val = _z_bytes_null(); }

size_t z_bytes_len(const z_loaned_bytes_t *bytes) { return _z_bytes_len(bytes); }

_Bool z_bytes_is_empty(const z_loaned_bytes_t *bytes) { return _z_bytes_is_empty(bytes); }

z_bytes_reader_t z_bytes_get_reader(const z_loaned_bytes_t *bytes) { return _z_bytes_get_reader(bytes); }

size_t z_bytes_reader_read(z_bytes_reader_t *reader, uint8_t *dst, size_t len) {
    return _z_bytes_reader_read(reader, dst, len);
}

int8_t z_bytes_reader_seek(z_bytes_reader_t *reader, int64_t offset, int origin) {
    return _z_bytes_reader_seek(reader, offset, origin);
}

int64_t z_bytes_reader_tell(z_bytes_reader_t *reader) { return _z_bytes_reader_tell(reader); }

z_bytes_iterator_t z_bytes_get_iterator(const z_loaned_bytes_t *bytes) { return _z_bytes_get_iterator(bytes); }

_Bool z_bytes_iterator_next(z_bytes_iterator_t *iter, z_owned_bytes_t *bytes) {
    z_bytes_empty(bytes);
    if (_z_bytes_iterator_next(iter, &bytes->_val) != _Z_RES_OK) {
        z_bytes_drop(bytes);
        return false;
    }
    return true;
}

void z_bytes_get_writer(z_loaned_bytes_t *bytes, z_owned_bytes_writer_t *writer) {
    writer->_val = _z_bytes_get_writer(bytes, Z_IOSLICE_SIZE);
}

int8_t z_bytes_writer_write(z_loaned_bytes_writer_t *writer, const uint8_t *src, size_t len) {
    return _z_bytes_writer_write(writer, src, len);
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
    parameters->_val.val = _Z_RC_IN_VAL(query)->_parameters;
    parameters->_val.len = strlen(_Z_RC_IN_VAL(query)->_parameters);
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
_Z_VIEW_FUNCTIONS_IMPL(_z_keyexpr_t, keyexpr, _z_keyexpr_check)
_Z_VIEW_FUNCTIONS_IMPL(_z_string_t, string, _z_string_check)

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
        z_view_string_wrap(str_out, WHAT_AM_I_TO_STRING_MAP[0]);
        return _Z_ERR_INVALID;
    } else {
        z_view_string_wrap(str_out, WHAT_AM_I_TO_STRING_MAP[idx]);
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
_Z_VIEW_FUNCTIONS_IMPL(_z_string_vec_t, string_array, _z_string_array_check)
_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_slice_t, slice, _z_slice_check, _z_slice_empty, _z_slice_copy, _z_slice_clear)
_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_bytes_t, bytes, _z_bytes_check, _z_bytes_null, _z_bytes_copy, _z_bytes_drop)

_Bool _z_bytes_writer_check(const _z_bytes_writer_t *w) { return w->bytes == NULL; }
_z_bytes_writer_t _z_bytes_writer_null(void) {
    return (_z_bytes_writer_t){.cache = NULL, .cache_size = 0, .bytes = NULL};
}
int8_t _z_bytes_writer_copy(_z_bytes_writer_t *dst, const _z_bytes_writer_t *src) {
    *dst = _z_bytes_get_writer(src->bytes, src->cache_size);
    return _Z_RES_OK;
}
void _z_bytes_writer_drop(_z_bytes_writer_t *w) { *w = _z_bytes_writer_null(); }
_Z_OWNED_FUNCTIONS_VALUE_IMPL(_z_bytes_writer_t, bytes_writer, _z_bytes_writer_check, _z_bytes_writer_null,
                              _z_bytes_writer_copy, _z_bytes_writer_drop)

#if Z_FEATURE_PUBLICATION == 1 || Z_FEATURE_QUERYABLE == 1 || Z_FEATURE_QUERY == 1
// Convert a user owned bytes payload to an internal bytes payload, returning an empty one if value invalid
static _z_bytes_t _z_bytes_from_owned_bytes(z_owned_bytes_t *bytes) {
    if ((bytes != NULL)) {
        return bytes->_val;
    } else {
        return _z_bytes_null();
    }
}

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

int8_t z_scout(z_owned_config_t *config, z_owned_closure_hello_t *callback, const z_scout_options_t *options) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->_val.context;
    callback->_val.context = NULL;

    // TODO[API-NET]: When API and NET are a single layer, there is no wrap the user callback and args
    //                to enclose the z_reply_t into a z_owned_reply_t.
    __z_hello_handler_wrapper_t *wrapped_ctx =
        (__z_hello_handler_wrapper_t *)z_malloc(sizeof(__z_hello_handler_wrapper_t));
    if (wrapped_ctx != NULL) {
        wrapped_ctx->user_call = callback->_val.call;
        wrapped_ctx->ctx = ctx;

        z_what_t what;
        if (options != NULL) {
            what = options->what;
        } else {
            char *opt_as_str = _z_config_get(&config->_val, Z_CONFIG_SCOUTING_WHAT_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = (char *)Z_CONFIG_SCOUTING_WHAT_DEFAULT;
            }
            what = strtol(opt_as_str, NULL, 10);
        }

        char *opt_as_str = _z_config_get(&config->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = (char *)Z_CONFIG_MULTICAST_LOCATOR_DEFAULT;
        }
        char *mcast_locator = opt_as_str;

        uint32_t timeout;
        if (options != NULL) {
            timeout = options->timeout_ms;
        } else {
            opt_as_str = _z_config_get(&config->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
            if (opt_as_str == NULL) {
                opt_as_str = (char *)Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
            }
            timeout = (uint32_t)strtoul(opt_as_str, NULL, 10);
        }

        _z_id_t zid = _z_id_empty();
        char *zid_str = _z_config_get(&config->_val, Z_CONFIG_SESSION_ZID_KEY);
        if (zid_str != NULL) {
            _z_uuid_to_bytes(zid.id, zid_str);
        }

        _z_scout(what, zid, mcast_locator, timeout, __z_hello_handler, wrapped_ctx, callback->_val.drop, ctx);

        z_free(wrapped_ctx);
        z_config_drop(config);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    z_closure_hello_null(callback);

    return ret;
}

int8_t z_open(z_owned_session_t *zs, z_owned_config_t *config) {
    z_session_null(zs);
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
    int8_t ret = _z_open(&zs->_rc, &config->_val);
    if (ret != _Z_RES_OK) {
        _z_session_rc_decr(&zs->_rc);
        z_session_null(zs);
        z_config_drop(config);
        return ret;
    }
    // Clean up
    z_config_drop(config);
    return _Z_RES_OK;
}

int8_t z_close(z_owned_session_t *zs) {
    if (zs == NULL || !z_session_check(zs)) {
        return _Z_RES_OK;
    }
    z_session_drop(zs);
    return _Z_RES_OK;
}

int8_t z_info_peers_zid(const z_loaned_session_t *zs, z_owned_closure_zid_t *callback) {
    // Call transport function
    switch (_Z_RC_IN_VAL(zs)->_tp._type) {
        case _Z_TRANSPORT_MULTICAST_TYPE:
        case _Z_TRANSPORT_RAWETH_TYPE:
            _zp_multicast_fetch_zid(&(_Z_RC_IN_VAL(zs)->_tp), &callback->_val);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->_val.context;
    callback->_val.context = NULL;
    // Drop if needed
    if (callback->_val.drop != NULL) {
        callback->_val.drop(ctx);
    }
    z_closure_zid_null(callback);
    return 0;
}

int8_t z_info_routers_zid(const z_loaned_session_t *zs, z_owned_closure_zid_t *callback) {
    // Call transport function
    switch (_Z_RC_IN_VAL(zs)->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _zp_unicast_fetch_zid(&(_Z_RC_IN_VAL(zs)->_tp), &callback->_val);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->_val.context;
    callback->_val.context = NULL;
    // Drop if needed
    if (callback->_val.drop != NULL) {
        callback->_val.drop(ctx);
    }
    z_closure_zid_null(callback);
    return 0;
}

z_id_t z_info_zid(const z_loaned_session_t *zs) { return _Z_RC_IN_VAL(zs)->_local_zid; }

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
z_qos_t z_sample_qos(const z_loaned_sample_t *sample) { return sample->qos; }
const z_loaned_bytes_t *z_sample_attachment(const z_loaned_sample_t *sample) { return &sample->attachment; }
z_congestion_control_t z_sample_congestion_control(const z_loaned_sample_t *sample) {
    return _z_n_qos_get_congestion_control(sample->qos);
}
bool z_sample_express(const z_loaned_sample_t *sample) { return _z_n_qos_get_express(sample->qos); }
z_priority_t z_sample_priority(const z_loaned_sample_t *sample) { return _z_n_qos_get_priority(sample->qos); }

const z_loaned_bytes_t *z_reply_err_payload(const z_loaned_reply_err_t *reply_err) { return &reply_err->payload; }
const z_loaned_encoding_t *z_reply_err_encoding(const z_loaned_reply_err_t *reply_err) { return &reply_err->encoding; }

const char *z_string_data(const z_loaned_string_t *str) { return str->val; }
size_t z_string_len(const z_loaned_string_t *str) { return str->len; }

int8_t z_string_from_str(z_owned_string_t *str, const char *value) {
    str->_val = _z_string_make(value);
    return _Z_RES_OK;
}

void z_string_empty(z_owned_string_t *str) { str->_val = _z_string_null(); }

int8_t z_string_from_substr(z_owned_string_t *str, const char *value, size_t len) {
    str->_val = _z_string_n_make(value, len);
    return _Z_RES_OK;
}

bool z_string_is_empty(const z_loaned_string_t *str) { return str->val == NULL; }

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

int8_t z_put(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, z_owned_bytes_t *payload,
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

    ret = _z_write(_Z_RC_IN_VAL(zs), keyexpr_aliased, _z_bytes_from_owned_bytes(payload),
                   _z_encoding_from_owned(opt.encoding), Z_SAMPLE_KIND_PUT, opt.congestion_control, opt.priority,
                   opt.is_express, opt.timestamp, _z_bytes_from_owned_bytes(opt.attachment));

    // Trigger local subscriptions
    _z_trigger_local_subscriptions(_Z_RC_IN_VAL(zs), keyexpr_aliased, _z_bytes_from_owned_bytes(payload),
                                   _z_n_qos_make(0, opt.congestion_control == Z_CONGESTION_CONTROL_BLOCK, opt.priority),
                                   _z_bytes_from_owned_bytes(opt.attachment));
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
    ret = _z_write(_Z_RC_IN_VAL(zs), *keyexpr, _z_bytes_null(), _z_encoding_null(), Z_SAMPLE_KIND_DELETE,
                   opt.congestion_control, opt.priority, opt.is_express, opt.timestamp, _z_bytes_null());

    return ret;
}

void z_publisher_options_default(z_publisher_options_t *options) {
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
            key = _z_rid_with_suffix(id, NULL);
        }
    }
    // Set options
    z_publisher_options_t opt;
    z_publisher_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
        opt.is_express = options->is_express;
    }
    // Set publisher
    _z_publisher_t int_pub = _z_declare_publisher(zs, key, opt.congestion_control, opt.priority, opt.is_express);
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

int8_t z_undeclare_publisher(z_owned_publisher_t *pub) { return _z_undeclare_and_clear_publisher(&pub->_val); }

void z_publisher_put_options_default(z_publisher_put_options_t *options) {
    options->encoding = NULL;
    options->attachment = NULL;
    options->is_express = false;
    options->timestamp = NULL;
}

void z_publisher_delete_options_default(z_publisher_delete_options_t *options) {
    options->is_express = false;
    options->timestamp = NULL;
}

int8_t z_publisher_put(const z_loaned_publisher_t *pub, z_owned_bytes_t *payload,
                       const z_publisher_put_options_t *options) {
    int8_t ret = 0;
    // Build options
    z_publisher_put_options_t opt;
    z_publisher_put_options_default(&opt);
    if (options != NULL) {
        opt.encoding = options->encoding;
        opt.is_express = options->is_express;
        opt.timestamp = options->timestamp;
        opt.attachment = options->attachment;
    } else {
        opt.is_express = pub->_is_express;
    }
    // Check if write filter is active before writing
    if (!_z_write_filter_active(pub)) {
        // Write value
        ret = _z_write(_Z_RC_IN_VAL(&pub->_zn), pub->_key, _z_bytes_from_owned_bytes(payload),
                       _z_encoding_from_owned(opt.encoding), Z_SAMPLE_KIND_PUT, pub->_congestion_control,
                       pub->_priority, opt.is_express, opt.timestamp, _z_bytes_from_owned_bytes(opt.attachment));
    }
    // Trigger local subscriptions
    _z_trigger_local_subscriptions(_Z_RC_IN_VAL(&pub->_zn), pub->_key, _z_bytes_from_owned_bytes(payload),
                                   _Z_N_QOS_DEFAULT, _z_bytes_from_owned_bytes(opt.attachment));
    // Clean-up
    z_encoding_drop(opt.encoding);
    z_bytes_drop(opt.attachment);
    z_bytes_drop(payload);
    return ret;
}

int8_t z_publisher_delete(const z_loaned_publisher_t *pub, const z_publisher_delete_options_t *options) {
    // Build options
    z_publisher_delete_options_t opt;
    z_publisher_delete_options_default(&opt);
    if (options != NULL) {
        opt.is_express = options->is_express;
        opt.timestamp = options->timestamp;
    } else {
        opt.is_express = pub->_is_express;
    }
    return _z_write(_Z_RC_IN_VAL(&pub->_zn), pub->_key, _z_bytes_null(), _z_encoding_null(), Z_SAMPLE_KIND_DELETE,
                    pub->_congestion_control, pub->_priority, opt.is_express, opt.timestamp, _z_bytes_null());
}

z_owned_keyexpr_t z_publisher_keyexpr(z_loaned_publisher_t *publisher) {
    z_owned_keyexpr_t ret;
    ret._val = _z_keyexpr_duplicate(publisher->_key);
    return ret;
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
             z_owned_closure_reply_t *callback, z_get_options_t *options) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->_val.context;
    callback->_val.context = NULL;

    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);

    z_get_options_t opt;
    z_get_options_default(&opt);
    if (options != NULL) {
        opt.consolidation = options->consolidation;
        opt.target = options->target;
        opt.encoding = options->encoding;
        opt.payload = z_bytes_move(options->payload);
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
    _z_value_t value = {.payload = _z_bytes_from_owned_bytes(opt.payload),
                        .encoding = _z_encoding_from_owned(opt.encoding)};

    ret = _z_query(_Z_RC_IN_VAL(zs), keyexpr_aliased, parameters, opt.target, opt.consolidation.mode, value,
                   callback->_val.call, callback->_val.drop, ctx, opt.timeout_ms,
                   _z_bytes_from_owned_bytes(opt.attachment), opt.congestion_control, opt.priority, opt.is_express);
    if (opt.payload != NULL) {
        z_bytes_drop(opt.payload);
    }
    // Clean-up
    z_encoding_drop(opt.encoding);
    z_bytes_drop(opt.attachment);
    z_closure_reply_null(callback);
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
                           const z_loaned_keyexpr_t *keyexpr, z_owned_closure_query_t *callback,
                           const z_queryable_options_t *options) {
    void *ctx = callback->_val.context;
    callback->_val.context = NULL;

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

    queryable->_val = _z_declare_queryable(zs, key, opt.complete, callback->_val.call, callback->_val.drop, ctx);

    z_closure_query_null(callback);
    return _Z_RES_OK;
}

int8_t z_undeclare_queryable(z_owned_queryable_t *queryable) {
    return _z_undeclare_and_clear_queryable(&queryable->_val);
}

void z_query_reply_options_default(z_query_reply_options_t *options) {
    options->encoding = NULL;
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
    options->timestamp = NULL;
    options->is_express = false;
    options->attachment = NULL;
}

int8_t z_query_reply(const z_loaned_query_t *query, const z_loaned_keyexpr_t *keyexpr, z_owned_bytes_t *payload,
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
    _z_value_t value = {.payload = _z_bytes_from_owned_bytes(payload),
                        .encoding = _z_encoding_from_owned(opts.encoding)};

    int8_t ret =
        _z_send_reply(_Z_RC_IN_VAL(query), &sess_rc, keyexpr_aliased, value, Z_SAMPLE_KIND_PUT, opts.congestion_control,
                      opts.priority, opts.is_express, opts.timestamp, _z_bytes_from_owned_bytes(opts.attachment));
    if (payload != NULL) {
        z_bytes_drop(payload);
    }
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
                               _z_bytes_from_owned_bytes(opts.attachment));
    // Clean-up
    _z_session_rc_drop(&sess_rc);
    z_bytes_drop(opts.attachment);
    return ret;
}

void z_query_reply_err_options_default(z_query_reply_err_options_t *options) { options->encoding = NULL; }

int8_t z_query_reply_err(const z_loaned_query_t *query, z_owned_bytes_t *payload,
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
    _z_value_t value = {.payload = _z_bytes_from_owned_bytes(payload),
                        .encoding = _z_encoding_from_owned(opts.encoding)};

    int8_t ret = _z_send_reply_err(_Z_RC_IN_VAL(query), &sess_rc, value);
    if (payload != NULL) {
        z_bytes_drop(payload);
    }
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
    z_keyexpr_null(key);
    char *name_copy = _z_str_n_clone(name, *len);
    if (name_copy == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_canonize(name_copy, len), z_free(name_copy));
    name_copy[*len] = '\0';
    key->_val = _z_rname(name_copy);
    _z_keyexpr_set_owns_suffix(&key->_val, true);
    return _Z_RES_OK;
}

int8_t z_keyexpr_from_str(z_owned_keyexpr_t *key, const char *name) {
    return z_keyexpr_from_substr(key, name, strlen(name));
}

int8_t z_keyexpr_from_substr(z_owned_keyexpr_t *key, const char *name, size_t len) {
    z_keyexpr_null(key);
    char *name_copy = _z_str_n_clone(name, len);
    if (name_copy == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    key->_val = _z_rname(name_copy);
    _z_keyexpr_set_owns_suffix(&key->_val, true);
    return _Z_RES_OK;
}

int8_t z_declare_keyexpr(z_owned_keyexpr_t *key, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr) {
    _z_keyexpr_t k = _z_keyexpr_alias_from_user_defined(*keyexpr, false);
    uint16_t id = _z_declare_resource(_Z_RC_IN_VAL(zs), k);
    key->_val = _z_rid_with_suffix(id, NULL);
    if (keyexpr->_suffix) {
        key->_val._suffix = _z_str_clone(keyexpr->_suffix);
    }
    // we still need to store the original suffix, for user needs
    // (for example to compare keys or perform other operations on their string representation).
    // Generally this breaks internal keyexpr representation, but is ok for user-defined keyexprs
    // since they consist of 2 disjoint sets: either they have a non-nul suffix or non-trivial id/mapping.
    // The resulting keyexpr can be separated later into valid internal keys using _z_keyexpr_alias_from_user_defined.
    if (key->_val._suffix == NULL && keyexpr->_suffix != NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _z_keyexpr_set_owns_suffix(&key->_val, true);
    return _Z_RES_OK;
}

int8_t z_undeclare_keyexpr(z_owned_keyexpr_t *keyexpr, const z_loaned_session_t *zs) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_resource(_Z_RC_IN_VAL(zs), keyexpr->_val._id);
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

void z_subscriber_options_default(z_subscriber_options_t *options) { options->reliability = Z_RELIABILITY_DEFAULT; }

int8_t z_declare_subscriber(z_owned_subscriber_t *sub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                            z_owned_closure_sample_t *callback, const z_subscriber_options_t *options) {
    void *ctx = callback->_val.context;
    callback->_val.context = NULL;
    char *suffix = NULL;

    _z_keyexpr_t keyexpr_aliased = _z_keyexpr_alias_from_user_defined(*keyexpr, true);
    _z_keyexpr_t key = keyexpr_aliased;

    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)->_tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(_Z_RC_IN_VAL(zs), &keyexpr_aliased);
        if (r == NULL) {
            char *wild = strpbrk(keyexpr_aliased._suffix, "*$");
            _Bool do_keydecl = true;
            _z_keyexpr_t resource_key = keyexpr_aliased;
            if (wild != NULL && wild != resource_key._suffix) {
                wild -= 1;
                size_t len = (size_t)(wild - resource_key._suffix);
                suffix = z_malloc(len + 1);
                if (suffix != NULL) {
                    memcpy(suffix, resource_key._suffix, len);
                    suffix[len] = 0;
                    resource_key._suffix = suffix;
                    _z_keyexpr_set_owns_suffix(&resource_key, false);
                } else {
                    do_keydecl = false;
                }
            }
            if (do_keydecl) {
                uint16_t id = _z_declare_resource(_Z_RC_IN_VAL(zs), resource_key);
                key = _z_rid_with_suffix(id, wild);
            }
            _z_keyexpr_clear(&resource_key);
        }
    }

    _z_subinfo_t subinfo = _z_subinfo_default();
    if (options != NULL) {
        subinfo.reliability = options->reliability;
    }
    _z_subscriber_t int_sub = _z_declare_subscriber(zs, key, subinfo, callback->_val.call, callback->_val.drop, ctx);
    if (suffix != NULL) {
        z_free(suffix);
    }
    z_closure_sample_null(callback);
    sub->_val = int_sub;

    if (!_z_subscriber_check(&sub->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    } else {
        return _Z_RES_OK;
    }
}

int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub) { return _z_undeclare_and_clear_subscriber(&sub->_val); }

int8_t z_subscriber_keyexpr(z_owned_keyexpr_t *keyexpr, z_loaned_subscriber_t *sub) {
    // Init keyexpr
    z_keyexpr_null(keyexpr);
    if ((keyexpr == NULL) || (sub == NULL)) {
        return _Z_ERR_GENERIC;
    }
    uint32_t lookup = sub->_entity_id;
    _z_subscription_rc_list_t *tail = _Z_RC_IN_VAL(&sub->_zn)->_local_subscriptions;
    while (tail != NULL && !z_keyexpr_check(keyexpr)) {
        _z_subscription_rc_t *head = _z_subscription_rc_list_head(tail);
        if (_Z_RC_IN_VAL(head)->_id == lookup) {
            _Z_RETURN_IF_ERR(_z_keyexpr_copy(&keyexpr->_val, &_Z_RC_IN_VAL(head)->_key));
        }
        tail = _z_subscription_rc_list_tail(tail);
    }
    return _Z_RES_OK;
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
