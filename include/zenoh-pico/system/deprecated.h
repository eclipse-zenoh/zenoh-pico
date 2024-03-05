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

#ifndef ZENOH_PICO_PLATFORM_DEPRECATED_H
#define ZENOH_PICO_PLATFORM_DEPRECATED_H

#include <stdint.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------ Random ------------------*/
uint8_t zp_random_u8(void);
uint16_t zp_random_u16(void);
uint32_t zp_random_u32(void);
uint64_t zp_random_u64(void);
void zp_random_fill(void *buf, size_t len);

/*------------------ Memory ------------------*/
void *zp_malloc(size_t size);
void *zp_realloc(void *ptr, size_t size);
void zp_free(void *ptr);

#if Z_FEATURE_MULTI_THREAD == 1
/*------------------ Thread ------------------*/
typedef z_task_t zp_task_t;
typedef z_task_attr_t zp_task_attr_t;

int8_t zp_task_init(zp_task_t *task, zp_task_attr_t *attr, void *(*fun)(void *), void *arg);
int8_t zp_task_join(zp_task_t *task);
void zp_task_free(zp_task_t **task);

/*------------------ Mutex ------------------*/
typedef z_mutex_t zp_mutex_t;
int8_t zp_mutex_init(zp_mutex_t *m);
int8_t zp_mutex_free(zp_mutex_t *m);

int8_t zp_mutex_lock(zp_mutex_t *m);
int8_t zp_mutex_trylock(zp_mutex_t *m);
int8_t zp_mutex_unlock(zp_mutex_t *m);

/*------------------ CondVar ------------------*/
typedef z_condvar_t zp_condvar_t;
int8_t zp_condvar_init(zp_condvar_t *cv);
int8_t zp_condvar_free(zp_condvar_t *cv);

int8_t zp_condvar_signal(zp_condvar_t *cv);
int8_t zp_condvar_wait(zp_condvar_t *cv, zp_mutex_t *m);
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int zp_sleep_us(size_t time);
int zp_sleep_ms(size_t time);
int zp_sleep_s(size_t time);

/*------------------ Clock ------------------*/
typedef z_clock_t zp_clock_t;
zp_clock_t zp_clock_now(void);
unsigned long zp_clock_elapsed_us(zp_clock_t *time);
unsigned long zp_clock_elapsed_ms(zp_clock_t *time);
unsigned long zp_clock_elapsed_s(zp_clock_t *time);

/*------------------ Time ------------------*/
typedef z_time_t zp_time_t;
zp_time_t zp_time_now(void);
const char *zp_time_now_as_str(char *const buf, unsigned long buflen);
unsigned long zp_time_elapsed_us(zp_time_t *time);
unsigned long zp_time_elapsed_ms(zp_time_t *time);
unsigned long zp_time_elapsed_s(zp_time_t *time);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_DEPRECATED_PLATFORM_H */
