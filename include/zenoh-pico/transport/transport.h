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
#include "zenoh-pico/collections/refcount.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/runtime/runtime.h"
#include "zenoh-pico/session/weak_session.h"
#include "zenoh-pico/utils/hash.h"

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
    z_whatami_t _remote_whatami;
    volatile bool _received;
#if Z_FEATURE_CONNECTIVITY == 1
    _z_string_t _link_src;
    _z_string_t _link_dst;
#endif
#if Z_FEATURE_FRAGMENTATION == 1
    // Defragmentation buffers
    uint8_t _state_reliable;
    uint8_t _state_best_effort;
    _z_wbuf_t _dbuf_reliable;
    _z_wbuf_t _dbuf_best_effort;
    // Patch
    uint8_t _patch;
#endif
} _z_transport_peer_common_t;

#if Z_FEATURE_CONNECTIVITY == 1
typedef struct {
    _z_id_t _remote_zid;
    z_whatami_t _remote_whatami;
    _z_string_view_t _link_src;
    _z_string_view_t _link_dst;
} _z_connectivity_peer_event_data_t;
#endif

void _z_transport_peer_common_clear(_z_transport_peer_common_t *src);
#if Z_FEATURE_CONNECTIVITY == 1
void _z_connectivity_peer_event_data_alias_from_common(_z_connectivity_peer_event_data_t *dst,
                                                       const _z_transport_peer_common_t *src);
#endif

typedef struct {
    _z_transport_peer_common_t common;
    _z_conduit_sn_list_t _sn_rx_sns;
    // SN numbers
    _z_zint_t _sn_res;
    volatile _z_zint_t _lease;
} _z_transport_peer_multicast_t;

void _z_transport_peer_multicast_clear(_z_transport_peer_multicast_t *src);

#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_TYPE _z_slice_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_TYPE _z_transport_peer_multicast_t
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_HASH_FN(x) _z_fnv1_hash((x)->start, (x)->len)
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_EQ_FN(x, y) _z_slice_eq((x), (y))
#define _ZP_STATIC_HASHMAP_TEMPLATE_NAME _z_address_to_transport_peer_multicast_hmap
#define _ZP_STATIC_HASHMAP_TEMPLATE_CAPACITY Z_MAX_NUM_PEERS
#define _ZP_STATIC_HASHMAP_TEMPLATE_KEY_DESTROY_FN _z_slice_clear
#define _ZP_STATIC_HASHMAP_TEMPLATE_VAL_DESTROY_FN _z_transport_peer_multicast_clear
// default move
#include "zenoh-pico/collections/static_hashmap_template.h"

typedef enum _z_unicast_peer_flow_state_e {
    _Z_FLOW_STATE_INACTIVE = 0,
    _Z_FLOW_STATE_PENDING_SIZE = 1,
    _Z_FLOW_STATE_PENDING_DATA = 2,
    _Z_FLOW_STATE_READY = 3,
} _z_unicast_peer_flow_state_e;

typedef struct {
    _z_transport_peer_common_t common;
    _z_sys_net_socket_t _socket;
    // FIXME: Temporary ownership flag to avoid double-closing sockets
    // when link and peer structs alias the same underlying fd/TLS.
    // This should be replaced by proper, explicit ownership semantics
    // (e.g. a ref-counted socket/TLS handle or single authoritative owner).
    bool _owns_socket;
    // SN numbers
    _z_zint_t _sn_rx_reliable;
    _z_zint_t _sn_rx_best_effort;
    bool _pending;
    uint8_t flow_state;
    uint16_t flow_curr_size;
    _z_zbuf_t flow_buff;
} _z_transport_peer_unicast_t;

void _z_transport_peer_unicast_clear(_z_transport_peer_unicast_t *src);

#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_TYPE _z_transport_peer_unicast_t
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_HASH_FN(x) _z_id_hash(&(x)->common._remote_zid)
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_EQ_FN(x, y) _z_id_eq(&(x)->common._remote_zid, &(y)->common._remote_zid)
#define _ZP_STATIC_HASHSET_TEMPLATE_NAME _z_transport_peer_unicast_hset
#define _ZP_STATIC_HASHSET_TEMPLATE_CAPACITY Z_MAX_NUM_PEERS
#define _ZP_STATIC_HASHSET_TEMPLATE_KEY_DESTROY_FN _z_transport_peer_unicast_clear
// default move
#include "zenoh-pico/collections/static_hashset_template.h"

