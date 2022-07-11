<img src="https://raw.githubusercontent.com/eclipse-zenoh/zenoh/master/zenoh-dragon.png" height="150">

![Build](https://github.com/eclipse-zenoh/zenoh-pico/workflows/build/badge.svg)
![Crossbuild](https://github.com/eclipse-zenoh/zenoh-pico/workflows/crossbuild/badge.svg)
![integration](https://github.com/eclipse-zenoh/zenoh-pico/workflows/integration/badge.svg)
[![Documentation Status](https://readthedocs.org/projects/zenoh-c/badge/?version=latest)](https://zenoh-c.readthedocs.io/en/latest/?badge=latest)
[![Discussion](https://img.shields.io/badge/discussion-on%20github-blue)](https://github.com/eclipse-zenoh/roadmap/discussions)
[![Discord](https://img.shields.io/badge/chat-on%20discord-blue)](https://discord.gg/vSDSpqnbkm)
[![License](https://img.shields.io/badge/License-EPL%202.0-blue)](https://choosealicense.com/licenses/epl-2.0/)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# Eclipse zenoh native C library for constrained devices

[Eclipse zenoh](http://zenoh.io) is an extremely efficient and fault-tolerant [Named Data Networking](http://named-data.net) (NDN) protocol
that is able to scale down to extremely constrainded devices and networks.

zenoh-pico targets constrained devices and offers a C API for pure clients, i.e., it does not support peer-to-peer communication.
zenoh-pico can be easily tested against a zenoh router running in a Docker container (see https://github.com/eclipse-zenoh/zenoh#how-to-test-it).

Check the website [zenoh.io](http://zenoh.io) and the [roadmap](https://github.com/eclipse-zenoh/roadmap) for more detailed information.

-------------------------------
## How to install it

The Eclipse zenoh-pico library is available as **Debian**, **RPM**, and **tgz** packages in the [Eclipse zenoh-pico download area](https://download.eclipse.org/zenoh/zenoh-pico/).
Those packages are built using manylinux2010 x86-32 and x86-64 to be compatible with most of the Linux platforms.
There are 2 kind of packages:

 - **libzenohpico**: only contains the library file (.so)
 - **libzenohpico-dev**: contains the zenoh-pico header files for development. Depends on *libzenohpico* package

For other platforms, you will need to clone and build the sources.

WARNING: Note that zenoh-pico has not been ported on Windows yet!

-------------------------------
## How to build it
To build the **zenoh-pico** client API you need to ensure that [cmake](https://cmake.org) is available on your platform -- if not please install it.

Once the [cmake](https://cmake.org) dependency is satisfied, just do the following for **CMake** version 3 and higher:

  -- CMake version 3 and higher --

  ```bash
  $ cd /path/to/zenoh-pico
  $ make
  $ make install # on Linux use **sudo**
  ```

If you want to build with debug symbols set the `BUILD_TYPE=Debug`environment variable before to run make:
  ```bash
  $ cd /path/to/zenoh-pico
  $ BUILD_TYPE=Debug make
  $ make install # on Linux use **sudo**
  ```

For those that still have **CMake** version 2.8, do the following commands:

  ```bash
  $ cd /path/to/zenoh-pico
  $ mkdir build
  $ cd build
  $ cmake -DCMAKE_BUILD_TYPE=Release ../cmake-2.8
  $ make
  $ make install # on Linux use **sudo**
  ```

### How to build for microcontrollers

In order to manage and ease the process of building and deploying into a
a variety of microcontrollers, [PlatformIO](https://platformio.org) can be
used as a supporting platform.

Once the PlatformIO dependency is satisfied, follow the steps below for the
tested micro controllers.

#### Zephyr
Note: tested with reel_board

A typical PlatformIO project for Zephyr framework must have the following
structure:

  ```bash
  project_dir
  ├── include
  ├── src
  │    └── main.c
  ├── zephyr
  │    ├── prj.conf
  │    └── CMakeLists.txt
  └── platformio.ini
  ```

To initialize this project structure, execute the following commands:

  ```bash
  $ mkdir -p /path/to/project_dir
  $ cd /path/to/project_dir
  $ platformio init -b reel_board
  $ platformio run
  ```

Include the CMakelist.txt and prj.conf in the project_dir/zephyr folder as
shown in the structure above, and add zenoh-pico as a library by doing:

  ```bash
  $ cp /path/to/zenoh-pico/docs/zephyr/reel_board/CMakelists.txt /path/to/project_dir/zephyr/
  $ cp /path/to/zenoh-pico/docs/zephyr/reel_board/prj.conf /path/to/project_dir/zephyr/
  $ ln -s /path/to/zenoh-pico /path/to/project_dir/lib/zenoh-pico
  ```

Finally, your code should go into project_dir/src/main.c (examples provided
with zenoh-pico work out of the box with Zephyr).

To build and upload the code into the board, run the following command:

  ```bash
  platformio run
  platformio run -t upload
  ```

#### ESP32
Note: tested with az-delivery-devkit-v4 board

A typical PlatformIO project for ESP32 framework must have the following
structure:

  ```bash
  project_dir
  ├── include
  ├── src
  │    └── main.ino
  └── platformio.ini
  ```

To initialize this project structure, execute the following commands:

  ```bash
  $ mkdir -p /path/to/project_dir
  $ cd /path/to/project_dir
  $ platformio init -b az-delivery-devkit-v4
  $ platformio run
  ```

Add zenoh-pico as a library by doing:

  ```bash
  $ ln -s /path/to/zenoh-pico /path/to/project_dir/lib/zenoh-pico
  ```

Finally, your code should go into project_dir/src/main.ino.
Check the examples provided in ./examples/net/esp32 directory.

To build and upload the code into the board, run the following command:

  ```bash
  platformio run
  platformio run -t upload
  ```

## Running the Examples
The simplest way to run some of the example is to get a Docker image of the **zenoh** network router (see http://zenoh.io/docs/getting-started/quick-test/) and then to run the examples on your machine.

### Starting the zenoh Network Service
Assuming you've pulled the Docker image of the **zenoh** network router on a Linux host (to leverage UDP multicast scouting has explained [here](https://zenoh.io/docs/getting-started/quick-test/#run-zenoh-router-in-a-docker-container)), then simply do:

```bash
$ docker run --init -net host eclipse/zenoh:master
```

To see the zenoh manual page, simply do:

```bash
$ docker run --init -net host eclipse/zenoh:master --help
```


### Basic Pub/Sub Example
Assuming that (1) you are running the **zenoh** network router,  and (2) you are under the build directory, do:
```bash
$ ./zn_sub
```

And on another shell, do:
```bash
$ ./zn_pub
```
## Basic Eval/Query Example
Assuming you are running the **zenoh** network service, do:
```bash
$ ./zn_eval
```

And on another shell, do:
```bash
$ ./zn_query
```







