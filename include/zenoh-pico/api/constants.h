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
 * What values.
 *
 * Enumerators:
 *     Z_WHAT_ROUTER: Router.
 *     Z_WHAT_PEER: Peer.
 *     Z_WHAT_CLIENT: Client.
 */
typedef enum {
    Z_WHAT_ROUTER = 0x01,  // Router
    Z_WHAT_PEER = 0x02,    // Peer
    Z_WHAT_CLIENT = 0x03   // Client
} z_what_t;

/**
 * Whatami values, defined as a bitmask.
 *
 * Enumerators:
 *     Z_WHATAMI_ROUTER: Bitmask to filter Zenoh routers.
 *     Z_WHATAMI_PEER: Bitmask to filter for Zenoh peers.
 *     Z_WHATAMI_CLIENT: Bitmask to filter for Zenoh clients.
 */
typedef enum { Z_WHATAMI_ROUTER = 0x00, Z_WHATAMI_PEER = 0x01, Z_WHATAMI_CLIENT = 0x02 } z_whatami_t;
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
 * Default encoding values used by Zenoh.
 *
 * An encoding has a similar role to Content-type in HTTP: it indicates, when present, how data should be interpreted by
 * the application.
 *
 * Please note the Zenoh protocol does not impose any encoding value nor it operates on it.
 * It can be seen as some optional metadata that is carried over by Zenoh in such a way the application may perform
 * different operations depending on the encoding value.
 *
 * A set of associated constants are provided to cover the most common encodings for user convenience.
 * This is particularly useful in helping Zenoh to perform additional network optimizations.
 */
