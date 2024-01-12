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
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

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

typedef struct {
    uint8_t id[16];
} _z_id_t;
uint8_t _z_id_len(_z_id_t id);
_Bool _z_id_check(_z_id_t id);
_z_id_t _z_id_empty(void);

/**
 * A zenoh encoding.
 */
typedef struct {
    _z_bytes_t suffix;
    z_encoding_prefix_t prefix;
} _z_encoding_t;

/**
 * A zenoh timestamp.
 */
typedef struct {
    _z_id_t id;
    uint64_t time;
} _z_timestamp_t;

_z_timestamp_t _z_timestamp_duplicate(const _z_timestamp_t *tstamp);
_z_timestamp_t _z_timestamp_null(void);
void _z_timestamp_clear(_z_timestamp_t *tstamp);
_Bool _z_timestamp_check(const _z_timestamp_t *stamp);

/**
 * The product of:
 * - top-most bit: whether or not the keyexpr containing this mapping owns its suffix (1=true)
 * - the mapping for the keyexpr prefix:
 *     - 0: local mapping.
 *     - 0x7fff (MAX): unknown remote mapping.
 *     - x: the mapping associated with the x-th peer.
 */
typedef struct {
    uint16_t _val;
} _z_mapping_t;
#define _Z_KEYEXPR_MAPPING_LOCAL 0
#define _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE 0x7fff

/**
 * A zenoh-net resource key.
 *
 * Members:
 *   _z_zint_t: The resource ID.
 *   char *val: A pointer to the string containing the resource name.
 */
typedef struct {
    uint16_t _id;
    _z_mapping_t _mapping;
    char *_suffix;
} _z_keyexpr_t;
static inline _Bool _z_keyexpr_owns_suffix(const _z_keyexpr_t *key) { return (key->_mapping._val & 0x8000) != 0; }
static inline uint16_t _z_keyexpr_mapping_id(const _z_keyexpr_t *key) { return key->_mapping._val & 0x7fff; }
static inline _Bool _z_keyexpr_is_local(const _z_keyexpr_t *key) {
    return (key->_mapping._val & 0x7fff) == _Z_KEYEXPR_MAPPING_LOCAL;
}
static inline _z_mapping_t _z_keyexpr_mapping(uint16_t id, _Bool owns_suffix) {
    assert(id <= _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE);
    _z_mapping_t mapping = {(uint16_t)((owns_suffix ? 0x8000 : 0) | id)};
    return mapping;
}
static inline void _z_keyexpr_set_mapping(_z_keyexpr_t *ke, uint16_t id) {
    assert(id <= _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE);
    ke->_mapping._val &= 0x8000;
    ke->_mapping._val |= id;
}
static inline void _z_keyexpr_fix_mapping(_z_keyexpr_t *ke, uint16_t id) {
    if (_z_keyexpr_mapping_id(ke) == _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE) {
        _z_keyexpr_set_mapping(ke, id);
    }
}
static inline void _z_keyexpr_set_owns_suffix(_z_keyexpr_t *ke, _Bool owns_suffix) {
    ke->_mapping._val &= 0x7fff;
    ke->_mapping._val |= owns_suffix ? 0x8000 : 0;
}
static inline _Bool _z_keyexpr_has_suffix(_z_keyexpr_t ke) { return (ke._suffix != NULL) && (ke._suffix[0] != 0); }
static inline _Bool _z_keyexpr_check(_z_keyexpr_t ke) { return (ke._id != 0) || _z_keyexpr_has_suffix(ke); }

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

/**
 * A zenoh-net data sample.
 *
 * A sample is the value associated to a given resource at a given point in time.
 *
 * Members:
 *   _z_keyexpr_t key: The resource key of this data sample.
 *   _z_bytes_t value: The value of this data sample.
 *   _z_encoding_t encoding: The encoding for the value of this data sample.
 */
typedef struct {
    _z_keyexpr_t keyexpr;
    _z_bytes_t payload;
    _z_timestamp_t timestamp;
    _z_encoding_t encoding;
    z_sample_kind_t kind;
} _z_sample_t;

/**
 * Represents a Zenoh value.
 *
 * Members:
 *   _z_encoding_t encoding: The encoding of the `payload`.
 *   _z_bytes_t payload: The payload of this zenoh value.
 */
typedef struct {
    _z_bytes_t payload;
    _z_encoding_t encoding;
} _z_value_t;
_z_value_t _z_value_null(void);
_z_value_t _z_value_steal(_z_value_t *value);
void _z_value_clear(_z_value_t *src);
void _z_value_free(_z_value_t **hello);

/**
 * A hello message returned by a zenoh entity to a scout message sent with :c:func:`_z_scout`.
 *
 * Members:
 *   _z_bytes_t zid: The Zenoh ID of the scouted entity (empty if absent).
 *   _z_str_array_t locators: The locators of the scouted entity.
 *   z_whatami_t whatami: The kind of zenoh entity.
 */
typedef struct {
    _z_id_t zid;
    _z_str_array_t locators;
    z_whatami_t whatami;
    uint8_t version;
} _z_hello_t;
void _z_hello_clear(_z_hello_t *src);
void _z_hello_free(_z_hello_t **hello);
_Z_ELEM_DEFINE(_z_hello, _z_hello_t, _z_noop_size, _z_hello_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_hello, _z_hello_t)

typedef struct {
    _z_zint_t n;
} _z_target_complete_body_t;

/**
 * The subscription period.
 *
 * Members:
 *     unsigned int origin:
 *     unsigned int period:
 *     unsigned int duration:
 */
typedef struct {
    unsigned int origin;
    unsigned int period;
    unsigned int duration;
} _z_period_t;

/**
 * Informations to be passed to :c:func:`_z_declare_subscriber` to configure the created
 * :c:type:`_z_subscription_rc_t`.
 *
 * Members:
 *     _z_period_t *period: The subscription period.
 *     z_reliability_t reliability: The subscription reliability.
 *     _z_submode_t mode: The subscription mode.
 */
typedef struct {
    _z_period_t period;
    z_reliability_t reliability;
    z_submode_t mode;
} _z_subinfo_t;

typedef struct {
    _z_id_t _id;
    uint32_t _entity_id;
    uint32_t _source_sn;
} _z_source_info_t;
_z_source_info_t _z_source_info_null(void);

typedef struct {
    uint32_t _request_id;
    uint32_t _entity_id;
} _z_reply_context_t;

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CORE_H */
