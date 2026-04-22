# zenohpico-mylinux

This directory contains a full out-of-tree package example for Zenoh-Pico.
It exports:

- `mylinux::system`
- `mylinux::serial_uart`

and provides:

- `platforms/mylinux.cmake`

The example defines one `mylinux` platform profile. The profile sets one
`ZP_PLATFORM_SOURCE_FILES` list with the reused POSIX network/TCP/UDP sources,
adds the package include directory for `zenoh_mylinux_platform.h`, and links
the exported `mylinux::system` and `mylinux::serial_uart` targets through
`ZP_PLATFORM_LINK_LIBRARIES`.

## Package layout

```text
examples/packages/zenohpico-mylinux/
  CMakeLists.txt
  include/
    zenoh_mylinux_platform.h
  cmake/
    zenohpico-mylinuxConfig.cmake
    platforms/
      mylinux.cmake
  consumer/
    CMakeLists.txt
    main.c
```

## What each file does

- `CMakeLists.txt` builds and installs the exported package targets
  `mylinux::system` and `mylinux::serial_uart`, and installs the custom
  platform header.
- `include/zenoh_mylinux_platform.h` defines the platform-specific types used
  when `ZENOH_MYLINUX` is selected.
- `cmake/zenohpico-mylinuxConfig.cmake` loads the exported targets file and
  registers the package platform descriptor directory.
- `cmake/platforms/mylinux.cmake` defines the `mylinux` platform profile with
  `ZP_PLATFORM_*` variables.
- `consumer/` is a minimal downstream project that uses an installed
  `zenohpico::static`.

## Build and install the package

From the Zenoh-Pico source tree root:

```bash
cmake -S examples/packages/zenohpico-mylinux -B /tmp/zenohpico-mylinux-pkg \
  -DZENOH_PICO_SOURCE_DIR="$PWD"
cmake --build /tmp/zenohpico-mylinux-pkg -j
cmake --install /tmp/zenohpico-mylinux-pkg --prefix /tmp/zenohpico-mylinux-prefix
```

## Build Zenoh-Pico with the package

```bash
cmake -S . -B /tmp/zenohpico-mylinux-build \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF \
  -DZP_EXTERNAL_PACKAGES=zenohpico-mylinux \
  -DCMAKE_PREFIX_PATH=/tmp/zenohpico-mylinux-prefix \
  -DZP_PLATFORM=mylinux
cmake --build /tmp/zenohpico-mylinux-build -j --target zenohpico_static
cmake --install /tmp/zenohpico-mylinux-build --prefix /tmp/zenohpico-install
```

## Build a downstream consumer

After installing Zenoh-Pico itself, the `consumer/` subdirectory can be used as
a minimal downstream project:

```bash
cmake -S examples/packages/zenohpico-mylinux/consumer -B /tmp/zenohpico-mylinux-consumer \
  -DCMAKE_PREFIX_PATH="/tmp/zenohpico-install;/tmp/zenohpico-mylinux-prefix"
cmake --build /tmp/zenohpico-mylinux-consumer -j
```
