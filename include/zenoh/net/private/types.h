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
#ifndef _ZENOH_NET_PICO_TYPES_H
#define _ZENOH_NET_PICO_TYPES_H

#include "zenoh/types.h"
#include "zenoh/net/types.h"

#define _ZN_IS_REMOTE 0
#define _ZN_IS_LOCAL 1

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
} _zn_resource_t;

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
    zn_subinfo_t info;
    zn_data_handler_t callback;
    void *arg;
} _zn_subscriber_t;

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
} _zn_publisher_t;

typedef struct
{
    zn_source_info_t source_info;
    zn_sample_t sample;
    z_timestamp_t tstamp;
} _zn_pending_reply_t;

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
    const char *predicate;
    zn_query_target_t target;
    zn_query_consolidation_t consolidation;
    _z_list_t *pending_replies;
    zn_query_handler_t callback;
    void *arg;
} _zn_pending_query_t;

typedef struct
{
    z_zint_t id;
    zn_reskey_t key;
    unsigned int kind;
    zn_queryable_handler_t callback;
    void *arg;
} _zn_queryable_t;

#endif /* _ZENOH_PICO_PRIVATE_TYPES_H */
