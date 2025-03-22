//
// Copyright (c) 2024 ZettaScale Technology
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

#include <limits.h>
#include <pico/rand.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "FreeRTOS.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { return (uint8_t)get_rand_32(); }

uint16_t z_random_u16(void) { return (uint16_t)get_rand_32(); }

uint32_t z_random_u32(void) { return get_rand_32(); }

uint64_t z_random_u64(void) { return get_rand_64(); }

void z_random_fill(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        *((uint8_t *)buf) = z_random_u8();
    }
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    return pvPortMalloc(size);
}

void *z_realloc(void *ptr, size_t size) {
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(size);
    // realloc not implemented in FreeRTOS
    assert(false);
    return NULL;
}

void z_free(void *ptr) { vPortFree(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1
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
    .stack_depth = 1024,
};

/*------------------ Thread ------------------*/
z_result_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
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

    if (xTaskCreate(z_task_wrapper, attr->name, attr->stack_depth, z_arg, attr->priority, &task->handle) != pdPASS) {
        return -1;
    }

    return 0;
}

z_result_t _z_task_join(_z_task_t *task) {
    xEventGroupWaitBits(task->join_event, 1, pdFALSE, pdFALSE, portMAX_DELAY);
    return 0;
}

z_result_t _z_task_detach(_z_task_t *task) {
    _ZP_UNUSED(task);
    return _Z_ERR_GENERIC;
}

z_result_t _z_task_cancel(_z_task_t *task) {
    vTaskDelete(task->handle);
    return 0;
}

void _z_task_exit(void) { vTaskDelete(NULL); }

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr->join_event);
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
z_result_t _z_mutex_init(_z_mutex_t *m) {
    *m = xSemaphoreCreateRecursiveMutex();
    return *m == NULL ? -1 : 0;
}

z_result_t _z_mutex_drop(_z_mutex_t *m) {
    z_free(*m);
    return 0;
}

z_result_t _z_mutex_lock(_z_mutex_t *m) { return xSemaphoreTakeRecursive(*m, portMAX_DELAY) == pdTRUE ? 0 : -1; }

z_result_t _z_mutex_try_lock(_z_mutex_t *m) { return xSemaphoreTakeRecursive(*m, 0) == pdTRUE ? 0 : -1; }

z_result_t _z_mutex_unlock(_z_mutex_t *m) { return xSemaphoreGiveRecursive(*m) == pdTRUE ? 0 : -1; }

z_result_t _z_mutex_rec_init(_z_mutex_rec_t *m) { return _z_mutex_init(m); }

z_result_t _z_mutex_rec_drop(_z_mutex_rec_t *m) { return _z_mutex_drop(m); }

z_result_t _z_mutex_rec_lock(_z_mutex_rec_t *m) { return _z_mutex_lock(m); }

z_result_t _z_mutex_rec_try_lock(_z_mutex_rec_t *m) { return _z_mutex_try_lock(m); }

z_result_t _z_mutex_rec_unlock(_z_mutex_rec_t *m) { return _z_mutex_unlock(m); }

/*------------------ CondVar ------------------*/
static UBaseType_t CONDVAR_MAX_WAITERS_COUNT = UINT_MAX;

z_result_t _z_condvar_init(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }
    cv->mutex = xSemaphoreCreateMutex();
    cv->sem = xSemaphoreCreateCounting(CONDVAR_MAX_WAITERS_COUNT, 0);
    cv->waiters = 0;

    if (!cv->mutex || !cv->sem) {
        return _Z_ERR_GENERIC;
    }
    return _Z_RES_OK;
}

z_result_t _z_condvar_drop(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }
    vSemaphoreDelete(cv->sem);
    vSemaphoreDelete(cv->mutex);
    return _Z_RES_OK;
}

z_result_t _z_condvar_signal(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }

    xSemaphoreTake(cv->mutex, portMAX_DELAY);
    if (cv->waiters > 0) {
        xSemaphoreGive(cv->sem);
        cv->waiters--;
    }
    xSemaphoreGive(cv->mutex);

    return _Z_RES_OK;
}

