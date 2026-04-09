# zenohpico-mylinux

This directory contains a runnable out-of-tree package example for Zenoh-Pico.
It exports:

- `mylinux::system`
- `mylinux::serial_uart`

and provides descriptor files for:

- `platforms/mylinux.cmake`
- `system/mylinux.cmake`
- `backends/serial/uart_mylinux.cmake`

The example reuses Zenoh-Pico's built-in `network`, `tcp`, and `udp` pieces and
adds only an external system layer and serial backend. The system layer uses a
custom platform marker, `ZENOH_MYLINUX`, and provides the corresponding
platform header, `zenoh_mylinux_platform.h`.

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
    system/
      mylinux.cmake
    backends/serial/
      uart_mylinux.cmake
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
  registers the package descriptor directories.
- `cmake/platforms/mylinux.cmake` defines the `mylinux` platform profile.
- `cmake/system/mylinux.cmake` maps the `mylinux` system layer to
  `mylinux::system` and declares the `ZENOH_MYLINUX` platform marker plus the
  custom platform header.
- `cmake/backends/serial/uart_mylinux.cmake` maps the `uart_mylinux` serial
  backend to `mylinux::serial_uart`.
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
