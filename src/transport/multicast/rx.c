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

#include "zenoh-pico/transport/common/rx.h"

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/multicast/transport.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1
static z_result_t _z_multicast_recv_t_msg_na(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                             _z_slice_t *addr) {
    _Z_DEBUG(">> recv session msg");
    z_result_t ret = _Z_RES_OK;

    _z_transport_rx_mutex_lock(&ztm->_common);
    size_t to_read = 0;
    do {
        switch (ztm->_common._link._cap._flow) {
            case Z_LINK_CAP_FLOW_STREAM:
                if (_z_zbuf_len(&ztm->_common._zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_link_recv_zbuf(&ztm->_common._link, &ztm->_common._zbuf, addr);
                    if (_z_zbuf_len(&ztm->_common._zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                        _z_zbuf_compact(&ztm->_common._zbuf);
                        ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                        break;
                    }
                }
                // Get stream size
                to_read = _z_read_stream_size(&ztm->_common._zbuf);
                // Read data
                if (_z_zbuf_len(&ztm->_common._zbuf) < to_read) {
                    _z_link_recv_zbuf(&ztm->_common._link, &ztm->_common._zbuf, addr);
                    if (_z_zbuf_len(&ztm->_common._zbuf) < to_read) {
                        _z_zbuf_set_rpos(&ztm->_common._zbuf,
                                         _z_zbuf_get_rpos(&ztm->_common._zbuf) - _Z_MSG_LEN_ENC_SIZE);
                        _z_zbuf_compact(&ztm->_common._zbuf);
                        ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                        break;
                    }
                }
                break;
            // Datagram capable links
            case Z_LINK_CAP_FLOW_DATAGRAM:
                _z_zbuf_compact(&ztm->_common._zbuf);
                to_read = _z_link_recv_zbuf(&ztm->_common._link, &ztm->_common._zbuf, addr);
                if (to_read == SIZE_MAX) {
                    ret = _Z_ERR_TRANSPORT_RX_FAILED;
                }
                break;
            default:
                break;
        }
    } while (false);  // The 1-iteration loop to use continue to break the entire loop on error

    if (ret == _Z_RES_OK) {
        _Z_DEBUG(">> \t transport_message_decode: %ju", (uintmax_t)_z_zbuf_len(&ztm->_common._zbuf));
        ret = _z_transport_message_decode(t_msg, &ztm->_common._zbuf, &ztm->_common._arc_pool, &ztm->_common._msg_pool);
    }
    _z_transport_rx_mutex_unlock(&ztm->_common);
    return ret;
}

z_result_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg, _z_slice_t *addr) {
    return _z_multicast_recv_t_msg_na(ztm, t_msg, addr);
}
#else
z_result_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg, _z_slice_t *addr) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(t_msg);
    _ZP_UNUSED(addr);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1

#if Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1

static _z_transport_peer_entry_t *_z_find_peer_entry(_z_transport_peer_entry_list_t *l, _z_slice_t *remote_addr) {
    _z_transport_peer_entry_t *ret = NULL;

    _z_transport_peer_entry_list_t *xs = l;
    for (; xs != NULL; xs = _z_transport_peer_entry_list_tail(xs)) {
        _z_transport_peer_entry_t *val = _z_transport_peer_entry_list_head(xs);
        if (val->_remote_addr.len != remote_addr->len) {
            continue;
        }

        if (memcmp(val->_remote_addr.start, remote_addr->start, remote_addr->len) == 0) {
            ret = val;
        }
    }

    return ret;
}