z_result_t _z_condvar_signal_all(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }

    xSemaphoreTake(cv->mutex, portMAX_DELAY);
    while (cv->waiters > 0) {
        xSemaphoreGive(cv->sem);
        cv->waiters--;
    }
    xSemaphoreGive(cv->mutex);

    return _Z_RES_OK;
}

z_result_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) {
    if (!cv || !m) {
        return _Z_ERR_GENERIC;
    }

    xSemaphoreTake(cv->mutex, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->mutex);

    _z_mutex_unlock(m);

    xSemaphoreTake(cv->sem, portMAX_DELAY);

    return _z_mutex_lock(m);
}

z_result_t _z_condvar_wait_until(_z_condvar_t *cv, _z_mutex_t *m, const z_clock_t *abstime) {
    if (!cv || !m) {
        return _Z_ERR_GENERIC;
    }

    TickType_t now = xTaskGetTickCount();
    TickType_t target_time = pdMS_TO_TICKS(abstime->tv_sec * 1000 + abstime->tv_nsec / 1000000);
    TickType_t block_duration = (target_time > now) ? (target_time - now) : 0;

    xSemaphoreTake(cv->mutex, portMAX_DELAY);
    cv->waiters++;
    xSemaphoreGive(cv->mutex);

    _z_mutex_unlock(m);

    bool timed_out = xSemaphoreTake(cv->sem, block_duration) == pdFALSE;

    _z_mutex_lock(m);

    if (timed_out) {
        xSemaphoreTake(cv->mutex, portMAX_DELAY);
        cv->waiters--;
        xSemaphoreGive(cv->mutex);
        return Z_ETIMEDOUT;
    }

    return _Z_RES_OK;
}
#endif  // Z_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
z_result_t z_sleep_us(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time / 1000));
    return 0;
}

z_result_t z_sleep_ms(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time));
    return 0;
}

z_result_t z_sleep_s(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time * 1000));
    return 0;
}

/*------------------ Clock ------------------*/
void __z_clock_gettime(z_clock_t *ts) {
    uint64_t m = xTaskGetTickCount() / portTICK_PERIOD_MS;
    ts->tv_sec = m / (uint64_t)1000;
    ts->tv_nsec = (m % (uint64_t)1000) * (uint64_t)1000000;
}

z_clock_t z_clock_now(void) {
    z_clock_t now;
    __z_clock_gettime(&now);
    return now;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now;
    __z_clock_gettime(&now);

    unsigned long elapsed =
        (unsigned long)(1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now;
    __z_clock_gettime(&now);

    unsigned long elapsed =
        (unsigned long)(1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now;
    __z_clock_gettime(&now);

    unsigned long elapsed = (unsigned long)(now.tv_sec - instant->tv_sec);
    return elapsed;
}

void z_clock_advance_us(z_clock_t *clock, unsigned long duration) {
    clock->tv_sec += duration / 1000000;
    clock->tv_nsec += (duration % 1000000) * 1000;

    if (clock->tv_nsec >= 1000000000) {
        clock->tv_sec += 1;
        clock->tv_nsec -= 1000000000;
    }
}

void z_clock_advance_ms(z_clock_t *clock, unsigned long duration) {
    clock->tv_sec += duration / 1000;
    clock->tv_nsec += (duration % 1000) * 1000000;

    if (clock->tv_nsec >= 1000000000) {
        clock->tv_sec += 1;
        clock->tv_nsec -= 1000000000;
    }
}

void z_clock_advance_s(z_clock_t *clock, unsigned long duration) { clock->tv_sec += duration; }

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
    z_time_t now;
    gettimeofday(&now, NULL);
    t->secs = (uint32_t)now.tv_sec;
    t->nanos = (uint32_t)now.tv_usec * 1000;
    return 0;
}
