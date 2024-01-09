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
uint8_t zp_random_u8(void) { return random(0xFF); }

uint16_t zp_random_u16(void) { return random(0xFFFF); }

uint32_t zp_random_u32(void) { return random(0xFFFFFFFF); }

uint64_t zp_random_u64(void) {
    uint64_t ret = 0;
    ret |= zp_random_u32();
    ret = ret << 32;
    ret |= zp_random_u32();

    return ret;
}

void zp_random_fill(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ((uint8_t *)buf)[i] = zp_random_u8();
    }
}

/*------------------ Memory ------------------*/
void *zp_malloc(size_t size) {
    // return pvPortMalloc(size); // FIXME: Further investigation is required to understand
    //        why pvPortMalloc or pvPortMallocAligned are failing
    return malloc(size);
}

void *zp_realloc(void *ptr, size_t size) {
    // Not implemented by the platform
    return NULL;
}

void zp_free(void *ptr) {
    // vPortFree(ptr); // FIXME: Further investigation is required to understand
    //        why vPortFree or vPortFreeAligned are failing
    return free(ptr);
}

#if Z_FEATURE_MULTI_THREAD == 1
#error "Multi-threading not supported yet on OpenCR port. Disable it by defining Z_FEATURE_MULTI_THREAD=0"

/*------------------ Task ------------------*/
int8_t zp_task_init(zp_task_t *task, zp_task_attr_t *attr, void *(*fun)(void *), void *arg) { return -1; }

int8_t zp_task_join(zp_task_t *task) { return -1; }

int8_t zp_task_cancel(zp_task_t *task) { return -1; }

void zp_task_free(zp_task_t **task) {
    zp_task_t *ptr = *task;
    zp_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t zp_mutex_init(zp_mutex_t *m) { return -1; }

int8_t zp_mutex_free(zp_mutex_t *m) { return -1; }

int8_t zp_mutex_lock(zp_mutex_t *m) { return -1; }

int8_t zp_mutex_trylock(zp_mutex_t *m) { return -1; }

int8_t zp_mutex_unlock(zp_mutex_t *m) { return -1; }

/*------------------ Condvar ------------------*/
int8_t zp_condvar_init(zp_condvar_t *cv) { return -1; }

int8_t zp_condvar_free(zp_condvar_t *cv) { return -1; }

int8_t zp_condvar_signal(zp_condvar_t *cv) { return -1; }

int8_t zp_condvar_wait(zp_condvar_t *cv, zp_mutex_t *m) { return -1; }
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int zp_sleep_us(size_t time) {
    delay_us(time);
    return 0;
}

int zp_sleep_ms(size_t time) {
    delay_ms(time);
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
void __zp_clock_gettime(zp_clock_t *ts) {
    uint64_t m = millis();
    ts->tv_sec = m / (uint64_t)1000000;
    ts->tv_nsec = (m % (uint64_t)1000000) * (uint64_t)1000;
}

zp_clock_t zp_clock_now(void) {
    zp_clock_t now;
    __zp_clock_gettime(&now);
    return now;
}

unsigned long zp_clock_elapsed_us(zp_clock_t *instant) {
    zp_clock_t now;
    __zp_clock_gettime(&now);

    unsigned long elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long zp_clock_elapsed_ms(zp_clock_t *instant) {
    zp_clock_t now;
    __zp_clock_gettime(&now);

    unsigned long elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long zp_clock_elapsed_s(zp_clock_t *instant) {
    zp_clock_t now;
    __zp_clock_gettime(&now);

    unsigned long elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
zp_time_t zp_time_now(void) {
    zp_time_t now;
    gettimeofday(&now, NULL);
    return now;
}

const char *zp_time_now_as_str(char *const buf, unsigned long buflen) {
    zp_time_t tv = zp_time_now();
    struct tm ts;
    ts = *localtime(&tv.tv_sec);
    strftime(buf, buflen, "%Y-%m-%dT%H:%M:%SZ", &ts);
    return buf;
}

unsigned long zp_time_elapsed_us(zp_time_t *time) {
    zp_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

unsigned long zp_time_elapsed_ms(zp_time_t *time) {
    zp_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

unsigned long zp_time_elapsed_s(zp_time_t *time) {
    zp_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}
