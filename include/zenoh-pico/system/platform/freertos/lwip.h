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
//   João Mário Lago, <joaolago@bluerobotics.com>
//

#ifndef ZENOH_PICO_SYSTEM_FREERTOS_PLUS_LWIP_TYPES_H
#define ZENOH_PICO_SYSTEM_FREERTOS_PLUS_LWIP_TYPES_H

#include <time.h>

#include "FreeRTOS.h"
#include "lwip/sockets.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_MULTI_THREAD == 1
#include "event_groups.h"

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
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    StaticEventGroup_t join_event_buffer;
#endif /* SUPPORT_STATIC_ALLOCATION */
} _z_task_t;

typedef struct {
    SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    StaticSemaphore_t buffer;
#endif /* SUPPORT_STATIC_ALLOCATION */
} _z_mutex_t;

typedef _z_mutex_t _z_mutex_rec_t;

typedef struct {
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t sem;
    int waiters;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    StaticSemaphore_t mutex_buffer;
    StaticSemaphore_t sem_buffer;
#endif /* SUPPORT_STATIC_ALLOCATION */
} _z_condvar_t;

typedef TaskHandle_t _z_task_id_t;
#endif  // Z_MULTI_THREAD == 1

typedef TickType_t z_clock_t;
typedef struct timeval z_time_t;

typedef struct {
    union {
#if defined(ZP_PLATFORM_SOCKET_LINKS_ENABLED)
        int _socket;
#endif
    };
} _z_sys_net_socket_t;

typedef struct {
    union {
#if defined(ZP_PLATFORM_SOCKET_LINKS_ENABLED)
        struct addrinfo *_iptcp;
#endif
    };
} _z_sys_net_endpoint_t;

#if defined(ZP_PLATFORM_SOCKET_LWIP) && defined(ZP_PLATFORM_SOCKET_LINKS_ENABLED)
#define ZP_LWIP_SOCKET_HELPERS_DEFINED 1
static inline int _z_lwip_socket_get(_z_sys_net_socket_t sock) { return sock._socket; }
static inline void _z_lwip_socket_set(_z_sys_net_socket_t *sock, int fd) { sock->_socket = fd; }
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on FreeRTOS + LWIP port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1
#error "Serial not supported yet on FreeRTOS + LWIP port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on FreeRTOS + LWIP port of Zenoh-Pico"
#endif

#ifdef __cplusplus
}
#endif

#endif
