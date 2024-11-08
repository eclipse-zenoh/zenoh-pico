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

#include "zenoh-pico/transport/unicast.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/utils/uuid.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1
void _zp_unicast_fetch_zid(const _z_transport_t *zt, _z_closure_zid_t *callback) {
    void *ctx = callback->context;
    z_id_t id = zt->_transport._unicast._remote_zid;
    callback->call(&id, ctx);
}

void _zp_unicast_info_session(const _z_transport_t *zt, _z_config_t *ps) {
    _z_id_t remote_zid = zt->_transport._unicast._remote_zid;

    _z_string_t remote_zid_str = _z_id_to_string(&remote_zid);
    _zp_config_insert_string(ps, Z_INFO_ROUTER_PID_KEY, &remote_zid_str);
    _z_string_clear(&remote_zid_str);
}

#else
void _zp_unicast_fetch_zid(const _z_transport_t *zt, _z_closure_zid_t *callback) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(callback);
}

void _zp_unicast_info_session(const _z_transport_t *zt, _z_config_t *ps) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(ps);
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1
