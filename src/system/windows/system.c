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
z_result_t _z_task_init(_z_task_t *task, z_task_attr_t *attr, void *(*fun)(void *), void *arg) {
    (void)(attr);
    z_result_t ret = _Z_RES_OK;
    *task = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)fun, arg, 0, NULL);
    if (*task == NULL) {
        ret = _Z_ERR_SYSTEM_TASK_FAILED;
    }
    return ret;
}

z_result_t _z_task_join(_z_task_t *task) {
    z_result_t ret = _Z_RES_OK;
    WaitForSingleObject(*task, INFINITE);
    return ret;
}

z_result_t _z_task_detach(_z_task_t *task) {
    z_result_t ret = _Z_RES_OK;
    CloseHandle(*task);
    *task = 0;
    return ret;
}

z_result_t _z_task_cancel(_z_task_t *task) {
    z_result_t ret = _Z_RES_OK;
    TerminateThread(*task, 0);
    return ret;
}

void _z_task_exit(void) { ExitThread(0); }

void _z_task_free(_z_task_t **task) {
    _z_task_t *ptr = *task;
    if (*ptr != 0) {
        CloseHandle(*ptr);
    }
    z_free(ptr);
    *task = NULL;
}

/*------------------ Mutex ------------------*/
z_result_t _z_mutex_init(_z_mutex_t *m) {
    z_result_t ret = _Z_RES_OK;
    InitializeSRWLock(m);
    return ret;
}

z_result_t _z_mutex_drop(_z_mutex_t *m) {
    (void)(m);
    z_result_t ret = _Z_RES_OK;
    return ret;
}

z_result_t _z_mutex_lock(_z_mutex_t *m) {
    z_result_t ret = _Z_RES_OK;
    AcquireSRWLockExclusive(m);
    return ret;
}

z_result_t _z_mutex_try_lock(_z_mutex_t *m) {
    z_result_t ret = _Z_RES_OK;
    if (!TryAcquireSRWLockExclusive(m)) {
        ret = _Z_ERR_GENERIC;
    }
    return ret;
}

z_result_t _z_mutex_unlock(_z_mutex_t *m) {
    z_result_t ret = _Z_RES_OK;
    ReleaseSRWLockExclusive(m);
    return ret;
}

z_result_t _z_mutex_rec_init(_z_mutex_rec_t *m) {
    InitializeCriticalSection(m);
    return _Z_RES_OK;
}

z_result_t _z_mutex_rec_drop(_z_mutex_rec_t *m) {
    DeleteCriticalSection(m);
    return _Z_RES_OK;
}

z_result_t _z_mutex_rec_lock(_z_mutex_rec_t *m) {
    EnterCriticalSection(m);
    return _Z_RES_OK;
}

z_result_t _z_mutex_rec_try_lock(_z_mutex_rec_t *m) {
    if (!TryEnterCriticalSection(m)) {
        return _Z_ERR_GENERIC;
    }
    return _Z_RES_OK;
}

z_result_t _z_mutex_rec_unlock(_z_mutex_rec_t *m) {
    LeaveCriticalSection(m);
    return _Z_RES_OK;
}

/*------------------ Condvar ------------------*/
z_result_t _z_condvar_init(_z_condvar_t *cv) {
    z_result_t ret = _Z_RES_OK;
    InitializeConditionVariable(cv);
    return ret;
}

z_result_t _z_condvar_drop(_z_condvar_t *cv) {
    (void)(cv);
    z_result_t ret = _Z_RES_OK;
    return ret;
}

z_result_t _z_condvar_signal(_z_condvar_t *cv) {
    z_result_t ret = _Z_RES_OK;
    WakeConditionVariable(cv);
    return ret;
}

z_result_t _z_condvar_signal_all(_z_condvar_t *cv) {
    z_result_t ret = _Z_RES_OK;
    WakeAllConditionVariable(cv);
    return ret;
}

z_result_t _z_condvar_wait(_z_condvar_t *cv, _z_mutex_t *m) {
    z_result_t ret = _Z_RES_OK;
    SleepConditionVariableSRW(cv, m, INFINITE, 0);
    return ret;
}

z_result_t _z_condvar_wait_until(_z_condvar_t *cv, _z_mutex_t *m, const z_clock_t *abstime) {
    z_clock_t now = z_clock_now();
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);  // ticks per second

    // Hardware not supporting QueryPerformanceFrequency
    if (frequency.QuadPart == 0) {
        return _Z_ERR_GENERIC;
    }

    double remaining = (double)(abstime->QuadPart - now.QuadPart) / frequency.QuadPart * 1000.0;
    DWORD block_duration = remaining > 0.0 ? (DWORD)remaining : 0;

    if (SleepConditionVariableSRW(cv, m, block_duration, 0) == 0) {
        if (GetLastError() == ERROR_TIMEOUT) {
            return Z_ETIMEDOUT;
        } else {
            return _Z_ERR_GENERIC;
        }
    }

    return _Z_RES_OK;
}
#endif  // Z_FEATURE_MULTI_THREAD == 1

/*------------------ Sleep ------------------*/
z_result_t z_sleep_us(size_t time) { return z_sleep_ms((time / 1000) + (time % 1000 == 0 ? 0 : 1)); }

z_result_t z_sleep_ms(size_t time) {
    // Guarantees that size_t is split into DWORD segments for Sleep
    uint8_t ratio = sizeof(size_t) / sizeof(DWORD);
    DWORD ratio_time = (DWORD)((time / ratio) + (time % ratio == 0 ? 0 : 1));
    for (uint8_t i = 0; i < ratio; i++) {
        Sleep(ratio_time);
    }
    return 0;
}

z_result_t z_sleep_s(size_t time) {
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

void z_clock_advance_us(z_clock_t *clock, unsigned long duration) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);  // ticks per second

    // Hardware not supporting QueryPerformanceFrequency
    if (frequency.QuadPart == 0) {
        return;
    }
    double ticks = (double)duration * frequency.QuadPart / 1000000.0;
    clock->QuadPart += (LONGLONG)ticks;
}

void z_clock_advance_ms(z_clock_t *clock, unsigned long duration) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);  // ticks per second

    // Hardware not supporting QueryPerformanceFrequency
    if (frequency.QuadPart == 0) {
        return;
    }
    double ticks = (double)duration * frequency.QuadPart / 1000.0;
    clock->QuadPart += (LONGLONG)ticks;
}

void z_clock_advance_s(z_clock_t *clock, unsigned long duration) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);  // ticks per second

    // Hardware not supporting QueryPerformanceFrequency
    if (frequency.QuadPart == 0) {
        return;
    }
    double ticks = (double)duration * frequency.QuadPart;
    clock->QuadPart += (LONGLONG)ticks;
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

z_result_t _z_get_time_since_epoch(_z_time_since_epoch *t) {
    z_time_t now;
    ftime(&now);
    t->secs = (uint32_t)now.time;
    t->nanos = (uint32_t)(now.millitm * 1000000);
    return 0;
}
