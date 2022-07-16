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

#include <zephyr.h>
#include <random/rand32.h>
#include <sys/time.h>
#include <unistd.h>

#include "zenoh-pico/system/platform.h"

#define Z_THREADS_NUM 4
#define Z_PTHREAD_STACK_SIZE_DEFAULT CONFIG_MAIN_STACK_SIZE + CONFIG_TEST_EXTRA_STACKSIZE
K_THREAD_STACK_ARRAY_DEFINE(thread_stack_area, Z_THREADS_NUM, Z_PTHREAD_STACK_SIZE_DEFAULT);
static int thread_index = 0;

/*------------------ Random ------------------*/
uint8_t z_random_u8(void)
{
    return z_random_u32();
}

uint16_t z_random_u16(void)
{
    return z_random_u32();
}

uint32_t z_random_u32(void)
{
    return sys_rand32_get();
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
    sys_rand_get(buf, len);
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size)
{
    return k_malloc(size);
}

void *z_realloc(void *ptr, size_t size)
{
    // k_realloc not implemented in Zephyr
    return NULL;
}

void z_free(void *ptr)
{
    k_free(ptr);
}

/*------------------ Task ------------------*/
int z_task_init(z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg)
{
    if (attr == NULL)
    {
        z_task_attr_t tmp;
        (void)pthread_attr_init(&tmp);
        (void)pthread_attr_setstack(&tmp, &thread_stack_area[thread_index++],
                                    Z_PTHREAD_STACK_SIZE_DEFAULT);
        attr = &tmp;
    }

    return pthread_create(task, attr, fun, arg);
}

int z_task_join(z_task_t *task)
{
    return pthread_join(*task, NULL);
}

int z_task_cancel(z_task_t *task)
{
    return pthread_cancel(*task);
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
    return pthread_mutex_init(m, 0);
}

int z_mutex_free(z_mutex_t *m)
{
    return pthread_mutex_destroy(m);
}

int z_mutex_lock(z_mutex_t *m)
{
    return pthread_mutex_lock(m);
}

int z_mutex_trylock(z_mutex_t *m)
{
    return pthread_mutex_trylock(m);
}

int z_mutex_unlock(z_mutex_t *m)
{
    return pthread_mutex_unlock(m);
}

/*------------------ Condvar ------------------*/
int z_condvar_init(z_condvar_t *cv)
{
    return pthread_cond_init(cv, 0);
}

int z_condvar_free(z_condvar_t *cv)
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
z_clock_t z_clock_now()
{
    z_clock_t now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant)
{
    z_clock_t now;
    clock_gettime(CLOCK_REALTIME, &now);

    unsigned long elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant)
{
    z_clock_t now;
    clock_gettime(CLOCK_REALTIME, &now);

    unsigned long elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant)
{
    z_clock_t now;
    clock_gettime(CLOCK_REALTIME, &now);

    unsigned long elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
z_time_t z_time_now()
{
    z_time_t now;
    gettimeofday(&now, NULL);
    return now;
}

unsigned long z_time_elapsed_us(z_time_t *time)
{
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

unsigned long z_time_elapsed_ms(z_time_t *time)
{
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

unsigned long z_time_elapsed_s(z_time_t *time)
{
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}
