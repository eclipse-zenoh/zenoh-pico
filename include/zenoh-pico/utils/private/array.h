/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef _ZENOH_PICO_UTILS_ARRAY_H
#define _ZENOH_PICO_UTILS_ARRAY_H

#include <stdint.h>
#include <stdio.h>
#include "zenoh-pico/utils/types.h"

/*------------------ Internal Array Macros ------------------*/
#define _ARRAY_DECLARE(type, name, prefix) \
    typedef struct                         \
    {                                      \
        size_t len;                        \
        type *val;                         \
    } prefix##name##_array_t;

#define _ARRAY_P_DECLARE(type, name, prefix) \
    typedef struct                           \
    {                                        \
        size_t len;                          \
        type **val;                          \
    } prefix##name##_p_array_t;

#define _ARRAY_S_DEFINE(type, name, prefix, arr, len) \
    prefix##name##_array_t arr = {len, (type *)malloc(len * sizeof(type))};

#define _ARRAY_P_S_DEFINE(type, name, prefix, arr, len) \
    prefix##name##_array_t arr = {len, (type **)malloc(len * sizeof(type *))};

#define _ARRAY_H_DEFINE(T, arr, len)                               \
    z_array_##T *arr = (z_array_##T *)malloc(sizeof(z_array_##T)); \
    arr->len = len;                                                \
    arr->val = (T *)malloc(len * sizeof(T));

#define _ARRAY_S_INIT(T, arr, len) \
    arr.len = len;                 \
    arr.val = (T *)malloc(len * sizeof(T));

#define _ARRAY_P_S_INIT(T, arr, len) \
    arr.len = len;                   \
    arr.val = (T **)malloc(len * sizeof(T *));

#define _ARRAY_H_INIT(T, arr, len) \
    arr->len = len;                \
    arr->val = (T *)malloc(len * sizeof(T))

#define _ARRAY_S_COPY(T, dst, src)              \
    dst.len = src.len;                          \
    dst.val = (T *)malloc(dst.len * sizeof(T)); \
    memcpy(dst.val, src.val, dst.len);

#define _ARRAY_H_COPY(T, dst, src)                \
    dst->len = src->len;                          \
    dst->val = (T *)malloc(dst->len * sizeof(T)); \
    memcpy(dst->val, src->val, dst->len);

#define _ARRAY_S_FREE(arr) \
    free(arr.val);         \
    arr.val = 0;           \
    arr.len = 0;

#define _ARRAY_H_FREE(arr) \
    free(arr->val);        \
    arr->val = 0;          \
    arr->len = 0

#endif /* _ZENOH_PICO_UTILS_ARRAY_H */
