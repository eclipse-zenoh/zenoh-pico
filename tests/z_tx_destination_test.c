//
// Copyright (c) 2026 ZettaScale Technology
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/transport/common/tx.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/transport/unicast/transport.h"

#if Z_FEATURE_UNICAST_TRANSPORT == 1

#define Z_TX_CHECK(expr)                                                             \
    do {                                                                             \
        if (!(expr)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr); \
            return false;                                                            \
        }                                                                            \
    } while (0)

typedef struct {
    size_t write_count;
    size_t byte_count;
    size_t max_write;
    bool zero_write;
    bool fail_write;
    uint8_t *capture;
    size_t capture_capacity;
} _z_tx_fake_write_state_t;

typedef struct {
    _z_link_peer_impl_t _base;
    _z_tx_fake_write_state_t *state;
} _z_tx_fake_peer_impl_t;

typedef struct {
    _z_link_t _base;
    _z_tx_fake_write_state_t *state;
} _z_tx_fake_link_t;

typedef struct {
    _z_session_t session;
    _z_tx_fake_link_t link;
    _z_tx_fake_write_state_t default_link;
    _z_tx_fake_write_state_t peer_state[2];
    _z_transport_peer_unicast_t *peer[2];
} _z_tx_fixture_t;

static size_t _z_tx_fake_link_write(const _z_link_t *link, const uint8_t *ptr, size_t len) {
    const _z_tx_fake_link_t *fake_link = (const _z_tx_fake_link_t *)link;
    _z_tx_fake_write_state_t *state = fake_link == NULL ? NULL : fake_link->state;
    len = state->zero_write ? 0 : (state->max_write != 0 && state->max_write < len) ? state->max_write : len;
    state->write_count++;
    if (state->fail_write) {
        return SIZE_MAX;
    }
    if (state->capture != NULL && state->byte_count < state->capture_capacity) {
        size_t to_capture = len;
        if (to_capture > state->capture_capacity - state->byte_count) {
            to_capture = state->capture_capacity - state->byte_count;
        }
        memcpy(&state->capture[state->byte_count], ptr, to_capture);
    }
    state->byte_count += len;
    return len;
}

static size_t _z_tx_fake_peer_write(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr, size_t len) {
    _ZP_UNUSED(link);

    const _z_tx_fake_peer_impl_t *impl = (const _z_tx_fake_peer_impl_t *)_z_link_peer_impl_const(peer);
    _z_tx_fake_write_state_t *state = impl == NULL ? NULL : impl->state;
    len = state->zero_write ? 0 : (state->max_write != 0 && state->max_write < len) ? state->max_write : len;
    state->write_count++;
    if (state->fail_write) {
        return SIZE_MAX;
    }
    if (state->capture != NULL && state->byte_count < state->capture_capacity) {
        size_t to_capture = len;
        if (to_capture > state->capture_capacity - state->byte_count) {
            to_capture = state->capture_capacity - state->byte_count;
        }
        memcpy(&state->capture[state->byte_count], ptr, to_capture);
    }
    state->byte_count += len;
    return len;
}

static const _z_link_peer_ops_t _z_tx_fake_peer_ops = {NULL, _z_tx_fake_peer_write, NULL, NULL, NULL};

static void _z_tx_fake_peer_impl_clear(_z_link_peer_impl_t *base) { _ZP_UNUSED(base); }

