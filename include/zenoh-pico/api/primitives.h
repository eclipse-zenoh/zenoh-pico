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
//

#ifndef INCLUDE_ZENOH_PICO_API_PRIMITIVES_H
#define INCLUDE_ZENOH_PICO_API_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>

#include "olv_macros.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/********* Data Types Handlers *********/
/**
 * Builds a :c:type:`z_view_string_t` by wrapping a ``const char *`` string.
 *
 * Parameters:
 *   value: Pointer to a null terminated string.
 *   str: Pointer to an uninitialized :c:type:`z_view_string_t`.
 *
 * Return:
 *   ``0`` if creation successful, ``negative value`` otherwise.
 */
int8_t z_view_string_wrap(z_view_string_t *str, const char *value);

/**
 * Builds a :c:type:`z_keyexpr_t` from a null-terminated string.
 * It is a loaned key expression that aliases ``name``.
 * Unlike it's counterpart in zenoh-c, this function does not test passed expression to correctness.
 *
 * Parameters:
 *   name: Pointer to string representation of the keyexpr as a null terminated string.
 *   keyexpr: Pointer to an uninitialized :c:type:`z_view_keyexpr_t`.
 *
 * Return:
 *   ``0`` if creation successful, ``negative value`` otherwise.
 */
int8_t z_view_keyexpr_from_str(z_view_keyexpr_t *keyexpr, const char *name);

/**
 * Builds a :c:type:`z_keyexpr_t` from a null-terminated string.
 * It is a loaned key expression that aliases ``name``.
 * Input key expression is not checked for correctness.
 *
 * Parameters:
 *   name: Pointer to string representation of the keyexpr as a null terminated string.
 *   keyexpr: Pointer to an uninitialized :c:type:`z_view_keyexpr_t`.
 *
 * Return:
 *   ``0`` if creation successful, ``negative value`` otherwise.
 */
int8_t z_view_keyexpr_from_str_unchecked(z_view_keyexpr_t *keyexpr, const char *name);

/**
 * Gets a null-terminated string from a :c:type:`z_keyexpr_t`.
 *
 * If given keyexpr contains a declared keyexpr, the resulting owned string will be uninitialized.
 * In that case, the user must use :c:func:`zp_keyexpr_resolve` to resolve the nesting declarations
 * and get its full expanded representation.
 *
 * Parameters:
 *   keyexpr: Pointer to a loaned instance of :c:type:`z_keyexpr_t`.
 *   str: Pointer to an uninitialized :c:type:`z_owned_string_t`.
 *
 * Return:
 *   ``0`` if creation successful, ``negative value`` otherwise.
 */
int8_t z_keyexpr_to_string(const z_loaned_keyexpr_t *keyexpr, z_owned_string_t *str);

/**
 * Builds a null-terminated string from a :c:type:`z_loaned_keyexpr_t` for a given
 * :c:type:`z_loaned_session_t`.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to resolve the keyexpr.
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to be resolved.
 *   str: Pointer to an uninitialized :c:type:`z_owned_string_t`.
 *
 * Return:
 *   ``0`` if creation successful, ``negative value`` otherwise.
 */
int8_t zp_keyexpr_resolve(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, z_owned_string_t *str);

/**
 * Checks if a given keyexpr is valid.
 *
 * Parameters:
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to be checked.
 *
 * Return:
 *   ``true`` if keyexpr is valid, or ``false`` otherwise.
 */
_Bool z_keyexpr_is_initialized(const z_loaned_keyexpr_t *keyexpr);

/**
 * Checks if a given keyexpr is valid and in canonical form.
 *
 * Parameters:
 *   start: Pointer to the keyexpr in its string representation as a non-null terminated string.
 *   len: Number of characters in ``start``.
 *
 * Return:
 *   ``0`` if passed string is a valid (and canon) key expression, or a ``negative value`` otherwise.
 *   Error codes are defined in :c:enum:`zp_keyexpr_canon_status_t`.
 */
int8_t z_keyexpr_is_canon(const char *start, size_t len);

/**
 * Checks if a given keyexpr is valid and in canonical form.
 *
 * Parameters:
 *   start: Pointer to the keyexpr in its string representation as a null terminated string.
 *   len: Number of characters in ``start``.
 *
 * Return:
 *   ``0`` if passed string is a valid (and canon) key expression, or a ``negative value`` otherwise.
 *   Error codes are defined in :c:enum:`zp_keyexpr_canon_status_t`.
 */
int8_t zp_keyexpr_is_canon_null_terminated(const char *start);

/**
 * Canonizes of a given keyexpr in string representation.
 * The canonization is performed over the passed string, possibly shortening it by modifying ``len``.
 *
 * Parameters:
 *   start: Pointer to the keyexpr in its string representation as a non-null terminated string.
 *   len: Number of characters in ``start``.
 *
 * Return:
 *   ``0`` if canonization successful, or a ``negative value`` otherwise.
 *   Error codes are defined in :c:enum:`zp_keyexpr_canon_status_t`.
 */
int8_t z_keyexpr_canonize(char *start, size_t *len);

/**
 * Canonizes a given keyexpr in string representation.
 * The canonization is performed over the passed string, possibly shortening it by modifying ``len``.
 *
 * Parameters:
 *   start: Pointer to the keyexpr in its string representation as a null terminated string.
 *   len: Number of characters in ``start``.
 *
 * Return:
 *   ``0`` if canonization successful, or a ``negative value`` otherwise.
 *   Error codes are defined in :c:enum:`zp_keyexpr_canon_status_t`.
 */
int8_t zp_keyexpr_canonize_null_terminated(char *start);

/**
 * Checks if a given keyexpr contains another keyexpr in its set.
 *
 * Parameters:
 *   l: Pointer to a :c:type:`z_loaned_keyexpr_t`.
 *   r: Pointer to a :c:type:`z_loaned_keyexpr_t`.
 *
 * Return:
 *   ``true`` if ``l`` includes ``r``, i.e. the set defined by ``l`` contains every key belonging to the set
 * defined by ``r``. Otherwise, returns ``false``.
 */
_Bool z_keyexpr_includes(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r);

/**
 * Checks if a given keyexpr contains another keyexpr in its set.
 *
 * Parameters:
 *   l: Pointer to the keyexpr in its string representation as a null terminated string.
 *   llen: Number of characters in ``l``.
 *   r: Pointer to the keyexpr in its string representation as a null terminated string.
 *   rlen: Number of characters in ``r``.
 *
 * Return:
 *   ``true`` if ``l`` includes ``r``, i.e. the set defined by ``l`` contains every key belonging to the set
 * defined by ``r``. Otherwise, returns ``false``.
 */
