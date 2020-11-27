
/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#ifndef _ZENOH_PICO_TYPES_H
#define _ZENOH_PICO_TYPES_H

#include <stdint.h>
#include "zenoh-pico/private/system.h"

/*------------------ Collection ------------------*/
typedef struct
{
    size_t _capacity;
    size_t _len;
    void **_val;
} _z_vec_t;

typedef int (*_z_list_predicate)(void *, void *);
typedef struct z_list
{
    void *val;
    struct z_list *tail;
} _z_list_t;

typedef struct
{
    size_t key;
    void *value;
} _z_i_map_entry_t;

typedef struct
{
    _z_list_t **vals;
    size_t capacity;
    size_t len;
} _z_i_map_t;

typedef struct
{
    void *elem;
    int full;
    _z_mutex_t mtx;
    _z_condvar_t can_put;
    _z_condvar_t can_get;
} _z_mvar_t;

/*------------------ IOBuf ------------------*/
typedef struct
{
    size_t r_pos;
    size_t w_pos;
    size_t capacity;
    int is_alloc;
    uint8_t *buf;
} _z_iosli_t;

typedef struct
{
    _z_iosli_t ios;
} _z_rbuf_t;

typedef struct
{
    size_t r_idx;
    size_t w_idx;
    size_t capacity;
    _z_vec_t ioss;
    int is_expandable;
} _z_wbuf_t;

#endif /* _ZENOH_PICO_TYPES_H */
