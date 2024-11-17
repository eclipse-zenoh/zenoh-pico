..
.. Copyright (c) 2024 ZettaScale Technology
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

********
Concepts
********

Types Classification
====================

Zenoh-Pico types fall into these categories:

- Owned types: `z_owned_xxx_t`
- Loaned types: `z_loaned_xxx_t`
- Moved types: `z_moved_xxx_t`
- View types: `z_view_xxx_t`
- Option structures: `z_xxx_options_t`
- Enums and plain data structures: `z_xxx_t`

.. _owned_types_concept:

Owned Types `z_owned_xxx_t`
---------------------------

The Zenoh-Pico library incorporates concepts like ownership, moving, and borrowing.

Types prefixed with `z_owned_xxx_t` "own" external resources (e.g., memory, file descriptors). 
These types must be destroyed at the end of their lifecycle using the `z_xxx_drop` function or 
the `z_drop` macro. Example:

.. code-block:: c

    z_owned_string_t s;
    z_string_copy_from_str(&s, "Hello, world!");
    //...
    z_drop(z_move(s));

Owned objects can be passed to functions in two ways: by moving (`z_moved_xxx_t`) or 
loaning (`z_loaned_xxx_t`).

.. _loaned_types_concept:

Loaned Types `z_loaned_xxx_t`
-----------------------------

To temporarily pass an owned object, it can be loaned using `z_xxx_loan` functions, which return 
a pointer to the corresponding `z_loaned_xxx_t`. For readability, the generic macro `z_loan` is also available.

Functions accepting a loaned object can either read (`const z_loaned_xxx_t*`) or read and 
modify (`z_loaned_xxx_t*`) the object. In both cases, ownership remains with the caller. Example:

.. code-block:: c

    z_owned_string_t s, s1;
    z_string_copy_from_str(&s, "Hello, world!");
    // notice that the prototype of z_string_clone is
    // void z_string_clone(z_owned_string_t* dst, const z_loaned_string_t* src);
    // I.e. the only way to pass the source string is by loaning it
    z_string_clone(&s1, z_loan(s));
    //...
    z_drop(z_move(s));
    z_drop(z_move(s1));

.. _moved_types_concept:

Moved types `z_moved_xxx_t`
---------------------------

When a function accepts a `z_moved_xxx_t*` parameter, it takes ownership of the passed object. 
To pass the object, use the `z_xxx_move` function or the `z_move` macro.

Once the object is moved, the caller should no longer use it. While calling `z_drop` is safe, 
it's not required. Note that `z_drop` itself takes ownership, so `z_move` is also needed in this case. Example:

.. code-block:: c
    
    z_owned_config_t cfg;
    z_config_default(&cfg);
    z_owned_session_t session;
    // session takes ownership of the config
    if (z_open(&session, z_move(cfg)) == Z_OK) {
        //...
        z_drop(z_move(session));
    }
    // z_drop(z_move(cfg)); // this is safe but useless

.. _view_types_concept:

View Types `z_view_xxx_t`
-------------------------

`z_view_xxx_t` types are reference types that point to external data. These values do not need to be dropped and 
remain valid only as long as the data they reference is valid. Typically the view types are the variants of
owned types that do not own the data. This allows to use view and owned types interchangeably.

.. code-block:: c

    z_owned_string_t owned;
    z_string_copy_from_str(&owned, "Hello, world!");
    z_view_string_t view;
    z_view_string_from_str(&view, "Hello, another world!");
    z_owned_string_t dst;
    z_string_clone(&dst, z_loan(owned));
    z_drop(z_move(dst));
    z_string_clone(&dst, z_loan(view));
    z_drop(z_move(dst));
    z_drop(z_move(owned)); // but no need to drop view

Options Structures `z_xxx_options_t`
------------------------------------

`z_xxx_options_t` are Plain Old Data (POD) structures used to pass multiple parameters to functions. This makes API 
compact and allows to extend the API keeping backward compatibility.

Note that when an "options" structure contains `z_moved_xxx_t*` fields, assigning `z_move` to this field does not 
affect the owned object. However, passing the structure to a function transfers ownership of the object. Example:

.. code-block:: c

    // assume that we want to mark our message with some metadate of type int64_t
    z_publisher_put_options_t options;
    z_publisher_put_options_default(&options);
    int64_t metadata = 42;
    z_owned_bytes_t attachment;
    ze_serialize_int64(&attachment, metadata);
    options.attachment = z_move(attachment); // the data itself is still in the `attachment`

    z_owned_bytes_t payload;
    z_bytes_copy_from_str(&payload, "Don't panic!"); 
    z_publisher_put(z_loan(pub), z_move(payload), &options);
    // the `payload` and `attachment` are consumed by the `z_publisher_put` function


Other Structures and Enums `z_xxx_t`
-----------------------------------------

Types named `z_xxx_t` are copyable, and can be passed by value. Some of them are just plain data structures or enums, like 
`z_timestamp_t`, `z_priority_t`. Some are temporary data access structures, like `z_bytes_slice_iterator_t`, `z_bytes_reader_t`, etc.

.. code-block:: c

    z_timestamp_t ts;
    z_timestamp_new(&ts, z_loan(session));
    z_timestamp_t ts1 = ts;

Name Prefixes `z_`, `zp_`
================================

Most functions and types in the C API use the `z_` prefix, which applies to the common zenoh C API
(currently Rust-based zenoh-c and pure C zenoh-pico).

The `zp_` prefix is used for functions that are exclusive to zenoh-pico, zenoh-c uses the `zc_` prefix for the same purpose.
