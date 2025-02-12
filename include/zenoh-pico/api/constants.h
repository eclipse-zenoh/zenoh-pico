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

#define Z_SELECTOR_TIME "_time="
#define Z_SELECTOR_QUERY_MATCH "_anyke"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * What bitmask for scouting.
 *
 * Enumerators:
 *   Z_WHAT_ROUTER: Router.
 *   Z_WHAT_PEER: Peer.
 *   Z_WHAT_CLIENT: Client.
 */
typedef enum {
    Z_WHAT_ROUTER = 0x01,  // Router
    Z_WHAT_PEER = 0x02,    // Peer
    Z_WHAT_CLIENT = 0x04,  // Client
    Z_WHAT_ROUTER_PEER = (0x01 | 0x02),
    Z_WHAT_ROUTER_CLIENT = (0x01 | 0x04),
    Z_WHAT_PEER_CLIENT = (0x02 | 0x04),
    Z_WHAT_ROUTER_PEER_CLIENT = ((0x01 | 0x02) | 0x04),
} z_what_t;

/**
 * Whatami values, defined as a bitmask.
 *
 * Enumerators:
 *   Z_WHATAMI_ROUTER: Bitmask to filter Zenoh routers.
 *   Z_WHATAMI_PEER: Bitmask to filter for Zenoh peers.
 *   Z_WHATAMI_CLIENT: Bitmask to filter for Zenoh clients.
 */
typedef enum z_whatami_t {
    Z_WHATAMI_ROUTER = 0x01,
    Z_WHATAMI_PEER = 0x02,
    Z_WHATAMI_CLIENT = 0x04,
} z_whatami_t;
#define Z_WHATAMI_DEFAULT Z_WHATAMI_ROUTER;

/**
 * Status values for keyexpr canonization operation.
 * Used as return value of canonization-related functions,
 * like :c:func:`z_keyexpr_is_canon` or :c:func:`z_keyexpr_canonize`.
 *
 * Enumerators:
 *   Z_KEYEXPR_CANON_SUCCESS: The key expression is canon.
 *   Z_KEYEXPR_CANON_LONE_DOLLAR_STAR: The key contains a ``$*`` chunk, which must be replaced by ``*``.
 *   Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR: The key contains ``** / *``, which must be replaced by ``* / **``.
 *   Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR: The key contains ``** / **``, which must be replaced by ``**``.
 *   Z_KEYEXPR_CANON_EMPTY_CHUNK: The key contains empty chunks.
 *   Z_KEYEXPR_CANON_STARS_IN_CHUNK: The key contains a ``*`` in a chunk without being escaped by a DSL, which is
 *     forbidden.
 *   Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR: The key contains ``$*$`` or ``$$``, which is forbidden.
 *   Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK: The key contains ``#`` or ``?``, which is forbidden.
 *   Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR: The key contains a ``$`` which is not bound to a DSL.
 */
typedef enum {
    Z_KEYEXPR_CANON_SUCCESS = 0,
    Z_KEYEXPR_CANON_LONE_DOLLAR_STAR = -1,
    Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR = -2,
    Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR = -3,
    Z_KEYEXPR_CANON_EMPTY_CHUNK = -4,
    Z_KEYEXPR_CANON_STARS_IN_CHUNK = -5,
    Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR = -6,
    Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK = -7,
    Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR = -8
} zp_keyexpr_canon_status_t;

/**
 * Intersection level of two key expressions.
 *
 * Enumerators:
 *   Z_KEYEXPR_INTERSECTION_LEVEL_DISJOINT: The two key expressions do not intersect.
 *   Z_KEYEXPR_INTERSECTION_LEVEL_INTERSECTS: The two key expressions intersect, i.e. there exists at least one key
 *     expression that is included by both.
 *   Z_KEYEXPR_INTERSECTION_LEVEL_INCLUDES: The first key expression is the superset of the second one.
 *   Z_KEYEXPR_INTERSECTION_LEVEL_EQUALS: The two key expressions are equal.
 */
typedef enum {
    Z_KEYEXPR_INTERSECTION_LEVEL_DISJOINT = 0,
    Z_KEYEXPR_INTERSECTION_LEVEL_INTERSECTS = 1,
    Z_KEYEXPR_INTERSECTION_LEVEL_INCLUDES = 2,
    Z_KEYEXPR_INTERSECTION_LEVEL_EQUALS = 3,
} z_keyexpr_intersection_level_t;

/**
 * Sample kind values.
 *
 * Enumerators:
 *   Z_SAMPLE_KIND_PUT: The Sample was issued by a ``put`` operation.
 *   Z_SAMPLE_KIND_DELETE: The Sample was issued by a ``delete`` operation.
 */
typedef enum { Z_SAMPLE_KIND_PUT = 0, Z_SAMPLE_KIND_DELETE = 1 } z_sample_kind_t;
#define Z_SAMPLE_KIND_DEFAULT Z_SAMPLE_KIND_PUT

