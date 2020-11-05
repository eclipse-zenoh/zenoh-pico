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
#include <pthread.h>
#include "zenoh/net/private/tasks.h"

/*------------------ Read task ------------------*/
void *pthread_read_task(void *arg)
{
    return _znp_read_task((zn_session_t *)arg);
}

int znp_start_read_task(zn_session_t *z)
{
    pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
    memset(thread, 0, sizeof(pthread_t));
    z->read_task_thread = thread;
    if (pthread_create(thread, 0, pthread_read_task, z) != 0)
    {
        return -1;
    }
    return 0;
}

int znp_stop_read_task(zn_session_t *z)
{
    z->read_task_running = 0;
    return 0;
}

/*------------------ Lease task ------------------*/
void *pthread_lease_task(void *arg)
{
    return _znp_lease_task((zn_session_t *)arg);
}

int znp_start_lease_task(zn_session_t *z)
{
    pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
    memset(thread, 0, sizeof(pthread_t));
    z->lease_task_thread = thread;
    if (pthread_create(thread, 0, pthread_lease_task, z) != 0)
    {
        return -1;
    }
    return 0;
}

int znp_stop_lease_task(zn_session_t *z)
{
    z->lease_task_running = 0;
    return 0;
}
