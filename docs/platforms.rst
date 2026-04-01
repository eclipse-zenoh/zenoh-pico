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

Zenoh-Pico builds are composed from a platform profile, a system layer, and a
set of lower backends.

Platform selection
==================

``ZP_PLATFORM`` is the primary build option. A platform profile selects:

* the system-layer implementation used by the build;
* the network layer used by the build;
* the default ``tcp``, ``udp`` and ``serial`` transport backends;
* internally, the selected UDP transport backend may resolve to separate
  ``udp_unicast`` and ``udp_multicast`` lower subpaths.

Host builds can usually rely on the default detected platform profile. Generic
embedded builds should set ``ZP_PLATFORM`` explicitly.

Current lower backend families
==============================

Zenoh-Pico currently uses three main lower backend families:

* ``stream`` for TCP-like transports;
* ``datagram`` for datagram-style lower transports, currently the UDP unicast path;
* ``rawio`` for serial and other byte-stream devices.

UDP multicast currently uses a dedicated companion component because its
platform-specific pieces do not always align with the unicast ``datagram``
backend. This lower split is an internal implementation detail. TLS and the
serial framing protocol are upper layers built on top of these lower backends.

The external selection and packaging model is transport-backend-oriented around
backends such as ``tcp``, ``udp``, ``tls``, ``ws`` and ``serial``, while these
lower backend families remain internal implementation building blocks.

Overriding network and backend defaults
=======================================

A platform profile can be overridden with explicit network and backend choices:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_PLATFORM=linux \
     -DZP_NETWORK=posix \
     -DZP_TCP_BACKEND=tcp_posix \
     -DZP_UDP_BACKEND=posix \
     -DZP_SERIAL_BACKEND=tty_posix

The explicit network choice must be compatible with the selected platform profile.
In the current model, that means it must match the network selected by the platform descriptor.

For UDP specifically, ``ZP_UDP_BACKEND`` is the canonical public selection
knob. Internally it resolves to the current ``udp_unicast`` and
``udp_multicast`` lower selections. Those lower UDP subpaths are an internal
decomposition, not part of the preferred public platform API. Platform
descriptors should use ``zp_platform_set_udp_backend(...)``.

When UDP multicast support is disabled, the broader UDP transport backend now
degrades cleanly to its unicast lower path and does not require a multicast
companion backend to configure successfully.

Adding a new platform
=====================

There are two ways to add a platform descriptor:

* built in: add a file under Zenoh-Pico's ``cmake/platforms/`` directory;
* external package: export real CMake targets and, if needed, register package-provided
  platform descriptors from the package config, then load the package through
  ``ZP_EXTERNAL_PACKAGES`` plus ``find_package()``.

The platform name is the descriptor file name without the ``.cmake`` suffix. For example,
``cmake/platforms/myrtos.cmake`` defines the ``myrtos`` platform profile.

In the common built-in case, adding a new platform only requires adding a new
file under ``cmake/platforms/`` when it reuses existing system/network/backend
pieces. If the platform also needs a new system-layer implementation, add a
descriptor under ``cmake/system/`` as well.

That file should:

* choose the system layer for the platform;
* choose the network layer for the platform;
* choose the default ``tcp`` and ``serial`` transport backends for the platform;
* choose the default broader UDP transport backend for the platform;
* optionally add extra platform-only sources or include directories, or adjust
  ``CHECK_THREADS`` if the platform needs it.

A platform descriptor selects the runtime system layer, chooses the network
layer, and selects the default transport backends. Existing system, network,
socket, and backend implementations should be reused when only part of the
platform support differs.

Example built-in platform descriptor:

.. code-block:: cmake

   # cmake/platforms/myrtos.cmake

   zp_platform_set_system_layer(myrtos)
   zp_platform_set_network(myrtos)
   # Optional:
   # These extra include directories and sources are only used when building zenoh-pico itself.
   # They are not exported as public installed headers.
   # zp_platform_add_include_dirs("${PROJECT_SOURCE_DIR}/src/system/myrtos/include")
   # zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/system/myrtos/platform_extra.c")
   # set(CHECK_THREADS OFF)
   zp_platform_set_tcp_backend(tcp_lwip)
   zp_platform_set_udp_backend(lwip)
   zp_platform_set_serial_backend(uart_myrtos)

