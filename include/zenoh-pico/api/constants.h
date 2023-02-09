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
 * Whatami values, defined as a bitmask.
 *
 * Enumerators:
 *     Z_WHATAMI_ROUTER: Bitmask to filter Zenoh routers.
 *     Z_WHATAMI_PEER: Bitmask to filter for Zenoh peers.
 *     Z_WHATAMI_CLIENT: Bitmask to filter for Zenoh clients.
 */
typedef enum {
    Z_WHATAMI_ROUTER = 0x01,  // 1 << 0
    Z_WHATAMI_PEER = 0x02,    // 1 << 1
    Z_WHATAMI_CLIENT = 0x04   // 1 << 2
} z_whatami_t;
#define Z_WHATAMI_DEFAULT Z_WHATAMI_ROUTER

/**
 * Status values for keyexpr canonization operation.
 * Used as return value of canonization-related functions,
 * like :c:func:`z_keyexpr_is_canon` or :c:func:`z_keyexpr_canonize`.
 *
 * Enumerators:
 *     Z_KEYEXPR_CANON_SUCCESS: The key expression is canon.
 *     Z_KEYEXPR_CANON_LONE_DOLLAR_STAR: The key contains a ``$*`` chunk, which must be replaced by ``*``.
 *     Z_KEYEXPR_CANON_SINGLE_STAR_AFTER_DOUBLE_STAR: The key contains ``** / *``, which must be replaced by ``* / **``.
 *     Z_KEYEXPR_CANON_DOUBLE_STAR_AFTER_DOUBLE_STAR: The key contains ``** / **``, which must be replaced by ``**``.
 *     Z_KEYEXPR_CANON_EMPTY_CHUNK: The key contains empty chunks.
 *     Z_KEYEXPR_CANON_STARS_IN_CHUNK: The key contains a ``*`` in a chunk without being escaped by a DSL, which is
 *         forbidden.
 *     Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR: The key contains ``$*$`` or ``$$``, which is forbidden.
 *     Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK: The key contains ``#`` or ``?``, which is forbidden.
 *     Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR: The key contains a ``$`` which is not bound to a DSL.
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
 * Sample kind values.
 *
 * Enumerators:
 *     Z_SAMPLE_KIND_PUT: The Sample was issued by a ``put`` operation.
 *     Z_SAMPLE_KIND_DELETE: The Sample was issued by a ``delete`` operation.
 */
typedef enum { Z_SAMPLE_KIND_PUT = 0, Z_SAMPLE_KIND_DELETE = 1 } z_sample_kind_t;
#define Z_SAMPLE_KIND_DEFAULT Z_SAMPLE_KIND_PUT

/**
 * Zenoh encoding values.
 * These values are based on already existing HTTP MIME types and extended with other relevant encodings.
 *
 * Enumerators:
 *     Z_ENCODING_PREFIX_EMPTY: Encoding not defined.
 *     Z_ENCODING_PREFIX_APP_OCTET_STREAM: ``application/octet-stream``. Default value for all other cases. An unknown
 *         file type should use this type. Z_ENCODING_PREFIX_APP_CUSTOM: Custom application type. Non IANA standard.
 *     Z_ENCODING_PREFIX_TEXT_PLAIN: ``text/plain``. Default value for textual files. A textual file should be
 *         human-readable and must not contain binary data. Z_ENCODING_PREFIX_APP_PROPERTIES: Application properties
 *         type. Non IANA standard. Z_ENCODING_PREFIX_APP_JSON: ``application/json``. JSON format.
 *     Z_ENCODING_PREFIX_APP_SQL: Application sql type. Non IANA standard. Z_ENCODING_PREFIX_APP_INTEGER: Application
 *         integer type. Non IANA standard. Z_ENCODING_PREFIX_APP_FLOAT: Application float type. Non IANA standard.
 *     Z_ENCODING_PREFIX_APP_XML: ``application/xml``. XML.
 *     Z_ENCODING_PREFIX_APP_XHTML_XML: ``application/xhtml+xml``. XHTML.
 *     Z_ENCODING_PREFIX_APP_X_WWW_FORM_URLENCODED: ``application/x-www-form-urlencoded``. The keys and values are
 *         encoded in key-value tuples separated by '&', with a '=' between the key and the value.
 *      Z_ENCODING_PREFIX_TEXT_JSON: Text JSON. Non IANA standard. Z_ENCODING_PREFIX_TEXT_HTML: ``text/html``. HyperText
 *         Markup Language (HTML). Z_ENCODING_PREFIX_TEXT_XML: ``text/xml``. `Application/xml` is recommended as of RFC
 *         7303 (section 4.1), but `text/xml` is still used sometimes. Z_ENCODING_PREFIX_TEXT_CSS: ``text/css``.
 *         Cascading Style Sheets (CSS). Z_ENCODING_PREFIX_TEXT_CSV: ``text/csv``. Comma-separated values (CSV).
 *     Z_ENCODING_PREFIX_TEXT_JAVASCRIPT: ``text/javascript``. JavaScript.
 *     Z_ENCODING_PREFIX_IMAGE_JPEG: ``image/jpeg``. JPEG images.
 *     Z_ENCODING_PREFIX_IMAGE_PNG: ``image/png``. Portable Network Graphics.
 *     Z_ENCODING_PREFIX_IMAGE_GIF: ``image/gif``. Graphics Interchange Format (GIF).
 */
