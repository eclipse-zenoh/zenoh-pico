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

Zenoh Types
-----------

Enums
~~~~~

.. autocenum:: constants.h::z_whatami_t
.. autocenum:: constants.h::zp_keyexpr_canon_status_t
.. autocenum:: constants.h::z_sample_kind_t
.. autocenum:: constants.h::z_encoding_prefix_t
.. autocenum:: constants.h::z_consolidation_mode_t
.. autocenum:: constants.h::z_reliability_t
.. autocenum:: constants.h::z_reply_tag_t
.. autocenum:: constants.h::z_congestion_control_t
.. autocenum:: constants.h::z_priority_t
.. autocenum:: constants.h::z_submode_t
.. autocenum:: constants.h::z_query_target_t

Data Structures
~~~~~~~~~~~~~~~

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
.. autoctype:: types.h::zp_task_read_options_t
.. autoctype:: types.h::zp_task_lease_options_t
.. autoctype:: types.h::zp_read_options_t
.. autoctype:: types.h::zp_send_keep_alive_options_t

Arrays
~~~~~~
.. c:type:: z_str_array_t

  Represents an array of ``char *``.

  Operations over :c:type:`z_str_array_t` must be done using the provided functions:

    - ``char *z_str_array_get(z_str_array_t *a, size_t k);``
    - ``size_t z_str_array_len(z_str_array_t *a);``
    - ``_Bool z_str_array_array_is_empty(z_str_array_t *a);``

Owned Types
~~~~~~~~~~~

Like most ``z_owned_X_t`` types, you may obtain an instance of ``z_X_t`` by loaning it using ``z_X_loan(&val)``.
The ``z_loan(val)`` macro, available if your compiler supports C11's ``_Generic``, is equivalent to writing ``z_X_loan(&val)``.

Like all ``z_owned_X_t``, an instance will be destroyed by any function which takes a mutable pointer to said instance, as this implies the instance's inners were moved.
To make this fact more obvious when reading your code, consider using ``z_move(val)`` instead of ``&val`` as the argument.
After a move, ``val`` will still exist, but will no longer be valid. The destructors are double-free-safe, but other functions will still trust that your ``val`` is valid.

To check if ``val`` is still valid, you may use ``z_X_check(&val)`` or ``z_check(val)`` if your compiler supports ``_Generic``, which will return ``true`` if ``val`` is valid.

.. c:type:: z_owned_bytes_t

  A zenoh-allocated :c:type:`z_bytes_t`.

.. c:type:: z_owned_str_t

  A zenoh-allocated :c:type:`char *`.

.. c:type:: z_owned_keyexpr_t

  A zenoh-allocated :c:type:`z_keyexpr_t`.

.. c:type:: z_owned_config_t

  A zenoh-allocated :c:type:`z_config_t`.

.. c:type:: z_owned_session_t

  A zenoh-allocated :c:type:`z_session_t`.

.. c:type:: z_owned_subscriber_t

  A zenoh-allocated :c:type:`z_subscriber_t`.

.. c:type:: z_owned_pull_subscriber_t

  A zenoh-allocated :c:type:`z_pull_subscriber_t`.

.. c:type:: z_owned_publisher_t

  A zenoh-allocated :c:type:`z_publisher_t`.

.. c:type:: z_owned_queryable_t

  A zenoh-allocated :c:type:`z_queryable_t`.

.. c:type:: z_owned_reply_t

  A zenoh-allocated :c:type:`z_reply_t`.

.. c:type:: z_owned_str_array_t

  A zenoh-allocated :c:type:`z_str_array_t`.

Closures
~~~~~~~~

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
.. autoctype:: types.h::z_owned_closure_hello_t
.. autoctype:: types.h::z_owned_closure_zid_t


Zenoh Functions
---------------

Macros
~~~~~~
.. autocmacro:: macros.h::z_loan
.. autocmacro:: macros.h::z_move
.. autocmacro:: macros.h::z_check
.. autocmacro:: macros.h::z_clone
.. autocmacro:: macros.h::z_drop
.. autocmacro:: macros.h::z_closure
.. autocmacro:: macros.h::z_null

Primitives
~~~~~~~~~~

.. autocfunction:: primitives.h::z_keyexpr
.. autocfunction:: primitives.h::z_keyexpr_to_string
.. autocfunction:: primitives.h::zp_keyexpr_resolve
.. autocfunction:: primitives.h::z_keyexpr_is_initialized
.. autocfunction:: primitives.h::z_keyexpr_is_canon
.. autocfunction:: primitives.h::zp_keyexpr_is_canon_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_canonize
.. autocfunction:: primitives.h::zp_keyexpr_canonize_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_includes
.. autocfunction:: primitives.h::zp_keyexpr_includes_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_intersects
.. autocfunction:: primitives.h::zp_keyexpr_intersect_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_equals
.. autocfunction:: primitives.h::zp_keyexpr_equals_null_terminated
.. autocfunction:: primitives.h::z_config_new
.. autocfunction:: primitives.h::z_config_default
.. autocfunction:: primitives.h::zp_config_get
.. autocfunction:: primitives.h::zp_config_insert
.. autocfunction:: primitives.h::z_scouting_config_default
.. autocfunction:: primitives.h::z_scouting_config_from
.. autocfunction:: primitives.h::zp_scouting_config_get
.. autocfunction:: primitives.h::zp_scouting_config_insert
.. autocfunction:: primitives.h::z_encoding_default
.. autocfunction:: primitives.h::z_query_target_default
.. autocfunction:: primitives.h::z_query_consolidation_auto
.. autocfunction:: primitives.h::z_query_consolidation_default
.. autocfunction:: primitives.h::z_query_consolidation_latest
.. autocfunction:: primitives.h::z_query_consolidation_monotonic
.. autocfunction:: primitives.h::z_query_consolidation_none
.. autocfunction:: primitives.h::z_query_parameters
.. autocfunction:: primitives.h::z_query_keyexpr
.. autocfunction:: primitives.h::z_query_value
.. autocfunction:: primitives.h::z_value_is_initialized
.. autocfunction:: primitives.h::z_closure_sample
.. autocfunction:: primitives.h::z_closure_query
.. autocfunction:: primitives.h::z_closure_reply
.. autocfunction:: primitives.h::z_closure_hello
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
.. autocfunction:: primitives.h::z_pull_subscriber_options_default
.. autocfunction:: primitives.h::z_declare_pull_subscriber
.. autocfunction:: primitives.h::z_undeclare_pull_subscriber
.. autocfunction:: primitives.h::z_subscriber_pull
.. autocfunction:: primitives.h::z_queryable_options_default
.. autocfunction:: primitives.h::z_declare_queryable
.. autocfunction:: primitives.h::z_undeclare_queryable
.. autocfunction:: primitives.h::z_query_reply
.. autocfunction:: primitives.h::z_reply_is_ok
.. autocfunction:: primitives.h::z_reply_ok
.. autocfunction:: primitives.h::z_reply_err
.. autocfunction:: primitives.h::zp_task_read_options_default
.. autocfunction:: primitives.h::zp_start_read_task
.. autocfunction:: primitives.h::zp_stop_read_task
.. autocfunction:: primitives.h::zp_task_lease_options_default
.. autocfunction:: primitives.h::zp_start_lease_task
.. autocfunction:: primitives.h::zp_stop_lease_task
.. autocfunction:: primitives.h::zp_read_options_default
.. autocfunction:: primitives.h::zp_read
.. autocfunction:: primitives.h::zp_send_keep_alive_options_default
.. autocfunction:: primitives.h::zp_send_keep_alive