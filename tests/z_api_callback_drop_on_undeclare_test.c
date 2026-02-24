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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zenoh-pico.h"
#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/liveliness.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#ifdef Z_ADVANCED_PUBSUB_TEST_USE_TCP_PROXY
#include "utils/tcp_proxy.h"
#endif

#undef NDEBUG
#include <assert.h>

typedef struct callback_arg_t {
    volatile bool called;
    volatile bool dropped;
} callback_arg_t;

#if Z_FEATURE_MULTI_THREAD == 1
typedef struct thread_arg_t {
    z_owned_session_t* session;
    z_view_keyexpr_t* ke;
} thread_arg_t;
#endif

z_result_t open_session(z_owned_session_t* s) {
    z_owned_config_t config;
    _Z_RETURN_IF_ERR(z_config_default(&config));
    return z_open(s, z_config_move(&config), NULL);
}

#define DECLARE_ON_RECEIVE_HANDLER(_type, cv)                     \
    void on_receive_##_type##_handler(cv _type* msg, void* arg) { \
        (void)msg;                                                \
        callback_arg_t* callback_arg = (callback_arg_t*)arg;      \
        z_sleep_s(3);                                             \
        callback_arg->called = true;                              \
    }                                                             \
    void on_drop_##_type##_handler(void* arg) {                   \
        callback_arg_t* callback_arg = (callback_arg_t*)arg;      \
        z_sleep_s(1);                                             \
        callback_arg->dropped = true;                             \
    }

#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1
#if Z_FEATURE_MULTI_THREAD == 1
void* put_from_another_thread(void* targ) {
    thread_arg_t* arg = (thread_arg_t*)targ;
    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, "payload");
    assert(z_put(z_session_loan(arg->session), z_view_keyexpr_loan(arg->ke), z_bytes_move(&payload), NULL) == Z_OK);
    return NULL;
}
#endif

DECLARE_ON_RECEIVE_HANDLER(z_loaned_sample_t, _ZP_NOTHING)
void test_subscriber_callback_drop_on_undeclare(bool background) {
    z_owned_session_t session1, session2;
    z_owned_subscriber_t subscriber;
    z_view_keyexpr_t ke;
    z_owned_closure_sample_t closure;

    assert(open_session(&session1) == Z_OK);
    assert(open_session(&session2) == Z_OK);

    if (!background) {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare/subscriber_callback_drop") == Z_OK);
    } else {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare_background/subscriber_callback_drop") == Z_OK);
    }

    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;

    printf("Test: Subscriber callback drop on undeclare: background = %d\n", background);
    z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler, (void*)&arg);
    if (!background) {
        assert(z_declare_subscriber(z_session_loan(&session1), &subscriber, z_view_keyexpr_loan(&ke),
                                    z_closure_sample_move(&closure), NULL) == Z_OK);
    } else {
        assert(z_declare_background_subscriber(z_session_loan(&session1), z_view_keyexpr_loan(&ke),
                                               z_closure_sample_move(&closure), NULL) == Z_OK);
    }
    z_sleep_s(2);

    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, "payload");
    assert(z_put(z_session_loan(&session2), z_view_keyexpr_loan(&ke), z_bytes_move(&payload), NULL) == Z_OK);
    z_sleep_s(1);
    if (!background) {
        z_undeclare_subscriber(z_subscriber_move(&subscriber));
    } else {
        z_close(z_session_loan_mut(&session1), NULL);
    }
    assert(arg.called == true);
    assert(arg.dropped == true);

#if Z_FEATURE_LOCAL_SUBSCRIBER == 1 && Z_FEATURE_MULTI_THREAD == 1
    printf("Test: Local Subscriber callback drop on undeclare: background = %d\n", background);
    arg.called = false;
    arg.dropped = false;
    z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler, (void*)&arg);
    if (!background) {
        assert(z_declare_subscriber(z_session_loan(&session2), &subscriber, z_view_keyexpr_loan(&ke),
                                    z_closure_sample_move(&closure), NULL) == Z_OK);
    } else {
        assert(z_declare_background_subscriber(z_session_loan(&session2), z_view_keyexpr_loan(&ke),
                                               z_closure_sample_move(&closure), NULL) == Z_OK);
    }
    thread_arg_t thread_arg = {
        .session = &session2,
        .ke = &ke,
    };
    z_owned_task_t task;
    z_task_init(&task, NULL, put_from_another_thread, &thread_arg);
    z_sleep_s(1);
    if (!background) {
        z_undeclare_subscriber(z_subscriber_move(&subscriber));
    } else {
        z_close(z_session_loan_mut(&session2), NULL);
    }
    assert(arg.called == true);
    assert(arg.dropped == true);
    z_task_join(z_task_move(&task));
#endif
    z_session_drop(z_session_move(&session1));
    z_session_drop(z_session_move(&session2));
}

