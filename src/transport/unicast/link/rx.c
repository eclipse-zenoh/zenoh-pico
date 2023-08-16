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

#if Z_UNICAST_TRANSPORT == 1

int8_t _z_unicast_recv_t_msg_na(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg) {
    _Z_DEBUG(">> recv session msg\n");
    int8_t ret = _Z_RES_OK;
#if Z_MULTI_THREAD == 1
    // Acquire the lock
    _z_mutex_lock(&ztu->_mutex_rx);
#endif  // Z_MULTI_THREAD == 1

    size_t to_read = 0;
    do {
        if (_Z_LINK_IS_STREAMED(ztu->_link._capabilities) == true) {
            if (_z_zbuf_len(&ztu->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
                if (_z_zbuf_len(&ztu->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_zbuf_compact(&ztu->_zbuf);
                    ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                    continue;
                }
            }

            for (uint8_t i = 0; i < _Z_MSG_LEN_ENC_SIZE; i++) {
                to_read |= _z_zbuf_read(&ztu->_zbuf) << (i * (uint8_t)8);
            }

            if (_z_zbuf_len(&ztu->_zbuf) < to_read) {
                _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
                if (_z_zbuf_len(&ztu->_zbuf) < to_read) {
                    _z_zbuf_set_rpos(&ztu->_zbuf, _z_zbuf_get_rpos(&ztu->_zbuf) - _Z_MSG_LEN_ENC_SIZE);
                    _z_zbuf_compact(&ztu->_zbuf);
                    ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                    continue;
                }
            }
        } else {
            _z_zbuf_compact(&ztu->_zbuf);
            to_read = _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
            if (to_read == SIZE_MAX) {
                ret = _Z_ERR_TRANSPORT_RX_FAILED;
            }
        }
    } while (false);  // The 1-iteration loop to use continue to break the entire loop on error

    // Wrap the main buffer for to_read bytes
    _z_zbuf_view(&ztu->_zbuf, to_read);

    if (ret == _Z_RES_OK) {
        _Z_DEBUG(">> \t transport_message_decode\n");
        ret = _z_transport_message_decode_na(t_msg, &ztu->_zbuf);

        // Mark the session that we have received data
        if (ret == _Z_RES_OK) {
            ztu->_received = true;
        }
    }

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&ztu->_mutex_rx);
#endif  // Z_MULTI_THREAD == 1

    return ret;
}

int8_t _z_unicast_recv_t_msg(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg) {
    return _z_unicast_recv_t_msg_na(ztu, t_msg);
}

int8_t _z_unicast_handle_transport_message(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg) {
    switch (_Z_MID(t_msg->_header)) {
        case _Z_MID_SCOUT: {
            _Z_INFO("Handling of Scout messages not implemented\n");
            break;
        }

        case _Z_MID_HELLO: {
            // Do nothing, zenoh-pico clients are not expected to handle hello messages
            break;
        }

        case _Z_MID_INIT: {
            // Do nothing, zenoh clients are not expected to handle accept messages on established sessions
            break;
        }

        case _Z_MID_OPEN: {
            // Do nothing, zenoh clients are not expected to handle accept messages on established sessions
            break;
        }

        case _Z_MID_CLOSE: {
            _Z_INFO("Closing session as requested by the remote peer\n");
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
            _Z_INFO("Received Z_KEEP_ALIVE message\n");
            break;
        }

        case _Z_MID_PING_PONG: {
            _Z_INFO("Handling of PingPong messages not implemented\n");
            break;
        }

        case _Z_MID_FRAME: {
            _Z_INFO("Received Z_FRAME message\n");
            // Check if the SN is correct
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_R) == true) {
                // @TODO: amend once reliability is in place. For the time being only
                //        monothonic SNs are ensured
                if (_z_sn_precedes(ztu->_sn_resolution_half, ztu->_sn_rx_reliable, t_msg->_body._frame._sn) == true) {
                    ztu->_sn_rx_reliable = t_msg->_body._frame._sn;
                } else {
                    _z_wbuf_clear(&ztu->_dbuf_reliable);
                    _Z_INFO("Reliable message dropped because it is out of order\n");
                    break;
                }
            } else {
                if (_z_sn_precedes(ztu->_sn_resolution_half, ztu->_sn_rx_best_effort, t_msg->_body._frame._sn) ==
                    true) {
                    ztu->_sn_rx_best_effort = t_msg->_body._frame._sn;
                } else {
                    _z_wbuf_clear(&ztu->_dbuf_best_effort);
                    _Z_INFO("Best effort message dropped because it is out of order\n");
                    break;
                }
            }

            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_F) == true) {
                // Select the right defragmentation buffer
                _z_wbuf_t *dbuf =
                    _Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_R) ? &ztu->_dbuf_reliable : &ztu->_dbuf_best_effort;

                _Bool drop = false;
                if ((_z_wbuf_len(dbuf) + t_msg->_body._frame._payload._fragment.len) > Z_FRAG_MAX_SIZE) {
                    // Filling the wbuf capacity as a way to signling the last fragment to reset the dbuf
                    // Otherwise, last (smaller) fragments can be understood as a complete message
                    _z_wbuf_write_bytes(dbuf, t_msg->_body._frame._payload._fragment.start, 0,
                                        _z_wbuf_space_left(dbuf));
                    drop = true;
                } else {
                    // Add the fragment to the defragmentation buffer
                    _z_wbuf_write_bytes(dbuf, t_msg->_body._frame._payload._fragment.start, 0,
                                        t_msg->_body._frame._payload._fragment.len);
                }

                // Check if this is the last fragment
                if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_E) == true) {
                    // Drop message if it is bigger the max buffer size
                    if (drop == true) {
                        _z_wbuf_reset(dbuf);
                        break;
                    }

                    // Convert the defragmentation buffer into a decoding buffer
                    _z_zbuf_t zbf = _z_wbuf_to_zbuf(dbuf);

                    // Decode the zenoh message
                    _z_zenoh_message_t zm;
                    int8_t ret = _z_zenoh_message_decode(&zm, &zbf);
                    if (ret == _Z_RES_OK) {
                        _z_handle_zenoh_message(ztu->_session, &zm);

                        // Clear must be explicitly called for fragmented zenoh messages.
                        // Non-fragmented zenoh messages are released when their transport message is released.
                        _z_msg_clear(&zm);
                    }

                    // Free the decoding buffer
                    _z_zbuf_clear(&zbf);
                    // Reset the defragmentation buffer
                    _z_wbuf_reset(dbuf);
                }

                break;
            } else {
                // Handle all the zenoh message, one by one
                size_t len = _z_vec_len(&t_msg->_body._frame._payload._messages);
                for (size_t i = 0; i < len; i++) {
                    _z_handle_zenoh_message(
                        ztu->_session, (_z_zenoh_message_t *)_z_vec_get(&t_msg->_body._frame._payload._messages, i));
                }
            }
            break;
        }

        default: {
            _Z_ERROR("Unknown session message ID\n");
            break;
        }
    }

    return _Z_RES_OK;
}

#endif  // Z_UNICAST_TRANSPORT == 1
