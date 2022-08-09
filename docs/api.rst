..
.. Copyright (c) 2022 ZettaScale Technology
..
.. This program and the accompanying materials are made available under the
.. terms of the Eclipse Public License 2.0 which is available at
.. http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
.. which is available at https://www.apache.org/licenses/LICENSE-2.0.
..
.. SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
..
.. Contributors:
..   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
..

*************
API Reference
*************

Enums
-----

.. autocenum:: constants.h::z_whatami_t
.. autocenum:: constants.h::z_keyexpr_canon_status_t
.. autocenum:: constants.h::z_sample_kind_t
.. autocenum:: constants.h::z_encoding_prefix_t
.. autocenum:: constants.h::z_queryable_kind_t
.. autocenum:: constants.h::z_consolidation_mode_t
.. autocenum:: constants.h::z_reliability_t
.. autocenum:: constants.h::z_query_consolidation_tag_t
.. autocenum:: constants.h::z_reply_tag_t
.. autocenum:: constants.h::z_congestion_control_t
.. autocenum:: constants.h::z_priority_t
.. autocenum:: constants.h::z_submode_t
.. autocenum:: constants.h::z_query_target_t

Types
-----

.. autoctype:: types.h::z_zint_t
.. autoctype:: types.h::z_bytes_t
.. autoctype:: types.h::z_id_t
.. autoctype:: types.h::z_string_t
.. autoctype:: types.h::z_keyexpr_t
.. autoctype:: types.h::z_config_t
.. autoctype:: types.h::z_session_t
.. autoctype:: types.h::z_subscriber_t
.. autoctype:: types.h::z_pull_subscriber_t
.. autoctype:: types.h::z_publisher_t
.. autoctype:: types.h::z_queryable_t
.. autoctype:: types.h::z_encoding_t
.. autoctype:: types.h::z_period_t
.. autoctype:: types.h::z_value_t
.. autoctype:: types.h::z_subscriber_options_t
.. autoctype:: types.h::z_pull_subscriber_options_t
.. autoctype:: types.h::z_query_consolidation_t
.. autoctype:: types.h::z_publisher_options_t
.. autoctype:: types.h::z_queryable_options_t
.. autoctype:: types.h::z_query_reply_options_t
.. autoctype:: types.h::z_put_options_t
.. autoctype:: types.h::z_delete_options_t
.. autoctype:: types.h::z_publisher_put_options_t
.. autoctype:: types.h::z_publisher_delete_options_t
.. autoctype:: types.h::z_get_options_t
.. autoctype:: types.h::z_sample_t
.. autoctype:: types.h::z_hello_t
.. autoctype:: types.h::z_reply_t
.. autoctype:: types.h::z_reply_data_t

Owned Types
-----------

Like most ``z_owned_X_t`` types, you may obtain an instance of ``z_X_t`` by loaning it using ``z_X_loan(&val)``.
The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is equivalent to writing ``z_X_loan(&val)``.

Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said instance, as this implies the instance's inners were moved.
To make this fact more obvious when reading your code, consider using ``z_move(val)`` instead of ``&val`` as the argument.
After a move, ``val`` will still exist, but will no longer be valid. The destructors are double-free-safe, but other functions will still trust that your ``val`` is valid.

To check if ``val`` is still valid, you may use ``z_X_check(&val)`` or ``z_check(val)`` if your compiler supports ``_Generic``, which will return ``true`` if ``val`` is valid.

.. c:type:: z_owned_bytes_t

  A zenoh-allocated :c:type:`z_bytes_t`.

.. c:type:: z_owned_string_t

  A zenoh-allocated :c:type:`z_string_t`.

.. c:type:: z_owned_keyexpr_t

  A zenoh-allocated :c:type:`z_keyexpr_t`.

.. c:type:: z_owned_config_t

  A zenoh-allocated :c:type:`z_config_t`.

.. c:type:: z_owned_session_t

  A zenoh-allocated :c:type:`z_session_t`.

.. c:type:: z_owned_info_t

  A zenoh-allocated :c:type:`z_info_t`.

.. c:type:: z_owned_subscriber_t

  A zenoh-allocated :c:type:`z_subscriber_t`.

.. c:type:: z_owned_pull_subscriber_t

  A zenoh-allocated :c:type:`z_pull_subscriber_t`.