static z_result_t _z_tx_fake_peer_init(_z_link_peer_t *peer, _z_tx_fake_write_state_t *state) {
    _z_tx_fake_peer_impl_t *impl = (_z_tx_fake_peer_impl_t *)z_malloc(sizeof(_z_tx_fake_peer_impl_t));
    if (impl == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _z_link_peer_impl_init(&impl->_base, &_z_tx_fake_peer_ops, _z_tx_fake_peer_impl_clear);
    impl->state = state;
    z_result_t ret = _z_link_peer_init(peer, &impl->_base);
    if (ret != _Z_RES_OK) {
        _z_link_peer_impl_clear(&impl->_base);
        z_free(impl);
        return ret;
    }
    return _Z_RES_OK;
}

static void _z_tx_fake_link_init(_z_tx_fake_link_t *link, _z_tx_fake_write_state_t *state) {
    memset(link, 0, sizeof(*link));
    link->state = state;
    link->_base._write_f = _z_tx_fake_link_write;
    link->_base._mtu = Z_BATCH_UNICAST_SIZE;
    link->_base._cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    link->_base._cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    link->_base._cap._is_reliable = 1;
}

static _z_network_message_t _z_tx_test_message(void) {
    _z_network_message_t msg;
    _z_n_msg_make_response_final(&msg, 1);
    return msg;
}

static bool _z_tx_fixture_add_peer(_z_tx_fixture_t *fixture, size_t idx) {
    _z_transport_unicast_t *ztu = &fixture->session._tp._transport._unicast;
    _z_transport_peer_unicast_slist_t *old_head = ztu->_peers;
    _z_transport_peer_unicast_slist_t *new_head = _z_transport_peer_unicast_slist_push_empty(old_head);
    Z_TX_CHECK(new_head != old_head);
    ztu->_peers = new_head;

    _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(new_head);
    memset(peer, 0, sizeof(*peer));
    Z_TX_CHECK(_z_tx_fake_peer_init(&peer->_link_peer, &fixture->peer_state[idx]) == _Z_RES_OK);
    fixture->peer[idx] = peer;
    return true;
}

static bool _z_tx_fixture_init(_z_tx_fixture_t *fixture) {
    memset(fixture, 0, sizeof(*fixture));

    _z_transport_unicast_establish_param_t param;
    memset(&param, 0, sizeof(param));
    param._batch_size = Z_BATCH_UNICAST_SIZE;
    param._seq_num_res = Z_SN_RESOLUTION;
    param._lease = Z_TRANSPORT_LEASE;

    _z_tx_fake_link_init(&fixture->link, &fixture->default_link);
    Z_TX_CHECK(_z_unicast_transport_create(&fixture->session._tp, &fixture->link._base, &param) == _Z_RES_OK);
    fixture->session._mode = Z_WHATAMI_PEER;

    Z_TX_CHECK(_z_tx_fixture_add_peer(fixture, 0));
    Z_TX_CHECK(_z_tx_fixture_add_peer(fixture, 1));
    return true;
}

static void _z_tx_fixture_clear(_z_tx_fixture_t *fixture) {
    fixture->session._tp._transport._unicast._common._link = NULL;
    _z_transport_clear(&fixture->session._tp);
}

static bool directed_send_reaches_only_selected_peer(void) {
    _z_tx_fixture_t fixture;
    Z_TX_CHECK(_z_tx_fixture_init(&fixture));

    _z_network_message_t msg = _z_tx_test_message();
    Z_TX_CHECK(_z_send_n_msg(&fixture.session, &msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK,
                             fixture.peer[0]) == _Z_RES_OK);

    Z_TX_CHECK(fixture.peer_state[0].write_count == 1);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 0);
    Z_TX_CHECK(fixture.default_link.write_count == 0);

    _z_tx_fixture_clear(&fixture);
    return true;
}

static bool directed_send_selected_peer_failure_propagates(void) {
    _z_tx_fixture_t fixture;
    Z_TX_CHECK(_z_tx_fixture_init(&fixture));

    fixture.peer_state[0].fail_write = true;

    _z_network_message_t msg = _z_tx_test_message();
    Z_TX_CHECK(_z_send_n_msg(&fixture.session, &msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK,
                             fixture.peer[0]) == _Z_ERR_TRANSPORT_TX_FAILED);

    Z_TX_CHECK(fixture.peer_state[0].write_count == 1);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 0);
    Z_TX_CHECK(fixture.default_link.write_count == 0);

    _z_tx_fixture_clear(&fixture);
    return true;
}

