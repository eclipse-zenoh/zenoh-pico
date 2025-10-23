//
// Copyright (c) 2025 ZettaScale Technology
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

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/net/reply.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/loopback.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/result.h"

#if (Z_FEATURE_SUBSCRIPTION == 1) && (Z_FEATURE_QUERYABLE == 1) && (Z_FEATURE_QUERY == 1)

static atomic_uint g_send_calls = 0;
static atomic_uint g_final_send_calls = 0;
static atomic_uint g_local_puts = 0;
static atomic_uint g_local_queries = 0;
static atomic_uint g_query_drops = 0;
static atomic_uint g_query_replies = 0;

static _z_session_t g_session;
static _z_transport_common_t g_fake_transport = {0};
static bool g_transport_ready = false;

static _z_transport_common_t *loopback_override(_z_session_t *zn) {
    if (g_transport_ready && zn == &g_session) {
        return &g_fake_transport;
    }
    return NULL;
}

static z_result_t z_send_override(_z_session_t *zn, const _z_network_message_t *n_msg, z_reliability_t reliability,
                                  z_congestion_control_t cong_ctrl, void *peer, bool *handled) {
    _ZP_UNUSED(zn);
    _ZP_UNUSED(reliability);
    _ZP_UNUSED(cong_ctrl);
    _ZP_UNUSED(peer);
    if (n_msg != NULL) {
        atomic_fetch_add_explicit(&g_send_calls, 1, memory_order_relaxed);
        if (n_msg->_tag == _Z_N_RESPONSE_FINAL) {
            atomic_fetch_add_explicit(&g_final_send_calls, 1, memory_order_relaxed);
        }
    }
    if (handled != NULL) {
        *handled = true;
    }
    return _Z_RES_OK;
}

static void local_sample_callback(_z_sample_t *sample, void *arg) {
    _ZP_UNUSED(sample);
    atomic_fetch_add_explicit((atomic_uint *)arg, 1, memory_order_relaxed);
}

static void local_query_callback(_z_query_rc_t *query_rc, void *arg) {
    atomic_fetch_add_explicit((atomic_uint *)arg, 1, memory_order_relaxed);

    const char data[] = "loopback-response";
    _z_bytes_t payload = _z_bytes_null();
    assert(_z_bytes_from_buf(&payload, (const uint8_t *)data, sizeof(data) - 1) == _Z_RES_OK);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t timestamp = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();
    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);

    _z_network_message_t msg;
    _z_n_msg_make_reply_ok_put(&msg, &g_session._local_zid, _Z_RC_IN_VAL(query_rc)->_request_id,
                               &_Z_RC_IN_VAL(query_rc)->_key, Z_RELIABILITY_DEFAULT, Z_CONSOLIDATION_MODE_DEFAULT, qos,
                               &timestamp, &source_info, &payload, &encoding, NULL);
    assert(_z_handle_network_message(&g_fake_transport, &msg, NULL) == _Z_RES_OK);
    _z_n_msg_clear(&msg);
}

static void query_reply_callback(_z_reply_t *reply, void *arg) {
    _ZP_UNUSED(reply);
    _ZP_UNUSED(arg);
    atomic_fetch_add_explicit(&g_query_replies, 1, memory_order_relaxed);
}

static void query_dropper(void *arg) {
    _ZP_UNUSED(arg);
    atomic_fetch_add_explicit(&g_query_drops, 1, memory_order_relaxed);
}

static void setup_session(void) {
    _z_id_t zid;
    _z_session_generate_zid(&zid, Z_ZID_LENGTH);
    assert(_z_session_init(&g_session, &zid) == _Z_RES_OK);

    g_fake_transport = (_z_transport_common_t){0};
    g_fake_transport._session._val = &g_session;
    g_fake_transport._session._cnt = NULL;
    g_transport_ready = true;
    _z_session_set_transport_common_override(loopback_override);
    _z_transport_set_send_n_msg_override(z_send_override);
}

static void teardown_session(void) {
    if (g_transport_ready) {
        g_transport_ready = false;
    }
    _z_session_clear(&g_session);
    _z_session_set_transport_common_override(NULL);
    _z_transport_set_send_n_msg_override(NULL);
}

