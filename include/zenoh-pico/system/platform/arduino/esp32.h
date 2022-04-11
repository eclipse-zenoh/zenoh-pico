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

#ifndef ZENOH_PICO_SYSTEM_ESP32_TYPES_H
#define ZENOH_PICO_SYSTEM_ESP32_TYPES_H

#include <pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef TaskHandle_t z_task_t;
typedef void *z_task_attr_t; // Not used in ESP32
typedef pthread_mutex_t z_mutex_t;
typedef pthread_cond_t z_condvar_t;

typedef struct timespec z_clock_t;
typedef struct timeval z_time_t;

#endif /* ZENOH_PICO_SYSTEM_ESP32_TYPES_H */
