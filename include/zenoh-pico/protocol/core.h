/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_PICO_PROTOCOL_CORE_H
#define ZENOH_PICO_PROTOCOL_CORE_H

#include <string.h>
#include "zenoh-pico/config.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/api/constants.h"

/**
 * Whatami values.
 */
#define Z_ROUTER 0x01 // 1 << 0
#define Z_PEER 0x02   // 1 << 1
#define Z_CLIENT 0x04 // 1 << 2

/**
 * Query kind values.
 */
#define Z_QUERYABLE_ALL_KINDS 0x01 // 1 << 0
#define Z_QUERYABLE_STORAGE 0x02   // 1 << 1
#define Z_QUERYABLE_EVAL 0x04      // 1 << 2

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
 * A zenoh encoding.
 */
typedef struct
{
    _z_zint_t _prefix;
    _z_str_t _suffix;
} _z_encoding_t;

/**
 * A zenoh timestamp.
 */
typedef struct
{
    uint64_t _time;
    _z_bytes_t _id;
} _z_timestamp_t;

/**
 * A zenoh-net resource key.
 *
 * Members:
 *   _z_zint_t: The resource ID.
 *   _z_str_t val: A pointer to the string containing the resource name.
 */
typedef struct
{
    _z_zint_t _rid;
    _z_str_t _rname;
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
typedef struct
{
    _z_keyexpr_t _key;
    _z_bytes_t _value;
    _z_encoding_t _encoding;
} _z_sample_t;

/**
 * A hello message returned by a zenoh entity to a scout message sent with :c:func:`_z_scout`.
 *
 * Members:
 *   unsigned int whatami: The kind of zenoh entity.
 *   _z_bytes_t pid: The peer id of the scouted entity (empty if absent).
 *   _z_str_array_t locators: The locators of the scouted entity.
 */
typedef struct
{
    unsigned int _whatami;
    _z_bytes_t _pid;
    _z_str_array_t _locators;
} _z_hello_t;
void _z_hello_clear(_z_hello_t *src);
void _z_hello_free(_z_hello_t **hello);
_Z_ELEM_DEFINE(_z_hello, _z_hello_t, _z_noop_size, _z_hello_clear, _z_noop_copy)
_Z_ARRAY_DEFINE(_z_hello, _z_hello_t)

/**
 * The possible values of :c:member:`_z_target_t.tag`.
 *
 *     - **Z_TARGET_BEST_MATCHING**: The nearest complete queryable if any else all matching queryables.
 *     - **Z_TARGET_ALL**: All matching queryables.
 *     - **Z_TARGET_NONE**: No queryables.
 *     - **Z_TARGET_COMPLETE**: A set of complete queryables.
 */
typedef enum
{
    Z_TARGET_BEST_MATCHING = 0,
    Z_TARGET_ALL = 1,
    Z_TARGET_NONE = 2,
    Z_TARGET_ALLCOMPLETE = 3,
    Z_TARGET_COMPLETE = 4
} _z_target_t;

typedef struct
{
    _z_zint_t _n;
} _z_target_complete_body_t;

/**
 * The zenoh-net queryables that should be target of a :c:func:`z_query`.
 *
 * Members:
 *     unsigned int kind: A mask of queryable kinds.
 *     _z_target_t target: The query target.
 */
typedef struct
{
    unsigned int _kind;
    _z_target_t _target;
    union
    {
        _z_target_complete_body_t _complete;
    } _type;

} _z_query_target_t;

/**
 * The congestion control.
 *
 *     - **Z_CONGESTION_CONTROL_BLOCK**
 *     - **Z_CONGESTION_CONTROL_DROP**
 */
typedef enum
{
    Z_CONGESTION_CONTROL_BLOCK = 0,
    Z_CONGESTION_CONTROL_DROP = 1,
} _z_congestion_control_t;

/**
 * The subscription period.
 *
 * Members:
 *     unsigned int origin:
 *     unsigned int period:
 *     unsigned int duration:
 */
typedef struct
{
    unsigned int _origin;
    unsigned int _period;
    unsigned int _duration;
} _z_period_t;

/**
 * The subscription mode.
 *
 *     - **Z_SUBMODE_PUSH**
 *     - **Z_SUBMODE_PULL**
 */
typedef enum
{
    Z_SUBMODE_PUSH = 0,
    Z_SUBMODE_PULL = 1,
} _z_submode_t;

/**
 * Informations to be passed to :c:func:`_z_declare_subscriber` to configure the created :c:type:`_z_subscription_t`.
 *
 * Members:
 *     z_reliability_t reliability: The subscription reliability.
 *     _z_submode_t mode: The subscription mode.
 *     _z_period_t *period: The subscription period.
 */
typedef struct
{
    z_reliability_t _reliability;
    _z_submode_t _mode;
    _z_period_t _period;
} _z_subinfo_t;

#endif /* ZENOH_PICO_PROTOCOL_CORE_H */
