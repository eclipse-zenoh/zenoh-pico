//
// Copyright (c) 2023 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   hyojun.an <anhj2473@ivis.ai>

#include "arch/sys_arch.h"

#include <string.h>

#include "FreeRTOS.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

void sys_init(void) {
    // Nothing to do here
}

err_t sys_sem_new(sys_sem_t *sem, u8_t count) {
    *sem = xSemaphoreCreateBinary();
    if (*sem == NULL) {
        return ERR_MEM;
    }
    if (count == 1) {
        xSemaphoreGive(*sem);
    }
    return ERR_OK;
}

void sys_sem_free(sys_sem_t *sem) {
    if (sem != NULL && *sem != SYS_SEM_NULL) {
        vSemaphoreDelete(*sem);
        *sem = SYS_SEM_NULL;
    }
}

void sys_sem_signal(sys_sem_t *sem) {
    if (sem != NULL && *sem != SYS_SEM_NULL) {
        xSemaphoreGive(*sem);
    }
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout) {
    TickType_t start_time = xTaskGetTickCount();
    TickType_t wait_ticks = (timeout == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout);

    if (xSemaphoreTake(*sem, wait_ticks) == pdTRUE) {
        TickType_t elapsed = xTaskGetTickCount() - start_time;
        return (u32_t)pdTICKS_TO_MS(elapsed);
    } else {
        return SYS_ARCH_TIMEOUT;
    }
}

err_t sys_mutex_new(sys_mutex_t *mutex) {
    *mutex = xSemaphoreCreateMutex();
    if (*mutex == NULL) {
        return ERR_MEM;
    }
    return ERR_OK;
}

void sys_mutex_free(sys_mutex_t *mutex) {
    if (mutex != NULL && *mutex != SYS_MUTEX_NULL) {
        vSemaphoreDelete(*mutex);
        *mutex = SYS_MUTEX_NULL;
    }
}

void sys_mutex_lock(sys_mutex_t *mutex) {
    if (mutex != NULL && *mutex != SYS_MUTEX_NULL) {
        xSemaphoreTake(*mutex, portMAX_DELAY);
    }
}

void sys_mutex_unlock(sys_mutex_t *mutex) {
    if (mutex != NULL && *mutex != SYS_MUTEX_NULL) {
        xSemaphoreGive(*mutex);
    }
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
    *mbox = xQueueCreate(size, sizeof(void *));
    if (*mbox == NULL) {
        return ERR_MEM;
    }
    return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox) {
    if (mbox != NULL && *mbox != SYS_MBOX_NULL) {
        vQueueDelete(*mbox);
        *mbox = SYS_MBOX_NULL;
    }
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg) {
    if (mbox != NULL && *mbox != SYS_MBOX_NULL) {
        xQueueSend(*mbox, &msg, portMAX_DELAY);
    }
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg) {
    if (mbox == NULL || *mbox == SYS_MBOX_NULL) {
        return ERR_MEM;
    }
    if (xQueueSend(*mbox, &msg, 0) == pdTRUE) {
        return ERR_OK;
    } else {
        return ERR_MEM;
    }
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout) {
    void *msg_dummy;
    TickType_t start_time = xTaskGetTickCount();
    TickType_t wait_ticks = (timeout == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout);

    if (msg == NULL) {
        msg = &msg_dummy;
    }

    if (xQueueReceive(*mbox, msg, wait_ticks) == pdTRUE) {
        TickType_t elapsed = xTaskGetTickCount() - start_time;
        return (u32_t)pdTICKS_TO_MS(elapsed);
    } else {
        *msg = NULL;
        return SYS_ARCH_TIMEOUT;
    }
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg) {
    void *msg_dummy;
    if (msg == NULL) {
        msg = &msg_dummy;
    }

    if (xQueueReceive(*mbox, msg, 0) == pdTRUE) {
        return 0;
    } else {
        *msg = NULL;
        return SYS_MBOX_EMPTY;
    }
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio) {
    TaskHandle_t task_handle;
    BaseType_t result = xTaskCreate(thread, name, stacksize / sizeof(StackType_t), arg, prio, &task_handle);

    if (result == pdPASS) {
        return task_handle;
    } else {
        return SYS_THREAD_NULL;
    }
}

u32_t sys_now(void) { return (u32_t)pdTICKS_TO_MS(xTaskGetTickCount()); }

void sys_msleep(u32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

sys_prot_t sys_arch_protect(void) {
    vTaskSuspendAll();
    return 1;
}

void sys_arch_unprotect(sys_prot_t pval) {
    (void)pval;
    xTaskResumeAll();
}

int sys_mbox_valid(sys_mbox_t *mbox) { return (mbox != NULL && *mbox != NULL); }

void sys_mbox_set_invalid(sys_mbox_t *mbox) {
    if (mbox != NULL) {
        *mbox = NULL;
    }
}

int sys_sem_valid(sys_sem_t *sem) { return (sem != NULL && *sem != NULL); }

void sys_sem_set_invalid(sys_sem_t *sem) {
    if (sem != NULL) {
        *sem = NULL;
    }
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (mbox == NULL || *mbox == NULL) {
        return ERR_MEM;
    }
    if (xQueueSendFromISR(*mbox, &msg, &xHigherPriorityTaskWoken) == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return ERR_OK;
    } else {
        return ERR_MEM;
    }
}
