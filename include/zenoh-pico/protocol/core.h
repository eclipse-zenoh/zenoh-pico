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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_CORE_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_CORE_H

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/refcount.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _Z_OPTIONAL
#define _Z_MOVE(x) x *

/**
 * The reserved resource ID indicating a string-only resource key.
 */
#define Z_RESOURCE_ID_NONE 0

/**
 * Priority values.
 */
#define Z_PRIORITIES_NUM 8

/**
 * A variable-length encoding unsigned integer.
 */
typedef size_t _z_zint_t;

/**
 * A zenoh ID.
 */

#define ZENOH_ID_SIZE 16
typedef struct {
    uint8_t id[ZENOH_ID_SIZE];
} _z_id_t;
extern const _z_id_t empty_id;
uint8_t _z_id_len(_z_id_t id);
static inline bool _z_id_check(_z_id_t id) { return memcmp(&id, &empty_id, sizeof(id)) != 0; }
static inline bool _z_id_eq(const _z_id_t *left, const _z_id_t *right) {
    return memcmp(left->id, right->id, ZENOH_ID_SIZE) == 0;
}
static inline _z_id_t _z_id_empty(void) { return (_z_id_t){0}; }

typedef struct {
    _z_id_t zid;
    uint32_t eid;
} _z_entity_global_id_t;

// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_entity_global_id_t _z_entity_global_id_null(void) { return (_z_entity_global_id_t){0}; }
static inline bool _z_entity_global_id_check(const _z_entity_global_id_t *info) {
    return _z_id_check(info->zid) || info->eid != 0;
}

/**
 * A zenoh timestamp.
 */
typedef struct {
    bool valid;
    _z_id_t id;
    uint64_t time;
} _z_timestamp_t;

// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_timestamp_t _z_timestamp_null(void) { return (_z_timestamp_t){0}; }
static inline void _z_timestamp_invalid(_z_timestamp_t *tstamp) { tstamp->valid = false; }
static inline bool _z_timestamp_check(const _z_timestamp_t *tstamp) { return tstamp->valid; }
void _z_timestamp_copy(_z_timestamp_t *dst, const _z_timestamp_t *src);
_z_timestamp_t _z_timestamp_duplicate(const _z_timestamp_t *tstamp);
void _z_timestamp_clear(_z_timestamp_t *tstamp);
void _z_timestamp_move(_z_timestamp_t *dst, _z_timestamp_t *src);
uint64_t _z_timestamp_ntp64_from_time(uint32_t seconds, uint32_t nanos);

/**
 * A zenoh key-expression.
 *
 * Members:
 *  uint16_t _id: The resource ID of the ke.
 *  uintptr_t _mapping: Address of the peer as id, if ke is remotely declared.
 *  _z_string_t _suffix: The string value of the ke.
 */
// Note on the _mapping field: there are collisions on _id value between peers/local, this field is used only to
// distinguish which peer/local id space we are in and should not be dereferenced, just compared. NULL/0 value is used
// for local declared keyexpr and the address of empty_id as a placeholder.
#define _Z_KEYEXPR_MAPPING_LOCAL (uintptr_t)0

typedef struct {
    uint16_t _id;
    uintptr_t _mapping;
    _z_string_t _suffix;
} _z_keyexpr_t;

static inline bool _z_keyexpr_is_local(const _z_keyexpr_t *key) { return key->_mapping == _Z_KEYEXPR_MAPPING_LOCAL; }
static inline bool _z_keyexpr_has_suffix(const _z_keyexpr_t *ke) { return _z_string_check(&ke->_suffix); }
static inline bool _z_keyexpr_check(const _z_keyexpr_t *ke) {
    return (ke->_id != Z_RESOURCE_ID_NONE) || _z_keyexpr_has_suffix(ke);
}

/**
 * Create a resource key from a resource name.
 *
 * Parameters:
 *     rname: The resource name. The caller keeps its ownership.
 *
 * Returns:
 *     A :c:type:`_z_keyexpr_t` containing a new resource key.
 */
