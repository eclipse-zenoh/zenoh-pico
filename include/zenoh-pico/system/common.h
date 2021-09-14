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

char *_zn_select_scout_iface(void);
_zn_socket_result_t _zn_open_tx_session(const char *locator);
void _zn_close_tx_session(_zn_socket_t sock);

struct sockaddr_in *_zn_make_socket_address(const char *addr, int port);
_zn_socket_result_t _zn_create_udp_socket(const char *addr, int port, int recv_timeout);

int _zn_send_dgram_to(_zn_socket_t sock, const _z_wbuf_t *wbf, const struct sockaddr *dest, socklen_t salen);
int _zn_recv_dgram_from(_zn_socket_t sock, _z_zbuf_t *zbf, struct sockaddr *from, socklen_t *salen);

int _zn_send_wbuf(_zn_socket_t sock, const _z_wbuf_t *wbf);
int _zn_recv_zbuf(_zn_socket_t sock, _z_zbuf_t *zbf);
int _zn_recv_bytes(_zn_socket_t sock, uint8_t *buf, size_t len);

#endif /* _ZENOH_PICO_SYSTEM_PRIVATE_COMMON_H */
