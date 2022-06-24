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

#include "zenoh-pico/system/platform.h"
#include <hw/driver/delay.h>

/*------------------ Random ------------------*/
uint8_t z_random_u8(void)
{
    return random(0xFF);
}

uint16_t z_random_u16(void)
{
    return random(0xFFFF);
}

uint32_t z_random_u32(void)
{
    return random(0xFFFFFFFF);
}

uint64_t z_random_u64(void)
{
    uint64_t ret = 0;
    ret |= z_random_u32();
    ret |= z_random_u32() << 8;

    return ret;
}

void z_random_fill(void *buf, size_t len)
{
    for (int i = 0; i < len; i++)
        *buf = z_random_u8();
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size)
{
    return pvPortMalloc(size);
}

void *z_realloc(void *ptr, size_t size)
{
    // TODO: not implemented
    return NULL;
}

void z_free(void *ptr)
{
    vPortFree(ptr);
}

/*------------------ Task ------------------*/
int z_task_init(z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg)
{
    return 0;
}

int z_task_join(z_task_t *task)
{
    return 0;
}

int z_task_cancel(z_task_t *task)
{
    return 0;
}

void z_task_free(z_task_t **task)
{
    z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int z_mutex_init(z_mutex_t *m)
{
    return 0;
}

int z_mutex_free(z_mutex_t *m)
{
    return 0;
}

int z_mutex_lock(z_mutex_t *m)
{
    return 0;
}

int z_mutex_trylock(z_mutex_t *m)
{
    return 0;
}

int z_mutex_unlock(z_mutex_t *m)
{
    return 0;
}

/*------------------ Condvar ------------------*/
int z_condvar_init(z_condvar_t *cv)
{
    return 0;
}

int z_condvar_free(z_condvar_t *cv)
{
    return 0;
}

int z_condvar_signal(z_condvar_t *cv)
{
    return 0;
}

int z_condvar_wait(z_condvar_t *cv, z_mutex_t *m)
{
    return 0;
}

/*------------------ Sleep ------------------*/
int z_sleep_us(unsigned int time)
{
    delay_us(time);
    return 0;
}

int z_sleep_ms(unsigned int time)
{
    delay_ms(time);
    return 0;
}

int z_sleep_s(unsigned int time)
{
    return z_sleep_ms(time * 1000);
}

/*------------------ Instant ------------------*/
void _zn_clock_gettime(clockid_t clk_id, z_clock_t *ts)
{
    uint64_t m = millis();
    ts->tv_sec = m / 1000000;
    ts->tv_nsec = (m % 1000000) * 1000;
}

z_clock_t z_clock_now()
{
    z_clock_t now;
    _zn_clock_gettime(NULL, &now);
    return now;
}

clock_t z_clock_elapsed_us(z_clock_t *instant)
{
    z_clock_t now;
    _zn_clock_gettime(NULL, &now);

    clock_t elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

clock_t z_clock_elapsed_ms(z_clock_t *instant)
{
    z_clock_t now;
    _zn_clock_gettime(NULL, &now);

    clock_t elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

clock_t z_clock_elapsed_s(z_clock_t *instant)
{
    z_clock_t now;
    _zn_clock_gettime(NULL, &now);

    clock_t elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
z_time_t z_time_now()
{
    z_time_t now;
    gettimeofday(&now, NULL);
    return now;
}

time_t z_time_elapsed_us(z_time_t *time)
{
    z_time_t now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

time_t z_time_elapsed_ms(z_time_t *time)
{
    z_time_t now;
    gettimeofday(&now, NULL);

    time_t elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

time_t z_time_elapsed_s(z_time_t *time)
{
    z_time_t now;
    gettimeofday(&now, NULL);

    time_t elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}
