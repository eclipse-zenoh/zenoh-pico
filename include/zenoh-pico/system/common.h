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

#include "zenoh-pico/protocol/private/iobuf.h"
#include "zenoh-pico/system/types.h"
#include "zenoh-pico/system/result.h"
#include "zenoh-pico/link/types.h"
#include "zenoh-pico/utils/private/result.h"

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
int _zn_send_wbuf(_zn_link_t *link, const _z_wbuf_t *wbf);
int _zn_recv_zbuf(_zn_link_t *link, _z_zbuf_t *zbf);
int _zn_recv_exact_zbuf(_zn_link_t *link, _z_zbuf_t *zbf, size_t len);

char *_zn_select_scout_iface(void);

// TCP
void* _zn_create_endpoint_tcp(const char *s_addr, const char *port);
void _zn_release_endpoint_tcp(void *arg);
int _zn_open_tcp(void *arg);
int _zn_listen_tcp(void *arg);
void _zn_close_tcp(int sock);
size_t _zn_read_exact_tcp(int sock, uint8_t *ptr, size_t len);
size_t _zn_read_tcp(int sock, uint8_t *ptr, size_t len);
size_t _zn_send_tcp(int sock, const uint8_t *ptr, size_t len);

// Generic UDP
void* _zn_create_endpoint_udp(const char *s_addr, const char *port);
void _zn_release_endpoint_udp(void *arg);
// Unicast UDP
int _zn_open_udp_unicast(void *arg, const clock_t tout);
int _zn_listen_udp_unicast(void *arg, const clock_t tout);
void _zn_close_udp_unicast(int sock);
size_t _zn_read_exact_udp_unicast(int sock, uint8_t *ptr, size_t len);
size_t _zn_read_udp_unicast(int sock, uint8_t *ptr, size_t len);
size_t _zn_send_udp_unicast(int sock, const uint8_t *ptr, size_t len, void *arg);
// Multicast UDP
int _zn_open_udp_multicast(void *arg_1, void **arg_2, const clock_t tout, const char *iface);
int _zn_listen_udp_multicast(void *arg, const clock_t tout, const char *iface);
void _zn_close_udp_multicast(int sock, void *arg);
size_t _zn_read_exact_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg);
size_t _zn_read_udp_multicast(int sock, uint8_t *ptr, size_t len, void *arg);
size_t _zn_send_udp_multicast(int sock, const uint8_t *ptr, size_t len, void *arg);

#endif /* _ZENOH_PICO_SYSTEM_PRIVATE_COMMON_H */
