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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico.h"

#undef NDEBUG
#include <assert.h>

static const char *ARG_LIST[] = {"Z_FEATURE_PUBLICATION", "Z_FEATURE_SUBSCRIPTION", "Z_FEATURE_QUERYABLE",
                                 "Z_FEATURE_QUERY"};
#define ARG_NB (sizeof(ARG_LIST) / sizeof(ARG_LIST[0]))

int test_publication(void) {
#if Z_FEATURE_PUBLICATION == 1
    const char *keyexpr = "demo/example/zenoh-pico-pub";
    const char *value = "Pub from Pico!";
    const char *mode = "client";

    // Set up config
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    // Open session
    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }
    // Declare publisher
    printf("Declaring publisher for '%s'...\n", keyexpr);
    z_owned_publisher_t pub = z_declare_publisher(z_loan(s), z_keyexpr(keyexpr), NULL);
    if (!z_check(pub)) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }
    // Put data
    printf("Putting Data ('%s': '%s')...\n", keyexpr, value);
    z_publisher_put_options_t options = z_publisher_put_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    z_publisher_put(z_loan(pub), (const uint8_t *)value, strlen(value), &options);

    // Clean-up
    z_undeclare_publisher(z_move(pub));
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));
    return 1;
#else
    return 0;
#endif
}

#if Z_FEATURE_SUBSCRIPTION == 1
static void subscription_data_handler(const z_sample_t *sample, void *ctx) {
    (void)(ctx);
    z_owned_str_t keystr = z_keyexpr_to_string(sample->keyexpr);
    printf(">> [Subscriber] Received ('%s': '%.*s')\n", z_loan(keystr), (int)sample->payload.len,
           sample->payload.start);
    z_drop(z_move(keystr));
}
#endif

int test_subscription(void) {
#if Z_FEATURE_SUBSCRIPTION == 1
    const char *keyexpr = "demo/example/**";
    const char *mode = "client";

    // Set up config
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    // Open session
    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }
    // Declare subscriber
    z_owned_closure_sample_t callback = z_closure(subscription_data_handler);
    printf("Declaring Subscriber on '%s'...\n", keyexpr);
    z_owned_subscriber_t sub = z_declare_subscriber(z_loan(s), z_keyexpr(keyexpr), z_move(callback), NULL);
    if (!z_check(sub)) {
        printf("Unable to declare subscriber.\n");
        return -1;
    }
    // Clean-up
    z_undeclare_subscriber(z_move(sub));
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));
    return 1;
#else
    return 0;
#endif
}

#if Z_FEATURE_QUERYABLE == 1
static const char *queryable_keyexpr = "demo/example/zenoh-pico-queryable";
static const char *queryable_value = "Queryable from Pico!";

void query_handler(const z_query_t *query, void *ctx) {
    (void)(ctx);
    z_owned_str_t keystr = z_keyexpr_to_string(z_query_keyexpr(query));
    z_bytes_t pred = z_query_parameters(query);
    z_value_t payload_value = z_query_value(query);
    printf(" >> [Queryable handler] Received Query '%s?%.*s'\n", z_loan(keystr), (int)pred.len, pred.start);
    if (payload_value.payload.len > 0) {
        printf("     with value '%.*s'\n", (int)payload_value.payload.len, payload_value.payload.start);
    }
    z_query_reply_options_t options = z_query_reply_options_default();
    options.encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    z_query_reply(query, z_keyexpr(queryable_keyexpr), (const unsigned char *)queryable_value, strlen(queryable_value), &options);
    z_drop(z_move(keystr));
}
#endif

int test_queryable(void) {
#if Z_FEATURE_QUERYABLE == 1
    const char *mode = "client";

    z_keyexpr_t ke = z_keyexpr(queryable_keyexpr);
    if (!z_check(ke)) {
        printf("%s is not a valid key expression", queryable_keyexpr);
        return -1;
    }
    // Set up config
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    // Open session
    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }
    // Declare queryable
    printf("Creating Queryable on '%s'...\n", queryable_keyexpr);
    z_owned_closure_query_t callback = z_closure(query_handler);
    z_owned_queryable_t qable = z_declare_queryable(z_loan(s), ke, z_move(callback), NULL);
    if (!z_check(qable)) {
        printf("Unable to create queryable.\n");
        return -1;
    }
    // Clean-up
    z_undeclare_queryable(z_move(qable));
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));

    return 1;
#else
    return 0;
#endif
}

#if Z_FEATURE_QUERY == 1
void reply_dropper(void *ctx) {
    (void)(ctx);
    printf(">> Received query final notification\n");
}

void reply_handler(z_owned_reply_t *reply, void *ctx) {
    (void)(ctx);
    if (z_reply_is_ok(reply)) {
        z_sample_t sample = z_reply_ok(reply);
        z_owned_str_t keystr = z_keyexpr_to_string(sample.keyexpr);
        printf(">> Received ('%s': '%.*s')\n", z_loan(keystr), (int)sample.payload.len, sample.payload.start);
        z_drop(z_move(keystr));
    } else {
        printf(">> Received an error\n");
    }
}
#endif

int test_query(void) {
#if Z_FEATURE_QUERY == 1
    const char *keyexpr = "demo/example/**";
    const char *mode = "client";

    z_keyexpr_t ke = z_keyexpr(keyexpr);
    if (!z_check(ke)) {
        printf("%s is not a valid key expression", keyexpr);
        return -1;
    }
    // Set up config
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(mode));
    // Open session
    printf("Opening session...\n");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        return -1;
    }
    // Start read and lease tasks for zenoh-pico
    if (zp_start_read_task(z_loan(s), NULL) < 0 || zp_start_lease_task(z_loan(s), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }
    // Send query
    printf("Sending Query '%s'...\n", keyexpr);
    z_get_options_t opts = z_get_options_default();
    z_owned_closure_reply_t callback = z_closure(reply_handler, reply_dropper);
    if (z_get(z_loan(s), ke, "", z_move(callback), &opts) < 0) {
        printf("Unable to send query.\n");
        return -1;
    }
    // Clean-up
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));
    z_close(z_move(s));

    return 1;
#else
    return 0;
#endif
}

// Send feature config as int list, and compare with compiled feature
int main(int argc, char **argv) {
    if (argc < (int)(ARG_NB + 1)) {
        printf("To start this test you must give the state of the feature config as argument\n");
        printf("Arg order: ");
        for (size_t i = 0; i < ARG_NB; i++) {
            printf("%s ", ARG_LIST[i]);
        }
        printf("\n");
        return -1;
    }
    if (test_publication() != atoi(argv[1])) {
        printf("Problem during publication testing\n");
        return -1;
    } else {
        printf("Publication status ok\n");
    }
    if (test_subscription() != atoi(argv[2])) {
        printf("Problem during subscription testing\n");
        return -1;
    } else {
        printf("Subscription status ok\n");
    }
    if (test_queryable() != atoi(argv[3])) {
        printf("Problem during queryable testing\n");
        return -1;
    } else {
        printf("Queryable status ok\n");
    }
    if (test_query() != atoi(argv[4])) {
        printf("Problem during query testing\n");
        return -1;
    } else {
        printf("Query status ok\n");
    }
    return 0;
}
