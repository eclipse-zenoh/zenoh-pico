#ifndef ZENOH_PICO_SYSTEM_FREERTOS_PLUS_TCP_TYPES_H
#define ZENOH_PICO_SYSTEM_FREERTOS_PLUS_TCP_TYPES_H

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "semphr.h"

#if Z_MULTI_THREAD == 1
typedef struct {
    const char *name;
    UBaseType_t priority;
    size_t stack_depth;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    _Bool static_allocation;
    StackType_t *stack_buffer;
    StaticTask_t *task_buffer;
#endif /* SUPPORT_STATIC_ALLOCATION */
} _z_task_attr_t;

typedef struct {
    TaskHandle_t handle;
    EventGroupHandle_t join_event;
} _z_task_t;

typedef SemaphoreHandle_t _z_mutex_t;
typedef void *_z_condvar_t;
#endif  // Z_MULTI_THREAD == 1

typedef TickType_t z_clock_t;
typedef TickType_t z_time_t;

typedef struct {
    union {
#if Z_LINK_TCP == 1 || Z_LINK_UDP_MULTICAST == 1 || Z_LINK_UDP_UNICAST == 1
        Socket_t _socket;
#endif
    };
} _z_sys_net_socket_t;

typedef struct {
    union {
#if Z_LINK_TCP == 1 || Z_LINK_UDP_MULTICAST == 1 || Z_LINK_UDP_UNICAST == 1
        struct freertos_addrinfo *_iptcp;
#endif
    };
} _z_sys_net_endpoint_t;

#endif