_Bool zp_keyexpr_includes_null_terminated(const char *l, const char *r);

/**
 * Checks if a given keyexpr intersects with another keyexpr.
 *
 * Parameters:
 *   l: Pointer to a :c:type:`z_loaned_keyexpr_t`.
 *   r: Pointer to a :c:type:`z_loaned_keyexpr_t`.
 *
 * Return:
 *   ``true`` if keyexprs intersect, i.e. there exists at least one key which is contained in both of the
 * sets defined by ``l`` and ``r``. Otherwise, returns ``false``.
 */
_Bool z_keyexpr_intersects(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r);

/**
 * Checks if a given keyexpr intersects with another keyexpr.
 *
 * Parameters:
 *   l: Pointer to the keyexpr in its string representation as a null terminated string.
 *   llen: Number of characters in ``l``.
 *   r: Pointer to the keyexpr in its string representation as a null terminated string.
 *   rlen: Number of characters in ``r``.
 *
 * Return:
 *   ``true`` if keyexprs intersect, i.e. there exists at least one key which is contained in both of the
 * sets defined by ``l`` and ``r``. Otherwise, returns ``false``.
 */
_Bool zp_keyexpr_intersect_null_terminated(const char *l, const char *r);

/**
 * Checks if two keyexpr are equal.
 *
 * Parameters:
 *   l: Pointer to a :c:type:`z_loaned_keyexpr_t`.
 *   r: Pointer to a :c:type:`z_loaned_keyexpr_t`.
 *
 * Return:
 *   ``true`` if both ``l`` and ``r`` are equal. Otherwise, returns  ``false``.
 */
_Bool z_keyexpr_equals(const z_loaned_keyexpr_t *l, const z_loaned_keyexpr_t *r);

/**
 * Checks if two keyexpr as null terminated string are equal.
 *
 * Parameters:
 *   l: Pointer to the keyexpr in its string representation as a null terminated string.
 *   llen: Number of characters in ``l``.
 *   r: Pointer to the keyexpr in its string representation as a null terminated string.
 *   rlen: Number of characters in ``r``.
 *
 * Return:
 *   ``true`` if both ``l`` and ``r`` are equal. Otherwise, it returns ``false``.
 */
_Bool zp_keyexpr_equals_null_terminated(const char *l, const char *r);

/**
 * Builds a new, zenoh-allocated, empty configuration.
 * It consists in an empty set of properties for zenoh session configuration.
 *
 * Parameters:
 *   config: Pointer to uninitialized :c:type:`z_owned_config_t`.
 */
void z_config_new(z_owned_config_t *config);

/**
 * Builds a new, zenoh-allocated, default configuration.
 * It consists in a default set of properties for zenoh session configuration.
 *
 * Parameters:
 *   config: Pointer to uninitialized :c:type:`z_owned_config_t`.
 */
void z_config_default(z_owned_config_t *config);

/**
 * Gets the property with the given integer key from the configuration.
 *
 * Parameters:
 *   config: Pointer to a :c:type:`z_loaned_config_t` to get the property from.
 *   key: Integer key of the requested property.
 *
 * Return:
 *   The requested property value.
 */
const char *zp_config_get(const z_loaned_config_t *config, uint8_t key);

/**
 * Inserts or replaces the property with the given integer key in the configuration.
 *
 * Parameters:
 *   config: Pointer to a :c:type:`z_loaned_config_t` to modify.
 *   key: Integer key of the property to be inserted.
 *   value: Property value to be inserted.
 *
 * Return:
 *   ``0`` if insertion successful, ``negative value`` otherwise.
 */
int8_t zp_config_insert(z_loaned_config_t *config, uint8_t key, const char *value);

/**
 * Builds a :c:type:`z_owned_encoding_t` from a null terminated string.
 *
 * Parameters:
 *   encoding: Pointer to an uninitialized :c:type:`z_owned_encoding_t`.
 *   s: Pointer to the null terminated string to use.
 *
 * Return:
 *   ``0`` if creation successful,``negative value`` otherwise.
 */
int8_t z_encoding_from_str(z_owned_encoding_t *encoding, const char *s);

/**
 * Builds a string from a :c:type:`z_loaned_encoding_t`.
 *
 * Parameters:
 *   encoding: Pointer to the :c:type:`z_loaned_encoding_t` to use.
 *   string: Pointer to an uninitialized :c:type:`z_owned_string_t` to store the string.
 *
 * Return:
 *   ``0`` if creation successful,``negative value`` otherwise.
 */
int8_t z_encoding_to_string(const z_loaned_encoding_t *encoding, z_owned_string_t *string);

/**
 * Gets the bytes data from a reply error payload by aliasing it.
 *
 * Parameters:
 *    reply_err: Pointer to a :c:type:`z_loaned_reply_err_t` to get data from.
 *
 * Return:
 *    Pointer to the data as a :c:type:`z_loaned_bytes_t`.
 */
const z_loaned_bytes_t *z_reply_err_payload(const z_loaned_reply_err_t *reply_err);

/**
 * Gets a reply error encoding by aliasing it.
 *
 * Parameters:
 *   query: Pointer to the :c:type:`z_loaned_reply_err_t` to get the encoding from.
 *
 * Return:
 *   Pointer to the encoding as a :c:type:`z_loaned_encoding_t`.
 */
const z_loaned_encoding_t *z_reply_err_encoding(const z_loaned_reply_err_t *reply_err);

/**
 * Gets date pointer of a bytes array.
 *
 * Parameters:
 *    slice: Pointer to a :c:type:`z_loaned_slice_t` to get data from.
 *
 * Return:
 *    The data pointer.
 */
const uint8_t *z_slice_data(const z_loaned_slice_t *slice);

/**
 * Gets total number of bytes in a bytes array.
 *
 * Parameters:
 *    slice: Pointer to a :c:type:`z_loaned_slice_t` to get length from.
 *
 * Return:
 *    The number of bytes.
 */
size_t z_slice_len(const z_loaned_slice_t *slice);

