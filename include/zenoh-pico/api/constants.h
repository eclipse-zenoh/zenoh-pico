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

#ifndef ZENOH_PICO_API_CONSTANTS_H
#define ZENOH_PICO_API_CONSTANTS_H

/**
 * Whatami values, defined as a bitmask.
 */
typedef enum
{
    Z_WHATAMI_ROUTER = 0x01, // 1 << 0
    Z_WHATAMI_PEER = 0x02,   // 1 << 1
    Z_WHATAMI_CLIENT = 0x04  // 1 << 2
} z_whatami_t;


typedef enum
{
    Z_KEYEXPR_CANON_SUCCESS = 0,                          // the key expression is canon
    Z_KEYEXPR_CANON_LONE_DOLLAR_STAR = -1,                // the key contains a `$*` chunk, which must be replaced by `*`
    Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR = -2,   // the key contains `**/*`, which must be replaced by `*/**`
    Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR = -3,   // the key contains `**/**`, which must be replaced by `**`
    Z_KEYEXPR_CANON_EMPTY_CHUNK = -4,                     // the key contains empty chunks
    Z_KEYEXPR_CANON_STARS_IN_CHUNK = -5,                  // the key contains a `*` in a chunk without being escaped by a DSL, which is forbidden
    Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR = -6,     // the key contains `$*$` or `$$`, which is forbidden
    Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK = -7,         // the key contains `#` or `?`, which is forbidden
    Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR = -8          // the key contains a `$` which isn't bound to a DSL
} z_keyexpr_canon_status_t;

typedef enum
{
    Z_SAMPLE_KIND_PUT = 0,
    Z_SAMPLE_KIND_DELETE = 1,
} z_sample_kind_t;

typedef enum
{
    Z_ENCODING_PREFIX_EMPTY = 0,
    Z_ENCODING_PREFIX_APP_OCTET_STREAM = 1,
    Z_ENCODING_PREFIX_APP_CUSTOM = 2,
    Z_ENCODING_PREFIX_TEXT_PLAIN = 3,
    Z_ENCODING_PREFIX_APP_PROPERTIES = 4,
    Z_ENCODING_PREFIX_APP_JSON = 5,
    Z_ENCODING_PREFIX_APP_SQL = 6,
    Z_ENCODING_PREFIX_APP_INTEGER = 7,
    Z_ENCODING_PREFIX_APP_FLOAT = 8,
    Z_ENCODING_PREFIX_APP_XML = 9,
    Z_ENCODING_PREFIX_APP_XHTML_XML = 10,
    Z_ENCODING_PREFIX_APP_X_WWW_FORM_URLENCODED = 11,
    Z_ENCODING_PREFIX_TEXT_JSON = 12,
    Z_ENCODING_PREFIX_TEXT_HTML = 13,
    Z_ENCODING_PREFIX_TEXT_XML = 14,
    Z_ENCODING_PREFIX_TEXT_CSS = 15,
    Z_ENCODING_PREFIX_TEXT_CSV = 16,
    Z_ENCODING_PREFIX_TEXT_JAVASCRIPT = 17,
    Z_ENCODING_PREFIX_IMAGE_JPEG = 18,
    Z_ENCODING_PREFIX_IMAGE_PNG = 19,
    Z_ENCODING_PREFIX_IMAGE_GIF = 20,
} z_encoding_prefix_t;

typedef enum
{
    Z_QUERYABLE_ALL_KINDS = 0x01,  // 1 << 0
    Z_QUERYABLE_STORAGE = 0x02,    // 1 << 1
    Z_QUERYABLE_EVAL = 0x04        // 1 << 2
} z_queryable_kind_t;

/**
 * The kind of consolidation that should be applied on replies to a :c:func:`z_query`.
 *
 *     - **Z_CONSOLIDATION_MODE_FULL**: Guaranties unicity of replies. Optimizes bandwidth.
 *     - **Z_CONSOLIDATION_MODE_LAZY**: Does not garanty unicity. Optimizes latency.
 *     - **Z_CONSOLIDATION_MODE_NONE**: No consolidation.
 */
typedef enum
{
    Z_CONSOLIDATION_MODE_NONE = 0,
    Z_CONSOLIDATION_MODE_LAZY = 1,
    Z_CONSOLIDATION_MODE_FULL = 2,
} z_consolidation_mode_t;

/**
 * The subscription reliability.
 *
 *     - **Z_RELIABILITY_BEST_EFFORT**
 *     - **Z_RELIABILITY_RELIABLE**
 */
typedef enum
{
    Z_RELIABILITY_BEST_EFFORT = 0,
    Z_RELIABILITY_RELIABLE = 1,
} z_reliability_t;

typedef enum
{
    Z_QUERY_CONSOLIDATION_AUTO = 0,
    Z_QUERY_CONSOLIDATION_MANUAL = 1,
} z_query_consolidation_tag_t;

/**
 * The possible values of :c:member:`_z_reply_t.tag`
 *
 *     - **Z_REPLY_TAG_DATA**: The reply contains some data.
 *     - **Z_REPLY_TAG_FINAL**: The reply does not contain any data and indicates that there will be no more replies for this query.
 */
typedef enum
{
    Z_REPLY_TAG_DATA = 0,
    Z_REPLY_TAG_FINAL = 1,
} z_reply_tag_t;

#define Z_PERIOD_NONE (z_period_t){.origin = 0, .period = 0, .duration = 0}

typedef enum
{
    Z_CONGESTION_CONTROL_BLOCK = 0,
    Z_CONGESTION_CONTROL_DROP = 1,
} z_congestion_control_t;

typedef enum
{
    Z_PRIORITY_CONTROL = 0,
    Z_PRIORITY_REALTIME = 1,
    Z_PRIORITY_INTERACTIVE_HIGH = 2,
    Z_PRIORITY_INTERACTIVE_LOW = 3,
    Z_PRIORITY_DATA_HIGH = 4,
    Z_PRIORITY_DATA = 5,
    Z_PRIORITY_DATA_LOW = 6,
    Z_PRIORITY_BACKGROUND = 7,
} z_priority_t;

typedef enum
{
    Z_SUBMODE_PUSH = 0,
    Z_SUBMODE_PULL = 1,
} z_submode_t;

/**
 * The possible values of :c:member:`z_query_target_t.tag`.
 *
 *     - **Z_TARGET_BEST_MATCHING**: The nearest complete queryable if any else all matching queryables.
 *     - **Z_TARGET_ALL**: All matching queryables.
 *     - **Z_TARGET_NONE**: No queryables.
 *     - **Z_TARGET_ALL_COMPLETE**: A set of complete queryables.
 */
typedef enum
{
    Z_QUERY_TARGET_BEST_MATCHING = 0,
    Z_QUERY_TARGET_ALL = 1,
    Z_QUERY_TARGET_ALL_COMPLETE = 2,
} z_query_target_t;

//FIXME: move to types.h
/**
 * The kind of consolidation that should be applied on replies to a :c:func:`z_query`
 * at the different stages of the reply process.
 *
 * Members:
 *   first_routers: The consolidation mode to apply on first routers of the replies routing path.
 *   last_router: The consolidation mode to apply on last router of the replies routing path.
 *   reception: The consolidation mode to apply at reception of the replies.
 */
typedef struct
{
    z_consolidation_mode_t first_routers;
    z_consolidation_mode_t last_router;
    z_consolidation_mode_t reception;
} z_consolidation_strategy_t;

#endif /* ZENOH_PICO_API_CONSTANTS_H */
