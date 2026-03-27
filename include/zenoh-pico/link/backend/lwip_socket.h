//
// Copyright (c) 2026 ZettaScale Technology
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

#ifndef ZENOH_PICO_LINK_BACKEND_LWIP_SOCKET_H
#define ZENOH_PICO_LINK_BACKEND_LWIP_SOCKET_H

#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ZENOH_FREERTOS_LWIP) || defined(ZENOH_RPI_PICO)

static inline int _z_lwip_socket_get(_z_sys_net_socket_t sock) {
#if defined(ZENOH_FREERTOS_LWIP)
    return sock._socket;
#elif defined(ZENOH_RPI_PICO)
    return sock._fd;
#endif
}

static inline void _z_lwip_socket_set(_z_sys_net_socket_t *sock, int fd) {
#if defined(ZENOH_FREERTOS_LWIP)
    sock->_socket = fd;
#elif defined(ZENOH_RPI_PICO)
    sock->_fd = fd;
#endif
}

#endif /* defined(ZENOH_FREERTOS_LWIP) || defined(ZENOH_RPI_PICO) */

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_LWIP_SOCKET_H */
