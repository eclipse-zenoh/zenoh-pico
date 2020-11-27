/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#ifndef _ZENOH_PICO_UNIX_TYPES_H
#define _ZENOH_PICO_UNIX_TYPES_H

#include <pthread.h>

typedef pthread_t _z_task_t;
typedef pthread_attr_t _z_task_attr_t;
typedef pthread_mutex_t _z_mutex_t;
typedef pthread_cond_t _z_condvar_t;

typedef struct timespec _z_clock_t;
typedef struct timeval _z_time_t;

#endif /* _ZENOH_PICO_UNIX_TYPES_H_ */