/**
 * Decodes data into a `int8_t` signed integer.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`int8_t` to contain the decoded int.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_int8(const z_loaned_bytes_t *bytes, int8_t *dst);

/**
 * Decodes data into a `int16_t` signed integer.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`int16_t` to contain the decoded int.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_int16(const z_loaned_bytes_t *bytes, int16_t *dst);

/**
 * Decodes data into a `int32_t` signed integer.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`int32_t` to contain the decoded int.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_int32(const z_loaned_bytes_t *bytes, int32_t *dst);

/**
 * Decodes data into a `int64_t` signed integer.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`int64_t` to contain the decoded int.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_int64(const z_loaned_bytes_t *bytes, int64_t *dst);

/**
 * Decodes data into a `uint8_t` unsigned integer.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`uint8_t` to contain the decoded int.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_uint8(const z_loaned_bytes_t *bytes, uint8_t *dst);

/**
 * Decodes data into a `uint16_t` unsigned integer.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`uint16_t` to contain the decoded int.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_uint16(const z_loaned_bytes_t *bytes, uint16_t *dst);

/**
 * Decodes data into a `uint32_t` unsigned integer.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`uint32_t` to contain the decoded int.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_uint32(const z_loaned_bytes_t *bytes, uint32_t *dst);

/**
 * Decodes data into a `uint64_t` unsigned integer.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`uint64_t` to contain the decoded int.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_uint64(const z_loaned_bytes_t *bytes, uint64_t *dst);

/**
 * Decodes data into a `float` floating number.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`float` to contain the decoded float.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_float(const z_loaned_bytes_t *bytes, float *dst);

/**
 * Decodes data into a `double` floating number.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   dst: Pointer to an uninitialized :c:type:`double` to contain the decoded float.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_double(const z_loaned_bytes_t *bytes, double *dst);

/**
 * Decodes data into a :c:type:`z_owned_slice_t`
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   str: Pointer to an uninitialized :c:type:`z_owned_slice_t` to contain the decoded slice.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_slice(const z_loaned_bytes_t *bytes, z_owned_slice_t *dst);

/**
 * Decodes data into a :c:type:`z_owned_string_t`
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   str: Pointer to an uninitialized :c:type:`z_owned_string_t` to contain the decoded string.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_string(const z_loaned_bytes_t *bytes, z_owned_string_t *str);

/**
 * Decodes data into a pair of :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 *   first: Pointer to an uninitialized :c:type:`z_owned_bytes_t` to contain the first element.
 *   second: Pointer to an uninitialized :c:type:`z_owned_bytes_t` to contain the second element.
 *
 * Return:
 *   ``0`` if decode successful, or a ``negative value`` otherwise.
 */
int8_t z_bytes_deserialize_into_pair(const z_loaned_bytes_t *bytes, z_owned_bytes_t *first, z_owned_bytes_t *second);

/**
 * Encodes a signed integer into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `int8_t` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_int8(z_owned_bytes_t *bytes, int8_t val);

/**
 * Encodes a signed integer into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `int16_t` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_int16(z_owned_bytes_t *bytes, int16_t val);

/**
 * Encodes a signed integer into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `int32_t` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_int32(z_owned_bytes_t *bytes, int32_t val);

/**
 * Encodes a signed integer into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `int64_t` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_int64(z_owned_bytes_t *bytes, int64_t val);

/**
 * Encodes an unsigned integer into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `uint8_t` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_uint8(z_owned_bytes_t *bytes, uint8_t val);

/**
 * Encodes an unsigned integer into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `uint16_t` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_uint16(z_owned_bytes_t *bytes, uint16_t val);

/**
 * Encodes an unsigned integer into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `uint32_t` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_uint32(z_owned_bytes_t *bytes, uint32_t val);

/**
 * Encodes an unsigned integer into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `uint64_t` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_uint64(z_owned_bytes_t *bytes, uint64_t val);

/**
 * Encodes a floating number into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `float` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_float(z_owned_bytes_t *bytes, float val);

/**
 * Encodes a floating number into a :c:type:`z_owned_bytes_t`
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded int.
 *   val: `double` value to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_double(z_owned_bytes_t *bytes, double val);

/**
 * Encodes a slice into a :c:type:`z_owned_bytes_t` by aliasing
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded slice.
 *   str: Pointer to the slice to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_slice(z_owned_bytes_t *bytes, const uint8_t *data, size_t len);

/**
 * Encodes a slice into a :c:type:`z_owned_bytes_t` by copying
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded slice.
 *   str: Pointer to the slice to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_slice_copy(z_owned_bytes_t *bytes, const uint8_t *data, size_t len);

/**
 * Encodes a string into a :c:type:`z_owned_bytes_t` by aliasing
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded string.
 *   s: Pointer to the string to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_str(z_owned_bytes_t *bytes, const char *s);

/**
 * Encodes a string into a :c:type:`z_owned_bytes_t` by copying
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded string.
 *   str: Pointer to the string to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_str_copy(z_owned_bytes_t *bytes, const char *s);

/**
 * Constructs payload from an iterator to `z_owned_bytes_t`.
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded payload.
 *   iterator_body: Iterator body function, providing data items. Returning false is treated as iteration end.
 *   context: Arbitrary context that will be passed to iterator_body.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_iter(z_owned_bytes_t *bytes, _Bool (*iterator_body)(z_owned_bytes_t *data, void *context),
                                   void *context);

/**
 * Append a pair of `z_owned_bytes` objects which are consumed in the process.
 *
 * Parameters:
 *   bytes: An uninitialized :c:type:`z_owned_bytes_t` to contain the encoded pair.
 *   first: Pointer to the first `z_owned_bytes` to encode.
 *   second: Pointer to the second `z_owned_bytes` to encode.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_serialize_from_pair(z_owned_bytes_t *bytes, z_owned_bytes_t *first, z_owned_bytes_t *second);

/**
 * Parameters:
 *   bytes: Pointer to an unitialized :c:type:`z_lowned_bytes_t` instance.
 */
void z_bytes_empty(z_owned_bytes_t *bytes);

/**
 * Returns total number of bytes in the container.
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 * Return:
 *  Number of bytes in the container.
 */
size_t z_bytes_len(const z_loaned_bytes_t *bytes);

/**
 * Checks if container is empty
 *
 * Parameters:
 *   bytes: Pointer to a :c:type:`z_loaned_bytes_t` to decode.
 * Return:
 *  ``true`` if conainer is empty,  ``false`` otherwise.
 */
_Bool z_bytes_is_empty(const z_loaned_bytes_t *bytes);

/**
 * Returns an iterator for multi-element serialized data.
 *
 * Parameters:
 *   bytes: Data to iterate over.
 *
 * Return:
 *   The constructed :c:type:`z_bytes_iterator_t`.
 */
z_bytes_iterator_t z_bytes_get_iterator(const z_loaned_bytes_t *bytes);

/**
 * Constructs :c:type:`z_owned_bytes_t` object corresponding to the next element of serialized data.
 *
 * Will construct null-state `z_owned_bytes_t` when iterator reaches the end (or in case of error).
 *
 * Parameters:
 *   iter: An iterator over multi-element serialized data.
 *   out: An uninitialized :c:type:`z_owned_bytes_t` that will contained next serialized element.
 * Return:
 *  ``false`` when iterator reaches the end,  ``true`` otherwise.
 */
