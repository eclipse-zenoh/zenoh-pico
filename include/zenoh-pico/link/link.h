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

#ifndef ZENOH_PICO_LINK_H
#define ZENOH_PICO_LINK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/refcount.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/config.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Link transport capability enum.
 *
 * Enumerators:
 *     Z_LINK_CAP_TRANSPORT_UNICAST: Link has unicast capabilities.
 *     Z_LINK_CAP_TRANSPORT_MULTICAST: Link has multicast capabilities.
 */
typedef enum {
    Z_LINK_CAP_TRANSPORT_UNICAST = 0,
    Z_LINK_CAP_TRANSPORT_MULTICAST = 1,
    Z_LINK_CAP_TRANSPORT_RAWETH = 2,
} _z_link_cap_transport_t;

/**
 * Link flow capability enum.
 *
 * Enumerators:
 *     Z_LINK_CAP_FLOW_DATAGRAM: Link uses datagrams.
 *     Z_LINK_CAP_FLOW_STREAM: Link uses a byte stream.
 */
typedef enum {
    Z_LINK_CAP_FLOW_DATAGRAM = 0,
    Z_LINK_CAP_FLOW_STREAM = 1,
} _z_link_cap_flow_t;

/**
 * Link capabilities, stored as a register-like object.
 *
 * Fields:
 *     transport: 2 bits, see _z_link_cap_transport_t enum.
 *     flow: 1 bit, see _z_link_cap_flow_t enum.
 *     reliable: 1 bit, 1 if the link is reliable (network definition)
 *     reserved: 4 bits, reserved for future use
 */
typedef struct _z_link_capabilities_t {
    uint8_t _transport : 2;
    uint8_t _flow : 1;
    uint8_t _is_reliable : 1;
    uint8_t _reserved : 4;
} _z_link_capabilities_t;

struct _z_link_t;            // Forward declaration to be used in _z_f_link_*
struct _z_link_peer_t;       // Forward declaration to be used in _z_link_peer_ops_t
struct _z_link_peer_iter_t;  // Forward declaration to be used in _z_f_link_wait_peers_readable

typedef void (*_z_f_link_close)(struct _z_link_t *self);
typedef size_t (*_z_f_link_write)(const struct _z_link_t *self, const uint8_t *ptr, size_t len);
typedef size_t (*_z_f_link_write_all)(const struct _z_link_t *self, const uint8_t *ptr, size_t len);
typedef size_t (*_z_f_link_read)(const struct _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr);
typedef size_t (*_z_f_link_read_exact)(const struct _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr);
typedef z_result_t (*_z_f_link_wait_peers_readable)(const struct _z_link_t *self, struct _z_link_peer_iter_t *peers,
                                                    uint32_t timeout_ms);
typedef z_result_t (*_z_f_link_open_peer)(const struct _z_link_t *self, struct _z_link_peer_t *peer,
                                          const _z_string_t *locator, const _z_config_t *session_cfg);
typedef z_result_t (*_z_f_link_peer_from_link)(const struct _z_link_t *self, struct _z_link_peer_t *peer);
typedef z_result_t (*_z_f_link_accept_peer)(const struct _z_link_t *self, struct _z_link_peer_t *peer);
typedef z_result_t (*_z_f_link_accept_peer_complete)(const struct _z_link_t *self, struct _z_link_peer_t *peer);
typedef void (*_z_link_state_drop_f)(void *state);

typedef void (*_z_link_peer_iter_reset_f)(struct _z_link_peer_iter_t *iter);
typedef bool (*_z_link_peer_iter_next_f)(struct _z_link_peer_iter_t *iter);
typedef const struct _z_link_peer_t *(*_z_link_peer_iter_get_peer_f)(const struct _z_link_peer_iter_t *iter);
typedef void (*_z_link_peer_iter_set_ready_f)(struct _z_link_peer_iter_t *iter, bool ready);

typedef struct _z_link_peer_iter_t {
    void *_ctx;
    void *_current_entry;
    _z_link_peer_iter_reset_f _reset_f;
    _z_link_peer_iter_next_f _next_f;
    _z_link_peer_iter_get_peer_f _get_peer_f;
    _z_link_peer_iter_set_ready_f _set_ready_f;
} _z_link_peer_iter_t;

