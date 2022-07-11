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

#include <mbed.h>
#include <randLIB.h>

extern "C"
{
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void)
{
    return randLIB_get_8bit();
}

uint16_t z_random_u16(void)
{
    return randLIB_get_16bit();
}

uint32_t z_random_u32(void)
{
    return randLIB_get_32bit();
}

uint64_t z_random_u64(void)
{
    return randLIB_get_64bit();
}

void z_random_fill(void *buf, size_t len)
{
    randLIB_get_n_bytes_random(buf, len);
}

/*------------------ Task ------------------*/
int z_task_init(z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg)
{
    *task = new Thread();
    mbed::Callback<void()> c = mbed::Callback<void()>(fun, arg);
    return ((Thread*)*task)->start(c);
}

int z_task_join(z_task_t *task)
{
    int res = ((Thread*)*task)->join();
    delete ((Thread*)*task);
    return res;
}

int z_task_cancel(z_task_t *task)
{
    int res = ((Thread*)*task)->terminate();
    delete ((Thread*)*task);
    return res;
}

void z_task_free(z_task_t **task)
{
    z_task_t *ptr = *task;
    free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int z_mutex_init(z_mutex_t *m)
{
    *m = new Mutex();
    return 0;
}

int z_mutex_free(z_mutex_t *m)
{
    delete ((Mutex*)*m);
    return 0;
}

int z_mutex_lock(z_mutex_t *m)
{
    ((Mutex*)*m)->lock();
    return 0;
}

int z_mutex_trylock(z_mutex_t *m)
{
    return ((Mutex*)*m)->trylock() == true ? 0 : -1;
}

int z_mutex_unlock(z_mutex_t *m)
{
    ((Mutex*)*m)->unlock();
    return 0;
}

/*------------------ Condvar ------------------*/
int z_condvar_init(z_condvar_t *cv)
{
    return 0;
}

int z_condvar_free(z_condvar_t *cv)
{
    delete ((ConditionVariable*)*cv);
    return 0;
}

int z_condvar_signal(z_condvar_t *cv)
{
    ((ConditionVariable*)*cv)->notify_all();
    return 0;
}

int z_condvar_wait(z_condvar_t *cv, z_mutex_t *m)
{
    *cv = new ConditionVariable(*((Mutex*)*m));
    ((ConditionVariable*)*cv)->wait();
    return 0;
}

/*------------------ Sleep ------------------*/
int z_sleep_us(unsigned int time)
{
    return -1; // Not supported
}

int z_sleep_ms(unsigned int time)
{
    ThisThread::sleep_for(chrono::milliseconds(time));
    return 0;
}

int z_sleep_s(unsigned int time)
{
    ThisThread::sleep_for(chrono::seconds(time));
    return 0;
}

/*------------------ Instant ------------------*/
z_clock_t z_clock_now()
{
    // TODO: not implemented
    return NULL;
}

clock_t z_clock_elapsed_us(z_clock_t *instant)
{
    // TODO: not implemented
    return -1;
}

clock_t z_clock_elapsed_ms(z_clock_t *instant)
{
    // TODO: not implemented
    return -1;
}

clock_t z_clock_elapsed_s(z_clock_t *instant)
{
    // TODO: not implemented
    return -1;
}

/*------------------ Time ------------------*/
z_time_t z_time_now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now;
}

time_t z_time_elapsed_us(z_time_t *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

time_t z_time_elapsed_ms(z_time_t *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

time_t z_time_elapsed_s(z_time_t *time)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    time_t elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}

} // extern "C"