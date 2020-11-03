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
#include <sys/time.h>
#include <unistd.h>
#include "zenoh/net/private/system.h"

/*------------------ Mutex ------------------*/
// As define in "zenoh/net/private/system.h"
// typedef pthread_mutex_t _zn_mutex_t;
int _zn_mutex_init(pthread_mutex_t *m)
{
    return pthread_mutex_init(m, 0);
}

int _zn_mutex_free(pthread_mutex_t *m)
{
    return pthread_mutex_destroy(m);
}

int _zn_mutex_lock(pthread_mutex_t *m)
{
    return pthread_mutex_lock(m);
}

int _zn_mutex_trylock(pthread_mutex_t *m)
{
    return pthread_mutex_trylock(m);
}

int _zn_mutex_unlock(pthread_mutex_t *m)
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

/*------------------ Instant ------------------*/
// As define in "zenoh/net/private/system.h"
// typedef struct timespec _zn_clock_t;
struct timespec _zn_clock_now()
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now;
}

clock_t _zn_clock_elapsed_us(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    clock_t elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

clock_t _zn_clock_elapsed_ms(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    clock_t elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

clock_t _zn_clock_elapsed_s(struct timespec *instant)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    clock_t elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
// As define in "zenoh/net/private/system.h"
// typedef struct timeval _zn_time_t;
struct timeval _zn_time_now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now;
}

time_t _zn_time_elapsed_us(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

time_t _zn_time_elapsed_ms(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

time_t _zn_time_elapsed_s(struct timeval *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}