typedef enum {
    Z_ENCODING_PREFIX_EMPTY = 0,
    Z_ENCODING_PREFIX_APP_OCTET_STREAM = 1,
    Z_ENCODING_PREFIX_APP_CUSTOM = 2,  // non iana standard
    Z_ENCODING_PREFIX_TEXT_PLAIN = 3,
    Z_ENCODING_PREFIX_APP_PROPERTIES = 4,  // non iana standard
    Z_ENCODING_PREFIX_APP_JSON = 5,        // if not readable from casual users
    Z_ENCODING_PREFIX_APP_SQL = 6,
    Z_ENCODING_PREFIX_APP_INTEGER = 7,  // non iana standard
    Z_ENCODING_PREFIX_APP_FLOAT = 8,    // non iana standard
    Z_ENCODING_PREFIX_APP_XML = 9,      // if not readable from casual users (RFC 3023, section 3)
    Z_ENCODING_PREFIX_APP_XHTML_XML = 10,
    Z_ENCODING_PREFIX_APP_X_WWW_FORM_URLENCODED = 11,
    Z_ENCODING_PREFIX_TEXT_JSON = 12,  // non iana standard - if readable from casual users
    Z_ENCODING_PREFIX_TEXT_HTML = 13,
    Z_ENCODING_PREFIX_TEXT_XML = 14,  // if readable from casual users (RFC 3023, section 3)
    Z_ENCODING_PREFIX_TEXT_CSS = 15,
    Z_ENCODING_PREFIX_TEXT_CSV = 16,
    Z_ENCODING_PREFIX_TEXT_JAVASCRIPT = 17,
    Z_ENCODING_PREFIX_IMAGE_JPEG = 18,
    Z_ENCODING_PREFIX_IMAGE_PNG = 19,
    Z_ENCODING_PREFIX_IMAGE_GIF = 20
} z_encoding_prefix_t;
#define Z_ENCODING_PREFIX_DEFAULT Z_ENCODING_PREFIX_EMPTY

/**
 * Consolidation mode values.
 *
 * Enumerators:
 *     Z_CONSOLIDATION_MODE_AUTO: Let Zenoh decide the best consolidation mode depending on the query selector.
 *     Z_CONSOLIDATION_MODE_NONE: No consolidation is applied. Replies may come in any order and any number.
 *     Z_CONSOLIDATION_MODE_MONOTONIC: It guarantees that any reply for a given key expression will be monotonic in time
 *         w.r.t. the previous received replies for the same key expression. I.e., for the same key expression multiple
 *         replies may be received. It is guaranteed that two replies received at t1 and t2 will have timestamp
 *         ts2 > ts1. It optimizes latency.
 *     Z_CONSOLIDATION_MODE_LATEST: It guarantees unicity of replies for the same key expression.
 *         It optimizes bandwidth.
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
 *     Z_RELIABILITY_BEST_EFFORT: Defines reliability as ``BEST_EFFORT``
 *     Z_RELIABILITY_RELIABLE: Defines reliability as ``RELIABLE``
 */
typedef enum { Z_RELIABILITY_BEST_EFFORT = 0, Z_RELIABILITY_RELIABLE = 1 } z_reliability_t;
#define Z_RELIABILITY_DEFAULT Z_RELIABILITY_RELIABLE

/**
 * Reply tag values.
 *
 * Enumerators:
 *     Z_REPLY_TAG_DATA: Tag identifying that the reply contains some data.
 *     Z_REPLY_TAG_FINAL: Tag identifying that the reply does not contain any data and that there will be no more
 *         replies for this query.
 */
typedef enum { Z_REPLY_TAG_DATA = 0, Z_REPLY_TAG_FINAL = 1 } z_reply_tag_t;

/**
 * Congestion control values.
 *
 * Enumerators:
 *     Z_CONGESTION_CONTROL_BLOCK: Defines congestion control as ``BLOCK``. Messages are not dropped in case of
 *         congestion control.
 *     Z_CONGESTION_CONTROL_DROP: Defines congestion control as ``DROP``. Messages are dropped in case
 *         of congestion control.
 */
typedef enum { Z_CONGESTION_CONTROL_BLOCK = 0, Z_CONGESTION_CONTROL_DROP = 1 } z_congestion_control_t;
#define Z_CONGESTION_CONTROL_DEFAULT Z_CONGESTION_CONTROL_BLOCK

/**
 * Priority of Zenoh messages values.
 *
 * Enumerators:
 *     _Z_PRIORITY_CONTROL: Priority for ``Control`` messages.
 *     Z_PRIORITY_REAL_TIME: Priority for ``RealTime`` messages.
 *     Z_PRIORITY_INTERACTIVE_HIGH: Highest priority for ``Interactive`` messages.
 *     Z_PRIORITY_INTERACTIVE_LOW: Lowest priority for ``Interactive`` messages.
 *     Z_PRIORITY_DATA_HIGH: Highest priority for ``Data`` messages.
 *     Z_PRIORITY_DATA: Default priority for ``Data`` messages.
 *     Z_PRIORITY_DATA_LOW: Lowest priority for ``Data`` messages.
 *     Z_PRIORITY_BACKGROUND: Priority for ``Background traffic`` messages.
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
 * Subscription mode values.
 *
 * Enumerators:
 *     Z_SUBMODE_PUSH: Defines the subscription with a push paradigm.
 *     Z_SUBMODE_PULL: Defines the subscription with a pull paradigm.
 */
typedef enum { Z_SUBMODE_PUSH = 0, Z_SUBMODE_PULL = 1 } z_submode_t;
#define Z_SUBMODE_DEFAULT Z_SUBMODE_PUSH

/**
 * Query target values.
 *
 * Enumerators:
 *     Z_QUERY_TARGET_BEST_MATCHING: The nearest complete queryable if any else all matching queryables.
 *     Z_QUERY_TARGET_ALL: All matching queryables.
 *     Z_QUERY_TARGET_ALL_COMPLETE: A set of complete queryables.
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