#if Z_FEATURE_LIVELINESS == 1
void test_liveliness_subscriber_callback_drop_on_undeclare(bool background) {
    z_owned_session_t session1, session2;
    z_owned_subscriber_t subscriber;
    z_view_keyexpr_t ke;
    z_owned_closure_sample_t closure;

    assert(open_session(&session1) == Z_OK);
    assert(open_session(&session2) == Z_OK);

    if (!background) {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare/liveliness_subscriber_callback_drop") == Z_OK);
    } else {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare_background/liveliness_subscriber_callback_drop") == Z_OK);
    }

    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;

    printf("Test: Liveliness Subscriber callback drop on undeclare: background = %d\n", background);
    z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler, (void*)&arg);
    if (!background) {
        assert(z_liveliness_declare_subscriber(z_session_loan(&session1), &subscriber, z_view_keyexpr_loan(&ke),
                                               z_closure_sample_move(&closure), NULL) == Z_OK);
    } else {
        assert(z_liveliness_declare_background_subscriber(z_session_loan(&session1), z_view_keyexpr_loan(&ke),
                                                          z_closure_sample_move(&closure), NULL) == Z_OK);
    }
    z_sleep_s(2);

    z_owned_liveliness_token_t token;
    assert(z_liveliness_declare_token(z_session_loan(&session2), &token, z_view_keyexpr_loan(&ke), NULL) == Z_OK);
    z_sleep_s(1);

    if (!background) {
        z_undeclare_subscriber(z_subscriber_move(&subscriber));
    } else {
        z_close(z_session_loan_mut(&session1), NULL);
    }
    assert(arg.called == true);
    assert(arg.dropped == true);

    z_liveliness_token_drop(z_liveliness_token_move(&token));
    z_session_drop(z_session_move(&session1));
    z_session_drop(z_session_move(&session2));
}
#endif

#if Z_FEATURE_ADVANCED_SUBSCRIPTION == 1 && Z_FEATURE_ADVANCED_PUBLICATION == 1
void test_advanced_subscriber_callback_drop_on_undeclare(bool background) {
    z_owned_session_t session1, session2;
    ze_owned_advanced_subscriber_t subscriber;
    z_view_keyexpr_t ke;
    z_owned_closure_sample_t closure;

    assert(open_session(&session1) == Z_OK);
    assert(open_session(&session2) == Z_OK);

    if (!background) {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare/advanced_subscriber_callback_drop") == Z_OK);
    } else {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare_background/advanced_subscriber_callback_drop") == Z_OK);
    }

    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;

    printf("Test: Advanced Subscriber callback drop on undeclare: background = %d\n", background);
    z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler, (void*)&arg);
    if (!background) {
        assert(ze_declare_advanced_subscriber(z_session_loan(&session1), &subscriber, z_view_keyexpr_loan(&ke),
                                              z_closure_sample_move(&closure), NULL) == Z_OK);
    } else {
        assert(ze_declare_background_advanced_subscriber(z_session_loan(&session1), z_view_keyexpr_loan(&ke),
                                                         z_closure_sample_move(&closure), NULL) == Z_OK);
    }
    z_sleep_s(2);

    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, "payload");
    assert(z_put(z_session_loan(&session2), z_view_keyexpr_loan(&ke), z_bytes_move(&payload), NULL) == Z_OK);
    z_sleep_s(1);
    if (!background) {
        ze_undeclare_advanced_subscriber(ze_advanced_subscriber_move(&subscriber));
    } else {
        z_close(z_session_loan_mut(&session1), NULL);
    }
    assert(arg.dropped == true);
    assert(arg.called == true);
    z_session_drop(z_session_move(&session1));

#if Z_FEATURE_LOCAL_SUBSCRIBER == 1 && Z_FEATURE_MULTI_THREAD == 1
    printf("Test: Local Advanced Subscriber callback drop on undeclare: background = %d\n", background);
    arg.called = false;
    arg.dropped = false;
    z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler, (void*)&arg);
    if (!background) {
        assert(ze_declare_advanced_subscriber(z_session_loan(&session2), &subscriber, z_view_keyexpr_loan(&ke),
                                              z_closure_sample_move(&closure), NULL) == Z_OK);
    } else {
        assert(ze_declare_background_advanced_subscriber(z_session_loan(&session2), z_view_keyexpr_loan(&ke),
                                                         z_closure_sample_move(&closure), NULL) == Z_OK);
    }
    thread_arg_t thread_arg = {
        .session = &session2,
        .ke = &ke,
    };
    z_owned_task_t task;
    z_task_init(&task, NULL, put_from_another_thread, &thread_arg);
    z_sleep_s(1);
    if (!background) {
        ze_undeclare_advanced_subscriber(ze_advanced_subscriber_move(&subscriber));
    } else {
        z_close(z_session_loan_mut(&session2), NULL);
    }

    assert(arg.called == true);
    assert(arg.dropped == true);
    z_task_join(z_task_move(&task));
#endif
    z_session_drop(z_session_move(&session1));
    z_session_drop(z_session_move(&session2));
}