_Bool z_bytes_iterator_next(z_bytes_iterator_t *iter, z_owned_bytes_t *out);

/**
 * Returns a reader for the `bytes`.
 *
 * The `bytes` should outlive the reader and should not be modified, while reader is in use.
 *
 * Parameters:
 *   bytes: Data to read.
 *
 * Return:
 *   The constructed :c:type:`z_bytes_reader_t`.
 */
z_bytes_reader_t z_bytes_get_reader(const z_loaned_bytes_t *bytes);

/**
 * Reads data into specified destination.
 *
 * Parameters:
 *  reader: Data reader to read from.
 *  dst: Buffer where the read data is written.
 *  len: Maximum number of bytes to read.
 *
 * Return:
 *  Number of bytes read. If return value is smaller than `len`, it means that the end of the data was reached.
 */
size_t z_bytes_reader_read(z_bytes_reader_t *reader, uint8_t *dst, size_t len);
/**
 * Sets the `reader` position indicator for the payload to the value pointed to by offset.
 * The new position is exactly `offset` bytes measured from the beginning of the payload if origin is `SEEK_SET`,
 * from the current reader position if origin is `SEEK_CUR`, and from the end of the payload if origin is `SEEK_END`.
 *
 * Parameters:
 *  reader: Data reader to reposition.
 *  offset: New position ffset in bytes.
 *  origin: Origin for the new position.
 *
 * Return:
 *  ​0​ upon success, negative error code otherwise.
 */
int8_t z_bytes_reader_seek(z_bytes_reader_t *reader, int64_t offset, int origin);
/**
 * Gets the read position indicator.
 *
 * Parameters:
 *  reader: Data reader to get position of.
 *
 * Return:
 *  Read position indicator on success or -1L if failure occurs.
 */
int64_t z_bytes_reader_tell(z_bytes_reader_t *reader);

/**
 * Constructs writer for :c:type:`z_loaned_bytes_t`.
 *
 * Parameters:
 *   bytes: Data container to write to.
 *   writer: Uninitialized memory location where writer is to be constructed.
 *
 */
void z_bytes_get_writer(z_loaned_bytes_t *bytes, z_owned_bytes_writer_t *writer);

/**
 * Writes `len` bytes from `src` into underlying :c:type:`z_loaned_bytes_t.
 *
 * Parameters:
 *   writer: A data writer
 *   src: Buffer to write from.
 *   len: Number of bytes to write.
 *
 * Return:
 *   ``0`` if encode successful, ``negative value`` otherwise.
 */
int8_t z_bytes_writer_write(z_loaned_bytes_writer_t *writer, const uint8_t *src, size_t len);

/**
 * Checks validity of a timestamp
 *
 * Parameters:
 *   ts: Timestamp value to check validity of.
 *
 * Return:
 *   ``true`` if timestamp is valid, ``false`` otherwise.
 */
_Bool z_timestamp_check(z_timestamp_t ts);

/**
 * Builds a default query target.
 *
 * Return:
 *   The constructed :c:type:`z_query_target_t`.
 */
z_query_target_t z_query_target_default(void);

/**
 * Builds an automatic query consolidation :c:type:`z_query_consolidation_t`.
 *
 * A query consolidation strategy will automatically be selected depending on the query selector.
 * If selector contains time range properties, no consolidation is performed.
 * Otherwise the :c:func:`z_query_consolidation_latest` strategy is used.
 *
 * Return:
 *   The constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_auto(void);

/**
 * Builds a default :c:type:`z_query_consolidation_t`.
 *
 * Return:
 *   The constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_default(void);

/**
 * Builds a latest query consolidation :c:type:`z_query_consolidation_t`.
 *
 * This strategy optimizes bandwidth on all links in the system but will provide a very poor latency.
 *
 * Return:
 *   The constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_latest(void);

/**
 * Builds a monotonic query consolidation :c:type:`z_query_consolidation_t`.
 *
 * This strategy offers the best latency. Replies are directly transmitted to the application when received
 * without needing to wait for all replies. This mode does not guarantee that there will be no duplicates.
 *
 * Return:
 *   The constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_monotonic(void);

/**
 * Builds a no query consolidation :c:type:`z_query_consolidation_t`.
 *
 * This strategy is useful when querying timeseries data bases or when using quorums.
 *
 * Return:
 *   The constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_none(void);

/**
 * Gets a query parameters field.
 *
 * Parameters:
 *   query: Pointer to the :c:type:`z_loaned_query_t` to get the parameters from.
 *   parameters: Pointer to an uninitialized :c:type:`z_view_string_t` to contain the parameters.
 */
void z_query_parameters(const z_loaned_query_t *query, z_view_string_t *parameters);

/**
 * Gets a query payload by aliasing it.
 *
 * Parameters:
 *   query: Pointer to the :c:type:`z_loaned_query_t` to get the value from.
 *
 * Return:
 *   Pointer to the payload as a :c:type:`z_loaned_bytes_t`.
 */
const z_loaned_bytes_t *z_query_payload(const z_loaned_query_t *query);

/**
 * Gets a query encoding by aliasing it.
 *
 * Parameters:
 *   query: Pointer to the :c:type:`z_loaned_query_t` to get the value from.
 *
 * Return:
 *   Pointer to the encoding as a :c:type:`z_loaned_encoding_t`.
 */
const z_loaned_encoding_t *z_query_encoding(const z_loaned_query_t *query);

/**
 * Gets a query attachment value by aliasing it.
 *
 * Parameters:
 *   query: Pointer to the :c:type:`z_loaned_query_t` to get the attachment from.
 *
 * Return:
 *   Pointer to the attachment as a :c:type:`z_loaned_bytes_t`.
 */
const z_loaned_bytes_t *z_query_attachment(const z_loaned_query_t *query);

/**
 * Gets a query keyexpr by aliasing it.
 *
 * Parameters:
 *   query: Pointer to the :c:type:`z_loaned_query_t` to get the keyexpr from.
 *
 * Return:
 *   The keyexpr wrapped as a:c:type:`z_keyexpr_t`.
 */
const z_loaned_keyexpr_t *z_query_keyexpr(const z_loaned_query_t *query);

/**
 * Builds a new sample closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Parameters:
 *   call: Pointer to the callback function. ``context`` will be passed as its last argument.
 *   drop: Pointer to the function that will free the callback state. ``context`` will be passed as its last argument.
 *   context: Pointer to an arbitrary state.
 *
 * Return:
 *   The sample closure.
 */