#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_NAME _z_peer_mask_bitset
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_SIZE Z_MAX_NUM_PEERS
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_BLOCK_TYPE uint8_t
#define _ZP_STATIC_BIT_VECTOR_TEMPLATE_IS_SET 1
#include "zenoh-pico/collections/static_bit_vector_template.h"

#define _Z_LOCAL_PEER_ID \
    Z_MAX_NUM_PEERS  // The local peer is not part of the peer mask, so we use a special value to represent it.

static inline _z_peer_mask_bitset_t _z_peer_mask_bitset_make_from_single_peer(size_t peer_id) {
    _z_peer_mask_bitset_t bitset = _z_peer_mask_bitset_new();
    _z_peer_mask_bitset_set_at(&bitset, peer_id, true);
    return bitset;
}
typedef enum _z_transport_state_t {
    _Z_TRANSPORT_STATE_CLOSED = 0,
    _Z_TRANSPORT_STATE_RECONNECTING = 1,
    _Z_TRANSPORT_STATE_OPEN = 2,
} _z_transport_state_t;

// Handles to the transport tasks, stored at predefined positions.
// Used by the reconnect task to resume them after a successful reconnection.
// Index via _Z_TRANSPORT_TASK_* constants defined below.
#define _Z_TRANSPORT_TASK_KEEP_ALIVE 0
#define _Z_TRANSPORT_TASK_LEASE 1
#define _Z_TRANSPORT_TASK_READ 2
#define _Z_TRANSPORT_TASK_SEND_JOIN 3  // multicast / raweth only
#define _Z_TRANSPORT_TASK_ADD_PEERS 4  // unicast only
#define _Z_TRANSPORT_TASK_COUNT 5
#if Z_FEATURE_AUTO_RECONNECT == 1
typedef struct _z_transport_tasks_t {
    _z_fut_handle_t _task_handles[_Z_TRANSPORT_TASK_COUNT];
} _z_transport_tasks_t;
#endif

typedef struct {
    _z_session_weak_t _session;
    _z_link_t *_link;
    // TX and RX buffers
    _z_wbuf_t _wbuf;
    _z_zbuf_t _zbuf;
    // SN numbers
    _z_zint_t _sn_res;
    _z_zint_t _sn_tx_reliable;
    _z_zint_t _sn_tx_best_effort;
    volatile _z_zint_t _lease;
    volatile bool _transmitted;
#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_t _mutex_tx;
    _z_mutex_rec_t _mutex_peer;
#endif
// Transport batching
#if Z_FEATURE_BATCHING == 1
    uint8_t _batch_state;
    size_t _batch_count;
#endif
    // Here we assume the value is set only by the session _z_open
    // and after it only read by the transport tasks, so we don't need to make it atomic or protect it with mutexes.
    _z_transport_state_t _state;
#if Z_FEATURE_AUTO_RECONNECT == 1
    _z_transport_tasks_t _tasks;
#endif
} _z_transport_common_t;

typedef enum {
    _Z_PENDING_PEER_STATE_PENDING = 0,
    _Z_PENDING_PEER_STATE_DONE = 1,
    _Z_PENDING_PEER_STATE_FAILED = 2,
} _z_pending_peer_state_t;

typedef struct {
    _z_string_t _locator;
    _z_pending_peer_state_t _state;
} _z_pending_peer_t;

static inline void _z_pending_peer_clear(_z_pending_peer_t *peer) { _z_string_clear(&peer->_locator); }
_Z_ELEM_DEFINE(_z_pending_peer, _z_pending_peer_t, _z_noop_size, _z_pending_peer_clear, _z_noop_copy, _z_noop_move,
               _z_noop_eq, _z_noop_cmp, _z_noop_hash)
_Z_SVEC_DEFINE_NO_COPY(_z_pending_peer, _z_pending_peer_t)

typedef struct {
    _z_pending_peer_svec_t _peers;
    int32_t _timeout_ms;
    z_clock_t _start;
    uint32_t _sleep_ms;
} _z_pending_peers_t;

_z_pending_peers_t _z_pending_peers_null(void);
z_result_t _z_pending_peers_copy_from_locators(_z_pending_peers_t *pending_peers, const _z_string_svec_t *locators);
bool _z_pending_peers_has_pending(const _z_pending_peers_t *pending_peers);
void _z_pending_peers_clear(_z_pending_peers_t *pending_peers);
void _z_pending_peers_move(_z_pending_peers_t *dst, _z_pending_peers_t *src);

