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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_QUERY == 1 && Z_FEATURE_QUERYABLE == 1 && \
    Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_LOCAL_SUBSCRIBER == 0 && Z_FEATURE_UNICAST_PEER == 1 &&                 \
    defined Z_FEATURE_UNSTABLE_API
typedef struct _node_ctx {
    z_owned_config_t config;
    const char *keyexpr_out;
    int id;
    int sub_msg_nb;
    int qybl_msg_nb;
} _node_ctx_t;

const char *keyexpr_in = "test/**";
const char *qybl_val = "Queryable data";
const char *pub_val = "Publisher data";
const int tx_nb = 5;
const int rx_nb = 15;

void query_handler(z_loaned_query_t *query, void *ctx) {
    _node_ctx_t *node_ctx = (_node_ctx_t *)ctx;
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_query_keyexpr(query), &keystr);
    z_view_string_t params;
    z_query_parameters(query, &params);
    printf(" >> [Queryable %d] Received ('%.*s': '%.*s')\n", node_ctx->id, (int)z_string_len(z_loan(keystr)),
           z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(params)), z_string_data(z_loan(params)));
    // Reply value
    z_owned_bytes_t reply_payload;
    z_bytes_from_static_str(&reply_payload, qybl_val);
    z_query_reply(query, z_query_keyexpr(query), z_move(reply_payload), NULL);
    node_ctx->qybl_msg_nb++;
}

void pub_handler(z_loaned_sample_t *sample, void *ctx) {
    _node_ctx_t *node_ctx = (_node_ctx_t *)ctx;
    z_view_string_t keystr;
    z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
    z_owned_string_t value;
    z_bytes_to_string(z_sample_payload(sample), &value);
    printf(">> [Subscriber %d] Received ('%.*s': '%.*s')\n", node_ctx->id, (int)z_string_len(z_loan(keystr)),
           z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(value)), z_string_data(z_loan(value)));
    z_drop(z_move(value));
    node_ctx->sub_msg_nb++;
}

