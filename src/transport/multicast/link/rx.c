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

#include <stddef.h>

#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_MULTICAST_TRANSPORT == 1

_z_transport_peer_entry_t *_z_find_peer_entry(_z_transport_peer_entry_list_t *l, _z_bytes_t *remote_addr) {
    for (; l != NULL; l = _z_transport_peer_entry_list_tail(l)) {
        _z_transport_peer_entry_t *val = _z_transport_peer_entry_list_head(l);
        if (val->_remote_addr.len != remote_addr->len) continue;

        if (memcmp(val->_remote_addr.start, remote_addr->start, remote_addr->len) == 0) return val;
    }

    return NULL;
}

/*------------------ Reception helper ------------------*/
void _z_multicast_recv_t_msg_na(_z_transport_multicast_t *ztm, _z_transport_message_result_t *r, _z_bytes_t *addr) {
    _Z_DEBUG(">> recv session msg\n");
    r->_tag = _Z_RES_OK;

#if Z_MULTI_THREAD == 1
    // Acquire the lock
    _z_mutex_lock(&ztm->_mutex_rx);
#endif  // Z_MULTI_THREAD == 1

    // Prepare the buffer
    _z_zbuf_reset(&ztm->_zbuf);

    if (_Z_LINK_IS_STREAMED(ztm->_link->_capabilities)) {
        // Read the message length
        if (_z_link_recv_exact_zbuf(ztm->_link, &ztm->_zbuf, _Z_MSG_LEN_ENC_SIZE, addr) != _Z_MSG_LEN_ENC_SIZE) {
            r->_tag = _Z_RES_ERR;
            r->_value._error = _Z_ERR_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }

        size_t len = 0;
        for (int i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) len |= _z_zbuf_read(&ztm->_zbuf) << (i * 8);

        _Z_DEBUG(">> \t msg len = %hu\n", len);
        size_t writable = _z_zbuf_capacity(&ztm->_zbuf) - _z_zbuf_len(&ztm->_zbuf);
        if (writable < len) {
            r->_tag = _Z_RES_ERR;
            r->_value._error = _Z_ERR_IOBUF_NO_SPACE;
            goto EXIT_SRCV_PROC;
        }

        // Read enough bytes to decode the message
        if (_z_link_recv_exact_zbuf(ztm->_link, &ztm->_zbuf, len, addr) != len) {
            r->_tag = _Z_RES_ERR;
            r->_value._error = _Z_ERR_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }
    } else {
        if (_z_link_recv_zbuf(ztm->_link, &ztm->_zbuf, addr) == SIZE_MAX) {
            r->_tag = _Z_RES_ERR;
            r->_value._error = _Z_ERR_IO_GENERIC;
            goto EXIT_SRCV_PROC;
        }
    }

    _Z_DEBUG(">> \t transport_message_decode\n");
    _z_transport_message_decode_na(&ztm->_zbuf, r);

EXIT_SRCV_PROC:
#if Z_MULTI_THREAD == 1
    // Release the lock
    _z_mutex_unlock(&ztm->_mutex_rx);
#endif  // Z_MULTI_THREAD == 1
    __asm__("nop");
}

_z_transport_message_result_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_bytes_t *addr) {
    _z_transport_message_result_t r;

    _z_multicast_recv_t_msg_na(ztm, &r, addr);
    return r;
}

int _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                          _z_bytes_t *addr) {
#if Z_MULTI_THREAD == 1
    // Acquire and keep the lock
    _z_mutex_lock(&ztm->_mutex_peer);
#endif  // Z_MULTI_THREAD == 1

    // Mark the session that we have received data from this peer
    _z_transport_peer_entry_t *entry = _z_find_peer_entry(ztm->_peers, addr);
    switch (_Z_MID(t_msg->_header)) {
        case _Z_MID_SCOUT: {
            // Do nothing, multicast transports are not expected to handle SCOUT messages on established sessions
            break;
        }

        case _Z_MID_HELLO: {
            // Do nothing, multicast transports are not expected to handle HELLO messages on established sessions
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
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_A)) {
                if (t_msg->_body._join._version != Z_PROTO_VERSION) break;
            }

            if (entry == NULL)  // New peer
            {
                entry = (_z_transport_peer_entry_t *)z_malloc(sizeof(_z_transport_peer_entry_t));
                entry->_remote_addr = _z_bytes_duplicate(addr);
                entry->_remote_pid = _z_bytes_duplicate(&t_msg->_body._join._pid);
                if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_S))
                    entry->_sn_resolution = t_msg->_body._join._sn_resolution;
                else
                    entry->_sn_resolution = Z_SN_RESOLUTION;
                entry->_sn_resolution_half = entry->_sn_resolution / 2;

                _z_conduit_sn_list_copy(&entry->_sn_rx_sns, &t_msg->_body._join._next_sns);
                _z_conduit_sn_list_decrement(entry->_sn_resolution, &entry->_sn_rx_sns);

#if Z_DYNAMIC_MEMORY_ALLOCATION == 1
                entry->_dbuf_reliable = _z_wbuf_make(0, 1);
                entry->_dbuf_best_effort = _z_wbuf_make(0, 1);
#else
                entry->_dbuf_reliable = _z_wbuf_make(Z_FRAG_MAX_SIZE, 0);
                entry->_dbuf_best_effort = _z_wbuf_make(Z_FRAG_MAX_SIZE, 0);
