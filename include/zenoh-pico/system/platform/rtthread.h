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

#ifndef ZENOH_PICO_SYSTEM_RTTHREAD_TYPES_H
#define ZENOH_PICO_SYSTEM_RTTHREAD_TYPES_H

#include <rtthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>

#include "zenoh-pico/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_MULTI_THREAD == 1

typedef struct {
    rt_thread_t handle;
    void *(*fun)(void *);
    void *arg;
    rt_event_t join_event;
    const char *name;
} _z_task_t;

typedef struct {
    const char *name;
    rt_uint8_t priority;
    rt_uint32_t stack_size;
} z_task_attr_t;

typedef struct {
    rt_mutex_t handle;
} _z_mutex_t;

typedef _z_mutex_t _z_mutex_rec_t;

typedef struct {
    rt_mutex_t mutex;
    rt_sem_t sem;
    int waiters;
} _z_condvar_t;

#endif  // Z_FEATURE_MULTI_THREAD == 1

typedef rt_tick_t z_clock_t;
typedef struct timeval z_time_t;

typedef struct {
    int _socket;
} _z_sys_net_socket_t;

typedef struct {
    struct addrinfo *_iptcp;
} _z_sys_net_endpoint_t;

typedef struct {
    int _socket;
    struct sockaddr_in _addr;
} _z_sys_net_socket_udp_t;

typedef struct {
    struct sockaddr_storage _sockaddr;
    socklen_t _socklen;
} _z_sys_net_endpoint_udp_t;

#if Z_FEATURE_LINK_SERIAL == 1
typedef struct {
    int _fd;
} _z_sys_net_socket_serial_t;

typedef struct {
    char *_device;
    int _baudrate;
} _z_sys_net_endpoint_serial_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_SYSTEM_RTTHREAD_TYPES_H */