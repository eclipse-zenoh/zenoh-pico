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

#undef NDEBUG
#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/api/macros.h"
#include "zenoh-pico/api/primitives.h"
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
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/result.h"

#if (Z_FEATURE_SUBSCRIPTION == 1) && (Z_FEATURE_QUERYABLE == 1) && (Z_FEATURE_QUERY == 1) && \
    (Z_FEATURE_LOCAL_SUBSCRIBER == 1) && (Z_FEATURE_LOCAL_QUERYABLE == 1)

static atomic_uint g_network_send_count = 0;
static atomic_uint g_network_final_send_count = 0;
static atomic_uint g_local_put_delivery_count = 0;
static atomic_uint g_local_query_delivery_count = 0;
static atomic_uint g_query_drop_callback_count = 0;
static atomic_uint g_query_reply_callback_count = 0;

static _z_session_t g_session;
static _z_session_rc_t g_session_rc = {0};
static _z_transport_common_t g_fake_transport = {0};
static bool g_transport_ready = false;
static _z_link_t g_dummy_link = {0};

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
        atomic_fetch_add_explicit(&g_network_send_count, 1, memory_order_relaxed);
        if (n_msg->_tag == _Z_N_RESPONSE_FINAL) {
            atomic_fetch_add_explicit(&g_network_final_send_count, 1, memory_order_relaxed);
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
    _z_keyexpr_t key_copy = _z_keyexpr_duplicate(&_Z_RC_IN_VAL(query_rc)->_key);
    _z_n_msg_make_reply_ok_put(&msg, &g_session._local_zid, _Z_RC_IN_VAL(query_rc)->_request_id, &key_copy,
                               Z_RELIABILITY_DEFAULT, Z_CONSOLIDATION_MODE_DEFAULT, qos, &timestamp, &source_info,
                               &payload, &encoding, NULL);
    assert(_z_handle_network_message(&g_fake_transport, &msg, NULL) == _Z_RES_OK);
}

static void query_reply_callback(_z_reply_t *reply, void *arg) {
    _ZP_UNUSED(reply);
    _ZP_UNUSED(arg);
    atomic_fetch_add_explicit(&g_query_reply_callback_count, 1, memory_order_relaxed);
}

static void query_dropper(void *arg) {
    _ZP_UNUSED(arg);
    atomic_fetch_add_explicit(&g_query_drop_callback_count, 1, memory_order_relaxed);
}

static void add_fake_peer(void) {
    // Add a fake peer to simulate a remote connection
    g_session._tp._transport._unicast._peers =
        _z_transport_peer_unicast_slist_push_empty(g_session._tp._transport._unicast._peers);
    _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(g_session._tp._transport._unicast._peers);
    *peer = (_z_transport_peer_unicast_t){0};
    g_session._tp._transport._unicast._common._link = &g_dummy_link;
}

static void setup_session(void) {
    _z_id_t zid;
    _z_session_generate_zid(&zid, Z_ZID_LENGTH);
    assert(_z_session_init(&g_session, &zid) == _Z_RES_OK);
    if (_Z_RC_IS_NULL(&g_session_rc)) {
        g_session_rc = _z_session_rc_new(&g_session);
        assert(!_Z_RC_IS_NULL(&g_session_rc));
    } else {
        g_session_rc._val = &g_session;
    }

    g_dummy_link = (_z_link_t){0};
    g_fake_transport = (_z_transport_common_t){0};
    g_fake_transport._session = _z_session_rc_clone_as_weak(&g_session_rc);
    g_fake_transport._link = &g_dummy_link;
    g_transport_ready = true;
    _z_session_set_transport_common_override(loopback_override);
    _z_transport_set_send_n_msg_override(z_send_override);

    g_session._tp._type = _Z_TRANSPORT_UNICAST_TYPE;
}

static void cleanup_session(void) {
    if (g_transport_ready) {
        g_transport_ready = false;
    }
    _z_transport_peer_unicast_slist_free(&g_session._tp._transport._unicast._peers);
    g_session._tp._transport._unicast._peers = NULL;
    g_session._tp._transport._unicast._common._link = NULL;
    g_session._tp._type = _Z_TRANSPORT_NONE;
    _z_session_clear(&g_session);
    _z_session_set_transport_common_override(NULL);
    _z_transport_set_send_n_msg_override(NULL);
}

