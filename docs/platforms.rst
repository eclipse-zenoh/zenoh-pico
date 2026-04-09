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

Zenoh-Pico builds are selected primarily by ``ZP_PLATFORM``. A platform profile
chooses the system layer, the network layer, and the default ``tcp``, ``udp``
and ``serial`` transport backends.

Platform profiles can reuse the built-in ``network``, ``socket``, ``tcp`` and
``udp`` components and add platform-specific parts such as a custom system
layer, serial backend, or transport backend.

Platform selection
==================

If neither ``ZP_PLATFORM`` nor ``ZP_SYSTEM_LAYER`` is set, Zenoh-Pico selects a
default host profile from the build environment.

For example, the built-in ``linux`` profile can be selected with:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_PLATFORM=linux

Individual component overrides are also available. For the built-in ``linux``
profile, the following longer form is equivalent because it simply repeats the
profile defaults explicitly:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_PLATFORM=linux \
     -DZP_NETWORK=posix \
     -DZP_TCP_BACKEND=tcp_posix \
     -DZP_UDP_BACKEND=posix \
     -DZP_SERIAL_BACKEND=tty_posix

``ZP_UDP_BACKEND`` selects the UDP transport backend. The selected backend may
provide both UDP unicast and UDP multicast support. The multicast part is
optional.

Adding a built-in platform
==========================

To add a built-in platform profile, add ``cmake/platforms/<name>.cmake``.

The profile contains:

* select the system layer;
* select the network layer;
* select the default ``tcp``, ``udp`` and ``serial`` transport backends;
* optional extra platform-only sources, include directories, or compile
  definitions.

Example:

.. code-block:: cmake

   # cmake/platforms/myrtos.cmake
   zp_platform_set_system_layer(myrtos)
   zp_platform_set_network(freertos_lwip)
   zp_platform_set_tcp_backend(tcp_lwip)
   zp_platform_set_udp_backend(lwip)
   zp_platform_set_serial_backend(uart_myrtos)

   # Optional platform-specific additions
   # zp_platform_add_definition(ZENOH_MYRTOS_BOARD)
   # zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/system/myrtos/platform_extra.c")
   # zp_platform_add_include_dirs("${PROJECT_SOURCE_DIR}/src/system/myrtos/include")

If the platform also needs a new system layer, add ``cmake/system/<name>.cmake``:

