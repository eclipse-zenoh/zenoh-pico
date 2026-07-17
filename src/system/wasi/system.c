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
#include <unistd.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/common/system_error.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Random ------------------*/
// wasi-libc provides arc4random via wasi:random
uint8_t z_random_u8(void) { return (uint8_t)arc4random(); }

uint16_t z_random_u16(void) { return (uint16_t)arc4random(); }

uint32_t z_random_u32(void) { return arc4random(); }

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
    ret |= z_random_u32();
    ret = ret << 32;
    ret |= z_random_u32();
    return ret;
}

void z_random_fill(void *buf, size_t len) { arc4random_buf(buf, len); }

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) { return malloc(size); }

void *z_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void z_free(void *ptr) { free(ptr); }

// Threading is not supported on WASI (Z_FEATURE_MULTI_THREAD must be 0).
// Dummy types are provided by platform.h.

/*------------------ Sleep ------------------*/
z_result_t z_sleep_us(size_t time) { _Z_CHECK_SYS_ERR(usleep((unsigned int)time)); }

z_result_t z_sleep_ms(size_t time) {
    z_time_t start = z_time_now();

    while (z_time_elapsed_ms(&start) < time) {
        z_result_t ret = z_sleep_us(1000);
        if (ret != _Z_RES_OK) {
            return ret;
        }
    }

    return _Z_RES_OK;
}

z_result_t z_sleep_s(size_t time) { _Z_CHECK_SYS_ERR((int)sleep((unsigned int)time)); }

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

unsigned long zp_clock_elapsed_us_since(z_clock_t *instant, z_clock_t *epoch) {
    long elapsed = (1000000 * (instant->tv_sec - epoch->tv_sec) + (instant->tv_nsec - epoch->tv_nsec) / 1000);
    return elapsed > 0 ? (unsigned long)elapsed : 0;
}

unsigned long zp_clock_elapsed_ms_since(z_clock_t *instant, z_clock_t *epoch) {
    long elapsed = (1000 * (instant->tv_sec - epoch->tv_sec) + (instant->tv_nsec - epoch->tv_nsec) / 1000000);
    return elapsed > 0 ? (unsigned long)elapsed : 0;
}

unsigned long zp_clock_elapsed_s_since(z_clock_t *instant, z_clock_t *epoch) {
    long elapsed = (instant->tv_sec - epoch->tv_sec);
    return elapsed > 0 ? (unsigned long)elapsed : 0;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return zp_clock_elapsed_us_since(&now, instant);
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return zp_clock_elapsed_ms_since(&now, instant);
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return zp_clock_elapsed_s_since(&now, instant);
}

void z_clock_advance_us(z_clock_t *clock, unsigned long duration) {
    clock->tv_sec += (time_t)(duration / 1000000);
    clock->tv_nsec += (long int)((duration % 1000000) * 1000);

    if (clock->tv_nsec >= 1000000000) {
        clock->tv_sec += 1;
        clock->tv_nsec -= 1000000000;
    }
}

void z_clock_advance_ms(z_clock_t *clock, unsigned long duration) {
    clock->tv_sec += (time_t)(duration / 1000);
    clock->tv_nsec += (long int)((duration % 1000) * 1000000);

    if (clock->tv_nsec >= 1000000000) {
        clock->tv_sec += 1;
        clock->tv_nsec -= 1000000000;
    }
}

void z_clock_advance_s(z_clock_t *clock, unsigned long duration) { clock->tv_sec += (time_t)duration; }

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

z_result_t _z_get_time_since_epoch(_z_time_since_epoch *t) {
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
        return _Z_ERR_SYSTEM_GENERIC;
    }
    t->secs = (uint32_t)now.tv_sec;
    t->nanos = (uint32_t)now.tv_nsec;
    return _Z_RES_OK;
}
