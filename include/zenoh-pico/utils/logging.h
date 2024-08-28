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

#include "zenoh-pico/system/platform-common.h"

// Logging values
#define _Z_LOG_LVL_ERROR 1
#define _Z_LOG_LVL_INFO 2
#define _Z_LOG_LVL_DEBUG 3

// Timestamp function
static inline void __z_print_timestamp(void) {
    char ret[64];
    printf("[%s ", z_time_now_as_str(ret, sizeof(ret)));
}

// Logging macros
#define _Z_LOG_PREFIX(prefix) \
    __z_print_timestamp();    \
    printf(#prefix " ::%s] ", __func__);

// Ignore print only if log deactivated and build is release
#if ZENOH_DEBUG == 0 && !defined(Z_BUILD_DEBUG)

#define _Z_DEBUG(...) (void)(0)
#define _Z_INFO(...) (void)(0)
#define _Z_ERROR(...) (void)(0)

#else  // ZENOH_DEBUG != 0 || defined(Z_BUILD_DEBUG)

#define _Z_DEBUG(...)                          \
    do {                                       \
        if (ZENOH_DEBUG >= _Z_LOG_LVL_DEBUG) { \
            _Z_LOG_PREFIX(DEBUG);              \
            printf(__VA_ARGS__);               \
            printf("\r\n");                    \
        }                                      \
    } while (false)

#define _Z_INFO(...)                          \
    do {                                      \
        if (ZENOH_DEBUG >= _Z_LOG_LVL_INFO) { \
            _Z_LOG_PREFIX(INFO);              \
            printf(__VA_ARGS__);              \
            printf("\r\n");                   \
        }                                     \
    } while (false)

#define _Z_ERROR(...)                          \
    do {                                       \
        if (ZENOH_DEBUG >= _Z_LOG_LVL_ERROR) { \
            _Z_LOG_PREFIX(ERROR);              \
            printf(__VA_ARGS__);               \
            printf("\r\n");                    \
        }                                      \
    } while (false)
#endif  // ZENOH_DEBUG == 0 && !defined(Z_BUILD_DEBUG)

#endif  // ZENOH_PICO_UTILS_LOGGING_H
