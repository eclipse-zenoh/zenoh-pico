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

#include <esp_heap_caps.h>
#include <esp_random.h>
#include <stddef.h>
#include <sys/time.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t zp_random_u8(void) { return zp_random_u32(); }

uint16_t zp_random_u16(void) { return zp_random_u32(); }

uint32_t zp_random_u32(void) { return esp_random(); }

uint64_t zp_random_u64(void) {
    uint64_t ret = 0;
    ret |= zp_random_u32();
    ret = ret << 32;
    ret |= zp_random_u32();

    return ret;
}

void zp_random_fill(void *buf, size_t len) { esp_fill_random(buf, len); }

/*------------------ Memory ------------------*/
void *zp_malloc(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_8BIT); }

void *zp_realloc(void *ptr, size_t size) { return heap_caps_realloc(ptr, size, MALLOC_CAP_8BIT); }

void zp_free(void *ptr) { heap_caps_free(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1
// This wrapper is only used for ESP32.
// In FreeRTOS, tasks created using xTaskCreate must end with vTaskDelete.
// A task function should __not__ simply return.
typedef struct {
    void *(*_fun)(void *);
    void *_arg;
} z_task_arg;

void z_task_wrapper(z_task_arg *targ) {
    targ->_fun(targ->_arg);
    vTaskDelete(NULL);
    zp_free(targ);
}

/*------------------ Task ------------------*/
int8_t zp_task_init(zp_task_t *task, zp_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    int ret = 0;

    z_task_arg *z_arg = (z_task_arg *)zp_malloc(sizeof(z_task_arg));
    if (z_arg != NULL) {
        z_arg->_fun = fun;
        z_arg->_arg = arg;
        if (xTaskCreate((void *)z_task_wrapper, "", 5120, z_arg, configMAX_PRIORITIES / 2, task) != pdPASS) {
            ret = -1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

int8_t zp_task_join(zp_task_t *task) {
    // Note: task/thread join not supported on FreeRTOS API, so we force its deletion instead.
    return zp_task_cancel(task);
}

int8_t zp_task_cancel(zp_task_t *task) {
    vTaskDelete(*task);
    return 0;
}

void zp_task_free(zp_task_t **task) {
    zp_task_t *ptr = *task;
    zp_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t zp_mutex_init(zp_mutex_t *m) { return pthread_mutex_init(m, NULL); }

int8_t zp_mutex_free(zp_mutex_t *m) { return pthread_mutex_destroy(m); }

int8_t zp_mutex_lock(zp_mutex_t *m) { return pthread_mutex_lock(m); }

int8_t zp_mutex_trylock(zp_mutex_t *m) { return pthread_mutex_trylock(m); }

int8_t zp_mutex_unlock(zp_mutex_t *m) { return pthread_mutex_unlock(m); }

/*------------------ Condvar ------------------*/
int8_t zp_condvar_init(zp_condvar_t *cv) { return pthread_cond_init(cv, NULL); }

int8_t zp_condvar_free(zp_condvar_t *cv) { return pthread_cond_destroy(cv); }

int8_t zp_condvar_signal(zp_condvar_t *cv) { return pthread_cond_signal(cv); }

int8_t zp_condvar_wait(zp_condvar_t *cv, zp_mutex_t *m) { return pthread_cond_wait(cv, m); }
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int zp_sleep_us(size_t time) { return usleep(time); }

int zp_sleep_ms(size_t time) {
    zp_time_t start = zp_time_now();

    // Most sleep APIs promise to sleep at least whatever you asked them to.
    // This may compound, so this approach may make sleeps longer than expected.
    // This extra check tries to minimize the amount of extra time it might sleep.
    while (zp_time_elapsed_ms(&start) < time) {
        zp_sleep_us(1000);
    }

    return 0;
}

int zp_sleep_s(size_t time) { return sleep(time); }

/*------------------ Instant ------------------*/
zp_clock_t zp_clock_now(void) {
    zp_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

unsigned long zp_clock_elapsed_us(zp_clock_t *instant) {
    zp_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long zp_clock_elapsed_ms(zp_clock_t *instant) {
    zp_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long zp_clock_elapsed_s(zp_clock_t *instant) {
    zp_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

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