.. c:type:: z_owned_publisher_t

  A zenoh-allocated :c:type:`z_publisher_t`.

.. c:type:: z_owned_queryable_t

  A zenoh-allocated :c:type:`z_queryable_t`.

.. c:type:: z_owned_encoding_t

  A zenoh-allocated :c:type:`z_encoding_t`.

.. c:type:: z_owned_period_t

  A zenoh-allocated :c:type:`z_period_t`.

.. c:type:: z_owned_consolidation_strategy_t

  A zenoh-allocated :c:type:`z_consolidation_strategy_t`.

.. c:type:: z_owned_query_target_t

  A zenoh-allocated :c:type:`z_query_target_t`.

.. c:type:: z_owned_query_consolidation_t

  A zenoh-allocated :c:type:`z_query_consolidation_t`.

.. c:type:: z_owned_reply_t

  A zenoh-allocated :c:type:`z_reply_t`.

.. c:type:: z_owned_reply_data_t

  A zenoh-allocated :c:type:`z_reply_data_t`.

.. c:type:: z_owned_str_array_t

  A zenoh-allocated :c:type:`z_str_array_t`.

.. c:type:: z_owned_hello_array_t

  A zenoh-allocated :c:type:`z_hello_array_t`.

.. c:type:: z_owned_reply_data_array_t

  A zenoh-allocated :c:type:`z_reply_data_array_t`.

Closures
--------

A closure is a structure that contains all the elements for stateful, memory-leak-free callbacks:
  - context: a pointer to an arbitrary state.
  - call: the typical callback function. ``context`` will be passed as its last argument.
  - drop: allows the callback's state to be freed. ``context`` will be passed as its last argument.

Closures are not guaranteed not to be called concurrently.

It is guaranteed that:
  - ``call`` will never be called once ``drop`` has started.
  - ``drop`` will only be called **once**, and **after every** ``call`` has ended.
  - The two previous guarantees imply that ``call`` and ``drop`` are never called concurrently.


.. autoctype:: types.h::z_owned_closure_sample_t
.. autoctype:: types.h::z_owned_closure_query_t
.. autoctype:: types.h::z_owned_closure_reply_t
.. autoctype:: types.h::z_owned_closure_zid_t

Macros
------
.. autocmacro:: macros.h::z_loan
.. autocmacro:: macros.h::z_move
.. autocmacro:: macros.h::z_check
.. autocmacro:: macros.h::z_clone
.. autocmacro:: macros.h::z_drop
.. autocmacro:: macros.h::z_closure

.. Collections
.. -----------

.. Arrays
.. ~~~~~~

.. .. c:type:: struct name_array_t

..   An array of elemets of type *name_t*.

..   .. c:member:: unsigned int length

..     The length of the array.

..   .. c:member:: name_t *elem

..     The *name_t* values.


.. .. c:type:: struct name_p_array_t

..   An array of pointers to elemets of type *name_t*.

..   .. c:member:: unsigned int length

..     The length of the array.

..   .. c:member:: name_t *elem

..     The pointers to the *name_t* values.


.. Vectors
.. ~~~~~~~

.. .. c:type:: struct z_vec_t

..   A sequence container that encapsulates a dynamic size array of pointers.

..   .. c:member:: unsigned int _capacity

..     The maximum capacity of the vector.

..   .. c:member:: unsigned int _length

..     The current length of the vector.

..   .. c:member:: void **_elem

..     The pointers to the values.

.. .. c:function:: z_vec_t z_vec_make(unsigned int capacity)

..   Initialize a :c:type:`z_vec_t` with a :c:member:`z_vec_t.capacity` of **capacity**,
..   a :c:member:`z_vec_t.length` of **0** and a :c:member:`z_vec_t._elem` pointing to a
..   newly allocated array of **capacity** pointers.

.. .. c:function:: unsigned int z_vec_len(const z_vec_t* v)

..   Return the current length of the given :c:type:z_vec_t.

.. .. c:function:: void z_vec_append(z_vec_t* v, void* e)

..   Append the element **e** to the vector **v** and take ownership of the appended element.

.. .. c:function:: void z_vec_set(z_vec_t* sv, unsigned int i, void* e)

..   Set the element **e** in the vector **v** at index **i** and take ownership of the element.

