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

#include <windows.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) { return 0; }

uint16_t z_random_u16(void) { return 0; }

uint32_t z_random_u32(void) { return 0; }

uint64_t z_random_u64(void) { return 0; }

void z_random_fill(void *buf, size_t len) {
    (void)(buf);
    (void)(len);
}

/*------------------ Memory ------------------*/
void *z_malloc(size_t size) {
    (void)(size);
    return 0;
}

void *z_realloc(void *ptr, size_t size) {
    (void)(ptr);
    (void)(size);
    return 0;
}

void z_free(void *ptr) {
    (void)(ptr);
    return 0;
}

#if Z_MULTI_THREAD == 1
/*------------------ Task ------------------*/
int8_t _z_task_init(_z_task_t *task, _z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    (void)(task);
    (void)(attr);
    (void)(fun);
    (void)(arg);
    return -1;
}

int8_t _z_task_join(_z_task_t *task) {
    (void)(task);
    return -1;
}

int8_t _z_task_cancel(_z_task_t *task) {
    (void)(task);
    return -1;
}

void _z_task_free(_z_task_t **task) { (void)(task); }

/*------------------ Mutex ------------------*/
int8_t _z_mutex_init(_z_mutex_t *m) {
    (void)(m);
    return -1;
}

int8_t _z_mutex_free(_z_mutex_t *m) {
    (void)(m);
    return -1;
}

int8_t _z_mutex_lock(_z_mutex_t *m) {
    (void)(m);
    return -1;
}

int8_t _z_mutex_trylock(_z_mutex_t *m) {
    (void)(m);
    return -1;
}

int8_t _z_mutex_unlock(_z_mutex_t *m) {
    (void)(m);
    return -1;
}

/*------------------ Condvar ------------------*/
int8_t _z_condvar_init(_z_condvar_t *cv) {
    (void)(cv);
    return -1;
}

int8_t _z_condvar_free(_z_condvar_t *cv) {
    (void)(cv);
    return -1;
}

int8_t _z_condvar_signal(_z_condvar_t *cv) {
    (void)(cv);
    return -1;
}

int8_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) {
    (void)(cv);
    return -1;
}
#endif  // Z_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(unsigned int time) {
    (void)(time);
    return 0;
}

int z_sleep_ms(unsigned int time) {
    (void)(time);
    return 0;
}

int z_sleep_s(unsigned int time) {
    (void)(time);
    return 0;
}

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) { return NULL; }

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    (void)(instant);
    return 0;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    (void)(instant);
    return 0;
}
unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    (void)(instant);
    return 0;
}

/*------------------ Time ------------------*/
z_time_t z_time_now(void) { return NULL; }

unsigned long z_time_elapsed_us(z_time_t *time) {
    (void)(time);
    return 0;
}

unsigned long z_time_elapsed_ms(z_time_t *time) {
    (void)(time);
    return 0;
}

unsigned long z_time_elapsed_s(z_time_t *time) {
    (void)(time);
    return 0;
}