static bool stream_send_retries_partial_writes(void) {
    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t captured[sizeof(data)] = {0};
    _z_tx_fake_write_state_t state = {0};
    _z_tx_fake_link_t link;
    _z_tx_fake_link_init(&link, &state);
    link._base._cap._flow = Z_LINK_CAP_FLOW_STREAM;
    state.max_write = 3;
    state.capture = captured;
    state.capture_capacity = sizeof(captured);

    _z_wbuf_t wbuf;
    Z_TX_CHECK(_z_wbuf_init(&wbuf, sizeof(data), false) == _Z_RES_OK);
    Z_TX_CHECK(_z_wbuf_write_bytes(&wbuf, data, 0, sizeof(data)) == _Z_RES_OK);

    Z_TX_CHECK(_z_link_send_wbuf(&link._base, &wbuf) == _Z_RES_OK);
    Z_TX_CHECK(state.write_count == 4);
    Z_TX_CHECK(state.byte_count == sizeof(data));
    Z_TX_CHECK(memcmp(captured, data, sizeof(data)) == 0);

    _z_wbuf_clear(&wbuf);
    return true;
}

static bool stream_peer_send_retries_partial_writes(void) {
    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t captured[sizeof(data)] = {0};
    _z_tx_fake_write_state_t link_state = {0};
    _z_tx_fake_link_t link;
    _z_tx_fake_link_init(&link, &link_state);
    link._base._cap._flow = Z_LINK_CAP_FLOW_STREAM;

    _z_tx_fake_write_state_t peer_state = {
        .max_write = 3,
        .capture = captured,
        .capture_capacity = sizeof(captured),
    };
    _z_link_peer_t peer = _z_link_peer_null();
    Z_TX_CHECK(_z_tx_fake_peer_init(&peer, &peer_state) == _Z_RES_OK);

    _z_wbuf_t wbuf;
    bool wbuf_initialized = false;
    bool ok = false;
    if (_z_wbuf_init(&wbuf, sizeof(data), false) != _Z_RES_OK) {
        goto cleanup;
    }
    wbuf_initialized = true;
    if (_z_wbuf_write_bytes(&wbuf, data, 0, sizeof(data)) != _Z_RES_OK) {
        goto cleanup;
    }

    ok = (_z_link_peer_send_wbuf(&link._base, &wbuf, &peer) == _Z_RES_OK) && (peer_state.write_count == 4) &&
         (peer_state.byte_count == sizeof(data)) && (memcmp(captured, data, sizeof(data)) == 0);

cleanup:
    if (wbuf_initialized) {
        _z_wbuf_clear(&wbuf);
    }
    _z_link_peer_clear(&peer);
    return ok;
}

static bool stream_send_zero_progress_fails(void) {
    _z_tx_fake_write_state_t state = {
        .zero_write = true,
    };
    _z_tx_fake_link_t link;
    _z_tx_fake_link_init(&link, &state);
    link._base._cap._flow = Z_LINK_CAP_FLOW_STREAM;

    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    _z_wbuf_t wbuf;
    Z_TX_CHECK(_z_wbuf_init(&wbuf, sizeof(data), false) == _Z_RES_OK);
    Z_TX_CHECK(_z_wbuf_write_bytes(&wbuf, data, 0, sizeof(data)) == _Z_RES_OK);

    Z_TX_CHECK(_z_link_send_wbuf(&link._base, &wbuf) == _Z_ERR_TRANSPORT_TX_FAILED);
    Z_TX_CHECK(state.write_count == 1);
    Z_TX_CHECK(state.byte_count == 0);

    _z_wbuf_clear(&wbuf);
    return true;
}

