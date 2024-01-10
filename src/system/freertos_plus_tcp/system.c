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
uint8_t zp_random_u8(void) { return zp_random_u32(); }

uint16_t zp_random_u16(void) { return zp_random_u32(); }

uint32_t zp_random_u32(void) {
    uint32_t ret = 0;
    xApplicationGetRandomNumber(&ret);
    return ret;
}

uint64_t zp_random_u64(void) {
    uint64_t ret = 0;
    ret |= zp_random_u32();
    ret = ret << 32;
    ret |= zp_random_u32();
    return ret;
}

void zp_random_fill(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        *((uint8_t *)buf) = zp_random_u8();
    }
}

/*------------------ Memory ------------------*/
void *zp_malloc(size_t size) { return pvPortMalloc(size); }

void *zp_realloc(void *ptr, size_t size) {
    // realloc not implemented in FreeRTOS
    return NULL;
}

void zp_free(void *ptr) { vPortFree(ptr); }

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

static zp_task_attr_t z_default_task_attr = {
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
int8_t zp_task_init(zp_task_t *task, zp_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    z_task_arg *z_arg = (z_task_arg *)zp_malloc(sizeof(z_task_arg));
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

int8_t zp_task_join(zp_task_t *task) {
    xEventGroupWaitBits(task->join_event, 1, pdFALSE, pdFALSE, portMAX_DELAY);
    return 0;
}

int8_t zp_task_cancel(zp_task_t *task) {
    vTaskDelete(task->handle);
    return 0;
}

void zp_task_free(zp_task_t **task) {
    zp_free((*task)->join_event);
    zp_free(*task);
}

/*------------------ Mutex ------------------*/
int8_t zp_mutex_init(zp_mutex_t *m) {
    *m = xSemaphoreCreateRecursiveMutex();
    return *m == NULL ? -1 : 0;
}

int8_t zp_mutex_free(zp_mutex_t *m) {
    zp_free(*m);
    return 0;
}

int8_t zp_mutex_lock(zp_mutex_t *m) { return xSemaphoreTakeRecursive(*m, portMAX_DELAY) == pdTRUE ? 0 : -1; }

int8_t zp_mutex_trylock(zp_mutex_t *m) { return xSemaphoreTakeRecursive(*m, 0) == pdTRUE ? 0 : -1; }

int8_t zp_mutex_unlock(zp_mutex_t *m) { return xSemaphoreGiveRecursive(*m) == pdTRUE ? 0 : -1; }

/*------------------ CondVar ------------------*/
// Condition variables not supported in FreeRTOS
int8_t zp_condvar_init(zp_condvar_t *cv) { return -1; }
int8_t zp_condvar_free(zp_condvar_t *cv) { return -1; }
int8_t zp_condvar_signal(zp_condvar_t *cv) { return -1; }
int8_t zp_condvar_wait(zp_condvar_t *cv, zp_mutex_t *m) { return -1; }
#endif  // Z_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int zp_sleep_us(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time / 1000));
    return 0;
}

int zp_sleep_ms(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time));
    return 0;
}

int zp_sleep_s(size_t time) {
    vTaskDelay(pdMS_TO_TICKS(time * 1000));
    return 0;
}

/*------------------ Clock ------------------*/
zp_clock_t zp_clock_now(void) { return zp_time_now(); }

unsigned long zp_clock_elapsed_us(zp_clock_t *instant) { return zp_clock_elapsed_ms(instant) * 1000; }

unsigned long zp_clock_elapsed_ms(zp_clock_t *instant) { return zp_time_elapsed_ms(instant); }

unsigned long zp_clock_elapsed_s(zp_clock_t *instant) { return zp_clock_elapsed_ms(instant) / 1000; }

/*------------------ Time ------------------*/
zp_time_t zp_time_now(void) { return xTaskGetTickCount(); }

const char *zp_time_now_as_str(char *const buf, unsigned long buflen) {
    snprintf(buf, buflen, "%u", zp_time_now());
    return buf;
}

unsigned long zp_time_elapsed_us(zp_time_t *time) { return zp_time_elapsed_ms(time) * 1000; }

unsigned long zp_time_elapsed_ms(zp_time_t *time) {
    zp_time_t now = zp_time_now();

    unsigned long elapsed = (now - *time) * portTICK_PERIOD_MS;
    return elapsed;
}

unsigned long zp_time_elapsed_s(zp_time_t *time) { return zp_time_elapsed_ms(time) / 1000; }
