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

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/time.h>
#include <unistd.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/common/system_error.h"
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

#ifdef CONFIG_ZENOH_PICO_THREADS_NUM
#define Z_THREADS_NUM CONFIG_ZENOH_PICO_THREADS_NUM
#else
#define Z_THREADS_NUM 4
#endif

#ifdef CONFIG_TEST_EXTRA_STACK_SIZE
#define Z_PTHREAD_STACK_SIZE_DEFAULT CONFIG_MAIN_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE
#elif CONFIG_TEST_EXTRA_STACKSIZE
#define Z_PTHREAD_STACK_SIZE_DEFAULT CONFIG_MAIN_STACK_SIZE + CONFIG_TEST_EXTRA_STACKSIZE
#else
#define Z_PTHREAD_STACK_SIZE_DEFAULT CONFIG_MAIN_STACK_SIZE
#endif

K_THREAD_STACK_ARRAY_DEFINE(thread_stack_area, Z_THREADS_NUM, Z_PTHREAD_STACK_SIZE_DEFAULT);
static bool thread_stack_in_use[Z_THREADS_NUM];
static pthread_mutex_t thread_stack_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    void *(*fun)(void *);
    void *arg;
    int stack_slot;
} _z_zephyr_task_ctx_t;

static _z_zephyr_task_ctx_t thread_ctx[Z_THREADS_NUM];

static void _z_zephyr_task_release_stack(int stack_slot) {
    (void)pthread_mutex_lock(&thread_stack_mutex);
    thread_stack_in_use[stack_slot] = false;
    (void)pthread_mutex_unlock(&thread_stack_mutex);
    _Z_DEBUG("zephyr task slot %d released", stack_slot);
}

static pthread_key_t thread_stack_key;
static pthread_once_t thread_stack_key_once = PTHREAD_ONCE_INIT;

static void _z_zephyr_task_stack_destr(void *ctx) {
    if (ctx == NULL) {
        return;
    }
    _z_zephyr_task_ctx_t *tctx = (_z_zephyr_task_ctx_t *)ctx;
    _z_zephyr_task_release_stack(tctx->stack_slot);
}

static void _z_zephyr_task_stack_key_init(void) {
    (void)pthread_key_create(&thread_stack_key, _z_zephyr_task_stack_destr);
}

static void *_z_zephyr_task_entry(void *ctx) {
    _z_zephyr_task_ctx_t *tctx = (_z_zephyr_task_ctx_t *)ctx;
    (void)pthread_once(&thread_stack_key_once, _z_zephyr_task_stack_key_init);
    (void)pthread_setspecific(thread_stack_key, tctx);
    return tctx->fun(tctx->arg);
}

/*------------------ Task ------------------*/
z_result_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    if (attr != NULL) {
        int rc = pthread_create(task, attr, fun, arg);
        if (rc != 0) {
            _z_report_system_error(rc);
            _Z_ERROR_RETURN(_Z_ERR_SYSTEM_GENERIC);
        }
        return _Z_RES_OK;
    }

    int stack_slot = -1;
    (void)pthread_mutex_lock(&thread_stack_mutex);
    for (int i = 0; i < Z_THREADS_NUM; i++) {
        if (!thread_stack_in_use[i]) {
            thread_stack_in_use[i] = true;
            stack_slot = i;
            break;
        }
    }
    (void)pthread_mutex_unlock(&thread_stack_mutex);
    if (stack_slot < 0) {
        _Z_ERROR("zephyr task stack slot OOM: all %d stack slots are in use", Z_THREADS_NUM);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    _Z_DEBUG("zephyr task slot %d allocated for task %p", stack_slot, fun);

    _z_zephyr_task_ctx_t *ctx = &thread_ctx[stack_slot];
    ctx->fun = fun;
    ctx->arg = arg;
    ctx->stack_slot = stack_slot;

    z_task_attr_t tmp;
    (void)pthread_attr_init(&tmp);
    (void)pthread_attr_setstack(&tmp, &thread_stack_area[stack_slot], Z_PTHREAD_STACK_SIZE_DEFAULT);

    int rc = pthread_create(task, &tmp, _z_zephyr_task_entry, ctx);
    if (rc != 0) {
        (void)pthread_mutex_lock(&thread_stack_mutex);
        thread_stack_in_use[stack_slot] = false;
        (void)pthread_mutex_unlock(&thread_stack_mutex);
        _z_report_system_error(rc);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_GENERIC);
    }
    return _Z_RES_OK;
}

z_result_t _z_task_join(_z_task_t *task) { _Z_CHECK_SYS_ERR(pthread_join(*task, NULL)); }

z_result_t _z_task_detach(_z_task_t *task) { _Z_CHECK_SYS_ERR(pthread_detach(*task)); }

