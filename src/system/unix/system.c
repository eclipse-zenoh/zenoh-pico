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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(ZENOH_LINUX)
#include <sys/random.h>
#include <sys/time.h>
#endif

#include <unistd.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) {
    uint8_t ret = 0;
#if defined(ZENOH_LINUX)
    while (getrandom(&ret, sizeof(uint8_t), 0) <= 0) {
        ZP_ASM_NOP;
    }
#elif defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    ret = z_random_u32();
#endif

    return ret;
}

uint16_t z_random_u16(void) {
    uint16_t ret = 0;
#if defined(ZENOH_LINUX)
    while (getrandom(&ret, sizeof(uint16_t), 0) <= 0) {
        ZP_ASM_NOP;
    }
#elif defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    ret = z_random_u32();
#endif

    return ret;
}

uint32_t z_random_u32(void) {
    uint32_t ret = 0;
#if defined(ZENOH_LINUX)
    while (getrandom(&ret, sizeof(uint32_t), 0) <= 0) {
        ZP_ASM_NOP;
    }
#elif defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    ret = arc4random();
#endif

    return ret;
}

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
#if defined(ZENOH_LINUX)
    while (getrandom(&ret, sizeof(uint64_t), 0) <= 0) {
        ZP_ASM_NOP;
    }
#elif defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    ret |= z_random_u32();
    ret = ret << 32;
    ret |= z_random_u32();
#endif

    return ret;
}

void z_random_fill(void *buf, size_t len) {
#if defined(ZENOH_LINUX)
    while (getrandom(buf, len, 0) <= 0) {
        ZP_ASM_NOP;
    }
#elif defined(ZENOH_MACOS) || defined(ZENOH_BSD)
    arc4random_buf(buf, len);
#endif
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) { return malloc(size); }

void *z_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void z_free(void *ptr) { free(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1
/*------------------ Task ------------------*/
int8_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    return (int8_t)pthread_create(task, attr, fun, arg);
}

int8_t _z_task_join(_z_task_t *task) { return (int8_t)pthread_join(*task, NULL); }

int8_t _z_task_cancel(_z_task_t *task) { return (int8_t)pthread_cancel(*task); }

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t _z_mutex_init(_z_mutex_t *m) { return (int8_t)pthread_mutex_init(m, 0); }

int8_t _z_mutex_drop(_z_mutex_t *m) { return (int8_t)pthread_mutex_destroy(m); }

int8_t _z_mutex_lock(_z_mutex_t *m) { return (int8_t)pthread_mutex_lock(m); }

int8_t _z_mutex_try_lock(_z_mutex_t *m) { return (int8_t)pthread_mutex_trylock(m); }

int8_t _z_mutex_unlock(_z_mutex_t *m) { return (int8_t)pthread_mutex_unlock(m); }

/*------------------ Condvar ------------------*/
int8_t _z_condvar_init(_z_condvar_t *cv) { return (int8_t)pthread_cond_init(cv, 0); }

int8_t _z_condvar_drop(_z_condvar_t *cv) { return (int8_t)pthread_cond_destroy(cv); }

int8_t _z_condvar_signal(_z_condvar_t *cv) { return (int8_t)pthread_cond_signal(cv); }

int8_t _z_condvar_signal_all(_z_condvar_t *cv) { return (int8_t)pthread_cond_broadcast(cv); }

int8_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) { return (int8_t)pthread_cond_wait(cv, m); }
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time) { return usleep((unsigned int)time); }

int z_sleep_ms(size_t time) {
    z_time_t start = z_time_now();

    // Most sleep APIs promise to sleep at least whatever you asked them to.
    // This may compound, so this approach may make sleeps longer than expected.
    // This extra check tries to minimize the amount of extra time it might sleep.
    while (z_time_elapsed_ms(&start) < time) {
        z_sleep_us(1000);
    }

    return 0;
}

int z_sleep_s(size_t time) { return (int)sleep((unsigned int)time); }

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed =
        (unsigned long)(1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed =
        (unsigned long)(1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed = (unsigned long)(now.tv_sec - instant->tv_sec);
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

    unsigned long elapsed = (unsigned long)(1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

unsigned long z_time_elapsed_ms(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (unsigned long)(1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

unsigned long z_time_elapsed_s(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (unsigned long)(now.tv_sec - time->tv_sec);
    return elapsed;
}

int8_t zp_get_time_since_epoch(zp_time_since_epoch *t) {
    z_time_t now;
    gettimeofday(&now, NULL);
    t->secs = (uint32_t)now.tv_sec;
    t->nanos = (uint32_t)now.tv_usec * 1000;
    return 0;
}
