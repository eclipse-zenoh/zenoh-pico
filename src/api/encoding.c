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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"

#if Z_FEATURE_ENCODING_VALUES == 1
#define ENCODING_SCHEMA_SEPARATOR ';'

#define ENCODING_CONSTANT_MACRO(_cname, _fname, _id)                                                                \
    const z_owned_encoding_t ZP_ENCODING_##_cname = {                                                               \
        ._val = {                                                                                                   \
            .id = _id,                                                                                              \
            .schema = {._slice = {.start = NULL, .len = 0, ._delete_context = {.deleter = NULL, .context = NULL}}}, \
        }};                                                                                                         \
    const z_loaned_encoding_t *z_encoding_##_fname(void) { return &ZP_ENCODING_##_cname._val; }

ENCODING_CONSTANT_MACRO(ZENOH_BYTES, zenoh_bytes, 0)
ENCODING_CONSTANT_MACRO(ZENOH_INT8, zenoh_int8, 1)
ENCODING_CONSTANT_MACRO(ZENOH_INT16, zenoh_int16, 2)
ENCODING_CONSTANT_MACRO(ZENOH_INT32, zenoh_int32, 3)
ENCODING_CONSTANT_MACRO(ZENOH_INT64, zenoh_int64, 4)
ENCODING_CONSTANT_MACRO(ZENOH_INT128, zenoh_int128, 5)
ENCODING_CONSTANT_MACRO(ZENOH_UINT8, zenoh_uint8, 6)
ENCODING_CONSTANT_MACRO(ZENOH_UINT16, zenoh_uint16, 7)
ENCODING_CONSTANT_MACRO(ZENOH_UINT32, zenoh_uint32, 8)
ENCODING_CONSTANT_MACRO(ZENOH_UINT64, zenoh_uint64, 9)
ENCODING_CONSTANT_MACRO(ZENOH_UINT128, zenoh_uint128, 10)
ENCODING_CONSTANT_MACRO(ZENOH_FLOAT32, zenoh_float32, 11)
ENCODING_CONSTANT_MACRO(ZENOH_FLOAT64, zenoh_float64, 12)
ENCODING_CONSTANT_MACRO(ZENOH_BOOL, zenoh_bool, 13)
ENCODING_CONSTANT_MACRO(ZENOH_STRING, zenoh_string, 14)
ENCODING_CONSTANT_MACRO(ZENOH_ERROR, zenoh_error, 15)
ENCODING_CONSTANT_MACRO(APPLICATION_OCTET_STREAM, application_octet_stream, 16)
ENCODING_CONSTANT_MACRO(TEXT_PLAIN, text_plain, 17)
ENCODING_CONSTANT_MACRO(APPLICATION_JSON, application_json, 18)
ENCODING_CONSTANT_MACRO(TEXT_JSON, text_json, 19)
ENCODING_CONSTANT_MACRO(APPLICATION_CDR, application_cdr, 20)
ENCODING_CONSTANT_MACRO(APPLICATION_CBOR, application_cbor, 21)
ENCODING_CONSTANT_MACRO(APPLICATION_YAML, application_yaml, 22)
ENCODING_CONSTANT_MACRO(TEXT_YAML, text_yaml, 23)
ENCODING_CONSTANT_MACRO(TEXT_JSON5, text_json5, 24)
ENCODING_CONSTANT_MACRO(APPLICATION_PYTHON_SERIALIZED_OBJECT, application_python_serialized_object, 25)
ENCODING_CONSTANT_MACRO(APPLICATION_PROTOBUF, application_protobuf, 26)
ENCODING_CONSTANT_MACRO(APPLICATION_JAVA_SERIALIZED_OBJECT, application_java_serialized_object, 27)
ENCODING_CONSTANT_MACRO(APPLICATION_OPENMETRICS_TEXT, application_openmetrics_text, 28)
ENCODING_CONSTANT_MACRO(IMAGE_PNG, image_png, 29)
ENCODING_CONSTANT_MACRO(IMAGE_JPEG, image_jpeg, 30)
ENCODING_CONSTANT_MACRO(IMAGE_GIF, image_gif, 31)
ENCODING_CONSTANT_MACRO(IMAGE_BMP, image_bmp, 32)
ENCODING_CONSTANT_MACRO(IMAGE_WEBP, image_webp, 33)
ENCODING_CONSTANT_MACRO(APPLICATION_XML, application_xml, 34)
ENCODING_CONSTANT_MACRO(APPLICATION_X_WWW_FORM_URLENCODED, application_x_www_form_urlencoded, 35)
ENCODING_CONSTANT_MACRO(TEXT_HTML, text_html, 36)
ENCODING_CONSTANT_MACRO(TEXT_XML, text_xml, 37)
ENCODING_CONSTANT_MACRO(TEXT_CSS, text_css, 38)
ENCODING_CONSTANT_MACRO(TEXT_JAVASCRIPT, text_javascript, 39)
ENCODING_CONSTANT_MACRO(TEXT_MARKDOWN, text_markdown, 40)
ENCODING_CONSTANT_MACRO(TEXT_CSV, text_csv, 41)
ENCODING_CONSTANT_MACRO(APPLICATION_SQL, application_sql, 42)
ENCODING_CONSTANT_MACRO(APPLICATION_COAP_PAYLOAD, application_coap_payload, 43)
ENCODING_CONSTANT_MACRO(APPLICATION_JSON_PATCH_JSON, application_json_patch_json, 44)
ENCODING_CONSTANT_MACRO(APPLICATION_JSON_SEQ, application_json_seq, 45)
ENCODING_CONSTANT_MACRO(APPLICATION_JSONPATH, application_jsonpath, 46)
ENCODING_CONSTANT_MACRO(APPLICATION_JWT, application_jwt, 47)
ENCODING_CONSTANT_MACRO(APPLICATION_MP4, application_mp4, 48)
ENCODING_CONSTANT_MACRO(APPLICATION_SOAP_XML, application_soap_xml, 49)
ENCODING_CONSTANT_MACRO(APPLICATION_YANG, application_yang, 50)
ENCODING_CONSTANT_MACRO(AUDIO_AAC, audio_aac, 51)
ENCODING_CONSTANT_MACRO(AUDIO_FLAC, audio_flac, 52)
ENCODING_CONSTANT_MACRO(AUDIO_MP4, audio_mp4, 53)
ENCODING_CONSTANT_MACRO(AUDIO_OGG, audio_ogg, 54)
ENCODING_CONSTANT_MACRO(AUDIO_VORBIS, audio_vorbis, 55)
ENCODING_CONSTANT_MACRO(VIDEO_H261, video_h261, 56)
ENCODING_CONSTANT_MACRO(VIDEO_H263, video_h263, 57)
ENCODING_CONSTANT_MACRO(VIDEO_H264, video_h264, 58)
ENCODING_CONSTANT_MACRO(VIDEO_H265, video_h265, 59)
ENCODING_CONSTANT_MACRO(VIDEO_H266, video_h266, 60)
ENCODING_CONSTANT_MACRO(VIDEO_MP4, video_mp4, 61)
ENCODING_CONSTANT_MACRO(VIDEO_OGG, video_ogg, 62)
ENCODING_CONSTANT_MACRO(VIDEO_RAW, video_raw, 63)
ENCODING_CONSTANT_MACRO(VIDEO_VP8, video_vp8, 64)
ENCODING_CONSTANT_MACRO(VIDEO_VP9, video_vp9, 65)

