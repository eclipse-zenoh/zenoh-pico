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

void _z_common_transport_clear(_z_transport_common_t *ztc, bool detach_tasks) {
#if Z_FEATURE_MULTI_THREAD == 1
    // Clean up tasks
    if (ztc->_read_task != NULL) {
        ztc->_read_task_running = false;
        if (detach_tasks) {
            _z_task_detach(ztc->_read_task);
        } else {
            _z_task_join(ztc->_read_task);
        }
        _z_task_free(&ztc->_read_task);
        ztc->_read_task = NULL;
    }
    if (ztc->_lease_task != NULL) {
        ztc->_lease_task_running = false;
        if (detach_tasks) {
            _z_task_detach(ztc->_lease_task);
        } else {
            _z_task_join(ztc->_lease_task);
        }
        _z_task_free(&ztc->_lease_task);
        ztc->_lease_task = NULL;
    }
    _zp_unicast_stop_accept_task(ztc);

    // Clean up the mutexes
    _z_mutex_drop(&ztc->_mutex_tx);
    _z_mutex_drop(&ztc->_mutex_rx);
    _z_mutex_rec_drop(&ztc->_mutex_peer);
#else
    _ZP_UNUSED(detach_tasks);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Clean up the buffers
    _z_wbuf_clear(&ztc->_wbuf);
    _z_zbuf_clear(&ztc->_zbuf);
    _z_arc_slice_svec_release(&ztc->_arc_pool);
    _z_network_message_svec_release(&ztc->_msg_pool);

    _z_link_clear(&ztc->_link);
}
