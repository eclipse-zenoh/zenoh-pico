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

#include "zenoh-pico/transport/link/task/join.h"

int8_t _z_send_join(_z_transport_t *zt) {
    int8_t ret = _Z_RES_OK;

#if Z_MULTICAST_TRANSPORT == 1
    // Join task only applies to multicast transports
    if (zt->_type == _Z_TRANSPORT_MULTICAST_TYPE) {
        ret = _zp_multicast_send_join(&zt->_transport._multicast);
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
    {
        (void)zt;
        ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    return ret;
}
