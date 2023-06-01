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

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#else
#include <unistd.h>
#endif

#include "zenoh-pico.h"

#define SLEEP 2
#define SCOUTING_TIMEOUT "1000"

const char *value = "Test value";

volatile unsigned int zids = 0;
void zid_handler(const z_id_t *id, void *arg) {
    (void)(arg);
    (void)(id);
    printf("%s\n", __func__);
    zids++;
}

volatile unsigned int hellos = 0;
void hello_handler(z_owned_hello_t *hello, void *arg) {
    (void)(arg);
    printf("%s\n", __func__);
    hellos++;
    z_hello_null();
    z_drop(hello);  // validate double-drop safety: caller drops hello if it's not dropped by the handler
}

volatile unsigned int queries = 0;
void query_handler(const z_query_t *query, void *arg) {
    printf("%s\n", __func__);
    queries++;

    z_owned_str_t k_str = z_keyexpr_to_string(z_query_keyexpr(query));
#ifdef ZENOH_PICO
    if (z_check(k_str) == false) {
        k_str = zp_keyexpr_resolve(*(z_session_t *)arg, z_query_keyexpr(query));
    }
#endif

    z_bytes_t pred = z_query_parameters(query);
    (void)(pred);
    z_value_t payload_value = z_query_value(query);
    (void)(payload_value);
    z_query_reply_options_t _ret_qreply_opt = z_query_reply_options_default();
    z_query_reply(query, z_keyexpr(z_loan(k_str)), (const uint8_t *)value, strlen(value), &_ret_qreply_opt);

    z_drop(z_move(k_str));
}

volatile unsigned int replies = 0;
void reply_handler(z_owned_reply_t *reply, void *arg) {
    printf("%s\n", __func__);
    replies++;

    if (z_reply_is_ok(reply)) {
        z_sample_t sample = z_reply_ok(reply);

        z_owned_str_t k_str = z_keyexpr_to_string(sample.keyexpr);
#ifdef ZENOH_PICO
        if (z_check(k_str) == false) {
            k_str = zp_keyexpr_resolve(*(z_session_t *)arg, sample.keyexpr);
        }
#endif
        z_drop(z_move(k_str));
    } else {
        z_value_t _ret_zvalue = z_reply_err(reply);
        (void)(_ret_zvalue);
    }

    z_reply_null();  // Does nothing. Just to test compilation
    z_drop(reply);   // validate double-drop safety: caller drops reply if it's not dropped by the handler
}

volatile unsigned int datas = 0;
void data_handler(const z_sample_t *sample, void *arg) {
    printf("%s\n", __func__);
    datas++;

    z_owned_str_t k_str = z_keyexpr_to_string(sample->keyexpr);
#ifdef ZENOH_PICO
    if (z_check(k_str) == false) {
        k_str = zp_keyexpr_resolve(*(z_session_t *)arg, sample->keyexpr);
    }
#endif
    z_drop(z_move(k_str));
}

