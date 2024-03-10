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

#ifndef ZENOH_PICO_SYSTEM_ESPIDF_TYPES_H
#define ZENOH_PICO_SYSTEM_ESPIDF_TYPES_H

#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include "zenoh-pico/config.h"

#if Z_FEATURE_MULTI_THREAD == 1
#include <pthread.h>

typedef struct {
    const char *name;
    UBaseType_t priority;
    size_t stack_depth;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    _Bool static_allocation;
    StackType_t *stack_buffer;
    StaticTask_t *task_buffer;
#endif /* SUPPORT_STATIC_ALLOCATION */
} zp_task_attr_t;
typedef struct {
    TaskHandle_t handle;
    EventGroupHandle_t join_event;
} zp_task_t;
typedef pthread_mutex_t zp_mutex_t;
typedef pthread_cond_t zp_condvar_t;
#endif  // Z_FEATURE_MULTI_THREAD == 1

typedef struct timespec zp_clock_t;
typedef struct timeval zp_time_t;

typedef struct {
    union {
#if Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
        int _fd;
#endif
#if Z_FEATURE_LINK_SERIAL == 1
        uart_port_t _serial;
#endif
    };
} _z_sys_net_socket_t;

typedef struct {
    union {
#if Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
        struct addrinfo *_iptcp;
#endif
    };
} _z_sys_net_endpoint_t;

#endif /* ZENOH_PICO_SYSTEM_ESPIDF_TYPES_H */