typedef struct {
    _z_transport_common_t _common;
    // Known valid peers
    _z_transport_peer_unicast_hset_t _peers;
    _z_pending_peers_t _pending_peers;
} _z_transport_unicast_t;

static inline _z_transport_peer_unicast_t *_z_transport_unicast_get_first_peer(_z_transport_unicast_t *ztu) {
    _z_transport_peer_unicast_hset_iter_t iter = _z_transport_peer_unicast_hset_begin(&ztu->_peers);
    if (iter == _z_transport_peer_unicast_hset_end(&ztu->_peers)) {
        return NULL;
    }
    return _z_transport_peer_unicast_hset_at(&ztu->_peers, iter);
}

#define _Z_MULTICAST_ADDR_BUFF_SIZE 32  // Arbitrary size that must be able to contain any link address.
typedef struct _z_transport_multicast_t {
    _z_transport_common_t _common;
    // Persistent source address associated with the current contents of _zbuf.
    // Required because datagram data may remain buffered across reads.
    uint8_t _zbuf_addr_buf[_Z_MULTICAST_ADDR_BUFF_SIZE];
    _z_slice_t _zbuf_addr;
    // Known valid peers
    _z_address_to_transport_peer_multicast_hmap_t _peers;
    bool _is_raweth;
} _z_transport_multicast_t;

typedef enum {
    _Z_TRANSPORT_UNICAST_TYPE,
    _Z_TRANSPORT_MULTICAST_TYPE,
    _Z_TRANSPORT_RAWETH_TYPE,
    _Z_TRANSPORT_NONE
} _z_transport_type_t;

typedef struct {
    union {
        _z_transport_unicast_t _unicast;
        _z_transport_multicast_t _multicast;
        _z_transport_multicast_t _raweth;
    } _transport;

    _z_transport_type_t _type;
} _z_transport_t;

typedef struct {
    _z_id_t _remote_zid;
    uint16_t _batch_size;
    _z_zint_t _initial_sn_rx;
    _z_zint_t _initial_sn_tx;
    _z_zint_t _lease;
    z_whatami_t _remote_whatami;
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

z_result_t _z_transport_peer_unicast_add(_z_transport_unicast_t *ztu, _z_transport_unicast_establish_param_t *param,
                                         _z_sys_net_socket_t socket, bool owns_socket, bool notify_connectivity);
_z_transport_common_t *_z_transport_get_common(_z_transport_t *zt);
size_t _z_transport_get_peers_count(const _z_transport_t *zt);
z_result_t _z_transport_close(_z_transport_t *zt, uint8_t reason);
void _z_transport_clear(_z_transport_t *zt);
void _z_transport_free(_z_transport_t **zt);

static inline void _z_transport_get_link_properties(const _z_transport_common_t *transport, uint16_t *mtu,
                                                    bool *is_streamed, bool *is_reliable) {
    *mtu = 0;
    *is_streamed = false;
    *is_reliable = false;
    if (transport != NULL && transport->_link != NULL) {
        *mtu = transport->_link->_mtu;
        *is_streamed = transport->_link->_cap._flow == Z_LINK_CAP_FLOW_STREAM;
        *is_reliable = transport->_link->_cap._is_reliable;
    }
}

static inline bool _z_transport_batch_hold_tx_mutex(void) {
#if Z_FEATURE_BATCHING == 1 && Z_FEATURE_BATCH_TX_MUTEX == 1
    return true;
#else
    return false;
#endif
}

static inline bool _z_transport_batch_hold_peer_mutex(void) {
#if Z_FEATURE_BATCHING == 1 && Z_FEATURE_BATCH_PEER_MUTEX == 1
    return true;
#else
    return false;
#endif
}

#if Z_FEATURE_BATCHING == 1
z_result_t _z_transport_start_batching(_z_transport_t *zt);
z_result_t _z_transport_stop_batching(_z_transport_t *zt);

#endif  // Z_FEATURE_BATCHING == 1

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
static inline void _z_transport_peer_mutex_lock(_z_transport_common_t *ztc) { _ZP_UNUSED(ztc); }
static inline void _z_transport_peer_mutex_unlock(_z_transport_common_t *ztc) { _ZP_UNUSED(ztc); }
#endif  // Z_FEATURE_MULTI_THREAD == 1

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_ZENOH_PICO_TRANSPORT_TRANSPORT_H */
