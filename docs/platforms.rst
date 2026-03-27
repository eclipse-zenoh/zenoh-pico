..
.. Copyright (c) 2026 ZettaScale Technology
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

**********************
Platforms and Backends
**********************

Zenoh-Pico builds are composed from a platform profile and a set of transport backends.

Platform selection
==================

``ZP_PLATFORM`` is the primary build option. A platform profile selects:

* the system-layer implementation used by the build;
* the default ``stream``, ``datagram`` and ``rawio`` backends.

Backend families
================

Zenoh-Pico uses three lower backend families:

* ``stream`` for TCP-like transports;
* ``datagram`` for UDP-like transports;
* ``rawio`` for serial and other byte-stream devices.

TLS and the serial framing protocol are upper layers built on top of these backends.

Overriding backend defaults
===========================

A platform profile can be overridden with explicit backend choices:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_PLATFORM=linux \
     -DZP_STREAM_BACKEND=tcp_posix \
     -DZP_DATAGRAM_BACKEND=udp_posix \
     -DZP_RAWIO_BACKEND=tty_posix

Adding a new platform
=====================

There are two ways to add a platform descriptor:

* built in: add a file under Zenoh-Pico's ``cmake/platforms/`` directory;
* external: keep the descriptor in your own tree and register its directory through ``ZP_EXTERNAL_CMAKE``.

The platform name is the descriptor file name without the ``.cmake`` suffix. For example,
``cmake/platforms/myrtos.cmake`` defines the ``myrtos`` platform profile.

In the common built-in case, adding a new platform only requires adding a new file under ``cmake/platforms/``.

That file should:

* declare the platform-specific compile definitions and sources;
* choose the default ``stream``, ``datagram`` and ``rawio`` backends for the platform.
* optionally add include directories or adjust ``CHECK_THREADS`` if the platform needs it.

A platform descriptor defines the system-layer sources for the platform and selects the default lower backends.
Existing backend implementations should be reused when only part of the platform support differs.

Example built-in platform descriptor:

.. code-block:: cmake

   # cmake/platforms/myrtos.cmake

   zp_platform_add_definition(ZENOH_MYRTOS)
   zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/system/myrtos/system.c")
   # Optional:
   # zp_platform_add_include_dirs("${PROJECT_SOURCE_DIR}/src/system/myrtos/include")
   # set(CHECK_THREADS OFF)
   zp_platform_set_stream_backend(tcp_lwip)
   zp_platform_set_datagram_backend(udp_lwip)
   zp_platform_set_rawio_backend(uart_myrtos)

Then configure Zenoh-Pico with:

.. code-block:: bash

   cmake -S . -B build -DZP_PLATFORM=myrtos

If the same kind of descriptor is provided by an external integration, keep the descriptor in the external tree and
register its directory through a small CMake shim:

.. code-block:: cmake

   # /path/to/myrtos/zenoh_pico.cmake
   zp_add_platform_dir("/path/to/myrtos/cmake/platforms")

The external tree then provides the actual platform descriptor:

.. code-block:: cmake

   # /path/to/myrtos/cmake/platforms/myrtos.cmake
   zp_platform_add_definition(ZENOH_MYRTOS)
   zp_platform_add_sources("/path/to/myrtos/src/system/myrtos/system.c")
   zp_platform_set_stream_backend(tcp_lwip)
   zp_platform_set_datagram_backend(udp_lwip)
   zp_platform_set_rawio_backend(uart_myrtos)

Then configure Zenoh-Pico with:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_CMAKE=/path/to/myrtos/zenoh_pico.cmake \
     -DZP_PLATFORM=myrtos

Adding a backend
================

Backend descriptors follow the same pattern as platform descriptors:

* built in: add a file under ``cmake/backends/stream/``, ``cmake/backends/datagram/`` or
  ``cmake/backends/rawio/``;
* external: keep the descriptor in your own tree and register its directory through ``ZP_EXTERNAL_CMAKE``.

The backend name is the descriptor file name without the ``.cmake`` suffix. For example,
``cmake/backends/rawio/uart_myrtos.cmake`` defines the ``uart_myrtos`` backend.

Example built-in backend descriptor:

.. code-block:: cmake

   # cmake/backends/rawio/uart_myrtos.cmake
   zp_register_rawio_backend(
     NAME uart_myrtos
     SYMBOL _z_uart_myrtos_rawio_ops
     SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_myrtos.c"
   )

An external integration can register its own backend directory:

.. code-block:: cmake

   # /path/to/myrtos/zenoh_pico.cmake
   zp_add_rawio_backend_dir("/path/to/myrtos/cmake/backends/rawio")

The backend can then be selected either from a platform descriptor:

.. code-block:: cmake

   zp_platform_set_rawio_backend(uart_myrtos)

or directly at configure time:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_CMAKE=/path/to/myrtos/zenoh_pico.cmake \
     -DZP_PLATFORM=linux \
     -DZP_RAWIO_BACKEND=uart_myrtos

Explicit system-layer selection
===============================

``ZP_SYSTEM_LAYER`` is an optional build setting. Most builds should set ``ZP_PLATFORM`` and leave
``ZP_SYSTEM_LAYER`` unset.

It is mainly intended for explicit builds and integration scenarios that need to choose the system-layer
implementation directly.

This is useful when an external integration wants to expose a stable system-layer name without requiring users to know
the platform descriptor name.

If the system-layer name and the descriptor file name differ, register the mapping with
``zp_register_system_layer_profile()`` in the external CMake file.

Example:

.. code-block:: cmake

   # /path/to/myrtos/cmake/platforms/myboard.cmake
   zp_platform_add_definition(ZENOH_MYRTOS)
   zp_platform_add_sources("/path/to/myrtos/src/system/myrtos/system.c")
   zp_platform_set_stream_backend(tcp_lwip)
   zp_platform_set_datagram_backend(udp_lwip)
   zp_platform_set_rawio_backend(uart_myrtos)

.. code-block:: cmake

   # /path/to/myrtos/zenoh_pico.cmake
   zp_add_platform_dir("/path/to/myrtos/cmake/platforms")
   zp_register_system_layer_profile(SYSTEM_LAYER myrtos PROFILE myboard)

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_CMAKE=/path/to/myrtos/zenoh_pico.cmake \
     -DZP_SYSTEM_LAYER=myrtos

In this example the descriptor file is named ``myboard.cmake``, but the external integration wants users to select the
system layer as ``myrtos``. ``ZP_SYSTEM_LAYER`` is what makes that possible. If the profile name and the system-layer
name are the same, no extra registration is needed.

In that case, the external integration should provide:

* a platform descriptor under its own ``cmake/platforms/`` directory, registered through ``ZP_EXTERNAL_CMAKE``;
* the system-layer sources, typically under ``src/system/<name>/``, for platform runtime and socket primitives;
* optional backend sources under ``src/link/backend/stream/``, ``src/link/backend/datagram/`` or
  ``src/link/backend/rawio/`` only when the built-in backends cannot be reused for that system layer.
