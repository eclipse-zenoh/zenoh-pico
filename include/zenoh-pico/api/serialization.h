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
//

#ifndef INCLUDE_ZENOH_PICO_API_SERIALIZATION_H
#define INCLUDE_ZENOH_PICO_API_SERIALIZATION_H

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/utils/endianness.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Writes a serialized :c:type:`uint8_t` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `uint8_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_uint8(z_bytes_writer_t *writer, uint8_t val) {
    return z_bytes_writer_write_all(writer, &val, 1);
}

/**
 * Writes a serialized :c:type:`uint16_t` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `uint16_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_uint16(z_bytes_writer_t *writer, uint16_t val) {
    _z_host_le_store16(val, (uint8_t *)&val);
    return z_bytes_writer_write_all(writer, (uint8_t *)&val, sizeof(val));
}

/**
 * Writes a serialized :c:type:`uint32_t` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `uint32_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_uint32(z_bytes_writer_t *writer, uint32_t val) {
    _z_host_le_store32(val, (uint8_t *)&val);
    return z_bytes_writer_write_all(writer, (uint8_t *)&val, sizeof(val));
}

/**
 * Writes a serialized :c:type:`uint64_t` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `uint64_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_uint64(z_bytes_writer_t *writer, uint64_t val) {
    _z_host_le_store64(val, (uint8_t *)&val);
    return z_bytes_writer_write_all(writer, (uint8_t *)&val, sizeof(val));
}

/**
 * Writes a serialized :c:type:`float` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `float` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_float(z_bytes_writer_t *writer, float val) {
    return z_bytes_writer_write_all(writer, (uint8_t *)&val, sizeof(val));
}

/**
 * Writes a serialized :c:type:`double` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `double` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_double(z_bytes_writer_t *writer, double val) {
    return z_bytes_writer_write_all(writer, (uint8_t *)&val, sizeof(val));
}

/**
 * Writes a serialized :c:type:`int8_t` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `int8_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_int8(z_bytes_writer_t *writer, int8_t val) {
    return z_bytes_writer_serialize_uint8(writer, (uint8_t)val);
}

/**
 * Writes a serialized :c:type:`int16_t` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `int16_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_int16(z_bytes_writer_t *writer, int16_t val) {
    return z_bytes_writer_serialize_uint16(writer, (uint16_t)val);
}

/**
 * Writes a serialized :c:type:`int32_t` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `int32_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_int32(z_bytes_writer_t *writer, int32_t val) {
    return z_bytes_writer_serialize_uint32(writer, (uint32_t)val);
}

/**
 * Writes a serialized :c:type:`int64_t` into underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: `int64_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_writer_serialize_int64(z_bytes_writer_t *writer, int64_t val) {
    return z_bytes_writer_serialize_uint64(writer, (uint64_t)val);
}

/**
 * Deserializes next portion of data into a `uint8_t` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`uint8_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_uint8(z_bytes_reader_t *reader, uint8_t *val) {
    return z_bytes_reader_read(reader, val, 1) == 1 ? _Z_RES_OK : _Z_ERR_DID_NOT_READ;
}

/**
 * Deserializes next portion of data into a `uint16_t` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`uint16_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_uint16(z_bytes_reader_t *reader, uint16_t *val) {
    if (z_bytes_reader_read(reader, (uint8_t *)val, sizeof(uint16_t)) != sizeof(uint16_t)) {
        return _Z_ERR_DID_NOT_READ;
    }
    *val = _z_host_le_load16((const uint8_t *)val);
    return _Z_RES_OK;
}

/**
 * Deserializes next portion of data into a `uint32_t` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`uint32_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_uint32(z_bytes_reader_t *reader, uint32_t *val) {
    if (z_bytes_reader_read(reader, (uint8_t *)val, sizeof(uint32_t)) != sizeof(uint32_t)) {
        return _Z_ERR_DID_NOT_READ;
    }
    *val = _z_host_le_load32((uint8_t *)val);
    return _Z_RES_OK;
}

/**
 * Deserializes next portion of data into a `uint64_t` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`uint64_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_uint64(z_bytes_reader_t *reader, uint64_t *val) {
    if (z_bytes_reader_read(reader, (uint8_t *)val, sizeof(uint64_t)) != sizeof(uint64_t)) {
        return _Z_ERR_DID_NOT_READ;
    }
    *val = _z_host_le_load64((uint8_t *)val);
    return _Z_RES_OK;
}

/**
 * Deserializes next portion of data into a `float` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`float` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_float(z_bytes_reader_t *reader, float *val) {
    if (z_bytes_reader_read(reader, (uint8_t *)val, sizeof(float)) != sizeof(float)) {
        return _Z_ERR_DID_NOT_READ;
    }
    return _Z_RES_OK;
}

/**
 * Deserializes next portion of data into a `double` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`double` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_double(z_bytes_reader_t *reader, double *val) {
    if (z_bytes_reader_read(reader, (uint8_t *)val, sizeof(double)) != sizeof(double)) {
        return _Z_ERR_DID_NOT_READ;
    }
    return _Z_RES_OK;
}

/**
 * Deserializes next portion of data into a `int8_t` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`int8_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_int8(z_bytes_reader_t *reader, int8_t *val) {
    return z_bytes_reader_deserialize_uint8(reader, (uint8_t *)val);
}

/**
 * Deserializes next portion of data into a `int16_t` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`int16_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_int16(z_bytes_reader_t *reader, int16_t *val) {
    return z_bytes_reader_deserialize_uint16(reader, (uint16_t *)val);
}

/**
 * Deserializes next portion of data into a `int32_t` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`int32_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_int32(z_bytes_reader_t *reader, int32_t *val) {
    return z_bytes_reader_deserialize_uint32(reader, (uint32_t *)val);
}

/**
 * Deserializes next portion of data into a `int64_t` and advances the reader.
 *
 * Parameters:
 *   reader: A reader instance.
 *   dst: Pointer to an uninitialized :c:type:`int64_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
static inline z_result_t z_bytes_reader_deserialize_int64(z_bytes_reader_t *reader, int64_t *val) {
    return z_bytes_reader_deserialize_uint64(reader, (uint64_t *)val);
}

/**
 * Serializes array of bytes and writes it into an underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: Pointer to the data to serialize.
 *   len: Number of bytes to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_writer_serialize_buf(z_bytes_writer_t *writer, const uint8_t *val, size_t len);

/**
 * Serializes slice and writes it into an underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: A slice to serialize.
 *   len: Number of bytes to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_writer_serialize_slice(z_bytes_writer_t *writer, const z_loaned_slice_t *val);

/**
 * Deserializes next portion of data and advances the reader position.
 *
 * Parameters:
 *   reader: A reader instance.
 *   val: Pointer to an uninitialized :c:type:`z_owned_slice_t` to contain the deserialized slice.
 *
 * Return:
 *   ``0`` if deserialization is successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_reader_deserialize_slice(z_bytes_reader_t *reader, z_owned_slice_t *val);

/**
 * Serializes a null-terminated string and writes it into an underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: Pointer to the string to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_writer_serialize_str(z_bytes_writer_t *writer, const char *val);

/**
 * Serializes a string and writes it into an underlying :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   writer: A writer instance.
 *   val: Pointer to the string to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_writer_serialize_string(z_bytes_writer_t *writer, const z_loaned_string_t *val);

/**
 * Deserializes next portion of data and advances the reader position.
 *
 * Parameters:
 *   reader: A reader instance.
 *   val: Pointer to an uninitialized :c:type:`z_owned_string_t` to contain the deserialized string.
 *
 * Return:
 *   ``0`` if deserialization is successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_reader_deserialize_string(z_bytes_reader_t *reader, z_owned_string_t *val);

/**
 * Initiate serialization of a sequence of multiple elements.
 *
 * Parameters:
 *   writer: A writer instance.
 *   len: Length of the sequence. Could be read during deserialization using
 * :c:func:`z_bytes_reader_deserialize_sequence_begin`.
 *
 * Return:
 *   ``0`` if deserialization is successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_writer_serialize_sequence_begin(z_bytes_writer_t *writer, size_t len);

/**
 * Finalize serialization of a sequence of multiple elements.
 *
 * Parameters:
 *   writer: A writer instance.
 *
 * Return:
 *   ``0`` if deserialization is successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_writer_serialize_sequence_end(z_bytes_writer_t *writer);

/**
 * Initiate deserialization of a sequence of multiple elements.
 *
 * Parameters:
 *   reader: A reader instance.
 *   len: A pointer where the length of the sequence (previously passed via
 * :c:func:`z_bytes_writer_serialize_sequence_begin`) will be written.
 *
 * Return:
 *   ``0`` if deserialization is successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_reader_deserialize_sequence_begin(z_bytes_reader_t *reader, size_t *len);

/**
 * Finalize deserialization of a sequence of multiple elements.
 *
 * Parameters:
 *   reader: A reader instance.
 *
 * Return:
 *   ``0`` if deserialization is successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_reader_deserialize_sequence_end(z_bytes_reader_t *reader);

/**
 * Serializes data into a :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized data.
 *   data: Pointer to the data to serialize. Ownership is transferred to the `bytes`.
 *   len: Number of bytes to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_buf(z_owned_bytes_t *bytes, const uint8_t *data, size_t len);

/**
 * Serializes a string into a :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized string.
 *   s: Pointer to the string to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_string(z_owned_bytes_t *bytes, const z_loaned_string_t *s);

/**
 * Serializes a null-terminated string into a :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized string.
 *   value: Pointer to the string to serialize. Ownership is transferred to the `bytes`.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_str(z_owned_bytes_t *bytes, const char *value);

/**
 * Serializes a slice into a :c:type:`z_owned_bytes_t`.
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized slice.
 *   slice: Pointer to the slice to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_slice(z_owned_bytes_t *bytes, const z_loaned_slice_t *slice);

/**
 * Serializes a :c:type:`int8_t` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `int8_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_int8(z_owned_bytes_t *bytes, int8_t val);

/**
 * Serializes a :c:type:`int16_t` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `int16_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_int16(z_owned_bytes_t *bytes, int16_t val);

/**
 * Serializes a :c:type:`int32_t` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `int32_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_int32(z_owned_bytes_t *bytes, int32_t val);

/**
 * Serializes a :c:type:`int64_t` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `int64_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_int64(z_owned_bytes_t *bytes, int64_t val);

/**
 * Serializes a :c:type:`uint8_t` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `uint8_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_uint8(z_owned_bytes_t *bytes, uint8_t val);

/**
 * Serializes a :c:type:`uint16_t` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `uint16_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_uint16(z_owned_bytes_t *bytes, uint16_t val);

/**
 * Serializes a :c:type:`uint32_t` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `uint32_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_uint32(z_owned_bytes_t *bytes, uint32_t val);

/**
 * Serializes a :c:type:`uint64_t` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `uint64_t` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_uint64(z_owned_bytes_t *bytes, uint64_t val);

/**
 * Serializes a :c:type:`float` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `float` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_float(z_owned_bytes_t *bytes, float val);

/**
 * Serializes a :c:type:`double` into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the serialized int.
 *   val: `double` value to serialize.
 *
 * Return:
 *   ``0`` if serialization is successful, ``negative value`` otherwise.
 */
