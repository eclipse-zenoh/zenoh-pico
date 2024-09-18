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

#define ENCODING_CONSTANT_MACRO(_name, _id)                                                                         \
    const z_owned_encoding_t _name = {                                                                              \
        ._val = {                                                                                                   \
            .id = _id,                                                                                              \
            .schema = {._slice = {.start = NULL, .len = 0, ._delete_context = {.deleter = NULL, .context = NULL}}}, \
        }}

ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_BYTES, 0);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_INT8, 1);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_INT16, 2);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_INT32, 3);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_INT64, 4);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_INT128, 5);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_UINT8, 6);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_UINT16, 7);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_UINT32, 8);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_UINT64, 9);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_UINT128, 10);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_FLOAT32, 11);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_FLOAT64, 12);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_BOOL, 13);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_STRING, 14);
ENCODING_CONSTANT_MACRO(ENCODING_ZENOH_ERROR, 15);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_OCTET_STREAM, 16);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_PLAIN, 17);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_JSON, 18);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_JSON, 19);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_CDR, 20);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_CBOR, 21);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_YAML, 22);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_YAML, 23);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_JSON5, 24);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_PYTHON_SERIALIZED_OBJECT, 25);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_PROTOBUF, 26);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_JAVA_SERIALIZED_OBJECT, 27);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_OPENMETRICS_TEXT, 28);
ENCODING_CONSTANT_MACRO(ENCODING_IMAGE_PNG, 29);
ENCODING_CONSTANT_MACRO(ENCODING_IMAGE_JPEG, 30);
ENCODING_CONSTANT_MACRO(ENCODING_IMAGE_GIF, 31);
ENCODING_CONSTANT_MACRO(ENCODING_IMAGE_BMP, 32);
ENCODING_CONSTANT_MACRO(ENCODING_IMAGE_WEBP, 33);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_XML, 34);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_X_WWW_FORM_URLENCODED, 35);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_HTML, 36);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_XML, 37);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_CSS, 38);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_JAVASCRIPT, 39);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_MARKDOWN, 40);
ENCODING_CONSTANT_MACRO(ENCODING_TEXT_CSV, 41);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_SQL, 42);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_COAP_PAYLOAD, 43);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_JSON_PATCH_JSON, 44);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_JSON_SEQ, 45);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_JSONPATH, 46);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_JWT, 47);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_MP4, 48);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_SOAP_XML, 49);
ENCODING_CONSTANT_MACRO(ENCODING_APPLICATION_YANG, 50);
ENCODING_CONSTANT_MACRO(ENCODING_AUDIO_AAC, 51);
ENCODING_CONSTANT_MACRO(ENCODING_AUDIO_FLAC, 52);
ENCODING_CONSTANT_MACRO(ENCODING_AUDIO_MP4, 53);
ENCODING_CONSTANT_MACRO(ENCODING_AUDIO_OGG, 54);
ENCODING_CONSTANT_MACRO(ENCODING_AUDIO_VORBIS, 55);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_H261, 56);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_H263, 57);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_H264, 58);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_H265, 59);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_H266, 60);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_MP4, 61);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_OGG, 62);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_RAW, 63);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_VP8, 64);
ENCODING_CONSTANT_MACRO(ENCODING_VIDEO_VP9, 65);

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
