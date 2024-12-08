//
// Copyright (c) 2022 ZettaScale Technology
// Copyright (c) 2023 Fictionlab sp. z o.o.
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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS_IP.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { return z_random_u32(); }

uint16_t z_random_u16(void) { return z_random_u32(); }

uint32_t z_random_u32(void) {
    uint32_t ret = 0;
    xApplicationGetRandomNumber(&ret);
    return ret;
}

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
    ret |= z_random_u32();
    ret = ret << 32;
    ret |= z_random_u32();
    return ret;
}

void z_random_fill(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        *((uint8_t *)buf) = z_random_u8();
    }
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) { return pvPortMalloc(size); }

void *z_realloc(void *ptr, size_t size) {
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(size);
    // realloc not implemented in FreeRTOS
    return NULL;
}

void z_free(void *ptr) { vPortFree(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1
// In FreeRTOS, tasks created using xTaskCreate must end with vTaskDelete.
// A task function should __not__ simply return.
static void z_task_wrapper(void *arg) {
    _z_task_t *task = (_z_task_t *)arg;
    task->fun(task->arg);
    xEventGroupSetBits(task->join_event, 1);
    task->handle = NULL;
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
z_result_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    task->fun = fun;
    task->arg = arg;

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    task->join_event = xEventGroupCreateStatic(&task->join_event_buffer);
#else
    task->join_event = xEventGroupCreate();
#endif /* SUPPORT_STATIC_ALLOCATION */

    if (attr == NULL) {
        attr = &z_default_task_attr;
    }

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    if (attr->static_allocation) {
        task->handle = xTaskCreateStatic(z_task_wrapper, attr->name, attr->stack_depth, task, attr->priority,
                                         attr->stack_buffer, attr->task_buffer);
        if (task->handle == NULL) {
            return _Z_ERR_GENERIC;
        }
    } else {
#endif /* SUPPORT_STATIC_ALLOCATION */
        if (xTaskCreate(z_task_wrapper, attr->name, attr->stack_depth, task, attr->priority, &task->handle) != pdPASS) {
            return -1;
        }
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    }
#endif /* SUPPORT_STATIC_ALLOCATION */

    return _Z_RES_OK;
}

z_result_t _z_task_join(_z_task_t *task) {
    xEventGroupWaitBits(task->join_event, 1, pdFALSE, pdFALSE, portMAX_DELAY);
    return _Z_RES_OK;
}

z_result_t _z_task_detach(_z_task_t *task) {
    // Note: task/thread detach not supported on FreeRTOS API, so we force its deletion instead.
    return _z_task_cancel(task);
}

z_result_t _z_task_cancel(_z_task_t *task) {
    xEventGroupSetBits(task->join_event, 1);
    if (task->handle != NULL) {
        vTaskDelete(task->handle);
        task->handle = NULL;
    }
    return _Z_RES_OK;
}

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    vEventGroupDelete(ptr->join_event);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
z_result_t _z_mutex_init(_z_mutex_t *m) {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    m->handle = xSemaphoreCreateRecursiveMutexStatic(&m->buffer);
#else
    m->handle = xSemaphoreCreateRecursiveMutex();
#endif /* SUPPORT_STATIC_ALLOCATION */
    return m->handle == NULL ? _Z_ERR_GENERIC : _Z_RES_OK;
}

z_result_t _z_mutex_drop(_z_mutex_t *m) {
    vSemaphoreDelete(m->handle);
    return _Z_RES_OK;
}

z_result_t _z_mutex_lock(_z_mutex_t *m) {
    return xSemaphoreTakeRecursive(m->handle, portMAX_DELAY) == pdTRUE ? _Z_RES_OK : _Z_ERR_GENERIC;
}

z_result_t _z_mutex_try_lock(_z_mutex_t *m) {
    return xSemaphoreTakeRecursive(m->handle, 0) == pdTRUE ? _Z_RES_OK : _Z_ERR_GENERIC;
}

z_result_t _z_mutex_unlock(_z_mutex_t *m) {
    return xSemaphoreGiveRecursive(m->handle) == pdTRUE ? _Z_RES_OK : _Z_ERR_GENERIC;
}

/*------------------ CondVar ------------------*/
z_result_t _z_condvar_init(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }

#if (configSUPPORT_STATIC_ALLOCATION == 1)
    cv->mutex = xSemaphoreCreateMutexStatic(&cv->mutex_buffer);
    cv->sem = xSemaphoreCreateCountingStatic((UBaseType_t)(~0), 0, &cv->sem_buffer);
#else
    cv->mutex = xSemaphoreCreateMutex();
    cv->sem = xSemaphoreCreateCounting((UBaseType_t)(~0), 0);
#endif /* SUPPORT_STATIC_ALLOCATION */
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

    _z_mutex_lock(m);

    return _Z_RES_OK;
}
#endif  // Z_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
z_result_t z_sleep_us(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time / 1000));
    return _Z_RES_OK;
}

z_result_t z_sleep_ms(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time));
    return _Z_RES_OK;
}

z_result_t z_sleep_s(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time * 1000));
    return _Z_RES_OK;
}

/*------------------ Clock ------------------*/
z_clock_t z_clock_now(void) { return z_time_now(); }

unsigned long z_clock_elapsed_us(z_clock_t *instant) { return z_clock_elapsed_ms(instant) * 1000; }

unsigned long z_clock_elapsed_ms(z_clock_t *instant) { return z_time_elapsed_ms(instant); }

unsigned long z_clock_elapsed_s(z_clock_t *instant) { return z_clock_elapsed_ms(instant) / 1000; }

/*------------------ Time ------------------*/
z_time_t z_time_now(void) { return xTaskGetTickCount(); }

const char *z_time_now_as_str(char *const buf, unsigned long buflen) {
    snprintf(buf, buflen, "%u", z_time_now());
    return buf;
}

unsigned long z_time_elapsed_us(z_time_t *time) { return z_time_elapsed_ms(time) * 1000; }

unsigned long z_time_elapsed_ms(z_time_t *time) {
    z_time_t now = z_time_now();

    unsigned long elapsed = (now - *time) * portTICK_PERIOD_MS;
    return elapsed;
}

unsigned long z_time_elapsed_s(z_time_t *time) { return z_time_elapsed_ms(time) / 1000; }

z_result_t _z_get_time_since_epoch(_z_time_since_epoch *t) { return -1; }
