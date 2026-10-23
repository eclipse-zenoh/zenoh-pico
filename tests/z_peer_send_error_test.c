// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/transport/common/tx.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1 && Z_FEATURE_UNICAST_PEER == 1

static _z_sys_net_socket_t *peer_sockets[3];
static unsigned failed_peers;
static size_t write_calls[3];
static uint8_t received[3][2048];
static size_t received_len[3];

static size_t write_peer(const _z_link_t *link, const uint8_t *data, size_t len, _z_sys_net_socket_t *socket) {
    (void)link;
    for (size_t i = 0; i < 3; i++) {
        if (socket == peer_sockets[i]) {
            write_calls[i]++;
            if ((failed_peers & (1u << i)) != 0) {
                return SIZE_MAX;
            }
            ASSERT_TRUE(received_len[i] + len <= sizeof(received[i]));
            memcpy(received[i] + received_len[i], data, len);
            received_len[i] += len;
            return len;
        }
    }
    ASSERT_TRUE(false);
    return SIZE_MAX;
}

static void test_send_errors(int mode) {
    const unsigned failure_masks[] = {0, 1, 2, 4, 7};
    uint8_t expected[2048];
    size_t expected_len = 0;
    for (size_t iteration = 0; iteration < sizeof(failure_masks) / sizeof(failure_masks[0]); iteration++) {
        _z_session_t session = {0};
        _z_link_t link = {0};
        session._mode = Z_WHATAMI_PEER;
        session._tp._type = _Z_TRANSPORT_UNICAST_TYPE;
        _z_transport_unicast_t *transport = &session._tp._transport._unicast;
        _z_transport_common_t *common = &transport->_common;
        common->_link = &link;
        common->_state = _Z_TRANSPORT_STATE_OPEN;
        common->_sn_res = UINT16_MAX;
        common->_wbuf = _z_wbuf_make(64, false);
        link._cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
        link._write_f = write_peer;
#if Z_FEATURE_MULTI_THREAD == 1
        ASSERT_OK(_z_mutex_init(&common->_mutex_tx));
        ASSERT_OK(_z_mutex_rec_init(&common->_mutex_peer));
#endif
        for (size_t i = 3; i > 0; i--) {
            transport->_peers = _z_transport_peer_unicast_slist_push_empty(transport->_peers);
            _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(transport->_peers);
            memset(peer, 0, sizeof(*peer));
            peer_sockets[i - 1] = &peer->_socket;
        }
        failed_peers = failure_masks[iteration];
        memset(write_calls, 0, sizeof(write_calls));
        memset(received_len, 0, sizeof(received_len));

        z_result_t result;
        if (mode == 0) {
            _z_transport_message_t message = _z_t_msg_make_keep_alive();
            result = _z_transport_tx_send_t_msg(common, &message, transport->_peers);
        } else {
            uint8_t data[256];
            for (size_t i = 0; i < sizeof(data); i++) {
                data[i] = (uint8_t)i;
            }
            _z_bytes_t payload = _z_bytes_null();
            ASSERT_OK(_z_bytes_from_buf(&payload, data, mode == 2 ? sizeof(data) : 8));
            _z_wireexpr_t key = _z_wireexpr_null();
            key._id = 1;
            _z_network_message_t message;
            _z_n_msg_make_push_put(&message, &key, &payload, NULL, _z_n_qos_make(false, true, Z_PRIORITY_DATA), NULL,
                                   NULL, Z_RELIABILITY_RELIABLE, NULL);
#if Z_FEATURE_BATCHING == 1
            if (mode == 3) {
                common->_batch_state = _Z_BATCHING_ACTIVE;
            }
#endif
            result = _z_send_n_msg(&session, &message, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, NULL);
#if Z_FEATURE_BATCHING == 1
            if (mode == 3) {
                ASSERT_OK(result);
                result = _z_send_n_batch(&session, Z_CONGESTION_CONTROL_BLOCK);
                ASSERT_TRUE(common->_batch_count == 0);
            }
#endif
            _z_bytes_drop(&payload);
        }

        printf("mode=%d failed_peers=%u result=%d\n", mode, failed_peers, result);
        if (failed_peers == 0) {
            ASSERT_OK(result);
            expected_len = received_len[0];
            memcpy(expected, received[0], expected_len);
        } else {
            ASSERT_ERR(result, _Z_ERR_TRANSPORT_TX_FAILED);
        }
        ASSERT_TRUE(common->_transmitted == (failed_peers != 7));
        for (size_t i = 0; i < 3; i++) {
            ASSERT_TRUE(write_calls[i] > 0);
            if ((failed_peers & (1u << i)) == 0) {
                ASSERT_TRUE(received_len[i] == expected_len);
                ASSERT_TRUE(memcmp(received[i], expected, expected_len) == 0);
                if (mode == 2) {
                    ASSERT_TRUE(write_calls[i] > 1);
                }
            }
        }

#if Z_FEATURE_BATCHING == 1
        if (mode == 3) {
            size_t previous_calls[3];
            memcpy(previous_calls, write_calls, sizeof(write_calls));
            ASSERT_OK(_z_send_n_batch(&session, Z_CONGESTION_CONTROL_BLOCK));
            ASSERT_TRUE(memcmp(previous_calls, write_calls, sizeof(write_calls)) == 0);
        }
#endif

        // A failed peer does not prevent subsequent sends through the same transport.
        failed_peers = 0;
        _z_transport_message_t keep_alive = _z_t_msg_make_keep_alive();
        ASSERT_OK(_z_transport_tx_send_t_msg(common, &keep_alive, transport->_peers));
        ASSERT_TRUE(common->_transmitted);
        if (failure_masks[iteration] != 0) {
            failed_peers = 7;
            ASSERT_ERR(_z_transport_tx_send_t_msg(common, &keep_alive, transport->_peers), _Z_ERR_TRANSPORT_TX_FAILED);
            ASSERT_TRUE(common->_transmitted);
        }

        _z_transport_peer_unicast_slist_free(&transport->_peers);
        _z_wbuf_clear(&common->_wbuf);
#if Z_FEATURE_MULTI_THREAD == 1
        _z_mutex_drop(&common->_mutex_tx);
        _z_mutex_rec_drop(&common->_mutex_peer);
#endif
    }
}

int main(int argc, char **argv) {
    int first = argc > 1 ? atoi(argv[1]) : 0;
    int end = argc > 1 ? first + 1 : 4;
    for (int mode = first; mode < end; mode++) {
#if Z_FEATURE_FRAGMENTATION != 1
        if (mode == 2) continue;
#endif
#if Z_FEATURE_BATCHING != 1
        if (mode == 3) continue;
#endif
        test_send_errors(mode);
    }
    return 0;
}
#else
int main(void) { return 0; }
#endif
