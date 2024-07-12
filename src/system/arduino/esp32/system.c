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
#include <stddef.h>
#include <sys/time.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { return z_random_u32(); }

uint16_t z_random_u16(void) { return z_random_u32(); }

uint32_t z_random_u32(void) { return esp_random(); }

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
    ret |= z_random_u32();
    ret = ret << 32;
    ret |= z_random_u32();

    return ret;
}

void z_random_fill(void *buf, size_t len) { esp_fill_random(buf, len); }

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_8BIT); }

void *z_realloc(void *ptr, size_t size) { return heap_caps_realloc(ptr, size, MALLOC_CAP_8BIT); }

void z_free(void *ptr) { heap_caps_free(ptr); }

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
    z_free(targ);
}

/*------------------ Task ------------------*/
int8_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    int ret = 0;

    z_task_arg *z_arg = (z_task_arg *)z_malloc(sizeof(z_task_arg));
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

int8_t _z_task_join(_z_task_t *task) {
    // Note: task/thread join not supported on FreeRTOS API, so we force its deletion instead.
    return _z_task_cancel(task);
}

int8_t _z_task_cancel(_z_task_t *task) {
    vTaskDelete(*task);
    return 0;
}

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t _z_mutex_init(_z_mutex_t *m) { return pthread_mutex_init(m, NULL); }

int8_t _z_mutex_drop(_z_mutex_t *m) { return pthread_mutex_destroy(m); }

int8_t _z_mutex_lock(_z_mutex_t *m) { return pthread_mutex_lock(m); }

int8_t _z_mutex_try_lock(_z_mutex_t *m) { return pthread_mutex_trylock(m); }

int8_t _z_mutex_unlock(_z_mutex_t *m) { return pthread_mutex_unlock(m); }

/*------------------ Condvar ------------------*/
int8_t _z_condvar_init(_z_condvar_t *cv) { return pthread_cond_init(cv, NULL); }

int8_t _z_condvar_free(_z_condvar_t *cv) { return pthread_cond_destroy(cv); }

int8_t _z_condvar_signal(_z_condvar_t *cv) { return pthread_cond_signal(cv); }

int8_t _z_condvar_signal_all(_z_condvar_t *cv) { return pthread_cond_broadcast(cv); }

int8_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) { return pthread_cond_wait(cv, m); }
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time) { return usleep(time); }

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

int z_sleep_s(size_t time) { return sleep(time); }

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

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