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

#ifndef ZENOH_PICO_SYSTEM_WINDOWS_H
#define ZENOH_PICO_SYSTEM_WINDOWS_H

#include "zenoh-pico/config.h"

#if Z_MULTI_THREAD == 1
typedef void *_z_task_t;
typedef void *_z_task_attr_t;
typedef void *_z_mutex_t;
typedef void *_z_condvar_t;
#endif  // Z_MULTI_THREAD == 1

typedef void *z_clock_t;
typedef void *z_time_t;

typedef struct {
    union {
#if Z_LINK_TCP == 1
        void *_tcp;  // As pointer to cross the boundary between C and C++
#endif
#if Z_LINK_UDP_MULTICAST == 1 || Z_LINK_UDP_UNICAST == 1
        void *_udp;  // As pointer to cross the boundary between C and C++
#endif
    };
} _z_sys_net_socket_t;

typedef struct {
    union {
#if Z_LINK_TCP == 1 || Z_LINK_UDP_MULTICAST == 1 || Z_LINK_UDP_UNICAST == 1
        void *_iptcp;  // As pointer to cross the boundary between C and C++
#endif
    };
} _z_sys_net_endpoint_t;

inline void __asm__(void) { ; }

#endif /* ZENOH_PICO_SYSTEM_VOID_H */
