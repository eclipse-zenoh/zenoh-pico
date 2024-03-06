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

#include "zenoh-pico/deprecated/platform.h"

#include "zenoh-pico/system/platform-common.h"

uint8_t zp_random_u8(void) { return z_random_u8(); };
uint16_t zp_random_u16(void) { return z_random_u16(); };
uint32_t zp_random_u32(void) { return z_random_u32(); };
uint64_t zp_random_u64(void) { return z_random_u64(); };
void zp_random_fill(void *buf, size_t len) { return z_random_fill(buf, len); };

void *zp_malloc(size_t size) { return z_malloc(size); };
void *zp_realloc(void *ptr, size_t size) { return z_realloc(ptr, size); };
void zp_free(void *ptr) { return z_free(ptr); };

#if Z_FEATURE_MULTI_THREAD == 1

int8_t zp_task_init(zp_task_t *task, zp_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    return z_task_init(task, attr, fun, arg);
};
int8_t zp_task_join(zp_task_t *task) { return z_task_join(task); };
void zp_task_free(zp_task_t **task) { return z_task_free(task); };

int8_t zp_mutex_init(zp_mutex_t *m) { return z_mutex_init(m); };
int8_t zp_mutex_free(zp_mutex_t *m) { return z_mutex_free(m); };
int8_t zp_mutex_lock(zp_mutex_t *m) { return z_mutex_lock(m); };
int8_t zp_mutex_trylock(zp_mutex_t *m) { return z_mutex_trylock(m); };
int8_t zp_mutex_unlock(zp_mutex_t *m) { return z_mutex_unlock(m); };

int8_t zp_condvar_init(zp_condvar_t *cv) { return z_condvar_init(cv); };
int8_t zp_condvar_free(zp_condvar_t *cv) { return z_condvar_free(cv); };

int8_t zp_condvar_signal(zp_condvar_t *cv) { return z_condvar_signal(cv); };
int8_t zp_condvar_wait(zp_condvar_t *cv, zp_mutex_t *m) { return z_condvar_wait(cv, m); };
#endif  // Z_FEATURE_MULTI_THREAD == 1

int zp_sleep_us(size_t time) { return z_sleep_us(time); };
int zp_sleep_ms(size_t time) { return z_sleep_ms(time); };
int zp_sleep_s(size_t time) { return z_sleep_s(time); };

zp_clock_t zp_clock_now(void) { return z_clock_now(); };
unsigned long zp_clock_elapsed_us(zp_clock_t *time) { return z_clock_elapsed_us(time); };
unsigned long zp_clock_elapsed_ms(zp_clock_t *time) { return z_clock_elapsed_ms(time); };
unsigned long zp_clock_elapsed_s(zp_clock_t *time) { return z_clock_elapsed_s(time); };

zp_time_t zp_time_now(void) { return z_time_now(); };
const char *zp_time_now_as_str(char *const buf, unsigned long buflen) { return z_time_now_as_str(buf, buflen); };
unsigned long zp_time_elapsed_us(zp_time_t *time) { return z_time_elapsed_us(time); };
unsigned long zp_time_elapsed_ms(zp_time_t *time) { return z_time_elapsed_ms(time); };
unsigned long zp_time_elapsed_s(zp_time_t *time) { return z_time_elapsed_s(time); };
