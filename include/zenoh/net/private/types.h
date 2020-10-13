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

#include "zenoh/net/types.h"

//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// |K|X|X| RESOURCE|
// +---------------+
// ~     ResID     ~
// +---------------+
// ~    ResKey     ~ if  K==1 then only numerical id
// +---------------+
typedef struct
{
    z_zint_t id;
    zn_res_key_t key;
} _zn_res_decl_t;

typedef struct
{
    z_zint_t id;
    zn_res_key_t key;
    zn_data_handler_t data_handler;
    void *arg;
} _zn_sub_t;

typedef struct
{
    char *rname;
    z_zint_t rid;
    z_zint_t id;
    zn_query_handler_t query_handler;
    void *arg;
} _zn_qle_t;

#endif /* ZENOH_C_PRIVATE_TYPES_H */
