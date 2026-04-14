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

*********
Platforms
*********

``ZP_PLATFORM`` selects a platform profile.
The profile is one CMake file.
Built-in profiles are defined in ``cmake/platforms/<name>.cmake``.

Platform Selection
==================

If ``ZP_PLATFORM`` is not set, Zenoh-Pico selects a default host profile.

Example:

.. code-block:: bash

   cmake -S . -B build \
     -DZP_PLATFORM=linux

One-File Platform Descriptors
=============================

A platform profile is a CMake file.
The template file is ``examples/platforms/myplatform.cmake``.

A minimal profile defines:

* ``ZP_PLATFORM_SOURCE_FILES``;
* optional ``ZP_PLATFORM_COMPILE_DEFINITIONS``,
  ``ZP_PLATFORM_INCLUDE_DIRS``, ``ZP_PLATFORM_COMPILE_OPTIONS``, and
  ``ZP_PLATFORM_LINK_LIBRARIES``;
* optional ``ZP_PLATFORM_SYSTEM_LAYER`` when the logical system-layer name
  differs from the profile name;
* optional ``ZP_PLATFORM_SYSTEM_PLATFORM_HEADER`` when
  ``include/zenoh-pico/system/common/platform.h`` should include a custom
  header instead of a built-in one.

Put all platform-specific sources into ``ZP_PLATFORM_SOURCE_FILES``:

* system-layer sources;
* network/socket sources;
* TCP, UDP, BT, and serial transport sources;
* any extra platform files.

Example platform profile:

.. code-block:: cmake

   # cmake/platforms/myrtos.cmake
   set(ZP_PLATFORM_COMPILE_DEFINITIONS
       ZENOH_MYRTOS)

   set(ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/system/myrtos/system.c"
       "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/transport/serial/uart_myrtos.c")

   if(ZP_UDP_MULTICAST_ENABLED)
     list(APPEND ZP_PLATFORM_SOURCE_FILES
          "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip.c"
          "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip_common.c")
   endif()

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

* ``ZP_PLATFORM_SOURCE_FILES`` is the only source-file bucket.
* The build does not classify files by role. Put a file into
  ``ZP_PLATFORM_SOURCE_FILES`` if it belongs to the selected platform.
* Set ``ZP_PLATFORM_SYSTEM_LAYER`` only when the logical system-layer name
  differs from the profile name. For example, ``opencr`` uses
  ``arduino_opencr``.
* Use ``ZP_UDP_MULTICAST_ENABLED`` when multicast sources depend on the current
  build configuration.

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
* registers the platform descriptor directory it provides.

Installed package platform descriptors should use installed library targets and
installed include paths instead of package source paths.

Example platform descriptor from an external package:

.. code-block:: cmake

   # <prefix>/lib/cmake/zenohpico-myrtos/platforms/myrtos.cmake
   set(ZP_PLATFORM_COMPILE_DEFINITIONS ZENOH_MYRTOS)
   set(ZP_PLATFORM_SYSTEM_PLATFORM_HEADER "zenoh_myrtos_platform.h")

   set(ZP_PLATFORM_SOURCE_FILES
       "${PROJECT_SOURCE_DIR}/src/system/socket/lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/transport/tcp/tcp_lwip.c"
       "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_lwip.c")

   if(ZP_UDP_MULTICAST_ENABLED)
     list(APPEND ZP_PLATFORM_SOURCE_FILES
          "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip.c"
          "${PROJECT_SOURCE_DIR}/src/link/transport/udp/udp_multicast_lwip_common.c")
   endif()

   list(APPEND ZP_PLATFORM_INCLUDE_DIRS
        "${CMAKE_CURRENT_LIST_DIR}/../../../../include")

   list(APPEND ZP_PLATFORM_LINK_LIBRARIES
        myrtos::system
        myrtos::serial_uart)

Example package config:

.. code-block:: cmake

   include("${CMAKE_CURRENT_LIST_DIR}/zenohpicoMyrtosTargets.cmake")

   if(COMMAND zp_add_platform_dir)
     zp_add_platform_dir("${CMAKE_CURRENT_LIST_DIR}/platforms")
   endif()

Build library targets in the package's own project, install/export them there,
and load the exported targets from the package's config (``*Config.cmake``)
file. Do not call ``add_library()`` or ``add_executable()`` directly in the
package's config file.

For a full out-of-tree example, see
``examples/packages/zenohpico-mylinux/README.md``.
