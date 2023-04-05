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

#ifndef ZENOH_PICO_API_PRIMITIVES_H
#define ZENOH_PICO_API_PRIMITIVES_H

#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/api/types.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/net/subscribe.h"

#ifdef __cplusplus
extern "C" {
#endif

/********* Data Types Handlers *********/
/**
 * Constructs a :c:type:`z_string_t` departing from a ``const char *``.
 * It is a loaned key expression that aliases ``value``.
 *
 * Parameters:
 *   value: Pointer to null terminated string.
 *
 * Returns:
 *   The :c:type:`z_string_t` corresponding to the given string.
 */
z_string_t z_string_make(const char *value);

/**
 * Constructs a :c:type:`z_keyexpr_t` departing from a string.
 * It is a loaned key expression that aliases ``name``.
 * Unlike it's counterpart in zenoh-c, this function does not test passed expression to correctness.
 *
 * Parameters:
 *   name: Pointer to string representation of the keyexpr as a null terminated string.
 *
 * Returns:
 *   The :c:type:`z_keyexpr_t` corresponding to the given string.
 */
z_keyexpr_t z_keyexpr(const char *name);

/**
 * Constructs a :c:type:`z_keyexpr_t` departing from a string.
 * It is a loaned key expression that aliases ``name``.
 * Input key expression is not checked for correctness.
 *
 * Parameters:
 *   name: Pointer to string representation of the keyexpr as a null terminated string.
 *
 * Returns:
 *   The :c:type:`z_keyexpr_t` corresponding to the given string.
 */
z_keyexpr_t z_keyexpr_unchecked(const char *name);

/**
 * Get null-terminated string departing from a :c:type:`z_keyexpr_t`.
 *
 * If given keyexpr contains a declared keyexpr, the resulting owned string will be unitialized.
 * In that case, the user must use :c:func:`zp_keyexpr_resolve` to resolve the nesting declarations
 * and get its full expanded representation.
 *
 * Parameters:
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t`
 *
 * Returns:
 *   The :c:type:`z_owned_str_t` containing key expression string representation if it's possible
 */
z_owned_str_t z_keyexpr_to_string(z_keyexpr_t keyexpr);

/**
 * Returns the key expression's internal string by aliasing it.
 *
 * Parameters:
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t`
 *
 * Returns:
 *   The :c:type:`z_bytes_t` pointing to key expression string representation if it's possible

 */
z_bytes_t z_keyexpr_as_bytes(z_keyexpr_t keyexpr);

/**
 * Constructs a null-terminated string departing from a :c:type:`z_keyexpr_t` for a given :c:type:`z_session_t`.
 * The user is responsible of droping the returned string using ``z_free``.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` to resolve the keyexpr.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to be resolved.
 *
 * Returns:
 *   The string representation of a keyexpr for a given session.
 */
z_owned_str_t zp_keyexpr_resolve(z_session_t zs, z_keyexpr_t keyexpr);

/**
 * Checks if a given keyexpr is valid.
 *
 * Parameters:
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to be checked.
 *
 * Returns:
 *   Returns ``true`` if the keyexpr is valid, or ``false`` otherwise.
 */
_Bool z_keyexpr_is_initialized(const z_keyexpr_t *keyexpr);

/**
 * Check if a given keyexpr is valid and in its canonical form.
 *
 * Parameters:
 *   start: Pointer to the keyexpr in its string representation as a non-null terminated string.
 *   len: Number of characters in ``start``.
 *
 * Returns:
 *   Returns ``0`` if the passed string is a valid (and canon) key expression, or a ``negative value`` otherwise.
 *   Error codes are defined in :c:enum:`zp_keyexpr_canon_status_t`.
 */
int8_t z_keyexpr_is_canon(const char *start, size_t len);

/**
 * Check if a given keyexpr is valid and in its canonical form.
 *
 * Parameters:
 *   start: Pointer to the keyexpr in its string representation as a null terminated string.
 *   len: Number of characters in ``start``.
 *
 * Returns:
 *   Returns ``0`` if the passed string is a valid (and canon) key expression, or a ``negative value`` otherwise.
 *   Error codes are defined in :c:enum:`zp_keyexpr_canon_status_t`.
 */
int8_t zp_keyexpr_is_canon_null_terminated(const char *start);

/**
 * Canonization of a given keyexpr in its its string representation.
 * The canonization is performed over the passed string, possibly shortening it by modifying ``len``.
 *
 * Parameters:
 *   start: Pointer to the keyexpr in its string representation as a non-null terminated string.
 *   len: Number of characters in ``start``.
 *
 * Returns:
 *   Returns ``0`` if the canonization is successful, or a ``negative value`` otherwise.
 *   Error codes are defined in :c:enum:`zp_keyexpr_canon_status_t`.
 */
int8_t z_keyexpr_canonize(char *start, size_t *len);

/**
 * Canonization of a given keyexpr in its its string representation.
 * The canonization is performed over the passed string, possibly shortening it by modifying ``len``.
 *
 * Parameters:
 *   start: Pointer to the keyexpr in its string representation as a null terminated string.
 *   len: Number of characters in ``start``.
 *
 * Returns:
 *   Returns ``0`` if the canonization is successful, or a ``negative value`` otherwise.
 *   Error codes are defined in :c:enum:`zp_keyexpr_canon_status_t`.
 */
int8_t zp_keyexpr_canonize_null_terminated(char *start);

/**
 * Check if a given keyexpr contains another keyexpr in its set.
 *
 * Parameters:
 *   l: The first keyexpr.
 *   r: The second keyexpr.
 *
 * Returns:
 *   Returns ``0`` if ``l`` includes ``r``, i.e. the set defined by ``l`` contains every key belonging to the set
 * defined by ``r``. Otherwise, it returns a ``-1``, or other ``negative value`` for errors.
 */
int8_t z_keyexpr_includes(z_keyexpr_t l, z_keyexpr_t r);

/**
 * Check if a given keyexpr contains another keyexpr in its set.
 *
 * Parameters:
 *   l: Pointer to the keyexpr in its string representation as a null terminated string.
 *   llen: Number of characters in ``l``.
 *   r: Pointer to the keyexpr in its string representation as a null terminated string.
 *   rlen: Number of characters in ``r``.
 *
 * Returns:
 *   Returns ``0`` if ``l`` includes ``r``, i.e. the set defined by ``l`` contains every key belonging to the set
 * defined by ``r``. Otherwise, it returns a ``-1``, or other ``negative value`` for errors.
 */
int8_t zp_keyexpr_includes_null_terminated(const char *l, const char *r);

/**
 * Check if a given keyexpr intersects with another keyexpr.
 *
 * Parameters:
 *   l: The first keyexpr.
 *   r: The second keyexpr.
 *
 * Returns:
 *   Returns ``0`` if the keyexprs intersect, i.e. there exists at least one key which is contained in both of the
 * sets defined by ``l`` and ``r``. Otherwise, it returns a ``-1``, or other ``negative value`` for errors.
 */
int8_t z_keyexpr_intersects(z_keyexpr_t l, z_keyexpr_t r);

/**
 * Check if a given keyexpr intersects with another keyexpr.
 *
 * Parameters:
 *   l: Pointer to the keyexpr in its string representation as a null terminated string.
 *   llen: Number of characters in ``l``.
 *   r: Pointer to the keyexpr in its string representation as a null terminated string.
 *   rlen: Number of characters in ``r``.
 *
 * Returns:
 *   Returns ``0`` if the keyexprs intersect, i.e. there exists at least one key which is contained in both of the
 * sets defined by ``l`` and ``r``. Otherwise, it returns a ``-1``, or other ``negative value`` for errors.
 */
int8_t zp_keyexpr_intersect_null_terminated(const char *l, const char *r);

/**
 * Check if a two keyexprs are equal.
 *
 * Parameters:
 *   l: The first keyexpr.
 *   r: The second keyexpr.
 *
 * Returns:
 *   Returns ``0`` if both ``l`` and ``r`` are equal. Otherwise, it returns a ``-1``, or other ``negative value`` for
 * errors.
 */
int8_t z_keyexpr_equals(z_keyexpr_t l, z_keyexpr_t r);

/**
 * Check if a two keyexprs are equal.
 *
 * Parameters:
 *   l: Pointer to the keyexpr in its string representation as a null terminated string.
 *   llen: Number of characters in ``l``.
 *   r: Pointer to the keyexpr in its string representation as a null terminated string.
 *   rlen: Number of characters in ``r``.
 *
 * Returns:
 *   Returns ``0`` if both ``l`` and ``r`` are equal. Otherwise, it returns a ``-1``, or other ``negative value`` for
 * errors.
 */
int8_t zp_keyexpr_equals_null_terminated(const char *l, const char *r);

/**
 * Return a new, zenoh-allocated, empty configuration.
 * It consists in an empty set of properties for zenoh session configuration.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_config_t` by loaning it using
 * ``z_config_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_config_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_config_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Returns:
 *   Returns a new, zenoh-allocated, empty configuration.
 */
z_owned_config_t z_config_new(void);

/**
 * Return a new, zenoh-allocated, default configuration.
 * It consists in a default set of properties for zenoh session configuration.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_config_t` by loaning it using
 * ``z_config_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_config_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_config_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Returns:
 *   Returns a new, zenoh-allocated, default configuration.
 */
z_owned_config_t z_config_default(void);

/**
 * Gets the property with the given integer key from the configuration.
 *
 * Parameters:
 *   config: A loaned instance of :c:type:`z_owned_config_t`.
 *   key: Integer key for the requested property.
 *
 * Returns:
 *   Returns the property with the given integer key from the configuration.
 */
const char *zp_config_get(z_config_t config, uint8_t key);

/**
 * Inserts or replaces the property with the given integer key in the configuration.
 *
 * Parameters:
 *   config: A loaned instance of :c:type:`z_owned_config_t`.
 *   key: Integer key for the property to be inserted.
 *   value: Property value to be inserted.
 *
 * Returns:
 *   Returns ``0`` if the insertion is successful, or a ``negative value`` otherwise.
 */
int8_t zp_config_insert(z_config_t config, uint8_t key, z_string_t value);

/**
 * Return a new, zenoh-allocated, default scouting configuration.
 * It consists in a default set of properties for scouting configuration.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_scouting_config_t` by loaning it
 * using
 * ``z_scouting_config_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``,
 * is equivalent to writing ``z_config_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_scouting_config_check(&val)`` or ``z_check(val)`` if your
 * compiler supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Returns:
 *   Returns a new, zenoh-allocated, default scouting configuration.
 */
z_owned_scouting_config_t z_scouting_config_default(void);

/**
 * Return a new, zenoh-allocated, scouting configuration extracted from a :c:type:`z_owned_config_t`.
 * It consists in a default set of properties for scouting configuration.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_scouting_config_t` by loaning it
 * using
 * ``z_scouting_config_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``,
 * is equivalent to writing ``z_config_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_scouting_config_check(&val)`` or ``z_check(val)`` if your
 * compiler supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   config: A loaned instance of :c:type:`z_owned_config_t`.
 *
 * Returns:
 *   Returns a new, zenoh-allocated, default scouting configuration.
 */
z_owned_scouting_config_t z_scouting_config_from(z_config_t config);

/**
 * Gets the property with the given integer key from the configuration.
 *
 * Parameters:
 *   config: A loaned instance of :c:type:`z_owned_scouting_config_t`.
 *   key: Integer key for the requested property.
 *
 * Returns:
 *   Returns the property with the given integer key from the configuration.
 */
const char *zp_scouting_config_get(z_scouting_config_t config, uint8_t key);

/**
 * Inserts or replaces the property with the given integer key in the configuration.
 *
 * Parameters:
 *   config: A loaned instance of :c:type:`z_owned_scouting_config_t`.
 *   key: Integer key for the property to be inserted.
 *   value: Property value to be inserted.
 *
 * Returns:
 *   Returns ``0`` if the insertion is successful, or a ``negative value`` otherwise.
 */
int8_t zp_scouting_config_insert(z_scouting_config_t config, uint8_t key, z_string_t value);

/**
 * Constructs a :c:type:`z_encoding_t`.
 *
 * Parameters:
 *   prefix: A known :c:type:`z_encoding_prefix_t`.
 *   suffix: A custom suffix to be appended to the prefix.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_encoding_t`.
 */
z_encoding_t z_encoding(z_encoding_prefix_t prefix, const char *suffix);

/**
 * Constructs a default encoding.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_encoding_t`.
 */
z_encoding_t z_encoding_default(void);

/**
 *
 * Checks validity of the timestamp
 *
 */
_Bool z_timestamp_check(z_timestamp_t ts);

/**
 * Constructs a default query target.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_query_target_t`.
 */
z_query_target_t z_query_target_default(void);

/**
 * Automatic query consolidation strategy selection.
 *
 * A query consolidation strategy will automatically be selected depending the query selector.
 * If the selector contains time range properties, no consolidation is performed.
 * Otherwise the :c:func:`z_query_consolidation_latest` strategy is used.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_auto(void);

/**
 * Constructs a default :c:type:`z_query_consolidation_t`.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_default(void);

/**
 * Latest consolidation.
 *
 * This strategy optimizes bandwidth on all links in the system but will provide a very poor latency.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_latest(void);

/**
 * Monotonic consolidation.
 *
 * This strategy offers the best latency. Replies are directly transmitted to the application when received
 * without needing to wait for all replies. This mode does not garantee that there will be no duplicates.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_monotonic(void);

/**
 * No consolidation.
 *
 * This strategy is usefull when querying timeseries data bases or when using quorums.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_query_consolidation_t`.
 */
z_query_consolidation_t z_query_consolidation_none(void);

/**
 * Get a query's value selector by aliasing it.
 *
 * Parameters:
 *   query: Pointer to the query to get the value selector from.
 *
 * Returns:
 *   Returns the value selector wrapped as a :c:type:`z_bytes_t`, since value selector is a user-defined representation.
 */
z_bytes_t z_query_parameters(const z_query_t *query);

/**
 * Get a query's payload value by aliasing it.
 * Note: This API has been marked as unstable: it works as advertised, but we may change it in a future release.
 *
 * Parameters:
 *   query: Pointer to the query to get the value selector from.
 *
 * Returns:
 *   Returns the payload value wrapped as a :c:type:`z_value_t`, since payload value is a user-defined representation.
 */
z_value_t z_query_value(const z_query_t *query);

/**
 * Get a query's key by aliasing it.
 *
 * Parameters:
 *   query: Pointer to the query to get keyexpr from.
 *
 * Returns:
 *   Returns the :c:type:`z_keyexpr_t` associated to the query.
 */
z_keyexpr_t z_query_keyexpr(const z_query_t *query);

/**
 * Return a new sample closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_closure_sample_t` by loaning it using
 * ``z_closure_sample_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``,
 * is equivalent to writing ``z_closure_sample_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_closure_sample_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   call: the typical callback function. ``context`` will be passed as its last argument.
 *   drop: allows the callback's state to be freed. ``context`` will be passed as its last argument.
 *   context: a pointer to an arbitrary state.
 *
 * Returns:
 *   Returns a new sample closure.
 */
z_owned_closure_sample_t z_closure_sample(_z_data_handler_t call, _z_dropper_handler_t drop, void *context);

/**
 * Return a new query closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_closure_query_t` by loaning it using
 * ``z_closure_query_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_closure_query_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_closure_query_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   call: the typical callback function. ``context`` will be passed as its last argument.
 *   drop: allows the callback's state to be freed. ``context`` will be passed as its last argument.
 *   context: a pointer to an arbitrary state.
 *
 * Returns:
 *   Returns a new query closure.
 */
z_owned_closure_query_t z_closure_query(_z_questionable_handler_t call, _z_dropper_handler_t drop, void *context);

/**
 * Return a new reply closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_closure_reply_t` by loaning it using
 * ``z_closure_reply_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_closure_reply_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_closure_reply_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   call: the typical callback function. ``context`` will be passed as its last argument.
 *   drop: allows the callback's state to be freed. ``context`` will be passed as its last argument.
 *   context: a pointer to an arbitrary state.
 *
 * Returns:
 *   Returns a new reply closure.
 */
z_owned_closure_reply_t z_closure_reply(z_owned_reply_handler_t call, _z_dropper_handler_t drop, void *context);

/**
 * Return a new hello closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_closure_hello_t` by loaning it using
 * ``z_closure_hello_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``,
 * is equivalent to writing ``z_closure_hello_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_closure_hello_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   call: the typical callback function. ``context`` will be passed as its last argument.
 *   drop: allows the callback's state to be freed. ``context`` will be passed as its last argument.
 *   context: a pointer to an arbitrary state.
 *
 * Returns:
 *   Returns a new hello closure.
 */
z_owned_closure_hello_t z_closure_hello(z_owned_hello_handler_t call, _z_dropper_handler_t drop, void *context);

/**
 * Return a new zid closure.
 * It consists on a structure that contains all the elements for stateful, memory-leak-free callbacks.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_closure_zid_t` by loaning it using
 * ``z_closure_zid_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_closure_zid_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_closure_zid_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   call: the typical callback function. ``context`` will be passed as its last argument.
 *   drop: allows the callback's state to be freed. ``context`` will be passed as its last argument.
 *   context: a pointer to an arbitrary state.
 *
 * Returns:
 *   Returns a new zid closure.
 */
z_owned_closure_zid_t z_closure_zid(z_id_handler_t call, _z_dropper_handler_t drop, void *context);

/**************** Loans ****************/
#define _OWNED_FUNCTIONS(type, ownedtype, name)    \
    _Bool z_##name##_check(const ownedtype *name); \
    type z_##name##_loan(const ownedtype *name);   \
    ownedtype *z_##name##_move(ownedtype *name);   \
    ownedtype z_##name##_clone(ownedtype *name);   \
    void z_##name##_drop(ownedtype *name);         \
    ownedtype z_##name##_null(void);

_OWNED_FUNCTIONS(z_str_t, z_owned_str_t, str)
_OWNED_FUNCTIONS(z_keyexpr_t, z_owned_keyexpr_t, keyexpr)
_OWNED_FUNCTIONS(z_config_t, z_owned_config_t, config)
_OWNED_FUNCTIONS(z_scouting_config_t, z_owned_scouting_config_t, scouting_config)
_OWNED_FUNCTIONS(z_session_t, z_owned_session_t, session)
_OWNED_FUNCTIONS(z_subscriber_t, z_owned_subscriber_t, subscriber)
_OWNED_FUNCTIONS(z_pull_subscriber_t, z_owned_pull_subscriber_t, pull_subscriber)
_OWNED_FUNCTIONS(z_publisher_t, z_owned_publisher_t, publisher)
_OWNED_FUNCTIONS(z_queryable_t, z_owned_queryable_t, queryable)
_OWNED_FUNCTIONS(z_hello_t, z_owned_hello_t, hello)
_OWNED_FUNCTIONS(z_reply_t, z_owned_reply_t, reply)
_OWNED_FUNCTIONS(z_str_array_t, z_owned_str_array_t, str_array)

#define _OWNED_FUNCTIONS_CLOSURE(ownedtype, name) \
    _Bool z_##name##_check(const ownedtype *val); \
    ownedtype *z_##name##_move(ownedtype *val);   \
    void z_##name##_drop(ownedtype *val);         \
    ownedtype z_##name##_null(void);

_OWNED_FUNCTIONS_CLOSURE(z_owned_closure_sample_t, closure_sample)
_OWNED_FUNCTIONS_CLOSURE(z_owned_closure_query_t, closure_query)
_OWNED_FUNCTIONS_CLOSURE(z_owned_closure_reply_t, closure_reply)
_OWNED_FUNCTIONS_CLOSURE(z_owned_closure_hello_t, closure_hello)
_OWNED_FUNCTIONS_CLOSURE(z_owned_closure_zid_t, closure_zid)

/************* Primitives **************/
/**
 * Looks for other Zenoh-enabled entities like routers and/or peers.
 *
 * Parameters:
 *   config: A moved instance of :c:type:`z_owned_scouting_config_t` containing the set properties to configure the
 * scouting. callback: A moved instance of :c:type:`z_owned_closure_hello_t` containg the callbacks to be called.
 *
 * Returns:
 *   Returns ``0`` if the scouting is successful triggered, or a ``negative value`` otherwise.
 */
int8_t z_scout(z_owned_scouting_config_t *config, z_owned_closure_hello_t *callback);

/**
 * Opens a Zenoh session.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_session_t` by loaning it using
 * ``z_session_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_session_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_session_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   config: A moved instance of :c:type:`z_owned_config_t` containing the set properties to configure the session.
 *
 * Returns:
 *   A :c:type:`z_owned_session_t` with either a valid open session or a failing session.
 *   Should the session opening fail, ``z_check(val)`` ing the returned value will return ``false``.
 */
z_owned_session_t z_open(z_owned_config_t *config);

/**
 * Closes a Zenoh session.
 *
 * Parameters:
 *   zs: A moved instance of the the :c:type:`z_owned_session_t` to close.
 *
 * Returns:
 *   Returns ``0`` if the session is successful closed, or a ``negative value`` otherwise.
 */
int8_t z_close(z_owned_session_t *zs);

/**
 * Fetches the Zenoh IDs of all connected peers.
 *
 * :c:var:`callback` will be called once for each ID. It is guaranteed to never be called concurrently,
 * and to be dropped before this function exits.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` to inquiry.
 *   callback: A moved instance of :c:type:`z_owned_closure_zid_t` containg the callbacks to be called.
 *
 * Returns:
 *   Returns ``0`` if the info is successful triggered, or a ``negative value`` otherwise.
 */
int8_t z_info_peers_zid(const z_session_t zs, z_owned_closure_zid_t *callback);

/**
 * Fetches the Zenoh IDs of all connected routers.
 *
 * :c:var:`callback` will be called once for each ID. It is guaranteed to never be called concurrently,
 * and to be dropped before this function exits.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` to inquiry.
 *   callback: A moved instance of :c:type:`z_owned_closure_zid_t` containg the callbacks to be called.
 *
 * Returns:
 *   Returns ``0`` if the info is successful triggered, or a ``negative value`` otherwise.
 */
int8_t z_info_routers_zid(const z_session_t zs, z_owned_closure_zid_t *callback);

/**
 * Get the local Zenoh ID associated to a given Zenoh session.
 *
 * Unless the :c:type:`z_session_t` is invalid, that ID is guaranteed to be non-zero.
 * In other words, this function returning an array of 16 zeros means you failed to pass it a valid session.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` to inquiry.
 *
 * Returns:
 *   Returns the local Zenoh ID of the given :c:type:`z_session_t`.
 */
z_id_t z_info_zid(const z_session_t zs);

/**
 * Constructs the default values for the put operation.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_put_options_t`.
 */
z_put_options_t z_put_options_default(void);

/**
 * Constructs the default values for the delete operation.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_delete_options_t`.
 */
z_delete_options_t z_delete_options_default(void);

/**
 * Puts data for a given keyexpr.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` through where data will be put.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to put.
 *   payload: Pointer to the data to put.
 *   payload_len: The length of the ``payload``.
 *   options: The put options to be applied in the put operation.
 *
 * Returns:
 *   Returns ``0`` if the put operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_put(z_session_t zs, z_keyexpr_t keyexpr, const uint8_t *payload, z_zint_t payload_len,
             const z_put_options_t *options);

/**
 * Deletes data from a given keyexpr.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` through where data will be put.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to put.
 *   options: The delete options to be applied in the delete operation.
 *
 * Returns:
 *   Returns ``0`` if the delete operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_delete(z_session_t zs, z_keyexpr_t keyexpr, const z_delete_options_t *options);

/**
 * Constructs the default values for the get operation.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_get_options_t`.
 */
z_get_options_t z_get_options_default(void);

/**
 * Issues a distributed query for a given keyexpr.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` through where data will be put.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to put.
 *   parameters: Pointer to the parameters as a null-terminated string.
 *   callback: A moved instance of :c:type:`z_owned_closure_reply_t` containg the callbacks to be called.
 *   options: The get options to be aplied in the distributed query.
 *
 * Returns:
 *   Returns ``0`` if the put operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_get(z_session_t zs, z_keyexpr_t keyexpr, const char *parameters, z_owned_closure_reply_t *callback,
             const z_get_options_t *options);

/**
 * Creates keyexpr owning string passed to it
 */
z_owned_keyexpr_t z_keyexpr_new(const char *name);

/**
 * Declares a keyexpr, so that it is internally mapped into into a numerical id.
 *
 * This numerical id is used on the network to save bandwidth and ease the retrieval of the concerned resource
 * in the routing tables.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_keyexpr_t` by loaning it using
 * ``z_keyexpr_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_keyexpr_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_keyexpr_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to declare the keyexpr.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to declare.
 *
 * Returns:
 *   A :c:type:`z_owned_keyexpr_t` with either a valid or invalid keyexpr.
 *   Should the keyexpr be invalid, ``z_check(val)`` ing the returned value will return ``false``.
 */
z_owned_keyexpr_t z_declare_keyexpr(z_session_t zs, z_keyexpr_t keyexpr);

/**
 * Undeclares the keyexpr generated by a call to :c:func:`z_declare_keyexpr`.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` through where data will be put.
 *   keyexpr: A moved instance of :c:type:`z_owned_keyexpr_t` to undeclare.
 *
 * Returns:
 *   Returns ``0`` if the undeclare keyexpr operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_undeclare_keyexpr(z_session_t zs, z_owned_keyexpr_t *keyexpr);

/**
 * Constructs the default values for the publisher entity.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_publisher_options_t`.
 */
z_publisher_options_t z_publisher_options_default(void);

/**
 * Declares a publisher for the given keyexpr.
 *
 * Data can be put and deleted with this publisher with the help of the
 * :c:func:`z_publisher_put` and :c:func:`z_publisher_delete` functions.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_publisher_t` by loaning it using
 * ``z_publisher_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_publisher_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_publisher_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to declare the publisher.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to associate with the publisher.
 *   options: The options to apply to the publisher. If ``NULL`` is passed, the default options will be applied.
 *
 * Returns:
 *   A :c:type:`z_owned_publisher_t` with either a valid publisher or a failing publisher.
 *   Should the publisher be invalid, ``z_check(val)`` ing the returned value will return ``false``.
 */
z_owned_publisher_t z_declare_publisher(z_session_t zs, z_keyexpr_t keyexpr, const z_publisher_options_t *options);

/**
 * Undeclares the publisher generated by a call to :c:func:`z_declare_publisher`.
 *
 * Parameters:
 *   pub: A moved instance of :c:type:`z_owned_publisher_t` to undeclare.
 *
 * Returns:
 *   Returns ``0`` if the undeclare publisher operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_undeclare_publisher(z_owned_publisher_t *pub);

/**
 * Constructs the default values for the put operation via a publisher entity.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_publisher_put_options_t`.
 */
z_publisher_put_options_t z_publisher_put_options_default(void);

/**
 * Constructs the default values for the delete operation via a publisher entity.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_publisher_delete_options_t`.
 */
z_publisher_delete_options_t z_publisher_delete_options_default(void);

/**
 * Puts data for the keyexpr associated to the given publisher.
 *
 * Parameters:
 *   pub: A loaned instance of :c:type:`z_publisher_t` from where to put the data.
 *   options: The options to apply to the put operation. If ``NULL`` is passed, the default options will be applied.
 *
 * Returns:
 *   Returns ``0`` if the put operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_publisher_put(const z_publisher_t pub, const uint8_t *payload, size_t len,
                       const z_publisher_put_options_t *options);

/**
 * Deletes data from the keyexpr associated to the given publisher.
 *
 * Parameters:
 *   pub: A loaned instance of :c:type:`z_publisher_t` from where to delete the data.
 *   options: The options to apply to the delete operation. If ``NULL`` is passed, the default options will be applied.
 *
 * Returns:
 *   Returns ``0`` if the delete operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_publisher_delete(const z_publisher_t pub, const z_publisher_delete_options_t *options);

/**
 * Constructs the default values for the subscriber entity.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_subscriber_options_t`.
 */
z_subscriber_options_t z_subscriber_options_default(void);

/**
 * Declares a (push) subscriber for the given keyexpr.
 *
 * Received data is processed by means of callbacks.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_subscriber_t` by loaning it using
 * ``z_subscriber_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_subscriber_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_subscriber_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to declare the subscriber.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to associate with the subscriber.
 *   callback: A moved instance of :c:type:`z_owned_closure_sample_t` containg the callbacks to be called and the
 * context to pass to them. options: The options to apply to the subscriber. If ``NULL`` is passed, the default options
 * will be applied.
 *
 * Returns:
 *   A :c:type:`z_owned_subscriber_t` with either a valid subscriber or a failing subscriber.
 *   Should the subscriber be invalid, ``z_check(val)`` ing the returned value will return ``false``.
 */
z_owned_subscriber_t z_declare_subscriber(z_session_t zs, z_keyexpr_t keyexpr, z_owned_closure_sample_t *callback,
                                          const z_subscriber_options_t *options);

/**
 * Undeclares the (push) subscriber generated by a call to :c:func:`z_declare_subscriber`.
 *
 * Parameters:
 *   sub: A moved instance of :c:type:`z_owned_subscriber_t` to undeclare.
 *
 * Returns:
 *   Returns ``0`` if the undeclare (push) subscriber operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_undeclare_subscriber(z_owned_subscriber_t *sub);

/**
 * Constructs the default values for the pull subscriber entity.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_pull_subscriber_options_t`.
 */
z_pull_subscriber_options_t z_pull_subscriber_options_default(void);

/**
 * Declares a pull subscriber for the given keyexpr.
 *
 * Data can be pulled with this subscriber with the help of the
 * :c:func:`z_pull` function. Received data is processed by means of callbacks.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_pull_subscriber_t` by loaning it
 * using ``z_pull_subscriber_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's
 * ``_Generic``, is equivalent to writing ``z_pull_subscriber_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_pull_subscriber_check(&val)`` or ``z_check(val)`` if your
 * compiler supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to declare the subscriber.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to associate with the subscriber.
 *   callback: A moved instance of :c:type:`z_owned_closure_sample_t` containg the callbacks to be called and the
 * context to pass to them. options: The options to apply to the pull subscriber. If ``NULL`` is passed, the default
 * options will be applied.
 *
 * Returns:
 *   A :c:type:`z_owned_pull_subscriber_t` with either a valid subscriber or a failing subscriber.
 *   Should the pull subscriber be invalid, ``z_check(val)`` ing the returned value will return ``false``.
 */
z_owned_pull_subscriber_t z_declare_pull_subscriber(z_session_t zs, z_keyexpr_t keyexpr,
                                                    z_owned_closure_sample_t *callback,
                                                    const z_pull_subscriber_options_t *options);

/**
 * Undeclares the pull subscriber generated by a call to :c:func:`z_declare_pull_subscriber`.
 *
 * Parameters:
 *   sub: A moved instance of :c:type:`z_owned_pull_subscriber_t` to undeclare.
 *
 * Returns:
 *   Returns ``0`` if the undeclare pull subscriber operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_undeclare_pull_subscriber(z_owned_pull_subscriber_t *sub);

/**
 * Pulls data for :c:type:`z_owned_pull_subscriber_t`. The pulled data will be provided
 * by calling the **callback** function provided to the :c:func:`z_declare_pull_subscriber` function.
 *
 * Parameters:
 *   sub: A loaned instance of :c:type:`z_pull_subscriber_t` from where to pull the data.
 *
 * Returns:
 *   Returns ``0`` if the pull operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_subscriber_pull(const z_pull_subscriber_t sub);

/**
 * Constructs the default values for the queryable entity.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_queryable_options_t`.
 */
z_queryable_options_t z_queryable_options_default(void);

/**
 * Declares a queryable for the given keyexpr.
 *
 * Received queries are processed by means of callbacks.
 *
 * Like most ``z_owned_X_t`` types, you may obtain an instance of :c:type:`z_owned_queryable_t` by loaning it using
 * ``z_queryable_loan(&val)``. The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is
 * equivalent to writing ``z_queryable_loan(&val)``.
 *
 * Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said
 * instance, as this implies the instance's inners were moved. To make this fact more obvious when reading your code,
 * consider using ``z_move(val)`` instead of ``&val`` as the argument. After a ``z_move``, ``val`` will still exist, but
 * will no longer be valid. The destructors are double-drop-safe, but other functions will still trust that your ``val``
 * is valid.
 *
 * To check if ``val`` is still valid, you may use ``z_queryable_check(&val)`` or ``z_check(val)`` if your compiler
 * supports ``_Generic``, which will return ``true`` if ``val`` is valid, or ``false`` otherwise.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to declare the subscriber.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to associate with the subscriber.
 *   callback: A moved instance of :c:type:`z_owned_closure_query_t` containg the callbacks to be called and the context
 * to pass to them. options: The options to apply to the queryable. If ``NULL`` is passed, the default options will be
 * applied.
 *
 * Returns:
 *   A :c:type:`z_owned_queryable_t` with either a valid queryable or a failing queryable.
 *   Should the queryable be invalid, ``z_check(val)`` ing the returned value will return ``false``.
 */
z_owned_queryable_t z_declare_queryable(z_session_t zs, z_keyexpr_t keyexpr, z_owned_closure_query_t *callback,
                                        const z_queryable_options_t *options);

/**
 * Undeclares the queryable generated by a call to :c:func:`z_declare_queryable`.
 *
 * Parameters:
 *   queryable: A moved instance of :c:type:`z_owned_queryable_t` to undeclare.
 *
 * Returns:
 *   Returns ``0`` if the undeclare queryable operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_undeclare_queryable(z_owned_queryable_t *queryable);

/**
 * Sends a reply to a query.
 *
 * This function must be called inside of a :c:type:`z_owned_closure_query_t` callback associated to the
 * :c:type:`z_owned_queryable_t`, passing the received query as parameters of the callback function. This function can
 * be called multiple times to send multiple replies to a query. The reply will be considered complete when the callback
 * returns.
 *
 * Parameters:
 *   query: Pointer to the received query.
 *   keyexpr: A loaned instance of :c:type:`z_keyexpr_t` to associate with the subscriber.
 *   payload: Pointer to the data to put.
 *   payload_len: The length of the ``payload``.
 *   options: The options to apply to the send query reply operation. If ``NULL`` is passed, the default options will be
 * applied.
 *
 * Returns:
 *   Returns ``0`` if the send query reply operation is successful, or a ``negative value`` otherwise.
 */
int8_t z_query_reply(const z_query_t *query, const z_keyexpr_t keyexpr, const uint8_t *payload, size_t payload_len,
                     const z_query_reply_options_t *options);

/**
 * Constructs the default values for the query reply operation.
 *
 * Returns:
 *   Returns the constructed :c:type:`z_query_reply_options_t`.
 */
z_query_reply_options_t z_query_reply_options_default(void);

/**
 * Checks if the queryable answered with an OK, which allows this value to be treated as a sample.
 *
 * If this returns ``false``, you should use ``z_check`` before trying to use :c:func:`z_reply_err` if you want to
 * process the error that may be here.
 *
 * Parameters:
 *   reply: Pointer to the received query reply.
 *
 * Returns:
 *   Returns ``true`` if the queryable answered with an OK, which allows this value to be treated as a sample, or
 * ``false`` otherwise.
 */
_Bool z_reply_is_ok(const z_owned_reply_t *reply);

/**
 * Yields the contents of the reply by asserting it indicates a success.
 *
 * You should always make sure that :c:func:`z_reply_is_ok` returns ``true`` before calling this function.
 *
 * Parameters:
 *   reply: Pointer to the received query reply.
 *
 * Returns:
 *   Returns the :c:type:`z_sample_t` wrapped in the query reply.
 */
z_sample_t z_reply_ok(const z_owned_reply_t *reply);

/**
 * Yields the contents of the reply by asserting it indicates a failure.
 *
 * You should always make sure that :c:func:`z_reply_is_ok` returns ``false`` before calling this function.
 *
 * Parameters:
 *   reply: Pointer to the received query reply.
 *
 * Returns:
 *   Returns the :c:type:`z_value_t` wrapped in the query reply.
 */
z_value_t z_reply_err(const z_owned_reply_t *reply);

/**
 * Checks if a given value is valid.
 *
 * Parameters:
 *   value: A loaned instance of :c:type:`z_value_t` to be checked.
 *
 * Returns:
 *   Returns ``true`` if the value is valid, or ``false`` otherwise.
 */
_Bool z_value_is_initialized(z_value_t *value);

/************* Multi Thread Taks helpers **************/
/**
 * Constructs the default values for the session read task.
 *
 * Returns:
 *   Returns the constructed :c:type:`zp_task_read_options_t`.
 */
zp_task_read_options_t zp_task_read_options_default(void);

/**
 * Start a separate task to read from the network and process the messages as soon as they are received.
 *
 * Note that the task can be implemented in form of thread, process, etc. and its implementation is platform-dependent.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to start the read task.
 *   options: The options to apply when starting the read task. If ``NULL`` is passed, the default options will be
 * applied.
 *
 * Returns:
 *   Returns ``0`` if the read task started successfully, or a ``negative value`` otherwise.
 */
int8_t zp_start_read_task(z_session_t zs, const zp_task_read_options_t *options);

/**
 * Stop the read task.
 *
 * This may result in stopping a thread or a process depending on the target platform.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to stop the read task.
 *
 * Returns:
 *   Returns ``0`` if the read task stopped successfully, or a ``negative value`` otherwise.
 */
int8_t zp_stop_read_task(z_session_t zs);

/**
 * Constructs the default values for the session lease task.
 *
 * Returns:
 *   Returns the constructed :c:type:`zp_task_lease_options_t`.
 */
zp_task_lease_options_t zp_task_lease_options_default(void);

/**
 * Start a separate task to handle the session lease.
 *
 * This task will send ``KeepAlive`` messages when needed and will close the session when the lease is expired.
 * When operating over a multicast transport, it also periodically sends the ``Join`` messages.
 * Note that the task can be implemented in form of thread, process, etc. and its implementation is platform-dependent.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to start the lease task.
 *   options: The options to apply when starting the lease task. If ``NULL`` is passed, the default options will be
 * applied.
 *
 * Returns:
 *   Returns ``0`` if the lease task started successfully, or a ``negative value`` otherwise.
 */
int8_t zp_start_lease_task(z_session_t zs, const zp_task_lease_options_t *options);

/**
 * Stop the lease task.
 *
 * This may result in stopping a thread or a process depending on the target platform.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where to stop the lease task.
 *
 * Returns:
 *   Returns ``0`` if the lease task stopped successfully, or a ``negative value`` otherwise.
 */
int8_t zp_stop_lease_task(z_session_t zs);

/************* Single Thread helpers **************/
/**
 * Constructs the default values for the reading procedure.
 *
 * Returns:
 *   Returns the constructed :c:type:`zp_read_options_t`.
 */
zp_read_options_t zp_read_options_default(void);

/**
 * Triggers a single execution of reading procedure from the network and processes of any received the message.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where trigger the reading procedure.
 *   options: The options to apply to the read. If ``NULL`` is passed, the default options will be
 * applied.
 *
 * Returns:
 *   Returns ``0`` if the reading procedure was executed successfully, or a ``negative value`` otherwise.
 */
int8_t zp_read(z_session_t zs, const zp_read_options_t *options);

/**
 * Constructs the default values for sending the keep alive.
 *
 * Returns:
 *   Returns the constructed :c:type:`zp_send_keep_alive_options_t`.
 */
zp_send_keep_alive_options_t zp_send_keep_alive_options_default(void);

/**
 * Triggers a single execution of keep alive procedure.
 *
 * It will send ``KeepAlive`` messages when needed and will close the session when the lease is expired.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where trigger the leasing procedure.
 *   options: The options to apply to the send of a ``KeepAlive`` messages. If ``NULL`` is passed, the default options
 * will be applied.
 *
 * Returns:
 *   Returns ``0`` if the leasing procedure was executed successfully, or a ``negative value`` otherwise.
 */
int8_t zp_send_keep_alive(z_session_t zs, const zp_send_keep_alive_options_t *options);

/**
 * Constructs the default values for sending the join.
 *
 * Returns:
 *   Returns the constructed :c:type:`zp_send_join_options_t`.
 */
zp_send_join_options_t zp_send_join_options_default(void);

/**
 * Triggers a single execution of join procedure.
 *
 * It will send ``Join`` messages.
 *
 * Parameters:
 *   zs: A loaned instance of the the :c:type:`z_session_t` where trigger the leasing procedure.
 *   options: The options to apply to the send of a ``Join`` messages. If ``NULL`` is passed, the default options will
 * be applied.
 *
 * Returns:
 *   Returns ``0`` if the leasing procedure was executed successfully, or a ``negative value`` otherwise.
 */
int8_t zp_send_join(z_session_t zs, const zp_send_join_options_t *options);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_API_PRIMITIVES_H */