static void test_local_put_delivery(void) {
    setup_session();

    const char *key_str = "zenoh-pico/tests/local/put";
    _z_string_t suffix = _z_string_copy_from_str(key_str);
    _z_keyexpr_t keyexpr;
    _z_keyexpr_from_string(&keyexpr, Z_RESOURCE_ID_NONE, &suffix);
    _z_string_clear(&suffix);

    uint16_t rid = _z_register_resource(&g_session, &keyexpr, Z_RESOURCE_ID_NONE, NULL);
    assert(rid != Z_RESOURCE_ID_NONE);

    _z_keyexpr_t expanded = _z_get_expanded_key_from_key(&g_session, &keyexpr, NULL);

    _z_subscription_t sub_entry = {0};
    sub_entry._id = _z_get_entity_id(&g_session);
    sub_entry._key_id = rid;
    sub_entry._declared_key = _z_keyexpr_duplicate(&expanded);
    sub_entry._key = _z_keyexpr_duplicate(&expanded);
    sub_entry._callback = local_sample_callback;
    sub_entry._dropper = NULL;
    sub_entry._arg = &g_local_puts;

    _z_subscription_rc_t *subscription_rc =
        _z_register_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, &sub_entry);
    assert(subscription_rc != NULL);

    atomic_store_explicit(&g_send_calls, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_puts, 0, memory_order_relaxed);

    const char payload_data[] = "payload";
    _z_bytes_t payload;
    assert(_z_bytes_from_buf(&payload, (const uint8_t *)payload_data, sizeof(payload_data) - 1) == _Z_RES_OK);

    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t ts = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();
    bool delivered = _z_session_deliver_push_locally(&g_session, &keyexpr, &payload, &encoding, Z_SAMPLE_KIND_PUT, qos,
                                                     &ts, NULL, Z_RELIABILITY_RELIABLE, &source_info);
    assert(delivered);
    assert(atomic_load_explicit(&g_local_puts, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_send_calls, memory_order_relaxed) == 0);

    _z_bytes_drop(&payload);
    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, subscription_rc);
    _z_unregister_resource(&g_session, rid, NULL);
    _z_keyexpr_clear(&expanded);
    _z_keyexpr_clear(&keyexpr);

    teardown_session();
}

static void test_local_query_delivery(void) {
    setup_session();

    const char *key_str = "zenoh-pico/tests/local/query";
    _z_string_t suffix = _z_string_copy_from_str(key_str);
    _z_keyexpr_t keyexpr;
    _z_keyexpr_from_string(&keyexpr, Z_RESOURCE_ID_NONE, &suffix);
    _z_string_clear(&suffix);

    uint16_t rid = _z_register_resource(&g_session, &keyexpr, Z_RESOURCE_ID_NONE, NULL);
    assert(rid != Z_RESOURCE_ID_NONE);

    _z_keyexpr_t expanded = _z_get_expanded_key_from_key(&g_session, &keyexpr, NULL);

    _z_session_queryable_t queryable_entry = {0};
    queryable_entry._id = _z_get_entity_id(&g_session);
    queryable_entry._key = _z_keyexpr_duplicate(&expanded);
    queryable_entry._declared_key = _z_keyexpr_duplicate(&expanded);
    queryable_entry._callback = local_query_callback;
    queryable_entry._dropper = NULL;
    queryable_entry._arg = &g_local_queries;
    queryable_entry._complete = false;

    _z_session_queryable_rc_t *queryable_rc = _z_register_session_queryable(&g_session, &queryable_entry);
    assert(queryable_rc != NULL);

    atomic_store_explicit(&g_send_calls, 0, memory_order_relaxed);
    atomic_store_explicit(&g_final_send_calls, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_queries, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_drops, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_replies, 0, memory_order_relaxed);

    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    z_result_t res =
        _z_query(&g_session, &keyexpr, NULL, 0, Z_QUERY_TARGET_DEFAULT, Z_CONSOLIDATION_MODE_DEFAULT, NULL, NULL,
                 query_reply_callback, query_dropper, NULL, 1000, NULL, qos, Z_CONGESTION_CONTROL_BLOCK);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_queries, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_replies, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_drops, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_send_calls, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_final_send_calls, memory_order_relaxed) == 0);
    assert(g_session._pending_queries == NULL);

    _z_unregister_session_queryable(&g_session, queryable_rc);
    _z_unregister_resource(&g_session, rid, NULL);
    _z_keyexpr_clear(&expanded);
    _z_keyexpr_clear(&keyexpr);

    teardown_session();
}

int main(void) {
    test_local_put_delivery();
    test_local_query_delivery();
    return 0;
}

#else
int main(void) { return 0; }
#endif
