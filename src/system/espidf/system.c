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
    void *(*fun)(void *);
    void *arg;
    EventGroupHandle_t join_event;
} z_task_arg;

static void z_task_wrapper(void *arg) {
    z_task_arg *targ = (z_task_arg *)arg;
    targ->fun(targ->arg);
    xEventGroupSetBits(targ->join_event, 1);
    vTaskDelete(NULL);
}

static z_task_attr_t z_default_task_attr = {
    .name = "",
    .priority = configMAX_PRIORITIES / 2,
    .stack_depth = 5120,
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    .static_allocation = false,
    .stack_buffer = NULL,
    .task_buffer = NULL,
#endif /* SUPPORT_STATIC_ALLOCATION */
};

/*------------------ Thread ------------------*/
int8_t z_task_init(z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    z_task_arg *z_arg = (z_task_arg *)z_malloc(sizeof(z_task_arg));
    if (z_arg == NULL) {
        return -1;
    }

    z_arg->fun = fun;
    z_arg->arg = arg;
    z_arg->join_event = task->join_event = xEventGroupCreate();

    if (attr == NULL) {
        attr = &z_default_task_attr;
    }

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    if (attr->static_allocation) {
        task->handle = xTaskCreateStatic(z_task_wrapper, attr->name, attr->stack_depth, z_arg, attr->priority,
                                         attr->stack_buffer, attr->task_buffer);
        if (task->handle == NULL) {
            return -1;
        }
    } else {
#endif /* SUPPORT_STATIC_ALLOCATION */
        if (xTaskCreate(z_task_wrapper, attr->name, attr->stack_depth, z_arg, attr->priority, &task->handle) !=
            pdPASS) {
            return -1;
        }
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    }
#endif /* SUPPORT_STATIC_ALLOCATION */

    return 0;
}

int8_t z_task_join(z_task_t *task) {
    xEventGroupWaitBits(task->join_event, 1, pdFALSE, pdFALSE, portMAX_DELAY);
    return 0;
}

int8_t z_task_cancel(z_task_t *task) {
    vTaskDelete(task->handle);
    return 0;
}

void z_task_free(z_task_t **task) {
    z_free((*task)->join_event);
    z_free(*task);
}

/*------------------ Mutex ------------------*/
int8_t z_mutex_init(z_mutex_t *m) { return pthread_mutex_init(m, NULL); }

int8_t z_mutex_free(z_mutex_t *m) { return pthread_mutex_destroy(m); }

int8_t z_mutex_lock(z_mutex_t *m) { return pthread_mutex_lock(m); }

int8_t z_mutex_trylock(z_mutex_t *m) { return pthread_mutex_trylock(m); }

int8_t z_mutex_unlock(z_mutex_t *m) { return pthread_mutex_unlock(m); }

/*------------------ Condvar ------------------*/
int8_t z_condvar_init(z_condvar_t *cv) { return pthread_cond_init(cv, NULL); }

int8_t z_condvar_free(z_condvar_t *cv) { return pthread_cond_destroy(cv); }

int8_t z_condvar_signal(z_condvar_t *cv) { return pthread_cond_signal(cv); }

int8_t z_condvar_wait(z_condvar_t *cv, z_mutex_t *m) { return pthread_cond_wait(cv, m); }
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time) { return usleep(time); }

int z_sleep_ms(size_t time) {
    z_time_t start = z_time_now();

    // Most sleep APIs promise to sleep at least whatever you asked them to.
    // This may compound, so this approach may make sleeps longer than expected.
    // This extra check tries to minimize the amount of extra time it might sleep.
    while (z_time_elapsed_ms(&start) < time) {
        // z_sleep_us(1000);
        vTaskDelay(1 / portTICK_PERIOD_MS);
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