z_result_t _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                                 _z_slice_t *addr) {
    z_result_t ret = _Z_RES_OK;
    _z_multicast_peer_mutex_lock(ztm);
    // Mark the session that we have received data from this peer
    _z_transport_peer_entry_t *entry = _z_find_peer_entry(ztm->_peers, addr);
    switch (_Z_MID(t_msg->_header)) {
        case _Z_MID_T_FRAME: {
            _Z_DEBUG("Received _Z_FRAME message");
            if (entry == NULL) {
                _Z_INFO("Dropping _Z_FRAME from unknown peer");
                break;
            }
            // Note that we receive data from peer
            entry->_received = true;
            z_reliability_t tmsg_reliability;
            // Check if the SN is correct
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_FRAME_R) == true) {
                tmsg_reliability = Z_RELIABILITY_RELIABLE;
                // @TODO: amend once reliability is in place. For the time being only
                //        monotonic SNs are ensured
                if (_z_sn_precedes(entry->_sn_res, entry->_sn_rx_sns._val._plain._reliable, t_msg->_body._frame._sn) ==
                    true) {
                    entry->_sn_rx_sns._val._plain._reliable = t_msg->_body._frame._sn;
                } else {
#if Z_FEATURE_FRAGMENTATION == 1
                    entry->_state_reliable = _Z_DBUF_STATE_NULL;
                    _z_wbuf_clear(&entry->_dbuf_reliable);
#endif
                    _Z_INFO("Reliable message dropped because it is out of order");
                    break;
                }
            } else {
                tmsg_reliability = Z_RELIABILITY_BEST_EFFORT;
                if (_z_sn_precedes(entry->_sn_res, entry->_sn_rx_sns._val._plain._best_effort,
                                   t_msg->_body._frame._sn) == true) {
                    entry->_sn_rx_sns._val._plain._best_effort = t_msg->_body._frame._sn;
                } else {
#if Z_FEATURE_FRAGMENTATION == 1
                    entry->_state_best_effort = _Z_DBUF_STATE_NULL;
                    _z_wbuf_clear(&entry->_dbuf_best_effort);
#endif
                    _Z_INFO("Best effort message dropped because it is out of order");
                    break;
                }
            }

            // Handle all the zenoh message, one by one
            uint16_t mapping = entry->_peer_id;
            size_t len = _z_svec_len(&t_msg->_body._frame._messages);
            for (size_t i = 0; i < len; i++) {
                _z_network_message_t *zm = _z_network_message_svec_get(&t_msg->_body._frame._messages, i);
                zm->_reliability = tmsg_reliability;

                _z_msg_fix_mapping(zm, mapping);
                _z_handle_network_message(ztm->_common._session, zm, mapping);
            }

            break;
        }

        case _Z_MID_T_FRAGMENT: {
            _Z_DEBUG("Received Z_FRAGMENT message");
#if Z_FEATURE_FRAGMENTATION == 1
            if (entry == NULL) {
                _Z_INFO("Dropping Z_FRAGMENT from unknown peer");
                break;
            }
            // Note that we receive data from the peer
            entry->_received = true;

            _z_wbuf_t *dbuf;
            uint8_t *dbuf_state;
            z_reliability_t tmsg_reliability;
            bool consecutive;
            // Select the right defragmentation buffer
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_FRAME_R)) {
                tmsg_reliability = Z_RELIABILITY_RELIABLE;
                // Check SN
                // @TODO: amend once reliability is in place. For the time being only
                //        monotonic SNs are ensured
                if (_z_sn_precedes(entry->_sn_res, entry->_sn_rx_sns._val._plain._reliable,
                                   t_msg->_body._fragment._sn) == true) {
                    consecutive = _z_sn_consecutive(entry->_sn_res, entry->_sn_rx_sns._val._plain._reliable,
                                                    t_msg->_body._fragment._sn);
                    entry->_sn_rx_sns._val._plain._reliable = t_msg->_body._fragment._sn;
                    dbuf = &entry->_dbuf_reliable;
                    dbuf_state = &entry->_state_reliable;
                } else {
                    _z_wbuf_clear(&entry->_dbuf_reliable);
                    entry->_state_reliable = _Z_DBUF_STATE_NULL;
                    _Z_INFO("Reliable message dropped because it is out of order");
                    break;
                }
            } else {
                tmsg_reliability = Z_RELIABILITY_BEST_EFFORT;
                // Check SN
                if (_z_sn_precedes(entry->_sn_res, entry->_sn_rx_sns._val._plain._best_effort,
                                   t_msg->_body._fragment._sn)) {
                    consecutive = _z_sn_consecutive(entry->_sn_res, entry->_sn_rx_sns._val._plain._best_effort,
                                                    t_msg->_body._fragment._sn);
                    entry->_sn_rx_sns._val._plain._best_effort = t_msg->_body._fragment._sn;
                    dbuf = &entry->_dbuf_best_effort;
                    dbuf_state = &entry->_state_best_effort;
                } else {
                    _z_wbuf_clear(&entry->_dbuf_best_effort);
                    entry->_state_best_effort = _Z_DBUF_STATE_NULL;
                    _Z_INFO("Best effort message dropped because it is out of order");
                    break;
                }
            }
            if (!consecutive && (_z_wbuf_len(dbuf) > 0)) {
                _z_wbuf_clear(dbuf);
                *dbuf_state = _Z_DBUF_STATE_NULL;
                _Z_INFO("Defragmentation buffer dropped because non-consecutive fragments received");
                break;
            }
            // Handle fragment markers
            if (_Z_PATCH_HAS_FRAGMENT_MARKERS(entry->_patch)) {
                if (t_msg->_body._fragment.first) {
                    _z_wbuf_reset(dbuf);
                } else if (_z_wbuf_len(dbuf) == 0) {
                    _Z_INFO("First fragment received without the first marker");
                    break;
                }
                if (t_msg->_body._fragment.drop) {
                    _z_wbuf_reset(dbuf);
                    break;
                }
            }
            // Allocate buffer if needed
            if (*dbuf_state == _Z_DBUF_STATE_NULL) {
                *dbuf = _z_wbuf_make(Z_FRAG_MAX_SIZE, false);
                if (_z_wbuf_capacity(dbuf) != Z_FRAG_MAX_SIZE) {
                    _Z_ERROR("Not enough memory to allocate peer defragmentation buffer");
                    ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
                    break;
                }
                *dbuf_state = _Z_DBUF_STATE_INIT;
            }
            // Process fragment data
            if (*dbuf_state == _Z_DBUF_STATE_INIT) {
                // Check overflow
                if ((_z_wbuf_len(dbuf) + t_msg->_body._fragment._payload.len) > Z_FRAG_MAX_SIZE) {
                    *dbuf_state = _Z_DBUF_STATE_OVERFLOW;
                } else {
                    // Fill buffer
                    _z_wbuf_write_bytes(dbuf, t_msg->_body._fragment._payload.start, 0,
                                        t_msg->_body._fragment._payload.len);
                }
            }
            // Process final fragment
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_FRAGMENT_M) == false) {
                // Drop message if it exceeds the fragmentation size
                if (*dbuf_state == _Z_DBUF_STATE_OVERFLOW) {
                    _Z_INFO("Fragment dropped because defragmentation buffer has overflown");
                    _z_wbuf_clear(dbuf);
                    *dbuf_state = _Z_DBUF_STATE_NULL;
                    break;
                }
                // Convert the defragmentation buffer into a decoding buffer
                _z_zbuf_t zbf = _z_wbuf_moved_as_zbuf(dbuf);
                if (_z_zbuf_capacity(&zbf) == 0) {
                    _Z_ERROR("Failed to convert defragmentation buffer into a decoding buffer!");
                    _z_wbuf_clear(dbuf);
                    *dbuf_state = _Z_DBUF_STATE_NULL;
                    ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
                    break;
                }
                // Decode message
                _z_zenoh_message_t zm = {0};
                assert(ztm->_common._arc_pool._capacity >= 1);
                _z_arc_slice_t *arcs = _z_arc_slice_svec_get_mut(&ztm->_common._arc_pool, 0);
                ret = _z_network_message_decode(&zm, &zbf, arcs);
                zm._reliability = tmsg_reliability;
                if (ret == _Z_RES_OK) {
                    uint16_t mapping = entry->_peer_id;
                    _z_msg_fix_mapping(&zm, mapping);
                    _z_handle_network_message(ztm->_common._session, &zm, mapping);
                } else {
                    _Z_INFO("Failed to decode defragmented message");
                    ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
                }
                // Free the decoding buffer
                _z_zbuf_clear(&zbf);
                *dbuf_state = _Z_DBUF_STATE_NULL;
            }
