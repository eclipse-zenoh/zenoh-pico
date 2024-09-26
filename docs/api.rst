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
.. autocenum:: constants.h::z_consolidation_mode_t
.. autocenum:: constants.h::z_reliability_t
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
.. autoctype:: types.h::zp_send_join_options_t
.. autoctype:: types.h::z_bytes_reader_t
.. autoctype:: types.h::z_bytes_slice_iterator_t
.. autoctype:: types.h::z_bytes_writer_t
  

Owned Types
~~~~~~~~~~~

See :ref:`owned_types_concept`

.. c:type:: z_owned_slice_t
  
  Represents an array of bytes.

.. c:type:: z_owned_bytes_t
  
  Represents an array of bytes container.

.. c:type:: z_owned_string_t

  Represents a string without null-terminator.

.. c:type:: z_owned_keyexpr_t

  Represents a key expression in Zenoh.

.. c:type:: z_owned_config_t

  Represents a Zenoh configuration, used to configure Zenoh sessions upon opening.

.. c:type:: z_owned_session_t

  Represents a Zenoh Session.

.. c:type:: z_owned_subscriber_t

  Represents a Zenoh Subscriber entity.

.. c:type:: z_owned_publisher_t

  Represents a Zenoh Publisher entity.

.. c:type:: z_owned_queryable_t

  Represents a Zenoh Queryable entity.

.. c:type:: z_owned_query_t

  Represents a Zenoh Query entity, received by Zenoh queryable entities.

.. c:type:: z_owned_encoding_t

  Represents the encoding of a payload, in a MIME-like format.

.. c:type:: z_owned_reply_err_t

  Represents a Zenoh reply error value.

.. c:type:: z_owned_sample_t

  Represents a data sample.

.. c:type:: z_owned_hello_t

  Represents the content of a `hello` message returned by a zenoh entity as a reply to a `scout` message.

.. c:type:: z_owned_reply_t

  Represents the reply to a query.

.. c:type:: z_owned_string_array_t

  Represents an array of non null-terminated string.

Loaned Types
~~~~~~~~~~~

See :ref:`loaned_types_concept`

.. c:type:: z_loaned_slice_t

  Represents an array of bytes.

.. c:type:: z_loaned_bytes_t

  Represents an array of bytes container.

.. c:type:: z_loaned_string_t

  Represents a string without null-terminator.

.. c:type:: z_loaned_keyexpr_t

  Represents a key expression in Zenoh.

.. c:type:: z_loaned_config_t

  Represents a Zenoh configuration, used to configure Zenoh sessions upon opening.

.. c:type:: z_loaned_session_t

  Represents a Zenoh Session.

.. c:type:: z_loaned_subscriber_t

  Represents a Zenoh Subscriber entity.

.. c:type:: z_loaned_publisher_t

  Represents a Zenoh Publisher entity.

.. c:type:: z_loaned_queryable_t

  Represents a Zenoh Queryable entity.
  
.. c:type:: z_loaned_query_t

  Represents a Zenoh Query entity, received by Zenoh queryable entities.

.. c:type:: z_loaned_encoding_t

  Represents the encoding of a payload, in a MIME-like format.

.. c:type:: z_loaned_reply_err_t

  Represents a Zenoh reply error.

.. c:type:: z_loaned_sample_t

  Represents a data sample.

.. c:type:: z_loaned_hello_t

  Represents the content of a `hello` message returned by a zenoh entity as a reply to a `scout` message.

.. c:type:: z_loaned_reply_t

  Represents the reply to a query.

.. c:type:: z_loaned_string_array_t

  Represents an array of non null-terminated string.

View Types
~~~~~~~~~~~

See :ref:`view_types_concept`

.. c:type:: z_view_string_t

  Represents a string without null-terminator.

.. c:type:: z_view_slice_t

  Represents an array of bytes.

.. c:type:: z_view_keyexpr_t

  Represents a key expression in Zenoh.

.. c:type:: z_view_string_array_t

  Represents an array of non null-terminated string.

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

Represents a `sample` closure.

.. c:type:: types.h::z_owned_closure_sample_t

Represents a loaned `sample` closure.