static inline void _z_link_peer_iter_reset(_z_link_peer_iter_t *iter) { iter->_reset_f(iter); }

static inline bool _z_link_peer_iter_next(_z_link_peer_iter_t *iter) { return iter->_next_f(iter); }

static inline const struct _z_link_peer_t *_z_link_peer_iter_get_peer(const _z_link_peer_iter_t *iter) {
    return iter->_get_peer_f(iter);
}

static inline void _z_link_peer_iter_set_ready(_z_link_peer_iter_t *iter, bool ready) {
    iter->_set_ready_f(iter, ready);
}

/**
 * Link peer operation callbacks.
 *
 * Peer I/O callbacks use a size_t byte-count convention:
 *
 * - A return value > 0 is the number of bytes transferred.
 * - 0 means that no bytes were transferred. Depending on the operation and
 *   transport state, callers may treat this as no progress, peer closure, or
 *   an incomplete transfer.
 * - SIZE_MAX is the sentinel for an error or no-data condition. The callback
 *   contract does not distinguish fatal errors from non-blocking "would block"
 *   cases; callers interpret SIZE_MAX according to the surrounding transport
 *   state.
 *
 * The `link` and `peer` arguments identify different parts of the operation:
 *
 * - `peer` is the concrete peer instance being operated on. It may be the
 *   link-owned default peer, or an accepted/opened peer owned by the transport.
 * - `link` provides shared link-level context such as capabilities,
 *   endpoint/configuration data, and driver-owned link state. Implementations
 *   that only need peer-local state may ignore it.
 *
 * The link-owned peer is only the default peer; it must not be assumed to be
 * the same peer passed to `_read_f` or `_write_f`.
 *
 * Fields:
 *     _read_f: Read up to len bytes from the peer into ptr.
 *     _write_f: Write up to len bytes from ptr to the peer.
 *     _set_blocking_f: Enable or disable blocking mode for the peer.
 *     _get_endpoints_f: Write the local and remote endpoint strings for the peer.
 *     _close_f: Close the peer.
 */
typedef struct _z_link_peer_ops_t {
    size_t (*_read_f)(const struct _z_link_t *link, const struct _z_link_peer_t *peer, uint8_t *ptr, size_t len);
    size_t (*_write_f)(const struct _z_link_t *link, const struct _z_link_peer_t *peer, const uint8_t *ptr, size_t len);
    z_result_t (*_set_blocking_f)(const struct _z_link_peer_t *peer, bool blocking);
    z_result_t (*_get_endpoints_f)(const struct _z_link_peer_t *peer, char *local, size_t local_len, char *remote,
                                   size_t remote_len);
    void (*_close_f)(struct _z_link_peer_t *peer);
} _z_link_peer_ops_t;

typedef void (*_z_link_peer_drop_f)(void *state);

typedef struct _z_link_peer_impl_t {
    const _z_link_peer_ops_t *_ops;
    void *_state;
    _z_link_peer_drop_f _drop_f;
} _z_link_peer_impl_t;

void _z_link_peer_impl_clear(_z_link_peer_impl_t *impl);
_Z_SIMPLE_REFCOUNT_DEFINE(_z_link_peer_impl, _z_link_peer_impl)

/*
 * _z_link_peer_t is a strong ref-counted handle to opaque driver-owned per-peer state.
 * _z_link_peer_clone() retains that state, _z_link_peer_move() transfers the handle,
 * _z_link_peer_close() performs the I/O close, and _z_link_peer_clear() releases a
 * reference. The driver drop callback runs on the final release.
 */
typedef struct _z_link_peer_t {
    _z_link_peer_impl_simple_rc_t _impl;
} _z_link_peer_t;

static inline _z_link_peer_t _z_link_peer_null(void) {
    _z_link_peer_t peer;
    peer._impl = _z_link_peer_impl_simple_rc_null();
    return peer;
}

static inline void _z_link_peer_move(_z_link_peer_t *dst, _z_link_peer_t *src) {
    *dst = *src;
    *src = _z_link_peer_null();
}

