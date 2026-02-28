//
// Copyright (c) 2024 ZettaScale Technology
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

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <rtthread.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { 
    return (uint8_t)(rt_tick_get() & 0xFF); 
}

uint16_t z_random_u16(void) { 
    return (uint16_t)(rt_tick_get() & 0xFFFF); 
}

uint32_t z_random_u32(void) {
    static uint32_t seed = 1;
    seed = seed * 1103515245 + 12345;
    return seed ^ rt_tick_get();
}

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
    ret |= z_random_u32();
    ret = ret << 32;
    ret |= z_random_u32();
    return ret;
}

void z_random_fill(void *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        *((uint8_t *)buf + i) = z_random_u8();
    }
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    return rt_malloc(size);
}

void *z_realloc(void *ptr, size_t size) {
    return rt_realloc(ptr, size);
}

void z_free(void *ptr) { 
    rt_free(ptr); 
}

#if Z_FEATURE_MULTI_THREAD == 1
/*------------------ Thread ------------------*/
static void z_task_wrapper(void *arg) {
    _z_task_t *task = (_z_task_t *)arg;

    // Run the task function
    task->fun(task->arg);

    // Notify the joiner that the task has finished
    rt_event_send(task->join_event, 1);

    // Exit the thread
    rt_thread_exit();
}

static z_task_attr_t z_default_task_attr = {
    .name = "zenoh_task",
    .priority = RT_THREAD_PRIORITY_MAX / 2,
    .stack_size = 4096,
};

