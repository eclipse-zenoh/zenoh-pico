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
// - Below are Primitives types, supported in all Zenoh bindings
// Just some bytes.
//
// Constant alias for string: `"zenoh/bytes"`.
//
// Usually used for types: `uint8_t[]`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_BYTES;
const z_loaned_encoding_t *z_encoding_zenoh_bytes(void);

// A VLE-encoded signed little-endian 8bit integer. Binary representation uses two's complement.
// Constant alias for string: `"zenoh/int8"`.
//
// Usually used for types: `int8_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT8;
const z_loaned_encoding_t *z_encoding_zenoh_int8(void);

// A VLE-encoded signed little-endian 16bit integer. Binary representation uses two's complement.
//
// Constant alias for string: `"zenoh/int16"`.
//
// Usually used for types: `int16_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT16;
const z_loaned_encoding_t *z_encoding_zenoh_int16(void);

// A VLE-encoded signed little-endian 32bit integer. Binary representation uses two's complement.
//
// Constant alias for string: `"zenoh/int32"`.
//
// Usually used for types: `int32_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT32;
const z_loaned_encoding_t *z_encoding_zenoh_int32(void);

// A VLE-encoded signed little-endian 64bit integer. Binary representation uses two's complement.
//
// Constant alias for string: `"zenoh/int64"`.
//
// Usually used for types: `int64_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT64;
const z_loaned_encoding_t *z_encoding_zenoh_int64(void);

// A VLE-encoded signed little-endian 128bit integer. Binary representation uses two's complement.
//
// Constant alias for string: `"zenoh/int128"`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_INT128;
const z_loaned_encoding_t *z_encoding_zenoh_int128(void);

// A VLE-encoded unsigned little-endian 8bit integer.
//
// Constant alias for string: `"zenoh/uint8"`.
//
// Usually used for types: `uint8_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT8;
const z_loaned_encoding_t *z_encoding_zenoh_uint8(void);

// A VLE-encoded unsigned little-endian 16bit integer.
//
// Constant alias for string: `"zenoh/uint16"`.
//
// Usually used for types: `uint16_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT16;
const z_loaned_encoding_t *z_encoding_zenoh_uint16(void);
// A VLE-encoded unsigned little-endian 32bit integer.
//
// Constant alias for string: `"zenoh/uint32"`.
//
// Usually used for types: `uint32_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT32;
const z_loaned_encoding_t *z_encoding_zenoh_uint32(void);

// A VLE-encoded unsigned little-endian 64bit integer.
//
// Constant alias for string: `"zenoh/uint64"`.
//
// Usually used for types: `uint64_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT64;
const z_loaned_encoding_t *z_encoding_zenoh_uint64(void);

// A VLE-encoded unsigned little-endian 128bit integer.
//
// Constant alias for string: `"zenoh/uint128"`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_UINT128;
const z_loaned_encoding_t *z_encoding_zenoh_uint128(void);

// A VLE-encoded 32bit float. Binary representation uses *IEEE 754-2008* *binary32* .
//
// Constant alias for string: `"zenoh/float32"`.
//
// Usually used for types: `float`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_FLOAT32;
const z_loaned_encoding_t *z_encoding_zenoh_float32(void);

// A VLE-encoded 64bit float. Binary representation uses *IEEE 754-2008* *binary64*.
//
// Constant alias for string: `"zenoh/float64"`.
//
// Usually used for types: `double`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_FLOAT64;
const z_loaned_encoding_t *z_encoding_zenoh_float64(void);

// A boolean. `0` is `false`, `1` is `true`. Other values are invalid.
//
// Constant alias for string: `"zenoh/bool"`.
//
// Usually used for types: `bool`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_BOOL;
const z_loaned_encoding_t *z_encoding_zenoh_bool(void);

