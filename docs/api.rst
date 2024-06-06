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
.. autocenum:: constants.h::z_encoding_id_t
.. autocenum:: constants.h::z_consolidation_mode_t
.. autocenum:: constants.h::z_reliability_t
.. autocenum:: constants.h::z_reply_tag_t
.. autocenum:: constants.h::z_congestion_control_t
.. autocenum:: constants.h::z_priority_t
.. autocenum:: constants.h::z_query_target_t

Data Structures
~~~~~~~~~~~~~~~

.. autoctype:: types.h::z_zint_t
.. autoctype:: types.h::z_id_t
.. autoctype:: types.h::z_timestamp_t
.. autoctype:: types.h::z_subscriber_options_t
.. autoctype:: types.h::z_query_consolidation_t
.. autoctype:: types.h::z_publisher_options_t
.. autoctype:: types.h::z_queryable_options_t
.. autoctype:: types.h::z_query_reply_options_t
.. autoctype:: types.h::z_put_options_t
.. autoctype:: types.h::z_delete_options_t
.. autoctype:: types.h::z_publisher_put_options_t
.. autoctype:: types.h::z_publisher_delete_options_t
.. autoctype:: types.h::z_get_options_t
.. autoctype:: types.h::zp_task_read_options_t
.. autoctype:: types.h::zp_task_lease_options_t
.. autoctype:: types.h::zp_read_options_t
.. autoctype:: types.h::zp_send_keep_alive_options_t
.. autoctype:: types.h::zp_send_keep_join_options_t
.. autoctype:: types.h::z_qos_t

Owned Types
~~~~~~~~~~~

TODO: owned type description

.. autoctype:: types.h::z_owned_bytes_t
.. autoctype:: types.h::z_owned_string_t
.. autoctype:: types.h::z_owned_keyexpr_t
.. autoctype:: types.h::z_owned_config_t
.. autoctype:: types.h::z_owned_scouting_config_t
.. autoctype:: types.h::z_owned_session_t
.. autoctype:: types.h::z_owned_subscriber_t
.. autoctype:: types.h::z_owned_publisher_t
.. autoctype:: types.h::z_owned_queryable_t
.. autoctype:: types.h::z_owned_query_t
.. autoctype:: types.h::z_owned_encoding_t
.. autoctype:: types.h::z_owned_value_t
.. autoctype:: types.h::z_owned_sample_t
.. autoctype:: types.h::z_owned_hello_t
.. autoctype:: types.h::z_owned_reply_t
.. autoctype:: types.h::z_owned_string_array_t
.. autoctype:: types.h::z_owned_bytes_map_t

Loaned Types
~~~~~~~~~~~

TODO: loaned type description

.. autoctype:: types.h::z_loaned_bytes_t
.. autoctype:: types.h::z_loaned_string_t
.. autoctype:: types.h::z_loaned_keyexpr_t
.. autoctype:: types.h::z_loaned_config_t
.. autoctype:: types.h::z_loaned_scouting_config_t
.. autoctype:: types.h::z_loaned_session_t
.. autoctype:: types.h::z_loaned_subscriber_t
.. autoctype:: types.h::z_loaned_publisher_t
.. autoctype:: types.h::z_loaned_queryable_t
.. autoctype:: types.h::z_loaned_query_t
.. autoctype:: types.h::z_loaned_encoding_t
.. autoctype:: types.h::z_loaned_value_t
.. autoctype:: types.h::z_loaned_sample_t
.. autoctype:: types.h::z_loaned_hello_t
.. autoctype:: types.h::z_loaned_reply_t
.. autoctype:: types.h::z_loaned_string_array_t

View Types
~~~~~~~~~~~

TODO: view type description

.. autoctype:: types.h::z_view_string_t
.. autoctype:: types.h::z_view_keyexpr_t
.. autoctype:: types.h::z_view_string_array_t

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

