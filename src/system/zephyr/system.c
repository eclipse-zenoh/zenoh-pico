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

#include <version.h>

#if KERNEL_VERSION_MAJOR == 2
#include <random/rand32.h>
#else
#include <zephyr/random/random.h>
#endif

#include <stddef.h>
#include <sys/time.h>
#include <unistd.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { return z_random_u32(); }

uint16_t z_random_u16(void) { return z_random_u32(); }

uint32_t z_random_u32(void) { return sys_rand32_get(); }

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
    ret |= z_random_u32();
    ret = ret << 32;
    ret |= z_random_u32();

    return ret;
}

void z_random_fill(void *buf, size_t len) { sys_rand_get(buf, len); }

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) { return k_malloc(size); }

void *z_realloc(void *ptr, size_t size) {
    // k_realloc not implemented in Zephyr
    return NULL;
}

void z_free(void *ptr) { k_free(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1

#define Z_THREADS_NUM 4

#ifdef CONFIG_TEST_EXTRA_STACK_SIZE
#define Z_PTHREAD_STACK_SIZE_DEFAULT CONFIG_MAIN_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE
#elif CONFIG_TEST_EXTRA_STACKSIZE
#define Z_PTHREAD_STACK_SIZE_DEFAULT CONFIG_MAIN_STACK_SIZE + CONFIG_TEST_EXTRA_STACKSIZE
#else
#define Z_PTHREAD_STACK_SIZE_DEFAULT CONFIG_MAIN_STACK_SIZE
#endif

K_THREAD_STACK_ARRAY_DEFINE(thread_stack_area, Z_THREADS_NUM, Z_PTHREAD_STACK_SIZE_DEFAULT);
static int thread_index = 0;

/*------------------ Task ------------------*/
int8_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    z_task_attr_t *lattr = NULL;
    z_task_attr_t tmp;
    if (attr == NULL) {
        (void)pthread_attr_init(&tmp);
        (void)pthread_attr_setstack(&tmp, &thread_stack_area[thread_index++], Z_PTHREAD_STACK_SIZE_DEFAULT);
        lattr = &tmp;
    }

    return pthread_create(task, lattr, fun, arg);
}

int8_t _z_task_join(_z_task_t *task) { return pthread_join(*task, NULL); }

int8_t _z_task_cancel(_z_task_t *task) { return pthread_cancel(*task); }

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t _z_mutex_init(_z_mutex_t *m) { return pthread_mutex_init(m, 0); }

int8_t _z_mutex_drop(_z_mutex_t *m) { return pthread_mutex_destroy(m); }

int8_t _z_mutex_lock(_z_mutex_t *m) { return pthread_mutex_lock(m); }

int8_t _z_mutex_try_lock(_z_mutex_t *m) { return pthread_mutex_trylock(m); }

int8_t _z_mutex_unlock(_z_mutex_t *m) { return pthread_mutex_unlock(m); }

/*------------------ Condvar ------------------*/
int8_t _z_condvar_init(_z_condvar_t *cv) { return pthread_cond_init(cv, 0); }

int8_t _z_condvar_drop(_z_condvar_t *cv) { return pthread_cond_destroy(cv); }

int8_t _z_condvar_signal(_z_condvar_t *cv) { return pthread_cond_signal(cv); }

int8_t _z_condvar_signal_all(_z_condvar_t *cv) { return pthread_cond_broadcast(cv); }

int8_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) { return pthread_cond_wait(cv, m); }
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time) {
    int32_t rem = time;
    while (rem > 0) {
        rem = k_usleep(rem);  // This function is unlikely to work as expected without kernel tuning.
                              // In particular, because the lower bound on the duration of a sleep is the
                              // duration of a tick, CONFIG_SYS_CLOCK_TICKS_PER_SEC must be adjusted to
                              // achieve the resolution desired. The implications of doing this must be
                              // understood before attempting to use k_usleep(). Use with caution.
                              // From: https://docs.zephyrproject.org/apidoc/latest/group__thread__apis.html
    }

    return 0;
}

int z_sleep_ms(size_t time) {
    int32_t rem = time;
    while (rem > 0) {
        rem = k_msleep(rem);
    }

    return 0;
}

int z_sleep_s(size_t time) {
    int32_t rem = time;
    while (rem > 0) {
        rem = k_sleep(K_SECONDS(rem));
    }

    return 0;
}

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed = (1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed = (1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long elapsed = now.tv_sec - instant->tv_sec;
    return elapsed;
}

/*------------------ Time ------------------*/
z_time_t z_time_now(void) {
    z_time_t now;
    gettimeofday(&now, NULL);
    return now;
}

const char *z_time_now_as_str(char *const buf, unsigned long buflen) {
    z_time_t tv = z_time_now();
    struct tm ts;
    ts = *localtime(&tv.tv_sec);
    strftime(buf, buflen, "%Y-%m-%dT%H:%M:%SZ", &ts);
    return buf;
}

unsigned long z_time_elapsed_us(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

unsigned long z_time_elapsed_ms(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

unsigned long z_time_elapsed_s(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}

int8_t zp_get_time_since_epoch(zp_time_since_epoch *t) {
    z_time_t now;
    gettimeofday(&now, NULL);
    t->secs = now.tv_sec;
    t->nanos = now.tv_usec * 1000;
    return 0;
}
