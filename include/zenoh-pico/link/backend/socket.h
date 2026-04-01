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

#ifndef ZENOH_PICO_LINK_BACKEND_SOCKET_H
#define ZENOH_PICO_LINK_BACKEND_SOCKET_H

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uintptr_t (*id)(const _z_sys_net_socket_t *sock);
    z_result_t (*set_non_blocking)(const _z_sys_net_socket_t *sock);
    z_result_t (*wait_readable)(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready, uint32_t timeout_ms);
    void (*close)(_z_sys_net_socket_t *sock);
} _z_socket_ops_t;

#if Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1 || Z_FEATURE_LINK_WS == 1 || \
    Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
#define _Z_SOCKET_OPS_ENABLED 1
#else
#define _Z_SOCKET_OPS_ENABLED 0
#endif

#if _Z_SOCKET_OPS_ENABLED == 1 && defined(ZP_DEFAULT_SOCKET_OPS)
extern const _z_socket_ops_t ZP_DEFAULT_SOCKET_OPS;
#endif

static inline const _z_socket_ops_t *_z_default_socket_ops(void) {
#if _Z_SOCKET_OPS_ENABLED == 1 && defined(ZP_DEFAULT_SOCKET_OPS)
    return &ZP_DEFAULT_SOCKET_OPS;
#else
    return NULL;
#endif
}

static inline uintptr_t _z_socket_id(const _z_sys_net_socket_t *sock) {
    const _z_socket_ops_t *ops = _z_default_socket_ops();
    if ((ops == NULL) || (ops->id == NULL)) {
        return 0;
    }
    return ops->id(sock);
}

static inline z_result_t _z_socket_wait_readable(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                                 uint32_t timeout_ms) {
    const _z_socket_ops_t *ops = _z_default_socket_ops();
    if ((ops == NULL) || (ops->wait_readable == NULL)) {
        return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }
    return ops->wait_readable(sockets, count, ready, timeout_ms);
}

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking);
z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock);
z_result_t _z_ip_port_to_endpoint(const uint8_t *address, size_t address_len, uint16_t port, char *dst, size_t dst_len);
z_result_t _z_socket_get_endpoints(const _z_sys_net_socket_t *sock, char *local, size_t local_len, char *remote,
                                   size_t remote_len);
void _z_socket_close(_z_sys_net_socket_t *sock);

#ifdef __cplusplus
}
#endif

#undef _Z_SOCKET_OPS_ENABLED

#endif /* ZENOH_PICO_LINK_BACKEND_SOCKET_H */