.. .. c:function:: const void* z_vec_get(const z_vec_t* v, unsigned int i)

..   Return the element at index **i** in vector **v**.


.. Data Structures
.. ---------------

.. .. c:type:: struct _z_keyexpr_t

..   Data structure representing a resource key.

..   .. c:member:: int kind

..     One of the following kinds:

..       | ``Z_INT_RES_KEY``
..       | ``Z_STR_RES_KEY``

..   .. c:member:: union _z_keyexpr_t key

..     .. c:member:: _z_zint_t rid

..       A resource id (integer) when :c:member:`_z_keyexpr_t.kind` equals ``Z_INT_RES_KEY``.

..     .. c:member:: char *rname

..       A resource name (string) when :c:member:`_z_keyexpr_t.kind` equals ``Z_STR_RES_KEY``.

.. .. c:type:: struct _z_subinfo_t

..   Data structure representing a subscription mode (see :c:func:`_z_declare_subscriber`).

..   .. c:member:: uint8_t kind

..     One of the following subscription modes:

..       | ``Z_PUSH_MODE``
..       | ``Z_PULL_MODE``
..       | ``Z_PERIODIC_PUSH_MODE``
..       | ``Z_PERIODIC_PULL_MODE``

..   .. c:member:: _z_period_t tprop

..     The period. *Unsupported*

.. .. c:type:: struct _z_timestamp_t

..   Data structure representing a unique timestamp.

..   .. c:member:: _z_zint_t time

..     The time as a 64-bit long, where:

..         - The higher 32-bit represent the number of seconds since midnight, January 1, 1970 UTC
..         - The lower 32-bit represent a fraction of 1 second.

..   .. c:member:: uint8_t clock_id[16]

..     The unique identifier of the clock that generated this timestamp.

.. .. c:type:: struct z_data_info_t

..   Data structure containing meta informations about the associated data.

..   .. c:member:: unsigned int flags

..     Flags indicating which meta information is present in the :c:type:`z_data_info_t`:

..       | ``Z_T_STAMP``
..       | ``Z_KIND``
..       | ``Z_ENCODING``

..   .. c:member:: _z_timestamp_t tstamp

..     The unique timestamp at which the data has been produced.

..   .. c:member:: uint8_t encoding

..     The encoding of the data.

..   .. c:member:: unsigned short kind

..     The kind of the data.

.. .. c:type:: struct z_query_dest_t

..   Data structure defining which storages or evals should be destination of a query (see :c:func:`z_query_wo`).

..   .. c:member:: uint8_t kind

..     One of the following destination kinds:

..       | ``Z_BEST_MATCH`` the nearest complete storage/eval if there is one, all storages/evals if not.
..       | ``Z_COMPLETE`` only complete storages/evals.
..       | ``Z_ALL`` all storages/evals.
..       | ``Z_NONE`` no storages/evals.

..   .. c:member:: uint8_t nb

..     The number of storages or evals that should be destination of the query when
..     :c:member:`z_query_dest_t.kind` equals ``Z_COMPLETE``.

.. .. c:type:: struct z_reply_value_t

..   Data structure containing one of the replies to a query (see :c:type:`z_reply_handler_t`).

..   .. c:member:: char kind

..     One of the following kinds:

..       | ``Z_STORAGE_DATA`` the reply contains some data from a storage.
..       | ``Z_STORAGE_FINAL`` the reply indicates that no more data is expected from the specified storage.
..       | ``Z_EVAL_DATA`` the reply contains some data from an eval.
..       | ``Z_EVAL_FINAL`` the reply indicates that no more data is expected from the specified eval.
..       | ``Z_REPLY_FINAL`` the reply indicates that no more replies are expected for the query.

..   .. c:member:: const unsigned char *srcid

..     The unique identifier of the storage or eval that sent the reply when :c:member:`z_reply_value_t.kind` equals
..     ``Z_STORAGE_DATA``, ``Z_STORAGE_FINAL``, ``Z_EVAL_DATA`` or ``Z_EVAL_FINAL``.

..   .. c:member:: size_t srcid_length

..     The length of the :c:member:`z_reply_value_t.srcid` when :c:member:`z_reply_value_t.kind` equals
..     ``Z_STORAGE_DATA``, ``Z_STORAGE_FINAL``, ``Z_EVAL_DATA`` or ``Z_EVAL_FINAL``.

