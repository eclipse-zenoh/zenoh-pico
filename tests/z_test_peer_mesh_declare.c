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
// Unit tests for unicast peer full-mesh declaration exchange.
//
// Test 1 — outbound connection declare push (_z_new_peer)
//   When a peer connects outbound via _z_new_peer after having already declared
//   subscribers, the connecting side must push those declarations to the acceptor.
//   Verified by declaring a subscriber on C before C connects to A, then asserting
//   that A (the acceptor/publisher) learns C's subscriber and delivers messages to it.
//
// Test 2 — publisher write filter via INTEREST on unicast
//   Publishers build a write filter that sends an INTEREST requesting the remote
//   peer's current subscribers.  On unicast the INTEREST must be processed and
//   answered so that the write filter reflects actual remote subscribers.
//   Verified with Z_TEST_HOOKS: the accept-path passive declare push is blocked so
//   the write filter can only be populated via the INTEREST round-trip.
//
// Topology:
//   Node A  --listen--> PORT_A   publisher + subscriber on "mesh/data"  (test 1)
//   Node C  --listen--> PORT_B   (test 1)
//              --connect-> PORT_A via _z_new_peer after declaring subscriber
//   Node A  --listen--> PORT_B   subscriber (test 2, separate port to avoid TIME_WAIT)
//   Node D  --connect-> PORT_B   publisher (test 2)

#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"
#include "zenoh-pico/api/olv_macros.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/session/loopback.h"
#include "zenoh-pico/transport/manager.h"

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_MULTI_THREAD == 1 && \
    Z_FEATURE_UNICAST_PEER == 1 && Z_FEATURE_LOCAL_SUBSCRIBER == 0

static const char *PORT_A = "tcp/127.0.0.1:7460";
static const char *PORT_B = "tcp/127.0.0.1:7461";

static const int TX_NB = 5;
static const int RX_NB = 5;

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

typedef struct {
    z_owned_config_t config;
    int id;
    atomic_int sub_msg_nb;
} node_ctx_t;

static void sample_handler(z_loaned_sample_t *sample, void *ctx) {
    node_ctx_t *node = (node_ctx_t *)ctx;
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);
    printf(">> [Node %d] Received ('%.*s': '%.*s')\n", node->id, (int)z_string_len(z_loan(keystr)),
           z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(value)), z_string_data(z_loan(value)));
    z_drop(z_move(value));
    atomic_fetch_add(&node->sub_msg_nb, 1);
}

// ---------------------------------------------------------------------------
// Test 1: outbound connection declare push
// ---------------------------------------------------------------------------
// Topology:
//   Node A: listen PORT_A, publisher + subscriber
//   Node C: listen PORT_B (peer mode requires at least one endpoint)
//            → declare subscriber
//            → _z_new_peer to PORT_A (without fix: A never learns C's subscriber)
//
// Without fix: A's write filter stays ACTIVE → puts dropped → C receives 0
// With fix: _z_new_peer pushes C's subscriber to A → A sends → C receives RX_NB

static void *node_a_outbound_task(void *ptr) {
    node_ctx_t *ctx = (node_ctx_t *)ptr;

    z_owned_session_t s;
    if (z_open(&s, z_move(ctx->config), NULL) != Z_OK) {
        printf("[Node A] Unable to open session!\n");
        return NULL;
    }
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, "mesh/data");

    z_owned_closure_sample_t sub_cb;
    z_closure(&sub_cb, sample_handler, NULL, ctx);
    if (z_declare_background_subscriber(z_loan(s), z_loan(ke), z_move(sub_cb), NULL) != Z_OK) {
        printf("[Node A] Unable to declare subscriber\n");
        z_drop(z_move(s));
        return NULL;
    }
    z_owned_publisher_t pub;
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) != Z_OK) {
        printf("[Node A] Unable to declare publisher\n");
        z_drop(z_move(s));
        return NULL;
    }

    // Wait for C to connect and push its declarations
    z_sleep_s(2);

    char buf[64];
    for (int i = 0; i < TX_NB; i++) {
        snprintf(buf, sizeof(buf), "A[%d]", i);
        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);
        z_publisher_put(z_loan(pub), z_move(payload), NULL);
        z_sleep_ms(100);
    }

    z_sleep_s(2);
    z_drop(z_move(pub));
    z_drop(z_move(s));
    return NULL;
}

