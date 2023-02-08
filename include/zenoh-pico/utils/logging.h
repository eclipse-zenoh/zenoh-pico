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

static inline void __z_print_timestamp(void) {
    char ret[64];
    printf("[%s ", z_time_now_as_str(ret, sizeof(ret)));
}

#if (ZENOH_DEBUG == 3)
#define _Z_DEBUG(x, ...)              \
    __z_print_timestamp();            \
    printf("DEBUG ::%s] ", __func__); \
    printf(x, ##__VA_ARGS__);
#define _Z_INFO(x, ...)              \
    __z_print_timestamp();           \
    printf("INFO ::%s] ", __func__); \
    printf(x, ##__VA_ARGS__);
#define _Z_ERROR(x, ...)              \
    __z_print_timestamp();            \
    printf("ERROR ::%s] ", __func__); \
    printf(x, ##__VA_ARGS__);

#elif (ZENOH_DEBUG == 2)
#define _Z_DEBUG(x, ...) (void)(0)
#define _Z_INFO(x, ...)              \
    __z_print_timestamp();           \
    printf("INFO ::%s] ", __func__); \
    printf(x, ##__VA_ARGS__);
#define _Z_ERROR(x, ...)              \
    __z_print_timestamp();            \
    printf("ERROR ::%s] ", __func__); \
    printf(x, ##__VA_ARGS__);

#elif (ZENOH_DEBUG == 1)
#define _Z_DEBUG(x, ...) (void)(0)
#define _Z_INFO(x, ...) (void)(0)
#define _Z_ERROR(x, ...)              \
    __z_print_timestamp();            \
    printf("ERROR ::%s] ", __func__); \
    printf(x, ##__VA_ARGS__);

#elif (ZENOH_DEBUG == 0)
#define _Z_DEBUG(x, ...) (void)(0)
#define _Z_INFO(x, ...) (void)(0)
#define _Z_ERROR(x, ...) (void)(0)
#endif

#endif /* ZENOH_PICO_UTILS_LOGGING_H */