void test_advanced_subscriber_late_join_callback_drop_on_undeclare(bool background) {
    z_owned_session_t session1, session2;
    z_view_keyexpr_t ke;
    z_owned_closure_sample_t closure;

    assert(open_session(&session1) == Z_OK);
    assert(open_session(&session2) == Z_OK);
    if (!background) {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare/late_advanced_subscriber_callback_drop") == Z_OK);
    } else {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare_background/late_advanced_subscriber_callback_drop") ==
               Z_OK);
    }
    printf("Test: Advanced Subscriber late join callback drop on undeclare: background = %d\n", background);

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    ze_advanced_publisher_cache_options_default(&pub_opts.cache);
    pub_opts.cache.max_samples = 10;
    pub_opts.publisher_detection = true;
    assert(ze_declare_advanced_publisher(z_loan(session2), &pub, z_view_keyexpr_loan(&ke), &pub_opts) == Z_OK);
    z_sleep_s(1);

    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, "payload");
    ze_advanced_publisher_put(z_loan(pub), z_bytes_move(&payload), NULL);

    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;
    z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler, (void*)&arg);
    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    ze_advanced_subscriber_history_options_default(&sub_opts.history);
    sub_opts.history.detect_late_publishers = true;
    if (!background) {
        assert(ze_declare_advanced_subscriber(z_session_loan(&session1), &sub, z_view_keyexpr_loan(&ke),
                                              z_closure_sample_move(&closure), &sub_opts) == Z_OK);
    } else {
        assert(ze_declare_background_advanced_subscriber(z_session_loan(&session1), z_view_keyexpr_loan(&ke),
                                                         z_closure_sample_move(&closure), &sub_opts) == Z_OK);
    }

    z_sleep_s(1);
    if (!background) {
        ze_undeclare_advanced_subscriber(ze_advanced_subscriber_move(&sub));
    } else {
        z_close(z_session_loan_mut(&session1), NULL);
    }
    assert(arg.called == true);
    assert(arg.dropped == true);
    ze_advanced_publisher_drop(ze_advanced_publisher_move(&pub));
    z_session_drop(z_session_move(&session1));
    z_session_drop(z_session_move(&session2));
}

#if Z_ADVANCED_PUBSUB_TEST_USE_TCP_PROXY == 1
DECLARE_ON_RECEIVE_HANDLER(ze_miss_t, const)
void put_str(const ze_loaned_advanced_publisher_t* pub, const char* str) {
    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, str);
    ze_advanced_publisher_put(pub, z_bytes_move(&payload), NULL);
}
void setup_two_peers_with_proxy(z_owned_session_t* s1, z_owned_session_t* s2, tcp_proxy_t** proxy,
                                uint16_t upstream_listen_port) {
    *proxy = tcp_proxy_create("127.0.0.1", "127.0.0.1", upstream_listen_port);
    assert(*proxy != NULL);

    int port = tcp_proxy_start(*proxy, 100);
    assert(port > 0);

    z_owned_config_t c1, c2;
    z_config_default(&c1);
    z_config_default(&c2);

    char uri[128];

    // s1 listens on the fixed upstream port
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_MODE_KEY, "peer");
    snprintf(uri, sizeof(uri), "tcp/127.0.0.1:%u#iface=lo", (unsigned)upstream_listen_port);
    zp_config_insert(z_loan_mut(c1), Z_CONFIG_LISTEN_KEY, uri);

    // s2 connects via proxy ephemeral port
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_MODE_KEY, "peer");
    snprintf(uri, sizeof(uri), "tcp/127.0.0.1:%u#iface=lo", (unsigned)port);
    zp_config_insert(z_loan_mut(c2), Z_CONFIG_CONNECT_KEY, uri);

    assert(z_open(s1, z_config_move(&c1), NULL) == Z_OK);
    assert(z_open(s2, z_config_move(&c2), NULL) == Z_OK);
}

void test_advanced_sample_miss_callback_drop_on_undeclare(bool background) {
    z_owned_session_t session1, session2;
    z_view_keyexpr_t ke;

    if (!background) {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare/sample_miss_listener_callback_drop") == Z_OK);
    } else {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare_background/sample_miss_listener_callback_drop") == Z_OK);
    }
    printf("Test: Sample Miss Listener callback drop on undeclare: background = %d\n", background);

    tcp_proxy_t* tcp_proxy;
    setup_two_peers_with_proxy(&session1, &session2, &tcp_proxy, 9000);

    ze_owned_advanced_subscriber_t sub;
    ze_advanced_subscriber_options_t sub_opts;
    ze_advanced_subscriber_options_default(&sub_opts);
    z_owned_closure_sample_t closure;
    z_owned_fifo_handler_sample_t handler;
    assert(z_fifo_channel_sample_new(&closure, &handler, 10) == Z_OK);
    assert(ze_declare_advanced_subscriber(z_session_loan(&session2), &sub, z_loan(ke), z_move(closure), &sub_opts) ==
           Z_OK);

    ze_owned_sample_miss_listener_t miss_listener;
    ze_owned_closure_miss_t miss_closure;
    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;
    ze_closure_miss(&miss_closure, on_receive_ze_miss_t_handler, on_drop_ze_miss_t_handler, (void*)&arg);
    if (!background) {
        assert(ze_advanced_subscriber_declare_sample_miss_listener(ze_advanced_subscriber_loan(&sub), &miss_listener,
                                                                   ze_closure_miss_move(&miss_closure)) == Z_OK);
    } else {
        assert(ze_advanced_subscriber_declare_background_sample_miss_listener(
                   ze_advanced_subscriber_loan(&sub), ze_closure_miss_move(&miss_closure)) == Z_OK);
    }
    z_sleep_s(1);

    ze_owned_advanced_publisher_t pub;
    ze_advanced_publisher_options_t pub_opts;
    ze_advanced_publisher_options_default(&pub_opts);
    ze_advanced_publisher_sample_miss_detection_options_default(&pub_opts.sample_miss_detection);
    assert(ze_declare_advanced_publisher(z_session_loan(&session1), &pub, z_loan(ke), &pub_opts) == Z_OK);

    put_str(z_loan(pub), "1");
    z_sleep_ms(500);
    put_str(z_loan(pub), "2");
    z_sleep_ms(500);

    tcp_proxy_set_enabled(tcp_proxy, false);
    z_sleep_ms(500);

    put_str(z_loan(pub), "3");
    z_sleep_ms(500);

    tcp_proxy_set_enabled(tcp_proxy, true);
    z_sleep_s(1);

    put_str(z_loan(pub), "4");
    z_sleep_ms(500);

    if (!background) {
        ze_undeclare_sample_miss_listener(ze_sample_miss_listener_move(&miss_listener));
    } else {
        ze_undeclare_advanced_subscriber(ze_advanced_subscriber_move(&sub));
    }
    assert(arg.dropped == true);
    assert(arg.called == true);

    z_fifo_handler_sample_drop(z_fifo_handler_sample_move(&handler));
    if (!background) {
        ze_advanced_subscriber_drop(ze_advanced_subscriber_move(&sub));
    }
    ze_advanced_publisher_drop(ze_advanced_publisher_move(&pub));
    z_session_drop(z_session_move(&session1));
    z_session_drop(z_session_move(&session2));

    tcp_proxy_stop(tcp_proxy);
    tcp_proxy_destroy(tcp_proxy);
}
#endif
#endif

