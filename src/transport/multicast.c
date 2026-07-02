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

#include "zenoh-pico/transport/multicast.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/algorithms_template.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/transport/common/lease.h"
#include "zenoh-pico/transport/common/read.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/unicast/rx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/uuid.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1
void _zp_multicast_fetch_zid(const _z_transport_t *zt, _z_closure_zid_t *callback) {
    void *ctx = callback->context;
    const _z_transport_peer_multicast_t *peer = NULL;
    _ZP_CONST_FOREACH_VAL (_z_address_to_transport_peer_multicast_hmap, &zt->_transport._multicast._peers, peer) {
        z_id_t id = peer->common._remote_zid;
        callback->call(&id, ctx);
    }
}

#else
void _zp_multicast_fetch_zid(const _z_transport_t *zt, _z_closure_zid_t *callback) {
    _ZP_UNUSED(zt);
    _ZP_UNUSED(callback);
}
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1
