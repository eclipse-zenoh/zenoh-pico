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

#ifndef ZENOH_PICO_LINK_BACKEND_DEFAULT_OPS_H
#define ZENOH_PICO_LINK_BACKEND_DEFAULT_OPS_H

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/backend/datagram.h"
#include "zenoh-pico/link/backend/rawio.h"
#include "zenoh-pico/link/backend/stream.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1 || Z_FEATURE_LINK_WS == 1) && defined(ZP_DEFAULT_STREAM_OPS)
extern const _z_stream_ops_t ZP_DEFAULT_STREAM_OPS;
#endif

#if (Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1) && defined(ZP_DEFAULT_DATAGRAM_OPS)
extern const _z_datagram_ops_t ZP_DEFAULT_DATAGRAM_OPS;
#endif

#if Z_FEATURE_LINK_SERIAL == 1 && defined(ZP_DEFAULT_RAWIO_OPS)
extern const _z_rawio_ops_t ZP_DEFAULT_RAWIO_OPS;
#endif

#ifdef __cplusplus
}
#endif

static inline const _z_stream_ops_t *_z_default_stream_ops(void) {
#if (Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_TLS == 1 || Z_FEATURE_LINK_WS == 1) && defined(ZP_DEFAULT_STREAM_OPS)
    return &ZP_DEFAULT_STREAM_OPS;
#else
    return NULL;
#endif
}

static inline const _z_datagram_ops_t *_z_default_datagram_ops(void) {
#if (Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1) && defined(ZP_DEFAULT_DATAGRAM_OPS)
    return &ZP_DEFAULT_DATAGRAM_OPS;
#else
    return NULL;
#endif
}

static inline const _z_rawio_ops_t *_z_default_rawio_ops(void) {
#if Z_FEATURE_LINK_SERIAL == 1 && defined(ZP_DEFAULT_RAWIO_OPS)
    return &ZP_DEFAULT_RAWIO_OPS;
#else
    return NULL;
#endif
}

#endif /* ZENOH_PICO_LINK_BACKEND_DEFAULT_OPS_H */
