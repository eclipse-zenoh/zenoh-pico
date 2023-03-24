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

#include "zenoh-pico/transport/link/rx.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_MULTICAST_TRANSPORT == 1

_z_transport_peer_entry_t *_z_find_peer_entry(_z_transport_peer_entry_list_t *l, _z_bytes_t *remote_addr) {
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

/*------------------ Reception helper ------------------*/
int8_t _z_multicast_recv_t_msg_na(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg, _z_bytes_t *addr) {
    _Z_DEBUG(">> recv session msg\n");
    uint8_t ret = _Z_RES_OK;

#if Z_MULTI_THREAD == 1
    // Acquire the lock
    _z_mutex_lock(&ztm->_mutex_rx);
#endif  // Z_MULTI_THREAD == 1

    // Prepare the buffer
    _z_zbuf_reset(&ztm->_zbuf);

    if (_Z_LINK_IS_STREAMED(ztm->_link._capabilities) == true) {
        // Read the message length
        if (_z_link_recv_exact_zbuf(&ztm->_link, &ztm->_zbuf, _Z_MSG_LEN_ENC_SIZE, addr) != _Z_MSG_LEN_ENC_SIZE) {
            size_t len = 0;
            for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
                len |= _z_zbuf_read(&ztm->_zbuf) << (i * (uint8_t)8);
            }

            _Z_DEBUG(">> \t msg len = %zu\n", len);
            size_t writable = _z_zbuf_capacity(&ztm->_zbuf) - _z_zbuf_len(&ztm->_zbuf);
            if (writable < len) {
                // Read enough bytes to decode the message
                if (_z_link_recv_exact_zbuf(&ztm->_link, &ztm->_zbuf, len, addr) != len) {
                    ret = _Z_ERR_TRANSPORT_RX_FAILED;
                }
            } else {
                ret = _Z_ERR_TRANSPORT_NO_SPACE;
            }
        } else {
            ret = _Z_ERR_TRANSPORT_RX_FAILED;
        }
    } else {
        if (_z_link_recv_zbuf(&ztm->_link, &ztm->_zbuf, addr) == SIZE_MAX) {
            ret = _Z_ERR_TRANSPORT_RX_FAILED;
        }
    }

    _Z_DEBUG(">> \t transport_message_decode\n");
    if (ret == _Z_RES_OK) {
        _z_transport_message_decode_na(t_msg, &ztm->_zbuf);
    }

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&ztm->_mutex_rx);
#endif  // Z_MULTI_THREAD == 1

    return ret;
}

int8_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg, _z_bytes_t *addr) {
    return _z_multicast_recv_t_msg_na(ztm, t_msg, addr);
}

