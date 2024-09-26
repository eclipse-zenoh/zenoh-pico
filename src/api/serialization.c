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

z_result_t __read_single_byte(uint8_t *b, void *context) {
    z_bytes_reader_t *reader = (z_bytes_reader_t *)context;
    return _z_bytes_reader_read(reader, b, 1) == 1 ? _Z_RES_OK : _Z_ERR_DID_NOT_READ;
}

z_result_t __read_zint(z_bytes_reader_t *reader, _z_zint_t *zint) {
    return _z_zsize_decode_with_reader(zint, __read_single_byte, reader);
}

z_result_t z_bytes_writer_serialize_sequence_begin(z_bytes_writer_t *writer, size_t len) {
    uint8_t buf[16];
    size_t bytes_used = _z_zsize_encode_buf(buf, len);
    return _z_bytes_writer_write_all(writer, buf, bytes_used);
}

z_result_t z_bytes_writer_serialize_sequence_end(z_bytes_writer_t *writer) {
    _ZP_UNUSED(writer);
    return _Z_RES_OK;
}

z_result_t z_bytes_reader_deserialize_sequence_begin(z_bytes_reader_t *reader, size_t *len) {
    return __read_zint(reader, len);
}

z_result_t z_bytes_reader_deserialize_sequence_end(z_bytes_reader_t *reader) {
    _ZP_UNUSED(reader);
    return _Z_RES_OK;
}

z_result_t z_bytes_writer_serialize_buf(z_bytes_writer_t *writer, const uint8_t *val, size_t len) {
    _Z_RETURN_IF_ERR(z_bytes_writer_serialize_sequence_begin(writer, len));
    _Z_RETURN_IF_ERR(_z_bytes_writer_write_all(writer, val, len));
    return z_bytes_writer_serialize_sequence_end(writer);
}

z_result_t z_bytes_writer_serialize_slice(z_bytes_writer_t *writer, const z_loaned_slice_t *val) {
    return z_bytes_writer_serialize_buf(writer, z_slice_data(val), z_slice_len(val));
}

z_result_t z_bytes_reader_deserialize_slice(z_bytes_reader_t *reader, z_owned_slice_t *val) {
    size_t len = 0;
    _Z_RETURN_IF_ERR(z_bytes_reader_deserialize_sequence_begin(reader, &len));
    _Z_RETURN_IF_ERR(_z_slice_init(&val->_val, len));
    if (z_bytes_reader_read(reader, (uint8_t *)val->_val.start, len) != len) {
        _z_slice_clear(&val->_val);
        return _Z_ERR_DID_NOT_READ;
    };
    return z_bytes_reader_deserialize_sequence_end(reader);
}

z_result_t z_bytes_writer_serialize_str(z_bytes_writer_t *writer, const char *val) {
    size_t len = strlen(val);
    return z_bytes_writer_serialize_buf(writer, (const uint8_t *)val, len);
}

z_result_t z_bytes_writer_serialize_string(z_bytes_writer_t *writer, const z_loaned_string_t *val) {
    return z_bytes_writer_serialize_buf(writer, (const uint8_t *)z_string_data(val), z_string_len(val));
}

z_result_t z_bytes_reader_deserialize_string(z_bytes_reader_t *reader, z_owned_string_t *val) {
    z_owned_slice_t s;
    _Z_RETURN_IF_ERR(z_bytes_reader_deserialize_slice(reader, &s));
    val->_val._slice = s._val;
    return _Z_RES_OK;
}

#define _Z_BUILD_BYTES_FROM_WRITER(expr)                                   \
    z_bytes_empty(bytes);                                                  \
    z_bytes_writer_t writer = z_bytes_get_writer(z_bytes_loan_mut(bytes)); \
    _Z_CLEAN_RETURN_IF_ERR(expr, z_bytes_drop(z_bytes_move(bytes)));

z_result_t z_bytes_serialize_from_buf(z_owned_bytes_t *bytes, const uint8_t *data, size_t len) {
    _Z_BUILD_BYTES_FROM_WRITER(z_bytes_writer_serialize_buf(&writer, data, len));
    return _Z_RES_OK;
}

z_result_t z_bytes_serialize_from_slice(z_owned_bytes_t *bytes, const z_loaned_slice_t *data) {
    _Z_BUILD_BYTES_FROM_WRITER(z_bytes_writer_serialize_slice(&writer, data));
    return _Z_RES_OK;
}

z_result_t z_bytes_deserialize_into_slice(const z_loaned_bytes_t *bytes, z_owned_slice_t *data) {
    z_bytes_reader_t reader = z_bytes_get_reader(bytes);
    return z_bytes_reader_deserialize_slice(&reader, data);
}

z_result_t z_bytes_serialize_from_str(z_owned_bytes_t *bytes, const char *data) {
    _Z_BUILD_BYTES_FROM_WRITER(z_bytes_writer_serialize_str(&writer, data));
    return _Z_RES_OK;
}

z_result_t z_bytes_serialize_from_string(z_owned_bytes_t *bytes, const z_loaned_string_t *data) {
    _Z_BUILD_BYTES_FROM_WRITER(z_bytes_writer_serialize_string(&writer, data));
    return _Z_RES_OK;
}

z_result_t z_bytes_deserialize_into_string(const z_loaned_bytes_t *bytes, z_owned_string_t *data) {
    z_bytes_reader_t reader = z_bytes_get_reader(bytes);
    return z_bytes_reader_deserialize_string(&reader, data);
}

#define _Z_IMPLEMENT_ZBYTES_ARITHMETIC(suffix, type)                                          \
    z_result_t z_bytes_serialize_from_##suffix(z_owned_bytes_t *bytes, type data) {           \
        _Z_BUILD_BYTES_FROM_WRITER(z_bytes_writer_serialize_##suffix(&writer, data))          \
        return _Z_RES_OK;                                                                     \
    }                                                                                         \
    z_result_t z_bytes_deserialize_into_##suffix(const z_loaned_bytes_t *bytes, type *data) { \
        z_bytes_reader_t reader = z_bytes_get_reader(bytes);                                  \
        return z_bytes_reader_deserialize_##suffix(&reader, data);                            \
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
