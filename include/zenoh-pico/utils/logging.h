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

#ifndef ZENOH_PICO_UTILS_LOGGING_H
#define ZENOH_PICO_UTILS_LOGGING_H

#include <stdio.h>

#include "zenoh-pico/system/common/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ZENOH_LOG_LEVEL
#define ZENOH_LOG_LEVEL 0
#endif

// Allows replacement of printf on targets that don't support it
#ifndef ZENOH_LOG_PRINT
#define ZENOH_LOG_PRINT printf
#endif

// Compatibility with legacy logging system where
// 1 -> ERROR
// 2 -> INFO
// 3 -> DEBUG
#ifdef ZENOH_DEBUG
#undef ZENOH_LOG_LEVEL
#define ZENOH_LOG_LEVEL (ZENOH_DEBUG * 10 + (ZENOH_DEBUG >= 2 ? 10 : 0))
#endif

// Logging values
#define _Z_LOG_LVL_ERROR 10
#define _Z_LOG_LVL_WARN 20
#define _Z_LOG_LVL_INFO 30
#define _Z_LOG_LVL_DEBUG 40
#define _Z_LOG_LVL_TRACE 50

// Timestamp function
static inline void __z_print_timestamp(void) {
    char ret[64];
    ZENOH_LOG_PRINT("[%s ", z_time_now_as_str(ret, sizeof(ret)));
}

// Release build only expand log code when the given level is enabled,
// while debug build always expand log code, to ensure it compiles fine.
#if ZENOH_LOG_LEVEL >= 0 || defined(Z_BUILD_LOG)
#define _Z_LOG(level, ...)                               \
    do {                                                 \
        if (ZENOH_LOG_LEVEL >= _Z_LOG_LVL_##level) {     \
            __z_print_timestamp();                       \
            ZENOH_LOG_PRINT(#level " ::%s] ", __func__); \
            ZENOH_LOG_PRINT(__VA_ARGS__);                \
            ZENOH_LOG_PRINT("\r\n");                     \
        }                                                \
    } while (false)
#else
#define _Z_LOG(level, ...) (void)(0)
#endif

#if ZENOH_LOG_LEVEL >= _Z_LOG_LVL_ERROR || defined(Z_BUILD_LOG)
#define _Z_ERROR(...) _Z_LOG(ERROR, __VA_ARGS__)
#else
#define _Z_ERROR(...) (void)(0)
#endif

#if ZENOH_LOG_LEVEL >= _Z_LOG_LVL_WARN || defined(Z_BUILD_LOG)
#define _Z_WARN(...) _Z_LOG(WARN, __VA_ARGS__)
#else
#define _Z_WARN(...) (void)(0)
#endif

#if ZENOH_LOG_LEVEL >= _Z_LOG_LVL_INFO || defined(Z_BUILD_LOG)
#define _Z_INFO(...) _Z_LOG(INFO, __VA_ARGS__)
#else
#define _Z_INFO(...) (void)(0)
#endif

#if ZENOH_LOG_LEVEL >= _Z_LOG_LVL_DEBUG || defined(Z_BUILD_LOG)
#define _Z_DEBUG(...) _Z_LOG(DEBUG, __VA_ARGS__)
#else
#define _Z_DEBUG(...) (void)(0)
#endif

#if ZENOH_LOG_LEVEL >= _Z_LOG_LVL_TRACE || defined(Z_BUILD_LOG)
#define _Z_TRACE(...) _Z_LOG(TRACE, __VA_ARGS__)
#else
#define _Z_TRACE(...) (void)(0)
#endif

#ifdef __cplusplus
}
#endif

#endif  // ZENOH_PICO_UTILS_LOGGING_H
