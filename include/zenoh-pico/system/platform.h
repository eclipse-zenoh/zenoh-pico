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

#ifndef ZENOH_PICO_SYSTEM_COMMON_H
#define ZENOH_PICO_SYSTEM_COMMON_H

#if defined(ZENOH_LINUX) || defined(ZENOH_MACOS)
#include "zenoh-pico/system/platform/unix.h"
#elif defined(ZENOH_ZEPHYR)
#include "zenoh-pico/system/platform/zephyr.h"
#elif defined(ZENOH_ESP32)
#include "zenoh-pico/system/platform/esp32.h"
#else
#include "zenoh-pico/system/platform/void.h"
#error "Unknown platform"
#endif

#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/system/link/tcp.h"
#include "zenoh-pico/system/link/udp.h"

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

#endif /* ZENOH_PICO_SYSTEM_COMMON_H */
