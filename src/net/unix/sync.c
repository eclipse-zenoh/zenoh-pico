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
#include <unistd.h>
#include "zenoh/net/private/sync.h"

/*------------------ Mutex ------------------*/
int _zn_mutex_init(_zn_mutex_t *m)
{
    return pthread_mutex_init(m, 0);
}

int _zn_mutex_free(_zn_mutex_t *m)
{
    return pthread_mutex_destroy(m);
}

int _zn_mutex_lock(_zn_mutex_t *m)
{
    return pthread_mutex_lock(m);
}

int _zn_mutex_trylock(_zn_mutex_t *m)
{
    return pthread_mutex_trylock(m);
}

int _zn_mutex_unlock(_zn_mutex_t *m)
{
    return pthread_mutex_unlock(m);
}

/*------------------ Sleep ------------------*/
int _zn_sleep_us(unsigned int time)
{
    return usleep(time);
}

int _zn_sleep_ms(unsigned int time)
{
    return usleep(1000 * time);
}

int _zn_sleep_s(unsigned int time)
{
    return sleep(time);
}