int8_t z_closure_sample(z_owned_closure_sample_t *closure, z_data_handler_t call, z_dropper_handler_t drop,
                        void *context);

/**
 * Builds a new query closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Parameters:
 *   call: Pointer to the callback function. ``context`` will be passed as its last argument.
 *   drop: Pointer to the function that will free the callback state. ``context`` will be passed as its last argument.
 *   context: Pointer to an arbitrary state.
 *
 * Return:
 *   The query closure.
 */
int8_t z_closure_query(z_owned_closure_query_t *closure, z_queryable_handler_t call, z_dropper_handler_t drop,
                       void *context);

/**
 * Builds a new reply closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Parameters:
 *   call: Pointer to the callback function. ``context`` will be passed as its last argument.
 *   drop: Pointer to the function that will free the callback state. ``context`` will be passed as its last argument.
 *   context: Pointer to an arbitrary state.
 *
 * Return:
 *   The reply closure.
 */
int8_t z_closure_reply(z_owned_closure_reply_t *closure, z_reply_handler_t call, z_dropper_handler_t drop,
                       void *context);

/**
 * Builds a new hello closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Parameters:
 *   call: Pointer to the callback function. ``context`` will be passed as its last argument.
 *   drop: Pointer to the function that will free the callback state. ``context`` will be passed as its last argument.
 *   context: Pointer to an arbitrary state.
 *
 * Return:
 *   The hello closure.
 */
int8_t z_closure_hello(z_owned_closure_hello_t *closure, z_loaned_hello_handler_t call, z_dropper_handler_t drop,
                       void *context);

/**
 * Builds a new zid closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Parameters:
 *   call: Pointer to the callback function. ``context`` will be passed as its last argument.
 *   drop: Pointer to the function that will free the callback state. ``context`` will be passed as its last argument.
 *   context: Pointer to an arbitrary state.
 *
 * Return:
 *   The hello closure.
 */
int8_t z_closure_zid(z_owned_closure_zid_t *closure, z_id_handler_t call, z_dropper_handler_t drop, void *context);

/**************** Loans ****************/
_Z_OWNED_FUNCTIONS_DEF(string)
_Z_OWNED_FUNCTIONS_DEF(keyexpr)
_Z_OWNED_FUNCTIONS_NO_COPY_DEF(config)
_Z_OWNED_FUNCTIONS_DEF(session)
_Z_OWNED_FUNCTIONS_DEF(subscriber)
_Z_OWNED_FUNCTIONS_DEF(publisher)
_Z_OWNED_FUNCTIONS_DEF(queryable)
_Z_OWNED_FUNCTIONS_DEF(hello)
_Z_OWNED_FUNCTIONS_DEF(reply)
_Z_OWNED_FUNCTIONS_DEF(string_array)
_Z_OWNED_FUNCTIONS_DEF(sample)
_Z_OWNED_FUNCTIONS_DEF(query)
_Z_OWNED_FUNCTIONS_DEF(slice)
_Z_OWNED_FUNCTIONS_DEF(bytes)
_Z_OWNED_FUNCTIONS_DEF(bytes_writer)
_Z_OWNED_FUNCTIONS_DEF(reply_err)
_Z_OWNED_FUNCTIONS_DEF(encoding)

_Z_OWNED_FUNCTIONS_CLOSURE_DEF(z_owned_closure_sample_t, closure_sample)
_Z_OWNED_FUNCTIONS_CLOSURE_DEF(z_owned_closure_query_t, closure_query)
_Z_OWNED_FUNCTIONS_CLOSURE_DEF(z_owned_closure_reply_t, closure_reply)
_Z_OWNED_FUNCTIONS_CLOSURE_DEF(z_owned_closure_hello_t, closure_hello)
_Z_OWNED_FUNCTIONS_CLOSURE_DEF(z_owned_closure_zid_t, closure_zid)

_Z_VIEW_FUNCTIONS_DEF(keyexpr)
_Z_VIEW_FUNCTIONS_DEF(string)

/**
 * Loans a :c:type:`z_owned_sample_t`.
 *
 * Parameters:
 *   sample: Pointer to a :c:type:`z_owned_sample_t` to loan.
 *
 * Return:
 *   Pointer to the loaned sample as a :c:type:`z_loaned_sample_t`.
 */
const z_loaned_sample_t *z_sample_loan(const z_owned_sample_t *sample);

/**
 * Gets data from a :c:type:`z_loaned_string_t`.
 *
 * Parameters:
 *   str: Pointer to a :c:type:`z_loaned_string_t` to get data from.
 *
 * Return:
 *   Pointer to the string data.
 */
const char *z_string_data(const z_loaned_string_t *str);

/**
 * Gets string length from a :c:type:`z_loaned_string_t`.
 *
 * Parameters:
 *   str: Pointer to a :c:type:`z_loaned_string_t` to get length from.
 *
 * Return:
 *   Length of the string.
 */
size_t z_string_len(const z_loaned_string_t *str);

/************* Primitives **************/
/**
 * Scouts for other Zenoh entities like routers and/or peers.
 *
 * Parameters:
 *   config: Pointer to a moved :c:type:`z_owned_config_t` to configure the scouting with.
 *   callback: Pointer to a moved :c:type:`z_owned_closure_hello_t` callback.
 *
 * Return:
 *   ``0`` if scouting successfully triggered, ``negative value`` otherwise.
 */
int8_t z_scout(z_owned_config_t *config, z_owned_closure_hello_t *callback);

/**
 * Opens a Zenoh session.
 *
 * Parameters:
 *   zs: Pointer to an uninitialized :c:type:`z_owned_session_t` to store the session info.
 *   config: Pointer to a moved :c:type:`z_owned_config_t` to configure the session with.
 *
 * Return:
 *   ``0`` if open successful, ``negative value`` otherwise.
 */
int8_t z_open(z_owned_session_t *zs, z_owned_config_t *config);

/**
 * Closes a Zenoh session.
 *
 * Parameters:
 *   zs: Pointer to a moved :c:type:`z_owned_session_t` to close.
 *
 * Return:
 *   ``0`` if close successful, ``negative value`` otherwise.
 */
int8_t z_close(z_owned_session_t *zs);

/**
 * Fetches Zenoh IDs of all connected peers.
 *
 * The callback will be called once for each ID. It is guaranteed to never be called concurrently,
 * and to be dropped before this function exits.
 *
 * Parameters:
 *   zs: Pointer to :c:type:`z_loaned_session_t` to fetch peer id from.
 *   callback: Pointer to a moved :c:type:`z_owned_closure_zid_t` callback.
 *
 * Return:
 *   ``0`` if operation successfully triggered, ``negative value`` otherwise.
 */
