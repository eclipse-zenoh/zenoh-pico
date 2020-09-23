![zenoh banner](./zenoh-dragon.png)

![Build (simple)](https://github.com/eclipse-zenoh/zenoh-c/workflows/Build%20(simple)/badge.svg)
![Build cross-platforms](https://github.com/eclipse-zenoh/zenoh-c/workflows/Build%20cross-platforms/badge.svg)
[![Documentation Status](https://readthedocs.org/projects/zenoh-c/badge/?version=latest)](https://zenoh-c.readthedocs.io/en/latest/?badge=latest)
[![Gitter](https://badges.gitter.im/atolab/zenoh.svg)](https://gitter.im/atolab/zenoh?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)
[![License](https://img.shields.io/badge/License-EPL%202.0-blue)](https://choosealicense.com/licenses/epl-2.0/)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)


# Eclipse zenoh C Client API

[Eclipse zenoh](http://zenoh.io) is an extremely efficient and fault-tolerant [Named Data Networking](http://named-data.net) (NDN) protocol 
that is able to scale down to extremely constrainded devices and networks. 

The C API is for pure clients, in other terms does not support peer-to-peer communication, can be easily tested against a zenoh router running in a Docker container (see https://github.com/eclipse-zenoh/zenoh#how-to-test-it). 

-------------------------------
## How to install it

The Eclipse zenoh-c library is available as **Debian** or **RPM** packages in the [Eclipse zenoh download area](https://download.eclipse.org/zenoh/zenoh-c/).
Those packages are built using manylinux2010 x86-32 and x86-64 to be compatible with most of the Linux platforms.
There are 2 kind of packages:

 - **libzenohc**: only contains the library file (.so)
 - **libzenohc-dev**: contains the zenoh-c header files for development. Depends on *libzenohc* package

For other platforms, you will need to clone and build the sources.

WARNING: Note that zenoh-c has not been ported on Windows yet!

-------------------------------
## How to build it 
To build the **zenoh-c** client API you need to ensure that [cmake](https://cmake.org) is available on your platform -- if not please install it. 

Once the [cmake](https://cmake.org) dependency is satisfied, just do the following for **CMake** version 3 and higher:

  -- CMake version 3 and higher -- 

  ```bash
  $ cd /path/to/zenoh-c
  $ make
  $ make install # on linux use **sudo**
  ```

If you want to build with debug symbols set the `BUILD_TYPE=Debug`environment variable before to run make:
  ```bash
  $ cd /path/to/zenoh-c
  $ BUILD_TYPE=Debug make
  $ make install # on linux use **sudo**
  ```

For those that still have **CMake** version 2.8, do the following commands:

  ```bash
  $ cd /path/to/zenoh-c
  $ mkdir build
  $ cd build
  $ cmake -DCMAKE_BUILD_TYPE=Release ../cmake-2.8
  $ make 
  $ make install # on linux use **sudo**
  ```

## Running the Examples
The simplest way to run some of the example is to get a Docker image of the **zenoh** network router (see https://github.com/eclipse-zenoh/zenoh#how-to-test-it) and then to run the examples on your machine.

### Starting the zenoh Network Service
Assuming you've pulled the Docker image of the **zenoh** network service, then simply do:

```bash
$ docker run --init -p 7447:7447/tcp -p 7447:7447/udp -p 8000:8000/tcp adlinktech/eclipse-zenoh:latest
```

To see the zenoh manual page, simply do:

```bash
$ docker run --init -p 7447:7447/tcp -p 7447:7447/udp -p 8000:8000/tcp adlinktech/eclipse-zenoh:latest --help
```


### Basic Pub/Sub Example
Assuming that (1) you are running the **zenoh** network service,  and (2) you are under the build directory, do:
```bash
$ ./z_sub
```

And on another shell, do:
```bash
$ ./z_pub
```
## Storage and Query Example
Assuming you are running the **zenoh** network service, do:
```bash
$ ./z_storage
```
And on another shell, do:
```bash
$ ./z_pub
```
After a few publications just terminate the publisher, and then try to query the storage:
```bash
$ ./z_query
```