int8_t _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                             _z_bytes_t *addr) {
    int8_t ret = _Z_RES_OK;
#if Z_MULTI_THREAD == 1
    // Acquire and keep the lock
    _z_mutex_lock(&ztm->_mutex_peer);
#endif  // Z_MULTI_THREAD == 1

    // Mark the session that we have received data from this peer
    _z_transport_peer_entry_t *entry = _z_find_peer_entry(ztm->_peers, addr);
    switch (_Z_MID(t_msg->_header)) {
        case _Z_MID_FRAME: {
            _Z_INFO("Received _Z_FRAME message\n");
            if (entry == NULL) {
                break;
            }
            entry->_received = true;

            // Check if the SN is correct
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_FRAME_R) == true) {
                // @TODO: amend once reliability is in place. For the time being only
                //        monothonic SNs are ensured
                if (_z_sn_precedes(entry->_sn_res, entry->_sn_rx_sns._val._plain._reliable, t_msg->_body._frame._sn) ==
                    true) {
                    entry->_sn_rx_sns._val._plain._reliable = t_msg->_body._frame._sn;
                } else {
                    _z_wbuf_clear(&entry->_dbuf_reliable);
                    _Z_INFO("Reliable message dropped because it is out of order\n");
                    break;
                }
            } else {
                if (_z_sn_precedes(entry->_sn_res, entry->_sn_rx_sns._val._plain._best_effort,
                                   t_msg->_body._frame._sn) == true) {
                    entry->_sn_rx_sns._val._plain._best_effort = t_msg->_body._frame._sn;
                } else {
                    _z_wbuf_clear(&entry->_dbuf_best_effort);
                    _Z_INFO("Best effort message dropped because it is out of order\n");
                    break;
                }
            }

            // Handle all the zenoh message, one by one
            size_t len = _z_vec_len(&t_msg->_body._frame._messages);
            for (size_t i = 0; i < len; i++) {
                _z_handle_zenoh_message(ztm->_session,
                                        (_z_zenoh_message_t *)_z_vec_get(&t_msg->_body._frame._messages, i));
            }

            break;
        }

        case _Z_MID_FRAGMENT: {
            _Z_INFO("Received Z_FRAGMENT message\n");
            if (entry == NULL) {
                break;
            }
            entry->_received = true;

            _z_wbuf_t *dbuf = _Z_HAS_FLAG(t_msg->_header, _Z_FLAG_FRAGMENT_R)
                                  ? &entry->_dbuf_reliable
                                  : &entry->_dbuf_best_effort;  // Select the right defragmentation buffer

            _Bool drop = false;
            if ((_z_wbuf_len(dbuf) + t_msg->_body._fragment._payload.len) > Z_FRAG_MAX_SIZE) {
                // Filling the wbuf capacity as a way to signling the last fragment to reset the dbuf
                // Otherwise, last (smaller) fragments can be understood as a complete message
                _z_wbuf_write_bytes(dbuf, t_msg->_body._fragment._payload.start, 0, _z_wbuf_space_left(dbuf));
                drop = true;
            } else {
                _z_wbuf_write_bytes(dbuf, t_msg->_body._fragment._payload.start, 0,
                                    t_msg->_body._fragment._payload.len);
            }

            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_FRAGMENT_M) == false) {
                if (drop == true) {  // Drop message if it exceeds the fragmentation size
                    _z_wbuf_reset(dbuf);
                    break;
                }

                _z_zbuf_t zbf = _z_wbuf_to_zbuf(dbuf);  // Convert the defragmentation buffer into a decoding buffer

                _z_zenoh_message_t zm;
                int8_t ret = _z_zenoh_message_decode(&zm, &zbf);
                if (ret == _Z_RES_OK) {
                    _z_handle_zenoh_message(ztm->_session, &zm);
                    _z_msg_clear(&zm);  // Clear must be explicitly called for fragmented zenoh messages. Non-fragmented
                                        // zenoh messages are released when their transport message is released.
                }

                // Free the decoding buffer
                _z_zbuf_clear(&zbf);
                // Reset the defragmentation buffer
                _z_wbuf_reset(dbuf);
            }
            break;
        }

        case _Z_MID_KEEP_ALIVE: {
            _Z_INFO("Received _Z_KEEP_ALIVE message\n");
            if (entry == NULL) {
                break;
            }
            entry->_received = true;

            break;
        }

        case _Z_MID_INIT: {
            // Do nothing, multicas transports are not expected to handle INIT messages
            break;
        }

        case _Z_MID_OPEN: {
            // Do nothing, multicas transports are not expected to handle OPEN messages
            break;
        }

        case _Z_MID_JOIN: {
            _Z_INFO("Received _Z_JOIN message\n");
            if (t_msg->_body._join._version != Z_PROTO_VERSION) {
                break;
            }

            if (entry == NULL)  // New peer
            {
                entry = (_z_transport_peer_entry_t *)z_malloc(sizeof(_z_transport_peer_entry_t));
                if (entry != NULL) {
                    entry->_sn_res = _z_sn_max(t_msg->_body._join._seq_num_res);

                    // If the new node has less representing capabilities then it is incompatible to communication
                    if ((t_msg->_body._join._seq_num_res < Z_SN_RESOLUTION) ||
                        (t_msg->_body._join._key_id_res < Z_KID_RESOLUTION) ||
                        (t_msg->_body._join._req_id_res < Z_REQ_RESOLUTION) ||
                        (t_msg->_body._join._batch_size < Z_BATCH_SIZE)) {
                        ret = _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
                    }

                    if (ret == _Z_RES_OK) {
                        entry->_remote_addr = _z_bytes_duplicate(addr);
                        entry->_remote_zid = _z_bytes_duplicate(&t_msg->_body._join._zid);

                        _z_conduit_sn_list_copy(&entry->_sn_rx_sns, &t_msg->_body._join._next_sn);
                        _z_conduit_sn_list_decrement(entry->_sn_res, &entry->_sn_rx_sns);

#if Z_DYNAMIC_MEMORY_ALLOCATION == 1
                        entry->_dbuf_reliable = _z_wbuf_make(0, true);
                        entry->_dbuf_best_effort = _z_wbuf_make(0, true);
#else
                        entry->_dbuf_reliable = _z_wbuf_make(Z_FRAG_MAX_SIZE, false);
                        entry->_dbuf_best_effort = _z_wbuf_make(Z_FRAG_MAX_SIZE, false);
#endif

                        // Update lease time (set as ms during)
                        entry->_lease = t_msg->_body._join._lease;
                        entry->_next_lease = entry->_lease;
                        entry->_received = true;

                        ztm->_peers = _z_transport_peer_entry_list_push(ztm->_peers, entry);
                    } else {
                        z_free(entry);
                    }
                } else {
                    ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
                }
            } else {  // Existing peer
                entry->_received = true;

                // Check if the representing capapbilities are still the same
                if ((t_msg->_body._join._seq_num_res < Z_SN_RESOLUTION) ||
                    (t_msg->_body._join._key_id_res < Z_KID_RESOLUTION) ||
                    (t_msg->_body._join._req_id_res < Z_REQ_RESOLUTION) ||
                    (t_msg->_body._join._batch_size < Z_BATCH_SIZE)) {
                    _z_transport_peer_entry_list_drop_filter(ztm->_peers, _z_transport_peer_entry_eq, entry);
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

        case _Z_MID_CLOSE: {
            _Z_INFO("Closing session as requested by the remote peer\n");

            if (entry == NULL) {
                break;
            }
            ztm->_peers = _z_transport_peer_entry_list_drop_filter(ztm->_peers, _z_transport_peer_entry_eq, entry);

            break;
        }

        default: {
            _Z_ERROR("Unknown session message ID\n");
            break;
        }
    }

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&ztm->_mutex_peer);
#endif  // Z_MULTI_THREAD == 1

    return ret;
}

#endif  // Z_MULTICAST_TRANSPORT == 1
