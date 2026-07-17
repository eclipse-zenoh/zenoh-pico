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

#ifndef ZENOH_PICO_SYSTEM_WASI_TYPES_H
#define ZENOH_PICO_SYSTEM_WASI_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

#include "zenoh-pico/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// WASI does not support pthreads — Z_FEATURE_MULTI_THREAD must be 0.
// Dummy threading types are provided by platform.h when multi-thread is disabled.

typedef struct timespec z_clock_t;
typedef struct timeval z_time_t;

typedef struct {
    union {
#if Z_FEATURE_LINK_TCP == 1
        int _fd;
#endif
    };
} _z_sys_net_socket_t;

typedef struct {
    union {
#if Z_FEATURE_LINK_TCP == 1
        struct addrinfo *_iptcp;
#endif
    };
} _z_sys_net_endpoint_t;

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_SYSTEM_WASI_TYPES_H */
