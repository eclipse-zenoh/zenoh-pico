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

#ifndef INCLUDE_ZENOH_PICO_TRANSPORT_TRANSPORT_H
#define INCLUDE_ZENOH_PICO_TRANSPORT_TRANSPORT_H

#include <assert.h>
#include <stdint.h>

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

enum _z_dbuf_state_e {
    _Z_DBUF_STATE_NULL = 0,
    _Z_DBUF_STATE_INIT = 1,
    _Z_DBUF_STATE_OVERFLOW = 2,
};

enum _z_batching_state_e {
    _Z_BATCHING_IDLE = 0,
    _Z_BATCHING_ACTIVE = 1,
};

typedef struct {
    _z_id_t _remote_zid;
    _z_slice_t _remote_addr;
    _z_conduit_sn_list_t _sn_rx_sns;
    // SN numbers
    _z_zint_t _sn_res;
    volatile _z_zint_t _lease;
    volatile _z_zint_t _next_lease;
    uint16_t _peer_id;
    volatile bool _received;

#if Z_FEATURE_FRAGMENTATION == 1
    // Defragmentation buffers
    uint8_t _state_reliable;
    uint8_t _state_best_effort;
    _z_wbuf_t _dbuf_reliable;
    _z_wbuf_t _dbuf_best_effort;
    // Patch
    uint8_t _patch;
#endif
} _z_transport_peer_entry_t;

size_t _z_transport_peer_entry_size(const _z_transport_peer_entry_t *src);
void _z_transport_peer_entry_clear(_z_transport_peer_entry_t *src);
void _z_transport_peer_entry_copy(_z_transport_peer_entry_t *dst, const _z_transport_peer_entry_t *src);
bool _z_transport_peer_entry_eq(const _z_transport_peer_entry_t *left, const _z_transport_peer_entry_t *right);
_Z_ELEM_DEFINE(_z_transport_peer_entry, _z_transport_peer_entry_t, _z_transport_peer_entry_size,
               _z_transport_peer_entry_clear, _z_transport_peer_entry_copy, _z_noop_move)
_Z_LIST_DEFINE(_z_transport_peer_entry, _z_transport_peer_entry_t)
_z_transport_peer_entry_list_t *_z_transport_peer_entry_list_insert(_z_transport_peer_entry_list_t *root,
                                                                    _z_transport_peer_entry_t *entry);

typedef enum _z_unicast_peer_flow_state_e {
    _Z_FLOW_STATE_INACTIVE = 0,
    _Z_FLOW_STATE_PENDING_SIZE = 1,
    _Z_FLOW_STATE_PENDING_DATA = 2,
    _Z_FLOW_STATE_READY = 3,
} _z_unicast_peer_flow_state_e;

typedef struct {
    _z_sys_net_socket_t _socket;
    _z_id_t _remote_zid;
    // SN numbers
    _z_zint_t _sn_rx_reliable;
    _z_zint_t _sn_rx_best_effort;
    volatile bool _received;
    bool _pending;
    uint8_t flow_state;
    uint16_t flow_curr_size;
    _z_zbuf_t flow_buff;

#if Z_FEATURE_FRAGMENTATION == 1
    // Defragmentation buffers
    uint8_t _state_reliable;
    uint8_t _state_best_effort;
    _z_wbuf_t _dbuf_reliable;
    _z_wbuf_t _dbuf_best_effort;
    // Patch
    uint8_t _patch;
#endif
} _z_transport_unicast_peer_t;

void _z_transport_unicast_peer_clear(_z_transport_unicast_peer_t *src);
void _z_transport_unicast_peer_copy(_z_transport_unicast_peer_t *dst, const _z_transport_unicast_peer_t *src);
size_t _z_transport_unicast_peer_size(const _z_transport_unicast_peer_t *src);
bool _z_transport_unicast_peer_eq(const _z_transport_unicast_peer_t *left, const _z_transport_unicast_peer_t *right);
_Z_ELEM_DEFINE(_z_transport_unicast_peer, _z_transport_unicast_peer_t, _z_transport_unicast_peer_size,
               _z_transport_unicast_peer_clear, _z_transport_unicast_peer_copy, _z_noop_move)
_Z_LIST_DEFINE(_z_transport_unicast_peer, _z_transport_unicast_peer_t)

// Forward type declaration to avoid cyclical include
typedef struct _z_session_rc_t _z_session_rc_ref_t;

#define _Z_RES_POOL_INIT_SIZE 8  // Arbitrary small value

typedef struct {
    _z_session_rc_ref_t *_session;
    _z_link_t _link;
    // TX and RX buffers
    _z_wbuf_t _wbuf;
    _z_zbuf_t _zbuf;
    // SN numbers
    _z_zint_t _sn_res;
    _z_zint_t _sn_tx_reliable;
    _z_zint_t _sn_tx_best_effort;
    _z_arc_slice_svec_t _arc_pool;
    _z_network_message_svec_t _msg_pool;
    volatile _z_zint_t _lease;
    volatile bool _transmitted;
#if Z_FEATURE_MULTI_THREAD == 1
    // TX and RX mutexes
    _z_mutex_t _mutex_rx;
    _z_mutex_t _mutex_tx;
    _z_mutex_rec_t _mutex_peer;

    _z_task_t *_read_task;
    _z_task_t *_lease_task;
    bool *_accept_task_running;
    volatile bool _read_task_running;
    volatile bool _lease_task_running;
#endif
// Transport batching
#if Z_FEATURE_BATCHING == 1
    uint8_t _batch_state;
    size_t _batch_count;
#endif
} _z_transport_common_t;