/**
 * Consolidation mode values.
 *
 * Enumerators:
 *   Z_CONSOLIDATION_MODE_AUTO: Let Zenoh decide the best consolidation mode depending on the query selector.
 *   Z_CONSOLIDATION_MODE_NONE: No consolidation is applied. Replies may come in any order and any number.
 *   Z_CONSOLIDATION_MODE_MONOTONIC: It guarantees that any reply for a given key expression will be monotonic in time
 *     w.r.t. the previous received replies for the same key expression. I.e., for the same key expression multiple
 *     replies may be received. It is guaranteed that two replies received at t1 and t2 will have timestamp
 *     ts2 > ts1. It optimizes latency.
 *   Z_CONSOLIDATION_MODE_LATEST: It guarantees unicity of replies for the same key expression.
 *     It optimizes bandwidth.
 */
typedef enum {
    Z_CONSOLIDATION_MODE_AUTO = -1,
    Z_CONSOLIDATION_MODE_NONE = 0,
    Z_CONSOLIDATION_MODE_MONOTONIC = 1,
    Z_CONSOLIDATION_MODE_LATEST = 2,
} z_consolidation_mode_t;
#define Z_CONSOLIDATION_MODE_DEFAULT Z_CONSOLIDATION_MODE_AUTO

/**
 * Reliability values.
 *
 * Enumerators:
 *   Z_RELIABILITY_BEST_EFFORT: Defines reliability as ``BEST_EFFORT``
 *   Z_RELIABILITY_RELIABLE: Defines reliability as ``RELIABLE``
 *
 * .. warning:: This API has been marked as unstable: it works as advertised, but it may be changed in a future release.
 */
typedef enum { Z_RELIABILITY_BEST_EFFORT = 1, Z_RELIABILITY_RELIABLE = 0 } z_reliability_t;
#define Z_RELIABILITY_DEFAULT Z_RELIABILITY_RELIABLE

/**
 * Congestion control values.
 *
 * Enumerators:
 *   Z_CONGESTION_CONTROL_BLOCK: Defines congestion control as ``BLOCK``. Messages are not dropped in case of
 *     congestion control.
 *   Z_CONGESTION_CONTROL_DROP: Defines congestion control as ``DROP``. Messages are dropped in case
 *     of congestion control.
 */
typedef enum { Z_CONGESTION_CONTROL_BLOCK = 1, Z_CONGESTION_CONTROL_DROP = 0 } z_congestion_control_t;
#define Z_CONGESTION_CONTROL_DEFAULT Z_CONGESTION_CONTROL_DROP

static inline z_congestion_control_t z_internal_congestion_control_default_push(void) {
    return Z_CONGESTION_CONTROL_DROP;
}
static inline z_congestion_control_t z_internal_congestion_control_default_request(void) {
    return Z_CONGESTION_CONTROL_BLOCK;
}
static inline z_congestion_control_t z_internal_congestion_control_default_response(void) {
    return Z_CONGESTION_CONTROL_BLOCK;
}

/**
 * Priority of Zenoh messages values.
 *
 * Enumerators:
 *   _Z_PRIORITY_CONTROL: Priority for ``Control`` messages.
 *   Z_PRIORITY_REAL_TIME: Priority for ``RealTime`` messages.
 *   Z_PRIORITY_INTERACTIVE_HIGH: Highest priority for ``Interactive`` messages.
 *   Z_PRIORITY_INTERACTIVE_LOW: Lowest priority for ``Interactive`` messages.
 *   Z_PRIORITY_DATA_HIGH: Highest priority for ``Data`` messages.
 *   Z_PRIORITY_DATA: Default priority for ``Data`` messages.
 *   Z_PRIORITY_DATA_LOW: Lowest priority for ``Data`` messages.
 *   Z_PRIORITY_BACKGROUND: Priority for ``Background traffic`` messages.
 */
typedef enum {
    _Z_PRIORITY_CONTROL = 0,
    Z_PRIORITY_REAL_TIME = 1,
    Z_PRIORITY_INTERACTIVE_HIGH = 2,
    Z_PRIORITY_INTERACTIVE_LOW = 3,
    Z_PRIORITY_DATA_HIGH = 4,
    Z_PRIORITY_DATA = 5,
    Z_PRIORITY_DATA_LOW = 6,
    Z_PRIORITY_BACKGROUND = 7
} z_priority_t;
#define Z_PRIORITY_DEFAULT Z_PRIORITY_DATA

/**
 * Query target values.
 *
 * Enumerators:
 *   Z_QUERY_TARGET_BEST_MATCHING: The nearest complete queryable if any else all matching queryables.
 *   Z_QUERY_TARGET_ALL: All matching queryables.
 *   Z_QUERY_TARGET_ALL_COMPLETE: A set of complete queryables.
 */
typedef enum {
    Z_QUERY_TARGET_BEST_MATCHING = 0,
    Z_QUERY_TARGET_ALL = 1,
    Z_QUERY_TARGET_ALL_COMPLETE = 2
} z_query_target_t;
#define Z_QUERY_TARGET_DEFAULT Z_QUERY_TARGET_BEST_MATCHING

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_API_CONSTANTS_H */
