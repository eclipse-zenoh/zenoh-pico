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

#include "zenoh-pico/transport/multicast/rx.h"

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/rx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1
static int8_t _z_multicast_recv_t_msg_na(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                         _z_slice_t *addr) {
    _Z_DEBUG(">> recv session msg");
    int8_t ret = _Z_RES_OK;

#if Z_FEATURE_MULTI_THREAD == 1
    // Acquire the lock
    _z_mutex_lock(&ztm->_mutex_rx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    size_t to_read = 0;
    do {
        switch (ztm->_link._cap._flow) {
            case Z_LINK_CAP_FLOW_STREAM:
                if (_z_zbuf_len(&ztm->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_link_recv_zbuf(&ztm->_link, &ztm->_zbuf, addr);
                    if (_z_zbuf_len(&ztm->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                        _z_zbuf_compact(&ztm->_zbuf);
                        ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                        break;
                    }
                }
                // Get stream size
                to_read = _z_read_stream_size(&ztm->_zbuf);
                // Read data
                if (_z_zbuf_len(&ztm->_zbuf) < to_read) {
                    _z_link_recv_zbuf(&ztm->_link, &ztm->_zbuf, addr);
                    if (_z_zbuf_len(&ztm->_zbuf) < to_read) {
                        _z_zbuf_set_rpos(&ztm->_zbuf, _z_zbuf_get_rpos(&ztm->_zbuf) - _Z_MSG_LEN_ENC_SIZE);
                        _z_zbuf_compact(&ztm->_zbuf);
                        ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                        break;
                    }
                }
                break;
            // Datagram capable links
            case Z_LINK_CAP_FLOW_DATAGRAM:
                _z_zbuf_compact(&ztm->_zbuf);
                to_read = _z_link_recv_zbuf(&ztm->_link, &ztm->_zbuf, addr);
                if (to_read == SIZE_MAX) {
                    ret = _Z_ERR_TRANSPORT_RX_FAILED;
                }
                break;
            default:
                break;
        }
    } while (false);  // The 1-iteration loop to use continue to break the entire loop on error

    if (ret == _Z_RES_OK) {
        _Z_DEBUG(">> \t transport_message_decode: %ju", (uintmax_t)_z_zbuf_len(&ztm->_zbuf));
        ret = _z_transport_message_decode(t_msg, &ztm->_zbuf);
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&ztm->_mutex_rx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return ret;
}

int8_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg, _z_slice_t *addr) {
    return _z_multicast_recv_t_msg_na(ztm, t_msg, addr);
}
#else
int8_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg, _z_slice_t *addr) {
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

int8_t _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                             _z_slice_t *addr) {
    int8_t ret = _Z_RES_OK;
#if Z_FEATURE_MULTI_THREAD == 1
    // Acquire and keep the lock
    _z_mutex_lock(&ztm->_mutex_peer);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    // Mark the session that we have received data from this peer
    _z_transport_peer_entry_t *entry = _z_find_peer_entry(ztm->_peers, addr);
    switch (_Z_MID(t_msg->_header)) {
        case _Z_MID_T_FRAME: {
            _Z_INFO("Received _Z_FRAME message");
            if (entry == NULL) {
                break;
            }
            entry->_received = true;

            // Check if the SN is correct
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_FRAME_R) == true) {
                // @TODO: amend once reliability is in place. For the time being only
                //        monotonic SNs are ensured
                if (_z_sn_precedes(entry->_sn_res, entry->_sn_rx_sns._val._plain._reliable, t_msg->_body._frame._sn) ==
                    true) {
                    entry->_sn_rx_sns._val._plain._reliable = t_msg->_body._frame._sn;
                } else {
#if Z_FEATURE_FRAGMENTATION == 1
                    _z_wbuf_clear(&entry->_dbuf_reliable);
#endif
                    _Z_INFO("Reliable message dropped because it is out of order");
                    break;
                }
            } else {
                if (_z_sn_precedes(entry->_sn_res, entry->_sn_rx_sns._val._plain._best_effort,
                                   t_msg->_body._frame._sn) == true) {
                    entry->_sn_rx_sns._val._plain._best_effort = t_msg->_body._frame._sn;
                } else {
#if Z_FEATURE_FRAGMENTATION == 1
                    _z_wbuf_clear(&entry->_dbuf_best_effort);
#endif
                    _Z_INFO("Best effort message dropped because it is out of order");
                    break;
                }
            }

            // Handle all the zenoh message, one by one
            uint16_t mapping = entry->_peer_id;
            size_t len = _z_vec_len(&t_msg->_body._frame._messages);
            for (size_t i = 0; i < len; i++) {
                _z_network_message_t *zm = _z_network_message_vec_get(&t_msg->_body._frame._messages, i);
                _z_msg_fix_mapping(zm, mapping);
                _z_handle_network_message(ztm->_session, zm, mapping);
            }

            break;
        }

        case _Z_MID_T_FRAGMENT: {
            _Z_INFO("Received Z_FRAGMENT message");
#if Z_FEATURE_FRAGMENTATION == 1
            if (entry == NULL) {
                break;
            }
            entry->_received = true;

            _z_wbuf_t *dbuf = _Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_FRAGMENT_R)
                                  ? &entry->_dbuf_reliable
                                  : &entry->_dbuf_best_effort;  // Select the right defragmentation buffer

            _Bool drop = false;
            if ((_z_wbuf_len(dbuf) + t_msg->_body._fragment._payload.len) > Z_FRAG_MAX_SIZE) {
                // Filling the wbuf capacity as a way to signaling the last fragment to reset the dbuf
                // Otherwise, last (smaller) fragments can be understood as a complete message
                _z_wbuf_write_bytes(dbuf, t_msg->_body._fragment._payload.start, 0, _z_wbuf_space_left(dbuf));
                drop = true;
            } else {
                _z_wbuf_write_bytes(dbuf, t_msg->_body._fragment._payload.start, 0,
                                    t_msg->_body._fragment._payload.len);
            }

            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_FRAGMENT_M) == false) {
                if (drop == true) {  // Drop message if it exceeds the fragmentation size
                    _z_wbuf_reset(dbuf);
                    break;
                }

                _z_zbuf_t zbf = _z_wbuf_to_zbuf(dbuf);  // Convert the defragmentation buffer into a decoding buffer

                _z_zenoh_message_t zm;
                ret = _z_network_message_decode(&zm, &zbf);
                if (ret == _Z_RES_OK) {
                    uint16_t mapping = entry->_peer_id;
                    _z_msg_fix_mapping(&zm, mapping);
                    _z_handle_network_message(ztm->_session, &zm, mapping);
                    _z_msg_clear(&zm);  // Clear must be explicitly called for fragmented zenoh messages. Non-fragmented
                                        // zenoh messages are released when their transport message is released.
                }

                // Free the decoding buffer
                _z_zbuf_clear(&zbf);
                // Reset the defragmentation buffer
                _z_wbuf_reset(dbuf);
            }
