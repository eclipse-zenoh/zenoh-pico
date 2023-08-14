#ifndef ZENOH_PICO_SYSTEM_FREERTOS_PLUS_TCP_TYPES_H
#define ZENOH_PICO_SYSTEM_FREERTOS_PLUS_TCP_TYPES_H

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"

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