// Send function prototype
typedef z_result_t (*_zp_f_send_tmsg)(_z_transport_common_t *self, const _z_transport_message_t *t_msg);

typedef struct {
    _z_transport_common_t _common;
    // Known valid peers
    _z_transport_unicast_peer_list_t *_peers;
} _z_transport_unicast_t;

typedef struct _z_transport_multicast_t {
    _z_transport_common_t _common;
    // Known valid peers
    _z_transport_peer_entry_list_t *_peers;
    // T message send function
    _zp_f_send_tmsg _send_f;
} _z_transport_multicast_t;

typedef struct {
    union {
        _z_transport_unicast_t _unicast;
        _z_transport_multicast_t _multicast;
        _z_transport_multicast_t _raweth;
    } _transport;

    enum { _Z_TRANSPORT_UNICAST_TYPE, _Z_TRANSPORT_MULTICAST_TYPE, _Z_TRANSPORT_RAWETH_TYPE, _Z_TRANSPORT_NONE } _type;
} _z_transport_t;

_Z_ELEM_DEFINE(_z_transport, _z_transport_t, _z_noop_size, _z_noop_clear, _z_noop_copy, _z_noop_move)
_Z_LIST_DEFINE(_z_transport, _z_transport_t)

typedef struct {
    _z_id_t _remote_zid;
    uint16_t _batch_size;
    _z_zint_t _initial_sn_rx;
    _z_zint_t _initial_sn_tx;
    _z_zint_t _lease;
    z_whatami_t _whatami;
    uint8_t _key_id_res;
    uint8_t _req_id_res;
    uint8_t _seq_num_res;
    bool _is_qos;
#if Z_FEATURE_FRAGMENTATION == 1
    uint8_t _patch;
#endif
} _z_transport_unicast_establish_param_t;

typedef struct {
    _z_conduit_sn_list_t _initial_sn_tx;
    uint8_t _seq_num_res;
} _z_transport_multicast_establish_param_t;

z_result_t _z_transport_unicast_peer_add(_z_transport_unicast_t *ztu, _z_transport_unicast_establish_param_t *param,
                                         _z_sys_net_socket_t socket);
_z_transport_common_t *_z_transport_get_common(_z_transport_t *zt);
z_result_t _z_transport_close(_z_transport_t *zt, uint8_t reason);
void _z_transport_clear(_z_transport_t *zt);
void _z_transport_free(_z_transport_t **zt);

#if Z_FEATURE_BATCHING == 1
bool _z_transport_start_batching(_z_transport_t *zt);
void _z_transport_stop_batching(_z_transport_t *zt);
#endif

#if Z_FEATURE_MULTI_THREAD == 1
static inline z_result_t _z_transport_tx_mutex_lock(_z_transport_common_t *ztc, bool block) {
    if (block) {
        _z_mutex_lock(&ztc->_mutex_tx);
        return _Z_RES_OK;
    } else {
        return _z_mutex_try_lock(&ztc->_mutex_tx);
    }
}
static inline void _z_transport_tx_mutex_unlock(_z_transport_common_t *ztc) { _z_mutex_unlock(&ztc->_mutex_tx); }
static inline void _z_transport_rx_mutex_lock(_z_transport_common_t *ztc) { _z_mutex_lock(&ztc->_mutex_rx); }
static inline void _z_transport_rx_mutex_unlock(_z_transport_common_t *ztc) { _z_mutex_unlock(&ztc->_mutex_rx); }
static inline void _z_transport_peer_mutex_lock(_z_transport_common_t *ztc) {
    (void)_z_mutex_rec_lock(&ztc->_mutex_peer);
}
static inline void _z_transport_peer_mutex_unlock(_z_transport_common_t *ztc) {
    (void)_z_mutex_rec_unlock(&ztc->_mutex_peer);
}
#else
static inline z_result_t _z_transport_tx_mutex_lock(_z_transport_common_t *ztc, bool block) {
    _ZP_UNUSED(ztc);
    _ZP_UNUSED(block);
    return _Z_RES_OK;
}
static inline void _z_transport_tx_mutex_unlock(_z_transport_common_t *ztc) { _ZP_UNUSED(ztc); }
static inline void _z_transport_rx_mutex_lock(_z_transport_common_t *ztc) { _ZP_UNUSED(ztc); }
static inline void _z_transport_rx_mutex_unlock(_z_transport_common_t *ztc) { _ZP_UNUSED(ztc); }
static inline void _z_transport_peer_mutex_lock(_z_transport_common_t *ztc) { _ZP_UNUSED(ztc); }
static inline void _z_transport_peer_mutex_unlock(_z_transport_common_t *ztc) { _ZP_UNUSED(ztc); }
#endif  // Z_FEATURE_MULTI_THREAD == 1

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_ZENOH_PICO_TRANSPORT_TRANSPORT_H */