#if Z_FEATURE_MATCHING == 1
#if Z_FEATURE_MULTI_THREAD == 1
typedef struct sub_thread_arg_t {
    z_owned_session_t* session;
    z_view_keyexpr_t* ke;
    z_owned_subscriber_t* sub;
    z_owned_closure_sample_t* closure;
    z_owned_fifo_handler_sample_t* handler;
} sub_thread_arg_t;

void* declare_sub_from_another_thread(void* targ) {
    sub_thread_arg_t* arg = (sub_thread_arg_t*)targ;
    z_owned_closure_sample_t sample_closure;
    z_owned_fifo_handler_sample_t sample_handler;
    assert(z_fifo_channel_sample_new(&sample_closure, &sample_handler, 3) == Z_OK);
    assert(z_declare_subscriber(z_session_loan(arg->session), arg->sub, z_view_keyexpr_loan(arg->ke),
                                z_closure_sample_move(&sample_closure), NULL) == Z_OK);
    *(arg->handler) = sample_handler;
    return NULL;
}
#endif
DECLARE_ON_RECEIVE_HANDLER(z_matching_status_t, const)
void test_matching_callback_drop_on_undeclare(bool background) {
    z_owned_session_t session1, session2;
    z_view_keyexpr_t ke;
    z_owned_publisher_t pub;
    z_owned_subscriber_t sub;
    z_owned_matching_listener_t listener;

    assert(open_session(&session1) == Z_OK);
    assert(open_session(&session2) == Z_OK);

    printf("Test: Matching callback drop on undeclare: background = %d\n", background);
    if (!background) {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare/matching_callback_drop") == Z_OK);
    } else {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare_background/matching_callback_drop") == Z_OK);
    }

    assert(z_declare_publisher(z_session_loan(&session1), &pub, z_view_keyexpr_loan(&ke), NULL) == Z_OK);
    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;
    z_owned_closure_matching_status_t closure;
    z_closure_matching_status(&closure, on_receive_z_matching_status_t_handler, on_drop_z_matching_status_t_handler,
                              (void*)&arg);
    if (!background) {
        assert(z_publisher_declare_matching_listener(z_publisher_loan(&pub), &listener,
                                                     z_closure_matching_status_move(&closure)) == Z_OK);
    } else {
        assert(z_publisher_declare_background_matching_listener(z_publisher_loan(&pub),
                                                                z_closure_matching_status_move(&closure)) == Z_OK);
    }
    z_sleep_s(1);
    z_owned_closure_sample_t sample_closure;
    z_owned_fifo_handler_sample_t sample_handler;
    assert(z_fifo_channel_sample_new(&sample_closure, &sample_handler, 3) == Z_OK);
    assert(z_declare_subscriber(z_session_loan(&session2), &sub, z_view_keyexpr_loan(&ke),
                                z_closure_sample_move(&sample_closure), NULL) == Z_OK);
    z_sleep_s(1);

    if (!background) {
        z_matching_listener_drop(z_matching_listener_move(&listener));
    } else {
        z_publisher_drop(z_publisher_move(&pub));
    }
    assert(arg.dropped == true);
    assert(arg.called == true);
    z_publisher_drop(z_publisher_move(&pub));
    z_session_drop(z_session_move(&session1));
    z_subscriber_drop(z_subscriber_move(&sub));
    z_fifo_handler_sample_drop(z_fifo_handler_sample_move(&sample_handler));
#if Z_FEATURE_LOCAL_SUBSCRIBER == 1 && Z_FEATURE_MULTI_THREAD == 1
    printf("Test: Local Matching callback drop on undeclare: background = %d\n", background);

    assert(z_declare_publisher(z_session_loan(&session2), &pub, z_view_keyexpr_loan(&ke), NULL) == Z_OK);
    arg.called = false;
    arg.dropped = false;
    z_closure_matching_status(&closure, on_receive_z_matching_status_t_handler, on_drop_z_matching_status_t_handler,
                              (void*)&arg);
    if (!background) {
        assert(z_publisher_declare_matching_listener(z_publisher_loan(&pub), &listener,
                                                     z_closure_matching_status_move(&closure)) == Z_OK);
    } else {
        assert(z_publisher_declare_background_matching_listener(z_publisher_loan(&pub),
                                                                z_closure_matching_status_move(&closure)) == Z_OK);
    }

    // Declare subscriber from another thread to avoid blocking on the matching callback
    sub_thread_arg_t sub_thread_arg = {
        .session = &session2,
        .ke = &ke,
        .sub = &sub,
        .closure = NULL,
        .handler = &sample_handler,
    };
    z_owned_task_t task;
    z_task_init(&task, NULL, declare_sub_from_another_thread, &sub_thread_arg);
    z_sleep_s(1);

    if (!background) {
        z_matching_listener_drop(z_matching_listener_move(&listener));
    } else {
        z_publisher_drop(z_publisher_move(&pub));
    }
    assert(arg.called == true);
    assert(arg.dropped == true);
    z_task_join(z_task_move(&task));
    z_publisher_drop(z_publisher_move(&pub));
    z_subscriber_drop(z_subscriber_move(&sub));
    z_fifo_handler_sample_drop(z_fifo_handler_sample_move(&sample_handler));
#endif
    z_session_drop(z_session_move(&session2));
}
#endif
#endif