int8_t z_info_peers_zid(const z_loaned_session_t *zs, z_owned_closure_zid_t *callback);

/**
 * Fetches Zenoh IDs of all connected routers.
 *
 * The callback will be called once for each ID. It is guaranteed to never be called concurrently,
 * and to be dropped before this function exits.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to fetch router id from.
 *   callback: Pointer to a moved :c:type:`z_owned_closure_zid_t` callback.
 *
 * Return:
 *   ``0`` if operation successfully triggered, ``negative value`` otherwise.
 */
int8_t z_info_routers_zid(const z_loaned_session_t *zs, z_owned_closure_zid_t *callback);

/**
 * Gets the local Zenoh ID associated to a given Zenoh session.
 *
 * If this function returns an array of 16 zeros, this means the session is invalid.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to get the id from.
 *
 * Return:
 *   The local Zenoh ID of the session as :c:type:`z_id_t`.
 */
z_id_t z_info_zid(const z_loaned_session_t *zs);

/**
 * Gets the keyexpr from a sample by aliasing it.
 *
 * Parameters:
 *   sample: Pointer to a :c:type:`z_loaned_sample_t` to get the keyexpr from.
 *
 * Return:
 *   The keyexpr wrapped as a :c:type:`z_loaned_keyexpr_t`.
 */
const z_loaned_keyexpr_t *z_sample_keyexpr(const z_loaned_sample_t *sample);

/**
 * Gets the payload of a sample by aliasing it.
 *
 * Parameters:
 *   sample: Pointer to a :c:type:`z_loaned_sample_t` to get the payload from.
 *
 * Return:
 *   The payload wrapped as a :c:type:`z_loaned_bytes_t`.
 */
const z_loaned_bytes_t *z_sample_payload(const z_loaned_sample_t *sample);

/**
 * Gets the timestamp of a sample by aliasing it.
 *
 * Parameters:
 *   sample: Pointer to a :c:type:`z_loaned_sample_t` to get the timestamp from.
 *
 * Return:
 *   The timestamp wrapped as a :c:type:`z_timestamp_t`.
 */
z_timestamp_t z_sample_timestamp(const z_loaned_sample_t *sample);

/**
 * Gets the encoding of a sample by aliasing it.
 *
 * Parameters:
 *   sample: Pointer to a :c:type:`z_loaned_sample_t` to get the encoding from.
 *
 * Return:
 *   The encoding wrapped as a :c:type:`z_loaned_encoding_t*`.
 */
const z_loaned_encoding_t *z_sample_encoding(const z_loaned_sample_t *sample);

/**
 * Gets the kind of a sample by aliasing it.
 *
 * Parameters:
 *   sample: Pointer to a :c:type:`z_loaned_sample_t` to get the kind from.
 *
 * Return:
 *   The sample kind wrapped as a :c:type:`z_sample_kind_t`.
 */
z_sample_kind_t z_sample_kind(const z_loaned_sample_t *sample);

/**
 * Gets the qos value of a sample by aliasing it.
 *
 * Parameters:
 *   sample: Pointer to a :c:type:`z_loaned_sample_t` to get the qos from.
 *
 * Return:
 *   The qos wrapped as a :c:type:`z_qos_t`.
 */
z_qos_t z_sample_qos(const z_loaned_sample_t *sample);

/**
 * Gets the attachment of a value by aliasing it.
 *
 * Parameters:
 *   sample: Pointer to a :c:type:`z_loaned_sample_t` to get the attachment from.
 *
 * Return:
 *   Pointer to the attachment as a :c:type:`z_loaned_bytes_t`.
 */
const z_loaned_bytes_t *z_sample_attachment(const z_loaned_sample_t *sample);

#if Z_FEATURE_PUBLICATION == 1
/**
 * Builds a :c:type:`z_put_options_t` with default values.
 *
 * Parameters:
 *   Pointer to an uninitialized :c:type:`z_put_options_t`.
 */

void z_put_options_default(z_put_options_t *options);

/**
 * Builds a :c:type:`z_delete_options_t` with default values.
 *
 * Parameters:
 *   Pointer to an uninitialized :c:type:`z_delete_options_t`.
 */
void z_delete_options_default(z_delete_options_t *options);

/**
 * Puts data for a given keyexpr.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to put the data through.
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to put the data for.
 *   payload: Pointer to a moved :c:type:`z_owned_bytes_t` containing the data to put.
 *   options: Pointer to a :c:type:`z_put_options_t` to configure the operation.
 *
 * Return:
 *   ``0`` if put operation successful, ``negative value`` otherwise.
 */
int8_t z_put(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, z_owned_bytes_t *payload,
             const z_put_options_t *options);

/**
 * Deletes data for a given keyexpr.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to delete the data through.
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to delete the data for.
 *   options: Pointer to a :c:type:`z_delete_options_t` to configure the operation.
 *
 * Return:
 *   ``0`` if delete operation successful, ``negative value`` otherwise.
 */
int8_t z_delete(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const z_delete_options_t *options);

/**
 * Builds a :c:type:`z_publisher_options_t` with default values.
 *
 * Parameters:
 *   Pointer to an uninitialized :c:type:`z_delete_options_t`.
 */
void z_publisher_options_default(z_publisher_options_t *options);

/**
 * Declares a publisher for a given keyexpr.
 *
 * Data can be put and deleted with this publisher with the help of the
 * :c:func:`z_publisher_put` and :c:func:`z_publisher_delete` functions.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to declare the publisher through.
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to bind the publisher with.
 *   options: Pointer to a :c:type:`z_publisher_options_t` to configure the operation.
 *
 * Return:
 *   ``0`` if declare successful, ``negative value`` otherwise.
 */
int8_t z_declare_publisher(z_owned_publisher_t *pub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                           const z_publisher_options_t *options);

/**
 * Undeclares a publisher.
 *
 * Parameters:
 *   pub: Pointer to a moved :c:type:`z_owned_publisher_t` to undeclare.
 *
 * Return:
 *   ``0`` if undeclare successful, ``negative value`` otherwise.
 */
int8_t z_undeclare_publisher(z_owned_publisher_t *pub);

z_owned_keyexpr_t z_publisher_keyexpr(z_loaned_publisher_t *publisher);

/**
 * Builds a :c:type:`z_publisher_put_options_t` with default values.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`z_publisher_put_options_t`.
 */
void z_publisher_put_options_default(z_publisher_put_options_t *options);

/**
 * Builds a :c:type:`z_publisher_delete_options_t` with default values.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`z_publisher_delete_options_t`.
 */
void z_publisher_delete_options_default(z_publisher_delete_options_t *options);