static void create_local_resource(const char *key_str, _z_keyexpr_t *keyexpr, _z_keyexpr_t *expanded, uint16_t *rid) {
    keyexpr->_id = Z_RESOURCE_ID_NONE;
    keyexpr->_mapping = _Z_KEYEXPR_MAPPING_LOCAL;
    keyexpr->_suffix = _z_string_copy_from_str(key_str);

    *rid = _z_register_resource(&g_session, keyexpr, Z_RESOURCE_ID_NONE, NULL);
    assert(*rid != Z_RESOURCE_ID_NONE);

    *expanded = _z_get_expanded_key_from_key(&g_session, keyexpr, NULL);
}

static void cleanup_local_resource(_z_keyexpr_t *keyexpr, _z_keyexpr_t *expanded, uint16_t rid) {
    _z_unregister_resource(&g_session, rid, NULL);
    _z_keyexpr_clear(expanded);
    _z_keyexpr_clear(keyexpr);
}

static _z_subscription_rc_t *register_local_subscription(const _z_keyexpr_t *expanded, uint16_t rid,
                                                         atomic_uint *counter, z_locality_t allowed_origin) {
    _z_subscription_t sub_entry = {0};
    sub_entry._id = _z_get_entity_id(&g_session);
    sub_entry._key_id = rid;
    sub_entry._declared_key = _z_keyexpr_duplicate(expanded);
    sub_entry._key = _z_keyexpr_duplicate(expanded);
    sub_entry._callback = local_sample_callback;
    sub_entry._dropper = NULL;
    sub_entry._arg = counter;
    sub_entry._allowed_origin = allowed_origin;

    _z_subscription_rc_t *subscription_rc =
        _z_register_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, &sub_entry);
    assert(subscription_rc != NULL);
    return subscription_rc;
}

static _z_session_queryable_rc_t *register_local_queryable(const _z_keyexpr_t *expanded, atomic_uint *counter,
                                                           z_locality_t allowed_origin) {
    _z_session_queryable_t queryable_entry = {0};
    queryable_entry._id = _z_get_entity_id(&g_session);
    queryable_entry._key = _z_keyexpr_duplicate(expanded);
    queryable_entry._declared_key = _z_keyexpr_duplicate(expanded);
    queryable_entry._callback = local_query_callback;
    queryable_entry._dropper = NULL;
    queryable_entry._arg = counter;
    queryable_entry._complete = false;
    queryable_entry._allowed_origin = allowed_origin;

    _z_session_queryable_rc_t *queryable_rc = _z_register_session_queryable(&g_session, &queryable_entry);
    assert(queryable_rc != NULL);
    return queryable_rc;
}

