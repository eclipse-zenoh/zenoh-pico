/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#ifndef ZENOH_C_SYNC_H
#define ZENOH_C_SYNC_H

#include "zenoh/net/types.h"

/*------------------ Mutex ------------------*/
int _zn_mutex_init(_zn_mutex_t *m);
int _zn_mutex_free(_zn_mutex_t *m);

int _zn_mutex_lock(_zn_mutex_t *m);
int _zn_mutex_trylock(_zn_mutex_t *m);
int _zn_mutex_unlock(_zn_mutex_t *m);

/*------------------ Sleep ------------------*/
int _zn_sleep_us(unsigned int time);
int _zn_sleep_ms(unsigned int time);
int _zn_sleep_s(unsigned int time);

/*------------------ Clock ------------------*/
_zn_clock_t _zn_clock_now(void);
clock_t _zn_clock_elapsed_us(_zn_clock_t *time);
clock_t _zn_clock_elapsed_ms(_zn_clock_t *time);
clock_t _zn_clock_elapsed_s(_zn_clock_t *time);

/*------------------ Time ------------------*/
_zn_time_t _zn_time_now(void);
time_t _zn_time_elapsed_us(_zn_time_t *time);
time_t _zn_time_elapsed_ms(_zn_time_t *time);
time_t _zn_time_elapsed_s(_zn_time_t *time);

#endif /* ZENOH_C_SYNC_H */