/**
 * Puts data for the keyexpr bound to the given publisher.
 *
 * Parameters:
 *   pub: Pointer to a :c:type:`z_loaned_publisher_t` from where to put the data.
 *   payload: Pointer to a moved :c:type:`z_owned_bytes_t` containing the data to put.
 *   options: Pointer to a :c:type:`z_publisher_put_options_t` to configure the operation.
 *
 * Return:
 *   ``0`` if put operation successful, ``negative value`` otherwise.
 */
int8_t z_publisher_put(const z_loaned_publisher_t *pub, z_owned_bytes_t *payload,
                       const z_publisher_put_options_t *options);

/**
 * Deletes data from the keyexpr bound to the given publisher.
 *
 * Parameters:
 *   pub: Pointer to a :c:type:`z_loaned_publisher_t` from where to delete the data.
 *   options: Pointer to a :c:type:`z_publisher_delete_options_t` to configure the delete operation.
 *
 * Return:
 *   ``0`` if delete operation successful, ``negative value`` otherwise.
 */
int8_t z_publisher_delete(const z_loaned_publisher_t *pub, const z_publisher_delete_options_t *options);
#endif

#if Z_FEATURE_QUERY == 1
/**
 * Builds a :c:type:`z_get_options_t` with default values.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`z_get_options_t`.
 */
void z_get_options_default(z_get_options_t *options);

/**
 * Sends a distributed query for a given keyexpr.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to send the query through.
 *   keyexpr: Pointer to a  :c:type:`z_loaned_keyexpr_t` to send the query for.
 *   parameters: Pointer to the parameters as a null-terminated string.
 *   callback: Pointer to a :c:type:`z_owned_closure_reply_t` callback.
 *   options: Pointer to a :c:type:`z_get_options_t` to configure the operation.
 *
 * Return:
 *   ``0`` if put operation successful, ``negative value`` otherwise.
 */
int8_t z_get(const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr, const char *parameters,
             z_owned_closure_reply_t *callback, z_get_options_t *options);
/**
 * Checks if queryable answered with an OK, which allows this value to be treated as a sample.
 *
 * Parameters:
 *   reply: Pointer to a :c:type:`z_loaned_reply_t` to check.
 *
 * Return:
 *   ``true`` if queryable answered with an OK, ``false`` otherwise.
 */
_Bool z_reply_is_ok(const z_loaned_reply_t *reply);

/**
 * Gets the content of an OK reply.
 *
 * You should always make sure that :c:func:`z_reply_is_ok` returns ``true`` before calling this function.
 *
 * Parameters:
 *   reply: Pointer to a :c:type:`z_loaned_reply_t` to get content from.
 *
 * Return:
 *   The OK reply content wrapped as a :c:type:`z_loaned_sample_t`.
 */
const z_loaned_sample_t *z_reply_ok(const z_loaned_reply_t *reply);

/**
 * Gets the contents of an error reply.
 *
 * You should always make sure that :c:func:`z_reply_is_ok` returns ``false`` before calling this function.
 *
 * Parameters:
 *   reply: Pointer to a :c:type:`z_loaned_reply_t` to get content from.
 *
 * Return:
 *   The error reply content wrapped as a :c:type:`z_loaned_reply_err_t`.
 */
const z_loaned_reply_err_t *z_reply_err(const z_loaned_reply_t *reply);
#endif

#if Z_FEATURE_QUERYABLE == 1
/**
 * Builds a :c:type:`z_queryable_options_t` with default values.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`z_queryable_options_t`.
 */
void z_queryable_options_default(z_queryable_options_t *options);

/**
 * Declares a queryable for a given keyexpr.
 *
 * Parameters:
 *   queryable: Pointer to an uninitialized :c:type:`z_owned_queryable_t` to contain the queryable.
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to declare the subscriber through.
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to bind the subscriber with.
 *   callback: Pointer to a :c:type:`z_owned_closure_query_t` callback.
 *   options: Pointer to a :c:type:`z_queryable_options_t` to configure the declare.
 *
 * Return:
 *   ``0`` if declare operation successful, ``negative value`` otherwise.
 */
int8_t z_declare_queryable(z_owned_queryable_t *queryable, const z_loaned_session_t *zs,
                           const z_loaned_keyexpr_t *keyexpr, z_owned_closure_query_t *callback,
                           const z_queryable_options_t *options);

/**
 * Undeclares a queryable.
 *
 * Parameters:
 *   queryable: Pointer to a :c:type:`z_owned_queryable_t` to undeclare.
 *
 * Return:
 *   ``0`` if undeclare operation successful, ``negative value`` otherwise.
 */
int8_t z_undeclare_queryable(z_owned_queryable_t *queryable);

/**
 * Builds a :c:type:`z_query_reply_options_t` with default values.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`z_query_reply_options_t`.
 */
void z_query_reply_options_default(z_query_reply_options_t *options);

/**
 * Sends a reply to a query.
 *
 * This function must be called inside of a :c:type:`z_owned_closure_query_t` callback associated to the
 * :c:type:`z_owned_queryable_t`, passing the received query as parameters of the callback function. This function can
 * be called multiple times to send multiple replies to a query. The reply will be considered complete when the callback
 * returns.
 *
 * Parameters:
 *   query: Pointer to a :c:type:`z_loaned_query_t` to reply.
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to bind the reply with.
 *   payload: Pointer to the reply data.
 *   payload_len: The length of the payload.
 *   options: Pointer to a :c:type:`z_query_reply_options_t` to configure the reply.
 *
 * Return:
 *   ``0`` if reply operation successful, ``negative value`` otherwise.
 */
int8_t z_query_reply(const z_loaned_query_t *query, const z_loaned_keyexpr_t *keyexpr, z_owned_bytes_t *payload,
                     const z_query_reply_options_t *options);
#endif

/**
 * Builds a new keyexpr.
 *
 * Parameters:
 *   keyexpr: Pointer to an uninitialized :c:type:`z_owned_keyexpr_t` to store the keyexpr.
 *   name: Pointer to the null-terminated string of the keyexpr.
 *
 * Return:
 *   ``0`` if creation successful, ``negative value`` otherwise.
 */
int8_t z_keyexpr_new(z_owned_keyexpr_t *keyexpr, const char *name);

/**
 * Declares a keyexpr, so that it is mapped on a numerical id.
 *
 * This numerical id is used on the network to save bandwidth and ease the retrieval of the concerned resource
 * in the routing tables.
 *
 * Parameters:
 *   ke: Pointer to an uninitialized :c:type:`z_owned_keyexpr_t` to contain the declared keyexpr.
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to declare the keyexpr through.
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to bind the keyexpr with.
 *
 * Return:
 *   ``0`` if declare successful, ``negative value`` otherwise.
 */
