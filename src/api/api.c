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
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/transport/unicast.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"
#include "zenoh-pico/utils/uuid.h"

/********* Data Types Handlers *********/

int8_t z_view_string_wrap(z_view_string_t *str, const char *value) {
    str->_val = _z_string_wrap((char *)value);
    return _Z_RES_OK;
}

const z_loaned_string_t *z_string_array_get(const z_loaned_string_array_t *a, size_t k) {
    return _z_string_vec_get(a, k);
}

size_t z_string_array_len(const z_loaned_string_array_t *a) { return _z_string_vec_len(a); }

_Bool z_string_array_is_empty(const z_loaned_string_array_t *a) { return _z_string_vec_is_empty(a); }

int8_t z_view_keyexpr_from_string(z_view_keyexpr_t *keyexpr, const char *name) {
    keyexpr->_val = _z_rname(name);
    return _Z_RES_OK;
}

int8_t z_view_keyexpr_from_string_unchecked(z_view_keyexpr_t *keyexpr, const char *name) {
    keyexpr->_val = _z_rname(name);
    return _Z_RES_OK;
}

int8_t z_keyexpr_to_string(const z_loaned_keyexpr_t *keyexpr, z_owned_string_t *s) {
    int8_t ret = _Z_RES_OK;
    if (keyexpr->_id == Z_RESOURCE_ID_NONE) {
        s->_val = (_z_string_t *)z_malloc(sizeof(_z_string_t));
        if (s->_val != NULL) {
            s->_val->val = _z_str_clone(keyexpr->_suffix);
            s->_val->len = strlen(keyexpr->_suffix);
        } else {
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    return ret;
}

int8_t zp_keyexpr_resolve(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, z_owned_string_t *str) {
    _z_keyexpr_t ekey = _z_get_expanded_key_from_key(&_Z_RC_IN_VAL(zs), keyexpr);
    *str->_val = _z_string_make((char *)ekey._suffix);  // ekey will be out of scope so
                                                        //  - suffix can be safely casted as non-const
                                                        //  - suffix does not need to be copied
    return _Z_RES_OK;
}

_Bool z_keyexpr_is_initialized(const z_loaned_keyexpr_t *keyexpr) {
    _Bool ret = false;

    if ((keyexpr->_id != Z_RESOURCE_ID_NONE) || (keyexpr->_suffix != NULL)) {
        ret = true;
    }

    return ret;
}

int8_t z_keyexpr_is_canon(const char *start, size_t len) { return _z_keyexpr_is_canon(start, len); }

int8_t zp_keyexpr_is_canon_null_terminated(const char *start) { return _z_keyexpr_is_canon(start, strlen(start)); }

int8_t z_keyexpr_canonize(char *start, size_t *len) { return _z_keyexpr_canonize(start, len); }

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

_Bool z_keyexpr_includes(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    if ((l->_id == Z_RESOURCE_ID_NONE) && (r->_id == Z_RESOURCE_ID_NONE)) {
        return zp_keyexpr_includes_null_terminated(l->_suffix, r->_suffix);
    }
    return false;
}

_Bool zp_keyexpr_includes_null_terminated(const char *l, const char *r) {
    if (l != NULL && r != NULL) {
        return _z_keyexpr_includes(l, strlen(l), r, strlen(r));
    }
    return false;
}

_Bool z_keyexpr_intersects(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    if ((l->_id == Z_RESOURCE_ID_NONE) && (r->_id == Z_RESOURCE_ID_NONE)) {
        return zp_keyexpr_intersect_null_terminated(l->_suffix, r->_suffix);
    }
    return false;
}

_Bool zp_keyexpr_intersect_null_terminated(const char *l, const char *r) {
    if (l != NULL && r != NULL) {
        return _z_keyexpr_intersects(l, strlen(l), r, strlen(r));
    }
    return false;
}

_Bool z_keyexpr_equals(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r) {
    if ((l->_id == Z_RESOURCE_ID_NONE) && (r->_id == Z_RESOURCE_ID_NONE)) {
        return zp_keyexpr_equals_null_terminated(l->_suffix, r->_suffix);
    }
    return false;
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

void z_config_default(z_owned_config_t *config) { config->_val = _z_config_default(); }

const char *zp_config_get(const z_loaned_config_t *config, uint8_t key) { return _z_config_get(config, key); }

int8_t zp_config_insert(z_loaned_config_t *config, uint8_t key, const char *value) {
    return _zp_config_insert(config, key, value);
}

void z_scouting_config_default(z_owned_scouting_config_t *sc) {
    sc->_val = _z_config_empty();

    _zp_config_insert(sc->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY, Z_CONFIG_MULTICAST_LOCATOR_DEFAULT);
    _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY, Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT);
    _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_WHAT_KEY, Z_CONFIG_SCOUTING_WHAT_DEFAULT);
}

int8_t z_scouting_config_from(z_owned_scouting_config_t *sc, const z_loaned_config_t *c) {
    sc->_val = _z_config_empty();

    char *opt;
    opt = _z_config_get(c, Z_CONFIG_MULTICAST_LOCATOR_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY, Z_CONFIG_MULTICAST_LOCATOR_DEFAULT);
    } else {
        _zp_config_insert(sc->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY, opt);
    }

    opt = _z_config_get(c, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY, Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT);
    } else {
        _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY, opt);
    }

    opt = _z_config_get(c, Z_CONFIG_SCOUTING_WHAT_KEY);
    if (opt == NULL) {
        _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_WHAT_KEY, Z_CONFIG_SCOUTING_WHAT_DEFAULT);
    } else {
        _zp_config_insert(sc->_val, Z_CONFIG_SCOUTING_WHAT_KEY, opt);
    }

    return _Z_RES_OK;
}

