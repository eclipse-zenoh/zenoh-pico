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

int8_t _z_new_transport_client(_z_transport_t *zt, char *locator, _z_bytes_t local_pid) {
    int8_t ret = _Z_RES_OK;

    _z_link_t zl;
    ret = _z_open_link(&zl, locator);
    if (ret == _Z_RES_OK) {
#if Z_UNICAST_TRANSPORT == 1
        if (_Z_LINK_IS_MULTICAST(zl._capabilities) == false) {
            _z_transport_unicast_establish_param_t tp_param;
            ret = _z_transport_unicast_open_client(&tp_param, &zl, local_pid);
            if (ret == _Z_RES_OK) {
                *zt = _z_transport_unicast(&zl, &tp_param);
            } else {
                _z_link_clear(&zl);
            }
        } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
            if (_Z_LINK_IS_MULTICAST(zl._capabilities) == true) {
            _z_transport_multicast_establish_param_t tp_param;
            ret = _z_transport_multicast_open_client(&tp_param, &zl, local_pid);
            if (ret == _Z_RES_OK) {
                *zt = _z_transport_multicast(&zl, &tp_param);
            } else {
                _z_link_clear(&zl);
            }
        } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        {
            _z_link_clear(&zl);
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
        }
    }

    return ret;
}

int8_t _z_new_transport_peer(_z_transport_t *zt, char *locator, _z_bytes_t local_pid) {
    int8_t ret = _Z_RES_OK;

    _z_link_t zl;
    ret = _z_listen_link(&zl, locator);
    if (ret == _Z_RES_OK) {
#if Z_UNICAST_TRANSPORT == 1
        if (_Z_LINK_IS_MULTICAST(zl._capabilities) == false) {
            _z_transport_unicast_establish_param_t tp_param;
            ret = _z_transport_unicast_open_peer(&tp_param, &zl, local_pid);
            if (ret == _Z_RES_OK) {
                *zt = _z_transport_unicast(&zl, &tp_param);
            } else {
                _z_link_clear(&zl);
            }
        } else
#endif  // Z_UNICAST_TRANSPORT == 1
#if Z_MULTICAST_TRANSPORT == 1
            if (_Z_LINK_IS_MULTICAST(zl._capabilities) == true) {
            _z_transport_multicast_establish_param_t tp_param;
            ret = _z_transport_multicast_open_peer(&tp_param, &zl, local_pid);
            if (ret == _Z_RES_OK) {
                *zt = _z_transport_multicast(&zl, &tp_param);
            } else {
                _z_link_clear(&zl);
            }
        } else
#endif  // Z_MULTICAST_TRANSPORT == 1
        {
            _z_link_clear(&zl);
            ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
        }
    }

    return ret;
}

int8_t _z_transport_manager_init(_z_transport_manager_t *ztm) {
    int8_t ret = _Z_RES_OK;

    // Randomly generate a peer ID
    ztm->_local_pid = _z_bytes_make(Z_ZID_LENGTH);
    if (ztm->_local_pid._is_alloc == true) {
        z_random_fill((uint8_t *)ztm->_local_pid.start, ztm->_local_pid.len);
    } else {
        ret = _Z_ERR_OUT_OF_MEMORY;
    }

    return ret;
}

void _z_transport_manager_clear(_z_transport_manager_t *ztm) { _z_bytes_clear(&ztm->_local_pid); }

void _z_transport_manager_free(_z_transport_manager_t **ztm) {
    _z_transport_manager_t *ptr = *ztm;

    if (ptr != NULL) {
        _z_transport_manager_clear(ptr);

        z_free(ptr);
        *ztm = NULL;
    }
}

int8_t _z_new_transport(_z_transport_t *zt, _z_transport_manager_t *ztm, char *locator, z_whatami_t mode) {
    int8_t ret;

    if (mode == Z_WHATAMI_CLIENT) {
        ret = _z_new_transport_client(zt, locator, ztm->_local_pid);
    } else {
        ret = _z_new_transport_peer(zt, locator, ztm->_local_pid);
    }

    return ret;
}