int8_t z_declare_keyexpr(z_owned_keyexpr_t *ke, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr);

/**
 * Undeclares a keyexpr.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to undeclare the data through.
 *   keyexpr: Pointer to a moved :c:type:`z_owned_keyexpr_t` to undeclare.
 *
 * Return:
 *   ``0`` if undeclare successful, ``negative value`` otherwise.
 */
// TODO(sashacmc): change parameters order?
int8_t z_undeclare_keyexpr(const z_loaned_session_t *zs, z_owned_keyexpr_t *keyexpr);

#if Z_FEATURE_SUBSCRIPTION == 1
/**
 * Builds a :c:type:`z_subscriber_options_t` with default values.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`z_subscriber_options_t`.
 */
void z_subscriber_options_default(z_subscriber_options_t *options);

/**
 * Declares a subscriber for a given keyexpr.
 *
 * Parameters:
 *   sub: Pointer to a :c:type:`z_owned_subscriber_t` to contain the subscriber.
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to declare the subscriber through.
 *   keyexpr: Pointer to a :c:type:`z_loaned_keyexpr_t` to bind the subscriber with.
 *   callback: Pointer to a`z_owned_closure_sample_t` callback.
 *   options: Pointer to a :c:type:`z_subscriber_options_t` to configure the operation
 *
 * Return:
 *   ``0`` if declare successful, ``negative value`` otherwise.
 */
int8_t z_declare_subscriber(z_owned_subscriber_t *sub, const z_loaned_session_t *zs, const z_loaned_keyexpr_t *keyexpr,
                            z_owned_closure_sample_t *callback, const z_subscriber_options_t *options);

/**
 * Undeclares the subscriber.
 *
 * Parameters:
 *   sub: Pointer to a :c:type:`z_owned_subscriber_t` to undeclare.
 *
 * Return:
 *   ``0`` if undeclare successful, ``negative value`` otherwise.
 */
int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub);

/**
 * Copies the keyexpr of a subscriber
 *
 * Parameters:
 *   keyexpr: Pointer to an uninitialized :c:type:`z_owned_keyexpr_t` to contain the keyexpr.
 *   sub: Pointer to a :c:type:`z_loaned_subscriber_t` to copy the keyexpr from.
 *
 * Return:
 *   ``0`` if copy successful, ``negative value`` otherwise.
 */
int8_t z_subscriber_keyexpr(z_owned_keyexpr_t *keyexpr, z_loaned_subscriber_t *sub);
#endif

/************* Multi Thread Tasks helpers **************/
/**
 * Builds a :c:type:`zp_task_read_options_t` with default value.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`zp_task_read_options_t`.
 */
void zp_task_read_options_default(zp_task_read_options_t *options);

/**
 * Starts a task to read from the network and process the received messages.
 *
 * Note that the task can be implemented in form of thread, process, etc. and its implementation is platform-dependent.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to start the task from.
 *   options: Pointer to a :c:type:`zp_task_read_options_t` to configure the task.
 *
 * Return:
 *   ``0`` if task started successfully, ``negative value`` otherwise.
 */
int8_t zp_start_read_task(z_loaned_session_t *zs, const zp_task_read_options_t *options);

/**
 * Stops the read task.
 *
 * This may result in stopping a thread or a process depending on the target platform.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to stop the task from.
 *
 * Return:
 *   ``0`` if task stopped successfully, ``negative value`` otherwise.
 */
int8_t zp_stop_read_task(z_loaned_session_t *zs);

/**
 * Builds a :c:type:`zp_task_lease_options_t` with default value.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`zp_task_lease_options_t`.
 */
void zp_task_lease_options_default(zp_task_lease_options_t *options);

/**
 * Starts a task to handle the session lease.
 *
 * This task will send ``KeepAlive`` messages when needed and will close the session when the lease is expired.
 * When operating over a multicast transport, it also periodically sends the ``Join`` messages.
 * Note that the task can be implemented in form of thread, process, etc. and its implementation is platform-dependent.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to start the task from.
 *   options: Pointer to a :c:type:`zp_task_lease_options_t` to configure the task.
 *
 * Return:
 *   ``0`` if task started successfully, ``negative value`` otherwise.
 */
int8_t zp_start_lease_task(z_loaned_session_t *zs, const zp_task_lease_options_t *options);

/**
 * Stops the lease task.
 *
 * This may result in stopping a thread or a process depending on the target platform.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to stop the task from.
 *
 * Return:
 *   ``0`` if task stopped successfully, ``negative value`` otherwise.
 */
int8_t zp_stop_lease_task(z_loaned_session_t *zs);

/************* Single Thread helpers **************/
/**
 * Builds a :c:type:`zp_read_options_t` with default value.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`zp_read_options_t`.
 */
void zp_read_options_default(zp_read_options_t *options);

/**
 * Executes a single read from the network and process received messages.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to execute the read for.
 *   options: Pointer to a :c:type:`zp_read_options_t` to configure the operation.
 *
 * Return:
 *   ``0`` if execution was successful, ``negative value`` otherwise.
 */
int8_t zp_read(const z_loaned_session_t *zs, const zp_read_options_t *options);

/**
 * Builds a :c:type:`zp_send_keep_alive_options_t` with default value.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`zp_send_keep_alive_options_t`.
 */
void zp_send_keep_alive_options_default(zp_send_keep_alive_options_t *options);

/**
 * Executes a single send keep alive procedure.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to execute the send for.
 *   options: Pointer to a :c:type:`zp_send_keep_alive_options_t` to configure the operation.
 *
 * Return:
 *   ``0`` if execution was successful, ``negative value`` otherwise.
 */
int8_t zp_send_keep_alive(const z_loaned_session_t *zs, const zp_send_keep_alive_options_t *options);

/**
 * Builds a :c:type:`zp_send_join_options_t` with default value.
 *
 * Parameters:
 *   options: Pointer to an uninitialized :c:type:`zp_send_join_options_t`.
 */
void zp_send_join_options_default(zp_send_join_options_t *options);

/**
 * Executes a single send join procedure.
 *
 * Parameters:
 *   zs: Pointer to a :c:type:`z_loaned_session_t` to execute the send for.
 *   options: Pointer to a :c:type:`zp_send_keep_alive_options_t` to configure the operation.
 *
 * Return:
 *   ``0`` if execution was successful, ``negative value`` otherwise.
 */
int8_t zp_send_join(const z_loaned_session_t *zs, const zp_send_join_options_t *options);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_API_PRIMITIVES_H */
