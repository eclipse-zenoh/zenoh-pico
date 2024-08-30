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

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/utils.h"

void _z_transport_peer_entry_clear(_z_transport_peer_entry_t *src) {
#if Z_FEATURE_FRAGMENTATION == 1
    _z_wbuf_clear(&src->_dbuf_reliable);
    _z_wbuf_clear(&src->_dbuf_best_effort);
#endif

    src->_remote_zid = _z_id_empty();
    _z_slice_clear(&src->_remote_addr);
}

void _z_transport_peer_entry_copy(_z_transport_peer_entry_t *dst, const _z_transport_peer_entry_t *src) {
#if Z_FEATURE_FRAGMENTATION == 1
    _z_wbuf_copy(&dst->_dbuf_reliable, &src->_dbuf_reliable);
    _z_wbuf_copy(&dst->_dbuf_best_effort, &src->_dbuf_best_effort);
#endif

    dst->_sn_res = src->_sn_res;
    _z_conduit_sn_list_copy(&dst->_sn_rx_sns, &src->_sn_rx_sns);

    dst->_lease = src->_lease;
    dst->_next_lease = src->_next_lease;
    dst->_received = src->_received;

    dst->_remote_zid = src->_remote_zid;
    _z_slice_copy(&dst->_remote_addr, &src->_remote_addr);
}

size_t _z_transport_peer_entry_size(const _z_transport_peer_entry_t *src) {
    (void)(src);
    return sizeof(_z_transport_peer_entry_t);
}

_Bool _z_transport_peer_entry_eq(const _z_transport_peer_entry_t *left, const _z_transport_peer_entry_t *right) {
    _Bool ret = true;
    if (memcmp(left->_remote_zid.id, right->_remote_zid.id, 16) != 0) {
        ret = false;
    }

    return ret;
}