void *node_task(void *ptr) {
    _node_ctx_t *ctx = (_node_ctx_t *)ptr;

    // Open session
    printf("Opening session...\n");
    z_owned_session_t s;
    if (z_open(&s, z_move(ctx->config), NULL) != Z_OK) {
        printf("Unable to open session!\n");
        return NULL;
    }
    // Start read and lease tasks
    if (zp_start_read_task(z_loan_mut(s), NULL) != Z_OK || zp_start_lease_task(z_loan_mut(s), NULL) != Z_OK) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return NULL;
    }
    // Create keyexprs
    z_view_keyexpr_t sub_qybl_ke;
    if (z_view_keyexpr_from_str(&sub_qybl_ke, keyexpr_in) != Z_OK) {
        printf("%s is not a valid key expression\n", keyexpr_in);
        return NULL;
    }
    z_view_keyexpr_t pub_qry_ke;
    if (z_view_keyexpr_from_str(&pub_qry_ke, ctx->keyexpr_out) < 0) {
        printf("%s is not a valid key expression\n", ctx->keyexpr_out);
        return NULL;
    }
    // Declare subscriber
    printf("Declaring Subscriber on '%s'...\n", keyexpr_in);
    z_owned_closure_sample_t sub_cb;
    z_closure(&sub_cb, pub_handler, NULL, ctx);
    if (z_declare_background_subscriber(z_loan(s), z_loan(sub_qybl_ke), z_move(sub_cb), NULL) != Z_OK) {
        printf("Unable to declare subscriber.\n");
        return NULL;
    }
    // Declare queryable
    printf("Creating Queryable on '%s'...\n", keyexpr_in);
    z_owned_closure_query_t qybl_cb;
    z_closure(&qybl_cb, query_handler, NULL, ctx);
    if (z_declare_background_queryable(z_loan(s), z_loan(sub_qybl_ke), z_move(qybl_cb), NULL) != Z_OK) {
        printf("Unable to create queryable.\n");
        return NULL;
    }
    // Declare publisher
    printf("Declaring publisher for '%s'...\n", ctx->keyexpr_out);
    z_owned_publisher_t pub;
    if (z_declare_publisher(z_loan(s), &pub, z_loan(pub_qry_ke), NULL) != Z_OK) {
        printf("Unable to declare publisher for key expression!\n");
        return NULL;
    }
    // Declare querier
    printf("Declaring Querier on '%s'...\n", ctx->keyexpr_out);
    z_owned_querier_t querier;
    if (z_declare_querier(z_loan(s), &querier, z_loan(pub_qry_ke), NULL) != Z_OK) {
        printf("Unable to declare Querier for key expression!\n");
        return NULL;
    }
    // Wait for other nodes to come online
    z_sleep_s(1);
    printf("Starting sending data\n");
    // Publish data
    char buf[256];
    for (int idx = 0; idx < tx_nb; ++idx) {
        z_sleep_s(1);
        // Create payload
        sprintf(buf, "[%4d] %s", idx, pub_val);
        printf("[Publisher %d] Sending ('%s': '%s')...\n", ctx->id, ctx->keyexpr_out, buf);
        z_owned_bytes_t payload;
        z_bytes_copy_from_str(&payload, buf);
        // Send data
        z_publisher_put(z_loan(pub), z_move(payload), NULL);
    }
    // Querier data
    for (int idx = 0; idx < tx_nb; ++idx) {
        z_sleep_s(1);
        // Create payload
        sprintf(buf, "[%4d] %s", idx, "");
        printf("[Querier %d] Sending ('%s': '%s')...\n", ctx->id, ctx->keyexpr_out, buf);
        // Query data
        z_owned_fifo_handler_reply_t qry_handler;
        z_owned_closure_reply_t qry_closure;
        z_fifo_channel_reply_new(&qry_closure, &qry_handler, 16);
        z_querier_get(z_loan(querier), NULL, z_move(qry_closure), NULL);
        // Process received data
        z_owned_reply_t reply;
        for (z_result_t res = z_recv(z_loan(qry_handler), &reply); res == Z_OK;
             res = z_recv(z_loan(qry_handler), &reply)) {
            if (z_reply_is_ok(z_loan(reply))) {
                const z_loaned_sample_t *sample = z_reply_ok(z_loan(reply));
                z_view_string_t keystr;
                z_keyexpr_as_view_string(z_sample_keyexpr(sample), &keystr);
                z_owned_string_t replystr;
                z_bytes_to_string(z_sample_payload(sample), &replystr);
                printf(">> [Query %d] Received ('%.*s': '%.*s')\n", ctx->id, (int)z_string_len(z_loan(keystr)),
                       z_string_data(z_loan(keystr)), (int)z_string_len(z_loan(replystr)),
                       z_string_data(z_loan(replystr)));
                z_drop(z_move(replystr));
            }
            z_drop(z_move(reply));
        }
        z_drop(z_move(qry_handler));
    }
    // Wait for sub & queryable data
    while (true) {
        if ((ctx->sub_msg_nb >= rx_nb) && (ctx->qybl_msg_nb >= rx_nb)) {
            break;
        }
        z_sleep_s(1);
    }
    // Wait for other threads to resolve properly
    z_sleep_s(1);
    // Clean up
    z_drop(z_move(pub));
    z_drop(z_move(querier));
    z_drop(z_move(s));
    return NULL;
}

