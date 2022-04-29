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
#include <sys/time.h>

// This wrapper is only used for ESP32.
// In FreeRTOS, tasks created using xTaskCreate must end with vTaskDelete.
// A task function should __not__ simply return.
typedef struct
{
    void *(*_fun)(void *);
    void *_arg;
} __z_task_arg;

void z_task_wrapper(void *arg)
{
    __z_task_arg *z_arg = (__z_task_arg*)arg;
    z_arg->_fun(z_arg->_arg);
    vTaskDelete(NULL);
    free(z_arg);
}


/*------------------ Task ------------------*/
int _z_task_init(_z_task_t *task, _z_task_attr_t *attr, void *(*fun)(void *), void *arg)
{
    __z_task_arg *z_arg = (__z_task_arg *)malloc(sizeof(__z_task_arg));
    z_arg->_fun = fun;
    z_arg->_arg = arg;
    if (xTaskCreate(z_task_wrapper, "", 5120, z_arg, 15, task) != pdPASS)
        return -1;

    return 0;
}

int _z_task_join(_z_task_t *task)
{
    // Note: join not supported using FreeRTOS API
    return 0;
}

int _z_task_cancel(_z_task_t *task)
{
    vTaskDelete(task);
    return 0;
}

void _z_task_free(_z_task_t **task)
{
    _z_task_t *ptr = *task;
    free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int _z_mutex_init(pthread_mutex_t *m)
{
    return pthread_mutex_init(m, NULL);
}

int _z_mutex_free(pthread_mutex_t *m)
{
    return pthread_mutex_destroy(m);
}

int _z_mutex_lock(pthread_mutex_t *m)
{
    return pthread_mutex_lock(m);
}

int _z_mutex_trylock(pthread_mutex_t *m)
{
    return pthread_mutex_trylock(m);
}

int _z_mutex_unlock(pthread_mutex_t *m)
{
    return pthread_mutex_unlock(m);
}

/*------------------ Condvar ------------------*/
int _z_condvar_init(pthread_cond_t *cv)
{
    return pthread_cond_init(cv, NULL);
}

int _z_condvar_free(pthread_cond_t *cv)
{
    return pthread_cond_destroy(cv);
}

int _z_condvar_signal(_z_condvar_t *cv)
{
    return pthread_cond_signal(cv);
}

int _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m)
{
    return pthread_cond_wait(cv, m);
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
struct timespec _z_clock_now()
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now;
}

clock_t _z_clock_elapsed_us(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    clock_t elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

clock_t _z_clock_elapsed_ms(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    clock_t elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

clock_t _z_clock_elapsed_s(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

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
