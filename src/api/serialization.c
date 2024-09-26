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

#include "zenoh-pico/api/serialization.h"

#include <string.h>

#include "zenoh-pico/protocol/codec/core.h"

ze_serializer_t ze_serializer(z_loaned_bytes_t *bytes) {
    ze_serializer_t s;
    s._writer = z_bytes_get_writer(bytes);
    return s;
}

ze_deserializer_t ze_deserializer(const z_loaned_bytes_t *bytes) {
    ze_deserializer_t d;
    d._reader = z_bytes_get_reader(bytes);
    return d;
}

z_result_t __read_single_byte(uint8_t *b, void *context) {
    z_bytes_reader_t *reader = (z_bytes_reader_t *)context;
    return _z_bytes_reader_read(reader, b, 1) == 1 ? _Z_RES_OK : _Z_ERR_DID_NOT_READ;
}

z_result_t __read_zint(z_bytes_reader_t *reader, _z_zint_t *zint) {
    return _z_zsize_decode_with_reader(zint, __read_single_byte, reader);
}

z_result_t ze_serializer_serialize_sequence_begin(ze_serializer_t *serializer, size_t len) {
    uint8_t buf[16];
    size_t bytes_used = _z_zsize_encode_buf(buf, len);
    return _z_bytes_writer_write_all(&serializer->_writer, buf, bytes_used);
}

z_result_t ze_serializer_serialize_sequence_end(ze_serializer_t *serializer) {
    _ZP_UNUSED(serializer);
    return _Z_RES_OK;
}

z_result_t ze_deserializer_deserialize_sequence_begin(ze_deserializer_t *deserializer, size_t *len) {
    return __read_zint(&deserializer->_reader, len);
}

z_result_t ze_deserializer_deserialize_sequence_end(ze_deserializer_t *deserializer) {
    _ZP_UNUSED(deserializer);
    return _Z_RES_OK;
}

z_result_t ze_serializer_serialize_buf(ze_serializer_t *serializer, const uint8_t *val, size_t len) {
    _Z_RETURN_IF_ERR(ze_serializer_serialize_sequence_begin(serializer, len));
    _Z_RETURN_IF_ERR(_z_bytes_writer_write_all(&serializer->_writer, val, len));
    return ze_serializer_serialize_sequence_end(serializer);
}

z_result_t ze_serializer_serialize_slice(ze_serializer_t *serializer, const z_loaned_slice_t *val) {
    return ze_serializer_serialize_buf(serializer, z_slice_data(val), z_slice_len(val));
}

z_result_t ze_deserializer_deserialize_slice(ze_deserializer_t *deserializer, z_owned_slice_t *val) {
    size_t len = 0;
    _Z_RETURN_IF_ERR(ze_deserializer_deserialize_sequence_begin(deserializer, &len));
    _Z_RETURN_IF_ERR(_z_slice_init(&val->_val, len));
    if (z_bytes_reader_read(&deserializer->_reader, (uint8_t *)val->_val.start, len) != len) {
        _z_slice_clear(&val->_val);
        return _Z_ERR_DID_NOT_READ;
    };
    return ze_deserializer_deserialize_sequence_end(deserializer);
}

z_result_t ze_serializer_serialize_str(ze_serializer_t *serializer, const char *val) {
    size_t len = strlen(val);
    return ze_serializer_serialize_buf(serializer, (const uint8_t *)val, len);
}

z_result_t ze_serializer_serialize_string(ze_serializer_t *serializer, const z_loaned_string_t *val) {
    return ze_serializer_serialize_buf(serializer, (const uint8_t *)z_string_data(val), z_string_len(val));
}

z_result_t ze_deserializer_deserialize_string(ze_deserializer_t *deserializer, z_owned_string_t *val) {
    z_owned_slice_t s;
    _Z_RETURN_IF_ERR(ze_deserializer_deserialize_slice(deserializer, &s));
    val->_val._slice = s._val;
    return _Z_RES_OK;
}

#define _Z_BUILD_BYTES_FROM_SERIALIZER(expr)                             \
    z_bytes_empty(bytes);                                                \
    ze_serializer_t serializer = ze_serializer(z_bytes_loan_mut(bytes)); \
    _Z_CLEAN_RETURN_IF_ERR(expr, z_bytes_drop(z_bytes_move(bytes)));

z_result_t ze_serialize_buf(z_owned_bytes_t *bytes, const uint8_t *data, size_t len) {
    _Z_BUILD_BYTES_FROM_SERIALIZER(ze_serializer_serialize_buf(&serializer, data, len));
    return _Z_RES_OK;
}

z_result_t ze_serialize_slice(z_owned_bytes_t *bytes, const z_loaned_slice_t *data) {
    _Z_BUILD_BYTES_FROM_SERIALIZER(ze_serializer_serialize_slice(&serializer, data));
    return _Z_RES_OK;
}

z_result_t ze_deserialize_slice(const z_loaned_bytes_t *bytes, z_owned_slice_t *data) {
    ze_deserializer_t deserializer = ze_deserializer(bytes);
    return ze_deserializer_deserialize_slice(&deserializer, data);
}

z_result_t ze_serialize_str(z_owned_bytes_t *bytes, const char *data) {
    _Z_BUILD_BYTES_FROM_SERIALIZER(ze_serializer_serialize_str(&serializer, data));
    return _Z_RES_OK;
}

z_result_t ze_serialize_string(z_owned_bytes_t *bytes, const z_loaned_string_t *data) {
    _Z_BUILD_BYTES_FROM_SERIALIZER(ze_serializer_serialize_string(&serializer, data));
    return _Z_RES_OK;
}

z_result_t ze_deserialize_string(const z_loaned_bytes_t *bytes, z_owned_string_t *data) {
    ze_deserializer_t deserializer = ze_deserializer(bytes);
    return ze_deserializer_deserialize_string(&deserializer, data);
}

#define _Z_IMPLEMENT_ZBYTES_ARITHMETIC(suffix, type)                                        \
    z_result_t ze_serialize_##suffix(z_owned_bytes_t *bytes, type data) {                   \
        _Z_BUILD_BYTES_FROM_SERIALIZER(ze_serializer_serialize_##suffix(&serializer, data)) \
        return _Z_RES_OK;                                                                   \
    }                                                                                       \
    z_result_t ze_deserialize_##suffix(const z_loaned_bytes_t *bytes, type *data) {         \
        ze_deserializer_t deserializer = ze_deserializer(bytes);                            \
        return ze_deserializer_deserialize_##suffix(&deserializer, data);                   \
    }

_Z_IMPLEMENT_ZBYTES_ARITHMETIC(uint8, uint8_t)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(uint16, uint16_t)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(uint32, uint32_t)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(uint64, uint64_t)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(int8, int8_t)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(int16, int16_t)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(int32, int32_t)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(int64, int64_t)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(float, float)
_Z_IMPLEMENT_ZBYTES_ARITHMETIC(double, double)