_z_keyexpr_t _z_rname(const char *rname);

/**
 * Create a resource key from a resource id and a suffix.
 *
 * Parameters:
 *     id: The resource id.
 *     suffix: The suffix.
 *
 * Returns:
 *     A :c:type:`_z_keyexpr_t` containing a new resource key.
 */
_z_keyexpr_t _z_rid_with_suffix(uint16_t rid, const char *suffix);
_z_keyexpr_t _z_rid_with_substr_suffix(uint16_t rid, const char *suffix, size_t suffix_len);

/**
 * QoS settings of zenoh message.
 */
typedef struct {
    uint8_t _val;
} _z_qos_t;

/**
 * Represents a Zenoh value.
 *
 * Members:
 *   _z_bytes_t payload: The payload of this zenoh value.
 *   _z_encoding_t encoding: The encoding of the `payload`.
 */
typedef struct {
    _z_bytes_t payload;
    _z_encoding_t encoding;
} _z_value_t;

// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_value_t _z_value_null(void) { return (_z_value_t){0}; }
static inline bool _z_value_check(const _z_value_t *value) {
    return _z_bytes_check(&value->payload) || _z_encoding_check(&value->encoding);
}
static inline _z_value_t _z_value_alias(_z_value_t *src) {
    _z_value_t dst;
    dst.payload = _z_bytes_alias(&src->payload);
    dst.encoding = _z_encoding_alias(&src->encoding);
    return dst;
}
_z_value_t _z_value_steal(_z_value_t *value);
z_result_t _z_value_copy(_z_value_t *dst, const _z_value_t *src);
z_result_t _z_value_move(_z_value_t *dst, _z_value_t *src);
void _z_value_clear(_z_value_t *src);
void _z_value_free(_z_value_t **hello);

/**
 * A hello message returned by a zenoh entity to a scout message sent with :c:func:`_z_scout`.
 *
 * Members:
 *   _z_slice_t zid: The Zenoh ID of the scouted entity (empty if absent).
 *   _z_string_vec_t locators: The locators of the scouted entity.
 *   z_whatami_t whatami: The kind of zenoh entity.
 */
typedef struct {
    _z_id_t _zid;
    _z_string_svec_t _locators;
    z_whatami_t _whatami;
    uint8_t _version;
} _z_hello_t;

// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_hello_t _z_hello_null(void) { return (_z_hello_t){0}; }
void _z_hello_clear(_z_hello_t *src);
void _z_hello_free(_z_hello_t **hello);
z_result_t _z_hello_copy(_z_hello_t *dst, const _z_hello_t *src);
z_result_t _z_hello_move(_z_hello_t *dst, _z_hello_t *src);
bool _z_hello_check(const _z_hello_t *hello);

_Z_ELEM_DEFINE(_z_hello, _z_hello_t, _z_noop_size, _z_hello_clear, _z_noop_copy, _z_noop_move)
_Z_SLIST_DEFINE(_z_hello, _z_hello_t, false)

typedef struct {
    _z_zint_t n;
} _z_target_complete_body_t;

typedef struct {
    _z_entity_global_id_t _source_id;
    uint32_t _source_sn;
} _z_source_info_t;

// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_source_info_t _z_source_info_null(void) { return (_z_source_info_t){0}; }
static inline void _z_source_info_clear(_z_source_info_t *info) { (void)(info); }
z_result_t _z_source_info_copy(_z_source_info_t *dst, const _z_source_info_t *src);
z_result_t _z_source_info_move(_z_source_info_t *dst, _z_source_info_t *src);
static inline bool _z_source_info_check(const _z_source_info_t *info) {
    return _z_entity_global_id_check(&info->_source_id) || info->_source_sn != 0;
}
static inline _z_source_info_t _z_source_info_steal(_z_source_info_t *info) {
    _z_source_info_t si;
    si._source_id = info->_source_id;
    si._source_sn = info->_source_sn;
    return si;
}
typedef struct {
    uint32_t _request_id;
    uint32_t _entity_id;
} _z_reply_context_t;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CORE_H */
