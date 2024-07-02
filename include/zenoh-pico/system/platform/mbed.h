//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#ifndef ZENOH_PICO_SYSTEM_MBED_TYPES_H
#define ZENOH_PICO_SYSTEM_MBED_TYPES_H

#include <stdint.h>
#include <sys/time.h>

#include "zenoh-pico/config.h"

typedef int _z_socket_t;

#if Z_FEATURE_MULTI_THREAD == 1
typedef void *_z_task_t;      // Workaround as MBED is a C++ library
typedef void *z_task_attr_t;  // Workaround as MBED is a C++ library
typedef void *_z_mutex_t;     // Workaround as MBED is a C++ library
typedef void *_z_condvar_t;   // Workaround as MBED is a C++ library
#endif                        // Z_FEATURE_MULTI_THREAD == 1

typedef void *z_clock_t;  // Not defined
typedef struct timeval z_time_t;

typedef struct BufferedSerial BufferedSerial;  // Forward declaration to be used in _z_sys_net_socket_t
typedef struct UDPSocket UDPSocket;            // Forward declaration to be used in _z_sys_net_socket_t
typedef struct TCPSocket TCPSocket;            // Forward declaration to be used in _z_sys_net_socket_t
typedef struct SocketAddress SocketAddress;    // Forward declaration to be used in _z_sys_net_endpoint_t

typedef struct {
    _Bool _err;
    union {
#if Z_FEATURE_LINK_TCP == 1
        TCPSocket *_tcp;  // As pointer to cross the boundary between C and C++
#endif
#if Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
        UDPSocket *_udp;  // As pointer to cross the boundary between C and C++
#endif
#if Z_FEATURE_LINK_SERIAL == 1
        BufferedSerial *_serial;  // As pointer to cross the boundary between C and C++
#endif
    };
} _z_sys_net_socket_t;

typedef struct {
    _Bool _err;
    union {
#if Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
        SocketAddress *_iptcp;  // As pointer to cross the boundary between C and C++
#endif
    };
} _z_sys_net_endpoint_t;

#endif /* ZENOH_PICO_SYSTEM_MBED_TYPES_H */