static void *node_c_outbound_task(void *ptr) {
    node_ctx_t *ctx = (node_ctx_t *)ptr;

    z_owned_session_t s;
    if (z_open(&s, z_move(ctx->config), NULL) != Z_OK) {
        printf("[Node C] Unable to open session!\n");
        return NULL;
    }
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, "mesh/data");

    // Declare subscriber BEFORE connecting to A.
    // Without fix: A never learns C's subscriber via the connection-time push.
    z_owned_closure_sample_t sub_cb;
    z_closure(&sub_cb, sample_handler, NULL, ctx);
    if (z_declare_background_subscriber(z_loan(s), z_loan(ke), z_move(sub_cb), NULL) != Z_OK) {
        printf("[Node C] Unable to declare subscriber\n");
        z_drop(z_move(s));
        return NULL;
    }

    z_sleep_ms(200);

    // Connect to A AFTER declaring subscriber.
    // Without fix: _z_new_peer does not push C's declarations → A's write filter ACTIVE.
    // With fix: _z_interest_push_declarations_to_peer is called → A's write filter OFF.
    _z_session_t *zn = _Z_RC_IN_VAL(z_loan_mut(s));
    _z_string_t locator_a = _z_string_alias_str(PORT_A);
    z_owned_config_t conn_cfg;
    z_config_default(&conn_cfg);
    z_result_t peer_ret = _z_new_peer(&zn->_tp, &zn->_local_zid, &locator_a, z_loan(conn_cfg));
    z_drop(z_move(conn_cfg));
    if (peer_ret != Z_OK) {
        printf("[Node C] Unable to connect to Node A (ret=%d)!\n", peer_ret);
        z_drop(z_move(s));
        return NULL;
    }

    z_sleep_s(3);

    z_owned_publisher_t pub;
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) != Z_OK) {
        printf("[Node C] Unable to declare publisher\n");
        z_drop(z_move(s));
        return NULL;
    }
    char buf[64];
    for (int i = 0; i < TX_NB; i++) {
        snprintf(buf, sizeof(buf), "C[%d]", i);
        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);
        z_publisher_put(z_loan(pub), z_move(payload), NULL);
        z_sleep_ms(100);
    }
    z_sleep_s(1);

    z_drop(z_move(pub));
    z_drop(z_move(s));
    return NULL;
}

static bool test_new_peer_outbound_declare_push(void) {
    printf("\n=== Test 1: outbound connection declare push ===\n");

    node_ctx_t node_a = {.id = 0};
    node_ctx_t node_c = {.id = 1};
    atomic_init(&node_a.sub_msg_nb, 0);
    atomic_init(&node_c.sub_msg_nb, 0);

    z_config_default(&node_a.config);
    zp_config_insert(z_loan_mut(node_a.config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(node_a.config), Z_CONFIG_LISTEN_KEY, PORT_A);

    z_config_default(&node_c.config);
    zp_config_insert(z_loan_mut(node_c.config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(node_c.config), Z_CONFIG_LISTEN_KEY, PORT_B);

    _z_task_t task_a, task_c;
    _z_task_init(&task_a, NULL, node_a_outbound_task, &node_a);
    z_sleep_ms(300);
    _z_task_init(&task_c, NULL, node_c_outbound_task, &node_c);

    _z_task_join(&task_a);
    _z_task_join(&task_c);

    int a_nb = atomic_load(&node_a.sub_msg_nb);
    int c_nb = atomic_load(&node_c.sub_msg_nb);

    printf("  Node A (acceptor+publisher): received %d/%d from C\n", a_nb, RX_NB);
    printf("  Node C (connector+subscriber): received %d/%d from A\n", c_nb, RX_NB);

    bool a_ok = a_nb >= RX_NB;
    bool c_ok = c_nb >= RX_NB;

    if (!c_ok) {
        printf("FAIL: Node C (connector) received %d/%d from A.\n", c_nb, RX_NB);
        printf("      _z_new_peer did not push C's declarations to A.\n");
    }
    if (!a_ok) {
        printf("FAIL: Node A received %d/%d from C.\n", a_nb, RX_NB);
    }
    return a_ok && c_ok;
}

// ---------------------------------------------------------------------------
// Test 2: publisher write filter via INTEREST on unicast
// ---------------------------------------------------------------------------
// Topology:
//   Node A: listen PORT_B, subscriber
//   Node D: connect PORT_B (via z_open), then declares publisher
//
// The hook blocks accept-path passive declare pushes (declares with interest_id == 0
// sent to a specific peer). This forces the publisher's write filter to rely solely
// on the INTEREST round-trip.
//
// Without fix: no INTEREST sent → filter stays ACTIVE → puts dropped → A gets 0
// With fix: INTEREST sent to A → A responds (interest_id != 0, allowed by hook)
//           → D's write filter goes OFF → puts delivered → A gets RX_NB

static atomic_bool g_block_passive_push = false;

static z_result_t passive_push_block_hook(_z_session_t *zn, const _z_network_message_t *n_msg,
                                           z_reliability_t reliability, z_congestion_control_t cong_ctrl,
                                           void *peer, bool *handled) {
    (void)zn;
    (void)reliability;
    (void)cong_ctrl;
    if (!atomic_load(&g_block_passive_push) || peer == NULL) {
        *handled = false;
        return _Z_RES_OK;
    }
    // Block targeted declares with interest_id == 0 (accept-path passive push).
    // Allow declares with interest_id != 0 (INTEREST responses).
    if (n_msg->_tag == _Z_N_DECLARE) {
        const _z_optional_id_t *oid = &n_msg->_body._declare._interest_id;
        if (oid->has_value && oid->value == 0) {
            *handled = true;  // drop the passive push
            return _Z_RES_OK;
        }
    }
    *handled = false;
    return _Z_RES_OK;
}

static void *node_a_interest_task(void *ptr) {
    node_ctx_t *ctx = (node_ctx_t *)ptr;

    z_owned_session_t s;
    if (z_open(&s, z_move(ctx->config), NULL) != Z_OK) {
        printf("[Node A] Unable to open session!\n");
        return NULL;
    }
    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, "mesh/data");

    z_owned_closure_sample_t sub_cb;
    z_closure(&sub_cb, sample_handler, NULL, ctx);
    if (z_declare_background_subscriber(z_loan(s), z_loan(ke), z_move(sub_cb), NULL) != Z_OK) {
        printf("[Node A] Unable to declare subscriber\n");
        z_drop(z_move(s));
        return NULL;
    }

    z_sleep_s(5);
    z_drop(z_move(s));
    return NULL;
}

static void *node_d_interest_task(void *ptr) {
    node_ctx_t *ctx = (node_ctx_t *)ptr;

    // The hook is already active (set before this task started).
    // It blocks targeted declares with interest_id == 0, so A's accept-path passive push
    // is suppressed — D's write filter can only be populated via the INTEREST round-trip.
    z_owned_session_t s;
    if (z_open(&s, z_move(ctx->config), NULL) != Z_OK) {
        printf("[Node D] Unable to open session!\n");
        return NULL;
    }

    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, "mesh/data");

    z_owned_publisher_t pub;
    if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) != Z_OK) {
        printf("[Node D] Unable to declare publisher\n");
        z_drop(z_move(s));
        return NULL;
    }

    // Wait for the INTEREST round-trip to complete.
    z_sleep_ms(500);

    char buf[64];
    for (int i = 0; i < TX_NB; i++) {
        snprintf(buf, sizeof(buf), "D[%d]", i);
        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);
        z_publisher_put(z_loan(pub), z_move(payload), NULL);
        z_sleep_ms(100);
    }

    z_sleep_s(1);

    z_drop(z_move(pub));
    z_drop(z_move(s));
    return NULL;
}

