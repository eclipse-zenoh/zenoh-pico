![zenoh banner](./zenoh-dragon.png)

![Build](https://github.com/eclipse-zenoh/zenoh-pico/workflows/build/badge.svg)
![Crossbuild](https://github.com/eclipse-zenoh/zenoh-pico/workflows/crossbuild/badge.svg)
![integration](https://github.com/eclipse-zenoh/zenoh-pico/workflows/integration/badge.svg)
[![Documentation Status](https://readthedocs.org/projects/zenoh-c/badge/?version=latest)](https://zenoh-c.readthedocs.io/en/latest/?badge=latest)
[![Gitter](https://badges.gitter.im/atolab/zenoh.svg)](https://gitter.im/atolab/zenoh?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![License](https://img.shields.io/badge/License-EPL%202.0-blue)](https://choosealicense.com/licenses/epl-2.0/)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# Eclipse zenoh C Client API

[Eclipse zenoh](http://zenoh.io) is an extremely efficient and fault-tolerant [Named Data Networking](http://named-data.net) (NDN) protocol 
that is able to scale down to extremely constrainded devices and networks. 

zenoh-pico targets constrained devices and offers a C API for pure clients, i.e., it does not support peer-to-peer communication.
zenoh-pico can be easily tested against a zenoh router running in a Docker container (see https://github.com/eclipse-zenoh/zenoh#how-to-test-it). 

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

## Running the Examples
The simplest way to run some of the example is to get a Docker image of the **zenoh** network router (see http://zenoh.io/docs/getting-started/quick-test/) and then to run the examples on your machine.

### Starting the zenoh Network Service
Assuming you've pulled the Docker image of the **zenoh** network router, then simply do:

```bash
$ docker run --init -p 7447:7447/tcp -p 7447:7447/udp -p 8000:8000/tcp eclipse/zenoh
```

To see the zenoh manual page, simply do:

```bash
$ docker run --init -p 7447:7447/tcp -p 7447:7447/udp -p 8000:8000/tcp eclipse/zenoh --help
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