static void test_put_local_only_single(void) {
    setup_session();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/put", &keyexpr, &expanded, &rid);
    _z_subscription_rc_t *subscription_rc =
        register_local_subscription(&expanded, rid, &g_local_put_delivery_count, Z_LOCALITY_SESSION_LOCAL);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);

    const char payload_data[] = "payload";
    _z_bytes_t payload;
    assert(_z_bytes_from_buf(&payload, (const uint8_t *)payload_data, sizeof(payload_data) - 1) == _Z_RES_OK);

    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t ts = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();
    z_result_t delivered = _z_session_deliver_push_locally(&g_session, &keyexpr, &payload, &encoding, Z_SAMPLE_KIND_PUT,
                                                           qos, &ts, NULL, Z_RELIABILITY_RELIABLE, &source_info);
    assert(delivered == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 0);

    _z_bytes_drop(&payload);
    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, subscription_rc);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_put_local_only_via_api(void) {
    setup_session();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/put/api", &keyexpr, &expanded, &rid);
    _z_subscription_rc_t *subscription_rc =
        register_local_subscription(&expanded, rid, &g_local_put_delivery_count, Z_LOCALITY_SESSION_LOCAL);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);

    const char payload_data[] = "payload";
    z_owned_bytes_t payload = {0};
    assert(z_bytes_from_buf(&payload, (uint8_t *)payload_data, sizeof(payload_data) - 1, NULL, NULL) == Z_OK);

    z_put_options_t opt;
    z_put_options_default(&opt);
    opt.allowed_destination = Z_LOCALITY_SESSION_LOCAL;

    z_result_t res = z_put(&g_session_rc, (const z_loaned_keyexpr_t *)&keyexpr, z_move(payload), &opt);
    assert(res == Z_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 0);

    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    z_owned_bytes_t payload2 = {0};
    assert(z_bytes_from_buf(&payload2, (uint8_t *)payload_data, sizeof(payload_data) - 1, NULL, NULL) == Z_OK);
    z_put_options_default(&opt);
    opt.allowed_destination = Z_LOCALITY_ANY;
    res = z_put(&g_session_rc, (const z_loaned_keyexpr_t *)&keyexpr, z_move(payload2), &opt);
    assert(res == Z_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 1);

    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, subscription_rc);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_put_local_and_remote_via_api(void) {
    setup_session();
    add_fake_peer();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/put/api-mixed", &keyexpr, &expanded, &rid);
    _z_subscription_rc_t *subscription_rc =
        register_local_subscription(&expanded, rid, &g_local_put_delivery_count, Z_LOCALITY_ANY);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);

    const char payload_data[] = "payload";
    z_owned_bytes_t payload = {0};
    assert(z_bytes_from_buf(&payload, (uint8_t *)payload_data, sizeof(payload_data) - 1, NULL, NULL) == Z_OK);

    z_put_options_t opt;
    z_put_options_default(&opt);
    opt.allowed_destination = Z_LOCALITY_SESSION_LOCAL;
    z_result_t res = z_put(&g_session_rc, (const z_loaned_keyexpr_t *)&keyexpr, z_move(payload), &opt);
    assert(res == Z_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 0);

    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    z_owned_bytes_t payload2 = {0};
    assert(z_bytes_from_buf(&payload2, (uint8_t *)payload_data, sizeof(payload_data) - 1, NULL, NULL) == Z_OK);
    opt.allowed_destination = Z_LOCALITY_ANY;
    res = z_put(&g_session_rc, (const z_loaned_keyexpr_t *)&keyexpr, z_move(payload2), &opt);
    assert(res == Z_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 1);

    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, subscription_rc);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_query_local_only_single(void) {
    setup_session();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/query", &keyexpr, &expanded, &rid);

    _z_session_queryable_rc_t *queryable_rc =
        register_local_queryable(&expanded, &g_local_query_delivery_count, Z_LOCALITY_SESSION_LOCAL);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_final_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_query_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_drop_callback_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_reply_callback_count, 0, memory_order_relaxed);

    // With Z_CONSOLIDATION_MODE_DEFAULT should be set to LATEST, when no time is specified
    // Explicitly set LATEST consolidation mode for clarity
    assert(g_session._tp._transport._unicast._common._link == NULL);
    assert(_z_transport_peer_unicast_slist_is_empty(g_session._tp._transport._unicast._peers));
    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    _z_zint_t query_id = 0;
    z_result_t res =
        _z_query(&g_session, &keyexpr, NULL, 0, Z_QUERY_TARGET_DEFAULT, Z_CONSOLIDATION_MODE_LATEST, NULL, NULL,
                 query_reply_callback, query_dropper, NULL, 1000, NULL, qos, NULL, Z_LOCALITY_SESSION_LOCAL, &query_id);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_query_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_reply_callback_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_drop_callback_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_network_final_send_count, memory_order_relaxed) == 0);
    assert(g_session._pending_queries == NULL);

    _z_unregister_session_queryable(&g_session, queryable_rc);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_put_local_only_multiple(void) {
    setup_session();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/put/multi", &keyexpr, &expanded, &rid);

    _z_subscription_rc_t *sub_primary =
        register_local_subscription(&expanded, rid, &g_local_put_delivery_count, Z_LOCALITY_SESSION_LOCAL);
    atomic_uint local_put_secondary_count = 0;
    _z_subscription_rc_t *sub_secondary =
        register_local_subscription(&expanded, rid, &local_put_secondary_count, Z_LOCALITY_SESSION_LOCAL);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&local_put_secondary_count, 0, memory_order_relaxed);

    const char payload_data[] = "payload";
    _z_bytes_t payload;
    assert(_z_bytes_from_buf(&payload, (const uint8_t *)payload_data, sizeof(payload_data) - 1) == _Z_RES_OK);

    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t ts = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();
    z_result_t delivered = _z_session_deliver_push_locally(&g_session, &keyexpr, &payload, &encoding, Z_SAMPLE_KIND_PUT,
                                                           qos, &ts, NULL, Z_RELIABILITY_RELIABLE, &source_info);
    assert(delivered == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&local_put_secondary_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 0);

    _z_bytes_drop(&payload);
    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, sub_secondary);
    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, sub_primary);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_put_local_and_remote(void) {
    setup_session();
    add_fake_peer();
    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/put/mixed", &keyexpr, &expanded, &rid);

    _z_subscription_rc_t *sub_primary =
        register_local_subscription(&expanded, rid, &g_local_put_delivery_count, Z_LOCALITY_ANY);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);

    const char payload_data[] = "payload";
    _z_bytes_t payload;
    assert(_z_bytes_from_buf(&payload, (const uint8_t *)payload_data, sizeof(payload_data) - 1) == _Z_RES_OK);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t ts = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();

    // Session-local only delivery should not touch transport
    z_result_t res =
        _z_write(&g_session, &keyexpr, &payload, &encoding, Z_SAMPLE_KIND_PUT, Z_CONGESTION_CONTROL_BLOCK,
                 Z_PRIORITY_DEFAULT, false, &ts, NULL, Z_RELIABILITY_RELIABLE, &source_info, Z_LOCALITY_SESSION_LOCAL);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 0);

    // Allow remote recipients; send to transport in addition to local delivery
    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    res = _z_write(&g_session, &keyexpr, &payload, &encoding, Z_SAMPLE_KIND_PUT, Z_CONGESTION_CONTROL_BLOCK,
                   Z_PRIORITY_DEFAULT, false, &ts, NULL, Z_RELIABILITY_RELIABLE, &source_info, Z_LOCALITY_ANY);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 1);

    _z_bytes_drop(&payload);
    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, sub_primary);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_query_local_only_multiple(void) {
    setup_session();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/query/multi", &keyexpr, &expanded, &rid);

    _z_session_queryable_rc_t *queryable_primary =
        register_local_queryable(&expanded, &g_local_query_delivery_count, Z_LOCALITY_SESSION_LOCAL);
    atomic_uint local_query_secondary_count = 0;
    _z_session_queryable_rc_t *queryable_secondary =
        register_local_queryable(&expanded, &local_query_secondary_count, Z_LOCALITY_SESSION_LOCAL);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_final_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_query_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&local_query_secondary_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_drop_callback_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_reply_callback_count, 0, memory_order_relaxed);

    // Explicitly set LATEST consolidation mode for clarity (should be default)
    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    _z_zint_t query_id = 0;
    z_result_t res =
        _z_query(&g_session, &keyexpr, NULL, 0, Z_QUERY_TARGET_DEFAULT, Z_CONSOLIDATION_MODE_LATEST, NULL, NULL,
                 query_reply_callback, query_dropper, NULL, 1000, NULL, qos, NULL, Z_LOCALITY_SESSION_LOCAL, &query_id);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_query_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&local_query_secondary_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_reply_callback_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_drop_callback_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_network_final_send_count, memory_order_relaxed) == 0);
    assert(g_session._pending_queries == NULL);

    _z_unregister_session_queryable(&g_session, queryable_secondary);
    _z_unregister_session_queryable(&g_session, queryable_primary);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_query_local_and_remote(void) {
    setup_session();
    add_fake_peer();
    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/query/mixed", &keyexpr, &expanded, &rid);
    _z_session_queryable_rc_t *queryable_primary =
        register_local_queryable(&expanded, &g_local_query_delivery_count, Z_LOCALITY_SESSION_LOCAL);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_final_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_query_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_drop_callback_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_reply_callback_count, 0, memory_order_relaxed);

    // Explicitly set LATEST consolidation mode for clarity (should be default)
    // Locality limited to session, loopback-only, transport untouched
    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    _z_zint_t query_id = 0;
    z_result_t res =
        _z_query(&g_session, &keyexpr, NULL, 0, Z_QUERY_TARGET_DEFAULT, Z_CONSOLIDATION_MODE_LATEST, NULL, NULL,
                 query_reply_callback, query_dropper, NULL, 1000, NULL, qos, NULL, Z_LOCALITY_SESSION_LOCAL, &query_id);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_query_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_reply_callback_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_drop_callback_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_network_final_send_count, memory_order_relaxed) == 0);
    assert(g_session._pending_queries == NULL);

    atomic_store_explicit(&g_local_query_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_reply_callback_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_drop_callback_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_final_send_count, 0, memory_order_relaxed);
    // Permit remote delivery; still send to loopback, but network request must be emitted as well
    res = _z_query(&g_session, &keyexpr, NULL, 0, Z_QUERY_TARGET_DEFAULT, Z_CONSOLIDATION_MODE_LATEST, NULL, NULL,
                   query_reply_callback, query_dropper, NULL, 1000, NULL, qos, NULL, Z_LOCALITY_ANY, &query_id);
    assert(res == _Z_RES_OK);

    assert(atomic_load_explicit(&g_local_query_delivery_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_reply_callback_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_query_drop_callback_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_network_final_send_count, memory_order_relaxed) == 0);
    assert(g_session._pending_queries != NULL);

    // Simulate REPLY from remote queryable
    _z_pending_query_t *pq = _z_pending_query_slist_value(g_session._pending_queries);
    _z_zint_t request_id = pq->_id;

    const char remote_data[] = "remote-response";
    _z_bytes_t remote_payload = _z_bytes_null();
    assert(_z_bytes_from_buf(&remote_payload, (const uint8_t *)remote_data, sizeof(remote_data) - 1) == _Z_RES_OK);

    _z_id_t remote_zid = _z_id_empty();
    _z_session_generate_zid(&remote_zid, Z_ZID_LENGTH);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t timestamp = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();

    _z_network_message_t reply_msg;
    _z_keyexpr_t ke_copy = _z_keyexpr_duplicate(&keyexpr);
    _z_n_msg_make_reply_ok_put(&reply_msg, &remote_zid, request_id, &ke_copy, Z_RELIABILITY_RELIABLE,
                               Z_CONSOLIDATION_MODE_DEFAULT, qos, &timestamp, &source_info, &remote_payload, &encoding,
                               NULL);
    res = _z_handle_network_message(&g_fake_transport, &reply_msg, NULL);
    assert(res == _Z_RES_OK);

    // Remote reply buffered (LATEST mode), not delivered yet,
    // will be delivered on RESPONSE_FINAL
    assert(atomic_load_explicit(&g_query_reply_callback_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_query_drop_callback_count, memory_order_relaxed) == 0);
    assert(g_session._pending_queries != NULL);

    // Receiving RESPONSE_FINAL from remote queryable
    _z_network_message_t final_msg;
    _z_n_msg_make_response_final(&final_msg, request_id);
    res = _z_handle_network_message(&g_fake_transport, &final_msg, NULL);
    assert(res == _Z_RES_OK);

    // Remote reply delivered, query finalized
    assert(atomic_load_explicit(&g_query_reply_callback_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_drop_callback_count, memory_order_relaxed) == 1);
    assert(g_session._pending_queries == NULL);

    _z_unregister_session_queryable(&g_session, queryable_primary);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_query_local_and_remote_via_api(void) {
    setup_session();
    add_fake_peer();

    const char *kestr = "zenoh-pico/tests/local/query/api-mixed";
    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource(kestr, &keyexpr, &expanded, &rid);

    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_final_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_local_query_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_drop_callback_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_reply_callback_count, 0, memory_order_relaxed);

    z_owned_closure_query_t q_closure = {0};
    assert(z_closure_query(&q_closure, (z_closure_query_callback_t)local_query_callback, NULL,
                           &g_local_query_delivery_count) == Z_OK);

    z_owned_queryable_t queryable = {0};
    z_queryable_options_t qopt;
    z_queryable_options_default(&qopt);
    qopt.allowed_origin = Z_LOCALITY_ANY;

    z_owned_keyexpr_t keyexpr_owned = {0};
    assert(z_keyexpr_from_str(&keyexpr_owned, kestr) == Z_OK);

    z_result_t res = z_declare_queryable((const z_loaned_session_t *)&g_session_rc, &queryable,
                                         (const z_loaned_keyexpr_t *)&keyexpr_owned, z_move(q_closure), &qopt);
    assert(res == Z_OK);

    z_owned_closure_reply_t r_closure = {0};
    assert(z_closure_reply(&r_closure, query_reply_callback, query_dropper, NULL) == Z_OK);

    z_get_options_t gopt;
    z_get_options_default(&gopt);
    gopt.allowed_destination = Z_LOCALITY_ANY;

    res = z_get((const z_loaned_session_t *)&g_session_rc, (const z_loaned_keyexpr_t *)&keyexpr_owned, "",
                z_move(r_closure), &gopt);
    assert(res == Z_OK);

    _z_pending_query_t *pq = _z_pending_query_slist_value(g_session._pending_queries);
    assert(pq != NULL);
    _z_zint_t request_id = pq->_id;

    const char remote_data[] = "remote-response";
    _z_bytes_t remote_payload = _z_bytes_null();
    assert(_z_bytes_from_buf(&remote_payload, (const uint8_t *)remote_data, sizeof(remote_data) - 1) == _Z_RES_OK);

    _z_id_t remote_zid = _z_id_empty();
    _z_session_generate_zid(&remote_zid, Z_ZID_LENGTH);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t timestamp = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();

    _z_network_message_t reply_msg;
    _z_keyexpr_t ke_copy = _z_keyexpr_duplicate(&keyexpr);
    _z_n_msg_make_reply_ok_put(&reply_msg, &remote_zid, request_id, &ke_copy, Z_RELIABILITY_RELIABLE,
                               Z_CONSOLIDATION_MODE_DEFAULT, _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT),
                               &timestamp, &source_info, &remote_payload, &encoding, NULL);
    res = _z_handle_network_message(&g_fake_transport, &reply_msg, NULL);
    assert(res == _Z_RES_OK);

    _z_network_message_t final_msg;
    _z_n_msg_make_response_final(&final_msg, request_id);
    res = _z_handle_network_message(&g_fake_transport, &final_msg, NULL);
    assert(res == _Z_RES_OK);

    assert(atomic_load_explicit(&g_query_reply_callback_count, memory_order_relaxed) == 1);
    assert(atomic_load_explicit(&g_query_drop_callback_count, memory_order_relaxed) == 1);
    assert(g_session._pending_queries == NULL);

    z_moved_queryable_t *mq = z_queryable_move(&queryable);
    z_queryable_drop(mq);
    z_moved_keyexpr_t *mk = z_keyexpr_move(&keyexpr_owned);
    z_keyexpr_drop(mk);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_put_remote_only_destination(void) {
    setup_session();
    add_fake_peer();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/put/remote-only", &keyexpr, &expanded, &rid);

    _z_subscription_rc_t *sub =
        register_local_subscription(&expanded, rid, &g_local_put_delivery_count, Z_LOCALITY_ANY);

    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);

    const char payload_data[] = "payload";
    _z_bytes_t payload;
    assert(_z_bytes_from_buf(&payload, (const uint8_t *)payload_data, sizeof(payload_data) - 1) == _Z_RES_OK);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t ts = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();

    z_result_t res =
        _z_write(&g_session, &keyexpr, &payload, &encoding, Z_SAMPLE_KIND_PUT, Z_CONGESTION_CONTROL_BLOCK,
                 Z_PRIORITY_DEFAULT, false, &ts, NULL, Z_RELIABILITY_RELIABLE, &source_info, Z_LOCALITY_REMOTE);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 1);

    _z_bytes_drop(&payload);
    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, sub);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_subscriber_remote_only_origin(void) {
    setup_session();
    add_fake_peer();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/put/remote-origin", &keyexpr, &expanded, &rid);

    _z_subscription_rc_t *sub =
        register_local_subscription(&expanded, rid, &g_local_put_delivery_count, Z_LOCALITY_REMOTE);

    atomic_store_explicit(&g_local_put_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);

    const char payload_data[] = "payload";
    _z_bytes_t payload;
    assert(_z_bytes_from_buf(&payload, (const uint8_t *)payload_data, sizeof(payload_data) - 1) == _Z_RES_OK);
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t ts = _z_timestamp_null();
    _z_source_info_t source_info = _z_source_info_null();

    z_result_t res =
        _z_write(&g_session, &keyexpr, &payload, &encoding, Z_SAMPLE_KIND_PUT, Z_CONGESTION_CONTROL_BLOCK,
                 Z_PRIORITY_DEFAULT, false, &ts, NULL, Z_RELIABILITY_RELIABLE, &source_info, Z_LOCALITY_ANY);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_put_delivery_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 1);

    _z_bytes_drop(&payload);
    _z_unregister_subscription(&g_session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, sub);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_query_remote_only_destination(void) {
    setup_session();
    add_fake_peer();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/query/remote-only", &keyexpr, &expanded, &rid);

    _z_session_queryable_rc_t *queryable_rc =
        register_local_queryable(&expanded, &g_local_query_delivery_count, Z_LOCALITY_ANY);

    atomic_store_explicit(&g_local_query_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_reply_callback_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_query_drop_callback_count, 0, memory_order_relaxed);

    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    _z_zint_t query_id = 0;
    z_result_t res =
        _z_query(&g_session, &keyexpr, NULL, 0, Z_QUERY_TARGET_DEFAULT, Z_CONSOLIDATION_MODE_LATEST, NULL, NULL,
                 query_reply_callback, query_dropper, NULL, 1000, NULL, qos, NULL, Z_LOCALITY_REMOTE, &query_id);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_query_delivery_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 1);

    // Clean pending query by simulating RESPONSE_FINAL
    _z_pending_query_t *pq = _z_pending_query_slist_value(g_session._pending_queries);
    assert(pq != NULL);
    _z_network_message_t final_msg;
    _z_n_msg_make_response_final(&final_msg, pq->_id);
    res = _z_handle_network_message(&g_fake_transport, &final_msg, NULL);
    assert(res == _Z_RES_OK);
    assert(g_session._pending_queries == NULL);

    _z_unregister_session_queryable(&g_session, queryable_rc);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