z_result_t _z_task_cancel(_z_task_t *task) { _Z_CHECK_SYS_ERR(pthread_cancel(*task)); }

void _z_task_exit(void) { pthread_exit(NULL); }

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
z_result_t _z_mutex_init(_z_mutex_t *m) { _Z_CHECK_SYS_ERR(pthread_mutex_init(m, NULL)); }

z_result_t _z_mutex_drop(_z_mutex_t *m) {
    if (m == NULL) {
        return 0;
    }
    _Z_CHECK_SYS_ERR(pthread_mutex_destroy(m));
}

z_result_t _z_mutex_lock(_z_mutex_t *m) { _Z_CHECK_SYS_ERR(pthread_mutex_lock(m)); }

z_result_t _z_mutex_try_lock(_z_mutex_t *m) { _Z_CHECK_SYS_ERR(pthread_mutex_trylock(m)); }

z_result_t _z_mutex_unlock(_z_mutex_t *m) { _Z_CHECK_SYS_ERR(pthread_mutex_unlock(m)); }

z_result_t _z_mutex_rec_init(_z_mutex_rec_t *m) {
    pthread_mutexattr_t attr;
    _Z_RETURN_IF_SYS_ERR(pthread_mutexattr_init(&attr));
    _Z_RETURN_IF_SYS_ERR(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));
    _Z_RETURN_IF_SYS_ERR(pthread_mutex_init(m, &attr));
    _Z_CHECK_SYS_ERR(pthread_mutexattr_destroy(&attr));
}

z_result_t _z_mutex_rec_drop(_z_mutex_rec_t *m) {
    if (m == NULL) {
        return 0;
    }
    _Z_CHECK_SYS_ERR(pthread_mutex_destroy(m));
}

z_result_t _z_mutex_rec_lock(_z_mutex_rec_t *m) { _Z_CHECK_SYS_ERR(pthread_mutex_lock(m)); }

z_result_t _z_mutex_rec_try_lock(_z_mutex_rec_t *m) { _Z_CHECK_SYS_ERR(pthread_mutex_trylock(m)); }

z_result_t _z_mutex_rec_unlock(_z_mutex_rec_t *m) { _Z_CHECK_SYS_ERR(pthread_mutex_unlock(m)); }

/*------------------ Condvar ------------------*/
z_result_t _z_condvar_init(_z_condvar_t *cv) {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    _Z_CHECK_SYS_ERR(pthread_cond_init(cv, &attr));
}

z_result_t _z_condvar_drop(_z_condvar_t *cv) { _Z_CHECK_SYS_ERR(pthread_cond_destroy(cv)); }

z_result_t _z_condvar_signal(_z_condvar_t *cv) { _Z_CHECK_SYS_ERR(pthread_cond_signal(cv)); }

z_result_t _z_condvar_signal_all(_z_condvar_t *cv) { _Z_CHECK_SYS_ERR(pthread_cond_broadcast(cv)); }

z_result_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) { _Z_CHECK_SYS_ERR(pthread_cond_wait(cv, m)); }

z_result_t _z_condvar_wait_until(_z_condvar_t *cv, _z_mutex_t *m, const z_clock_t *abstime) {
    int error = pthread_cond_timedwait(cv, m, abstime);

    if (error == ETIMEDOUT) {
        return Z_ETIMEDOUT;
    }

    _Z_CHECK_SYS_ERR(error);
}
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
z_result_t z_sleep_us(size_t time) {
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

z_result_t z_sleep_ms(size_t time) {
    int32_t rem = time;
    while (rem > 0) {
        rem = k_msleep(rem);
    }

    return 0;
}

z_result_t z_sleep_s(size_t time) {
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

void z_clock_advance_us(z_clock_t *clock, unsigned long duration) {
    clock->tv_sec += duration / 1000000;
    clock->tv_nsec += (duration % 1000000) * 1000;

    if (clock->tv_nsec >= 1000000000) {
        clock->tv_sec += 1;
        clock->tv_nsec -= 1000000000;
    }
}

void z_clock_advance_ms(z_clock_t *clock, unsigned long duration) {
    clock->tv_sec += duration / 1000;
    clock->tv_nsec += (duration % 1000) * 1000000;

    if (clock->tv_nsec >= 1000000000) {
        clock->tv_sec += 1;
        clock->tv_nsec -= 1000000000;
    }
}

void z_clock_advance_s(z_clock_t *clock, unsigned long duration) { clock->tv_sec += duration; }

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

z_result_t _z_get_time_since_epoch(_z_time_since_epoch *t) {
    z_time_t now;
    gettimeofday(&now, NULL);
    t->secs = now.tv_sec;
    t->nanos = now.tv_usec * 1000;
    return 0;
}