#endif

                // Update lease time (set as ms during)
                entry->_lease = t_msg->_body._join._lease;
                entry->_next_lease = entry->_lease;
                entry->_received = 1;

                ztm->_peers = _z_transport_peer_entry_list_push(ztm->_peers, entry);
            } else  // Existing peer
            {
                entry->_received = 1;

                // Check if the sn resolution remains the same
                if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_S) &&
                    (entry->_sn_resolution != t_msg->_body._join._sn_resolution)) {
                    _z_transport_peer_entry_list_drop_filter(ztm->_peers, _z_transport_peer_entry_eq, entry);
                    break;
                }

                // Update SNs
                _z_conduit_sn_list_copy(&entry->_sn_rx_sns, &t_msg->_body._join._next_sns);
                _z_conduit_sn_list_decrement(entry->_sn_resolution, &entry->_sn_rx_sns);

                // Update lease time (set as ms during)
                entry->_lease = t_msg->_body._join._lease;
            }
            break;
        }

        case _Z_MID_CLOSE: {
            _Z_INFO("Closing session as requested by the remote peer\n");

            if (entry == NULL) break;

            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_I)) {
                // Check if the Peer ID matches the remote address in the knonw peer list
                if (entry->_remote_pid.len != t_msg->_body._close._pid.len ||
                    memcmp(entry->_remote_pid.start, t_msg->_body._close._pid.start, entry->_remote_pid.len) != 0)
                    break;
            }
            ztm->_peers = _z_transport_peer_entry_list_drop_filter(ztm->_peers, _z_transport_peer_entry_eq, entry);

            break;
        }

        case _Z_MID_SYNC: {
            _Z_INFO("Handling of Sync messages not implemented\n");
            break;
        }

        case _Z_MID_ACK_NACK: {
            _Z_INFO("Handling of AckNack messages not implemented\n");
            break;
        }

        case _Z_MID_KEEP_ALIVE: {
            _Z_INFO("Received _Z_KEEP_ALIVE message\n");
            if (entry == NULL) break;
            entry->_received = 1;

            break;
        }

        case _Z_MID_PING_PONG: {
            _Z_DEBUG("Handling of PingPong messages not implemented");
            break;
        }

        case _Z_MID_FRAME: {
            _Z_INFO("Received _Z_FRAME message\n");
            if (entry == NULL) break;
            entry->_received = 1;

            // Check if the SN is correct
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_R)) {
                // @TODO: amend once reliability is in place. For the time being only
                //        monothonic SNs are ensured
                if (_z_sn_precedes(entry->_sn_resolution_half, entry->_sn_rx_sns._val._plain._reliable,
                                   t_msg->_body._frame._sn))
                    entry->_sn_rx_sns._val._plain._reliable = t_msg->_body._frame._sn;
                else {
                    _z_wbuf_clear(&entry->_dbuf_reliable);
                    _Z_INFO("Reliable message dropped because it is out of order");
                    break;
                }
            } else {
                if (_z_sn_precedes(entry->_sn_resolution_half, entry->_sn_rx_sns._val._plain._best_effort,
                                   t_msg->_body._frame._sn))
                    entry->_sn_rx_sns._val._plain._best_effort = t_msg->_body._frame._sn;
                else {
                    _z_wbuf_clear(&entry->_dbuf_best_effort);
                    _Z_INFO("Best effort message dropped because it is out of order");
                    break;
                }
            }

            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_F)) {
                // Select the right defragmentation buffer
                _z_wbuf_t *dbuf =
                    _Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_R) ? &entry->_dbuf_reliable : &entry->_dbuf_best_effort;

                uint8_t drop = 0;
                if (_z_wbuf_len(dbuf) + t_msg->_body._frame._payload._fragment.len > Z_FRAG_MAX_SIZE) {
                    // Filling the wbuf capacity as a way to signling the last fragment to reset the dbuf
                    // Otherwise, last (smaller) fragments can be understood as a complete message
                    _z_wbuf_write_bytes(dbuf, t_msg->_body._frame._payload._fragment.start, 0,
                                        _z_wbuf_space_left(dbuf));
                    drop = 1;
                } else {
                    // Add the fragment to the defragmentation buffer
                    _z_wbuf_write_bytes(dbuf, t_msg->_body._frame._payload._fragment.start, 0,
                                        t_msg->_body._frame._payload._fragment.len);
                }

                // Check if this is the last fragment
                if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_E)) {
                    // Drop message if it is bigger the max buffer size
                    if (drop == 1) {
                        _z_wbuf_reset(dbuf);
                        break;
                    }

                    // Convert the defragmentation buffer into a decoding buffer
                    _z_zbuf_t zbf = _z_wbuf_to_zbuf(dbuf);

                    // Decode the zenoh message
                    _z_zenoh_message_result_t r_zm = _z_zenoh_message_decode(&zbf);
                    if (r_zm._tag == _Z_RES_OK) {
                        _z_zenoh_message_t d_zm = r_zm._value._zenoh_message;
                        _z_handle_zenoh_message(ztm->_session, &d_zm);

                        // Clear must be explicitly called for fragmented zenoh messages.
                        // Non-fragmented zenoh messages are released when their transport message is released.
                        _z_msg_clear(&d_zm);
                    }

                    // Free the decoding buffer
                    _z_zbuf_clear(&zbf);
                    // Reset the defragmentation buffer
                    _z_wbuf_reset(dbuf);
                }
            } else {
                // Handle all the zenoh message, one by one
                unsigned int len = _z_vec_len(&t_msg->_body._frame._payload._messages);
                for (unsigned int i = 0; i < len; i++)
                    _z_handle_zenoh_message(
                        ztm->_session, (_z_zenoh_message_t *)_z_vec_get(&t_msg->_body._frame._payload._messages, i));
            }
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

    return _Z_RES_OK;
}

#endif  // Z_MULTICAST_TRANSPORT == 1
