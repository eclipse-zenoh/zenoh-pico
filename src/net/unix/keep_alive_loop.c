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
#include "zenoh/net/keep_alive_loop.h"

void *pthread_keep_alive_loop(void *arg)
{
    return zn_keep_alive_loop((zn_session_t *)arg);
}

int zn_start_keep_alive_loop(zn_session_t *z)
{
    pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
    memset(thread, 0, sizeof(pthread_t));
    z->keep_alive_loop_thread = thread;
    if (pthread_create(thread, 0, pthread_keep_alive_loop, z) != 0)
    {
        return -1;
    }
    return 0;
}

int zn_stop_keep_alive_loop(zn_session_t *z)
{
    z->running = 0;
    return 0;
}
