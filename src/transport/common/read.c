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

#include "zenoh-pico/transport/link/task/read.h"

#include <stddef.h>

int8_t _z_read(_z_transport_t *zt) {
    int8_t ret = _Z_RES_OK;

#if Z_UNICAST_TRANSPORT == 1
    if (zt->_type == _Z_TRANSPORT_UNICAST_TYPE) {
        ret = _zp_unicast_read(&zt->_transport._unicast);
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (zt->_type == _Z_TRANSPORT_MULTICAST_TYPE) {
        ret = _zp_multicast_read(&zt->_transport._multicast);
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
    {
        ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    return ret;
}

void *_zp_read_task(void *zt_arg) {
    void *ret = NULL;
    _z_transport_t *zt = (_z_transport_t *)zt_arg;

#if Z_UNICAST_TRANSPORT == 1
    if (zt->_type == _Z_TRANSPORT_UNICAST_TYPE) {
        ret = _zp_unicast_read_task(&zt->_transport._unicast);
    } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
        if (zt->_type == _Z_TRANSPORT_MULTICAST_TYPE) {
        ret = _zp_multicast_read_task(&zt->_transport._multicast);
    } else
#endif  // Z_MULTICAST_TRANSPORT == 1
    {
        ret = NULL;
    }

    return ret;
}
