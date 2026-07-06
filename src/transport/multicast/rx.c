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
#include <string.h>

#include "zenoh-pico/collections/algorithms_template.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/codec/network.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/definitions/transport.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/multicast/connectivity.h"
#include "zenoh-pico/transport/multicast/rx.h"
#include "zenoh-pico/transport/multicast/transport.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_MULTICAST_TRANSPORT == 1
z_result_t _z_multicast_recv_zbuf(_z_transport_multicast_t *ztm, size_t *to_read) {
    z_result_t ret = _Z_RES_OK;

    do {
        // Read bytes from socket to the main buffer
        switch (ztm->_common._link->_cap._flow) {
            case Z_LINK_CAP_FLOW_STREAM:
                if (_z_zbuf_readable_len(&ztm->_common._zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                    _z_link_recv_zbuf(ztm->_common._link, &ztm->_common._zbuf, &ztm->_zbuf_addr);
                    if (_z_zbuf_readable_len(&ztm->_common._zbuf) < _Z_MSG_LEN_ENC_SIZE) {
                        _z_zbuf_compact(&ztm->_common._zbuf);
                        ret = _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES;
                        break;
                    }
                }
                // Get stream size
                *to_read = _z_read_stream_size(&ztm->_common._zbuf);
                // Read data
                if (_z_zbuf_readable_len(&ztm->_common._zbuf) < *to_read) {
                    _z_link_recv_zbuf(ztm->_common._link, &ztm->_common._zbuf, NULL);
                    if (_z_zbuf_readable_len(&ztm->_common._zbuf) < *to_read) {
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
                if (_z_zbuf_readable_len(&ztm->_common._zbuf) == 0) {
                    _z_zbuf_compact(&ztm->_common._zbuf);
                    *to_read = _z_link_recv_zbuf(ztm->_common._link, &ztm->_common._zbuf, &ztm->_zbuf_addr);
                    if (*to_read == SIZE_MAX) {
                        ret = _Z_ERR_TRANSPORT_RX_FAILED;
                    }
                } else {
                    *to_read = _z_zbuf_readable_len(&ztm->_common._zbuf);
                }
                break;
            default:
                break;
        }
    } while (false);  // The 1-iteration loop to use continue to break the entire loop on error

    return ret;
}

z_result_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg) {
    _Z_DEBUG(">> recv session msg");
    size_t to_read = 0;

    z_result_t ret = _z_multicast_recv_zbuf(ztm, &to_read);

    if (ret == _Z_RES_OK) {
        _Z_DEBUG(">> \t transport_message_decode: %ju", (uintmax_t)_z_zbuf_readable_len(&ztm->_common._zbuf));

        // Wrap the main buffer to_read bytes
        _z_zbuf_t zbuf = _z_zbuf_view(&ztm->_common._zbuf, to_read);
        ret = _z_transport_message_decode(t_msg, &zbuf);
        if (ret == _Z_RES_OK) {
            _z_zbuf_set_rpos(&ztm->_common._zbuf, _z_zbuf_get_rpos(&ztm->_common._zbuf) + _z_zbuf_get_rpos(&zbuf));
        } else {
            _Z_ERROR("Malformed transport message: %d", ret);
            _z_zbuf_set_rpos(&ztm->_common._zbuf, _z_zbuf_get_rpos(&ztm->_common._zbuf) + to_read);
        }
    }

    return ret;
}
#else
z_result_t _z_multicast_recv_t_msg(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(t_msg);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1

#if Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1

#if Z_FEATURE_CONNECTIVITY == 1
static z_result_t _z_multicast_remote_addr_to_endpoint(const _z_transport_multicast_t *ztm,
                                                       const _z_slice_t *remote_addr, _z_string_t *endpoint) {
    char addr[96] = {0};
    _z_locator_t locator;
    const uint8_t *bytes = remote_addr->start;
    size_t address_len = 0;
    uint16_t port = 0;

    *endpoint = _z_string_null();
    if (ztm == NULL || ztm->_common._link == NULL || remote_addr == NULL || bytes == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    if (remote_addr->len == sizeof(uint32_t) + sizeof(uint16_t)) {
        address_len = sizeof(uint32_t);
        port = ((uint16_t)bytes[4] << 8) | (uint16_t)bytes[5];
    } else if (remote_addr->len == 16 + sizeof(uint16_t)) {
        address_len = 16;
        port = ((uint16_t)bytes[16] << 8) | (uint16_t)bytes[17];
    } else {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    _Z_RETURN_IF_ERR(_z_ip_port_to_endpoint(bytes, address_len, port, addr, sizeof(addr)));

    _z_locator_init(&locator);
    if (_z_string_check(&ztm->_common._link->_endpoint._locator._protocol)) {
        locator._protocol = _z_string_alias(ztm->_common._link->_endpoint._locator._protocol);
    } else {
        locator._protocol = _z_string_alias_str("udp");
    }
    locator._address = _z_string_alias_str(addr);

    *endpoint = _z_locator_to_string(&locator);
    _z_locator_clear(&locator);
    if (!_z_string_check(endpoint)) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    return _Z_RES_OK;
}
#endif

static z_result_t _z_multicast_handle_frame(_z_transport_multicast_t *ztm, uint8_t header, _z_t_msg_frame_t *msg,
                                            _z_address_to_transport_peer_multicast_hmap_iter_t iter) {
    // Check peer
    if (iter == _z_address_to_transport_peer_multicast_hmap_end(&ztm->_peers)) {
        _Z_INFO("Dropping _Z_FRAME from unknown peer");
        return _Z_RES_OK;
    }
    _z_transport_peer_multicast_t *peer = &_z_address_to_transport_peer_multicast_hmap_at(&ztm->_peers, iter)->val;
    // Note that we receive data from peer
    peer->common._received = true;

    z_reliability_t tmsg_reliability;
    // Check if the SN is correct
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_FRAME_R)) {
        tmsg_reliability = Z_RELIABILITY_RELIABLE;
        // @TODO: amend once reliability is in place. For the time being only
        //        monotonic SNs are ensured
        if (_z_sn_precedes(peer->_sn_res, peer->_sn_rx_sns._val._plain._reliable, msg->_sn)) {
            peer->_sn_rx_sns._val._plain._reliable = msg->_sn;
        } else {
#if Z_FEATURE_FRAGMENTATION == 1
            peer->common._state_reliable = _Z_DBUF_STATE_NULL;
            _z_wbuf_clear(&peer->common._dbuf_reliable);
#endif
            _Z_INFO("Reliable message dropped because it is out of order");
            return _Z_RES_OK;
        }
    } else {
        tmsg_reliability = Z_RELIABILITY_BEST_EFFORT;
        if (_z_sn_precedes(peer->_sn_res, peer->_sn_rx_sns._val._plain._best_effort, msg->_sn)) {
            peer->_sn_rx_sns._val._plain._best_effort = msg->_sn;
        } else {
#if Z_FEATURE_FRAGMENTATION == 1
            peer->common._state_best_effort = _Z_DBUF_STATE_NULL;
            _z_wbuf_clear(&peer->common._dbuf_best_effort);
#endif
            _Z_INFO("Best effort message dropped because it is out of order");
            return _Z_RES_OK;
        }
    }
    // Handle all the zenoh message, one by one
    // From this point, memory cleaning must be handled by the network message layer
    _z_network_message_t curr_nmsg = {0};
    _z_zbuf_t buf = _z_slice_as_zbuf(_z_slice_view_deref(&msg->_payload));
    while (_z_zbuf_readable_len(&buf) > 0) {
        _Z_RETURN_IF_ERR(_z_network_message_decode(&curr_nmsg, &buf));
        curr_nmsg._reliability = tmsg_reliability;
        _Z_RETURN_IF_ERR(_z_handle_network_message(&ztm->_common, &curr_nmsg, iter));
    }
    return _Z_RES_OK;
}

static z_result_t _z_multicast_handle_fragment(_z_transport_multicast_t *ztm, uint8_t header, _z_t_msg_fragment_t *msg,
                                               _z_address_to_transport_peer_multicast_hmap_iter_t iter) {
    z_result_t ret = _Z_RES_OK;
#if Z_FEATURE_FRAGMENTATION == 1
    // Check peer
    if (iter == _z_address_to_transport_peer_multicast_hmap_end(&ztm->_peers)) {
        _Z_INFO("Dropping Z_FRAGMENT from unknown peer");
        return _Z_RES_OK;
    }
    _z_transport_peer_multicast_t *peer = &_z_address_to_transport_peer_multicast_hmap_at(&ztm->_peers, iter)->val;
    // Note that we receive data from the peer
    peer->common._received = true;

    _z_wbuf_t *dbuf;
    uint8_t *dbuf_state;
    z_reliability_t tmsg_reliability;
    bool consecutive;

    // Select the right defragmentation buffer
    if (_Z_HAS_FLAG(header, _Z_FLAG_T_FRAME_R)) {
        tmsg_reliability = Z_RELIABILITY_RELIABLE;
        // Check SN
        // @TODO: amend once reliability is in place. For the time being only
        //        monotonic SNs are ensured
        if (_z_sn_precedes(peer->_sn_res, peer->_sn_rx_sns._val._plain._reliable, msg->_sn)) {
            consecutive = _z_sn_consecutive(peer->_sn_res, peer->_sn_rx_sns._val._plain._reliable, msg->_sn);
            peer->_sn_rx_sns._val._plain._reliable = msg->_sn;
            dbuf = &peer->common._dbuf_reliable;
            dbuf_state = &peer->common._state_reliable;
        } else {
            _z_wbuf_clear(&peer->common._dbuf_reliable);
            peer->common._state_reliable = _Z_DBUF_STATE_NULL;
            _Z_INFO("Reliable message dropped because it is out of order");
            return _Z_RES_OK;
        }
    } else {
        tmsg_reliability = Z_RELIABILITY_BEST_EFFORT;
        // Check SN
        if (_z_sn_precedes(peer->_sn_res, peer->_sn_rx_sns._val._plain._best_effort, msg->_sn)) {
            consecutive = _z_sn_consecutive(peer->_sn_res, peer->_sn_rx_sns._val._plain._best_effort, msg->_sn);
            peer->_sn_rx_sns._val._plain._best_effort = msg->_sn;
            dbuf = &peer->common._dbuf_best_effort;
            dbuf_state = &peer->common._state_best_effort;
        } else {
            _z_wbuf_clear(&peer->common._dbuf_best_effort);
            peer->common._state_best_effort = _Z_DBUF_STATE_NULL;
            _Z_INFO("Best effort message dropped because it is out of order");
            return _Z_RES_OK;
        }
    }
    if (!consecutive && (_z_wbuf_len(dbuf) > 0)) {
        _z_wbuf_clear(dbuf);
        *dbuf_state = _Z_DBUF_STATE_NULL;
        _Z_INFO("Defragmentation buffer dropped because non-consecutive fragments received");
        return _Z_RES_OK;
    }
    // Handle fragment markers
    if (_Z_PATCH_HAS_FRAGMENT_MARKERS(peer->common._patch)) {
        if (msg->first) {
            _z_wbuf_reset(dbuf);
        } else if (_z_wbuf_len(dbuf) == 0) {
            _Z_INFO("First fragment received without the first marker");
            return _Z_RES_OK;
        }
        if (msg->drop) {
            _z_wbuf_reset(dbuf);
            return _Z_RES_OK;
        }
    }
    // Allocate buffer if needed
    if (*dbuf_state == _Z_DBUF_STATE_NULL) {
        if (_z_wbuf_init(dbuf, Z_FRAG_MAX_SIZE, false) != _Z_RES_OK) {
            _Z_ERROR("Not enough memory to allocate peer defragmentation buffer");
            _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
        }
        *dbuf_state = _Z_DBUF_STATE_INIT;
    }
    // Process fragment data
    if (*dbuf_state == _Z_DBUF_STATE_INIT) {
        const _z_slice_t *payload = _z_slice_view_deref(&msg->_payload);
        // Check overflow
        if ((_z_wbuf_len(dbuf) + payload->len) > Z_FRAG_MAX_SIZE) {
            *dbuf_state = _Z_DBUF_STATE_OVERFLOW;
        } else {
            // Fill buffer
            _z_wbuf_write_bytes(dbuf, payload->start, 0, payload->len);
        }
    }
    // Process final fragment
    if (!_Z_HAS_FLAG(header, _Z_FLAG_T_FRAGMENT_M)) {
        // Drop message if it exceeds the fragmentation size
        if (*dbuf_state == _Z_DBUF_STATE_OVERFLOW) {
            _Z_INFO("Fragment dropped because defragmentation buffer has overflown");
            _z_wbuf_clear(dbuf);
            *dbuf_state = _Z_DBUF_STATE_NULL;
            return _Z_RES_OK;
        }
        // Convert the defragmentation buffer into a decoding buffer
        _z_zbuf_t zbf = _z_wbuf_moved_as_zbuf(dbuf);
        if (_z_zbuf_capacity(&zbf) == 0) {
            _Z_ERROR("Failed to convert defragmentation buffer into a decoding buffer!");
            _z_wbuf_clear(dbuf);
            *dbuf_state = _Z_DBUF_STATE_NULL;
            _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
        }
        // Decode message
        _z_network_message_t zm = {0};
        ret = _z_network_message_decode(&zm, &zbf);
        zm._reliability = tmsg_reliability;
        if (ret == _Z_RES_OK) {
            _z_handle_network_message(&ztm->_common, &zm, iter);
        } else {
            _Z_INFO("Failed to decode defragmented message");
            _Z_ERROR_LOG(_Z_ERR_MESSAGE_DESERIALIZATION_FAILED);
            ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
        // Free the decoding buffer
        _z_zbuf_clear(&zbf);
        *dbuf_state = _Z_DBUF_STATE_NULL;
    }
#else
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(header);
    _ZP_UNUSED(msg);
    _ZP_UNUSED(entry);
    _Z_INFO("Fragment dropped because fragmentation feature is deactivated");
#endif
    return ret;
}

static z_result_t _z_multicast_handle_join_existing_peer(_z_transport_multicast_t *ztm, _z_slice_t *addr,
                                                         _z_t_msg_join_t *msg,
                                                         _z_address_to_transport_peer_multicast_hmap_iter_t iter) {
    (void)ztm;
    _z_transport_peer_multicast_t *peer = &_z_address_to_transport_peer_multicast_hmap_at(&ztm->_peers, iter)->val;
    // Note that we receive data from the peer
    peer->common._received = true;

    // Check representing capabilities
    if ((msg->_seq_num_res != Z_SN_RESOLUTION) || (msg->_req_id_res != Z_REQ_RESOLUTION) ||
        (msg->_batch_size != Z_BATCH_MULTICAST_SIZE)) {
        _Z_WARN("Distant node at %.*s is incompatible config wise", (int)addr->len, (const char *)addr->start);
        return _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION;
    }
    // Update SNs
    _z_conduit_sn_list_copy(&peer->_sn_rx_sns, &msg->_next_sn);
    _z_conduit_sn_list_decrement(peer->_sn_res, &peer->_sn_rx_sns);
    // Update lease time (set as ms during)
    peer->_lease = msg->_lease;
    return _Z_RES_OK;
}

static z_result_t _z_multicast_handle_join_new_peer(_z_transport_multicast_t *ztm, _z_slice_t *addr,
                                                    _z_t_msg_join_t *msg) {
    // If the new node has less representing capabilities then we can't communicate
    if ((msg->_seq_num_res != Z_SN_RESOLUTION) || (msg->_req_id_res != Z_REQ_RESOLUTION) ||
        (msg->_batch_size != Z_BATCH_MULTICAST_SIZE)) {
        _Z_INFO("Couldn't accept peer because distant node at %.*s is incompatible config wise.", (int)addr->len,
                (const char *)addr->start);
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION);
    }
    // Initialize entry
    _z_slice_t remote_addr = _z_slice_null();
    _Z_RETURN_IF_ERR(_z_slice_copy(&remote_addr, addr));
    _z_transport_peer_mutex_lock(&ztm->_common);
    _z_address_to_transport_peer_multicast_hmap_iter_t iter =
        _z_address_to_transport_peer_multicast_hmap_insert(&ztm->_peers, &remote_addr, NULL);
    if (iter == _z_address_to_transport_peer_multicast_hmap_end(&ztm->_peers)) {
        _z_transport_peer_mutex_unlock(&ztm->_common);
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    _z_transport_peer_multicast_t *entry = &_z_address_to_transport_peer_multicast_hmap_at(&ztm->_peers, iter)->val;
    entry->_sn_res = _z_sn_max(msg->_seq_num_res);
    _z_conduit_sn_list_copy(&entry->_sn_rx_sns, &msg->_next_sn);
    _z_conduit_sn_list_decrement(entry->_sn_res, &entry->_sn_rx_sns);
    // Update lease time (set as ms during)
    entry->_lease = msg->_lease;
    entry->common._remote_zid = msg->_zid;
    entry->common._remote_whatami = msg->_whatami;
    entry->common._received = true;
#if Z_FEATURE_CONNECTIVITY == 1
    entry->common._link_src = _z_string_null();
    entry->common._link_dst = _z_string_null();
    if (ztm->_common._link != NULL) {
        entry->common._link_src = _z_endpoint_to_string(&ztm->_common._link->_endpoint);
        (void)_z_multicast_remote_addr_to_endpoint(ztm, addr, &entry->common._link_dst);
    }
#endif
#if Z_FEATURE_FRAGMENTATION == 1
    entry->common._patch = msg->_patch < _Z_CURRENT_PATCH ? msg->_patch : _Z_CURRENT_PATCH;
    entry->common._state_reliable = _Z_DBUF_STATE_NULL;
    entry->common._state_best_effort = _Z_DBUF_STATE_NULL;
    entry->common._dbuf_reliable = _z_wbuf_null();
    entry->common._dbuf_best_effort = _z_wbuf_null();
#endif
    _z_transport_peer_mutex_unlock(&ztm->_common);
    _zp_multicast_report_connected_event(ztm, iter);

    return _Z_RES_OK;
}

static z_result_t _z_multicast_handle_join(_z_transport_multicast_t *ztm, _z_slice_t *addr, _z_t_msg_join_t *msg,
                                           _z_address_to_transport_peer_multicast_hmap_iter_t iter) {
    // Check proto version
    if (msg->_version != Z_PROTO_VERSION) {
        _Z_INFO("Couldn't accept peer because distant node at %.*s is incompatible config wise", (int)addr->len,
                (const char *)addr->start);
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    z_result_t ret;
    if (iter == _z_address_to_transport_peer_multicast_hmap_end(&ztm->_peers)) {
        ret = _z_multicast_handle_join_new_peer(ztm, addr, msg);
    } else {
        ret = _z_multicast_handle_join_existing_peer(ztm, addr, msg, iter);
    }
    return ret;
}

z_result_t _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                                 _z_slice_t *addr) {
    z_result_t ret = _Z_RES_OK;
    // Mark the session that we have received data from this peer
    _z_address_to_transport_peer_multicast_hmap_iter_t iter =
        _z_address_to_transport_peer_multicast_hmap_get_iter(&ztm->_peers, addr);
    if (iter == _z_address_to_transport_peer_multicast_hmap_end(&ztm->_peers) &&
        _Z_MID(t_msg->_header) != _Z_MID_T_JOIN) {
        _Z_INFO("Received message from unknown peer %.*s", (int)addr->len, (const char *)addr->start);
        return _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN;
    }
    switch (_Z_MID(t_msg->_header)) {
        case _Z_MID_T_FRAME: {
            _Z_DEBUG("Received _Z_FRAME message");
            ret = _z_multicast_handle_frame(ztm, t_msg->_header, &t_msg->_body._frame, iter);
            break;
        }

        case _Z_MID_T_FRAGMENT:
            _Z_DEBUG("Received Z_FRAGMENT message");
            ret = _z_multicast_handle_fragment(ztm, t_msg->_header, &t_msg->_body._fragment, iter);
            break;

        case _Z_MID_T_KEEP_ALIVE: {
            _Z_DEBUG("Received _Z_KEEP_ALIVE message");
            _z_address_to_transport_peer_multicast_hmap_at(&ztm->_peers, iter)->val.common._received = true;
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
            ret = _z_multicast_handle_join(ztm, addr, &t_msg->_body._join, iter);
            break;
        }

        case _Z_MID_T_CLOSE: {
            _Z_INFO("Closing connection as requested by the remote peer");
            _zp_multicast_report_disconnected_event(ztm, iter);
            _zp_multicast_remove_peer_by_iter(ztm, iter);
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
z_result_t _z_multicast_handle_transport_message(_z_transport_multicast_t *ztm, _z_transport_message_t *t_msg,
                                                 _z_slice_t *addr) {
    _ZP_UNUSED(ztm);
    _ZP_UNUSED(t_msg);
    _ZP_UNUSED(addr);
    _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NOT_AVAILABLE);
}
#endif  // Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1
