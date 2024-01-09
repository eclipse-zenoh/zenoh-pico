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
uint8_t zp_random_u8(void) { return randLIB_get_8bit(); }

uint16_t zp_random_u16(void) { return randLIB_get_16bit(); }

uint32_t zp_random_u32(void) { return randLIB_get_32bit(); }

uint64_t zp_random_u64(void) { return randLIB_get_64bit(); }

void zp_random_fill(void *buf, size_t len) { randLIB_get_n_bytes_random(buf, len); }

/*------------------ Memory ------------------*/
void *zp_malloc(size_t size) { return malloc(size); }

void *zp_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void zp_free(void *ptr) { free(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1
/*------------------ Task ------------------*/
int8_t zp_task_init(zp_task_t *task, zp_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    *task = new Thread();
    mbed::Callback<void()> c = mbed::Callback<void()>(fun, arg);
    return ((Thread *)*task)->start(c);
}

int8_t zp_task_join(zp_task_t *task) {
    int res = ((Thread *)*task)->join();
    delete ((Thread *)*task);
    return res;
}

int8_t zp_task_cancel(zp_task_t *task) {
    int res = ((Thread *)*task)->terminate();
    delete ((Thread *)*task);
    return res;
}

void zp_task_free(zp_task_t **task) {
    zp_task_t *ptr = *task;
    zp_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t zp_mutex_init(zp_mutex_t *m) {
    *m = new Mutex();
    return 0;
}

int8_t zp_mutex_free(zp_mutex_t *m) {
    delete ((Mutex *)*m);
    return 0;
}

int8_t zp_mutex_lock(zp_mutex_t *m) {
    ((Mutex *)*m)->lock();
    return 0;
}

int8_t zp_mutex_trylock(zp_mutex_t *m) { return (((Mutex *)*m)->trylock() == true) ? 0 : -1; }

int8_t zp_mutex_unlock(zp_mutex_t *m) {
    ((Mutex *)*m)->unlock();
    return 0;
}

/*------------------ Condvar ------------------*/
int8_t zp_condvar_init(zp_condvar_t *cv) { return 0; }

int8_t zp_condvar_free(zp_condvar_t *cv) {
    delete ((ConditionVariable *)*cv);
    return 0;
}

int8_t zp_condvar_signal(zp_condvar_t *cv) {
    ((ConditionVariable *)*cv)->notify_all();
    return 0;
}

int8_t zp_condvar_wait(zp_condvar_t *cv, zp_mutex_t *m) {
    *cv = new ConditionVariable(*((Mutex *)*m));
    ((ConditionVariable *)*cv)->wait();
    return 0;
}
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int zp_sleep_us(size_t time) {
    ThisThread::sleep_for(chrono::milliseconds(((time / 1000) + (time % 1000 == 0 ? 0 : 1))));
    return 0;
}

int zp_sleep_ms(size_t time) {
    ThisThread::sleep_for(chrono::milliseconds(time));
    return 0;
}

int zp_sleep_s(size_t time) {
    ThisThread::sleep_for(chrono::seconds(time));
    return 0;
}

/*------------------ Instant ------------------*/
zp_clock_t zp_clock_now(void) {
    // Not supported by default
    return NULL;
}

unsigned long zp_clock_elapsed_us(zp_clock_t *instant) {
    // Not supported by default
    return -1;
}

unsigned long zp_clock_elapsed_ms(zp_clock_t *instant) {
    // Not supported by default
    return -1;
}

unsigned long zp_clock_elapsed_s(zp_clock_t *instant) {
    // Not supported by default
    return -1;
}

/*------------------ Time ------------------*/
zp_time_t zp_time_now(void) {
    zp_time_t now;
    gettimeofday(&now, NULL);
    return now;
}

const char *zp_time_now_as_str(char *const buf, unsigned long buflen) {
    zp_time_t tv = zp_time_now();
    struct tm ts;
    ts = *localtime(&tv.tv_sec);
    strftime(buf, buflen, "%Y-%m-%dT%H:%M:%SZ", &ts);
    return buf;
}

unsigned long zp_time_elapsed_us(zp_time_t *time) {
    zp_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec));
    return elapsed;
}

unsigned long zp_time_elapsed_ms(zp_time_t *time) {
    zp_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (1000 * (now.tv_sec - time->tv_sec) + (now.tv_usec - time->tv_usec) / 1000);
    return elapsed;
}

unsigned long zp_time_elapsed_s(zp_time_t *time) {
    zp_time_t now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = now.tv_sec - time->tv_sec;
    return elapsed;
}

}  // extern "C"