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

## Getting Started

To build and run zenoh-pico, ensure you have `cmake` installed.

```bash
mkdir build && cd build
cmake ..
make
```
You can find examples in the `examples` directory. To run a simple subscriber:
```bash
./examples/z_sub
```

## Platform Support

Currently, zenoh-pico provides support for the following (RT)OSs and protocols:

|    **(RT)OS**         |        **Transport Layer**       |  **Network Layer**  |                 **Data Link Layer**                |
|:---------------------:|:--------------------------------:|:-------------------:|:--------------------------------------------------:|
|       **Unix**        | UDP (unicast and multicast), TCP | IPv4, IPv6, 6LoWPAN |               WiFi, Ethernet, Thread               |
|     **Windows**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     |                   WiFi, Ethernet                   |
|      **Zephyr**       | UDP (unicast and multicast), TCP | IPv4, IPv6, 6LoWPAN |           WiFi, Ethernet, Thread, Serial           |
|     **Arduino**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     | WiFi, Ethernet, Bluetooth (Serial profile), Serial |
|     **ESP-IDF**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     |               WiFi, Ethernet, Serial               |
|      **MbedOS**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     |               WiFi, Ethernet, Serial               |
|      **OpenCR**       | UDP (unicast and multicast), TCP |      IPv4, IPv6     |               WiFi, Ethernet, Serial               |