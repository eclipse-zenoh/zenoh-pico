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

#include "zenoh-pico/system/platform.h"
#include <hw/driver/delay.h>

/*------------------ Task ------------------*/
int _z_task_init(_z_task_t *task, _z_task_attr_t *attr, void *(*fun)(void *), void *arg)
{
    return 0;
}

int _z_task_join(_z_task_t *task)
{
    return 0;
}

int _z_task_cancel(_z_task_t *task)
{
    return 0;
}

void _z_task_free(_z_task_t **task)
{
    _z_task_t *ptr = *task;
    free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int _z_mutex_init(_z_mutex_t *m)
{
    return 0;
}

int _z_mutex_free(_z_mutex_t *m)
{
    return 0;
}

int _z_mutex_lock(_z_mutex_t *m)
{
    return 0;
}

int _z_mutex_trylock(_z_mutex_t *m)
{
    return 0;
}

int _z_mutex_unlock(_z_mutex_t *m)
{
    return 0;
}

/*------------------ Condvar ------------------*/
int _z_condvar_init(_z_condvar_t *cv)
{
    return 0;
}

int _z_condvar_free(_z_condvar_t *cv)
{
    return 0;
}

int _z_condvar_signal(_z_condvar_t *cv)
{
    return 0;
}

int _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m)
{
    return 0;
}

/*------------------ Sleep ------------------*/
int _z_sleep_us(unsigned int time)
{
    return usleep(time);
}

int _z_sleep_ms(unsigned int time)
{
    return usleep(1000 * time);
}

int _z_sleep_s(unsigned int time)
{
    return sleep(time);
}

/*------------------ Instant ------------------*/
void _z_clock_gettime(clockid_t clk_id, _z_clock_t *ts)
{
    uint64_t m = millis();
    ts->tv_sec = m / 1000000;
    ts->tv_nsec = (m % 1000000) * 1000;
}

_z_clock_t _z_clock_now()
{
    _z_clock_t now;
    _z_clock_gettime(NULL, &now);
    return now;
}

clock_t _z_clock_elapsed_us(_z_clock_t *instant)
{
    _z_clock_t now;
    _z_clock_gettime(NULL, &now);

    clock_t elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

clock_t _z_clock_elapsed_ms(_z_clock_t *instant)
{
    _z_clock_t now;
    _z_clock_gettime(NULL, &now);

    clock_t elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

clock_t _z_clock_elapsed_s(_z_clock_t *instant)
{
    _z_clock_t now;
    _z_clock_gettime(NULL, &now);

    clock_t elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
struct timeval _z_time_now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now;
}

time_t _z_time_elapsed_us(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

time_t _z_time_elapsed_ms(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

time_t _z_time_elapsed_s(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}