z_result_t z_bytes_serialize_from_double(z_owned_bytes_t *bytes, double val);

/**
 * Deserializes data into a :c:type:`z_owned_slice_t`
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   str: Pointer to an uninitialized :c:type:`z_owned_slice_t` to contain the deserialized slice.
 *
 * Return:
 *   ``0`` if deserialization is successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_slice(const z_loaned_bytes_t *bytes, z_owned_slice_t *dst);

/**
 * Deserializes data into a :c:type:`z_owned_string_t`
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   str: Pointer to an uninitialized :c:type:`z_owned_string_t` to contain the deserialized string.
 *
 * Return:
 *   ``0`` if deserialization is successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_string(const z_loaned_bytes_t *bytes, z_owned_string_t *str);

/**
 * Deserializes data into a `:c:type:`int8_t`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`int8_t` to contain the deserialized.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_int8(const z_loaned_bytes_t *bytes, int8_t *dst);

/**
 * Deserializes data into a :c:type:`int16_t`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`int16_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_int16(const z_loaned_bytes_t *bytes, int16_t *dst);

/**
 * Deserializes data into a :c:type:`int32_t`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`int32_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_int32(const z_loaned_bytes_t *bytes, int32_t *dst);

/**
 * Deserializes data into a :c:type:`int64_t`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`int64_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_int64(const z_loaned_bytes_t *bytes, int64_t *dst);

/**
 * Deserializes data into a :c:type:`uint8_t`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`uint8_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_uint8(const z_loaned_bytes_t *bytes, uint8_t *dst);

/**
 * Deserializes data into a :c:type:`uint16_t`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`uint16_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_uint16(const z_loaned_bytes_t *bytes, uint16_t *dst);

/**
 * Deserializes data into a :c:type:`uint32_t`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`uint32_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_uint32(const z_loaned_bytes_t *bytes, uint32_t *dst);

/**
 * Deserializes data into a `:c:type:`uint64_t`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`uint64_t` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_uint64(const z_loaned_bytes_t *bytes, uint64_t *dst);

/**
 * Deserializes data into a `:c:type:`float`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`float` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_float(const z_loaned_bytes_t *bytes, float *dst);

/**
 * Deserializes data into a :c:type:`double`.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to deserialize.
 *   dst: Pointer to an uninitialized :c:type:`double` to contain the deserialized number.
 *
 * Return:
 *   ``0`` if deserialization successful, or a ``negative value`` otherwise.
 */
z_result_t z_bytes_deserialize_into_double(const z_loaned_bytes_t *bytes, double *dst);

#ifdef __cplusplus
}
#endif

#endif
