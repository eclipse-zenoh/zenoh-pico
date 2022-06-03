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

#include "zenoh-pico/net/resource.h"

_z_keyexpr_t _z_rid(unsigned long rid)
{
    _z_keyexpr_t rk;
    rk.id = rid;
    rk.suffix = NULL;
    return rk;
}

_z_keyexpr_t _z_rname(const char *rname)
{
    _z_keyexpr_t rk;
    rk.id = Z_RESOURCE_ID_NONE;
    rk.suffix = NULL;
    if (rname != NULL)
        rk.suffix = _z_str_clone(rname);

    return rk;
}

_z_keyexpr_t _z_rid_with_suffix(unsigned long rid, const char *suffix)
{
    _z_keyexpr_t rk;
    rk.id = rid;
    rk.suffix = NULL;
    if (suffix != NULL)
        rk.suffix = _z_str_clone(suffix);

    return rk;
}
