/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef _ZENOH_PICO_SYSTEM_PRIVATE_ZEPHYR_TYPES_H
#define _ZENOH_PICO_SYSTEM_PRIVATE_ZEPHYR_TYPES_H

#include <zephyr.h>
#include <pthread.h>
#include "utils.h"

typedef int _zn_socket_t;

typedef pthread_t z_task_t;
typedef pthread_attr_t z_task_attr_t;
typedef pthread_mutex_t z_mutex_t;
typedef pthread_cond_t z_condvar_t;

typedef struct timespec z_clock_t;
typedef struct timeval z_time_t;

typedef clockid_t clock_t;

#endif /* _ZENOH_PICO_SYSTEM_PRIVATE_ZEPHYR_TYPES_H */