static void test_packet_transmission(void) {
    // Create node context
    _node_ctx_t node_ctx_0 = {.keyexpr_out = "test/A", .id = 0, .qybl_msg_nb = 0, .sub_msg_nb = 0};
    _node_ctx_t node_ctx_1 = {.keyexpr_out = "test/B", .id = 1, .qybl_msg_nb = 0, .sub_msg_nb = 0};
    _node_ctx_t node_ctx_2 = {.keyexpr_out = "test/C", .id = 2, .qybl_msg_nb = 0, .sub_msg_nb = 0};
    _node_ctx_t node_ctx_3 = {.keyexpr_out = "test/D", .id = 3, .qybl_msg_nb = 0, .sub_msg_nb = 0};
    // Init config
    z_config_default(&node_ctx_0.config);
    z_config_default(&node_ctx_1.config);
    z_config_default(&node_ctx_2.config);
    z_config_default(&node_ctx_3.config);
    // Fill config
    zp_config_insert(z_loan_mut(node_ctx_0.config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(node_ctx_0.config), Z_CONFIG_LISTEN_KEY, "tcp/127.0.0.1:7447");
    zp_config_insert(z_loan_mut(node_ctx_1.config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(node_ctx_1.config), Z_CONFIG_LISTEN_KEY, "tcp/127.0.0.1:7448");
    zp_config_insert(z_loan_mut(node_ctx_1.config), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7447");
    zp_config_insert(z_loan_mut(node_ctx_2.config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(node_ctx_2.config), Z_CONFIG_LISTEN_KEY, "tcp/127.0.0.1:7449");
    zp_config_insert(z_loan_mut(node_ctx_2.config), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7447");
    zp_config_insert(z_loan_mut(node_ctx_2.config), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7448");
    zp_config_insert(z_loan_mut(node_ctx_3.config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(node_ctx_3.config), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7447");
    zp_config_insert(z_loan_mut(node_ctx_3.config), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7448");
    zp_config_insert(z_loan_mut(node_ctx_3.config), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7449");
    // Init threads in a staggered manner to let time for sockets to establish
    _z_task_t task0, task1, task2, task3;
    _z_task_init(&task0, NULL, node_task, &node_ctx_0);
    z_sleep_ms(100);
    _z_task_init(&task1, NULL, node_task, &node_ctx_1);
    z_sleep_ms(100);
    _z_task_init(&task2, NULL, node_task, &node_ctx_2);
    z_sleep_ms(100);
    _z_task_init(&task3, NULL, node_task, &node_ctx_3);
    // Wait a bit
    z_sleep_s(20);
    // Clean up
    _z_task_join(&task0);
    _z_task_join(&task1);
    _z_task_join(&task2);
    _z_task_join(&task3);
}

static bool test_peer_connection(void) {
    // Init config
    z_owned_config_t config;
    z_config_default(&config);
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, "tcp/127.0.0.1:7447");
    // Open main session
    z_owned_session_t s;
    if (z_open(&s, z_move(config), NULL) != Z_OK) {
        printf("Unable to open main session!\n");
        return false;
    }
    if (zp_start_read_task(z_loan_mut(s), NULL) != Z_OK || zp_start_lease_task(z_loan_mut(s), NULL) != Z_OK) {
        printf("Unable to start read and lease tasks\n");
        z_session_drop(z_session_move(&s));
        return NULL;
    }
    z_owned_session_t sess_array[Z_LISTEN_MAX_CONNECTION_NB + 1];
    z_owned_config_t cfg_array[Z_LISTEN_MAX_CONNECTION_NB + 1];
    // // Open max peers
    for (int i = 0; i < Z_LISTEN_MAX_CONNECTION_NB; i++) {
        z_config_default(&cfg_array[i]);
        zp_config_insert(z_loan_mut(cfg_array[i]), Z_CONFIG_MODE_KEY, "peer");
        zp_config_insert(z_loan_mut(cfg_array[i]), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7447");
        if (z_open(&sess_array[i], z_move(cfg_array[i]), NULL) != Z_OK) {
            printf("Unable to open peer session!\n");
            return false;
        }
        z_sleep_ms(100);
    }
    // Fail to open a new one
    z_config_default(&cfg_array[Z_LISTEN_MAX_CONNECTION_NB]);
    zp_config_insert(z_loan_mut(cfg_array[Z_LISTEN_MAX_CONNECTION_NB]), Z_CONFIG_MODE_KEY, "peer");
    zp_config_insert(z_loan_mut(cfg_array[Z_LISTEN_MAX_CONNECTION_NB]), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7447");
    if (z_open(&sess_array[Z_LISTEN_MAX_CONNECTION_NB], z_move(cfg_array[Z_LISTEN_MAX_CONNECTION_NB]), NULL) == Z_OK) {
        printf("Should not have been able to open this session\n");
        return false;
    }
    // Close first session
    z_drop(z_move(sess_array[0]));
    z_sleep_ms(100);
    // Alternate opening and closing first session a few times
    for (int i = 0; i < 5; i++) {
        z_config_default(&cfg_array[0]);
        zp_config_insert(z_loan_mut(cfg_array[0]), Z_CONFIG_MODE_KEY, "peer");
        zp_config_insert(z_loan_mut(cfg_array[0]), Z_CONFIG_CONNECT_KEY, "tcp/127.0.0.1:7447");
        if (z_open(&sess_array[0], z_move(cfg_array[0]), NULL) != Z_OK) {
            printf("Unable to open peer session!\n");
            return false;
        }
        z_sleep_ms(100);
        z_drop(z_move(sess_array[0]));
        z_sleep_ms(100);
    }
    z_drop(z_move(s));
    return true;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_packet_transmission();
    printf("Test connections...");
    if (!test_peer_connection()) {
        return -1;
    }
    printf(" Ok\n");
    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf(
        "Missing config token to build this test. This test requires: Z_FEATURE_SUBSCRIPTION, Z_FEATURE_PUBLICATION, "
        "Z_FEATURE_QUERY, Z_FEATURE_QUERYABLE, Z_FEATURE_MULTI_THREAD, Z_FEATURE_UNICAST_PEER and "
        "Z_FEATURE_UNSTABLE_API (until querier becomes stable)\n");
    printf("It also requires Z_FEATURE_LOCAL_SUBSCRIBER to be deactivated\n");
    return 0;
}

#endif