bool _z_link_peer_check(const _z_link_peer_t *peer);
z_result_t _z_link_peer_init(_z_link_peer_t *peer, const _z_link_peer_ops_t *ops, void *state,
                             _z_link_peer_drop_f drop_f);
_z_link_peer_t _z_link_peer_clone(const _z_link_peer_t *peer);
void *_z_link_peer_state(_z_link_peer_t *peer);
const void *_z_link_peer_state_const(const _z_link_peer_t *peer);
void _z_link_peer_close(_z_link_peer_t *peer);
void _z_link_peer_clear(_z_link_peer_t *peer);
size_t _z_link_peer_read(const struct _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len);
size_t _z_link_peer_write(const struct _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr, size_t len);
z_result_t _z_link_peer_set_blocking(const _z_link_peer_t *peer, bool blocking);
z_result_t _z_link_peer_get_endpoints(const _z_link_peer_t *peer, char *local, size_t local_len, char *remote,
                                      size_t remote_len);
z_result_t _z_link_wait_peers_readable(const struct _z_link_t *link, _z_link_peer_iter_t *peers, uint32_t timeout_ms);

typedef struct _z_link_t {
    _z_endpoint_t _endpoint;
    void *_state;
    _z_link_state_drop_f _state_drop_f;
    _z_link_peer_t _peer;

    _z_f_link_close _close_f;
    _z_f_link_write _write_f;
    _z_f_link_write_all _write_all_f;
    _z_f_link_read _read_f;
    _z_f_link_read_exact _read_exact_f;
    _z_f_link_wait_peers_readable _wait_peers_readable_f;
    _z_f_link_open_peer _open_peer_f;
    _z_f_link_peer_from_link _peer_from_link_f;
    // Performs only the minimal accept needed to create the provisional peer.
    _z_f_link_accept_peer _accept_peer_f;
    // Optionally finishes protocol-specific setup, such as a TLS handshake,
    // after transport-level admission checks.
    _z_f_link_accept_peer_complete _accept_peer_complete_f;

    uint16_t _mtu;
    _z_link_capabilities_t _cap;
} _z_link_t;

void _z_link_clear(_z_link_t *zl);
void _z_link_free(_z_link_t **zl);
void *_z_link_state(_z_link_t *zl);
const void *_z_link_state_const(const _z_link_t *zl);
z_result_t _z_link_open_peer(const _z_link_t *zl, _z_link_peer_t *peer, const _z_string_t *locator,
                             const _z_config_t *session_cfg);
z_result_t _z_link_peer_from_default(const _z_link_t *zl, _z_link_peer_t *peer);
z_result_t _z_link_peer_from_link(const _z_link_t *zl, _z_link_peer_t *peer);
bool _z_link_can_accept_peers(const _z_link_t *zl);
z_result_t _z_link_accept_peer(const _z_link_t *zl, _z_link_peer_t *peer);
z_result_t _z_link_accept_peer_complete(const _z_link_t *zl, _z_link_peer_t *peer);
z_result_t _z_open_link(_z_link_t *zl, const _z_string_t *locator, const _z_config_t *session_cfg);
z_result_t _z_listen_link(_z_link_t *zl, const _z_string_t *locator, const _z_config_t *session_cfg);

z_result_t _z_link_send_wbuf(const _z_link_t *zl, const _z_wbuf_t *wbf);
z_result_t _z_link_peer_send_wbuf(const _z_link_t *zl, const _z_wbuf_t *wbf, const _z_link_peer_t *peer);
size_t _z_link_recv_zbuf(const _z_link_t *zl, _z_zbuf_t *zbf, _z_slice_t *addr);
size_t _z_link_recv_exact_zbuf(const _z_link_t *zl, _z_zbuf_t *zbf, size_t len, _z_slice_t *addr);
size_t _z_link_peer_recv_zbuf(const _z_link_t *zl, _z_zbuf_t *zbf, const _z_link_peer_t *peer);
size_t _z_link_peer_recv_exact_zbuf(const _z_link_t *zl, _z_zbuf_t *zbf, size_t len, const _z_link_peer_t *peer);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_H */
