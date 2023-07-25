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

#ifndef ZENOH_PICO_PROTOCOL_CORE_H
#define ZENOH_PICO_PROTOCOL_CORE_H

#include <string.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"

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
void _z_timestamp_reset(_z_timestamp_t *tstamp);
_Bool _z_timestamp_check(const _z_timestamp_t *stamp);

/**
 * A zenoh-net resource key.
 *
 * Members:
 *   _z_zint_t: The resource ID.
 *   char *val: A pointer to the string containing the resource name.
 */
typedef struct {
    _z_zint_t _id;
    const char *_suffix;
} _z_keyexpr_t;

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

/**
 * A hello message returned by a zenoh entity to a scout message sent with :c:func:`_z_scout`.
 *
 * Members:
 *   _z_bytes_t zid: The Zenoh ID of the scouted entity (empty if absent).
 *   _z_str_array_t locators: The locators of the scouted entity.
 *   uint8_t whatami: The kind of zenoh entity.
 */
typedef struct {
    _z_bytes_t zid;
    _z_str_array_t locators;
    uint8_t whatami;
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
 * :c:type:`_z_subscription_sptr_t`.
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

#endif /* ZENOH_PICO_PROTOCOL_CORE_H */
