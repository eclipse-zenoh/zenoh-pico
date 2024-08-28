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

#include <sys/types.h>
#include <windows.h>
// The following includes must come after winsock2
#include <ntsecapi.h>
#include <time.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/result.h"

/*------------------ Random ------------------*/
uint8_t z_random_u8(void) {
    uint8_t ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

uint16_t z_random_u16(void) {
    uint16_t ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

uint32_t z_random_u32(void) {
    uint32_t ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

uint64_t z_random_u64(void) {
    uint64_t ret = 0;
    z_random_fill(&ret, sizeof(ret));
    return ret;
}

void z_random_fill(void *buf, size_t len) { RtlGenRandom(buf, (unsigned long)len); }

/*------------------ Memory ------------------*/
// #define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
// #define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
void *z_malloc(size_t size) { return malloc(size); }

void *z_realloc(void *ptr, size_t size) { return realloc(ptr, size); }

void z_free(void *ptr) { free(ptr); }

#if Z_FEATURE_MULTI_THREAD == 1
/*------------------ Task ------------------*/
int8_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    (void)(attr);
    int8_t ret = _Z_RES_OK;
    *task = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fun, arg, 0, NULL);
    if (*task == NULL) {
        ret = _Z_ERR_SYSTEM_TASK_FAILED;
    }
    return ret;
}

int8_t _z_task_join(_z_task_t *task) {
    int8_t ret = _Z_RES_OK;
    WaitForSingleObject(*task, INFINITE);
    return ret;
}

int8_t _z_task_cancel(_z_task_t *task) {
    int8_t ret = _Z_RES_OK;
    TerminateThread(*task, 0);
    return ret;
}

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    CloseHandle(*ptr);
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
int8_t _z_mutex_init(_z_mutex_t *m) {
    int8_t ret = _Z_RES_OK;
    InitializeSRWLock(m);
    return ret;
}

int8_t _z_mutex_drop(_z_mutex_t *m) {
    (void)(m);
    int8_t ret = _Z_RES_OK;
    return ret;
}

int8_t _z_mutex_lock(_z_mutex_t *m) {
    int8_t ret = _Z_RES_OK;
    AcquireSRWLockExclusive(m);
    return ret;
}

int8_t _z_mutex_try_lock(_z_mutex_t *m) {
    int8_t ret = _Z_RES_OK;
    if (TryAcquireSRWLockExclusive(m) == 0) {
        ret = _Z_ERR_GENERIC;
    }
    return ret;
}

int8_t _z_mutex_unlock(_z_mutex_t *m) {
    int8_t ret = _Z_RES_OK;
    ReleaseSRWLockExclusive(m);
    return ret;
}

/*------------------ Condvar ------------------*/
int8_t _z_condvar_init(_z_condvar_t *cv) {
    int8_t ret = _Z_RES_OK;
    InitializeConditionVariable(cv);
    return ret;
}

int8_t _z_condvar_drop(_z_condvar_t *cv) {
    (void)(cv);
    int8_t ret = _Z_RES_OK;
    return ret;
}

int8_t _z_condvar_signal(_z_condvar_t *cv) {
    int8_t ret = _Z_RES_OK;
    WakeConditionVariable(cv);
    return ret;
}

int8_t _z_condvar_signal_all(_z_condvar_t *cv) {
    int8_t ret = _Z_RES_OK;
    WakeAllConditionVariable(cv);
    return ret;
}

int8_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) {
    int8_t ret = _Z_RES_OK;
    SleepConditionVariableSRW(cv, m, INFINITE, 0);
    return ret;
}
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
int z_sleep_us(size_t time) { return z_sleep_ms((time / 1000) + (time % 1000 == 0 ? 0 : 1)); }

int z_sleep_ms(size_t time) {
    // Guarantees that size_t is split into DWORD segments for Sleep
    uint8_t ratio = sizeof(size_t) / sizeof(DWORD);
    DWORD ratio_time = (DWORD)((time / ratio) + (time % ratio == 0 ? 0 : 1));
    for (uint8_t i = 0; i < ratio; i++) {
        Sleep(ratio_time);
    }
    return 0;
}

int z_sleep_s(size_t time) {
    z_time_t start = z_time_now();

    // Most sleep APIs promise to sleep at least whatever you asked them to.
    // This may compound, so this approach may make sleeps longer than expected.
    // This extra check tries to minimize the amount of extra time it might sleep.
    while (z_time_elapsed_s(&start) < time) {
        z_sleep_ms(1000);
    }

    return 0;
}

/*------------------ Instant ------------------*/
z_clock_t z_clock_now(void) {
    z_clock_t now;
    QueryPerformanceCounter(&now);
    return now;
}

unsigned long z_clock_elapsed_us(z_clock_t *instant) {
    z_clock_t now;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&frequency);  // ticks per second

    // Hardware not supporting QueryPerformanceFrequency
    if (frequency.QuadPart == 0) {
        return 0;
    }
    double elapsed = (double)(now.QuadPart - instant->QuadPart) * 1000000.0;
    elapsed /= frequency.QuadPart;
    return (unsigned long)elapsed;
}

unsigned long z_clock_elapsed_ms(z_clock_t *instant) {
    z_clock_t now;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&frequency);  // ticks per second

    // Hardware not supporting QueryPerformanceFrequency
    if (frequency.QuadPart == 0) {
        return 0;
    }
    double elapsed = (double)(now.QuadPart - instant->QuadPart) * 1000.0;
    elapsed /= frequency.QuadPart;
    return (unsigned long)elapsed;
}

unsigned long z_clock_elapsed_s(z_clock_t *instant) {
    z_clock_t now;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&frequency);  // ticks per second

    // Hardware not supporting QueryPerformanceFrequency
    if (frequency.QuadPart == 0) {
        return 0;
    }
    double elapsed = (double)(now.QuadPart - instant->QuadPart) / frequency.QuadPart;
    return (unsigned long)elapsed;
}

/*------------------ Time ------------------*/
z_time_t z_time_now(void) {
    z_time_t now;
    ftime(&now);
    return now;
}

const char *z_time_now_as_str(char *const buf, unsigned long buflen) {
    z_time_t tv = z_time_now();
    struct tm ts;
    ts = *localtime(&tv.time);
    strftime(buf, buflen, "%Y-%m-%dT%H:%M:%SZ", &ts);
    return buf;
}

unsigned long z_time_elapsed_us(z_time_t *time) { return z_time_elapsed_ms(time) * 1000; }

unsigned long z_time_elapsed_ms(z_time_t *time) {
    z_time_t now;
    ftime(&now);

    unsigned long elapsed = ((unsigned long)(now.time - time->time) * 1000) + (now.millitm - time->millitm);
    return elapsed;
}

unsigned long z_time_elapsed_s(z_time_t *time) {
    z_time_t now;
    ftime(&now);

    unsigned long elapsed = (unsigned long)(now.time - time->time);
    return elapsed;
}

int8_t zp_get_time_since_epoch(zp_time_since_epoch *t) {
    z_time_t now;
    ftime(&now);
    t->secs = (uint32_t)now.time;
    t->nanos = (uint32_t)(now.millitm * 1000000);
    return 0;
}