// A UTF-8 string.
//
// Constant alias for string: `"zenoh/string"`.
//
// Usually used for types: `char[]`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_STRING;
const z_loaned_encoding_t *z_encoding_zenoh_string(void);
// A zenoh error.
//
// Constant alias for string: `"zenoh/error"`.
//
// Usually used for types: `z_reply_err_t`.
extern const z_owned_encoding_t ZP_ENCODING_ZENOH_ERROR;
const z_loaned_encoding_t *z_encoding_zenoh_error(void);

// - Below are Advanced types, may be supported in some of the Zenoh bindings.
// An application-specific stream of bytes.
//
// Constant alias for string: `"application/octet-stream"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_OCTET_STREAM;
const z_loaned_encoding_t *z_encoding_application_octet_stream(void);

// A textual file.
//
// Constant alias for string: `"text/plain"`.
extern const z_owned_encoding_t ENCODING_TEXT_PLAIN;
const z_loaned_encoding_t *z_encoding_text_plain(void);

// JSON data intended to be consumed by an application.
//
// Constant alias for string: `"application/json"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JSON;
const z_loaned_encoding_t *z_encoding_application_json(void);

// JSON data intended to be human readable.
//
// Constant alias for string: `"text/json"`.
extern const z_owned_encoding_t ENCODING_TEXT_JSON;
const z_loaned_encoding_t *z_encoding_text_json(void);

// A Common Data Representation (CDR)-encoded data.
//
// Constant alias for string: `"application/cdr"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_CDR;
const z_loaned_encoding_t *z_encoding_application_cdr(void);

// A Concise Binary Object Representation (CBOR)-encoded data.
//
// Constant alias for string: `"application/cbor"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_CBOR;
const z_loaned_encoding_t *z_encoding_application_cbor(void);

// YAML data intended to be consumed by an application.
//
// Constant alias for string: `"application/yaml"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_YAML;
const z_loaned_encoding_t *z_encoding_application_yaml(void);

// YAML data intended to be human readable.
//
// Constant alias for string: `"text/yaml"`.
extern const z_owned_encoding_t ENCODING_TEXT_YAML;
const z_loaned_encoding_t *z_encoding_text_yaml(void);

// JSON5 encoded data that are human readable.
//
// Constant alias for string: `"text/json5"`.
extern const z_owned_encoding_t ENCODING_TEXT_JSON5;
const z_loaned_encoding_t *z_encoding_text_json5(void);

// A Python object serialized using [pickle](https://docs.python.org/3/library/pickle.html).
//
// Constant alias for string: `"application/python-serialized-object"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_PYTHON_SERIALIZED_OBJECT;
const z_loaned_encoding_t *z_encoding_application_python_serialized_object(void);

// An application-specific protobuf-encoded data.
//
// Constant alias for string: `"application/protobuf"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_PROTOBUF;
const z_loaned_encoding_t *z_encoding_application_protobuf(void);

// A Java serialized object.
//
// Constant alias for string: `"application/java-serialized-object"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JAVA_SERIALIZED_OBJECT;
const z_loaned_encoding_t *z_encoding_application_java_serialized_object(void);

// An [openmetrics](https://github.com/OpenObservability/OpenMetrics) data, common used by
// [Prometheus](https://prometheus.io/).
//
// Constant alias for string: `"application/openmetrics-text"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_OPENMETRICS_TEXT;
const z_loaned_encoding_t *z_encoding_application_openmetrics_text(void);

// A Portable Network Graphics (PNG) image.
//
// Constant alias for string: `"image/png"`.
extern const z_owned_encoding_t ENCODING_IMAGE_PNG;
const z_loaned_encoding_t *z_encoding_image_png(void);

// A Joint Photographic Experts Group (JPEG) image.
//
// Constant alias for string: `"image/jpeg"`.
extern const z_owned_encoding_t ENCODING_IMAGE_JPEG;
const z_loaned_encoding_t *z_encoding_image_jpeg(void);

// A Graphics Interchange Format (GIF) image.
//
// Constant alias for string: `"image/gif"`.
extern const z_owned_encoding_t ENCODING_IMAGE_GIF;
const z_loaned_encoding_t *z_encoding_image_gif(void);

