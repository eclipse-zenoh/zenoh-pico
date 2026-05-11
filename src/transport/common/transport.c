//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/transport/transport.h"

#include <stdbool.h>

#include "zenoh-pico/link/link.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/unicast/accept.h"
#include "zenoh-pico/utils/result.h"

void _z_transport_common_clear(_z_transport_common_t *ztc) {
    // Prevent any further TX access before clearing transport resources.
    _z_atomic_bool_store(&ztc->_tx_ready, false, _z_memory_order_release);
#if Z_FEATURE_MULTI_THREAD == 1
    // Drain current TX critical section (if any) before clearing buffers/link.
    _z_mutex_lock(&ztc->_mutex_tx);
    _z_mutex_unlock(&ztc->_mutex_tx);
#endif
    // Clean up the buffers
    _z_wbuf_clear(&ztc->_wbuf);
    _z_zbuf_clear(&ztc->_zbuf);

    _z_link_free(&ztc->_link);
    _z_session_weak_drop(&ztc->_session);
}
