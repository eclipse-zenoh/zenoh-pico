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

#ifndef ZENOH_PICO_SYSTEM_WASM_TYPES_H
#define ZENOH_PICO_SYSTEM_WASM_TYPES_H

#include <stdint.h>

#include "zenoh-pico/config.h"

#if Z_FEATURE_MULTI_THREAD == 1
#include <pthread.h>

typedef pthread_t _z_task_t;
typedef pthread_attr_t z_task_attr_t;
typedef pthread_mutex_t _z_mutex_t;
typedef pthread_cond_t _z_condvar_t;
#endif  // Z_FEATURE_MULTI_THREAD == 1

typedef double z_clock_t;
typedef double z_time_t;

typedef struct {
    union {
#if Z_FEATURE_LINK_WS == 1
        struct {
            int _fd;
            uint32_t _tout;
        } _ws;
#endif
    };
} _z_sys_net_socket_t;

typedef struct {
    union {
#if Z_FEATURE_LINK_WS == 1
        struct addrinfo *_iptcp;
#endif
    };
} _z_sys_net_endpoint_t;

#endif /* ZENOH_PICO_SYSTEM_WASM_TYPES_H */
