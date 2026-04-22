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

#ifndef ZENOH_PICO_LINK_TRANSPORT_SOCKET_H
#define ZENOH_PICO_LINK_TRANSPORT_SOCKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _z_socket_wait_iter_t _z_socket_wait_iter_t;
typedef void (*_z_socket_wait_iter_reset_f)(_z_socket_wait_iter_t *iter);
typedef bool (*_z_socket_wait_iter_next_f)(_z_socket_wait_iter_t *iter);
typedef const _z_sys_net_socket_t *(*_z_socket_wait_iter_get_socket_f)(const _z_socket_wait_iter_t *iter);
typedef void (*_z_socket_wait_iter_set_ready_f)(_z_socket_wait_iter_t *iter, bool ready);

struct _z_socket_wait_iter_t {
    void *_ctx;
    void *_current_entry;
    _z_socket_wait_iter_reset_f _reset;
    _z_socket_wait_iter_next_f _next;
    _z_socket_wait_iter_get_socket_f _get_socket;
    _z_socket_wait_iter_set_ready_f _set_ready;
};

static inline void _z_socket_wait_iter_reset(_z_socket_wait_iter_t *iter) { iter->_reset(iter); }
static inline bool _z_socket_wait_iter_next(_z_socket_wait_iter_t *iter) { return iter->_next(iter); }
static inline const _z_sys_net_socket_t *_z_socket_wait_iter_get_socket(const _z_socket_wait_iter_t *iter) {
    return iter->_get_socket(iter);
}
static inline void _z_socket_wait_iter_set_ready(_z_socket_wait_iter_t *iter, bool ready) {
    iter->_set_ready(iter, ready);
}

z_result_t _z_socket_wait_readable(_z_socket_wait_iter_t *iter, uint32_t timeout_ms);

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking);
z_result_t _z_ip_port_to_endpoint(const uint8_t *address, size_t address_len, uint16_t port, char *dst, size_t dst_len);
z_result_t _z_socket_get_endpoints(const _z_sys_net_socket_t *sock, char *local, size_t local_len, char *remote,
                                   size_t remote_len);
void _z_socket_close(_z_sys_net_socket_t *sock);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_TRANSPORT_SOCKET_H */
