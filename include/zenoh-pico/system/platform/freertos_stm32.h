//
// Copyright (c) 2023 Fictionlab sp. z o.o.
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   Błażej Sowa, <blazej@fictionlab.pl>

#ifndef ZENOH_PICO_SYSTEM_FREERTOS_STM32_TYPES_H
#define ZENOH_PICO_SYSTEM_FREERTOS_STM32_TYPES_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "usart.h"

#if Z_FEATURE_MULTI_THREAD == 1
typedef struct {
    const char *name;
    UBaseType_t priority;
    size_t stack_depth;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    bool static_allocation;
    StackType_t *stack_buffer;
    StaticTask_t *task_buffer;
#endif /* SUPPORT_STATIC_ALLOCATION */
} z_task_attr_t;

typedef struct {
    TaskHandle_t handle;
    EventGroupHandle_t join_event;
    void *(*fun)(void *);
    void *arg;
} _z_task_t;

typedef struct {
    SemaphoreHandle_t handle;
} _z_mutex_t;

typedef _z_mutex_t _z_mutex_rec_t;

typedef struct {
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t sem;
    int waiters;
} _z_condvar_t;
#endif  // Z_MULTI_THREAD == 1

typedef TickType_t z_clock_t;
typedef struct timeval z_time_t;

typedef struct {
    union {
#if Z_FEATURE_LINK_SERIAL == 1
        struct {
			UART_HandleTypeDef *_serial;
        };
#endif
	};
} _z_sys_net_socket_t;

typedef struct {
    union {
    };
} _z_sys_net_endpoint_t;

#endif