const char *ENCODING_VALUES_ID_TO_STR[] = {
    "zenoh/bytes",
    "zenoh/int8",
    "zenoh/int16",
    "zenoh/int32",
    "zenoh/int64",
    "zenoh/int128",
    "zenoh/uint8",
    "zenoh/uint16",
    "zenoh/uint32",
    "zenoh/uint64",
    "zenoh/uint128",
    "zenoh/float32",
    "zenoh/float64",
    "zenoh/bool",
    "zenoh/string",
    "zenoh/error",
    "application/octet-stream",
    "text/plain",
    "application/json",
    "text/json",
    "application/cdr",
    "application/cbor",
    "application/yaml",
    "text/yaml",
    "text/json5",
    "application/python-serialized-object",
    "application/protobuf",
    "application/java-serialized-object",
    "application/openmetrics-text",
    "image/png",
    "image/jpeg",
    "image/gif",
    "image/bmp",
    "image/webp",
    "application/xml",
    "application/x-www-form-urlencoded",
    "text/html",
    "text/xml",
    "text/css",
    "text/javascript",
    "text/markdown",
    "text/csv",
    "application/sql",
    "application/coap-payload",
    "application/json-patch+json",
    "application/json-seq",
    "application/jsonpath",
    "application/jwt",
    "application/mp4",
    "application/soap+xml",
    "application/yang",
    "audio/aac",
    "audio/flac",
    "audio/mp4",
    "audio/ogg",
    "audio/vorbis",
    "video/h261",
    "video/h263",
    "video/h264",
    "video/h265",
    "video/h266",
    "video/mp4",
    "video/ogg",
    "video/raw",
    "video/vp8",
    "video/vp9",
};