static bool stream_peer_send_zero_progress_fails(void) {
    _z_tx_fake_write_state_t link_state = {0};
    _z_tx_fake_link_t link;
    _z_tx_fake_link_init(&link, &link_state);
    link._base._cap._flow = Z_LINK_CAP_FLOW_STREAM;

    _z_tx_fake_write_state_t peer_state = {
        .zero_write = true,
    };
    _z_link_peer_t peer = _z_link_peer_null();
    Z_TX_CHECK(_z_tx_fake_peer_init(&peer, &peer_state) == _Z_RES_OK);

    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    _z_wbuf_t wbuf;
    bool wbuf_initialized = false;
    bool ok = false;
    if (_z_wbuf_init(&wbuf, sizeof(data), false) != _Z_RES_OK) {
        goto cleanup;
    }
    wbuf_initialized = true;
    if (_z_wbuf_write_bytes(&wbuf, data, 0, sizeof(data)) != _Z_RES_OK) {
        goto cleanup;
    }

    ok = (_z_link_peer_send_wbuf(&link._base, &wbuf, &peer) == _Z_ERR_TRANSPORT_TX_FAILED) &&
         (peer_state.write_count == 1) && (peer_state.byte_count == 0);

cleanup:
    if (wbuf_initialized) {
        _z_wbuf_clear(&wbuf);
    }
    _z_link_peer_clear(&peer);
    return ok;
}

static bool datagram_peer_send_rejects_partial_writes(void) {
    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t captured[sizeof(data)] = {0};
    _z_tx_fake_write_state_t link_state = {0};
    _z_tx_fake_link_t link;
    _z_tx_fake_link_init(&link, &link_state);

    _z_tx_fake_write_state_t peer_state = {
        .max_write = 3,
        .capture = captured,
        .capture_capacity = sizeof(captured),
    };
    _z_link_peer_t peer = _z_link_peer_null();
    Z_TX_CHECK(_z_tx_fake_peer_init(&peer, &peer_state) == _Z_RES_OK);

    _z_wbuf_t wbuf;
    bool wbuf_initialized = false;
    bool ok = false;
    if (_z_wbuf_init(&wbuf, sizeof(data), false) != _Z_RES_OK) {
        goto cleanup;
    }
    wbuf_initialized = true;
    if (_z_wbuf_write_bytes(&wbuf, data, 0, sizeof(data)) != _Z_RES_OK) {
        goto cleanup;
    }

    ok = (_z_link_peer_send_wbuf(&link._base, &wbuf, &peer) == _Z_ERR_TRANSPORT_TX_FAILED) &&
         (peer_state.write_count == 1) && (peer_state.byte_count == 3) &&
         (memcmp(captured, data, peer_state.byte_count) == 0);

cleanup:
    if (wbuf_initialized) {
        _z_wbuf_clear(&wbuf);
    }
    _z_link_peer_clear(&peer);
    return ok;
}

#if Z_FEATURE_BATCHING == 1
static bool directed_send_flushes_shared_batch_once(void) {
    _z_tx_fixture_t fixture;
    Z_TX_CHECK(_z_tx_fixture_init(&fixture));

    Z_TX_CHECK(_z_transport_start_batching(&fixture.session._tp) == _Z_RES_OK);

    _z_network_message_t broadcast_msg = _z_tx_test_message();
    Z_TX_CHECK(_z_send_n_msg(&fixture.session, &broadcast_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK,
                             NULL) == _Z_RES_OK);
    Z_TX_CHECK(fixture.peer_state[0].write_count == 0);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 0);

    _z_network_message_t directed_msg = _z_tx_test_message();
    Z_TX_CHECK(_z_send_n_msg(&fixture.session, &directed_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK,
                             fixture.peer[0]) == _Z_RES_OK);
    Z_TX_CHECK(fixture.peer_state[0].write_count == 2);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 1);

    Z_TX_CHECK(_z_send_n_batch(&fixture.session, Z_CONGESTION_CONTROL_BLOCK) == _Z_RES_OK);
    Z_TX_CHECK(fixture.peer_state[0].write_count == 2);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 1);
    Z_TX_CHECK(fixture.default_link.write_count == 0);

    Z_TX_CHECK(_z_transport_stop_batching(&fixture.session._tp) == _Z_RES_OK);
    _z_tx_fixture_clear(&fixture);
    return true;
}