#if Z_FEATURE_QUERYABLE == 1 && Z_FEATURE_QUERY == 1
#if Z_FEATURE_MULTI_THREAD == 1
void* get_from_another_thread(void* targ) {
    thread_arg_t* arg = (thread_arg_t*)targ;
    z_owned_closure_reply_t callback_reply;
    z_owned_fifo_handler_reply_t fifo_reply;
    assert(z_fifo_channel_reply_new(&callback_reply, &fifo_reply, 3) == Z_OK);
    assert(z_get(z_session_loan(arg->session), z_view_keyexpr_loan(arg->ke), "", z_closure_reply_move(&callback_reply),
                 NULL) == Z_OK);
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&fifo_reply));
    return NULL;
}
#endif
DECLARE_ON_RECEIVE_HANDLER(z_loaned_query_t, _ZP_NOTHING)
void test_queryable_callback_drop_on_undeclare(bool background) {
    z_owned_session_t session1, session2;
    z_owned_queryable_t queryable;
    z_view_keyexpr_t ke;
    z_owned_closure_query_t closure;

    assert(open_session(&session1) == Z_OK);
    assert(open_session(&session2) == Z_OK);

    if (!background) {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare/queryable_callback_drop") == Z_OK);
    } else {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare_background/queryable_callback_drop") == Z_OK);
    }

    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;

    printf("Test: Queryable callback drop on undeclare: background = %d\n", background);
    z_closure_query(&closure, on_receive_z_loaned_query_t_handler, on_drop_z_loaned_query_t_handler, (void*)&arg);
    if (!background) {
        assert(z_declare_queryable(z_session_loan(&session1), &queryable, z_view_keyexpr_loan(&ke),
                                   z_closure_query_move(&closure), NULL) == Z_OK);
    } else {
        assert(z_declare_background_queryable(z_session_loan(&session1), z_view_keyexpr_loan(&ke),
                                              z_closure_query_move(&closure), NULL) == Z_OK);
    }
    z_sleep_s(2);

    z_owned_closure_reply_t callback_reply;
    z_owned_fifo_handler_reply_t fifo_reply;
    assert(z_fifo_channel_reply_new(&callback_reply, &fifo_reply, 3) == Z_OK);
    assert(z_get(z_session_loan(&session2), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&callback_reply),
                 NULL) == Z_OK);
    z_sleep_s(1);
    if (!background) {
        z_undeclare_queryable(z_queryable_move(&queryable));
    } else {
        z_close(z_session_loan_mut(&session1), NULL);
    }
    assert(arg.called == true);
    assert(arg.dropped == true);

#if Z_FEATURE_LOCAL_QUERYABLE == 1 && Z_FEATURE_MULTI_THREAD == 1
    printf("Test: Local Queryable callback drop on undeclare, background = %d\n", background);
    arg.called = false;
    arg.dropped = false;
    z_closure_query(&closure, on_receive_z_loaned_query_t_handler, on_drop_z_loaned_query_t_handler, (void*)&arg);
    if (!background) {
        assert(z_declare_queryable(z_session_loan(&session2), &queryable, z_view_keyexpr_loan(&ke),
                                   z_closure_query_move(&closure), NULL) == Z_OK);
    } else {
        assert(z_declare_background_queryable(z_session_loan(&session2), z_view_keyexpr_loan(&ke),
                                              z_closure_query_move(&closure), NULL) == Z_OK);
    }
    thread_arg_t thread_arg = {
        .session = &session2,
        .ke = &ke,
    };
    z_owned_task_t task;
    z_task_init(&task, NULL, get_from_another_thread, &thread_arg);
    z_sleep_s(1);
    if (!background) {
        z_undeclare_queryable(z_queryable_move(&queryable));
    } else {
        z_close(z_session_loan_mut(&session2), NULL);
    }
    assert(arg.called == true);
    assert(arg.dropped == true);
    z_task_join(z_task_move(&task));