static uint16_t _z_encoding_values_str_to_int(const char *schema, size_t len) {
    for (size_t i = 0; i < _ZP_ARRAY_SIZE(ENCODING_VALUES_ID_TO_STR); i++) {
        if (strncmp(schema, ENCODING_VALUES_ID_TO_STR[i], len) == 0) {
            return (uint16_t)i;
        }
    }
    return UINT16_MAX;
}

static z_result_t _z_encoding_convert_from_substr(z_owned_encoding_t *encoding, const char *s, size_t len) {
    size_t pos = 0;
    for (; pos < len; ++pos) {
        if (s[pos] == ENCODING_SCHEMA_SEPARATOR) break;
    }

    // Check id_end value + corner cases
    if ((pos != len) && (pos != 0)) {
        uint16_t id = _z_encoding_values_str_to_int(s, pos);
        // Check id
        if (id != UINT16_MAX) {
            const char *ptr = (pos + 1 == len) ? NULL : s + pos + 1;
            return _z_encoding_make(&encoding->_val, id, ptr, len - pos - 1);
        }
    }
    // By default store the string as schema
    return _z_encoding_make(&encoding->_val, _Z_ENCODING_ID_DEFAULT, s, len);
}

static z_result_t _z_encoding_convert_into_string(const z_loaned_encoding_t *encoding, z_owned_string_t *s) {
    const char *prefix = NULL;
    size_t prefix_len = 0;
    // Convert id
    if (encoding->id < _ZP_ARRAY_SIZE(ENCODING_VALUES_ID_TO_STR)) {
        prefix = ENCODING_VALUES_ID_TO_STR[encoding->id];
        prefix_len = strlen(prefix);
    }
    _Bool has_schema = _z_string_len(&encoding->schema) > 0;
    // Size include null terminator
    size_t total_len = prefix_len + _z_string_len(&encoding->schema) + 1;
    // Check for schema separator
    if (has_schema) {
        total_len += 1;
    }
    // Allocate string
    char *value = (char *)z_malloc(sizeof(char) * total_len);
    memset(value, 0, total_len);
    if (value == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    // Copy prefix
    char sep = ENCODING_SCHEMA_SEPARATOR;
    (void)strncpy(value, prefix, prefix_len);
    // Copy schema and separator
    if (has_schema) {
        (void)strncat(value, &sep, 1);
        (void)strncat(value, _z_string_data(&encoding->schema), _z_string_len(&encoding->schema));
    }
    // Fill container
    s->_val = _z_string_from_str_custom_deleter(value, _z_delete_context_default());
    return _Z_RES_OK;
}

#else
static z_result_t _z_encoding_convert_from_substr(z_owned_encoding_t *encoding, const char *s, size_t len) {
    return _z_encoding_make(&encoding->_val, _Z_ENCODING_ID_DEFAULT, s, len);
}

static z_result_t _z_encoding_convert_into_string(const z_loaned_encoding_t *encoding, z_owned_string_t *s) {
    _z_string_copy(&s->_val, &encoding->schema);
    return _Z_RES_OK;
}
#endif

z_result_t z_encoding_from_str(z_owned_encoding_t *encoding, const char *s) {
    // Init owned encoding
    z_internal_encoding_null(encoding);
    // Convert string to encoding
    if (s != NULL) {
        return _z_encoding_convert_from_substr(encoding, s, strlen(s));
    }
    return _Z_RES_OK;
}

z_result_t z_encoding_from_substr(z_owned_encoding_t *encoding, const char *s, size_t len) {
    // Init owned encoding
    z_internal_encoding_null(encoding);
    // Convert string to encoding
    if (s != NULL) {
        return _z_encoding_convert_from_substr(encoding, s, len);
    }
    return _Z_RES_OK;
}
z_result_t z_encoding_set_schema_from_str(z_loaned_encoding_t *encoding, const char *schema) {
    return z_encoding_set_schema_from_substr(encoding, schema, strlen(schema));
}

z_result_t z_encoding_set_schema_from_substr(z_loaned_encoding_t *encoding, const char *schema, size_t len) {
    _z_string_clear(&encoding->schema);
    if (schema == NULL && len > 0) {
        return _Z_ERR_INVALID;
    }
    encoding->schema = _z_string_copy_from_substr(schema, len);
    if (_z_string_len(&encoding->schema) != len) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return _Z_RES_OK;
}

z_result_t z_encoding_to_string(const z_loaned_encoding_t *encoding, z_owned_string_t *s) {
    // Init owned string
    z_internal_string_null(s);
    // Convert encoding to string
    _z_encoding_convert_into_string(encoding, s);
    return _Z_RES_OK;
}
