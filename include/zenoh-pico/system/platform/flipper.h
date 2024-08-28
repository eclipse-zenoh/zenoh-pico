//
// Copyright (c) 2024 ZettaScale Technology
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

#ifndef ZENOH_PICO_SYSTEM_FLIPPER_TYPES_H
#define ZENOH_PICO_SYSTEM_FLIPPER_TYPES_H

#include <furi.h>
#include <furi_hal.h>
#include <sys/time.h>

#include "zenoh-pico/config.h"

#define FLIPPER_DEFAULT_THREAD_STACK_SIZE 2048
#define FLIPPER_SERIAL_STREAM_BUFFER_SIZE 512
#define FLIPPER_SERIAL_STREAM_TRIGGERED_LEVEL 10
#define FLIPPER_SERIAL_TIMEOUT_MS 200

#if Z_FEATURE_MULTI_THREAD == 1
typedef FuriThread* _z_task_t;
typedef uint32_t z_task_attr_t;
typedef FuriMutex* _z_mutex_t;
typedef void* _z_condvar_t;
#endif  // Z_FEATURE_MULTI_THREAD == 1

typedef struct timespec z_clock_t;
typedef struct timeval z_time_t;

typedef struct {
#if Z_FEATURE_LINK_SERIAL == 1
    FuriStreamBuffer* _rx_stream;
    FuriHalSerialHandle* _serial;
#endif
} _z_sys_net_socket_t;

#endif /* ZENOH_PICO_SYSTEM_FLIPPER_TYPES_H */