static void test_queryable_remote_only_origin(void) {
    setup_session();
    add_fake_peer();

    _z_keyexpr_t keyexpr = _z_keyexpr_null();
    _z_keyexpr_t expanded = _z_keyexpr_null();
    uint16_t rid = 0;
    create_local_resource("zenoh-pico/tests/local/query/remote-origin", &keyexpr, &expanded, &rid);

    _z_session_queryable_rc_t *queryable_rc =
        register_local_queryable(&expanded, &g_local_query_delivery_count, Z_LOCALITY_REMOTE);

    atomic_store_explicit(&g_local_query_delivery_count, 0, memory_order_relaxed);
    atomic_store_explicit(&g_network_send_count, 0, memory_order_relaxed);

    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);
    _z_zint_t query_id = 0;
    z_result_t res =
        _z_query(&g_session, &keyexpr, NULL, 0, Z_QUERY_TARGET_DEFAULT, Z_CONSOLIDATION_MODE_LATEST, NULL, NULL,
                 query_reply_callback, query_dropper, NULL, 1000, NULL, qos, NULL, Z_LOCALITY_ANY, &query_id);
    assert(res == _Z_RES_OK);
    assert(atomic_load_explicit(&g_local_query_delivery_count, memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&g_network_send_count, memory_order_relaxed) == 1);

    _z_pending_query_t *pq = _z_pending_query_slist_value(g_session._pending_queries);
    assert(pq != NULL);
    _z_network_message_t final_msg2;
    _z_n_msg_make_response_final(&final_msg2, pq->_id);
    res = _z_handle_network_message(&g_fake_transport, &final_msg2, NULL);
    assert(res == _Z_RES_OK);

    _z_unregister_session_queryable(&g_session, queryable_rc);
    cleanup_local_resource(&keyexpr, &expanded, rid);

    cleanup_session();
}

int main(void) {
    test_put_local_only_single();
    test_put_local_only_via_api();
    test_put_local_and_remote_via_api();
    test_put_local_only_multiple();
    test_put_local_and_remote();
    test_query_local_only_single();
    test_query_local_only_multiple();
    test_query_local_and_remote();
    test_query_local_and_remote_via_api();
    test_put_remote_only_destination();
    test_subscriber_remote_only_origin();
    test_query_remote_only_destination();
    test_queryable_remote_only_origin();
    return 0;
}

#else
int main(void) {
    printf(
        "This test requires: Z_FEATURE_SUBSCRIPTION, Z_FEATURE_QUERYABLE, Z_FEATURE_QUERY, Z_FEATURE_LOCAL_SUBSCRIBER"
        "and Z_FEATURE_LOCAL_QUERYABLE.\n");
    return 0;
}
#endif