If the platform also introduces a new system-layer implementation, add a
system-layer descriptor:

.. code-block:: cmake

   # cmake/system/myrtos.cmake

   zp_register_system_layer(
     NAME myrtos
     SOURCES "${PROJECT_SOURCE_DIR}/src/system/myrtos/system.c"
     COMPILE_DEFINITIONS ZENOH_MYRTOS
   )

Then configure Zenoh-Pico with:

.. code-block:: bash

   cmake -S . -B build -DZP_PLATFORM=myrtos

Zenoh-Pico can also consume external pieces from real CMake packages. In that
model, the package config creates or imports the actual build payload as
targets, registers them through the same ``zp_register_*`` macros used by the
built-in descriptors, and, if it provides a custom platform profile name,
registers its own ``platforms/`` directory.

The package may define or import any subset of these targets:

* ``zenohpico::system_<name>``
* ``zenohpico::network_<name>``
* ``zenohpico::socket_<name>``
* ``zenohpico::tcp_backend_<name>``
* ``zenohpico::udp_backend_<name>``
* ``zenohpico::udp_unicast_backend_<name>``
* ``zenohpico::udp_multicast_backend_<name>``
* ``zenohpico::serial_backend_<name>``

Example thin external platform descriptor:

.. code-block:: cmake

   # /path/to/myrtos/cmake/platforms/myrtos.cmake
   zp_platform_set_system_layer(myrtos)
   zp_platform_set_network(myrtos)
   zp_platform_set_tcp_backend(tcp_lwip)
   zp_platform_set_udp_backend(lwip)
   zp_platform_set_serial_backend(uart_myrtos)

Example package targets:

.. code-block:: cmake

   add_library(zenohpico::system_myrtos INTERFACE IMPORTED GLOBAL)
   set_property(TARGET zenohpico::system_myrtos PROPERTY
                INTERFACE_SOURCES "/path/to/myrtos/src/system/myrtos/system.c")
   set_property(TARGET zenohpico::system_myrtos PROPERTY
                INTERFACE_COMPILE_DEFINITIONS "ZENOH_MYRTOS")
   zp_register_system_layer(NAME myrtos TARGET zenohpico::system_myrtos)

   add_library(zenohpico::network_myrtos INTERFACE IMPORTED GLOBAL)
   set_property(TARGET zenohpico::network_myrtos PROPERTY
                ZP_NETWORK_SOCKET_COMPONENT lwip)
   zp_register_network(NAME myrtos TARGET zenohpico::network_myrtos SOCKET_COMPONENT lwip)

   add_library(zenohpico::serial_backend_uart_myrtos INTERFACE IMPORTED GLOBAL)
   set_property(TARGET zenohpico::serial_backend_uart_myrtos PROPERTY
                INTERFACE_SOURCES "/path/to/myrtos/src/link/backend/rawio/uart_myrtos.c")
   set_property(TARGET zenohpico::serial_backend_uart_myrtos PROPERTY
                ZP_BACKEND_SYMBOL _z_uart_myrtos_rawio_ops)
   zp_register_serial_backend(
     NAME uart_myrtos
     TARGET zenohpico::serial_backend_uart_myrtos
     SYMBOL _z_uart_myrtos_rawio_ops
   )

   zp_add_platform_dir("${CMAKE_CURRENT_LIST_DIR}/platforms")

Then configure Zenoh-Pico with:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_PACKAGES=zenohpico-myrtos \
     -DCMAKE_PREFIX_PATH=/path/to/prefix \
     -DZP_PLATFORM=myrtos

Adding a backend
================

Backend descriptors follow the same pattern as platform descriptors:

* built in: add a file under the lower-backend directories
  ``cmake/backends/stream/``, ``cmake/backends/datagram/``, or
  ``cmake/backends/rawio/``;
* external package: define or import the corresponding backend targets from a
  CMake package, then register them through the normal ``zp_register_*``
  macros.

The backend name is the descriptor file name without the ``.cmake`` suffix. For example,
``cmake/backends/rawio/uart_myrtos.cmake`` defines the ``uart_myrtos`` serial transport backend.

Example built-in backend descriptor:

