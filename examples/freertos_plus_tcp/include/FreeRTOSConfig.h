//
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
//   Błażej Sowa, <blazej@fictionlab.pl>

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "bits/pthread_stack_min.h"

/* Core options */
#define configUSE_PREEMPTION 1
#define configTICK_RATE_HZ ((TickType_t)1000)
#define configMAX_PRIORITIES (56)
#define configMINIMAL_STACK_SIZE ((uint16_t)PTHREAD_STACK_MIN)
#define configMAX_TASK_NAME_LEN (16)
#define configUSE_16_BIT_TICKS 0
#define configQUEUE_REGISTRY_SIZE 0
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configUSE_DAEMON_TASK_STARTUP_HOOK 0
#define configUSE_SB_COMPLETED_CALLBACK 0

/* Enable/Disable features */
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_CO_ROUTINES 0
#define configUSE_TIMERS 0
#define configUSE_TRACE_FACILITY 0
#define configUSE_QUEUE_SETS 0
#define configUSE_NEWLIB_REENTRANT 0

/* Set the API functions which should be included */
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskCleanUpResources 0
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTimerPendFunctionCall 1
#define INCLUDE_xQueueGetMutexHolder 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_eTaskGetState 1
#define INCLUDE_xTimerPendFunctionCall 0

#endif /* FREERTOS_CONFIG_H */
