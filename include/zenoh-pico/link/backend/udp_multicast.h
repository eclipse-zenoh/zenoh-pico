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

#ifndef ZENOH_PICO_LINK_BACKEND_UDP_MULTICAST_H
#define ZENOH_PICO_LINK_BACKEND_UDP_MULTICAST_H

#include <stdint.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/link/backend/datagram.h"
#include "zenoh-pico/link/backend/default_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1

typedef struct {
    z_result_t (*endpoint_init_from_address)(_z_sys_net_endpoint_t *ep, const _z_string_t *address);
    void (*endpoint_clear)(_z_sys_net_endpoint_t *ep);

    // flawfinder: ignore
    z_result_t (*open)(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                       uint32_t tout, const char *iface);
    z_result_t (*listen)(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout, const char *iface,
                         const char *join);
    void (*close)(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend, const _z_sys_net_endpoint_t rep,
                  const _z_sys_net_endpoint_t lep);

    size_t (*read_exact)(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                         _z_slice_t *ep);
    // flawfinder: ignore
    size_t (*read)(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep, _z_slice_t *ep);
    size_t (*write)(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t rep);
} _z_udp_multicast_ops_t;

#if defined(ZP_DEFAULT_UDP_MULTICAST_OPS)
extern const _z_udp_multicast_ops_t ZP_DEFAULT_UDP_MULTICAST_OPS;
#endif

static inline const _z_udp_multicast_ops_t *_z_default_udp_multicast_ops(void) {
#if defined(ZP_DEFAULT_UDP_MULTICAST_OPS)
    return &ZP_DEFAULT_UDP_MULTICAST_OPS;
#else
    return NULL;
#endif
}

static inline z_result_t _z_udp_multicast_default_endpoint_init_from_address(_z_sys_net_endpoint_t *ep,
                                                                             const _z_string_t *address) {
    return _z_datagram_endpoint_init_from_address(_z_default_datagram_ops(), ep, address);
}

static inline void _z_udp_multicast_default_endpoint_clear(_z_sys_net_endpoint_t *ep) {
    const _z_datagram_ops_t *ops = _z_default_datagram_ops();
    if (ops != NULL) {
        _z_datagram_endpoint_clear(ops, ep);
    }
}

static inline z_result_t _z_udp_multicast_endpoint_init_from_address(const _z_udp_multicast_ops_t *ops,
                                                                     _z_sys_net_endpoint_t *ep,
                                                                     const _z_string_t *address) {
    return ops->endpoint_init_from_address(ep, address);
}

static inline void _z_udp_multicast_endpoint_clear(const _z_udp_multicast_ops_t *ops, _z_sys_net_endpoint_t *ep) {
    ops->endpoint_clear(ep);
}

static inline z_result_t _z_udp_multicast_open(const _z_udp_multicast_ops_t *ops, _z_sys_net_socket_t *sock,
                                               const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep,
                                               uint32_t tout, const char *iface) {
    // flawfinder: ignore
    return ops->open(sock, rep, lep, tout, iface);
}

static inline z_result_t _z_udp_multicast_listen(const _z_udp_multicast_ops_t *ops, _z_sys_net_socket_t *sock,
                                                 const _z_sys_net_endpoint_t rep, uint32_t tout, const char *iface,
                                                 const char *join) {
    return ops->listen(sock, rep, tout, iface, join);
}

static inline void _z_udp_multicast_close(const _z_udp_multicast_ops_t *ops, _z_sys_net_socket_t *sockrecv,
                                          _z_sys_net_socket_t *socksend, const _z_sys_net_endpoint_t rep,
                                          const _z_sys_net_endpoint_t lep) {
    ops->close(sockrecv, socksend, rep, lep);
}

static inline size_t _z_udp_multicast_read_exact(const _z_udp_multicast_ops_t *ops, const _z_sys_net_socket_t sock,
                                                 uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep,
                                                 _z_slice_t *ep) {
    return ops->read_exact(sock, ptr, len, lep, ep);
}

static inline size_t _z_udp_multicast_read(const _z_udp_multicast_ops_t *ops, const _z_sys_net_socket_t sock,
                                           uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep, _z_slice_t *ep) {
    // flawfinder: ignore
    return ops->read(sock, ptr, len, lep, ep);
}

static inline size_t _z_udp_multicast_write(const _z_udp_multicast_ops_t *ops, const _z_sys_net_socket_t sock,
                                            const uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t rep) {
    return ops->write(sock, ptr, len, rep);
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_UDP_MULTICAST_H */