.. code-block:: cmake

   # cmake/backends/rawio/uart_myrtos.cmake
   zp_register_serial_backend(
     NAME uart_myrtos
     SYMBOL _z_uart_myrtos_rawio_ops
     SOURCES "${PROJECT_SOURCE_DIR}/src/link/backend/rawio/uart_myrtos.c"
     # Optional:
     # COMPATIBLE_SYSTEM_LAYERS myrtos
   )

UDP also has a canonical broader backend registry above the lower registries.
Built-in UDP backend descriptors currently live under ``cmake/backends/udp/``
and map one broader UDP selection to the current unicast/multicast lower
pieces. Out-of-tree integrations can mirror the same model by defining a
broader ``zenohpico::udp_backend_<name>`` target plus any lower UDP targets it
references, then registering those targets.

.. code-block:: cmake

   add_library(zenohpico::udp_unicast_backend_udp_myrtos INTERFACE IMPORTED GLOBAL)
   set_property(TARGET zenohpico::udp_unicast_backend_udp_myrtos PROPERTY
                INTERFACE_SOURCES "/path/to/myrtos/src/link/backend/datagram/udp_myrtos.c")
   set_property(TARGET zenohpico::udp_unicast_backend_udp_myrtos PROPERTY
                ZP_BACKEND_SYMBOL _z_udp_myrtos_datagram_ops)
   zp_register_udp_unicast_backend(
     NAME udp_myrtos
     TARGET zenohpico::udp_unicast_backend_udp_myrtos
     SYMBOL _z_udp_myrtos_datagram_ops
   )

   add_library(zenohpico::udp_multicast_backend_udp_multicast_myrtos INTERFACE IMPORTED GLOBAL)
   set_property(TARGET zenohpico::udp_multicast_backend_udp_multicast_myrtos PROPERTY
                INTERFACE_SOURCES "/path/to/myrtos/src/link/backend/datagram/udp_multicast_myrtos.c")
   set_property(TARGET zenohpico::udp_multicast_backend_udp_multicast_myrtos PROPERTY
                ZP_BACKEND_SYMBOL _z_udp_multicast_myrtos_ops)
   zp_register_udp_multicast_backend(
     NAME udp_multicast_myrtos
     TARGET zenohpico::udp_multicast_backend_udp_multicast_myrtos
     SYMBOL _z_udp_multicast_myrtos_ops
   )

   add_library(zenohpico::udp_backend_myrtos INTERFACE IMPORTED GLOBAL)
   set_property(TARGET zenohpico::udp_backend_myrtos PROPERTY
                ZP_UDP_BACKEND_UNICAST_BACKEND udp_myrtos)
   set_property(TARGET zenohpico::udp_backend_myrtos PROPERTY
                ZP_UDP_BACKEND_MULTICAST_BACKEND udp_multicast_myrtos)
   zp_register_udp_backend(
     NAME myrtos
     TARGET zenohpico::udp_backend_myrtos
     UNICAST_BACKEND udp_myrtos
     MULTICAST_BACKEND udp_multicast_myrtos
   )

The lower-backend families ``stream`` and ``datagram`` can also declare
``COMPATIBLE_NETWORKS``
when they should only be selected with specific network implementations.
They can also declare ``SOCKET_COMPONENT`` when they require a specific
socket/network support component, or ``COMPATIBLE_SOCKET_COMPONENTS`` when the
same backend can work with several compatible socket support implementations.

The UDP multicast backend uses the same backend-style compatibility metadata
(``COMPATIBLE_NETWORKS``, ``SOCKET_COMPONENT``,
``COMPATIBLE_SOCKET_COMPONENTS``, ``COMPATIBLE_SYSTEM_LAYERS``), but it
remains an internal UDP piece rather than a separate public transport
backend family.

``COMPATIBLE_SOCKET_COMPONENTS`` is intended for backends that are genuinely
portable across several network descriptors sharing the same socket support.
Current built-in network-backed backends now primarily validate through
``SOCKET_COMPONENT`` plus ``COMPATIBLE_SYSTEM_LAYERS`` where a family is
shared but some variants are still platform-specific. ``COMPATIBLE_NETWORKS``
remains available for cases that really depend on one exact network
implementation, but it is no longer the main compatibility mechanism for the
built-in network-backed families.

The backend can then be selected either from a package-provided platform descriptor:

.. code-block:: cmake

   zp_platform_set_serial_backend(uart_myrtos)