.. code-block:: cmake

   # cmake/system/myrtos.cmake
   set(ZP_SYSTEM_LAYER_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/system/myrtos/system.c")
   set(ZP_SYSTEM_LAYER_COMPILE_DEFINITIONS
       ZENOH_MYRTOS)

``ZP_SYSTEM_LAYER_COMPILE_DEFINITIONS`` declares the platform markers associated
with the selected system layer. For built-in layers these definitions are used
to compile the system-layer sources and are also needed when building code that
includes Zenoh-Pico headers.

If it needs a new built-in transport backend, add a descriptor in the
appropriate backend directory:

* ``cmake/backends/tcp/`` for TCP backends;
* ``cmake/backends/udp/`` for UDP backends;
* ``cmake/backends/serial/`` for ``serial`` backends.

Transport backends provide the entry points declared in the matching backend
headers. Backend descriptors select the source files or imported target that
provides those entry points.

* TCP backends define the functions from
  ``include/zenoh-pico/link/backend/tcp.h``.
* UDP backends define the functions from
  ``include/zenoh-pico/link/backend/udp_unicast.h`` and, when multicast support
  is present, ``include/zenoh-pico/link/backend/udp_multicast.h``.
* serial backends define the functions from
  ``include/zenoh-pico/link/backend/serial.h``.

Example built-in serial backend:

.. code-block:: cmake

   # cmake/backends/serial/uart_myrtos.cmake
   set(ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/serial/uart_myrtos.c")
   set(ZP_BACKEND_COMPATIBLE_SYSTEM_LAYERS
       "myrtos")

Example built-in UDP backend:

.. code-block:: cmake

   # cmake/backends/udp/myrtos.cmake
   set(ZP_BACKEND_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_myrtos.c"
       "${PROJECT_SOURCE_DIR}/src/link/backend/udp/udp_multicast_myrtos.c")
   set(ZP_BACKEND_SOCKET_COMPONENT
       "lwip")

Then configure Zenoh-Pico with:

.. code-block:: bash

   cmake -S . -B build -DZP_PLATFORM=myrtos

Out-of-Tree Packages
====================

Out-of-tree integrations are provided as CMake packages.
For each package name listed in ``ZP_EXTERNAL_PACKAGES``, Zenoh-Pico calls
``find_package(<name> CONFIG REQUIRED)`` during configuration.

For example:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_PACKAGES=zenohpico-myrtos \
     -DCMAKE_PREFIX_PATH=/path/to/prefix \
     -DZP_PLATFORM=myrtos

An external package may provide:

* a platform profile, if it wants to expose a new ``ZP_PLATFORM`` value;
* a custom system-layer library;
* transport-backend libraries for ``serial``, ``tcp`` or ``udp``.

Out-of-tree platforms can reuse Zenoh-Pico's built-in ``network`` and
``socket`` layers.

See ``examples/packages/zenohpico-mylinux`` for a package that exports an
external system layer and serial backend.

External Libraries and Descriptors
----------------------------------

Providing an external component requires:

* a library target exported by the package's own build;
* a Zenoh-Pico descriptor file that points to that target and provides the
  Zenoh-Pico-specific metadata.

The library target may use any package-native imported name such as
``myrtos::tcp`` or ``myrtos::serial_uart``. The descriptor passes that target
name to Zenoh-Pico through ``ZP_BACKEND_IMPORTED_TARGET``.

For external backend descriptors, ``ZP_BACKEND_IMPORTED_TARGET`` is required.
Built-in backend descriptors use ``ZP_BACKEND_SOURCE_FILES`` and related build
variables instead. A backend descriptor uses one form or the other, not both.

The library defines the entry points for the backend type:

* TCP backends define the functions declared in
  ``include/zenoh-pico/link/backend/tcp.h``.
* UDP backends define the functions declared in
  ``include/zenoh-pico/link/backend/udp_unicast.h`` and, when UDP multicast
  support is enabled in the build, in
  ``include/zenoh-pico/link/backend/udp_multicast.h``.
* serial backends define the functions declared in
  ``include/zenoh-pico/link/backend/serial.h``.

Example TCP backend descriptor:

.. code-block:: cmake

   # <prefix>/lib/cmake/zenohpico-myrtos/backends/tcp/tcp_myrtos.cmake
   set(ZP_BACKEND_IMPORTED_TARGET "myrtos::tcp")
   set(ZP_BACKEND_SOCKET_COMPONENT "lwip")

An external UDP backend library defines the ``_z_udp_unicast_*`` functions and,
when UDP multicast support is enabled, the ``_z_udp_multicast_*`` functions
declared in the corresponding headers.

Example UDP backend descriptor:

.. code-block:: cmake

   # <prefix>/lib/cmake/zenohpico-myrtos/backends/udp/myrtos.cmake
   set(ZP_BACKEND_IMPORTED_TARGET "myrtos::udp")
   set(ZP_BACKEND_SOCKET_COMPONENT "lwip")

Example serial descriptor:

.. code-block:: cmake

   # <prefix>/lib/cmake/zenohpico-myrtos/backends/serial/uart_myrtos.cmake
   set(ZP_BACKEND_IMPORTED_TARGET "myrtos::serial_uart")

Example system-layer descriptor:

.. code-block:: cmake

   # <prefix>/lib/cmake/zenohpico-myrtos/system/myrtos.cmake
   set(ZP_SYSTEM_LAYER_IMPORTED_TARGET "myrtos::system")
   set(ZP_SYSTEM_LAYER_COMPILE_DEFINITIONS ZENOH_MYRTOS)

External backend libraries that include Zenoh-Pico headers use the same
system-layer definitions in their own build.
``include/zenoh-pico/system/common/platform.h`` uses these definitions to
select the platform branch and define socket-family macros such as
``ZP_PLATFORM_SOCKET_LWIP``.

``ZP_BACKEND_SOCKET_COMPONENT`` matches the built-in network selected by
the platform profile. For example, the built-in ``posix`` network requires the
``posix`` socket component, while ``freertos_lwip`` requires ``lwip``.

When an external component has its own sources, dependencies, or code
generation, build it in the package project and install/export its target
there. The package config file then loads that exported target.
Do not call ``add_library()`` directly in the package config file.

Example:

.. code-block:: cmake

   # In the package project's CMakeLists.txt
   add_library(myrtos_udp STATIC ...)
   install(TARGETS myrtos_udp EXPORT zenohpicoMyrtosTargets ...)
   install(EXPORT zenohpicoMyrtosTargets
           NAMESPACE myrtos::
           FILE zenohpicoMyrtosTargets.cmake
           DESTINATION lib/cmake/zenohpico-myrtos)

   # In zenohpico-myrtosConfig.cmake
   include("${CMAKE_CURRENT_LIST_DIR}/zenohpicoMyrtosTargets.cmake")

Package Layout and Config
-------------------------

Example package layout:

.. code-block:: text

   <prefix>/lib/cmake/zenohpico-myrtos/
     zenohpico-myrtosConfig.cmake
     platforms/
       myrtos.cmake
     system/
       myrtos.cmake
     backends/tcp/
       tcp_myrtos.cmake
     backends/udp/
       myrtos.cmake
     backends/serial/
       uart_myrtos.cmake

A package contains only the descriptor directories and targets that it
provides. For example, a package that exports only a system layer and a serial
backend does not need ``backends/tcp/`` or ``backends/udp/``.

The package config:

* registers its platform descriptors with ``zp_add_platform_dir(...)``;
* appends any package-provided system-layer descriptor directories to
  ``ZP_SYSTEM_LAYER_DIRS``;
* appends any package-provided backend descriptor directories to the matching
  search lists, for example ``ZP_TCP_BACKEND_DIRS`` or
  ``ZP_UDP_BACKEND_DIRS`` or ``ZP_SERIAL_BACKEND_DIRS``;
* includes the package's exported targets file so that the descriptors can point
  to real imported targets.

Example package config:

.. code-block:: cmake

   # <prefix>/lib/cmake/zenohpico-myrtos/zenohpico-myrtosConfig.cmake
   include("${CMAKE_CURRENT_LIST_DIR}/zenohpicoMyrtosTargets.cmake")

   if(COMMAND zp_add_platform_dir)
     zp_add_platform_dir("${CMAKE_CURRENT_LIST_DIR}/platforms")
   endif()

   list(APPEND ZP_SYSTEM_LAYER_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/system")
   list(APPEND ZP_TCP_BACKEND_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/backends/tcp")
   list(APPEND ZP_UDP_BACKEND_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/backends/udp")
   list(APPEND ZP_SERIAL_BACKEND_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/backends/serial")

Package Example
---------------

For a full package example see
``examples/packages/zenohpico-mylinux/README.md``.

It shows:

* a package project that exports ``mylinux::system`` and
  ``mylinux::serial_uart``;
* descriptor files in ``cmake/platforms/``, ``cmake/system/`` and
  ``cmake/backends/serial/``;
* a Zenoh-Pico build that loads the package through
  ``-DZP_EXTERNAL_PACKAGES=zenohpico-mylinux``;
* a downstream consumer that uses the installed ``zenohpico::static`` target.

Explicit System-Layer Selection
===============================

``ZP_SYSTEM_LAYER`` is an optional override. A build can set ``ZP_PLATFORM``
and leave ``ZP_SYSTEM_LAYER`` unset.

It can be used when an external integration wants users to select a stable
system-layer name even if the platform profile has a different file name. In
that case, the package can register the mapping from its package config:

.. code-block:: cmake

   zp_register_system_layer_profile(SYSTEM_LAYER myrtos PROFILE myboard)

Then users can configure:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_PACKAGES=zenohpico-myrtos \
     -DCMAKE_PREFIX_PATH=/path/to/prefix \
     -DZP_SYSTEM_LAYER=myrtos

This mapping is only needed when the platform profile name and the system-layer
name are different. When they are the same, no extra configuration is required.
