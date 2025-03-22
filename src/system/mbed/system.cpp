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
#include <stddef.h>

#include "zenoh-pico/config.h"

extern "C" {
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { return randLIB_get_8bit(); }

uint16_t z_random_u16(void) { return randLIB_get_16bit(); }

uint32_t z_random_u32(void) { return randLIB_get_32bit(); }

uint64_t z_random_u64(void) { return randLIB_get_64bit(); }

void z_random_fill(void *buf, size_t len) { randLIB_get_n_bytes_random(buf, len); }

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) { return malloc(size); }

void *z_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void z_free(void *ptr) { free(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1
/*------------------ Task ------------------*/
z_result_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    *task = new Thread();
    mbed::Callback<void()> c = mbed::Callback<void()>(fun, arg);
    return ((Thread *)*task)->start(c);
}

z_result_t _z_task_join(_z_task_t *task) {
    int res = ((Thread *)*task)->join();
    delete ((Thread *)*task);
    return res;
}

z_result_t _z_task_detach(_z_task_t *task) {
    // Not implemented
    return _Z_ERR_GENERIC;
}

z_result_t _z_task_cancel(_z_task_t *task) {
    int res = ((Thread *)*task)->terminate();
    delete ((Thread *)*task);
    return res;
}

void _z_task_exit(void) {}

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
z_result_t _z_mutex_init(_z_mutex_t *m) {
    *m = new Mutex();
    return 0;
}

z_result_t _z_mutex_drop(_z_mutex_t *m) {
    delete ((Mutex *)*m);
    return 0;
}

z_result_t _z_mutex_lock(_z_mutex_t *m) {
    ((Mutex *)*m)->lock();
    return 0;
}

z_result_t _z_mutex_try_lock(_z_mutex_t *m) { return (((Mutex *)*m)->trylock() == true) ? 0 : -1; }

z_result_t _z_mutex_unlock(_z_mutex_t *m) {
    ((Mutex *)*m)->unlock();
    return 0;
}

z_result_t _z_mutex_rec_init(_z_mutex_rec_t *m) { return _z_mutex_init(m); }

z_result_t _z_mutex_rec_drop(_z_mutex_rec_t *m) { return _z_mutex_drop(m); }

z_result_t _z_mutex_rec_lock(_z_mutex_rec_t *m) { return _z_mutex_lock(m); }

z_result_t _z_mutex_rec_try_lock(_z_mutex_rec_t *m) { return _z_mutex_try_lock(m); }

z_result_t _z_mutex_rec_unlock(_z_mutex_rec_t *m) { return _z_mutex_unlock(m); }

/*------------------ Condvar ------------------*/
struct condvar {
    Mutex mutex;
    Semaphore sem{0};
    int waiters{0};
};

z_result_t _z_condvar_init(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }

    *cv = new condvar();
    return 0;
}

z_result_t _z_condvar_drop(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }

    delete ((condvar *)*cv);
    return 0;
}

z_result_t _z_condvar_signal(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }

    auto &cond_var = *(condvar *)*cv;

    cond_var.mutex.lock();
    if (cond_var.waiters > 0) {
        cond_var.sem.release();
        cond_var.waiters--;
    }
    cond_var.mutex.unlock();

    return _Z_RES_OK;
}

z_result_t _z_condvar_signal_all(_z_condvar_t *cv) {
    if (!cv) {
        return _Z_ERR_GENERIC;
    }

    auto &cond_var = *(condvar *)*cv;

    cond_var.mutex.lock();
    while (cond_var.waiters > 0) {
        cond_var.sem.release();
        cond_var.waiters--;
    }
    cond_var.mutex.unlock();

    return _Z_RES_OK;
}

z_result_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) {
    if (!cv || !m) {
        return _Z_ERR_GENERIC;
    }

    auto &cond_var = *(condvar *)*cv;

    cond_var.mutex.lock();
    cond_var.waiters++;
    cond_var.mutex.unlock();

    _z_mutex_unlock(m);

    cond_var.sem.acquire();

    _z_mutex_lock(m);

    return _Z_RES_OK;
}

z_result_t _z_condvar_wait_until(_z_condvar_t *cv, _z_mutex_t *m, const z_clock_t *abstime) {
    if (!cv || !m) {
        return _Z_ERR_GENERIC;
    }

    auto &cond_var = *(condvar *)*cv;

    auto target_time =
        Kernel::Clock::time_point(Kernel::Clock::duration(abstime->tv_sec * 1000LL + abstime->tv_nsec / 1000000));

    cond_var.mutex.lock();
    cond_var.waiters++;
    cond_var.mutex.unlock();

    _z_mutex_unlock(m);

    bool timed_out = cond_var.sem.try_acquire_until(target_time) == false;

    _z_mutex_lock(m);

    if (timed_out) {
        cond_var.mutex.lock();
        cond_var.waiters--;
        cond_var.mutex.unlock();
        return Z_ETIMEDOUT;
    }

    return _Z_RES_OK;
}
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
z_result_t z_sleep_us(size_t time) {
    ThisThread::sleep_for(chrono::milliseconds(((time / 1000) + (time % 1000 == 0 ? 0 : 1))));
    return 0;
}

z_result_t z_sleep_ms(size_t time) {
    ThisThread::sleep_for(chrono::milliseconds(time));
    return 0;
}

z_result_t z_sleep_s(size_t time) {
    ThisThread::sleep_for(chrono::seconds(time));
    return 0;
}

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) {
    auto now = Kernel::Clock::now();
    auto duration = now.time_since_epoch();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - secs);

    z_clock_t ts;
    ts.tv_sec = secs.count();
    ts.tv_nsec = nanos.count();
    return ts;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now = z_clock_now();
    unsigned long elapsed =
        (unsigned long)(1000000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000);
    return elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now = z_clock_now();
    unsigned long elapsed =
        (unsigned long)(1000 * (now.tv_sec - instant->tv_sec) + (now.tv_nsec - instant->tv_nsec) / 1000000);
    return elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now = z_clock_now();
    unsigned long elapsed = (unsigned long)(now.tv_sec - instant->tv_sec);
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

}  // extern "C"
