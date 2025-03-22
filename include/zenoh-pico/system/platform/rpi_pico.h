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

#ifndef ZENOH_PICO_SYSTEM_RPI_PICO_TYPES_H
#define ZENOH_PICO_SYSTEM_RPI_PICO_TYPES_H

#include <time.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#if Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
#include "lwip/ip4_addr.h"
#endif
#if Z_FEATURE_LINK_SERIAL == 1
#include "hardware/gpio.h"
#include "hardware/uart.h"
#endif
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_MULTI_THREAD == 1
typedef struct {
    const char *name;
    UBaseType_t priority;
    size_t stack_depth;
} z_task_attr_t;

typedef struct {
    TaskHandle_t handle;
    EventGroupHandle_t join_event;
} _z_task_t;

typedef SemaphoreHandle_t _z_mutex_t;
typedef SemaphoreHandle_t _z_mutex_rec_t;
typedef struct {
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t sem;
    int waiters;
} _z_condvar_t;
#endif  // Z_MULTI_THREAD == 1

typedef struct timespec z_clock_t;
typedef struct timeval z_time_t;

typedef struct {
    union {
#if Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
        int _fd;
#endif
#if Z_FEATURE_LINK_SERIAL == 1
        uart_inst_t *_serial;
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

#ifdef __cplusplus
}
#endif

#if Z_FEATURE_LINK_SERIAL_USB == 1
void _z_usb_uart_init();
void _z_usb_uart_deinit();
void _z_usb_uart_write(const uint8_t *buf, int length);
uint8_t _z_usb_uart_getc();
#endif

#endif  // ZENOH_PICO_SYSTEM_RPI_PICO_TYPES_H
