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
int8_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    *task = new Thread();
    mbed::Callback<void()> c = mbed::Callback<void()>(fun, arg);
    return ((Thread *)*task)->start(c);
}

int8_t _z_task_join(_z_task_t *task) {
    int res = ((Thread *)*task)->join();
    delete ((Thread *)*task);
    return res;
}

int8_t _z_task_cancel(_z_task_t *task) {
    int res = ((Thread *)*task)->terminate();
    delete ((Thread *)*task);
    return res;
}

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t _z_mutex_init(_z_mutex_t *m) {
    *m = new Mutex();
    return 0;
}

int8_t _z_mutex_drop(_z_mutex_t *m) {
    delete ((Mutex *)*m);
    return 0;
}

int8_t _z_mutex_lock(_z_mutex_t *m) {
    ((Mutex *)*m)->lock();
    return 0;
}

int8_t _z_mutex_try_lock(_z_mutex_t *m) { return (((Mutex *)*m)->trylock() == true) ? 0 : -1; }

int8_t _z_mutex_unlock(_z_mutex_t *m) {
    ((Mutex *)*m)->unlock();
    return 0;
}

/*------------------ Condvar ------------------*/
int8_t _z_condvar_init(_z_condvar_t *cv) { return 0; }

int8_t _z_condvar_drop(_z_condvar_t *cv) {
    delete ((ConditionVariable *)*cv);
    return 0;
}

int8_t _z_condvar_signal(_z_condvar_t *cv) {
    ((ConditionVariable *)*cv)->notify_one();
    return 0;
}

int8_t _z_condvar_signal_all(_z_condvar_t *cv) {
    ((ConditionVariable *)*cv)->notify_all();
    return 0;
}

int8_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) {
    *cv = new ConditionVariable(*((Mutex *)*m));
    ((ConditionVariable *)*cv)->wait();
    return 0;
}
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time) {
    ThisThread::sleep_for(chrono::milliseconds(((time / 1000) + (time % 1000 == 0 ? 0 : 1))));
    return 0;
}

int z_sleep_ms(size_t time) {
    ThisThread::sleep_for(chrono::milliseconds(time));
    return 0;
}

int z_sleep_s(size_t time) {
    ThisThread::sleep_for(chrono::seconds(time));
    return 0;
}

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) {
    // Not supported by default
    return NULL;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    // Not supported by default
    return -1;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    // Not supported by default
    return -1;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    // Not supported by default
    return -1;
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

}  // extern "C"