const char *zp_scouting_config_get(const z_loaned_scouting_config_t *sc, uint8_t key) { return _z_config_get(sc, key); }

int8_t zp_scouting_config_insert(z_loaned_scouting_config_t *sc, uint8_t key, const char *value) {
    return _zp_config_insert(sc, key, value);
}

int8_t zp_encoding_make(z_owned_encoding_t *encoding, z_encoding_id_t id, const char *schema) {
    // Init encoding
    encoding->_val = (_z_encoding_t *)z_malloc(sizeof(_z_encoding_t));
    if (encoding->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _z_encoding_make(encoding->_val, id, schema);
}

z_owned_encoding_t *z_encoding_move(z_owned_encoding_t *encoding) { return encoding; }

int8_t z_encoding_null(z_owned_encoding_t *encoding) { return zp_encoding_make(encoding, Z_ENCODING_ID_DEFAULT, NULL); }

_Bool z_encoding_check(const z_owned_encoding_t *encoding) { return _z_encoding_check(encoding->_val); }

void z_encoding_drop(z_owned_encoding_t *encoding) {
    if (encoding == NULL) {
        return;
    }
    if (!_z_slice_is_empty(&encoding->_val->schema)) {
        _z_slice_clear(&encoding->_val->schema);
    }
    z_free(encoding->_val);
}

const z_loaned_encoding_t *z_encoding_loan(const z_owned_encoding_t *encoding) { return encoding->_val; }

z_loaned_encoding_t *z_encoding_loan_mut(z_owned_encoding_t *encoding) { return encoding->_val; }

// Convert a user owned encoding to an internal encoding, return default encoding if value invalid
static _z_encoding_t _z_encoding_from_owned(const z_owned_encoding_t *encoding) {
    if (encoding == NULL) {
        return _z_encoding_null();
    }
    if (encoding->_val == NULL) {
        return _z_encoding_null();
    }
    return *encoding->_val;
}

const z_loaned_bytes_t *z_query_payload(const z_loaned_query_t *query) { return &query->in->val._value.payload; }
const z_loaned_encoding_t *z_query_encoding(const z_loaned_query_t *query) { return &query->in->val._value.encoding; }

const uint8_t *z_slice_data(const z_loaned_slice_t *slice) { return slice->start; }

size_t z_slice_len(const z_loaned_slice_t *slice) { return slice->len; }

int8_t z_bytes_deserialize_into_int8(const z_loaned_bytes_t *bytes, int8_t *dst) {
    *dst = (int8_t)_z_bytes_to_uint8(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_int16(const z_loaned_bytes_t *bytes, int16_t *dst) {
    *dst = (int16_t)_z_bytes_to_uint16(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_int32(const z_loaned_bytes_t *bytes, int32_t *dst) {
    *dst = (int32_t)_z_bytes_to_uint32(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_int64(const z_loaned_bytes_t *bytes, int64_t *dst) {
    *dst = (int64_t)_z_bytes_to_uint64(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_uint8(const z_loaned_bytes_t *bytes, uint8_t *dst) {
    *dst = _z_bytes_to_uint8(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_uint16(const z_loaned_bytes_t *bytes, uint16_t *dst) {
    *dst = _z_bytes_to_uint16(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_uint32(const z_loaned_bytes_t *bytes, uint32_t *dst) {
    *dst = _z_bytes_to_uint32(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_uint64(const z_loaned_bytes_t *bytes, uint64_t *dst) {
    *dst = _z_bytes_to_uint64(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_float(const z_loaned_bytes_t *bytes, float *dst) {
    *dst = _z_bytes_to_float(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_double(const z_loaned_bytes_t *bytes, double *dst) {
    *dst = _z_bytes_to_double(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_slice(const z_loaned_bytes_t *bytes, z_owned_slice_t *dst) {
    // Init owned slice
    z_slice_null(dst);
    dst->_val = (_z_slice_t *)z_malloc(sizeof(_z_slice_t));
    if (dst->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Convert bytes to slice
    *dst->_val = _z_bytes_to_slice(bytes);
    return _Z_RES_OK;
}

int8_t z_bytes_deserialize_into_string(const z_loaned_bytes_t *bytes, z_owned_string_t *s) {
    // Init owned string
    z_string_null(s);
    s->_val = (_z_string_t *)z_malloc(sizeof(_z_string_t));
    if (s->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Convert bytes to string
    *s->_val = _z_string_from_bytes(&bytes->_slice);
    return _Z_RES_OK;
}

int8_t zp_bytes_deserialize_into_pair(const z_loaned_bytes_t *bytes, z_owned_bytes_t *first, z_owned_bytes_t *second,
                                      size_t *curr_idx) {
    // Check bound size
    if (*curr_idx >= bytes->_slice.len) {
        return _Z_ERR_GENERIC;
    }
    // Init pair of owned bytes
    z_bytes_null(first);
    z_bytes_null(second);
    first->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    second->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if ((first->_val == NULL) || (second->_val == NULL)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Extract first item size
    size_t first_len = 0;
    // FIXME: size endianness, Issue #420
    memcpy(&first_len, &bytes->_slice.start[*curr_idx], sizeof(uint32_t));
    *curr_idx += sizeof(uint32_t);
    // Allocate first item bytes
    *first->_val = _z_bytes_make(first_len);
    if (!_z_bytes_check(*first->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Extract first item data
    memcpy((uint8_t *)first->_val->_slice.start, &bytes->_slice.start[*curr_idx], first_len);
    *curr_idx += first_len;

    // Extract second item size
    size_t second_len = 0;
    memcpy(&second_len, &bytes->_slice.start[*curr_idx], sizeof(uint32_t));
    *curr_idx += sizeof(uint32_t);
    // Allocate second item bytes
    *second->_val = _z_bytes_make(second_len);
    if (!_z_bytes_check(*second->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Extract second item data
    memcpy((uint8_t *)second->_val->_slice.start, &bytes->_slice.start[*curr_idx], second_len);
    *curr_idx += second_len;
    return _Z_RES_OK;
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
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode data
    *bytes->_val = _z_bytes_from_uint8(val);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_uint16(z_owned_bytes_t *bytes, uint16_t val) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode data
    *bytes->_val = _z_bytes_from_uint16(val);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_uint32(z_owned_bytes_t *bytes, uint32_t val) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode data
    *bytes->_val = _z_bytes_from_uint32(val);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_uint64(z_owned_bytes_t *bytes, uint64_t val) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode data
    *bytes->_val = _z_bytes_from_uint64(val);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_float(z_owned_bytes_t *bytes, float val) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode data
    *bytes->_val = _z_bytes_from_float(val);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_double(z_owned_bytes_t *bytes, double val) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode data
    *bytes->_val = _z_bytes_from_double(val);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_slice(z_owned_bytes_t *bytes, const uint8_t *data, size_t len) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode data
    bytes->_val->_slice = _z_slice_wrap((uint8_t *)data, len);
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_slice_copy(z_owned_bytes_t *bytes, const uint8_t *data, size_t len) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Allocate bytes
    *bytes->_val = _z_bytes_make(len);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Copy data
    memcpy((uint8_t *)bytes->_val->_slice.start, data, len);
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_string(z_owned_bytes_t *bytes, const char *s) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Encode string without null terminator
    size_t len = strlen(s);
    bytes->_val->_slice = _z_slice_wrap((uint8_t *)s, len);
    return _Z_RES_OK;
}

int8_t z_bytes_serialize_from_string_copy(z_owned_bytes_t *bytes, const char *s) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Allocate bytes
    size_t len = strlen(s);
    *bytes->_val = _z_bytes_make(len);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Copy string without null terminator
    memcpy((uint8_t *)bytes->_val->_slice.start, s, len);
    return _Z_RES_OK;
}

int8_t zp_bytes_serialize_from_iter(z_owned_bytes_t *bytes,
                                    _Bool (*iterator_body)(z_owned_bytes_t *data, void *context, size_t *curr_idx),
                                    void *context, size_t total_len) {
    // Init owned bytes
    z_bytes_null(bytes);
    bytes->_val = (_z_bytes_t *)z_malloc(sizeof(_z_bytes_t));
    if (bytes->_val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Allocate bytes
    *bytes->_val = _z_bytes_make(total_len);
    if (!_z_bytes_check(*bytes->_val)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    size_t curr_idx = 0;
    while (iterator_body(bytes, context, &curr_idx))
        ;
    return _Z_RES_OK;
}

int8_t zp_bytes_serialize_from_pair(z_owned_bytes_t *bytes, z_owned_bytes_t *first, z_owned_bytes_t *second,
                                    size_t *curr_idx) {
    // Calculate pair size
    size_t first_len = z_slice_len(&first->_val->_slice);
    size_t second_len = z_slice_len(&second->_val->_slice);
    size_t len = 2 * sizeof(uint32_t) + first_len + second_len;
    // Copy data
    // FIXME: size endianness, Issue #420
    memcpy((uint8_t *)&bytes->_val->_slice.start[*curr_idx], &first_len, sizeof(uint32_t));
    *curr_idx += sizeof(uint32_t);
    memcpy((uint8_t *)&bytes->_val->_slice.start[*curr_idx], z_slice_data(&first->_val->_slice), first_len);
    *curr_idx += first_len;
    memcpy((uint8_t *)&bytes->_val->_slice.start[*curr_idx], &second_len, sizeof(uint32_t));
    *curr_idx += sizeof(uint32_t);
    memcpy((uint8_t *)&bytes->_val->_slice.start[*curr_idx], z_slice_data(&second->_val->_slice), second_len);
    *curr_idx += second_len;
    // Clean up
    z_bytes_drop(first);
    z_bytes_drop(second);
    return _Z_RES_OK;
}

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
    parameters->_val.val = query->in->val._parameters;
    parameters->_val.len = strlen(query->in->val._parameters);
}

const z_loaned_bytes_t *z_query_attachment(const z_loaned_query_t *query) { return &query->in->val.attachment; }

const z_loaned_keyexpr_t *z_query_keyexpr(const z_loaned_query_t *query) { return &query->in->val._key; }

void z_closure_sample_call(const z_owned_closure_sample_t *closure, const z_loaned_sample_t *sample) {
    if (closure->call != NULL) {
        (closure->call)(sample, closure->context);
    }
}

void z_closure_owned_sample_call(const z_owned_closure_owned_sample_t *closure, z_owned_sample_t *sample) {
    if (closure->call != NULL) {
        (closure->call)(sample, closure->context);
    }
}

void z_closure_query_call(const z_owned_closure_query_t *closure, const z_loaned_query_t *query) {
    if (closure->call != NULL) {
        (closure->call)(query, closure->context);
    }
}

void z_closure_owned_query_call(const z_owned_closure_owned_query_t *closure, z_owned_query_t *query) {
    if (closure->call != NULL) {
        (closure->call)(query, closure->context);
    }
}

void z_closure_reply_call(const z_owned_closure_reply_t *closure, const z_loaned_reply_t *reply) {
    if (closure->call != NULL) {
        (closure->call)(reply, closure->context);
    }
}

void z_closure_owned_reply_call(const z_owned_closure_owned_reply_t *closure, z_owned_reply_t *reply) {
    if (closure->call != NULL) {
        (closure->call)(reply, closure->context);
    }
}

void z_closure_hello_call(const z_owned_closure_hello_t *closure, const z_loaned_hello_t *hello) {
    if (closure->call != NULL) {
        (closure->call)(hello, closure->context);
    }
}

void z_closure_zid_call(const z_owned_closure_zid_t *closure, const z_id_t *id) {
    if (closure->call != NULL) {
        (closure->call)(id, closure->context);
    }
}

#define OWNED_FUNCTIONS_PTR(type, name, f_copy, f_free)                                             \
    _Bool z_##name##_check(const z_owned_##name##_t *obj) { return obj->_val != NULL; }             \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *obj) { return obj->_val; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *obj) { return obj->_val; }         \
    void z_##name##_null(z_owned_##name##_t *obj) { obj->_val = NULL; }                             \
    z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *obj) { return obj; }                    \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {              \
        int8_t ret = _Z_RES_OK;                                                                     \
        obj->_val = (type *)z_malloc(sizeof(type));                                                 \
        if (obj->_val != NULL) {                                                                    \
            f_copy(obj->_val, src);                                                                 \
        } else {                                                                                    \
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                      \
        }                                                                                           \
        return ret;                                                                                 \
    }                                                                                               \
    void z_##name##_drop(z_owned_##name##_t *obj) {                                                 \
        if ((obj != NULL) && (obj->_val != NULL)) {                                                 \
            f_free(&obj->_val);                                                                     \
        }                                                                                           \
    }

#define OWNED_FUNCTIONS_RC(name)                                                                    \
    _Bool z_##name##_check(const z_owned_##name##_t *val) { return val->_rc.in != NULL; }           \
    const z_loaned_##name##_t *z_##name##_loan(const z_owned_##name##_t *val) { return &val->_rc; } \
    z_loaned_##name##_t *z_##name##_loan_mut(z_owned_##name##_t *val) { return &val->_rc; }         \
    void z_##name##_null(z_owned_##name##_t *val) { val->_rc.in = NULL; }                           \
    z_owned_##name##_t *z_##name##_move(z_owned_##name##_t *val) { return val; }                    \
    int8_t z_##name##_clone(z_owned_##name##_t *obj, const z_loaned_##name##_t *src) {              \
        int8_t ret = _Z_RES_OK;                                                                     \
        obj->_rc = _z_##name##_rc_clone((z_loaned_##name##_t *)src);                                \
        if (obj->_rc.in == NULL) {                                                                  \
            ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;                                                      \
        }                                                                                           \
        return ret;                                                                                 \
    }                                                                                               \
    void z_##name##_drop(z_owned_##name##_t *val) {                                                 \
        if (val->_rc.in != NULL) {                                                                  \
            if (_z_##name##_rc_drop(&val->_rc)) {                                                   \
                val->_rc.in = NULL;                                                                 \
            }                                                                                       \
        }                                                                                           \
    }

#define VIEW_FUNCTIONS_PTR(type, name)                                                                   \
    const z_loaned_##name##_t *z_view_##name##_loan(const z_view_##name##_t *obj) { return &obj->_val; } \
    z_loaned_##name##_t *z_view_##name##_loan_mut(z_view_##name##_t *obj) { return &obj->_val; }

static inline void _z_owner_noop_copy(void *dst, const void *src) {
    (void)(dst);
    (void)(src);
}

OWNED_FUNCTIONS_PTR(_z_config_t, config, _z_owner_noop_copy, _z_config_free)
OWNED_FUNCTIONS_PTR(_z_scouting_config_t, scouting_config, _z_owner_noop_copy, _z_scouting_config_free)
OWNED_FUNCTIONS_PTR(_z_string_t, string, _z_string_copy, _z_string_free)
OWNED_FUNCTIONS_PTR(_z_value_t, reply_err, _z_value_copy, _z_value_free)

OWNED_FUNCTIONS_PTR(_z_keyexpr_t, keyexpr, _z_keyexpr_copy, _z_keyexpr_free)
VIEW_FUNCTIONS_PTR(_z_keyexpr_t, keyexpr)
VIEW_FUNCTIONS_PTR(_z_string_t, string)

OWNED_FUNCTIONS_PTR(_z_hello_t, hello, _z_owner_noop_copy, _z_hello_free)
OWNED_FUNCTIONS_PTR(_z_string_vec_t, string_array, _z_owner_noop_copy, _z_string_vec_free)
VIEW_FUNCTIONS_PTR(_z_string_vec_t, string_array)
OWNED_FUNCTIONS_PTR(_z_slice_t, slice, _z_slice_copy, _z_slice_free)
OWNED_FUNCTIONS_PTR(_z_bytes_t, bytes, _z_bytes_copy, _z_bytes_free)

static _z_bytes_t _z_bytes_from_owned_bytes(z_owned_bytes_t *bytes) {
    _z_bytes_t b = _z_bytes_null();
    if ((bytes != NULL) && (bytes->_val != NULL)) {
        b._slice = _z_slice_wrap(bytes->_val->_slice.start, bytes->_val->_slice.len);
    }
    return b;
}

OWNED_FUNCTIONS_RC(sample)
OWNED_FUNCTIONS_RC(session)

#define OWNED_FUNCTIONS_CLOSURE(ownedtype, name, f_call, f_drop)                   \
    _Bool z_##name##_check(const ownedtype *val) { return val->call != NULL; }     \
    ownedtype *z_##name##_move(ownedtype *val) { return val; }                     \
    void z_##name##_drop(ownedtype *val) {                                         \
        if (val->drop != NULL) {                                                   \
            (val->drop)(val->context);                                             \
            val->drop = NULL;                                                      \
        }                                                                          \
        val->call = NULL;                                                          \
        val->context = NULL;                                                       \
    }                                                                              \
    void z_##name##_null(ownedtype *val) {                                         \
        val->call = NULL;                                                          \
        val->drop = NULL;                                                          \
        val->context = NULL;                                                       \
    }                                                                              \
    int8_t z_##name(ownedtype *closure, f_call call, f_drop drop, void *context) { \
        closure->call = call;                                                      \
        closure->drop = drop;                                                      \
        closure->context = context;                                                \
                                                                                   \
        return _Z_RES_OK;                                                          \
    }

OWNED_FUNCTIONS_CLOSURE(z_owned_closure_sample_t, closure_sample, _z_data_handler_t, z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_owned_sample_t, closure_owned_sample, z_owned_sample_handler_t,
                        z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_query_t, closure_query, _z_queryable_handler_t, z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_owned_query_t, closure_owned_query, z_owned_query_handler_t,
                        z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_reply_t, closure_reply, _z_reply_handler_t, z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_owned_reply_t, closure_owned_reply, z_owned_reply_handler_t,
                        z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_hello_t, closure_hello, z_loaned_hello_handler_t, z_dropper_handler_t)
OWNED_FUNCTIONS_CLOSURE(z_owned_closure_zid_t, closure_zid, z_id_handler_t, z_dropper_handler_t)

/************* Primitives **************/
typedef struct __z_hello_handler_wrapper_t {
    z_loaned_hello_handler_t user_call;
    void *ctx;
} __z_hello_handler_wrapper_t;

void __z_hello_handler(_z_hello_t *hello, __z_hello_handler_wrapper_t *wrapped_ctx) {
    wrapped_ctx->user_call(hello, wrapped_ctx->ctx);
}

int8_t z_scout(z_owned_scouting_config_t *config, z_owned_closure_hello_t *callback) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->context;
    callback->context = NULL;

    // TODO[API-NET]: When API and NET are a single layer, there is no wrap the user callback and args
    //                to enclose the z_reply_t into a z_owned_reply_t.
    __z_hello_handler_wrapper_t *wrapped_ctx =
        (__z_hello_handler_wrapper_t *)z_malloc(sizeof(__z_hello_handler_wrapper_t));
    if (wrapped_ctx != NULL) {
        wrapped_ctx->user_call = callback->call;
        wrapped_ctx->ctx = ctx;

        char *opt_as_str = _z_config_get(config->_val, Z_CONFIG_SCOUTING_WHAT_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = (char *)Z_CONFIG_SCOUTING_WHAT_DEFAULT;
        }
        z_what_t what = strtol(opt_as_str, NULL, 10);

        opt_as_str = _z_config_get(config->_val, Z_CONFIG_MULTICAST_LOCATOR_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = (char *)Z_CONFIG_MULTICAST_LOCATOR_DEFAULT;
        }
        char *mcast_locator = opt_as_str;

        opt_as_str = _z_config_get(config->_val, Z_CONFIG_SCOUTING_TIMEOUT_KEY);
        if (opt_as_str == NULL) {
            opt_as_str = (char *)Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
        }
        uint32_t timeout = strtoul(opt_as_str, NULL, 10);

        _z_id_t zid = _z_id_empty();
        char *zid_str = _z_config_get(config->_val, Z_CONFIG_SESSION_ZID_KEY);
        if (zid_str != NULL) {
            _z_uuid_to_bytes(zid.id, zid_str);
        }

        _z_scout(what, zid, mcast_locator, timeout, __z_hello_handler, wrapped_ctx, callback->drop, ctx);

        z_free(wrapped_ctx);
        z_scouting_config_drop(config);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return ret;
}

int8_t z_open(z_owned_session_t *zs, z_owned_config_t *config) {
    z_session_null(zs);

    // Create rc
    _z_session_rc_t zsrc = _z_session_rc_new();
    if (zsrc.in == NULL) {
        z_config_drop(config);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Open session
    int8_t ret = _z_open(&zsrc.in->val, config->_val);
    if (ret != _Z_RES_OK) {
        _z_session_rc_drop(&zsrc);
        z_config_drop(config);
        return ret;
    }
    // Store rc in session
    zs->_rc = zsrc;
    z_config_drop(config);
    return _Z_RES_OK;
}

int8_t z_close(z_owned_session_t *zs) {
    if (zs == NULL || !z_session_check(zs)) {
        return _Z_RES_OK;
    }
    _z_close(&_Z_OWNED_RC_IN_VAL(zs));
    z_session_drop(zs);
    return _Z_RES_OK;
}

int8_t z_info_peers_zid(const z_loaned_session_t *zs, z_owned_closure_zid_t *callback) {
    // Call transport function
    switch (_Z_RC_IN_VAL(zs)._tp._type) {
        case _Z_TRANSPORT_MULTICAST_TYPE:
        case _Z_TRANSPORT_RAWETH_TYPE:
            _zp_multicast_fetch_zid(&(_Z_RC_IN_VAL(zs)._tp), callback);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->context;
    callback->context = NULL;
    // Drop if needed
    if (callback->drop != NULL) {
        callback->drop(ctx);
    }
    return 0;
}

int8_t z_info_routers_zid(const z_loaned_session_t *zs, z_owned_closure_zid_t *callback) {
    // Call transport function
    switch (_Z_RC_IN_VAL(zs)._tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _zp_unicast_fetch_zid(&(_Z_RC_IN_VAL(zs)._tp), callback);
            break;
        default:
            break;
    }
    // Note and clear context
    void *ctx = callback->context;
    callback->context = NULL;
    // Drop if needed
    if (callback->drop != NULL) {
        callback->drop(ctx);
    }
    return 0;
}

z_id_t z_info_zid(const z_loaned_session_t *zs) { return _Z_RC_IN_VAL(zs)._local_zid; }

const z_loaned_keyexpr_t *z_sample_keyexpr(const z_loaned_sample_t *sample) { return &_Z_RC_IN_VAL(sample).keyexpr; }
z_sample_kind_t z_sample_kind(const z_loaned_sample_t *sample) { return _Z_RC_IN_VAL(sample).kind; }
const z_loaned_bytes_t *z_sample_payload(const z_loaned_sample_t *sample) { return &_Z_RC_IN_VAL(sample).payload; }
z_timestamp_t z_sample_timestamp(const z_loaned_sample_t *sample) { return _Z_RC_IN_VAL(sample).timestamp; }
const z_loaned_encoding_t *z_sample_encoding(const z_loaned_sample_t *sample) { return &_Z_RC_IN_VAL(sample).encoding; }
z_qos_t z_sample_qos(const z_loaned_sample_t *sample) { return _Z_RC_IN_VAL(sample).qos; }
const z_loaned_bytes_t *z_sample_attachment(const z_loaned_sample_t *sample) {
    return &_Z_RC_IN_VAL(sample).attachment;
}

const char *z_string_data(const z_loaned_string_t *str) { return str->val; }
size_t z_string_len(const z_loaned_string_t *str) { return str->len; }

#if Z_FEATURE_PUBLICATION == 1
int8_t _z_publisher_drop(_z_publisher_t **pub) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_publisher(*pub);
    _z_publisher_free(pub);

    return ret;
}

OWNED_FUNCTIONS_PTR(_z_publisher_t, publisher, _z_owner_noop_copy, _z_publisher_drop)

void z_put_options_default(z_put_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
    options->encoding = NULL;
    options->attachment = NULL;
}

void z_delete_options_default(z_delete_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
}

int8_t z_put(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const uint8_t *payload,
             z_zint_t payload_len, const z_put_options_t *options) {
    int8_t ret = 0;

    z_put_options_t opt;
    z_put_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.encoding = options->encoding;
        opt.priority = options->priority;
        opt.attachment = options->attachment;
    }

    ret = _z_write(&_Z_RC_IN_VAL(zs), *keyexpr, (const uint8_t *)payload, payload_len,
                   _z_encoding_from_owned(opt.encoding), Z_SAMPLE_KIND_PUT, opt.congestion_control, opt.priority,
                   _z_bytes_from_owned_bytes(opt.attachment));

    // Trigger local subscriptions
    _z_trigger_local_subscriptions(&_Z_RC_IN_VAL(zs), *keyexpr, payload, payload_len,
                                   _z_n_qos_make(0, opt.congestion_control == Z_CONGESTION_CONTROL_BLOCK, opt.priority),
                                   _z_bytes_from_owned_bytes(opt.attachment));
    // Clean-up
    z_encoding_drop(opt.encoding);
    z_bytes_drop(opt.attachment);
    return ret;
}

int8_t z_delete(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const z_delete_options_t *options) {
    int8_t ret = 0;

    z_delete_options_t opt;
    z_delete_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
    }
    ret = _z_write(&_Z_RC_IN_VAL(zs), *keyexpr, NULL, 0, _z_encoding_null(), Z_SAMPLE_KIND_DELETE,
                   opt.congestion_control, opt.priority, _z_bytes_null());

    return ret;
}

void z_publisher_options_default(z_publisher_options_t *options) {
    options->congestion_control = Z_CONGESTION_CONTROL_DEFAULT;
    options->priority = Z_PRIORITY_DEFAULT;
}

int8_t z_declare_publisher(z_owned_publisher_t *pub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                           const z_publisher_options_t *options) {
    _z_keyexpr_t key = *keyexpr;

    pub->_val = NULL;
    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&_Z_RC_IN_VAL(zs), keyexpr);
        if (r == NULL) {
            uint16_t id = _z_declare_resource(&_Z_RC_IN_VAL(zs), *keyexpr);
            key = _z_rid_with_suffix(id, NULL);
        }
    }
    // Set options
    z_publisher_options_t opt;
    z_publisher_options_default(&opt);
    if (options != NULL) {
        opt.congestion_control = options->congestion_control;
        opt.priority = options->priority;
    }
    // Set publisher
    _z_publisher_t *int_pub = _z_declare_publisher(zs, key, opt.congestion_control, opt.priority);
    if (int_pub == NULL) {
        if (key._id != Z_RESOURCE_ID_NONE) {
            _z_undeclare_resource(&_Z_RC_IN_VAL(zs), key._id);
        }
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Create write filter
    int8_t res = _z_write_filter_create(int_pub);
    if (res != _Z_RES_OK) {
        if (key._id != Z_RESOURCE_ID_NONE) {
            _z_undeclare_resource(&_Z_RC_IN_VAL(zs), key._id);
        }
        return res;
    }
    pub->_val = int_pub;
    return _Z_RES_OK;
}

int8_t z_undeclare_publisher(z_owned_publisher_t *pub) { return _z_publisher_drop(&pub->_val); }

void z_publisher_put_options_default(z_publisher_put_options_t *options) {
    options->encoding = NULL;
    options->attachment = NULL;
}

void z_publisher_delete_options_default(z_publisher_delete_options_t *options) { options->__dummy = 0; }

int8_t z_publisher_put(const z_loaned_publisher_t *pub, const uint8_t *payload, size_t len,
                       const z_publisher_put_options_t *options) {
    int8_t ret = 0;
    // Build options
    z_publisher_put_options_t opt;
    z_publisher_put_options_default(&opt);
    if (options != NULL) {
        opt.encoding = options->encoding;
        opt.attachment = options->attachment;
    }
    // Check if write filter is active before writing
    if (!_z_write_filter_active(pub)) {
        // Write value
        ret = _z_write(&pub->_zn.in->val, pub->_key, payload, len, _z_encoding_from_owned(opt.encoding),
                       Z_SAMPLE_KIND_PUT, pub->_congestion_control, pub->_priority,
                       _z_bytes_from_owned_bytes(opt.attachment));
    }
    // Trigger local subscriptions
    _z_trigger_local_subscriptions(&pub->_zn.in->val, pub->_key, payload, len, _Z_N_QOS_DEFAULT,
                                   _z_bytes_from_owned_bytes(opt.attachment));
    // Clean-up
    z_encoding_drop(opt.encoding);
    z_bytes_drop(opt.attachment);
    return ret;
}

int8_t z_publisher_delete(const z_loaned_publisher_t *pub, const z_publisher_delete_options_t *options) {
    (void)(options);
    return _z_write(&pub->_zn.in->val, pub->_key, NULL, 0, _z_encoding_null(), Z_SAMPLE_KIND_DELETE,
                    pub->_congestion_control, pub->_priority, _z_bytes_null());
}

z_owned_keyexpr_t z_publisher_keyexpr(z_loaned_publisher_t *publisher) {
    z_owned_keyexpr_t ret = {._val = z_malloc(sizeof(_z_keyexpr_t))};
    if (ret._val != NULL && publisher != NULL) {
        *ret._val = _z_keyexpr_duplicate(publisher->_key);
    }
    return ret;
}
#endif

#if Z_FEATURE_QUERY == 1
OWNED_FUNCTIONS_RC(reply)

void z_get_options_default(z_get_options_t *options) {
    options->target = z_query_target_default();
    options->consolidation = z_query_consolidation_default();
    options->encoding = NULL;
    options->payload = NULL;
    options->attachment = NULL;
    options->timeout_ms = Z_GET_TIMEOUT_DEFAULT;
}

int8_t z_get(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const char *parameters,
             z_owned_closure_reply_t *callback, z_get_options_t *options) {
    int8_t ret = _Z_RES_OK;

    void *ctx = callback->context;
    callback->context = NULL;

    z_get_options_t opt;
    z_get_options_default(&opt);
    if (options != NULL) {
        opt.consolidation = options->consolidation;
        opt.target = options->target;
        opt.encoding = options->encoding;
        opt.payload = z_bytes_move(options->payload);
        opt.attachment = options->attachment;
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

    ret = _z_query(&_Z_RC_IN_VAL(zs), *keyexpr, parameters, opt.target, opt.consolidation.mode, value, callback->call,
                   callback->drop, ctx, opt.timeout_ms, _z_bytes_from_owned_bytes(opt.attachment));
    if (opt.payload != NULL) {
        z_bytes_drop(opt.payload);
    }
    // Clean-up
    z_encoding_drop(opt.encoding);
    z_bytes_drop(opt.attachment);
    return ret;
}

_Bool z_reply_is_ok(const z_loaned_reply_t *reply) {
    _ZP_UNUSED(reply);
    // For the moment always return TRUE.
    // The support for reply errors will come in the next release.
    return true;
}

const z_loaned_sample_t *z_reply_ok(const z_loaned_reply_t *reply) { return &reply->in->val.data.sample; }

const z_loaned_reply_err_t *z_reply_err(const z_loaned_reply_t *reply) {
    _ZP_UNUSED(reply);
    return NULL;
}
#endif

#if Z_FEATURE_QUERYABLE == 1
int8_t _z_queryable_drop(_z_queryable_t **queryable) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_queryable(*queryable);
    _z_queryable_free(queryable);
    return ret;
}

OWNED_FUNCTIONS_RC(query)
OWNED_FUNCTIONS_PTR(_z_queryable_t, queryable, _z_owner_noop_copy, _z_queryable_drop)

void z_queryable_options_default(z_queryable_options_t *options) { options->complete = _Z_QUERYABLE_COMPLETE_DEFAULT; }

int8_t z_declare_queryable(z_owned_queryable_t *queryable, const z_loaned_session_t *zs,
                           const z_loaned_keyexpr_t *keyexpr, z_owned_closure_query_t *callback,
                           const z_queryable_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;

    _z_keyexpr_t key = *keyexpr;

    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&_Z_RC_IN_VAL(zs), keyexpr);
        if (r == NULL) {
            uint16_t id = _z_declare_resource(&_Z_RC_IN_VAL(zs), *keyexpr);
            key = _z_rid_with_suffix(id, NULL);
        }
    }

    z_queryable_options_t opt;
    z_queryable_options_default(&opt);
    if (options != NULL) {
        opt.complete = options->complete;
    }

    queryable->_val = _z_declare_queryable(zs, key, opt.complete, callback->call, callback->drop, ctx);

    return _Z_RES_OK;
}

int8_t z_undeclare_queryable(z_owned_queryable_t *queryable) { return _z_queryable_drop(&queryable->_val); }

void z_query_reply_options_default(z_query_reply_options_t *options) {
    options->encoding = NULL;
    options->attachment = NULL;
}

int8_t z_query_reply(const z_loaned_query_t *query, const z_loaned_keyexpr_t *keyexpr, z_owned_bytes_t *payload,
                     const z_query_reply_options_t *options) {
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
        _z_send_reply(&query->in->val, *keyexpr, value, Z_SAMPLE_KIND_PUT, _z_bytes_from_owned_bytes(opts.attachment));
    if (payload != NULL) {
        z_bytes_drop(payload);
    }
    // Clean-up
    z_encoding_drop(opts.encoding);
    z_bytes_drop(opts.attachment);
    return ret;
}
#endif

int8_t z_keyexpr_new(z_owned_keyexpr_t *key, const char *name) {
    int8_t ret = _Z_RES_OK;

    key->_val = name ? (_z_keyexpr_t *)z_malloc(sizeof(_z_keyexpr_t)) : NULL;
    if (key->_val != NULL) {
        *key->_val = _z_rid_with_suffix(Z_RESOURCE_ID_NONE, name);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return ret;
}

int8_t z_declare_keyexpr(z_owned_keyexpr_t *key, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr) {
    int8_t ret = _Z_RES_OK;

    key->_val = (_z_keyexpr_t *)z_malloc(sizeof(_z_keyexpr_t));
    if (key->_val != NULL) {
        uint16_t id = _z_declare_resource(&_Z_RC_IN_VAL(zs), *keyexpr);
        *key->_val = _z_rid_with_suffix(id, NULL);
    } else {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }

    return ret;
}

int8_t z_undeclare_keyexpr(const z_loaned_session_t *zs, z_owned_keyexpr_t *keyexpr) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_resource(&_Z_RC_IN_VAL(zs), keyexpr->_val->_id);
    z_keyexpr_drop(keyexpr);

    return ret;
}

#if Z_FEATURE_SUBSCRIPTION == 1
int8_t _z_subscriber_drop(_z_subscriber_t **sub) {
    int8_t ret = _Z_RES_OK;

    ret = _z_undeclare_subscriber(*sub);
    _z_subscriber_free(sub);

    return ret;
}

OWNED_FUNCTIONS_PTR(_z_subscriber_t, subscriber, _z_owner_noop_copy, _z_subscriber_drop)

void z_subscriber_options_default(z_subscriber_options_t *options) { options->reliability = Z_RELIABILITY_DEFAULT; }

int8_t z_declare_subscriber(z_owned_subscriber_t *sub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                            z_owned_closure_sample_t *callback, const z_subscriber_options_t *options) {
    void *ctx = callback->context;
    callback->context = NULL;
    char *suffix = NULL;

    _z_keyexpr_t key = *keyexpr;
    // TODO: Currently, if resource declarations are done over multicast transports, the current protocol definition
    //       lacks a way to convey them to later-joining nodes. Thus, in the current version automatic
    //       resource declarations are only performed on unicast transports.
    if (_Z_RC_IN_VAL(zs)._tp._type == _Z_TRANSPORT_UNICAST_TYPE) {
        _z_resource_t *r = _z_get_resource_by_key(&_Z_RC_IN_VAL(zs), keyexpr);
        if (r == NULL) {
            char *wild = strpbrk(keyexpr->_suffix, "*$");
            _Bool do_keydecl = true;
            _z_keyexpr_t resource_key = *keyexpr;
            if (wild != NULL && wild != resource_key._suffix) {
                wild -= 1;
                size_t len = wild - resource_key._suffix;
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
                uint16_t id = _z_declare_resource(&_Z_RC_IN_VAL(zs), resource_key);
                key = _z_rid_with_suffix(id, wild);
            }
            _z_keyexpr_clear(&resource_key);
        }
    }

    _z_subinfo_t subinfo = _z_subinfo_default();
    if (options != NULL) {
        subinfo.reliability = options->reliability;
    }
    _z_subscriber_t *int_sub = _z_declare_subscriber(zs, key, subinfo, callback->call, callback->drop, ctx);
    if (suffix != NULL) {
        z_free(suffix);
    }

    sub->_val = int_sub;

    if (int_sub == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    } else {
        return _Z_RES_OK;
    }
}

int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub) { return _z_subscriber_drop(&sub->_val); }

int8_t z_subscriber_keyexpr(z_owned_keyexpr_t *keyexpr, z_loaned_subscriber_t *sub) {
    // Init keyexpr
    z_keyexpr_null(keyexpr);
    if ((keyexpr == NULL) || (sub == NULL)) {
        return _Z_ERR_GENERIC;
    }
    uint32_t lookup = sub->_entity_id;
    _z_subscription_rc_list_t *tail = sub->_zn.in->val._local_subscriptions;
    while (tail != NULL && !z_keyexpr_check(keyexpr)) {
        _z_subscription_rc_t *head = _z_subscription_rc_list_head(tail);
        if (head->in->val._id == lookup) {
            // Allocate keyexpr
            keyexpr->_val = (_z_keyexpr_t *)z_malloc(sizeof(_z_keyexpr_t));
            if (keyexpr->_val == NULL) {
                return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }
            _z_keyexpr_copy(keyexpr->_val, &head->in->val._key);
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
    return _zp_start_read_task(&_Z_RC_IN_VAL(zs), opt.task_attributes);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_read_task(z_loaned_session_t *zs) {
#if Z_FEATURE_MULTI_THREAD == 1
    return _zp_stop_read_task(&_Z_RC_IN_VAL(zs));
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
    return _zp_start_lease_task(&_Z_RC_IN_VAL(zs), opt.task_attributes);
#else
    (void)(zs);
    return -1;
#endif
}

int8_t zp_stop_lease_task(z_loaned_session_t *zs) {
#if Z_FEATURE_MULTI_THREAD == 1
    return _zp_stop_lease_task(&_Z_RC_IN_VAL(zs));
#else
    (void)(zs);
    return -1;
#endif
}

void zp_read_options_default(zp_read_options_t *options) { options->__dummy = 0; }

int8_t zp_read(const z_loaned_session_t *zs, const zp_read_options_t *options) {
    (void)(options);
    return _zp_read(&_Z_RC_IN_VAL(zs));
}

void zp_send_keep_alive_options_default(zp_send_keep_alive_options_t *options) { options->__dummy = 0; }

int8_t zp_send_keep_alive(const z_loaned_session_t *zs, const zp_send_keep_alive_options_t *options) {
    (void)(options);
    return _zp_send_keep_alive(&_Z_RC_IN_VAL(zs));
}

void zp_send_join_options_default(zp_send_join_options_t *options) { options->__dummy = 0; }

int8_t zp_send_join(const z_loaned_session_t *zs, const zp_send_join_options_t *options) {
    (void)(options);
    return _zp_send_join(&_Z_RC_IN_VAL(zs));
}
