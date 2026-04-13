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
Platforms and Transports
**********************

``ZP_PLATFORM`` selects a platform profile.
The profile defines:

* the system layer;
* the network layer;
* the ``tcp``, ``udp``, ``bt`` and ``serial`` transport implementations.

Built-in profiles are defined in ``cmake/platforms/<name>.cmake``.

Platform Selection
==================

If ``ZP_PLATFORM`` is not set, Zenoh-Pico selects a default host profile.

Example:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_PLATFORM=linux

Explicit component overrides:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_PLATFORM=linux \
     -DZP_NETWORK=posix \
     -DZP_TCP_TRANSPORT=tcp_posix \
     -DZP_UDP_TRANSPORT=posix \
     -DZP_SERIAL_TRANSPORT=tty_posix

``ZP_NETWORK`` and ``ZP_*_TRANSPORT`` override the inline component definitions
from the selected platform profile.

One-File Platform Descriptors
=============================

A platform profile is a CMake file.
The template file is ``examples/platforms/myplatform.cmake``.

A minimal profile defines:

* either ``ZP_PLATFORM_SYSTEM_SOURCE_FILES`` or
  ``ZP_PLATFORM_SYSTEM_IMPORTED_TARGET``;
* either ``ZP_PLATFORM_NETWORK_SOURCE_FILES`` or
  ``ZP_PLATFORM_NETWORK_IMPORTED_TARGET``;
* for each enabled transport, either
  ``ZP_PLATFORM_<transport>_SOURCE_FILES`` or
  ``ZP_PLATFORM_<transport>_IMPORTED_TARGET``;
* optional platform-wide variables such as
  ``ZP_PLATFORM_SOURCE_FILES``, ``ZP_PLATFORM_INCLUDE_DIRS``,
  ``ZP_PLATFORM_COMPILE_DEFINITIONS``, ``ZP_PLATFORM_COMPILE_OPTIONS`` and
  ``ZP_PLATFORM_LINK_LIBRARIES``.

Use ``*_SOURCE_FILES`` for sources built inside Zenoh-Pico.
Use ``*_IMPORTED_TARGET`` for code provided by an external package.
Do not define both forms for the same component.

Example platform profile:

