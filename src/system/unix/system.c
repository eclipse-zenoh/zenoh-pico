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

#include <sys/time.h>
#include <unistd.h>
#include "zenoh-pico/system/platform.h"

/*------------------ Task ------------------*/
int z_task_init(pthread_t *task, pthread_attr_t *attr, void *(*fun)(void *), void *arg)
{
    return pthread_create(task, attr, fun, arg);
}

int z_task_join(pthread_t *task)
{
    return pthread_join(*task, NULL);
}

int z_task_cancel(pthread_t *task)
{
    return pthread_cancel(*task);
}

void z_task_free(pthread_t **task)
{
    pthread_t *ptr = *task;
    free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int z_mutex_init(pthread_mutex_t *m)
{
    return pthread_mutex_init(m, 0);
}

int z_mutex_free(pthread_mutex_t *m)
{
    return pthread_mutex_destroy(m);
}

int z_mutex_lock(pthread_mutex_t *m)
{
    return pthread_mutex_lock(m);
}

int z_mutex_trylock(pthread_mutex_t *m)
{
    return pthread_mutex_trylock(m);
}

int z_mutex_unlock(pthread_mutex_t *m)
{
    return pthread_mutex_unlock(m);
}

/*------------------ Condvar ------------------*/
int z_condvar_init(pthread_cond_t *cv)
{
    return pthread_cond_init(cv, 0);
}

int z_condvar_free(pthread_cond_t *cv)
{
    return pthread_cond_destroy(cv);
}

int z_condvar_signal(z_condvar_t *cv)
{
    return pthread_cond_signal(cv);
}

int z_condvar_wait(z_condvar_t *cv, z_mutex_t *m)
{
    return pthread_cond_wait(cv, m);
}

/*------------------ Sleep ------------------*/
int z_sleep_us(unsigned int time)
{
    return usleep(time);
}

int z_sleep_ms(unsigned int time)
{
    return usleep(1000 * time);
}

int z_sleep_s(unsigned int time)
{
    return sleep(time);
}

/*------------------ Instant ------------------*/
struct timespec z_clock_now()
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now;
}

clock_t z_clock_elapsed_us(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    clock_t elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

clock_t z_clock_elapsed_ms(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    clock_t elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

clock_t z_clock_elapsed_s(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    clock_t elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
struct timeval z_time_now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now;
}

time_t z_time_elapsed_us(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

time_t z_time_elapsed_ms(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

time_t z_time_elapsed_s(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}
