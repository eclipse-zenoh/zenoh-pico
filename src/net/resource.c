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

#include "zenoh-pico/net/resource.h"

#include <stddef.h>

_z_keyexpr_t _z_rname(const char *rname) {
    _z_keyexpr_t rk;
    rk._id = Z_RESOURCE_ID_NONE;
    rk._suffix = NULL;
    if (rname != NULL) {
        rk._suffix = rname;
    }

    return rk;
}

_z_keyexpr_t _z_rid_with_suffix(_z_zint_t rid, const char *suffix) {
    _z_keyexpr_t rk;
    rk._id = rid;
    rk._suffix = NULL;
    if (suffix != NULL) {
        rk._suffix = _z_str_clone(suffix);
    }

    return rk;
}
