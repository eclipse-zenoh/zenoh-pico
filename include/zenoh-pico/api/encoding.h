//
// Copyright (c) 2024 ZettaScale Technology
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

#ifndef ZENOH_PICO_API_ENCODING_H
#define ZENOH_PICO_API_ENCODING_H

#include "zenoh-pico/api/types.h"

#ifdef __cplusplus
extern "C" {
#endif
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
 * This is particularly useful in helping Zenoh to perform additional wire-level optimizations.
 *
 * Register your encoding metadata from a string with :c:func:`z_encoding_from_str`. To get the optimization, you need
 * Z_FEATURE_ENCODING_VALUES to 1 and your string should follow the format: "<constant>;<optional additional data>"
 *
 * E.g: "text/plain;utf8"
 *
 * Or you can set the value to the constants directly with this list of constants:
 */

#if Z_FEATURE_ENCODING_VALUES == 1

extern const z_owned_encoding_t ZP_ENCODING_ZENOH_BYTES;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT8;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT16;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT32;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT64;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT128;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT8;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT16;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT32;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT64;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT128;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_FLOAT32;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_FLOAT64;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_BOOL;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_STRING;
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_ERROR;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_OCTET_STREAM;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_PLAIN;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_JSON;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_JSON;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_CDR;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_CBOR;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_YAML;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_YAML;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_JSON5;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_PYTHON_SERIALIZED_OBJECT;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_PROTOBUF;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_JAVA_SERIALIZED_OBJECT;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_OPENMETRICS_TEXT;
extern const z_owned_encoding_t ZP_ENCODING_IMAGE_PNG;
extern const z_owned_encoding_t ZP_ENCODING_IMAGE_JPEG;
extern const z_owned_encoding_t ZP_ENCODING_IMAGE_GIF;
extern const z_owned_encoding_t ZP_ENCODING_IMAGE_BMP;
extern const z_owned_encoding_t ZP_ENCODING_IMAGE_WEBP;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_XML;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_X_WWW_FORM_URLENCODED;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_HTML;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_XML;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_CSS;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_JAVASCRIPT;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_MARKDOWN;
extern const z_owned_encoding_t ZP_ENCODING_TEXT_CSV;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_SQL;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_COAP_PAYLOAD;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_JSON_PATCH_JSON;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_JSON_SEQ;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_JSONPATH;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_JWT;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_MP4;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_SOAP_XML;
extern const z_owned_encoding_t ZP_ENCODING_APPLICATION_YANG;
extern const z_owned_encoding_t ZP_ENCODING_AUDIO_AAC;
extern const z_owned_encoding_t ZP_ENCODING_AUDIO_FLAC;
extern const z_owned_encoding_t ZP_ENCODING_AUDIO_MP4;
extern const z_owned_encoding_t ZP_ENCODING_AUDIO_OGG;
extern const z_owned_encoding_t ZP_ENCODING_AUDIO_VORBIS;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_H261;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_H263;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_H264;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_H265;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_H266;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_MP4;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_OGG;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_RAW;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_VP8;
extern const z_owned_encoding_t ZP_ENCODING_VIDEO_VP9;

// - Below are Primitives types, supported in all Zenoh bindings
/**
 * Just some bytes.
 *
 * Constant alias for string: `"zenoh/bytes"`.
 *
 * Usually used for types: `uint8_t[]`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_bytes(void) { return &ZP_ENCODING_ZENOH_BYTES._val; }

/**
 * A VLE-encoded signed little-endian 8bit integer. Binary representation uses two's complement.
 * Constant alias for string: `"zenoh/int8"`.
 *
 * Usually used for types: `int8_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_int8(void) { return &ZP_ENCODING_ZENOH_INT8._val; }

/**
 * A VLE-encoded signed little-endian 16bit integer. Binary representation uses two's complement.
 * Constant alias for string: `"zenoh/int16"`.
 *
 * Usually used for types: `int16_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_int16(void) { return &ZP_ENCODING_ZENOH_INT16._val; }

/**
 * A VLE-encoded signed little-endian 32bit integer. Binary representation uses two's complement.
 * Constant alias for string: `"zenoh/int32"`.
 *
 * Usually used for types: `int32_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_int32(void) { return &ZP_ENCODING_ZENOH_INT32._val; }

/**
 * A VLE-encoded signed little-endian 64bit integer. Binary representation uses two's complement.
 * Constant alias for string: `"zenoh/int64"`.
 *
 * Usually used for types: `int64_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_int64(void) { return &ZP_ENCODING_ZENOH_INT64._val; }

/**
 * A VLE-encoded signed little-endian 128bit integer. Binary representation uses two's complement.
 * Constant alias for string: `"zenoh/int128"`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_int128(void) { return &ZP_ENCODING_ZENOH_INT128._val; }

/**
 * A VLE-encoded unsigned little-endian 8bit integer.
 * Constant alias for string: `"zenoh/uint8"`.
 *
 * Usually used for types: `uint8_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_uint8(void) { return &ZP_ENCODING_ZENOH_UINT8._val; }

/**
 * A VLE-encoded unsigned little-endian 16bit integer.
 * Constant alias for string: `"zenoh/uint16"`.
 *
 * Usually used for types: `uint16_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_uint16(void) { return &ZP_ENCODING_ZENOH_UINT16._val; }

/**
 * A VLE-encoded unsigned little-endian 32bit integer.
 * Constant alias for string: `"zenoh/uint32"`.
 *
 * Usually used for types: `uint32_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_uint32(void) { return &ZP_ENCODING_ZENOH_UINT32._val; }

/**
 * A VLE-encoded unsigned little-endian 64bit integer.
 * Constant alias for string: `"zenoh/uint64"`.
 *
 * Usually used for types: `uint64_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_uint64(void) { return &ZP_ENCODING_ZENOH_UINT64._val; }

/**
 * A VLE-encoded unsigned little-endian 128bit integer.
 * Constant alias for string: `"zenoh/uint128"`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_uint128(void) { return &ZP_ENCODING_ZENOH_UINT128._val; }

/**
 * A VLE-encoded 32bit float. Binary representation uses *IEEE 754-2008* *binary32*.
 * Constant alias for string: `"zenoh/float32"`.
 *
 * Usually used for types: `float`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_float32(void) { return &ZP_ENCODING_ZENOH_FLOAT32._val; }

/**
 * A VLE-encoded 64bit float. Binary representation uses *IEEE 754-2008* *binary64*.
 * Constant alias for string: `"zenoh/float64"`.
 *
 * Usually used for types: `double`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_float64(void) { return &ZP_ENCODING_ZENOH_FLOAT64._val; }

/**
 * A boolean. `0` is `false`, `1` is `true`. Other values are invalid.
 * Constant alias for string: `"zenoh/bool"`.
 *
 * Usually used for types: `bool`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_bool(void) { return &ZP_ENCODING_ZENOH_BOOL._val; }

/**
 * A UTF-8 string.
 * Constant alias for string: `"zenoh/string"`.
 *
 * Usually used for types: `char[]`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_string(void) { return &ZP_ENCODING_ZENOH_STRING._val; }

/**
 * A zenoh error.
 * Constant alias for string: `"zenoh/error"`.
 *
 * Usually used for types: `z_reply_err_t`.
 */
static inline const z_loaned_encoding_t *z_encoding_zenoh_error(void) { return &ZP_ENCODING_ZENOH_ERROR._val; }

/**
 * An application-specific stream of bytes.
 * Constant alias for string: `"application/octet-stream"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_octet_stream(void) {
    return &ZP_ENCODING_APPLICATION_OCTET_STREAM._val;
}

/**
 * A textual file.
 * Constant alias for string: `"text/plain"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_plain(void) { return &ZP_ENCODING_TEXT_PLAIN._val; }

/**
 * JSON data intended to be consumed by an application.
 * Constant alias for string: `"application/json"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_json(void) {
    return &ZP_ENCODING_APPLICATION_JSON._val;
}

/**
 * JSON data intended to be human readable.
 * Constant alias for string: `"text/json"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_json(void) { return &ZP_ENCODING_TEXT_JSON._val; }

/**
 * A Common Data Representation (CDR)-encoded data.
 * Constant alias for string: `"application/cdr"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_cdr(void) { return &ZP_ENCODING_APPLICATION_CDR._val; }

/**
 * A Concise Binary Object Representation (CBOR)-encoded data.
 * Constant alias for string: `"application/cbor"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_cbor(void) {
    return &ZP_ENCODING_APPLICATION_CBOR._val;
}

/**
 * YAML data intended to be consumed by an application.
 * Constant alias for string: `"application/yaml"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_yaml(void) {
    return &ZP_ENCODING_APPLICATION_YAML._val;
}

/**
 * YAML data intended to be human readable.
 * Constant alias for string: `"text/yaml"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_yaml(void) { return &ZP_ENCODING_TEXT_YAML._val; }

/**
 * JSON5 encoded data that are human readable.
 * Constant alias for string: `"text/json5"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_json5(void) { return &ZP_ENCODING_TEXT_JSON5._val; }

/**
 * A Python object serialized using [pickle](https://docs.python.org/3/library/pickle.html).
 * Constant alias for string: `"application/python-serialized-object"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_python_serialized_object(void) {
    return &ZP_ENCODING_APPLICATION_PYTHON_SERIALIZED_OBJECT._val;
}

/**
 * An application-specific protobuf-encoded data.
 * Constant alias for string: `"application/protobuf"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_protobuf(void) {
    return &ZP_ENCODING_APPLICATION_PROTOBUF._val;
}

/**
 * A Java serialized object.
 * Constant alias for string: `"application/java-serialized-object"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_java_serialized_object(void) {
    return &ZP_ENCODING_APPLICATION_JAVA_SERIALIZED_OBJECT._val;
}

/**
 * An [openmetrics](https://github.com/OpenObservability/OpenMetrics) data, commonly used by
 * [Prometheus](https://prometheus.io/).
 * Constant alias for string: `"application/openmetrics-text"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_openmetrics_text(void) {
    return &ZP_ENCODING_APPLICATION_OPENMETRICS_TEXT._val;
}

/**
 * A Portable Network Graphics (PNG) image.
 * Constant alias for string: `"image/png"`.
 */
static inline const z_loaned_encoding_t *z_encoding_image_png(void) { return &ZP_ENCODING_IMAGE_PNG._val; }

/**
 * A Joint Photographic Experts Group (JPEG) image.
 * Constant alias for string: `"image/jpeg"`.
 */
static inline const z_loaned_encoding_t *z_encoding_image_jpeg(void) { return &ZP_ENCODING_IMAGE_JPEG._val; }

/**
 * A Graphics Interchange Format (GIF) image.
 * Constant alias for string: `"image/gif"`.
 */
static inline const z_loaned_encoding_t *z_encoding_image_gif(void) { return &ZP_ENCODING_IMAGE_GIF._val; }

/**
 * A BitMap (BMP) image.
 * Constant alias for string: `"image/bmp"`.
 */
static inline const z_loaned_encoding_t *z_encoding_image_bmp(void) { return &ZP_ENCODING_IMAGE_BMP._val; }

/**
 * A Web Portable (WebP) image.
 * Constant alias for string: `"image/webp"`.
 */
static inline const z_loaned_encoding_t *z_encoding_image_webp(void) { return &ZP_ENCODING_IMAGE_WEBP._val; }

/**
 * An XML file intended to be consumed by an application.
 * Constant alias for string: `"application/xml"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_xml(void) { return &ZP_ENCODING_APPLICATION_XML._val; }

/**
 * An encoded list of tuples, each consisting of a name and a value.
 * Constant alias for string: `"application/x-www-form-urlencoded"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_x_www_form_urlencoded(void) {
    return &ZP_ENCODING_APPLICATION_X_WWW_FORM_URLENCODED._val;
}

/**
 * An HTML file.
 * Constant alias for string: `"text/html"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_html(void) { return &ZP_ENCODING_TEXT_HTML._val; }

/**
 * An XML file that is human-readable.
 * Constant alias for string: `"text/xml"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_xml(void) { return &ZP_ENCODING_TEXT_XML._val; }

/**
 * A CSS file.
 * Constant alias for string: `"text/css"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_css(void) { return &ZP_ENCODING_TEXT_CSS._val; }

/**
 * A JavaScript file.
 * Constant alias for string: `"text/javascript"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_javascript(void) { return &ZP_ENCODING_TEXT_JAVASCRIPT._val; }

/**
 * A Markdown file.
 * Constant alias for string: `"text/markdown"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_markdown(void) { return &ZP_ENCODING_TEXT_MARKDOWN._val; }

/**
 * A CSV file.
 * Constant alias for string: `"text/csv"`.
 */
static inline const z_loaned_encoding_t *z_encoding_text_csv(void) { return &ZP_ENCODING_TEXT_CSV._val; }

/**
 * An application-specific SQL query.
 * Constant alias for string: `"application/sql"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_sql(void) { return &ZP_ENCODING_APPLICATION_SQL._val; }

/**
 * Constrained Application Protocol (CoAP) data intended for CoAP-to-HTTP and HTTP-to-CoAP proxies.
 * Constant alias for string: `"application/coap-payload"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_coap_payload(void) {
    return &ZP_ENCODING_APPLICATION_COAP_PAYLOAD._val;
}

/**
 * Defines a JSON document structure for expressing a sequence of operations to apply to a JSON document.
 * Constant alias for string: `"application/json-patch+json"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_json_patch_json(void) {
    return &ZP_ENCODING_APPLICATION_JSON_PATCH_JSON._val;
}

/**
 * A JSON text sequence consists of any number of JSON texts, all encoded in UTF-8.
 * Constant alias for string: `"application/json-seq"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_json_seq(void) {
    return &ZP_ENCODING_APPLICATION_JSON_SEQ._val;
}

/**
 * A JSONPath defines a string syntax for selecting and extracting JSON values from within a given JSON value.
 * Constant alias for string: `"application/jsonpath"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_jsonpath(void) {
    return &ZP_ENCODING_APPLICATION_JSONPATH._val;
}

/**
 * A JSON Web Token (JWT).
 * Constant alias for string: `"application/jwt"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_jwt(void) { return &ZP_ENCODING_APPLICATION_JWT._val; }

/**
 * An application-specific MPEG-4 encoded data, either audio or video.
 * Constant alias for string: `"application/mp4"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_mp4(void) { return &ZP_ENCODING_APPLICATION_MP4._val; }

/**
 * A SOAP 1.2 message serialized as XML 1.0.
 * Constant alias for string: `"application/soap+xml"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_soap_xml(void) {
    return &ZP_ENCODING_APPLICATION_SOAP_XML._val;
}

/**
 * A YANG-encoded data commonly used by the Network Configuration Protocol (NETCONF).
 * Constant alias for string: `"application/yang"`.
 */
static inline const z_loaned_encoding_t *z_encoding_application_yang(void) {
    return &ZP_ENCODING_APPLICATION_YANG._val;
}

/**
 * A MPEG-4 Advanced Audio Coding (AAC) media.
 * Constant alias for string: `"audio/aac"`.
 */
static inline const z_loaned_encoding_t *z_encoding_audio_aac(void) { return &ZP_ENCODING_AUDIO_AAC._val; }

/**
 * A Free Lossless Audio Codec (FLAC) media.
 * Constant alias for string: `"audio/flac"`.
 */
static inline const z_loaned_encoding_t *z_encoding_audio_flac(void) { return &ZP_ENCODING_AUDIO_FLAC._val; }

/**
 * An audio codec defined in MPEG-1, MPEG-2, MPEG-4, or registered at the MP4 registration authority.
 * Constant alias for string: `"audio/mp4"`.
 */
static inline const z_loaned_encoding_t *z_encoding_audio_mp4(void) { return &ZP_ENCODING_AUDIO_MP4._val; }

/**
 * An Ogg-encapsulated audio stream.
 * Constant alias for string: `"audio/ogg"`.
 */
static inline const z_loaned_encoding_t *z_encoding_audio_ogg(void) { return &ZP_ENCODING_AUDIO_OGG._val; }

/**
 * A Vorbis-encoded audio stream.
 * Constant alias for string: `"audio/vorbis"`.
 */
static inline const z_loaned_encoding_t *z_encoding_audio_vorbis(void) { return &ZP_ENCODING_AUDIO_VORBIS._val; }

/**
 * A h261-encoded video stream.
 * Constant alias for string: `"video/h261"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_h261(void) { return &ZP_ENCODING_VIDEO_H261._val; }

/**
 * A h263-encoded video stream.
 * Constant alias for string: `"video/h263"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_h263(void) { return &ZP_ENCODING_VIDEO_H263._val; }

/**
 * A h264-encoded video stream.
 * Constant alias for string: `"video/h264"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_h264(void) { return &ZP_ENCODING_VIDEO_H264._val; }

/**
 * A h265-encoded video stream.
 * Constant alias for string: `"video/h265"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_h265(void) { return &ZP_ENCODING_VIDEO_H265._val; }

/**
 * A h266-encoded video stream.
 * Constant alias for string: `"video/h266"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_h266(void) { return &ZP_ENCODING_VIDEO_H266._val; }

/**
 * A video codec defined in MPEG-1, MPEG-2, MPEG-4, or registered at the MP4 registration authority.
 * Constant alias for string: `"video/mp4"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_mp4(void) { return &ZP_ENCODING_VIDEO_MP4._val; }

/**
 * An Ogg-encapsulated video stream.
 * Constant alias for string: `"video/ogg"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_ogg(void) { return &ZP_ENCODING_VIDEO_OGG._val; }

/**
 * An uncompressed, studio-quality video stream.
 * Constant alias for string: `"video/raw"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_raw(void) { return &ZP_ENCODING_VIDEO_RAW._val; }

/**
 * A VP8-encoded video stream.
 * Constant alias for string: `"video/vp8"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_vp8(void) { return &ZP_ENCODING_VIDEO_VP8._val; }

/**
 * A VP9-encoded video stream.
 * Constant alias for string: `"video/vp9"`.
 */
static inline const z_loaned_encoding_t *z_encoding_video_vp9(void) { return &ZP_ENCODING_VIDEO_VP9._val; }

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_API_ENCODING_H */
