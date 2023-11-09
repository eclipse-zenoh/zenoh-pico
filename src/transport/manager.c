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

#include "zenoh-pico/transport/manager.h"

#include <stddef.h>
#include <stdlib.h>

#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/transport/unicast.h"

int8_t _z_new_transport_client(_z_transport_t *zt, char *locator, _z_id_t *local_zid) {
    int8_t ret = _Z_RES_OK;
    // Init link
    _z_link_t zl;
    memset(&zl, 0, sizeof(_z_link_t));
    // Open link
    ret = _z_open_link(&zl, locator);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    // Open transport
    if (_Z_LINK_IS_MULTICAST(zl._capabilities)) {
        _z_transport_multicast_establish_param_t tp_param;
        ret = _z_multicast_open_client(&tp_param, &zl, local_zid);
        if (ret != _Z_RES_OK) {
            _z_link_clear(&zl);
            return ret;
        }
        ret = _z_multicast_transport_create(zt, &zl, &tp_param);
    } else {
        _z_transport_unicast_establish_param_t tp_param;
        ret = _z_unicast_open_client(&tp_param, &zl, local_zid);
        if (ret != _Z_RES_OK) {
            _z_link_clear(&zl);
            return ret;
        }
        ret = _z_unicast_transport_create(zt, &zl, &tp_param);
    }
    return ret;
}

int8_t _z_new_transport_peer(_z_transport_t *zt, char *locator, _z_id_t *local_zid) {
    int8_t ret = _Z_RES_OK;
    // Init link
    _z_link_t zl;
    memset(&zl, 0, sizeof(_z_link_t));
    // Listen link
    ret = _z_listen_link(&zl, locator);
    if (ret != _Z_RES_OK) {
        return ret;
    }
    if (_Z_LINK_IS_MULTICAST(zl._capabilities)) {
        _z_transport_multicast_establish_param_t tp_param;
        ret = _z_multicast_open_peer(&tp_param, &zl, local_zid);
        if (ret != _Z_RES_OK) {
            _z_link_clear(&zl);
            return ret;
        }
        ret = _z_multicast_transport_create(zt, &zl, &tp_param);
    } else {
        _z_transport_unicast_establish_param_t tp_param;
        ret = _z_unicast_open_peer(&tp_param, &zl, local_zid);
        if (ret != _Z_RES_OK) {
            _z_link_clear(&zl);
            return ret;
        }
        ret = _z_unicast_transport_create(zt, &zl, &tp_param);
    }
    return ret;
}

int8_t _z_new_transport(_z_transport_t *zt, _z_id_t *bs, char *locator, z_whatami_t mode) {
    int8_t ret;

    if (mode == Z_WHATAMI_CLIENT) {
        ret = _z_new_transport_client(zt, locator, bs);
    } else {
        ret = _z_new_transport_peer(zt, locator, bs);
    }

    return ret;
}
