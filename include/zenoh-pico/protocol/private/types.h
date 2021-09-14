
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

#ifndef _ZENOH_PICO_PROTOCOL_PRIVATE_TYPES_H
#define _ZENOH_PICO_PROTOCOL_PRIVATE_TYPES_H

#include "zenoh-pico/protocol/types.h"
#include "zenoh-pico/utils/types.h"

#define _ZN_PRIORITIES_NUM 8

/*------------------ IOBuf ------------------*/
typedef struct
{
    union
    {
        z_zint_t sn;
        z_zint_t sns[_ZN_PRIORITIES_NUM];
    } val;
    uint8_t is_qos;
} _zn_sns_t;

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
} _z_zbuf_t;

typedef struct
{
    size_t r_idx;
    size_t w_idx;
    size_t capacity;
    z_vec_t ioss;
    int is_expandable;
} _z_wbuf_t;

#endif /* _ZENOH_PICO_PROTOCOL_PRIVATE_TYPES_H */
