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
#ifndef ZENOH_C_PRIVATE_TYPES_H
#define ZENOH_C_PRIVATE_TYPES_H

#include "zenoh/types.h"
#include "zenoh/net/types.h"

#define _ZN_IS_REMOTE 0
#define _ZN_IS_LOCAL 1

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
    uint8_t encoding;
    uint8_t kind;
    void *context;
} _zn_resource_t;

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
    zn_subinfo_t info;
    zn_data_handler_t data_handler;
    void *arg;
} _zn_subscriber_t;

typedef struct
{
    z_str_t rname;
    z_zint_t rid;
    z_zint_t id;
    zn_query_handler_t query_handler;
    void *arg;
} _zn_queryable_t;

#endif /* ZENOH_C_PRIVATE_TYPES_H */
