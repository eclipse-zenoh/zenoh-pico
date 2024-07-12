//
// Copyright (c) 2022 ZettaScale Technology
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
//   Błażej Sowa, <blazej@fictionlab.pl>

#ifndef ZENOH_PICO_SYSTEM_PLATFORM_COMMON_H
#define ZENOH_PICO_SYSTEM_PLATFORM_COMMON_H

#include <stdint.h>

#include "zenoh-pico/api/olv_macros.h"
#include "zenoh-pico/config.h"

#if defined(ZENOH_LINUX) || defined(ZENOH_MACOS) || defined(ZENOH_BSD)
#include "zenoh-pico/system/platform/unix.h"
#elif defined(ZENOH_WINDOWS)
#include "zenoh-pico/system/platform/windows.h"
#elif defined(ZENOH_ESPIDF)
#include "zenoh-pico/system/platform/espidf.h"
#elif defined(ZENOH_MBED)
#include "zenoh-pico/system/platform/mbed.h"
#elif defined(ZENOH_ZEPHYR)
#include "zenoh-pico/system/platform/zephyr.h"
#elif defined(ZENOH_ARDUINO_ESP32)
#include "zenoh-pico/system/platform/arduino/esp32.h"
#elif defined(ZENOH_ARDUINO_OPENCR)
#include "zenoh-pico/system/platform/arduino/opencr.h"
#elif defined(ZENOH_EMSCRIPTEN)
#include "zenoh-pico/system/platform/emscripten.h"
#elif defined(ZENOH_FLIPPER)
#include "zenoh-pico/system/platform/flipper.h"
#elif defined(ZENOH_FREERTOS_PLUS_TCP)
#include "zenoh-pico/system/platform/freertos_plus_tcp.h"
#else
#include "zenoh-pico/system/platform/void.h"
#error "Unknown platform"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*------------------ Random ------------------*/
uint8_t z_random_u8(void);
uint16_t z_random_u16(void);
uint32_t z_random_u32(void);
uint64_t z_random_u64(void);
void z_random_fill(void *buf, size_t len);

/*------------------ Memory ------------------*/
void *z_malloc(size_t size);
void *z_realloc(void *ptr, size_t size);
void z_free(void *ptr);

#if Z_FEATURE_MULTI_THREAD == 0
// dummy types for correct macros work
typedef void *_z_task_t;
typedef void *_z_mutex_t;
typedef void *_z_condvar_t;
typedef void *z_task_attr_t;
#endif

/*------------------ Thread ------------------*/
int8_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg);
int8_t _z_task_join(_z_task_t *task);
int8_t _z_task_cancel(_z_task_t *task);
void _z_task_free(_z_task_t **task);

_Z_OWNED_TYPE_VALUE(_z_task_t, task)
_Z_LOANED_TYPE(_z_task_t, task)
_Z_OWNED_FUNCTIONS_SYSTEM_DEF(task)

int8_t z_task_init(z_owned_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg);
int8_t z_task_join(z_owned_task_t *task);

/*------------------ Mutex ------------------*/
int8_t _z_mutex_init(_z_mutex_t *m);
int8_t _z_mutex_drop(_z_mutex_t *m);

int8_t _z_mutex_lock(_z_mutex_t *m);
int8_t _z_mutex_try_lock(_z_mutex_t *m);
int8_t _z_mutex_unlock(_z_mutex_t *m);

_Z_OWNED_TYPE_VALUE(_z_mutex_t, mutex)
_Z_LOANED_TYPE(_z_mutex_t, mutex)
_Z_OWNED_FUNCTIONS_SYSTEM_DEF(mutex)

int8_t z_mutex_init(z_owned_mutex_t *m);
int8_t z_mutex_drop(z_owned_mutex_t *m);

int8_t z_mutex_lock(z_loaned_mutex_t *m);
int8_t z_mutex_try_lock(z_loaned_mutex_t *m);
int8_t z_mutex_unlock(z_loaned_mutex_t *m);

/*------------------ CondVar ------------------*/
int8_t _z_condvar_init(_z_condvar_t *cv);
int8_t _z_condvar_drop(_z_condvar_t *cv);

int8_t _z_condvar_signal(_z_condvar_t *cv);
int8_t _z_condvar_signal_all(_z_condvar_t *cv);
int8_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m);

_Z_OWNED_TYPE_VALUE(_z_condvar_t, condvar)
_Z_LOANED_TYPE(_z_condvar_t, condvar)
_Z_OWNED_FUNCTIONS_SYSTEM_DEF(condvar)

int8_t z_condvar_init(z_owned_condvar_t *cv);
int8_t z_condvar_drop(z_owned_condvar_t *cv);

int8_t z_condvar_signal(z_loaned_condvar_t *cv);
int8_t z_condvar_wait(z_loaned_condvar_t *cv, z_loaned_mutex_t *m);

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time);
int z_sleep_ms(size_t time);
int z_sleep_s(size_t time);

/*------------------ Clock ------------------*/
z_clock_t z_clock_now(void);
unsigned long z_clock_elapsed_us(z_clock_t *time);
unsigned long z_clock_elapsed_ms(z_clock_t *time);
unsigned long z_clock_elapsed_s(z_clock_t *time);

/*------------------ Time ------------------*/
z_time_t z_time_now(void);
const char *z_time_now_as_str(char *const buf, unsigned long buflen);
unsigned long z_time_elapsed_us(z_time_t *time);
unsigned long z_time_elapsed_ms(z_time_t *time);
unsigned long z_time_elapsed_s(z_time_t *time);

typedef struct {
    uint32_t secs;
    uint32_t nanos;
} zp_time_since_epoch;

int8_t zp_get_time_since_epoch(zp_time_since_epoch *t);
#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_SYSTEM_PLATFORM_COMMON_H */
