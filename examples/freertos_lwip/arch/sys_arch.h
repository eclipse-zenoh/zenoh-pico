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

#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/* FreeRTOS-based sys_arch definitions */
typedef SemaphoreHandle_t sys_sem_t;
typedef SemaphoreHandle_t sys_mutex_t;
typedef QueueHandle_t sys_mbox_t;
typedef TaskHandle_t sys_thread_t;
typedef UBaseType_t sys_prot_t;

#define SYS_MBOX_NULL (QueueHandle_t)0
#define SYS_SEM_NULL (SemaphoreHandle_t)0
#define SYS_MUTEX_NULL (SemaphoreHandle_t)0
#define SYS_THREAD_NULL (TaskHandle_t)0

#endif /* LWIP_ARCH_SYS_ARCH_H */