.. code-block:: cmake

   # cmake/platforms/myrtos.cmake
   set(ZP_PLATFORM_SYSTEM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/system/myrtos/system.c")
   set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS
       ZENOH_MYRTOS)

   set(ZP_PLATFORM_NETWORK_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c")

   set(ZP_PLATFORM_TCP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_lwip.c")

   set(ZP_PLATFORM_UDP_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_lwip.c")
   if(ZP_UDP_MULTICAST_ENABLED)
     list(APPEND ZP_PLATFORM_UDP_SOURCE_FILES
          "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip.c"
          "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip_common.c")
   endif()

   set(ZP_PLATFORM_SERIAL_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_myrtos.c")

   # Optional platform-wide additions
   # list(APPEND ZP_PLATFORM_SOURCE_FILES
   #      "${PROJECT_SOURCE_DIR}/src/system/myrtos/platform_extra.c")
   # list(APPEND ZP_PLATFORM_INCLUDE_DIRS
   #      "${PROJECT_SOURCE_DIR}/src/system/myrtos/include")
   # list(APPEND ZP_PLATFORM_COMPILE_DEFINITIONS
   #      ZENOH_MYRTOS_BOARD)
   # list(APPEND ZP_PLATFORM_COMPILE_OPTIONS
   #      -Wall)
   # list(APPEND ZP_PLATFORM_LINK_LIBRARIES
   #      myrtos::sdk)

Notes:

* If ``ZP_PLATFORM_SYSTEM_LAYER`` is not set, the system-layer name defaults to
  the platform profile name.
* Set ``ZP_PLATFORM_SYSTEM_LAYER`` only when the logical system-layer name
  differs from the profile name. For example, ``opencr`` uses
  ``arduino_opencr``.
* Network source files are provided inline via ``ZP_PLATFORM_NETWORK_SOURCE_FILES``.
  Alternatively, pass ``-DZP_NETWORK=...`` to select a named network descriptor
  from ``cmake/network/``.
* Use ``ZP_UDP_MULTICAST_ENABLED`` when multicast sources depend on the current
  build configuration.

Optional Component Metadata
---------------------------

The same optional metadata is available for the system layer, network layer,
and each transport component in a platform profile:

* ``*_INCLUDE_DIRS``
* ``*_COMPILE_DEFINITIONS``

Use ``ZP_PLATFORM_COMPILE_OPTIONS`` and ``ZP_PLATFORM_LINK_LIBRARIES`` for
platform-wide compiler and linker settings.

Custom Platform Headers
-----------------------

Set ``ZP_PLATFORM_SYSTEM_PLATFORM_HEADER`` only when the system layer needs a
custom platform header.

Example:

.. code-block:: cmake

   set(ZP_PLATFORM_SYSTEM_IMPORTED_TARGET myrtos::system)
   set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_MYRTOS)
   set(ZP_PLATFORM_SYSTEM_PLATFORM_HEADER "zenoh_myrtos_platform.h")

The header provides the platform-specific types used by
``include/zenoh-pico/system/common/platform.h``.

Reusable Named Components
=========================

Use inline component definitions when the code belongs to one platform profile.

Add named descriptors only when a component:

* is reused by more than one platform;
* must be selected explicitly with ``-DZP_NETWORK=...`` or
  ``-DZP_*_TRANSPORT=...``.

Directories:

* ``cmake/network/`` for named network descriptors;
* ``cmake/transports/tcp/`` for named TCP transports;
* ``cmake/transports/udp/`` for named UDP transports;
* ``cmake/transports/bt/`` for named BT transports;
* ``cmake/transports/serial/`` for named serial transports.

Named network descriptors use ``ZP_NETWORK_*`` variables.
Named transport descriptors use ``ZP_TRANSPORT_*`` variables.

Example named UDP transport:

.. code-block:: cmake

   # cmake/transports/udp/myrtos.cmake
   set(ZP_TRANSPORT_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_myrtos.c")
   if(ZP_UDP_MULTICAST_ENABLED)
     list(APPEND ZP_TRANSPORT_SOURCE_FILES
          "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_myrtos.c")
   endif()

Out-of-Tree Packages
====================

Out-of-tree integrations are provided as CMake packages.
For each name listed in ``ZP_EXTERNAL_PACKAGES``, Zenoh-Pico runs
``find_package(<name> CONFIG REQUIRED)``.

Example:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_EXTERNAL_PACKAGES=zenohpico-myrtos \
     -DCMAKE_PREFIX_PATH=/path/to/prefix \
     -DZP_PLATFORM=myrtos

A package usually:

* exports the library targets it owns;
* registers any platform, network, or transport descriptor directories it
  provides.

Installed package descriptors should use imported targets instead of package
source paths.

Example platform descriptor from an external package:

.. code-block:: cmake

   # <prefix>/lib/cmake/zenohpico-myrtos/platforms/myrtos.cmake
   set(ZP_PLATFORM_SYSTEM_IMPORTED_TARGET myrtos::system)
   set(ZP_PLATFORM_SYSTEM_COMPILE_DEFINITIONS ZENOH_MYRTOS)
   set(ZP_PLATFORM_SYSTEM_PLATFORM_HEADER "zenoh_myrtos_platform.h")

   set(ZP_PLATFORM_NETWORK_IMPORTED_TARGET myrtos::network)

   set(ZP_PLATFORM_TCP_IMPORTED_TARGET myrtos::tcp)
   set(ZP_PLATFORM_UDP_IMPORTED_TARGET myrtos::udp)
   set(ZP_PLATFORM_SERIAL_IMPORTED_TARGET myrtos::serial_uart)

Example package config:

.. code-block:: cmake

   include("${CMAKE_CURRENT_LIST_DIR}/zenohpicoMyrtosTargets.cmake")

   if(COMMAND zp_add_platform_dir)
     zp_add_platform_dir("${CMAKE_CURRENT_LIST_DIR}/platforms")
   endif()

   # Optional: only when the package exports reusable named components
   list(APPEND ZP_NETWORK_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/network")
   list(APPEND ZP_TCP_TRANSPORT_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/transports/tcp")

For a complete out-of-tree example, see
``examples/packages/zenohpico-mylinux/README.md``.
