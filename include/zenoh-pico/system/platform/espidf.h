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
#include <pthread.h>

#include "zenoh-pico/config.h"

#if Z_MULTI_THREAD == 1
typedef TaskHandle_t _z_task_t;
typedef void *_z_task_attr_t;  // Not used in ESP32
typedef pthread_mutex_t _z_mutex_t;
typedef pthread_cond_t _z_condvar_t;
#endif  // Z_MULTI_THREAD == 1

typedef struct timespec z_clock_t;
typedef struct timeval z_time_t;

typedef struct {
    union {
#if Z_LINK_TCP == 1 || Z_LINK_UDP_MULTICAST == 1 || Z_LINK_UDP_UNICAST == 1
        int _fd;
#endif
#if Z_LINK_SERIAL == 1
        uart_port_t _serial;
#endif
    };
} _z_sys_net_socket_t;

typedef struct {
    union {
#if Z_LINK_TCP == 1 || Z_LINK_UDP_MULTICAST == 1 || Z_LINK_UDP_UNICAST == 1
        struct addrinfo *_iptcp;
#endif
    };
} _z_sys_net_endpoint_t;

#endif /* ZENOH_PICO_SYSTEM_ESPIDF_TYPES_H */