static bool directed_send_ignores_peer_list_flush_failure(void) {
    _z_tx_fixture_t fixture;
    Z_TX_CHECK(_z_tx_fixture_init(&fixture));

    Z_TX_CHECK(_z_transport_start_batching(&fixture.session._tp) == _Z_RES_OK);

    _z_network_message_t broadcast_msg = _z_tx_test_message();
    Z_TX_CHECK(_z_send_n_msg(&fixture.session, &broadcast_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK,
                             NULL) == _Z_RES_OK);
    Z_TX_CHECK(fixture.peer_state[0].write_count == 0);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 0);

    fixture.peer_state[1].fail_write = true;

    _z_network_message_t directed_msg = _z_tx_test_message();
    Z_TX_CHECK(_z_send_n_msg(&fixture.session, &directed_msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK,
                             fixture.peer[0]) == _Z_RES_OK);
    Z_TX_CHECK(fixture.peer_state[0].write_count == 2);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 1);

    Z_TX_CHECK(_z_send_n_batch(&fixture.session, Z_CONGESTION_CONTROL_BLOCK) == _Z_RES_OK);
    Z_TX_CHECK(fixture.peer_state[0].write_count == 2);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 1);
    Z_TX_CHECK(fixture.default_link.write_count == 0);

    Z_TX_CHECK(_z_transport_stop_batching(&fixture.session._tp) == _Z_RES_OK);
    _z_tx_fixture_clear(&fixture);
    return true;
}

static bool broadcast_batching_reaches_all_peers(void) {
    _z_tx_fixture_t fixture;
    Z_TX_CHECK(_z_tx_fixture_init(&fixture));

    Z_TX_CHECK(_z_transport_start_batching(&fixture.session._tp) == _Z_RES_OK);

    _z_network_message_t msg = _z_tx_test_message();
    Z_TX_CHECK(_z_send_n_msg(&fixture.session, &msg, Z_RELIABILITY_RELIABLE, Z_CONGESTION_CONTROL_BLOCK, NULL) ==
               _Z_RES_OK);
    Z_TX_CHECK(fixture.peer_state[0].write_count == 0);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 0);

    Z_TX_CHECK(_z_send_n_batch(&fixture.session, Z_CONGESTION_CONTROL_BLOCK) == _Z_RES_OK);
    Z_TX_CHECK(fixture.peer_state[0].write_count == 1);
    Z_TX_CHECK(fixture.peer_state[1].write_count == 1);
    Z_TX_CHECK(fixture.default_link.write_count == 0);

    Z_TX_CHECK(_z_transport_stop_batching(&fixture.session._tp) == _Z_RES_OK);
    _z_tx_fixture_clear(&fixture);
    return true;
}
#endif  // Z_FEATURE_BATCHING == 1

int main(void) {
    if (!directed_send_reaches_only_selected_peer() || !directed_send_selected_peer_failure_propagates() ||
        !stream_send_retries_partial_writes() || !stream_peer_send_retries_partial_writes() ||
        !stream_send_zero_progress_fails() || !stream_peer_send_zero_progress_fails() ||
        !datagram_peer_send_rejects_partial_writes()) {
        return 1;
    }

#if Z_FEATURE_BATCHING == 1
    if (!directed_send_flushes_shared_batch_once() || !directed_send_ignores_peer_list_flush_failure() ||
        !broadcast_batching_reaches_all_peers()) {
        return 1;
    }
#else
    printf("Skipping batching-specific TX destination tests (Z_FEATURE_BATCHING disabled)\n");
#endif

    return 0;
}

#else
int main(void) {
    printf("Skipping TX destination tests (Z_FEATURE_UNICAST_TRANSPORT disabled)\n");
    return 0;
}
#endif  // Z_FEATURE_UNICAST_TRANSPORT == 1