// A BitMap (BMP) image.
//
// Constant alias for string: `"image/bmp"`.
extern const z_owned_encoding_t ENCODING_IMAGE_BMP;
const z_loaned_encoding_t *z_encoding_image_bmp(void);

// A Web Portable (WebP) image.
//
//  Constant alias for string: `"image/webp"`.
extern const z_owned_encoding_t ENCODING_IMAGE_WEBP;
const z_loaned_encoding_t *z_encoding_image_webp(void);

// An XML file intended to be consumed by an application..
//
// Constant alias for string: `"application/xml"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_XML;
const z_loaned_encoding_t *z_encoding_application_xml(void);

// An encoded a list of tuples, each consisting of a name and a value.
//
// Constant alias for string: `"application/x-www-form-urlencoded"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_X_WWW_FORM_URLENCODED;
const z_loaned_encoding_t *z_encoding_application_x_www_form_urlencoded(void);

// An HTML file.
//
// Constant alias for string: `"text/html"`.
extern const z_owned_encoding_t ENCODING_TEXT_HTML;
const z_loaned_encoding_t *z_encoding_text_html(void);

// An XML file that is human readable.
//
// Constant alias for string: `"text/xml"`.
extern const z_owned_encoding_t ENCODING_TEXT_XML;
const z_loaned_encoding_t *z_encoding_text_xml(void);

// A CSS file.
//
// Constant alias for string: `"text/css"`.
extern const z_owned_encoding_t ENCODING_TEXT_CSS;
const z_loaned_encoding_t *z_encoding_text_css(void);

// A JavaScript file.
//
// Constant alias for string: `"text/javascript"`.
extern const z_owned_encoding_t ENCODING_TEXT_JAVASCRIPT;
const z_loaned_encoding_t *z_encoding_text_javascript(void);

// A MarkDown file.
//
// Constant alias for string: `"text/markdown"`.
extern const z_owned_encoding_t ENCODING_TEXT_MARKDOWN;
const z_loaned_encoding_t *z_encoding_text_markdown(void);

// A CSV file.
//
// Constant alias for string: `"text/csv"`.
extern const z_owned_encoding_t ENCODING_TEXT_CSV;
const z_loaned_encoding_t *z_encoding_text_csv(void);

// An application-specific SQL query.
//
// Constant alias for string: `"application/sql"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_SQL;
const z_loaned_encoding_t *z_encoding_application_sql(void);

// Constrained Application Protocol (CoAP) data intended for CoAP-to-HTTP and HTTP-to-CoAP proxies.
//
// Constant alias for string: `"application/coap-payload"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_COAP_PAYLOAD;
const z_loaned_encoding_t *z_encoding_application_coap_payload(void);

// Defines a JSON document structure for expressing a sequence of operations to apply to a JSON document.
//
// Constant alias for string: `"application/json-patch+json"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JSON_PATCH_JSON;
const z_loaned_encoding_t *z_encoding_application_json_patch_json(void);

// A JSON text sequence consists of any number of JSON texts, all encoded in UTF-8.
//
// Constant alias for string: `"application/json-seq"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JSON_SEQ;
const z_loaned_encoding_t *z_encoding_application_json_seq(void);

// A JSONPath defines a string syntax for selecting and extracting JSON values from within a given JSON value.
//
// Constant alias for string: `"application/jsonpath"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JSONPATH;
const z_loaned_encoding_t *z_encoding_application_jsonpath(void);

// A JSON Web Token (JWT).
//
// Constant alias for string: `"application/jwt"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_JWT;
const z_loaned_encoding_t *z_encoding_application_jwt(void);

// An application-specific MPEG-4 encoded data, either audio or video.
//
// Constant alias for string: `"application/mp4"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_MP4;
const z_loaned_encoding_t *z_encoding_application_mp4(void);

