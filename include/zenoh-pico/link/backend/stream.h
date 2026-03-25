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

#ifndef ZENOH_PICO_LINK_BACKEND_STREAM_H
#define ZENOH_PICO_LINK_BACKEND_STREAM_H

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    _z_sys_net_socket_t _sock;
    _z_sys_net_endpoint_t _rep;
} _z_tcp_socket_t;

typedef struct {
    z_result_t (*endpoint_init)(_z_sys_net_endpoint_t *ep, const char *address, const char *port);
    void (*endpoint_clear)(_z_sys_net_endpoint_t *ep);

    // flawfinder: ignore
    z_result_t (*open)(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint, uint32_t tout);
    z_result_t (*listen)(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t endpoint);
    void (*close)(_z_sys_net_socket_t *sock);

    // flawfinder: ignore
    size_t (*read)(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
    size_t (*read_exact)(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
    size_t (*write)(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len);
} _z_stream_ops_t;

char *_z_stream_address_parse_host(const _z_string_t *address);
z_result_t _z_stream_address_valid(const _z_string_t *address);
z_result_t _z_stream_endpoint_init_from_address(const _z_stream_ops_t *ops, _z_sys_net_endpoint_t *ep,
                                                const _z_string_t *address);

static inline z_result_t _z_stream_endpoint_init(const _z_stream_ops_t *ops, _z_sys_net_endpoint_t *ep,
                                                 const char *address, const char *port) {
    return ops->endpoint_init(ep, address, port);
}

static inline void _z_stream_endpoint_clear(const _z_stream_ops_t *ops, _z_sys_net_endpoint_t *ep) {
    ops->endpoint_clear(ep);
}

static inline z_result_t _z_stream_open(const _z_stream_ops_t *ops, _z_sys_net_socket_t *sock,
                                        const _z_sys_net_endpoint_t endpoint, uint32_t tout) {
    // flawfinder: ignore
    return ops->open(sock, endpoint, tout);
}

static inline z_result_t _z_stream_listen(const _z_stream_ops_t *ops, _z_sys_net_socket_t *sock,
                                          const _z_sys_net_endpoint_t endpoint) {
    return ops->listen(sock, endpoint);
}

static inline void _z_stream_close(const _z_stream_ops_t *ops, _z_sys_net_socket_t *sock) { ops->close(sock); }

static inline size_t _z_stream_read(const _z_stream_ops_t *ops, _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    // flawfinder: ignore
    return ops->read(sock, ptr, len);
}

static inline size_t _z_stream_read_exact(const _z_stream_ops_t *ops, _z_sys_net_socket_t sock, uint8_t *ptr,
                                          size_t len) {
    return ops->read_exact(sock, ptr, len);
}

static inline size_t _z_stream_write(const _z_stream_ops_t *ops, _z_sys_net_socket_t sock, const uint8_t *ptr,
                                     size_t len) {
    return ops->write(sock, ptr, len);
}

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_STREAM_H */
