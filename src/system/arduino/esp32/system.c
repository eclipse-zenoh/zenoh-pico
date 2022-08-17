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
    ret |= z_random_u32() << 8;

    return ret;
}

void z_random_fill(void *buf, size_t len) { esp_fill_random(buf, len); }

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_8BIT); }

void *z_realloc(void *ptr, size_t size) { return heap_caps_realloc(ptr, size, MALLOC_CAP_8BIT); }

void z_free(void *ptr) { heap_caps_free(ptr); }

#if Z_MULTI_THREAD == 1
// This wrapper is only used for ESP32.
// In FreeRTOS, tasks created using xTaskCreate must end with vTaskDelete.
// A task function should __not__ simply return.
typedef struct {
    void *(*_fun)(void *);
    void *_arg;
} __z_task_arg;

void z_task_wrapper(void *arg) {
    __z_task_arg *z_arg = (__z_task_arg *)arg;
    z_arg->_fun(z_arg->_arg);
    vTaskDelete(NULL);
    z_free(z_arg);
}

/*------------------ Task ------------------*/
int _z_task_init(_z_task_t *task, _z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    __z_task_arg *z_arg = (__z_task_arg *)z_malloc(sizeof(__z_task_arg));
    z_arg->_fun = fun;
    z_arg->_arg = arg;
    if (xTaskCreate(z_task_wrapper, "", 5120, z_arg, 15, task) != pdPASS) return -1;

    return 0;
}

int _z_task_join(_z_task_t *task) {
    // Note: join not supported using FreeRTOS API
    return 0;
}

int _z_task_cancel(_z_task_t *task) {
    vTaskDelete(task);
    return 0;
}

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int _z_mutex_init(_z_mutex_t *m) { return pthread_mutex_init(m, NULL); }

int _z_mutex_free(_z_mutex_t *m) { return pthread_mutex_destroy(m); }

int _z_mutex_lock(_z_mutex_t *m) { return pthread_mutex_lock(m); }

int _z_mutex_trylock(_z_mutex_t *m) { return pthread_mutex_trylock(m); }

int _z_mutex_unlock(_z_mutex_t *m) { return pthread_mutex_unlock(m); }

/*------------------ Condvar ------------------*/
int _z_condvar_init(_z_condvar_t *cv) { return pthread_cond_init(cv, NULL); }

int _z_condvar_free(_z_condvar_t *cv) { return pthread_cond_destroy(cv); }

int _z_condvar_signal(_z_condvar_t *cv) { return pthread_cond_signal(cv); }

int _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) { return pthread_cond_wait(cv, m); }
#endif  // Z_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(unsigned int time) { return usleep(time); }

int z_sleep_ms(unsigned int time) { return usleep(1000 * time); }

int z_sleep_s(unsigned int time) { return sleep(time); }

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) {
    z_clock_t now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_REALTIME, &now);

    unsigned long elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_REALTIME, &now);

    unsigned long elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_REALTIME, &now);

    unsigned long elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
z_time_t z_time_now(void) {
    z_time_t now;
    gettimeofday(&now, NULL);
    return now;
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
