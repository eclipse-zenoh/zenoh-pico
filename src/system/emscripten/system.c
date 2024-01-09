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
//

#include <emscripten/emscripten.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t zp_random_u8(void) { return (emscripten_random() * 255.0); }

uint16_t zp_random_u16(void) { return (emscripten_random() * 65535.0); }

uint32_t zp_random_u32(void) { return (emscripten_random() * 4294967295.0); }

uint64_t zp_random_u64(void) { return (emscripten_random() * 18446744073709551615.0); }

void zp_random_fill(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        *((uint8_t *)buf) = zp_random_u8();
    }
}

/*------------------ Memory ------------------*/
void *zp_malloc(size_t size) { return malloc(size); }

void *zp_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void zp_free(void *ptr) { free(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1
/*------------------ Task ------------------*/
int8_t zp_task_init(zp_task_t *task, pthread_attr_t *attr, void *(*fun)(void *), void *arg) {
    return pthread_create(task, attr, fun, arg);
}

int8_t zp_task_join(zp_task_t *task) { return pthread_join(*task, NULL); }

int8_t zp_task_cancel(zp_task_t *task) { return pthread_cancel(*task); }

void zp_task_free(zp_task_t **task) {
    zp_task_t *ptr = *task;
    zp_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t zp_mutex_init(zp_mutex_t *m) { return pthread_mutex_init(m, 0); }

int8_t zp_mutex_free(zp_mutex_t *m) { return pthread_mutex_destroy(m); }

int8_t zp_mutex_lock(zp_mutex_t *m) { return pthread_mutex_lock(m); }

int8_t zp_mutex_trylock(zp_mutex_t *m) { return pthread_mutex_trylock(m); }

int8_t zp_mutex_unlock(zp_mutex_t *m) { return pthread_mutex_unlock(m); }

/*------------------ Condvar ------------------*/
int8_t zp_condvar_init(zp_condvar_t *cv) { return pthread_cond_init(cv, 0); }

int8_t zp_condvar_free(zp_condvar_t *cv) { return pthread_cond_destroy(cv); }

int8_t zp_condvar_signal(zp_condvar_t *cv) { return pthread_cond_signal(cv); }

int8_t zp_condvar_wait(zp_condvar_t *cv, zp_mutex_t *m) { return pthread_cond_wait(cv, m); }
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int zp_sleep_us(size_t time) {
    emscripten_sleep((time / 1000) + (time % 1000 == 0 ? 0 : 1));
    return 0;
}

int zp_sleep_ms(size_t time) {
    emscripten_sleep(time);
    return 0;
}

int zp_sleep_s(size_t time) {
    zp_time_t start = zp_time_now();

    // Most sleep APIs promise to sleep at least whatever you asked them to.
    // This may compound, so this approach may make sleeps longer than expected.
    // This extra check tries to minimize the amount of extra time it might sleep.
    while (zp_time_elapsed_s(&start) < time) {
        zp_sleep_ms(1000);
    }

    return 0;
}

/*------------------ Instant ------------------*/
zp_clock_t zp_clock_now(void) { return zp_time_now(); }

unsigned long zp_clock_elapsed_us(zp_clock_t *instant) { return zp_clock_elapsed_ms(instant) * 1000; }

unsigned long zp_clock_elapsed_ms(zp_clock_t *instant) { return zp_time_elapsed_ms(instant); }

unsigned long zp_clock_elapsed_s(zp_clock_t *instant) { return zp_time_elapsed_ms(instant) * 1000; }

/*------------------ Time ------------------*/
zp_time_t zp_time_now(void) { return emscripten_get_now(); }

const char *zp_time_now_as_str(char *const buf, unsigned long buflen) {
    snprintf(buf, buflen, "%f", zp_time_now());
    return buf;
}

unsigned long zp_time_elapsed_us(zp_time_t *time) { return zp_time_elapsed_ms(time) * 1000; }

unsigned long zp_time_elapsed_ms(zp_time_t *time) {
    zp_time_t now = emscripten_get_now();

    unsigned long elapsed = now - *time;
    return elapsed;
}

unsigned long zp_time_elapsed_s(zp_time_t *time) { return zp_time_elapsed_ms(time) * 1000; }