#endif
    z_session_drop(z_session_move(&session1));
    z_session_drop(z_session_move(&session2));
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&fifo_reply));
}

#if Z_FEATURE_MULTI_THREAD == 1
typedef struct query_thread_arg_t {
    z_owned_fifo_handler_query_t* receiver;
    z_view_keyexpr_t* ke;
} query_thread_arg_t;

void* reply_from_another_thread(void* targ) {
    query_thread_arg_t* arg = (query_thread_arg_t*)targ;
    z_owned_query_t query;
    assert(z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(arg->receiver), &query) == Z_OK);

    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, "payload");
    z_query_reply(z_query_loan(&query), z_view_keyexpr_loan(arg->ke), z_bytes_move(&payload), NULL);
    z_query_drop(z_query_move(&query));
    z_fifo_handler_query_drop(z_fifo_handler_query_move(arg->receiver));
    return NULL;
}
#endif

DECLARE_ON_RECEIVE_HANDLER(z_loaned_reply_t, _ZP_NOTHING)
void test_querier_callback_drop_on_undeclare(bool background) {
    z_owned_session_t session1, session2;
    z_owned_queryable_t queryable;
    z_owned_querier_t querier;
    z_view_keyexpr_t ke;

    assert(open_session(&session1) == Z_OK);
    assert(open_session(&session2) == Z_OK);

    if (!background) {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare/querier_callback_drop") == Z_OK);
    } else {
        assert(z_view_keyexpr_from_str(&ke, "test/undeclare_background/querier_callback_drop") == Z_OK);
    }

    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;

    printf("Test: Querier callback drop on undeclare: background = %d\n", background);
    z_owned_closure_query_t query_closure;
    z_owned_fifo_handler_query_t query_handler;
    z_fifo_channel_query_new(&query_closure, &query_handler, 3);
    assert(z_declare_queryable(z_session_loan(&session2), &queryable, z_view_keyexpr_loan(&ke),
                               z_closure_query_move(&query_closure), NULL) == Z_OK);

    z_owned_closure_reply_t reply_closure;
    z_closure_reply(&reply_closure, on_receive_z_loaned_reply_t_handler, on_drop_z_loaned_reply_t_handler, (void*)&arg);
    if (!background) {
        assert(z_declare_querier(z_session_loan(&session1), &querier, z_view_keyexpr_loan(&ke), NULL) == Z_OK);
        z_sleep_s(2);
        assert(z_querier_get(z_querier_loan(&querier), "", z_closure_reply_move(&reply_closure), NULL) == Z_OK);
    } else {
        z_sleep_s(2);
        assert(z_get(z_session_loan(&session1), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&reply_closure),
                     NULL) == Z_OK);
    }

    z_owned_query_t query;
    z_sleep_s(1);
    assert(z_fifo_handler_query_try_recv(z_fifo_handler_query_loan(&query_handler), &query) == Z_OK);
    z_owned_bytes_t payload;
    z_bytes_from_static_str(&payload, "payload");
    assert(z_query_reply(z_query_loan(&query), z_view_keyexpr_loan(&ke), z_bytes_move(&payload), NULL) == Z_OK);
    z_query_drop(z_query_move(&query));
    z_sleep_s(1);

    if (!background) {
        z_undeclare_querier(z_querier_move(&querier));
    } else {
        z_close(z_session_loan_mut(&session1), NULL);
    }
    assert(arg.called == true);
    assert(arg.dropped == true);
    z_undeclare_queryable(z_queryable_move(&queryable));
    z_fifo_handler_query_drop(z_fifo_handler_query_move(&query_handler));

#if Z_FEATURE_LOCAL_QUERYABLE == 1 && Z_FEATURE_MULTI_THREAD == 1
    printf("Test: Local Querier callback drop on undeclare: background = %d\n", background);
    arg.called = false;
    arg.dropped = false;
    z_fifo_channel_query_new(&query_closure, &query_handler, 3);
    assert(z_declare_queryable(z_session_loan(&session2), &queryable, z_view_keyexpr_loan(&ke),
                               z_closure_query_move(&query_closure), NULL) == Z_OK);
    z_closure_reply(&reply_closure, on_receive_z_loaned_reply_t_handler, on_drop_z_loaned_reply_t_handler, (void*)&arg);
    if (!background) {
        assert(z_declare_querier(z_session_loan(&session2), &querier, z_view_keyexpr_loan(&ke), NULL) == Z_OK);
        assert(z_querier_get(z_querier_loan(&querier), "", z_closure_reply_move(&reply_closure), NULL) == Z_OK);
    } else {
        assert(z_get(z_session_loan(&session2), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&reply_closure),
                     NULL) == Z_OK);
    }

    query_thread_arg_t thread_arg = {
        .receiver = &query_handler,
        .ke = &ke,
    };
    z_owned_task_t task;
    z_task_init(&task, NULL, reply_from_another_thread, &thread_arg);
    z_sleep_s(1);
    if (!background) {
        z_undeclare_querier(z_querier_move(&querier));
    } else {
        z_close(z_session_loan_mut(&session2), NULL);
    }
    assert(arg.called == true);
    assert(arg.dropped == true);
    z_task_join(z_task_move(&task));
    z_undeclare_queryable(z_queryable_move(&queryable));