..   .. c:member:: _z_zint_t rsn

..     The sequence number of the reply from the identified storage or eval when :c:member:`z_reply_value_t.kind` equals
..     ``Z_STORAGE_DATA``, ``Z_STORAGE_FINAL``, ``Z_EVAL_DATA`` or ``Z_EVAL_FINAL``.

..   .. c:member:: const char *rname

..     The resource name of the received data when :c:member:`z_reply_value_t.kind` equals
..     ``Z_STORAGE_DATA`` or ``Z_EVAL_DATA``.

..   .. c:member:: const unsigned char *data

..     A pointer to the received data when :c:member:`z_reply_value_t.kind` equals
..     ``Z_STORAGE_DATA`` or ``Z_EVAL_DATA``.

..   .. c:member:: size_t data_length

..     The length of the received :c:member:`z_reply_value_t.data` when :c:member:`z_reply_value_t.kind` equals
..     ``Z_STORAGE_DATA`` or ``Z_EVAL_DATA``.

..   .. c:member:: z_data_info_t info

..     Some meta information about the received :c:member:`z_reply_value_t.data` when :c:member:`z_reply_value_t.kind` equals
..     ``Z_STORAGE_DATA`` or ``Z_EVAL_DATA``.

.. .. c:type:: struct _z_property_t

..   A key/value pair where the key is an integer and the value a byte sequence.

..   .. c:member:: _z_zint_t id

..     The key of the :c:type:`_z_property_t`.

..   .. c:member:: z_array_uint8_t value

..     The value of the :c:type:`_z_property_t`.

Functions
---------

.. autocfunction:: primitives.h::z_init_logger
.. autocfunction:: primitives.h::z_keyexpr
.. autocfunction:: primitives.h::z_keyexpr_to_string
.. autocfunction:: primitives.h::zp_keyexpr_resolve
.. autocfunction:: primitives.h::z_keyexpr_is_valid
.. autocfunction:: primitives.h::z_keyexpr_is_canon
.. autocfunction:: primitives.h::zp_keyexpr_is_canon_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_canonize
.. autocfunction:: primitives.h::zp_keyexpr_canonize_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_includes
.. autocfunction:: primitives.h::zp_keyexpr_includes_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_intersect
.. autocfunction:: primitives.h::zp_keyexpr_intersect_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_equals
.. autocfunction:: primitives.h::zp_keyexpr_equals_null_terminated
.. autocfunction:: primitives.h::zp_config_new
.. autocfunction:: primitives.h::zp_config_empty
.. autocfunction:: primitives.h::zp_config_default
.. autocfunction:: primitives.h::zp_config_get
.. autocfunction:: primitives.h::zp_config_insert
.. autocfunction:: primitives.h::z_encoding_default
.. autocfunction:: primitives.h::z_query_target_default
.. autocfunction:: primitives.h::z_query_consolidation_auto
.. autocfunction:: primitives.h::z_query_consolidation_default
.. autocfunction:: primitives.h::z_query_consolidation_full
.. autocfunction:: primitives.h::z_query_consolidation_last_router
.. autocfunction:: primitives.h::z_query_consolidation_lazy
.. autocfunction:: primitives.h::z_query_consolidation_none
.. autocfunction:: primitives.h::z_query_consolidation_reception
.. autocfunction:: primitives.h::z_query_value_selector
.. autocfunction:: primitives.h::z_query_keyexpr
.. autocfunction:: primitives.h::z_closure_sample
.. autocfunction:: primitives.h::z_closure_query
.. autocfunction:: primitives.h::z_closure_reply
.. autocfunction:: primitives.h::z_closure_zid
.. autocfunction:: primitives.h::z_scout
.. autocfunction:: primitives.h::z_open
.. autocfunction:: primitives.h::z_close
.. autocfunction:: primitives.h::z_info_peers_zid
.. autocfunction:: primitives.h::z_info_routers_zid
.. autocfunction:: primitives.h::z_info_zid
.. autocfunction:: primitives.h::z_put_options_default
.. autocfunction:: primitives.h::z_delete_options_default
.. autocfunction:: primitives.h::z_put
.. autocfunction:: primitives.h::z_delete
.. autocfunction:: primitives.h::z_get_options_default
.. autocfunction:: primitives.h::z_get
.. autocfunction:: primitives.h::z_declare_keyexpr
.. autocfunction:: primitives.h::z_undeclare_keyexpr
.. autocfunction:: primitives.h::z_publisher_options_default
.. autocfunction:: primitives.h::z_declare_publisher
.. autocfunction:: primitives.h::z_undeclare_publisher
.. autocfunction:: primitives.h::z_publisher_put_options_default
.. autocfunction:: primitives.h::z_publisher_delete_options_default
.. autocfunction:: primitives.h::z_publisher_put
.. autocfunction:: primitives.h::z_publisher_delete
.. autocfunction:: primitives.h::z_subscriber_options_default
.. autocfunction:: primitives.h::z_declare_subscriber
.. autocfunction:: primitives.h::z_undeclare_subscriber
.. autocfunction:: primitives.h::z_declare_pull_subscriber
.. autocfunction:: primitives.h::z_undeclare_pull_subscriber
.. autocfunction:: primitives.h::z_pull
.. autocfunction:: primitives.h::z_queryable_options_default
.. autocfunction:: primitives.h::z_declare_queryable
.. autocfunction:: primitives.h::z_undeclare_queryable
.. autocfunction:: primitives.h::z_query_reply
.. autocfunction:: primitives.h::z_reply_is_ok
.. autocfunction:: primitives.h::z_reply_ok
.. autocfunction:: primitives.h::z_reply_err
.. autocfunction:: primitives.h::zp_start_read_task
.. autocfunction:: primitives.h::zp_stop_read_task
.. autocfunction:: primitives.h::zp_start_lease_task
.. autocfunction:: primitives.h::zp_stop_lease_task
.. autocfunction:: primitives.h::zp_read
.. autocfunction:: primitives.h::zp_send_keep_alive