#else
            _Z_INFO("Fragment dropped because fragmentation feature is deactivated");
#endif
            break;
        }

        case _Z_MID_T_KEEP_ALIVE: {
            _Z_INFO("Received _Z_KEEP_ALIVE message");
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
            _Z_INFO("Received _Z_JOIN message");
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
#if Z_FEATURE_DYNAMIC_MEMORY_ALLOCATION == 1
                        entry->_dbuf_reliable = _z_wbuf_make(0, true);
                        entry->_dbuf_best_effort = _z_wbuf_make(0, true);
#else
                        entry->_dbuf_reliable = _z_wbuf_make(Z_FRAG_MAX_SIZE, false);
                        entry->_dbuf_best_effort = _z_wbuf_make(Z_FRAG_MAX_SIZE, false);

                        if ((_z_wbuf_capacity(&entry->_dbuf_reliable) != Z_FRAG_MAX_SIZE) ||
                            (_z_wbuf_capacity(&entry->_dbuf_best_effort) != Z_FRAG_MAX_SIZE)) {
                            _Z_ERROR("Not enough memory to allocate peer defragmentation buffers!");
                        }
#endif
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

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&ztm->_mutex_peer);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return ret;
}
#else
int8_t _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                             _z_slice_t *addr) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(t_msg);
    _ZP_UNUSED(addr);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1
