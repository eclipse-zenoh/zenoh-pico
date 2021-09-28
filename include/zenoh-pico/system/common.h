/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef _ZENOH_PICO_SYSTEM_PRIVATE_COMMON_H
#define _ZENOH_PICO_SYSTEM_PRIVATE_COMMON_H

// @TODO: remove the platform-specific include and data types
#include <netinet/in.h>
#include "zenoh-pico/protocol/private/iobuf.h"
#include "zenoh-pico/system/types.h"
#include "zenoh-pico/utils/private/result.h"
#include "zenoh-pico/transport/private/link.h"

/*------------------ Thread ------------------*/
int z_task_init(z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg);

/*------------------ Mutex ------------------*/
int z_mutex_init(z_mutex_t *m);
int z_mutex_free(z_mutex_t *m);

int z_mutex_lock(z_mutex_t *m);
int z_mutex_trylock(z_mutex_t *m);
int z_mutex_unlock(z_mutex_t *m);

/*------------------ CondVar ------------------*/
int z_condvar_init(z_condvar_t *cv);
int z_condvar_free(z_condvar_t *cv);

int z_condvar_signal(z_condvar_t *cv);
int z_condvar_wait(z_condvar_t *cv, z_mutex_t *m);

/*------------------ Sleep ------------------*/
int z_sleep_us(unsigned int time);
int z_sleep_ms(unsigned int time);
int z_sleep_s(unsigned int time);

/*------------------ Clock ------------------*/
z_clock_t z_clock_now(void);
clock_t z_clock_elapsed_us(z_clock_t *time);
clock_t z_clock_elapsed_ms(z_clock_t *time);
clock_t z_clock_elapsed_s(z_clock_t *time);

/*------------------ Time ------------------*/
z_time_t z_time_now(void);
time_t z_time_elapsed_us(z_time_t *time);
time_t z_time_elapsed_ms(z_time_t *time);
time_t z_time_elapsed_s(z_time_t *time);

/*------------------ Network ------------------*/
_ZN_RESULT_DECLARE(_zn_socket_t, socket)

int _zn_send_wbuf(_zn_link_t *link, const _z_wbuf_t *wbf);
int _zn_recv_zbuf(_zn_link_t *link, _z_zbuf_t *zbf);
int _zn_recv_exact_zbuf(_zn_link_t *link, _z_zbuf_t *zbf, size_t len);

char *_zn_select_scout_iface(void);

// TCP
void* _zn_create_tcp_endpoint(const char* s_addr, int port);
_zn_socket_result_t _zn_tcp_open(void* arg);
int _zn_tcp_close(_zn_socket_t sock);
int _zn_tcp_read_exact(_zn_socket_t sock, uint8_t *ptr, size_t len);
int _zn_tcp_read(_zn_socket_t sock, uint8_t *ptr, size_t len);
int _zn_tcp_send(_zn_socket_t sock, const uint8_t *ptr, size_t len);

// UDP
void* _zn_create_udp_endpoint(const char* s_addr, int port);
_zn_socket_result_t _zn_udp_open(void *arg);
int _zn_udp_close(_zn_socket_t sock);
int _zn_udp_read_exact(_zn_socket_t sock, uint8_t *ptr, size_t len);
int _zn_udp_read(_zn_socket_t sock, uint8_t *ptr, size_t len);
int _zn_udp_send(_zn_socket_t sock, const uint8_t *ptr, size_t len, void *arg);

#endif /* _ZENOH_PICO_SYSTEM_PRIVATE_COMMON_H */