static bool test_publisher_write_filter_via_interest(void) {
    printf("\n=== Test 2: publisher write filter via INTEREST on unicast ===\n");

    node_ctx_t node_a = {.id = 0};
    node_ctx_t node_d = {.id = 1};
    atomic_init(&node_a.sub_msg_nb, 0);
    atomic_init(&node_d.sub_msg_nb, 0);

    z_config_default(&node_a.config);
    zp_config_insert(z_loan_mut(node_a.config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(node_a.config), Z_CONFIG_LISTEN_KEY, PORT_B);

    z_config_default(&node_d.config);
    zp_config_insert(z_loan_mut(node_d.config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(node_d.config), Z_CONFIG_CONNECT_KEY, PORT_B);

    // Install hook BEFORE starting any tasks so it is active when A's accept task
    // fires during D's z_open.  This blocks the accept-path passive push (interest_id 0)
    // while allowing INTEREST responses (interest_id != 0).
    atomic_store(&g_block_passive_push, true);
    _z_transport_set_send_n_msg_override(passive_push_block_hook);

    _z_task_t task_a, task_d;
    _z_task_init(&task_a, NULL, node_a_interest_task, &node_a);
    z_sleep_ms(300);
    _z_task_init(&task_d, NULL, node_d_interest_task, &node_d);

    _z_task_join(&task_a);
    _z_task_join(&task_d);

    _z_transport_set_send_n_msg_override(NULL);
    atomic_store(&g_block_passive_push, false);

    int a_nb = atomic_load(&node_a.sub_msg_nb);
    printf("  Node A (acceptor+subscriber): received %d/%d from D\n", a_nb, RX_NB);

    bool a_ok = a_nb >= RX_NB;
    if (!a_ok) {
        printf("FAIL: Node A received %d/%d from D.\n", a_nb, RX_NB);
        printf("      Publisher write filter stayed ACTIVE: INTEREST not sent or not handled.\n");
    }
    return a_ok;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    bool t1_ok = test_new_peer_outbound_declare_push();
    bool t2_ok = test_publisher_write_filter_via_interest();

    printf("\nSummary:\n");
    printf("  outbound connection declare push: %s\n", t1_ok ? "PASS" : "FAIL");
    printf("  publisher write filter via INTEREST: %s\n", t2_ok ? "PASS" : "FAIL");

    return (t1_ok && t2_ok) ? 0 : -1;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf(
        "Skipped: requires Z_FEATURE_SUBSCRIPTION, Z_FEATURE_PUBLICATION, Z_FEATURE_MULTI_THREAD, "
        "Z_FEATURE_UNICAST_PEER, and Z_FEATURE_LOCAL_SUBSCRIBER=0\n");
    return 0;
}

#endif