/*------------------ Thread ------------------*/
z_result_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    task->fun = fun;
    task->arg = arg;

    if (attr == NULL) {
        attr = &z_default_task_attr;
    }

    // Create join event
    task->join_event = rt_event_create("zenoh_join", RT_IPC_FLAG_FIFO);
    if (task->join_event == RT_NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    // Create and start thread
    task->handle = rt_thread_create(attr->name, z_task_wrapper, task, 
                                   attr->stack_size, attr->priority, 20);
    if (task->handle == RT_NULL) {
        rt_event_delete(task->join_event);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    if (rt_thread_startup(task->handle) != RT_EOK) {
        rt_thread_delete(task->handle);
        rt_event_delete(task->join_event);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

z_result_t _z_task_join(_z_task_t *task) {
    rt_uint32_t recved;
    
    // Wait for task completion
    if (rt_event_recv(task->join_event, 1, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                     RT_WAITING_FOREVER, &recved) != RT_EOK) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    // Clean up thread handle
    if (task->handle != RT_NULL) {
        rt_thread_delete(task->handle);
        task->handle = RT_NULL;
    }

    return _Z_RES_OK;
}

z_result_t _z_task_detach(_z_task_t *task) {
    // RT-Thread doesn't need explicit detach, just clean up
    if (task->handle != RT_NULL) {
        task->handle = RT_NULL;
    }
    return _Z_RES_OK;
}

z_result_t _z_task_cancel(_z_task_t *task) {
    if (task->handle != RT_NULL) {
        rt_thread_delete(task->handle);
        task->handle = RT_NULL;
    }

    // Signal join event
    rt_event_send(task->join_event, 1);

    return _Z_RES_OK;
}

void _z_task_exit(void) { 
    rt_thread_exit(); 
}

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    if (ptr->join_event != RT_NULL) {
        rt_event_delete(ptr->join_event);
    }
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
z_result_t _z_mutex_init(_z_mutex_t *m) {
    m->handle = rt_mutex_create("zenoh_mutex", RT_IPC_FLAG_FIFO);
    return m->handle == RT_NULL ? _Z_ERR_GENERIC : _Z_RES_OK;
}

z_result_t _z_mutex_drop(_z_mutex_t *m) {
    rt_mutex_delete(m->handle);
    return _Z_RES_OK;
}

z_result_t _z_mutex_lock(_z_mutex_t *m) {
    return rt_mutex_take(m->handle, RT_WAITING_FOREVER) == RT_EOK ? _Z_RES_OK : _Z_ERR_GENERIC;
}

z_result_t _z_mutex_try_lock(_z_mutex_t *m) {
    return rt_mutex_take(m->handle, 0) == RT_EOK ? _Z_RES_OK : _Z_ERR_GENERIC;
}

z_result_t _z_mutex_unlock(_z_mutex_t *m) {
    return rt_mutex_release(m->handle) == RT_EOK ? _Z_RES_OK : _Z_ERR_GENERIC;
}

z_result_t _z_mutex_rec_init(_z_mutex_rec_t *m) { 
    return _z_mutex_init(m); 
}

z_result_t _z_mutex_rec_drop(_z_mutex_rec_t *m) { 
    return _z_mutex_drop(m); 
}

z_result_t _z_mutex_rec_lock(_z_mutex_rec_t *m) { 
    return _z_mutex_lock(m); 
}

z_result_t _z_mutex_rec_try_lock(_z_mutex_rec_t *m) { 
    return _z_mutex_try_lock(m); 
}

z_result_t _z_mutex_rec_unlock(_z_mutex_rec_t *m) { 
    return _z_mutex_unlock(m); 
}

/*------------------ CondVar ------------------*/
z_result_t _z_condvar_init(_z_condvar_t *cv) {
    if (!cv) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    cv->mutex = rt_mutex_create("zenoh_cv_mutex", RT_IPC_FLAG_FIFO);
    cv->sem = rt_sem_create("zenoh_cv_sem", 0, RT_IPC_FLAG_FIFO);
    cv->waiters = 0;

    if (!cv->mutex || !cv->sem) {
        if (cv->mutex) rt_mutex_delete(cv->mutex);
        if (cv->sem) rt_sem_delete(cv->sem);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    return _Z_RES_OK;
}

z_result_t _z_condvar_drop(_z_condvar_t *cv) {
    if (!cv) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    rt_sem_delete(cv->sem);
    rt_mutex_delete(cv->mutex);
    return _Z_RES_OK;
}

z_result_t _z_condvar_signal(_z_condvar_t *cv) {
    if (!cv) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    rt_mutex_take(cv->mutex, RT_WAITING_FOREVER);
    if (cv->waiters > 0) {
        rt_sem_release(cv->sem);
        cv->waiters--;
    }
    rt_mutex_release(cv->mutex);

    return _Z_RES_OK;
}

z_result_t _z_condvar_signal_all(_z_condvar_t *cv) {
    if (!cv) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    rt_mutex_take(cv->mutex, RT_WAITING_FOREVER);
    while (cv->waiters > 0) {
        rt_sem_release(cv->sem);
        cv->waiters--;
    }
    rt_mutex_release(cv->mutex);

    return _Z_RES_OK;
}

z_result_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) {
    if (!cv || !m) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    rt_mutex_take(cv->mutex, RT_WAITING_FOREVER);
    cv->waiters++;
    rt_mutex_release(cv->mutex);

    _z_mutex_unlock(m);

    rt_sem_take(cv->sem, RT_WAITING_FOREVER);

    _z_mutex_lock(m);

    return _Z_RES_OK;
}

z_result_t _z_condvar_wait_until(_z_condvar_t *cv, _z_mutex_t *m, const z_clock_t *abstime) {
    if (!cv || !m) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    rt_tick_t now = rt_tick_get();
    rt_tick_t target_time = *abstime;
    rt_tick_t timeout = (target_time > now) ? (target_time - now) : 0;

    rt_mutex_take(cv->mutex, RT_WAITING_FOREVER);
    cv->waiters++;
    rt_mutex_release(cv->mutex);

    _z_mutex_unlock(m);

    rt_err_t result = rt_sem_take(cv->sem, timeout);

    _z_mutex_lock(m);

    if (result != RT_EOK) {
        rt_mutex_take(cv->mutex, RT_WAITING_FOREVER);
        cv->waiters--;
        rt_mutex_release(cv->mutex);
        return Z_ETIMEDOUT;
    }

    return _Z_RES_OK;
}
#endif  // Z_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
z_result_t z_sleep_us(size_t time) {
    rt_thread_mdelay(time / 1000);
    return _Z_RES_OK;
}

z_result_t z_sleep_ms(size_t time) {
    rt_thread_mdelay(time);
    return _Z_RES_OK;
}

z_result_t z_sleep_s(size_t time) {
    rt_thread_mdelay(time * 1000);
    return _Z_RES_OK;
}

/*------------------ Clock ------------------*/
z_clock_t z_clock_now(void) { 
    return rt_tick_get(); 
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) { 
    return z_clock_elapsed_ms(instant) * 1000; 
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now = z_clock_now();
    unsigned long elapsed = (now - *instant) * (1000 / RT_TICK_PER_SECOND);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) { 
    return z_clock_elapsed_ms(instant) / 1000; 
}

void z_clock_advance_us(z_clock_t *clock, unsigned long duration) { 
    z_clock_advance_ms(clock, duration / 1000); 
}

void z_clock_advance_ms(z_clock_t *clock, unsigned long duration) {
    unsigned long ticks = (duration * RT_TICK_PER_SECOND) / 1000;
    *clock += ticks;
}

void z_clock_advance_s(z_clock_t *clock, unsigned long duration) { 
    z_clock_advance_ms(clock, duration * 1000); 
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

    unsigned long elapsed = (unsigned long)(1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

unsigned long z_time_elapsed_ms(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (unsigned long)(1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

unsigned long z_time_elapsed_s(z_time_t *time) {
    z_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (unsigned long)(now.tv_sec - time->tv_sec);
    return elapsed;
}

z_result_t _z_get_time_since_epoch(_z_time_since_epoch *t) {
    z_time_t now;
    gettimeofday(&now, NULL);
    t->secs = (uint32_t)now.tv_sec;
    t->nanos = (uint32_t)now.tv_usec * 1000;
    return _Z_RES_OK;
}