.. autocfunction:: primitives.h::z_view_str_wrap
.. autocfunction:: primitives.h::z_view_keyexpr_from_string
.. autocfunction:: primitives.h::z_view_keyexpr_from_string_unchecked
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
.. autocfunction:: primitives.h::zp_encoding_make
.. autocfunction:: primitives.h::zp_encoding_default
.. autocfunction:: primitives.h::z_encoding_check
.. autocfunction:: primitives.h::z_encoding_drop
.. autocfunction:: primitives.h::z_encoding_loan
.. autocfunction:: primitives.h::z_encoding_move
.. autocfunction:: primitives.h::z_encoding_null
.. autocfunction:: primitives.h::z_value_payload
.. autocfunction:: primitives.h::z_bytes_len
.. autocfunction:: primitives.h::z_bytes_decode_into_string
.. autocfunction:: primitives.h::z_bytes_encode_from_string
.. autocfunction:: primitives.h::z_timestamp_check
.. autocfunction:: primitives.h::z_query_target_default
.. autocfunction:: primitives.h::z_query_consolidation_auto
.. autocfunction:: primitives.h::z_query_consolidation_default
.. autocfunction:: primitives.h::z_query_consolidation_latest
.. autocfunction:: primitives.h::z_query_consolidation_monotonic
.. autocfunction:: primitives.h::z_query_consolidation_none
.. autocfunction:: primitives.h::z_query_parameters
.. autocfunction:: primitives.h::z_query_value
.. autocfunction:: primitives.h::z_query_attachment
.. autocfunction:: primitives.h::z_query_keyexpr
.. autocfunction:: primitives.h::z_closure_sample
.. autocfunction:: primitives.h::z_closure_query
.. autocfunction:: primitives.h::z_closure_reply
.. autocfunction:: primitives.h::z_closure_hello
.. autocfunction:: primitives.h::z_closure_zid
.. autocfunction:: primitives.h::z_sample_loan
.. autocfunction:: primitives.h::z_string_data
.. autocfunction:: primitives.h::z_scout
.. autocfunction:: primitives.h::z_open
.. autocfunction:: primitives.h::z_close
.. autocfunction:: primitives.h::z_info_peers_zid
.. autocfunction:: primitives.h::z_info_routers_zid
.. autocfunction:: primitives.h::z_info_zid
.. autocfunction:: primitives.h::z_sample_keyexpr
.. autocfunction:: primitives.h::z_sample_payload
.. autocfunction:: primitives.h::z_sample_timestamp
.. autocfunction:: primitives.h::z_sample_encoding
.. autocfunction:: primitives.h::z_sample_kind
.. autocfunction:: primitives.h::z_sample_qos
.. autocfunction:: primitives.h::z_sample_attachment
.. autocfunction:: primitives.h::z_put_options_default
.. autocfunction:: primitives.h::z_delete_options_default
.. autocfunction:: primitives.h::z_put
.. autocfunction:: primitives.h::z_delete
.. autocfunction:: primitives.h::z_publisher_options_default
.. autocfunction:: primitives.h::z_declare_publisher
.. autocfunction:: primitives.h::z_undeclare_publisher
.. autocfunction:: primitives.h::z_publisher_put_options_default
.. autocfunction:: primitives.h::z_publisher_delete_options_default
.. autocfunction:: primitives.h::z_publisher_put
.. autocfunction:: primitives.h::z_publisher_delete
.. autocfunction:: primitives.h::z_get_options_default
.. autocfunction:: primitives.h::z_get
.. autocfunction:: primitives.h::z_reply_is_ok
.. autocfunction:: primitives.h::z_reply_ok
.. autocfunction:: primitives.h::z_reply_err
.. autocfunction:: primitives.h::z_queryable_options_default
.. autocfunction:: primitives.h::z_declare_queryable
.. autocfunction:: primitives.h::z_undeclare_queryable
.. autocfunction:: primitives.h::z_query_reply_options_default
.. autocfunction:: primitives.h::z_query_reply
.. autocfunction:: primitives.h::z_keyexpr_new
.. autocfunction:: primitives.h::z_declare_keyexpr
.. autocfunction:: primitives.h::z_undeclare_keyexpr
.. autocfunction:: primitives.h::z_subscriber_options_default
.. autocfunction:: primitives.h::z_declare_subscriber
.. autocfunction:: primitives.h::z_undeclare_subscriber
.. autocfunction:: primitives.h::z_subscriber_keyexpr
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
.. autocfunction:: primitives.h::zp_send_join_options_default
.. autocfunction:: primitives.h::zp_send_join