.. c:type:: types.h::z_loaned_closure_sample_t

Represents a `query` closure.

.. c:type:: types.h::z_owned_closure_query_t

Represents a loaned `query` closure.

.. c:type:: types.h::z_loaned_closure_query_t

Represents a `reply` closure.

.. c:type:: types.h::z_owned_closure_reply_t

Represents a loaned `reply` closure.

.. c:type:: types.h::z_loaned_closure_reply_t

Represents a `hello` closure.

.. c:type:: types.h::z_owned_closure_hello_t

Represents a loaned `hello` closure.

.. c:type:: types.h::z_loaned_closure_hello_t

Represents a `Zenoh id` closure.

.. c:type:: types.h::z_owned_closure_zid_t

Represents a loaned `Zenoh id` closure.

.. c:type:: types.h::z_loaned_closure_zid_t


Zenoh Functions
---------------

Macros
~~~~~~
.. autocmacro:: macros.h::z_loan
.. autocmacro:: macros.h::z_move
.. autocmacro:: macros.h::z_clone
.. autocmacro:: macros.h::z_drop
.. autocmacro:: macros.h::z_closure

Primitives
~~~~~~~~~~

.. autocfunction:: primitives.h::z_view_string_from_str
.. autocfunction:: primitives.h::z_view_string_from_substr
.. autocfunction:: primitives.h::z_view_keyexpr_from_str
.. autocfunction:: primitives.h::z_view_keyexpr_from_str_unchecked
.. autocfunction:: primitives.h::z_view_keyexpr_from_str_autocanonize
.. autocfunction:: primitives.h::z_keyexpr_as_view_string
.. autocfunction:: primitives.h::z_keyexpr_is_canon
.. autocfunction:: primitives.h::z_keyexpr_canonize
.. autocfunction:: primitives.h::z_keyexpr_canonize_null_terminated
.. autocfunction:: primitives.h::z_keyexpr_includes
.. autocfunction:: primitives.h::z_keyexpr_intersects
.. autocfunction:: primitives.h::z_keyexpr_equals
.. autocfunction:: primitives.h::z_keyexpr_relation_to
.. autocfunction:: primitives.h::z_keyexpr_concat
.. autocfunction:: primitives.h::z_keyexpr_join
.. autocfunction:: primitives.h::z_config_new
.. autocfunction:: primitives.h::z_config_default
.. autocfunction:: primitives.h::zp_config_get
.. autocfunction:: primitives.h::zp_config_insert
.. autocfunction:: primitives.h::z_encoding_from_str
.. autocfunction:: primitives.h::z_encoding_from_substr
.. autocfunction:: primitives.h::z_encoding_set_schema_from_str
.. autocfunction:: primitives.h::z_encoding_set_schema_from_substr
.. autocfunction:: primitives.h::z_encoding_to_string
.. autocfunction:: primitives.h::z_encoding_equals
.. autocfunction:: primitives.h::z_reply_err_payload
.. autocfunction:: primitives.h::z_reply_err_encoding
.. autocfunction:: primitives.h::z_slice_from_buf
.. autocfunction:: primitives.h::z_slice_copy_from_buf
.. autocfunction:: primitives.h::z_slice_data
.. autocfunction:: primitives.h::z_slice_len
.. autocfunction:: primitives.h::z_slice_empty
.. autocfunction:: primitives.h::z_slice_is_empty
.. autocfunction:: serialization.h::ze_deserializer_deserialize_int8
.. autocfunction:: serialization.h::ze_deserializer_deserialize_int16
.. autocfunction:: serialization.h::ze_deserializer_deserialize_int32
.. autocfunction:: serialization.h::ze_deserializer_deserialize_int64
.. autocfunction:: serialization.h::ze_deserializer_deserialize_uint8
.. autocfunction:: serialization.h::ze_deserializer_deserialize_uint16
.. autocfunction:: serialization.h::ze_deserializer_deserialize_uint32
.. autocfunction:: serialization.h::ze_deserializer_deserialize_uint64
.. autocfunction:: serialization.h::ze_deserializer_deserialize_float
.. autocfunction:: serialization.h::ze_deserializer_deserialize_double
.. autocfunction:: serialization.h::ze_deserializer_deserialize_slice
.. autocfunction:: serialization.h::ze_deserializer_deserialize_string
.. autocfunction:: serialization.h::ze_deserializer_deserialize_sequence_begin
.. autocfunction:: serialization.h::ze_deserializer_deserialize_sequence_end
.. autocfunction:: serialization.h::ze_serializer_serialize_int8
.. autocfunction:: serialization.h::ze_serializer_serialize_int16
.. autocfunction:: serialization.h::ze_serializer_serialize_int32
.. autocfunction:: serialization.h::ze_serializer_serialize_int64
.. autocfunction:: serialization.h::ze_serializer_serialize_uint8
.. autocfunction:: serialization.h::ze_serializer_serialize_uint16
.. autocfunction:: serialization.h::ze_serializer_serialize_uint32
.. autocfunction:: serialization.h::ze_serializer_serialize_uint64
.. autocfunction:: serialization.h::ze_serializer_serialize_float
.. autocfunction:: serialization.h::ze_serializer_serialize_double
.. autocfunction:: serialization.h::ze_serializer_serialize_slice
.. autocfunction:: serialization.h::ze_serializer_serialize_buf
.. autocfunction:: serialization.h::ze_serializer_serialize_string
.. autocfunction:: serialization.h::ze_serializer_serialize_str
.. autocfunction:: serialization.h::ze_serializer_serialize_sequence_begin
.. autocfunction:: serialization.h::ze_serializer_serialize_sequence_end
.. autocfunction:: serialization.h::ze_deserialize_into_int8
.. autocfunction:: serialization.h::ze_deserialize_into_int16
.. autocfunction:: serialization.h::ze_deserialize_into_int32
.. autocfunction:: serialization.h::ze_deserialize_into_int64
.. autocfunction:: serialization.h::ze_deserialize_into_uint8
.. autocfunction:: serialization.h::ze_deserialize_into_uint16
.. autocfunction:: serialization.h::ze_deserialize_into_uint32
.. autocfunction:: serialization.h::ze_deserialize_into_uint64
.. autocfunction:: serialization.h::ze_deserialize_into_float
.. autocfunction:: serialization.h::ze_deserialize_into_double
.. autocfunction:: serialization.h::ze_deserialize_into_slice
.. autocfunction:: serialization.h::ze_deserialize_into_string
.. autocfunction:: serialization.h::ze_serialize_from_int8
.. autocfunction:: serialization.h::ze_serialize_from_int16
.. autocfunction:: serialization.h::ze_serialize_from_int32
.. autocfunction:: serialization.h::ze_serialize_from_int64
.. autocfunction:: serialization.h::ze_serialize_from_uint8
.. autocfunction:: serialization.h::ze_serialize_from_uint16
.. autocfunction:: serialization.h::ze_serialize_from_uint32
.. autocfunction:: serialization.h::ze_serialize_from_uint64
.. autocfunction:: serialization.h::ze_serialize_from_float
.. autocfunction:: serialization.h::ze_serialize_from_double
.. autocfunction:: serialization.h::ze_serialize_from_slice
.. autocfunction:: serialization.h::ze_serialize_from_buf
.. autocfunction:: serialization.h::ze_serialize_from_string
.. autocfunction:: serialization.h::ze_serialize_from_str
.. autocfunction:: primitives.h::z_bytes_from_slice
.. autocfunction:: primitives.h::z_bytes_copy_from_slice
.. autocfunction:: primitives.h::z_bytes_from_buf
.. autocfunction:: primitives.h::z_bytes_copy_from_buf
.. autocfunction:: primitives.h::z_bytes_from_static_buf
.. autocfunction:: primitives.h::z_bytes_from_string
.. autocfunction:: primitives.h::z_bytes_copy_from_string
.. autocfunction:: primitives.h::z_bytes_from_str
.. autocfunction:: primitives.h::z_bytes_copy_from_str
.. autocfunction:: primitives.h::z_bytes_from_static_str
.. autocfunction:: primitives.h::z_bytes_empty
.. autocfunction:: primitives.h::z_bytes_len
.. autocfunction:: primitives.h::z_bytes_is_empty
.. autocfunction:: primitives.h::z_bytes_get_slice_iterator
.. autocfunction:: primitives.h::z_bytes_slice_iterator_next
.. autocfunction:: primitives.h::z_bytes_get_reader
.. autocfunction:: primitives.h::z_bytes_reader_read
.. autocfunction:: primitives.h::z_bytes_reader_seek
.. autocfunction:: primitives.h::z_bytes_reader_tell
.. autocfunction:: primitives.h::z_bytes_get_writer
.. autocfunction:: primitives.h::z_bytes_writer_write_all
.. autocfunction:: primitives.h::z_bytes_writer_append
.. autocfunction:: primitives.h::z_timestamp_check
.. autocfunction:: primitives.h::z_query_target_default
.. autocfunction:: primitives.h::z_query_consolidation_auto
.. autocfunction:: primitives.h::z_query_consolidation_default
.. autocfunction:: primitives.h::z_query_consolidation_latest
.. autocfunction:: primitives.h::z_query_consolidation_monotonic
.. autocfunction:: primitives.h::z_query_consolidation_none
.. autocfunction:: primitives.h::z_query_parameters
.. autocfunction:: primitives.h::z_query_payload
.. autocfunction:: primitives.h::z_query_encoding
.. autocfunction:: primitives.h::z_query_attachment
.. autocfunction:: primitives.h::z_query_keyexpr
.. autocfunction:: primitives.h::z_closure_sample
.. autocfunction:: primitives.h::z_closure_query
.. autocfunction:: primitives.h::z_closure_reply
.. autocfunction:: primitives.h::z_closure_hello
.. autocfunction:: primitives.h::z_closure_zid
.. autocfunction:: primitives.h::z_sample_loan
.. autocfunction:: primitives.h::z_string_data
.. autocfunction:: primitives.h::z_string_len
.. autocfunction:: primitives.h::z_string_from_str
.. autocfunction:: primitives.h::z_string_copy_from_str
.. autocfunction:: primitives.h::z_string_copy_from_substr
.. autocfunction:: primitives.h::z_string_empty
.. autocfunction:: primitives.h::z_string_is_empty
.. autocfunction:: primitives.h::z_string_as_slice
.. autocfunction:: primitives.h::z_hello_zid
.. autocfunction:: primitives.h::z_hello_whatami
.. autocfunction:: primitives.h::z_hello_locators
.. autocfunction:: primitives.h::zp_hello_locators
.. autocfunction:: primitives.h::z_whatami_to_view_string
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
.. autocfunction:: primitives.h::z_sample_reliability
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
.. autocfunction:: primitives.h::z_query_reply_del_options_default
.. autocfunction:: primitives.h::z_query_reply_del
.. autocfunction:: primitives.h::z_query_reply_err_options_default
.. autocfunction:: primitives.h::z_query_reply_err
.. autocfunction:: primitives.h::z_keyexpr_from_str
.. autocfunction:: primitives.h::z_keyexpr_from_substr
.. autocfunction:: primitives.h::z_keyexpr_from_str_autocanonize
.. autocfunction:: primitives.h::z_keyexpr_from_substr_autocanonize
.. autocfunction:: primitives.h::z_string_array_new
.. autocfunction:: primitives.h::z_string_array_push_by_alias
.. autocfunction:: primitives.h::z_string_array_push_by_copy
.. autocfunction:: primitives.h::z_string_array_get
.. autocfunction:: primitives.h::z_string_array_len
.. autocfunction:: primitives.h::z_string_array_is_empty
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
.. autocfunction:: encoding.h::z_encoding_zenoh_bytes
.. autocfunction:: encoding.h::z_encoding_zenoh_int8
.. autocfunction:: encoding.h::z_encoding_zenoh_int16
.. autocfunction:: encoding.h::z_encoding_zenoh_int32
.. autocfunction:: encoding.h::z_encoding_zenoh_int64
.. autocfunction:: encoding.h::z_encoding_zenoh_int128
.. autocfunction:: encoding.h::z_encoding_zenoh_uint8
.. autocfunction:: encoding.h::z_encoding_zenoh_uint16
.. autocfunction:: encoding.h::z_encoding_zenoh_uint32
.. autocfunction:: encoding.h::z_encoding_zenoh_uint64
.. autocfunction:: encoding.h::z_encoding_zenoh_uint128
.. autocfunction:: encoding.h::z_encoding_zenoh_float32
.. autocfunction:: encoding.h::z_encoding_zenoh_float64
.. autocfunction:: encoding.h::z_encoding_zenoh_bool
.. autocfunction:: encoding.h::z_encoding_zenoh_string
.. autocfunction:: encoding.h::z_encoding_zenoh_error
.. autocfunction:: encoding.h::z_encoding_application_octet_stream
.. autocfunction:: encoding.h::z_encoding_text_plain
.. autocfunction:: encoding.h::z_encoding_application_json
.. autocfunction:: encoding.h::z_encoding_text_json
.. autocfunction:: encoding.h::z_encoding_application_cdr
.. autocfunction:: encoding.h::z_encoding_application_cbor
.. autocfunction:: encoding.h::z_encoding_application_yaml
.. autocfunction:: encoding.h::z_encoding_text_yaml
.. autocfunction:: encoding.h::z_encoding_text_json5
.. autocfunction:: encoding.h::z_encoding_application_python_serialized_object
.. autocfunction:: encoding.h::z_encoding_application_protobuf
.. autocfunction:: encoding.h::z_encoding_application_java_serialized_object
.. autocfunction:: encoding.h::z_encoding_application_openmetrics_text
.. autocfunction:: encoding.h::z_encoding_image_png
.. autocfunction:: encoding.h::z_encoding_image_jpeg
.. autocfunction:: encoding.h::z_encoding_image_gif
.. autocfunction:: encoding.h::z_encoding_image_bmp
.. autocfunction:: encoding.h::z_encoding_image_webp
.. autocfunction:: encoding.h::z_encoding_application_xml
.. autocfunction:: encoding.h::z_encoding_application_x_www_form_urlencoded
.. autocfunction:: encoding.h::z_encoding_text_html
.. autocfunction:: encoding.h::z_encoding_text_xml
.. autocfunction:: encoding.h::z_encoding_text_css
.. autocfunction:: encoding.h::z_encoding_text_javascript
.. autocfunction:: encoding.h::z_encoding_text_markdown
.. autocfunction:: encoding.h::z_encoding_text_csv
.. autocfunction:: encoding.h::z_encoding_application_sql
.. autocfunction:: encoding.h::z_encoding_application_coap_payload
.. autocfunction:: encoding.h::z_encoding_application_json_patch_json
.. autocfunction:: encoding.h::z_encoding_application_json_seq
.. autocfunction:: encoding.h::z_encoding_application_jsonpath
.. autocfunction:: encoding.h::z_encoding_application_jwt
.. autocfunction:: encoding.h::z_encoding_application_mp4
.. autocfunction:: encoding.h::z_encoding_application_soap_xml
.. autocfunction:: encoding.h::z_encoding_application_yang
.. autocfunction:: encoding.h::z_encoding_audio_aac
.. autocfunction:: encoding.h::z_encoding_audio_flac
.. autocfunction:: encoding.h::z_encoding_audio_mp4
.. autocfunction:: encoding.h::z_encoding_audio_ogg
.. autocfunction:: encoding.h::z_encoding_audio_vorbis
.. autocfunction:: encoding.h::z_encoding_video_h261
.. autocfunction:: encoding.h::z_encoding_video_h263
.. autocfunction:: encoding.h::z_encoding_video_h264
.. autocfunction:: encoding.h::z_encoding_video_h265
.. autocfunction:: encoding.h::z_encoding_video_h266
.. autocfunction:: encoding.h::z_encoding_video_mp4
.. autocfunction:: encoding.h::z_encoding_video_ogg
.. autocfunction:: encoding.h::z_encoding_video_raw
.. autocfunction:: encoding.h::z_encoding_video_vp8
.. autocfunction:: encoding.h::z_encoding_video_vp9