#endif
    z_session_drop(z_session_move(&session1));
    z_session_drop(z_session_move(&session2));
}

#if Z_FEATURE_LIVELINESS == 1
void test_liveliness_get_callback_drop_on_undeclare(void) {
    z_owned_session_t session1, session2;
    z_view_keyexpr_t ke;
    z_owned_closure_reply_t closure;

    assert(open_session(&session1) == Z_OK);
    assert(open_session(&session2) == Z_OK);
    assert(z_view_keyexpr_from_str(&ke, "test/undeclare/liveliness_get_callback_drop") == Z_OK);

    callback_arg_t arg;
    arg.called = false;
    arg.dropped = false;

    printf("Test: Liveliness Get callback drop on undeclare\n");
    z_closure_reply(&closure, on_receive_z_loaned_reply_t_handler, on_drop_z_loaned_reply_t_handler, (void*)&arg);
    z_owned_liveliness_token_t token;
    assert(z_liveliness_declare_token(z_session_loan(&session2), &token, z_view_keyexpr_loan(&ke), NULL) == Z_OK);
    z_sleep_s(1);
    assert(z_liveliness_get(z_session_loan(&session1), z_view_keyexpr_loan(&ke), z_closure_reply_move(&closure),
                            NULL) == Z_OK);
    z_sleep_s(1);
    z_close(z_session_loan_mut(&session1), NULL);

    assert(arg.called == true);
    assert(arg.dropped == true);

    z_liveliness_token_drop(z_liveliness_token_move(&token));
    z_session_drop(z_session_move(&session1));
    z_session_drop(z_session_move(&session2));
}
#endif

#endif

void test_no_declare_after_session_close(void) {
#if Z_FEATURE_SUBSCRIPTION == 1
    {
        printf("Test: No subscriber can be declared after session close\n");
        z_owned_session_t session;
        z_owned_subscriber_t subscriber;
        z_view_keyexpr_t ke;
        z_owned_closure_sample_t closure;

        callback_arg_t arg;
        arg.called = false;
        arg.dropped = false;

        assert(open_session(&session) == Z_OK);
        assert(z_view_keyexpr_from_str(&ke, "test/closed/subscriber") == Z_OK);
        z_close(z_session_loan_mut(&session), NULL);

        z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler,
                         (void*)&arg);
        assert(z_declare_subscriber(z_session_loan(&session), &subscriber, z_view_keyexpr_loan(&ke),
                                    z_closure_sample_move(&closure), NULL) != Z_OK);
        assert(arg.called == false);
        assert(arg.dropped == true);
        z_session_drop(z_session_move(&session));
    }
#endif

#if Z_FEATURE_QUERYABLE == 1
    {
        printf("Test: No queryable can be declared after session close\n");
        z_owned_session_t session;
        z_owned_queryable_t queryable;
        z_view_keyexpr_t ke;
        z_owned_closure_query_t closure;

        callback_arg_t arg;
        arg.called = false;
        arg.dropped = false;

        assert(open_session(&session) == Z_OK);
        assert(z_view_keyexpr_from_str(&ke, "test/closed/queryable") == Z_OK);
        z_close(z_session_loan_mut(&session), NULL);

        z_closure_query(&closure, on_receive_z_loaned_query_t_handler, on_drop_z_loaned_query_t_handler, (void*)&arg);
        assert(z_declare_queryable(z_session_loan(&session), &queryable, z_view_keyexpr_loan(&ke),
                                   z_closure_query_move(&closure), NULL) != Z_OK);
        assert(arg.called == false);
        assert(arg.dropped == true);
        z_session_drop(z_session_move(&session));
    }
#endif

#if Z_FEATURE_LIVELINESS == 1
#if Z_FEATURE_SUBSCRIPTION == 1
    {
        printf("Test: No liveliness subscriber can be declared after session close\n");
        z_owned_session_t session;
        z_owned_subscriber_t subscriber;
        z_view_keyexpr_t ke;
        z_owned_closure_sample_t closure;

        assert(open_session(&session) == Z_OK);
        assert(z_view_keyexpr_from_str(&ke, "test/closed/liveliness_subscriber") == Z_OK);
        z_close(z_session_loan_mut(&session), NULL);

        callback_arg_t arg;
        arg.called = false;
        arg.dropped = false;

        z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler,
                         (void*)&arg);
        assert(z_liveliness_declare_subscriber(z_session_loan(&session), &subscriber, z_view_keyexpr_loan(&ke),
                                               z_closure_sample_move(&closure), NULL) != Z_OK);
        assert(arg.called == false);
        assert(arg.dropped == true);
        z_session_drop(z_session_move(&session));
    }
#endif