or directly at configure time after the package is loaded:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_PACKAGES=zenohpico-myrtos \
     -DCMAKE_PREFIX_PATH=/path/to/prefix \
     -DZP_PLATFORM=linux \
     -DZP_SERIAL_BACKEND=uart_myrtos

Adding a network layer
======================

Network descriptors follow the same pattern as platform and backend descriptors:

* built in: add a file under ``cmake/network/``;
* external package: export a ``zenohpico::network_<name>`` target.

The network name is the descriptor file name without the ``.cmake`` suffix. For example,
``cmake/network/myrtos.cmake`` defines the ``myrtos`` network descriptor.

Example built-in network descriptor:

.. code-block:: cmake

   # cmake/network/myrtos.cmake
   zp_register_network(
     NAME myrtos
     SOCKET_OPS_SYMBOL _z_myrtos_socket_ops
     SOCKET_COMPONENT lwip
     SOURCES "${PROJECT_SOURCE_DIR}/src/system/myrtos/network.c"
     # LINK_LIBRARIES myrtos-netlib
   )

``SOCKET_COMPONENT`` identifies the socket/network support component expected by
the selected backends. A network descriptor may simply point to an existing
component, or it may also provide its own ``SOCKET_OPS_SYMBOL`` and sources
when it needs a network-specific overlay on top of that socket support. If
``SOCKET_COMPONENT`` is omitted, the network descriptor is treated as
self-contained. In the current built-in tree, network descriptors are mostly
thin selectors over socket components.

Adding a socket component
=========================

Socket-component descriptors follow the same pattern as network and backend
descriptors:

* built in: add a file under ``cmake/socket/``;
* external package: export a ``zenohpico::socket_<name>`` target.

The socket-component name is the descriptor file name without the ``.cmake``
suffix. For example, ``cmake/socket/lwip.cmake`` defines the
``lwip`` socket component.

Example built-in socket-component descriptor:

.. code-block:: cmake

   # cmake/socket/lwip.cmake
   zp_register_socket_component(
     NAME lwip
     SOCKET_OPS_SYMBOL _z_lwip_socket_ops
     SOURCES "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c"
   )

Explicit system-layer selection
===============================

``ZP_SYSTEM_LAYER`` is an optional build setting. Most builds should set ``ZP_PLATFORM`` and leave
``ZP_SYSTEM_LAYER`` unset.

It is mainly intended for explicit builds and integration scenarios that need to choose the system-layer
implementation directly.

This is useful when an external integration wants to expose a stable system-layer name without requiring users to know
the platform descriptor name.

If the system-layer name and the descriptor file name differ, register the mapping with
``zp_register_system_layer_profile()`` from the package config that also
registers the package-provided platform descriptor directory.

Example:

.. code-block:: cmake

   # /path/to/myrtos/cmake/platforms/myboard.cmake
   zp_platform_set_system_layer(myrtos)
   zp_platform_set_network(myrtos)
   zp_platform_set_tcp_backend(tcp_lwip)
   zp_platform_set_udp_backend(lwip)
   zp_platform_set_serial_backend(uart_myrtos)

.. code-block:: cmake

   # /path/to/prefix/lib/cmake/zenohpico-myrtos/zenohpico-myrtosConfig.cmake
   zp_add_platform_dir("${CMAKE_CURRENT_LIST_DIR}/platforms")
   zp_register_system_layer_profile(SYSTEM_LAYER myrtos PROFILE myboard)

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_PACKAGES=zenohpico-myrtos \
     -DCMAKE_PREFIX_PATH=/path/to/prefix \
     -DZP_SYSTEM_LAYER=myrtos

In this example the descriptor file is named ``myboard.cmake``, but the external integration wants users to select the
system layer as ``myrtos``. ``ZP_SYSTEM_LAYER`` is what makes that possible. If the profile name and the system-layer
name are the same, no extra registration is needed.

In that case, the external integration should provide:

* a platform descriptor under its own ``cmake/platforms/`` directory, registered from the package config;
* a package-defined or imported ``zenohpico::system_<name>`` target;
* a package-defined or imported ``zenohpico::network_<name>`` target when the socket/network side is not
  already covered by an existing built-in or external network implementation;
* optional package-defined or imported backend targets only when the built-in ``tcp`` / ``udp`` / ``serial``
  pieces cannot be reused for that system layer.
