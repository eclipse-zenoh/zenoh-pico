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

#include "zenoh-pico/transport/unicast/rx.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/common/rx.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

int8_t _z_unicast_recv_t_msg_na(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg) {
    _Z_DEBUG(">> recv session msg");
    int8_t ret = _Z_RES_OK;
#if Z_FEATURE_MULTI_THREAD == 1
    // Acquire the lock
    _z_mutex_lock(&ztu->_mutex_rx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    size_t to_read = 0;
    do {
        switch (ztu->_link._cap._flow) {
            // Stream capable links
            case Z_LINK_CAP_FLOW_STREAM:
                if (_z_zbuf_len(&ztu->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
                    if (_z_zbuf_len(&ztu->_zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                        _z_zbuf_compact(&ztu->_zbuf);
                        ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                        continue;
                    }
                }
                // Get stream size
                to_read = _z_read_stream_size(&ztu->_zbuf);
                // Read data
                if (_z_zbuf_len(&ztu->_zbuf) < to_read) {
                    _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
                    if (_z_zbuf_len(&ztu->_zbuf) < to_read) {
                        _z_zbuf_set_rpos(&ztu->_zbuf, _z_zbuf_get_rpos(&ztu->_zbuf) - _Z_MSG_LEN_ENC_SIZE);
                        _z_zbuf_compact(&ztu->_zbuf);
                        ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                        continue;
                    }
                }
                break;
            // Datagram capable links
            case Z_LINK_CAP_FLOW_DATAGRAM:
                _z_zbuf_compact(&ztu->_zbuf);
                to_read = _z_link_recv_zbuf(&ztu->_link, &ztu->_zbuf, NULL);
                if (to_read == SIZE_MAX) {
                    ret = _Z_ERR_TRANSPORT_RX_FAILED;
                }
                break;
            default:
                break;
        }
    } while (false);  // The 1-iteration loop to use continue to break the entire loop on error

    if (ret == _Z_RES_OK) {
        _Z_DEBUG(">> \t transport_message_decode");
        ret = _z_transport_message_decode(t_msg, &ztu->_zbuf);

        // Mark the session that we have received data
        if (ret == _Z_RES_OK) {
            ztu->_received = true;
        }
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_mutex_unlock(&ztu->_mutex_rx);
#endif  // Z_FEATURE_MULTI_THREAD == 1

    return ret;
}

int8_t _z_unicast_recv_t_msg(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg) {
    return _z_unicast_recv_t_msg_na(ztu, t_msg);
}

int8_t _z_unicast_handle_transport_message(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg) {
    int8_t ret = _Z_RES_OK;

    switch (_Z_MID(t_msg->_header)) {
        case _Z_MID_T_FRAME: {
            _Z_INFO("Received Z_FRAME message");
            // Check if the SN is correct
            if (_Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_FRAME_R) == true) {
                // @TODO: amend once reliability is in place. For the time being only
                //        monotonic SNs are ensured
                if (_z_sn_precedes(ztu->_sn_res, ztu->_sn_rx_reliable, t_msg->_body._frame._sn) == true) {
                    ztu->_sn_rx_reliable = t_msg->_body._frame._sn;
                } else {
#if Z_FEATURE_FRAGMENTATION == 1
                    _z_wbuf_clear(&ztu->_dbuf_reliable);
#endif
                    _Z_INFO("Reliable message dropped because it is out of order");
                    break;
                }
            } else {
                if (_z_sn_precedes(ztu->_sn_res, ztu->_sn_rx_best_effort, t_msg->_body._frame._sn) == true) {
                    ztu->_sn_rx_best_effort = t_msg->_body._frame._sn;
                } else {
#if Z_FEATURE_FRAGMENTATION == 1
                    _z_wbuf_clear(&ztu->_dbuf_best_effort);
#endif
                    _Z_INFO("Best effort message dropped because it is out of order");
                    break;
                }
            }

            // Handle all the zenoh message, one by one
            size_t len = _z_vec_len(&t_msg->_body._frame._messages);
            for (size_t i = 0; i < len; i++) {
                _z_handle_network_message(ztu->_session,
                                          (_z_zenoh_message_t *)_z_vec_get(&t_msg->_body._frame._messages, i),
                                          _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE);
            }

            break;
        }

        case _Z_MID_T_FRAGMENT: {
            _Z_INFO("Received Z_FRAGMENT message");
#if Z_FEATURE_FRAGMENTATION == 1
            _z_wbuf_t *dbuf = _Z_HAS_FLAG(t_msg->_header, _Z_FLAG_T_FRAGMENT_R)
                                  ? &ztu->_dbuf_reliable
                                  : &ztu->_dbuf_best_effort;  // Select the right defragmentation buffer

            _Bool drop = false;
            if ((_z_wbuf_len(dbuf) + t_msg->_body._fragment._payload.len) > Z_FRAG_MAX_SIZE) {
                // Filling the wbuf capacity as a way to signal the last fragment to reset the dbuf
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
                    _z_handle_network_message(ztu->_session, &zm, _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE);
                    _z_msg_clear(&zm);  // Clear must be explicitly called for fragmented zenoh messages. Non-fragmented
                                        // zenoh messages are released when their transport message is released.
                } else {
                    _Z_DEBUG("Failed to decode defragmented message");
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
            _Z_INFO("Received Z_KEEP_ALIVE message");
            break;
        }

        case _Z_MID_T_INIT: {
            // Do nothing, zenoh clients are not expected to handle accept messages on established sessions
            break;
        }

        case _Z_MID_T_OPEN: {
            // Do nothing, zenoh clients are not expected to handle accept messages on established sessions
            break;
        }

        case _Z_MID_T_CLOSE: {
            _Z_INFO("Closing session as requested by the remote peer");
            ret = _Z_ERR_CONNECTION_CLOSED;
            break;
        }

        default: {
            _Z_ERROR("Unknown session message ID");
            break;
        }
    }

    return ret;
}
#else
int8_t _z_unicast_recv_t_msg(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(t_msg);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

int8_t _z_unicast_handle_transport_message(_z_transport_unicast_t *ztu, _z_transport_message_t *t_msg) {
    _ZP_UNUSED(ztu);
    _ZP_UNUSED(t_msg);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1