#if Z_FEATURE_QUERY == 1
    {
        printf("Test: No z_liveliness_get can be issued after session close\n");
        z_owned_session_t session;
        z_view_keyexpr_t ke;
        z_owned_closure_reply_t closure;

        callback_arg_t arg;
        arg.called = false;
        arg.dropped = false;

        assert(open_session(&session) == Z_OK);
        assert(z_view_keyexpr_from_str(&ke, "test/closed/liveliness_get") == Z_OK);
        z_close(z_session_loan_mut(&session), NULL);

        z_closure_reply(&closure, on_receive_z_loaned_reply_t_handler, on_drop_z_loaned_reply_t_handler, (void*)&arg);
        assert(z_liveliness_get(z_session_loan(&session), z_view_keyexpr_loan(&ke), z_closure_reply_move(&closure),
                                NULL) != Z_OK);
        assert(arg.called == false);
        assert(arg.dropped == true);
        z_session_drop(z_session_move(&session));
    }
#endif

    {
        printf("Test: No liveliness token can be declared after session close\n");
        z_owned_session_t session;
        z_owned_liveliness_token_t token;
        z_view_keyexpr_t ke;

        assert(open_session(&session) == Z_OK);
        assert(z_view_keyexpr_from_str(&ke, "test/closed/liveliness_token") == Z_OK);
        z_close(z_session_loan_mut(&session), NULL);

        assert(z_liveliness_declare_token(z_session_loan(&session), &token, z_view_keyexpr_loan(&ke), NULL) != Z_OK);
        z_session_drop(z_session_move(&session));
    }
#endif

#if Z_FEATURE_QUERY == 1
    {
        printf("Test: No z_get can be issued after session close\n");
        z_owned_session_t session;
        z_view_keyexpr_t ke;
        z_owned_closure_reply_t closure;

        callback_arg_t arg;
        arg.called = false;
        arg.dropped = false;

        assert(open_session(&session) == Z_OK);
        assert(z_view_keyexpr_from_str(&ke, "test/closed/get") == Z_OK);
        z_close(z_session_loan_mut(&session), NULL);

        z_closure_reply(&closure, on_receive_z_loaned_reply_t_handler, on_drop_z_loaned_reply_t_handler, (void*)&arg);
        assert(z_get(z_session_loan(&session), z_view_keyexpr_loan(&ke), "", z_closure_reply_move(&closure), NULL) !=
               Z_OK);
        assert(arg.called == false);
        assert(arg.dropped == true);
        z_session_drop(z_session_move(&session));
    }
#endif

#if Z_FEATURE_ADVANCED_SUBSCRIPTION == 1
    {
        printf("Test: No advanced subscriber can be declared after session close\n");
        z_owned_session_t session;
        ze_owned_advanced_subscriber_t subscriber;
        z_view_keyexpr_t ke;
        z_owned_closure_sample_t closure;

        callback_arg_t arg;
        arg.called = false;
        arg.dropped = false;

        assert(open_session(&session) == Z_OK);
        assert(z_view_keyexpr_from_str(&ke, "test/closed/advanced_subscriber") == Z_OK);
        z_close(z_session_loan_mut(&session), NULL);
        z_closure_sample(&closure, on_receive_z_loaned_sample_t_handler, on_drop_z_loaned_sample_t_handler,
                         (void*)&arg);
        assert(ze_declare_advanced_subscriber(z_session_loan(&session), &subscriber, z_view_keyexpr_loan(&ke),
                                              z_closure_sample_move(&closure), NULL) != Z_OK);
        assert(arg.called == false);
        assert(arg.dropped == true);
        z_session_drop(z_session_move(&session));
    }
#endif
}

int main(void) {
#if Z_FEATURE_SUBSCRIPTION == 1 && Z_FEATURE_PUBLICATION == 1
    test_subscriber_callback_drop_on_undeclare(false);
    test_subscriber_callback_drop_on_undeclare(true);
#if Z_FEATURE_MATCHING == 1
    test_matching_callback_drop_on_undeclare(false);
    test_matching_callback_drop_on_undeclare(true);
#endif
#if Z_FEATURE_LIVELINESS == 1
    test_liveliness_subscriber_callback_drop_on_undeclare(false);
    test_liveliness_subscriber_callback_drop_on_undeclare(true);
#endif
#endif
#if Z_FEATURE_QUERYABLE == 1 && Z_FEATURE_QUERY == 1
    test_queryable_callback_drop_on_undeclare(false);
    test_queryable_callback_drop_on_undeclare(true);
    test_querier_callback_drop_on_undeclare(false);
    test_querier_callback_drop_on_undeclare(true);
#if Z_FEATURE_LIVELINESS == 1
    test_liveliness_get_callback_drop_on_undeclare();
#endif
#endif
#if Z_FEATURE_ADVANCED_SUBSCRIPTION == 1 && Z_FEATURE_ADVANCED_PUBLICATION == 1
    test_advanced_subscriber_callback_drop_on_undeclare(false);
    test_advanced_subscriber_callback_drop_on_undeclare(true);
    test_advanced_subscriber_late_join_callback_drop_on_undeclare(false);
    test_advanced_subscriber_late_join_callback_drop_on_undeclare(true);
#if Z_ADVANCED_PUBSUB_TEST_USE_TCP_PROXY == 1
    test_advanced_sample_miss_callback_drop_on_undeclare(false);
    test_advanced_sample_miss_callback_drop_on_undeclare(true);
#endif
#endif
    test_no_declare_after_session_close();
    return 0;
}