// A SOAP 1.2 message serialized as XML 1.0.
//
// Constant alias for string: `"application/soap+xml"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_SOAP_XML;
const z_loaned_encoding_t *z_encoding_application_soap_xml(void);

// A YANG-encoded data commonly used by the Network Configuration Protocol (NETCONF).
//
// Constant alias for string: `"application/yang"`.
extern const z_owned_encoding_t ENCODING_APPLICATION_YANG;
const z_loaned_encoding_t *z_encoding_application_yang(void);

// A MPEG-4 Advanced Audio Coding (AAC) media.
//
// Constant alias for string: `"audio/aac"`.
extern const z_owned_encoding_t ENCODING_AUDIO_AAC;
const z_loaned_encoding_t *z_encoding_audio_aac(void);

// A Free Lossless Audio Codec (FLAC) media.
//
// Constant alias for string: `"audio/flac"`.
extern const z_owned_encoding_t ENCODING_AUDIO_FLAC;
const z_loaned_encoding_t *z_encoding_audio_flac(void);

// An audio codec defined in MPEG-1, MPEG-2, MPEG-4, or registered at the MP4 registration authority.
//
// Constant alias for string: `"audio/mp4"`.
extern const z_owned_encoding_t ENCODING_AUDIO_MP4;
const z_loaned_encoding_t *z_encoding_audio_mp4(void);

// An Ogg-encapsulated audio stream.
//
// Constant alias for string: `"audio/ogg"`.
extern const z_owned_encoding_t ENCODING_AUDIO_OGG;
const z_loaned_encoding_t *z_encoding_audio_ogg(void);

// A Vorbis-encoded audio stream.
//
// Constant alias for string: `"audio/vorbis"`.
extern const z_owned_encoding_t ENCODING_AUDIO_VORBIS;
const z_loaned_encoding_t *z_encoding_audio_vorbis(void);

// A h261-encoded video stream.
//
// Constant alias for string: `"video/h261"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H261;
const z_loaned_encoding_t *z_encoding_video_h261(void);

// A h263-encoded video stream.
//
// Constant alias for string: `"video/h263"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H263;
const z_loaned_encoding_t *z_encoding_video_h263(void);

// A h264-encoded video stream.
//
// Constant alias for string: `"video/h264"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H264;
const z_loaned_encoding_t *z_encoding_video_h264(void);

// A h265-encoded video stream.
//
// Constant alias for string: `"video/h265"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H265;
const z_loaned_encoding_t *z_encoding_video_h265(void);

// A h266-encoded video stream.
//
// Constant alias for string: `"video/h266"`.
extern const z_owned_encoding_t ENCODING_VIDEO_H266;
const z_loaned_encoding_t *z_encoding_video_h266(void);

// A video codec defined in MPEG-1, MPEG-2, MPEG-4, or registered at the MP4 registration authority.
//
// Constant alias for string: `"video/mp4"`.
extern const z_owned_encoding_t ENCODING_VIDEO_MP4;
const z_loaned_encoding_t *z_encoding_video_mp4(void);

// An Ogg-encapsulated video stream.
//
// Constant alias for string: `"video/ogg"`.
extern const z_owned_encoding_t ENCODING_VIDEO_OGG;
const z_loaned_encoding_t *z_encoding_video_ogg(void);

// An uncompressed, studio-quality video stream.
//
// Constant alias for string: `"video/raw"`.
extern const z_owned_encoding_t ENCODING_VIDEO_RAW;
const z_loaned_encoding_t *z_encoding_video_raw(void);

// A VP8-encoded video stream.
//
// Constant alias for string: `"video/vp8"`.
extern const z_owned_encoding_t ENCODING_VIDEO_VP8;
const z_loaned_encoding_t *z_encoding_video_vp8(void);

// A VP9-encoded video stream.
//
// Constant alias for string: `"video/vp9"`.
extern const z_owned_encoding_t ENCODING_VIDEO_VP9;
const z_loaned_encoding_t *z_encoding_video_vp9(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_API_ENCODING_H */