#else
            _Z_INFO("Fragment dropped because fragmentation feature is deactivated");
#endif
            break;
        }

        case _Z_MID_T_KEEP_ALIVE: {
            _Z_DEBUG("Received _Z_KEEP_ALIVE message");
            if (entry == NULL) {
                break;
            }
            entry->_received = true;

            break;
        }

        case _Z_MID_T_INIT: {
            // Do nothing, multicast transports are not expected to handle INIT messages
            break;
        }

        case _Z_MID_T_OPEN: {
            // Do nothing, multicast transports are not expected to handle OPEN messages
            break;
        }

        case _Z_MID_T_JOIN: {
            _Z_DEBUG("Received _Z_JOIN message");
            if (t_msg->_body._join._version != Z_PROTO_VERSION) {
                break;
            }

            if (entry == NULL)  // New peer
            {
                entry = (_z_transport_peer_entry_t *)z_malloc(sizeof(_z_transport_peer_entry_t));
                if (entry != NULL) {
                    entry->_sn_res = _z_sn_max(t_msg->_body._join._seq_num_res);

                    // If the new node has less representing capabilities then it is incompatible to communication
                    if ((t_msg->_body._join._seq_num_res != Z_SN_RESOLUTION) ||
                        (t_msg->_body._join._req_id_res != Z_REQ_RESOLUTION) ||
                        (t_msg->_body._join._batch_size != Z_BATCH_MULTICAST_SIZE)) {
                        ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
                    }

                    if (ret == _Z_RES_OK) {
                        entry->_remote_addr = _z_slice_duplicate(addr);
                        entry->_remote_zid = t_msg->_body._join._zid;

                        _z_conduit_sn_list_copy(&entry->_sn_rx_sns, &t_msg->_body._join._next_sn);
                        _z_conduit_sn_list_decrement(entry->_sn_res, &entry->_sn_rx_sns);

#if Z_FEATURE_FRAGMENTATION == 1
                        entry->_patch =
                            t_msg->_body._join._patch < _Z_CURRENT_PATCH ? t_msg->_body._join._patch : _Z_CURRENT_PATCH;
                        entry->_state_reliable = _Z_DBUF_STATE_NULL;
                        entry->_state_best_effort = _Z_DBUF_STATE_NULL;
                        entry->_dbuf_reliable = _z_wbuf_null();
                        entry->_dbuf_best_effort = _z_wbuf_null();
#endif
                        // Update lease time (set as ms during)
                        entry->_lease = t_msg->_body._join._lease;
                        entry->_next_lease = entry->_lease;
                        entry->_received = true;

                        ztm->_peers = _z_transport_peer_entry_list_insert(ztm->_peers, entry);
                    } else {
                        z_free(entry);
                    }
                } else {
                    ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
                }
            } else {  // Existing peer
                entry->_received = true;

                // Check if the representing capabilities are still the same
                if ((t_msg->_body._join._seq_num_res != Z_SN_RESOLUTION) ||
                    (t_msg->_body._join._req_id_res != Z_REQ_RESOLUTION) ||
                    (t_msg->_body._join._batch_size != Z_BATCH_MULTICAST_SIZE)) {
                    _z_transport_peer_entry_list_drop_filter(ztm->_peers, _z_transport_peer_entry_eq, entry);
                    // TODO: cleanup here should also be done on mappings/subs/etc...
                    break;
                }

                // Update SNs
                _z_conduit_sn_list_copy(&entry->_sn_rx_sns, &t_msg->_body._join._next_sn);
                _z_conduit_sn_list_decrement(entry->_sn_res, &entry->_sn_rx_sns);

                // Update lease time (set as ms during)
                entry->_lease = t_msg->_body._join._lease;
            }
            break;
        }

        case _Z_MID_T_CLOSE: {
            _Z_INFO("Closing session as requested by the remote peer");

            if (entry == NULL) {
                break;
            }
            ztm->_peers = _z_transport_peer_entry_list_drop_filter(ztm->_peers, _z_transport_peer_entry_eq, entry);

            break;
        }

        default: {
            _Z_ERROR("Unknown session message ID");
            break;
        }
    }
    _z_multicast_peer_mutex_unlock(ztm);
    return ret;
}

z_result_t _z_multicast_update_rx_buffer(_z_transport_multicast_t *ztm) {
    // Check if user or defragment buffer took ownership of buffer
    if (_z_zbuf_get_ref_count(&ztm->_common._zbuf) != 1) {
        // Allocate a new buffer
        _z_zbuf_t new_zbuf = _z_zbuf_make(Z_BATCH_MULTICAST_SIZE);
        if (_z_zbuf_capacity(&new_zbuf) != Z_BATCH_MULTICAST_SIZE) {
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
        // Recopy leftover bytes
        size_t leftovers = _z_zbuf_len(&ztm->_common._zbuf);
        if (leftovers > 0) {
            _z_zbuf_copy_bytes(&new_zbuf, &ztm->_common._zbuf);
        }
        // Drop buffer & update
        _z_zbuf_clear(&ztm->_common._zbuf);
        ztm->_common._zbuf = new_zbuf;
    }
    return _Z_RES_OK;
}

#else
z_result_t _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                                 _z_slice_t *addr) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(t_msg);
    _ZP_UNUSED(addr);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1
