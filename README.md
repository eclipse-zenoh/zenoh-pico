<img src="https://raw.githubusercontent.com/eclipse-zenoh/zenoh/master/zenoh-dragon.png" height="150">

![Build](https://github.com/eclipse-zenoh/zenoh-pico/workflows/build/badge.svg)
![integration](https://github.com/eclipse-zenoh/zenoh-pico/workflows/integration/badge.svg)
[![Documentation Status](https://readthedocs.org/projects/zenoh-pico/badge/?version=latest)](https://zenoh-pico.readthedocs.io/en/latest/?badge=latest)
[![Discussion](https://img.shields.io/badge/discussion-on%20github-blue)](https://github.com/eclipse-zenoh/roadmap/discussions)
[![Discord](https://img.shields.io/badge/chat-on%20discord-blue)](https://discord.gg/2GJ958VuHs)
[![License](https://img.shields.io/badge/License-EPL%202.0-blue)](https://choosealicense.com/licenses/epl-2.0/)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# Eclipse Zenoh

The Eclipse Zenoh: Zero Overhead Pub/sub, Store/Query and Compute.

Zenoh (pronounce _/zeno/_) unifies data in motion, data at rest, and computations. It carefully blends traditional pub/sub with geo-distributed storages, queries and computations, while retaining a level of time and space efficiency that is well beyond any of the mainstream stacks.

Check the website [zenoh.io](http://zenoh.io) and the [roadmap](https://github.com/eclipse-zenoh/roadmap) for more detailed information.

-------------------------------

# Zenoh-Pico: native C library for constrained devices

zenoh-pico is the [Eclipse zenoh](http://zenoh.io) implementation that targets constrained devices, offering a native C API.
It is fully compatible with its main [Rust Zenoh implementation](https://github.com/eclipse-zenoh/zenoh), providing a lightweight implementation of most functionalities.

Currently, zenoh-pico provides support for the following (RT)OSs and protocols:

|    **(RT)OS**         |        **Transport Layer**       |  **Network Layer**  |                 **Data Link Layer**                |
|:---------------------:|:--------------------------------:|:-------------------:|:--------------------------------------------------:|
|       **Unix**        | UDP (unicast and multicast), TCP | IPv4, IPv6, 6LoWPAN |               WiFi, Ethernet, Thread               |
|     **Windows**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     |                   WiFi, Ethernet                   |
|      **Zephyr**       | UDP (unicast and multicast), TCP | IPv4, IPv6, 6LoWPAN |           WiFi, Ethernet, Thread, Serial           |
|     **Arduino**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     | WiFi, Ethernet, Bluetooth (Serial profile), Serial |
|     **ESP-IDF**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     |               WiFi, Ethernet, Serial               |
|      **MbedOS**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     |               WiFi, Ethernet, Serial               |
|      **OpenCR**       | UDP (unicast and multicast), TCP |         IPv4        |                        WiFi                        |
|    **Emscripten**     |             Websocket            |      IPv4, IPv6     |                   WiFi, Ethernet                   |
| **FreeRTOS-Plus-TCP** |         UDP (unicast), TCP       |         IPv4        |                      Ethernet                      |
| **Raspberry Pi Pico** | UDP (unicast and multicast), TCP |         IPv4        |      WiFi (for "W" version), Serial, USB (CDC)     |

Check the website [zenoh.io](http://zenoh.io) and the [roadmap](https://github.com/eclipse-zenoh/roadmap) for more detailed information.

-------------------------------

## 1. How to install it

The Eclipse zenoh-pico library is available as **Debian**, **RPM**, and **tgz** packages in the [Eclipse zenoh-pico download area](https://download.eclipse.org/zenoh/zenoh-pico/).
Those packages are built using manylinux2010 x86-32 and x86-64 for compatibility with most Linux platforms.
There are two kind of packages:

- **libzenohpico**: only contains the library file (.so)
- **libzenohpico-dev**: contains the zenoh-pico header files for development and depends on the _libzenohpico_ package

For other platforms - like RTOS for embedded systems / microcontrollers -, you will need to clone and build the sources. Check [below](#2. how-to-build-it) for more details.

-------------------------------

## 2. How to build it

### 2.1 Unix Environments

To build the **zenoh-pico** library, you need to ensure that [cmake](https://cmake.org) is available
on your platform -- if not please install it.

Once the [cmake](https://cmake.org) dependency is satisfied, just do the following for **CMake** version 3 and higher:

  -- CMake version 3 and higher --

  ```bash
  cd /path/to/zenoh-pico
  make
  make install # on Linux use **sudo**
  ```

If you want to build with debug symbols, set the `BUILD_TYPE=Debug`environment variable before to run make:

  ```bash
  cd /path/to/zenoh-pico
  BUILD_TYPE=Debug make
  make install # on Linux use **sudo**
  ```

For those that still have **CMake** version 2.8, do the following commands:

  ```bash
  cd /path/to/zenoh-pico
  mkdir build
  cd build
  cmake -DCMAKE_BUILD_TYPE=Release ../cmake-2.8
  make
  make install # on Linux use **sudo**
  ```

### 2.2. Real Time Operating System (RTOS) for Embedded Systems and Microcontrollers

In order to manage and ease the process of building and deploying into a a variety of platforms and frameworks
for embedded systems and microcontrollers, [PlatformIO](https://platformio.org) can be
used as a supporting platform.

Once the PlatformIO dependency is satisfied, follow the steps below for the
tested micro controllers.

#### 2.2.1. Zephyr

Note: tested with reel_board, nucleo-f767zi, nucleo-f420zi, and nRF52840 boards.

A typical PlatformIO project for Zephyr framework must have the following
structure:

  ```bash
  project_dir
  ├── include
  ├── lib
  ├── src
  │    └── main.c
  ├── zephyr
  │    ├── prj.conf
  │    └── CMakeLists.txt
  └── platformio.ini
  ```

To initialize this project structure, execute the following commands:

  ```bash
  mkdir -p /path/to/project_dir
  cd /path/to/project_dir
  platformio init -b reel_board
  platformio run
  ```

Include the CMakelist.txt and prj.conf in the project_dir/zephyr folder as
shown in the structure above,

  ```bash
  cp /path/to/zenoh-pico/docs/zephyr/reel_board/CMakelists.txt /path/to/project_dir/zephyr/
  cp /path/to/zenoh-pico/docs/zephyr/reel_board/prj.conf /path/to/project_dir/zephyr/
  ```

and add zenoh-pico as a library by doing:

  ```bash
  ln -s /path/to/zenoh-pico /path/to/project_dir/lib/zenoh-pico
  ```

or just include the following line in platformio.ini:

  ```ini
  lib_deps = https://github.com/eclipse-zenoh/zenoh-pico
  ```

Finally, your code should go into project_dir/src/main.c.
Check the examples provided in examples directory.

To build and upload the code into the board, run the following command:

  ```bash
  platformio run
  platformio run -t upload
  ```

#### 2.2.2. Arduino

Note: tested with az-delivery-devkit-v4 ESP32 board

A typical PlatformIO project for Arduino framework must have the following
structure:

  ```bash
  project_dir
  ├── include
  ├── lib
  ├── src
  │    └── main.ino
  └── platformio.ini
  ```

To initialize this project structure, execute the following commands:

  ```bash
  mkdir -p /path/to/project_dir
  cd /path/to/project_dir
  platformio init -b az-delivery-devkit-v4
  platformio run
  ```

Add zenoh-pico as a library by doing:

  ```bash
  ln -s /path/to/zenoh-pico /path/to/project_dir/lib/zenoh-pico
  ```

or just include the following line in platformio.ini:

  ```ini
  lib_deps = https://github.com/eclipse-zenoh/zenoh-pico
  ```

Finally, your code should go into project_dir/src/main.ino.
Check the examples provided in examples directory.

To build and upload the code into the board, run the following command:

  ```bash
  platformio run
  platformio run -t upload
  ```

#### 2.2.3. ESP-IDF

Note: tested with az-delivery-devkit-v4 ESP32 board

A typical PlatformIO project for ESP-IDF framework must have the following
structure:

  ```bash
  project_dir
  ├── include
  ├── lib
  ├── src
  |    ├── CMakeLists.txt
  │    └── main.c
  ├── CMakeLists.txt
  └── platformio.ini
  ```

To initialize this project structure, execute the following commands:

  ```bash
  mkdir -p /path/to/project_dir
  cd /path/to/project_dir
  platformio init -b az-delivery-devkit-v4
  platformio run
  ```

Add zenoh-pico as a library by doing:

  ```bash
  ln -s /path/to/zenoh-pico /path/to/project_dir/lib/zenoh-pico
  ```

or just include the following line in platformio.ini:

  ```ini
  lib_deps = https://github.com/eclipse-zenoh/zenoh-pico
  ```

Finally, your code should go into project_dir/src/main.ino.
Check the examples provided in examples directory.

To build and upload the code into the board, run the following command:

  ```bash
  platformio run
  platformio run -t upload
  ```

#### 2.2.4. MbedOS

Note: tested with nucleo-f747zi and nucleo-f429zi boards

A typical PlatformIO project for MbedOS framework must have the following structure:

  ```bash
  project_dir
  ├── include
  ├── src
  │    └── main.ino
  └── platformio.ini
  ```

To initialize this project structure, execute the following commands:

  ```bash
  mkdir -p /path/to/project_dir
  cd /path/to/project_dir
  platformio init -b az-delivery-devkit-v4
  platformio run
  ```

Add zenoh-pico as a library by doing:

  ```bash
  ln -s /path/to/zenoh-pico /path/to/project_dir/lib/zenoh-pico
  ```

or just include the following line in platformio.ini:

  ```ini
  lib_deps = https://github.com/eclipse-zenoh/zenoh-pico
  ```

Finally, your code should go into project_dir/src/main.ino.
Check the examples provided in examples directory.

To build and upload the code into the board, run the following command:

  ```bash
  platformio run
  platformio run -t upload
  ```

#### 2.2.5. OpenCR

Note: tested with ROBOTIS OpenCR 1.0 board

A typical PlatformIO project for OpenCR framework must have the following structure:

  ```bash
  project_dir
  ├── include
  ├── lib
  ├── src
  │    └── main.ino
  └── platformio.ini
  ```

Note: to add support for OpenCR in PlatformIO, follow the steps presented in our [blog](https://zenoh.io/blog/2022-02-08-dragonbot/).

To initialize this project structure, execute the following commands:

  ```bash
  mkdir -p /path/to/project_dir
  cd /path/to/project_dir
  platformio init -b opencr
  platformio run
  ```

Add zenoh-pico as a library by doing:

  ```bash
  ln -s /path/to/zenoh-pico /path/to/project_dir/lib/zenoh-pico
  ```

or just include the following line in platformio.ini:

  ```ini
  lib_deps = https://github.com/eclipse-zenoh/zenoh-pico
  ```

Finally, your code should go into project_dir/src/main.ino.
Check the examples provided in examples directory.

To build and upload the code into the board, run the following command:

  ```bash
  platformio run
  platformio run -t upload
  ```

#### 2.2.6. Raspberry Pi Pico

Note: tested with `Raspberry Pi Pico W` and `Raspberry Pi Pico 2 W` boards

Ensure your system has the necessary tools and libraries installed. Run the following commands:

```bash
sudo apt update
sudo apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential g++ libstdc++-arm-none-eabi-newlib
```

Set up the Raspberry Pi Pico SDK by cloning the repository:

```bash
export PICO_SDK_PATH=$HOME/src/pico-sdk
mkdir -p $PICO_SDK_PATH
cd $PICO_SDK_PATH
git clone https://github.com/raspberrypi/pico-sdk.git .
git submodule update --init
```

Clone the FreeRTOS Kernel repository for the project:

```bash
export FREERTOS_KERNEL_PATH=$HOME/src/FreeRTOS-Kernel/
mkdir -p $FREERTOS_KERNEL_PATH
cd $FREERTOS_KERNEL_PATH
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git .
git submodule update --init
```

Setup and build the examples:

- `PICO_BOARD` - Pico board type: pico, pico_w, pico2, pico2_w (default: pico_w)
- `WIFI_SSID` - Wi-Fi network SSID
- `WIFI_PASSWORD` - Wi-Fi password
- `ZENOH_CONFIG_MODE` - client or peer mode (default: client)
- `ZENOH_CONFIG_CONNECT` - connect endpoint (only for client mode, optional)
- `ZENOH_CONFIG_LISTEN` - listen endpoint (only for peer mode, optional)

```bash
cd examples/rpi_pico
cmake -Bbuild -DPICO_BOARD="pico" -DWIFI_SSID=wifi_network_ssid -DWIFI_PASSWORD=wifi_network_password -DZENOH_CONFIG_MODE=[client|peer] -DZENOH_CONFIG_CONNECT=connect -DZENOH_CONFIG_LISTEN=listen
cmake --build ./build
```

To flash the Raspberry Pi Pico board, connect it in bootloader mode (it will appear as a removable drive) and copy the generated .uf2 file onto it.

**Serial connection**:

To connect via UART specify pins or predefined device name and baud rate:

e.g.

```bash
-DZENOH_CONFIG_CONNECT="serial/0.1#baudrate=38400"
-DZENOH_CONFIG_CONNECT="serial/uart1_0#baudrate=38400"
```

Valid PIN combinations and associated device names:

| **PINS** | **Device name** |
|---------:|:---------------:|
|  0.1     |     uart0_0     |
|  4.5     |     uart1_0     |
|  8.9     |     uart1_1     |
|  12.13   |     uart0_1     |
|  16.17   |     uart0_2     |

**USB Serial connection (experemental)**:

To enable this feature, zenoh-pico should be compiled with `Z_FEATURE_LINK_SERIAL_USB` and `Z_FEATURE_UNSTABLE_API` enabled.

To connect via USB CDC, specify `usb` device:

e.g.

```bash
-DZENOH_CONFIG_CONNECT="serial/usb#baudrate=112500"
```

On the host Zenoh, specify the USB CDC device:

e.g.

```bash
zenohd -l serial//dev/ttyACM1#baudrate=112500
```

#### 2.2.7. STM32 ThreadX

1. Create a new project using STMCubeIDE for your target MCU.

2. In CubeMX project configuration tool:
    - Add and enable AZURE-RTOS(threadx) Middleware.
    - Enable UART periphery, set up RX DMA in circular mode and enable UART global interrupt.
    - Move HAL_Tick to TIM peripheral for threadx to work with HAL. (SYS - Timebase source configuration)

3. Generate initialization code with CubeMX.

4. Clone zenoh-pico repository to your project folder (or submodule, copy files)

5. Add zenoh-pico/src to project source folders. Exclude any folders in platforms/* except common and threadx/stm32.

6. Add zenoh-pico/include to project include paths.

7. Edit zenoh-pico/include/config.h to exclude unsuported features or add compile flags

8. Add "hal.h" file to your project. This file should include your target hal headers (ex. `#include "stm32f4xx_hal.h"`)

9. Add defines to project:
    - `ZENOH_THREADX_STM32`
    - `ZENOH_HUART=huartx` (huart1 / huar2, etc...)
    - `ZENOH_THREADX_STM32_GEN_IRQ=0` (only if you want to write your own interrupt handler)

10. Exclude from build <project_root>/Core/Src/app_threadx.c and replace with one of the z_*.c* files from zenoh-pico/examples/threadx_stm32/

11. Set `TICK_INT_PRIORITY` in stm32xx_hal_conf.h to higher value than SysTick. (0 works ok for testing).

12. Set static bytepool size bigger than 25kB.

13. On host compile zenohd with serial support and run with:

    ```bash
    zenohd -l serial//dev/ttyACM0#baudrate=115200
    ```

## 3. Running the Examples

The simplest way to run some of the example is to get a Docker image of the **zenoh** router (see [http://zenoh.io/docs/getting-started/quick-test/](http://zenoh.io/docs/getting-started/quick-test/)) and then to run the examples on your machine.

### 3.1. Starting the Zenoh Router

Assuming you've pulled the Docker image of the **zenoh** router on a Linux host (to leverage UDP multicast scouting as explained [here](https://zenoh.io/docs/getting-started/quick-test/#run-zenoh-router-in-a-docker-container), then simply do:

```bash
docker run --init --net host eclipse/zenoh:main
```

To see the zenoh manual page, simply do:

```bash
docker run --init --net host eclipse/zenoh:main --help
```

:warning: **Please notice that the `--net host` option in Docker is restricted to Linux only.**
The cause is that Docker doesn't support UDP multicast between a container and its host (see cases [moby/moby#23659](https://github.com/moby/moby/issues/23659), [moby/libnetwork#2397](https://github.com/moby/libnetwork/issues/2397) or [moby/libnetwork#552](https://github.com/moby/libnetwork/issues/552)). The only known way to make it work is to use the `--net host` option that is [only supported on Linux hosts](https://docs.docker.com/network/host/).

### 3.2. Basic Pub/Sub Example

Assuming that (1) you are running the **zenoh** router,  and (2) you are under the build directory, do:

```bash
./z_sub
```

And on another shell, do:

```bash
./z_pub
```

### 3.3. Basic Queryable/Get Example

Assuming you are running the **zenoh** router, do:

```bash
./z_queryable
```

And on another shell, do:

```bash
./z_get
```

### 3.4. Basic Pub/Sub Example - P2P over UDP multicast

Zenoh-Pico can also work in P2P mode over UDP multicast. This allows a Zenoh-Pico application to communicate directly with another Zenoh-Pico application without requiring a Zenoh Router.

Assuming that (1) you are under the build directory, do:

```bash
./z_sub -m peer -l udp/224.0.0.123:7447#iface=lo0
```

And on another shell, do:

```bash
./z_pub -m peer -l udp/224.0.0.123:7447#iface=lo0
```

where `lo0` is the network interface you want to use for multicast communication.

> [!WARNING]
> Multicast communication does not perform any negotiation upon group joining. Because of that, it is important that all transport parameters are the same to make sure all your nodes in the system can communicate.
> One common parameter to configure is the batch size since its default value depends on the actual platform when operating on multicast:
>
> - with zenoh-pico you can configure it via the `BATCH_MULTICAST_SIZE` build option (see [below](#error-when-opening-a-session-on-a-microcontroller))
> - with other Zenoh APIs, set the "transport/link/tx/batch_size" value in configuration file
>
> E.g., the batch size on Linux and Windows is 65535 bytes, on Mac OS X is 9216, anything else is 8192.

### 3.4. Basic Pub/Sub Example - Mixing Client and P2P communication

To allow Zenoh-Pico unicast clients to talk to Zenoh-Pico multicast peers, as well as with any other Zenoh client/peer, you need to start a Zenoh Router that listens on both multicast and unicast:

```bash
docker run --init --net host eclipse/zenoh:main -l udp/224.0.0.123:7447#iface=lo0 -l tcp/127.0.0.1:7447
```

Assuming that (1) you are running the **zenoh** router as indicated above, and (2) you are under the build directory, do:

```bash
./z_sub -m client -e tcp/127.0.0.1:7447 
```

A subscriber will connect in client mode to the **zenoh** router over TCP unicast.

And on another shell, do:

```bash
./z_pub -m peer -l udp/224.0.0.123:7447#iface=lo0
```

A publisher will start publishing over UDP multicast and the **zenoh** router will take care of forwarding data from the Zenoh-Pico publisher to the Zenoh-Pico subscriber.

## Troubleshooting

### Activate debug logs

By default debug logs are deactivated but if you're encountering issues they can help you finding the cause. To activate them you need to pass the build flag value: `-DZENOH_LOG_DEBUG`

### Error when opening a session on a microcontroller

If you get an error when opening the session even though everything is setup correctly, it might be because the default buffer sizes are too large for the limited memory available on your system.

The first thing to try is to reduce the values of the following configuration options (found in `CMakeLists.txt`):

- BATCH_UNICAST_SIZE: The maximum size of a packet in client mode.
- BATCH_MULTICAST_SIZE: The maximum size of a packet in peer mode.
- FRAG_MAX_SIZE: The maximum size of a message that can be fragmented into multiple packets.

Until you find values that suits both your app requirements and your system memory constraints.

These values can also be passed directly as cmake args. For example, in a `platformio.ini` you might write:

```bash
board_build.cmake_extra_args=
  -DBATCH_UNICAST_SIZE=1024
  -DFRAG_MAX_SIZE=2048
```