.. Handlers
.. --------

.. .. c:type:: void (*_z_data_handler_t)(const z_resource_id_t *rid, const unsigned char *data, size_t length, const z_data_info_t *info, void *arg)

..   Function to pass as argument of :c:func:`_z_declare_subscriber` or :c:func:`z_declare_storage`.
..   It will be called on reception of data matching the subscribed/stored resource selection.

..   | **rid** is the resource id of the received data.
..   | **data** is a pointer to the received data.
..   | **length** is the length of the received data.
..   | **info** is the :c:type:`z_data_info_t` associated with the received data.
..   | **arg** is the pointer passed to :c:func:`_z_declare_subscriber` or :c:func:`z_declare_storage`.

.. .. c:type:: void (*_z_reply_handler_t)(const char *rname, const char *value_selector, z_replies_sender_t send_replies, void *query_handle, void *arg)

..   Function to pass as argument of :c:func:`z_declare_storage` or :c:func:`z_declare_eval`.
..   It will be called on reception of query matching the stored/evaluated resource selection.
..   The :c:type:`_z_reply_handler_t` must provide the data matching the resource *rname* by calling
..   the *send_replies* function with the *query_handle* and the data as arguments. The *send_replies*
..   function MUST be called but accepts empty data array.

..   | **rname** is the resource name of the queried data.
..   | **value_selector** is a string provided by the querier refining the data to be provided.
..   | **send_replies** is a function that MUST be called with the *query_handle* and the provided data as arguments.
..   | **query_handle** is a pointer to pass as argument of *send_replies*.
..   | **arg** is the pointer passed to :c:func:`z_declare_storage` or :c:func:`z_declare_eval`.

.. .. c:type:: void (*z_reply_handler_t)(const z_reply_value_t *reply, void *arg)

..   Function to pass as argument of :c:func:`z_query` or :c:func:`z_query_wo`.
..   It will be called on reception of replies to the query sent by :c:func:`z_query` or :c:func:`z_query_wo`.

..   | **reply** is the actual :c:type:`reply<z_reply_value_t>`.
..   | **arg** is the pointer passed to :c:func:`z_query` or :c:func:`z_query_wo`.

.. .. c:type:: void (*z_on_disconnect_t)(void *z)

..   Function to pass as argument of :c:func:`_z_open`.
..   It will be called each time the client API is disconnected from the infrastructure.

..   | **z** is the zenoh-net session.
