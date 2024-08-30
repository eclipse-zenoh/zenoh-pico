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

#include <FreeRTOS.h>
#include <hw/driver/delay.h>
#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { return random(0xFF); }

uint16_t z_random_u16(void) { return random(0xFFFF); }

uint32_t z_random_u32(void) { return random(0xFFFFFFFF); }

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
    ret |= z_random_u32();
    ret = ret << 32;
    ret |= z_random_u32();

    return ret;
}

void z_random_fill(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ((uint8_t *)buf)[i] = z_random_u8();
    }
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) {
    // return pvPortMalloc(size); // FIXME: Further investigation is required to understand
    //        why pvPortMalloc or pvPortMallocAligned are failing
    return malloc(size);
}

void *z_realloc(void *ptr, size_t size) {
    // Not implemented by the platform
    return NULL;
}

void z_free(void *ptr) {
    // vPortFree(ptr); // FIXME: Further investigation is required to understand
    //        why vPortFree or vPortFreeAligned are failing
    return free(ptr);
}

#if Z_FEATURE_MULTI_THREAD == 1
#error "Multi-threading not supported yet on OpenCR port. Disable it by defining Z_FEATURE_MULTI_THREAD=0"

/*------------------ Task ------------------*/
int8_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) { return -1; }

int8_t _z_task_join(_z_task_t *task) { return -1; }

int8_t _z_task_cancel(_z_task_t *task) { return -1; }

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t _z_mutex_init(_z_mutex_t *m) { return -1; }

int8_t _z_mutex_drop(_z_mutex_t *m) { return -1; }

int8_t _z_mutex_lock(_z_mutex_t *m) { return -1; }

int8_t _z_mutex_try_lock(_z_mutex_t *m) { return -1; }

int8_t _z_mutex_unlock(_z_mutex_t *m) { return -1; }

/*------------------ Condvar ------------------*/
int8_t _z_condvar_init(_z_condvar_t *cv) { return -1; }

int8_t _z_condvar_drop(_z_condvar_t *cv) { return -1; }

int8_t _z_condvar_signal(_z_condvar_t *cv) { return -1; }
int8_t _z_condvar_signal_all(_z_condvar_t *cv) { return -1; }

int8_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) { return -1; }
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time) {
    delay_us(time);
    return 0;
}

int z_sleep_ms(size_t time) {
    delay_ms(time);
    return 0;
}

int z_sleep_s(size_t time) {
    z_time_t start = z_time_now();

    // Most sleep APIs promise to sleep at least whatever you asked them to.
    // This may compound, so this approach may make sleeps longer than expected.
    // This extra check tries to minimize the amount of extra time it might sleep.
    while (z_time_elapsed_s(&start) < time) {
        z_sleep_ms(1000);
    }

    return 0;
}

/*------------------ Instant ------------------*/
void __z_clock_gettime(z_clock_t *ts) {
    uint64_t m = millis();
    ts->tv_sec = m / (uint64_t)1000000;
    ts->tv_nsec = (m % (uint64_t)1000000) * (uint64_t)1000;
}

z_clock_t z_clock_now(void) {
    z_clock_t now;
    __z_clock_gettime(&now);
    return now;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now;
    __z_clock_gettime(&now);

    unsigned long elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now;
    __z_clock_gettime(&now);

    unsigned long elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now;
    __z_clock_gettime(&now);

    unsigned long elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
z_time_t z_time_now(void) {
    z_time_t now;
    gettimeofday(&now, NULL);
    return now;
}

const char *z_time_now_as_str(char *const buf, unsigned long buflen) {
    z_time_t tv = z_time_now();
    struct tm ts;
    ts = *localtime(&tv.tv_sec);
    strftime(buf, buflen, "%Y-%m-%dT%H:%M:%SZ", &ts);
    return buf;
}

unsigned long z_time_elapsed_us(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

unsigned long z_time_elapsed_ms(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

unsigned long z_time_elapsed_s(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}

int8_t zp_get_time_since_epoch(zp_time_since_epoch *t) {
    z_time_t now;
    gettimeofday(&now, NULL);
    t->secs = now.tv_sec;
    t->nanos = now.tv_usec * 1000;
    return 0;
}