typedef enum {
    Z_ENCODING_ID_BYTES = 0,             // Primitives types supported in all Zenoh bindings, just some bytes.
    Z_ENCODING_ID_INT = 1,               // A VLE-encoded signed little-endian integer.
    Z_ENCODING_ID_UINT = 2,              // A VLE-encoded little-endian unsigned integer.
    Z_ENCODING_ID_FLOAT = 3,             // A VLE-encoded float.
    Z_ENCODING_ID_BOOL = 4,              // A boolean.
    Z_ENCODING_ID_STRING = 5,            // A UTF-8 string.
    Z_ENCODING_ID_ERROR = 6,             // A zenoh error.
    Z_ENCODING_ID_APP_OCTET_STREAM = 7,  // An application-specific stream of bytes.
    Z_ENCODING_ID_TEXT_PLAIN = 8,        // A textual file.
    Z_ENCODING_ID_APP_JSON = 9,          // JSON data intended to be consumed by an application.
    Z_ENCODING_ID_TEXT_JSON = 10,        // JSON data intended to be human readable.
    Z_ENCODING_ID_APP_CDR = 11,          // A Common Data Representation (CDR)-encoded data.
    Z_ENCODING_ID_APP_CBOR = 12,         // A Concise Binary Object Representation (CBOR)-encoded data.
    Z_ENCODING_ID_APP_YAML = 13,         // YAML data intended to be consumed by an application.
    Z_ENCODING_ID_TEXT_YAML = 14,        // YAML data intended to be human readable.
    Z_ENCODING_ID_TEXT_JSON5 = 15,       // JSON5 encoded data that are human readable.
    Z_ENCODING_ID_APP_PYTHON_SERIALIZED_OBJECT = 16,  // A Python object serialized using pickle.
    Z_ENCODING_ID_APP_PROTOBUF = 17,                  // An application-specific protobuf-encoded data.
    Z_ENCODING_ID_APP_JAVA_SERIALIZED_OBJECT = 18,    // A Java serialized object.
    Z_ENCODING_ID_APP_OPENMETRICS_TEXT = 19,          // An openmetrics data.
    Z_ENCODING_ID_IMAGE_PNG = 20,                     // A Portable Network Graphics (PNG) image.
    Z_ENCODING_ID_IMAGE_JPEG = 21,                    // A Joint Photographic Experts Group (JPEG) image.
    Z_ENCODING_ID_IMAGE_GIF = 22,                     // A Graphics Interchange Format (GIF) image.
    Z_ENCODING_ID_IMAGE_BMP = 23,                     // A BitMap (BMP) image.
    Z_ENCODING_ID_IMAGE_WEBP = 24,                    // A Web Portable (WebP) image.
    Z_ENCODING_ID_APP_XML = 25,                       // An XML file intended to be consumed by an application.
    Z_ENCODING_ID_APP_X_WWW_FORM_URLENCODED = 26,     // An encoded a list of tuples.
    Z_ENCODING_ID_TEXT_HTML = 27,                     // An HTML file.
    Z_ENCODING_ID_TEXT_XML = 28,                      // An XML file that is human readable.
    Z_ENCODING_ID_TEXT_CSS = 29,                      // A CSS file.
    Z_ENCODING_ID_TEXT_JAVASCRIPT = 30,               // A JavaScript file.
    Z_ENCODING_ID_TEXT_MARKDOWN = 31,                 // A MarkDown file.
    Z_ENCODING_ID_TEXT_CSV = 32,                      // A CSV file.
    Z_ENCODING_ID_APP_SQL = 33,                       // An application-specific SQL query.
    Z_ENCODING_ID_APP_COAP_PAYLOAD = 34,              // Constrained Application Protocol (CoAP) data.
    Z_ENCODING_ID_APP_JSON_PATCH_JSON = 35,           // Defines a JSON document structure.
    Z_ENCODING_ID_APP_JSON_SEQ = 36,                  // A JSON text sequence.
    Z_ENCODING_ID_APP_JSONPATH = 37,                  // A JSONPath defines a string syntax.
    Z_ENCODING_ID_APP_JWT = 38,                       // A JSON Web Token (JWT).
    Z_ENCODING_ID_APP_MP4 = 39,                       // An application-specific MPEG-4 encoded data.
    Z_ENCODING_ID_APP_SOAP_XML = 40,                  // A SOAP 1.2 message serialized as XML 1.0.
    Z_ENCODING_ID_APP_YANG = 41,                      // A YANG-encoded data.
    Z_ENCODING_ID_AUDIO_AAC = 42,                     // A MPEG-4 Advanced Audio Coding (AAC) media.
    Z_ENCODING_ID_AUDIO_FLAC = 43,                    // A Free Lossless Audio Codec (FLAC) media.
    Z_ENCODING_ID_AUDIO_MP4 = 44,                     // An audio codec defined in MPEG-4.
    Z_ENCODING_ID_AUDIO_OGG = 45,                     // An Ogg-encapsulated audio stream.
    Z_ENCODING_ID_AUDIO_VORBIS = 46,                  // A Vorbis-encoded audio stream.
    Z_ENCODING_ID_VIDEO_H261 = 47,                    // A h261-encoded video stream.
    Z_ENCODING_ID_VIDEO_H263 = 48,                    // A h263-encoded video stream.
    Z_ENCODING_ID_VIDEO_H264 = 49,                    // A h264-encoded video stream.
    Z_ENCODING_ID_VIDEO_H265 = 50,                    // A h265-encoded video stream.
    Z_ENCODING_ID_VIDEO_H266 = 51,                    // A h266-encoded video stream.
    Z_ENCODING_ID_VIDEO_MP4 = 52,                     // A video codec defined in MPEG-4.
    Z_ENCODING_ID_VIDEO_OGG = 53,                     // An Ogg-encapsulated video stream.
    Z_ENCODING_ID_VIDEO_RAW = 54,                     // An uncompressed, studio-quality video stream.
    Z_ENCODING_ID_VIDEO_VP8 = 55,                     // A VP8-encoded video stream.
    Z_ENCODING_ID_VIDEO_VP9 = 56                      // A VP9-encoded video stream.
} z_encoding_id_t;

#define Z_ENCODING_ID_DEFAULT Z_ENCODING_ID_BYTES

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
typedef enum { Z_RELIABILITY_BEST_EFFORT = 1, Z_RELIABILITY_RELIABLE = 0 } z_reliability_t;
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