int main(int argc, char **argv) {
    assert(argc == 2);
    (void)(argc);
    setvbuf(stdout, NULL, _IOLBF, 1024);

#ifdef ZENOH_C
    zc_init_logger();
#endif

    printf("Testing Keyexpr...");
    z_keyexpr_t key = z_keyexpr("demo/example");
    _Bool _ret_bool = z_keyexpr_is_initialized(&key);
    assert(_ret_bool == true);

    int8_t _ret_int8 = z_keyexpr_includes(z_keyexpr("demo/example/**"), z_keyexpr("demo/example/a"));
    assert(_ret_int8 == 0);
#ifdef ZENOH_PICO
    _ret_int8 = zp_keyexpr_includes_null_terminated("demo/example/**", "demo/example/a");
    assert(_ret_int8 == 0);
#endif
    _ret_int8 = z_keyexpr_intersects(z_keyexpr("demo/example/**"), z_keyexpr("demo/example/a"));
    assert(_ret_int8 == 0);
#ifdef ZENOH_PICO
    _ret_int8 = zp_keyexpr_intersect_null_terminated("demo/example/**", "demo/example/a");
    assert(_ret_int8 == 0);
#endif
    _ret_int8 = z_keyexpr_equals(z_keyexpr("demo/example/**"), z_keyexpr("demo/example"));
    assert(_ret_int8 == -1);
#ifdef ZENOH_PICO
    _ret_int8 = zp_keyexpr_equals_null_terminated("demo/example/**", "demo/example");
    assert(_ret_int8 == -1);
#endif

    sleep(SLEEP);

    size_t keyexpr_len = strlen("demo/example/**/*");
    char *keyexpr_str = (char *)malloc(keyexpr_len + 1);
    memcpy(keyexpr_str, "demo/example/**/*", keyexpr_len);
    keyexpr_str[keyexpr_len] = '\0';
    _ret_int8 = z_keyexpr_is_canon(keyexpr_str, keyexpr_len);
    assert(_ret_int8 < 0);

#ifdef ZENOH_PICO
    _ret_int8 = zp_keyexpr_is_canon_null_terminated(keyexpr_str);
    assert(_ret_int8 < 0);
#endif
    _ret_int8 = z_keyexpr_canonize(keyexpr_str, &keyexpr_len);
    assert(_ret_int8 == 0);
    assert(strlen("demo/example/**/*") == keyexpr_len);
#ifdef ZENOH_PICO
    _ret_int8 = zp_keyexpr_canonize_null_terminated(keyexpr_str);
    assert(_ret_int8 == 0);
    assert(strlen("demo/example/**/*") == keyexpr_len);
#endif

    printf("Ok\n");
    sleep(SLEEP);

    printf("Testing Configs...");
    z_owned_config_t _ret_config = z_config_new();
    assert(z_check(_ret_config));
    z_drop(z_move(_ret_config));
    _ret_config = z_config_default();
    assert(z_check(_ret_config));
#ifdef ZENOH_PICO
    _ret_int8 = zp_config_insert(z_loan(_ret_config), Z_CONFIG_PEER_KEY, z_string_make(argv[1]));
    assert(_ret_int8 == 0);
    const char *_ret_cstr = zp_config_get(z_loan(_ret_config), Z_CONFIG_PEER_KEY);
    assert(strlen(_ret_cstr) == strlen(argv[1]));
    assert(strncmp(_ret_cstr, argv[1], strlen(_ret_cstr)) == 0);
#endif

    z_owned_scouting_config_t _ret_sconfig = z_scouting_config_default();
    assert(z_check(_ret_sconfig));
#ifdef ZENOH_PICO
    _ret_int8 =
        zp_scouting_config_insert(z_loan(_ret_sconfig), Z_CONFIG_SCOUTING_TIMEOUT_KEY, z_string_make(SCOUTING_TIMEOUT));
    assert(_ret_int8 == 0);
    _ret_cstr = zp_scouting_config_get(z_loan(_ret_sconfig), Z_CONFIG_SCOUTING_TIMEOUT_KEY);
    assert(strlen(_ret_cstr) == strlen(SCOUTING_TIMEOUT));
    assert(strncmp(_ret_cstr, SCOUTING_TIMEOUT, strlen(_ret_cstr)) == 0);
#endif
    z_drop(z_move(_ret_sconfig));

    printf("Ok\n");
    sleep(SLEEP);

    printf("Testing Scouting...");
    _ret_sconfig = z_scouting_config_from(z_loan(_ret_config));
    z_owned_closure_hello_t _ret_closure_hello = z_closure(hello_handler, NULL, NULL);
    _ret_int8 = z_scout(z_move(_ret_sconfig), z_move(_ret_closure_hello));
    assert(_ret_int8 == 0);
    assert(hellos >= 1);

    uint32_t _scouting_timeout = strtoul(SCOUTING_TIMEOUT, NULL, 10);
    z_sleep_ms(_scouting_timeout);
    printf("Ok\n");
    z_sleep_s(SLEEP);

    z_owned_session_t s1 = z_open(z_move(_ret_config));
    assert(z_check(s1));
    z_id_t _ret_zid = z_info_zid(z_loan(s1));
    printf("Session 1 with PID: 0x");
    for (unsigned long i = 0; i < sizeof(_ret_zid); i++) {
        printf("%.2X", _ret_zid.id[i]);
    }
    printf("\n");

    z_owned_closure_zid_t _ret_closure_zid = z_closure(zid_handler, NULL, NULL);
    _ret_int8 = z_info_peers_zid(z_loan(s1), z_move(_ret_closure_zid));
    assert(_ret_int8 == 0);
    sleep(SLEEP);
    assert(zids == 0);

    _ret_int8 = z_info_routers_zid(z_loan(s1), z_move(_ret_closure_zid));
    assert(_ret_int8 == 0);

    sleep(SLEEP);
    assert(zids == 1);

#ifdef ZENOH_PICO
    zp_task_read_options_t _ret_read_opt = zp_task_read_options_default();
    zp_start_read_task(z_loan(s1), &_ret_read_opt);
    zp_task_lease_options_t _ret_lease_opt = zp_task_lease_options_default();
    zp_start_lease_task(z_loan(s1), &_ret_lease_opt);
#endif

    sleep(SLEEP);

    _ret_config = z_config_default();
#ifdef ZENOH_PICO
    _ret_int8 = zp_config_insert(z_loan(_ret_config), Z_CONFIG_PEER_KEY, z_string_make(argv[1]));
    assert(_ret_int8 == 0);
    _ret_cstr = zp_config_get(z_loan(_ret_config), Z_CONFIG_PEER_KEY);
    assert(strlen(_ret_cstr) == strlen(argv[1]));
    assert(strncmp(_ret_cstr, argv[1], strlen(_ret_cstr)) == 0);
#endif

    z_owned_session_t s2 = z_open(z_move(_ret_config));
    assert(z_check(s2));
    _ret_zid = z_info_zid(z_loan(s2));
    printf("Session 2 with PID: 0x");
    for (unsigned long i = 0; i < sizeof(_ret_zid); i++) {
        printf("%.2X", _ret_zid.id[i]);
    }
    printf("\n");

#ifdef ZENOH_PICO
    zp_start_read_task(z_loan(s2), NULL);
    zp_start_lease_task(z_loan(s2), NULL);
#endif

    sleep(SLEEP);

    z_session_t ls1 = z_loan(s1);
    printf("Declaring Subscriber...");
    z_owned_closure_sample_t _ret_closure_sample = z_closure(data_handler, NULL, &ls1);
    z_subscriber_options_t _ret_sub_opt = z_subscriber_options_default();
    z_owned_subscriber_t _ret_sub =
        z_declare_subscriber(z_loan(s2), z_keyexpr(keyexpr_str), z_move(_ret_closure_sample), &_ret_sub_opt);
    assert(z_check(_ret_sub));
    printf("Ok\n");

    sleep(SLEEP);

    printf("Declaring Keyexpr...");
    char *s1_res = (char *)malloc(64);
    snprintf(s1_res, 64, "%s/chunk/%d", keyexpr_str, 1);
    z_owned_keyexpr_t _ret_expr = z_declare_keyexpr(z_loan(s1), z_keyexpr(s1_res));
    assert(z_check(_ret_expr));
    printf("Ok\n");

    printf("Session Put...");
    z_put_options_t _ret_put_opt = z_put_options_default();
    _ret_put_opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
    z_encoding_t _ret_encoding = z_encoding_default();
    (void)(_ret_encoding);
    _ret_encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    _ret_put_opt.encoding = _ret_encoding;
    _ret_int8 = z_put(z_loan(s1), z_loan(_ret_expr), (const uint8_t *)value, strlen(value), &_ret_put_opt);
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert(datas == 1);

    printf("Session delete...");
    z_delete_options_t _ret_delete_opt = z_delete_options_default();
    _ret_int8 = z_delete(z_loan(s1), z_loan(_ret_expr), &_ret_delete_opt);
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert(datas == 2);

    printf("Undeclaring Keyexpr...");
    _ret_int8 = z_undeclare_keyexpr(z_loan(s1), z_move(_ret_expr));
    assert(_ret_int8 == 0);
    assert(!z_check(_ret_expr));
    printf("Ok\n");

    _ret_int8 = z_undeclare_subscriber(z_move(_ret_sub));
    assert(_ret_int8 == 0);

    printf("Declaring Pull Subscriber...");
    z_owned_closure_sample_t _ret_closure_sample2 = z_closure(data_handler, NULL, &ls1);
    z_pull_subscriber_options_t _ret_psub_opt = z_pull_subscriber_options_default();
    z_owned_pull_subscriber_t _ret_psub =
        z_declare_pull_subscriber(z_loan(s2), z_keyexpr(keyexpr_str), z_move(_ret_closure_sample2), &_ret_psub_opt);
    assert(z_check(_ret_psub));
    printf("Ok\n");

    printf("Declaring Publisher...");
    z_publisher_options_t _ret_pub_opt = z_publisher_options_default();
    _ret_pub_opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
    z_owned_publisher_t _ret_pub = z_declare_publisher(z_loan(s1), z_keyexpr(keyexpr_str), &_ret_pub_opt);
    assert(z_check(_ret_pub));
    printf("Ok\n");

    sleep(SLEEP);

    printf("Publisher Put...");
    z_publisher_put_options_t _ret_pput_opt = z_publisher_put_options_default();
    _ret_int8 = z_publisher_put(z_loan(_ret_pub), (const uint8_t *)value, strlen(value), &_ret_pput_opt);
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP);

    printf("Pull Subscriber Pulling data...");
    _ret_int8 = z_subscriber_pull(z_loan(_ret_psub));
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert(datas == 3);

    printf("Publisher Delete...");
    z_publisher_delete_options_t _ret_pdelete_opt = z_publisher_delete_options_default();
    _ret_int8 = z_publisher_delete(z_loan(_ret_pub), &_ret_pdelete_opt);
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP);

    printf("Pull Subscriber Pulling data...");
    _ret_int8 = z_subscriber_pull(z_loan(_ret_psub));
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert(datas == 4);

    printf("Undeclaring Publisher...");
    _ret_int8 = z_undeclare_publisher(z_move(_ret_pub));
    assert(_ret_int8 == 0);
    assert(!z_check(_ret_pub));
    printf("Ok\n");

    printf("Undeclaring Pull Subscriber...");
    _ret_int8 = z_undeclare_pull_subscriber(z_move(_ret_psub));
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP);

    printf("Declaring Queryable...");
    z_owned_closure_query_t _ret_closure_query = z_closure(query_handler, NULL, &ls1);
    z_queryable_options_t _ret_qle_opt = z_queryable_options_default();
    z_owned_queryable_t qle =
        z_declare_queryable(z_loan(s1), z_keyexpr(s1_res), z_move(_ret_closure_query), &_ret_qle_opt);
    assert(z_check(qle));
    printf("Ok\n");

    sleep(SLEEP);

    printf("Testing Consolidations...");
    z_session_t ls2 = z_loan(s2);
    z_owned_closure_reply_t _ret_closure_reply = z_closure(reply_handler, NULL, &ls2);
    z_get_options_t _ret_get_opt = z_get_options_default();
    _ret_get_opt.target = z_query_target_default();
    _ret_get_opt.consolidation = z_query_consolidation_auto();
    (void)(_ret_get_opt.consolidation);
    _ret_get_opt.consolidation = z_query_consolidation_default();
    (void)(_ret_get_opt.consolidation);
    _ret_get_opt.consolidation = z_query_consolidation_latest();
    (void)(_ret_get_opt.consolidation);
    _ret_get_opt.consolidation = z_query_consolidation_monotonic();
    (void)(_ret_get_opt.consolidation);
    _ret_get_opt.consolidation = z_query_consolidation_none();
    (void)(_ret_get_opt.consolidation);
    printf("Ok\n");

    printf("Testing Get...");
    _ret_int8 = z_get(z_loan(s2), z_keyexpr(s1_res), "", z_move(_ret_closure_reply), &_ret_get_opt);
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert(queries == 1);
    assert(replies == 1);

    printf("Undeclaring Queryable...");
    _ret_int8 = z_undeclare_queryable(z_move(qle));
    assert(_ret_int8 == 0);
    printf("Ok\n");

#ifdef ZENOH_PICO
    zp_stop_read_task(z_loan(s1));
    zp_stop_lease_task(z_loan(s1));
#endif

    printf("Close sessions...");
    _ret_int8 = z_close(z_move(s1));
    assert(_ret_int8 == 0);

#ifdef ZENOH_PICO
    zp_stop_read_task(z_loan(s2));
    zp_stop_lease_task(z_loan(s2));
#endif
    _ret_int8 = z_close(z_move(s2));
    assert(_ret_int8 == 0);
    printf("Ok\n");

    sleep(SLEEP * 5);

    free(s1_res);
    free(keyexpr_str);

    return 0;
}
