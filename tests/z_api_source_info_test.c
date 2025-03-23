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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zenoh-pico.h"
#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#if Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_SUBSCRIPTION == 1 && defined(Z_FEATURE_UNSTABLE_API)

#undef NDEBUG
#include <assert.h>

const char *keyexpr = "zenoh-pico/source_info/test";
const char *test_payload = "source-info-test";

#define assert_ok(x)                            \
    {                                           \
        int ret = (int)x;                       \
        if (ret != Z_OK) {                      \
            printf("%s failed: %d\n", #x, ret); \
            assert(false);                      \
        }                                       \
    }

void test_source_info(bool put, bool publisher, bool local_subscriber) {
    z_owned_config_t c1;
    z_config_default(&c1);

    z_owned_session_t s1;
    assert_ok(z_open(&s1, z_config_move(&c1), NULL));

    assert_ok(zp_start_read_task(z_loan_mut(s1), NULL));
    assert_ok(zp_start_lease_task(z_loan_mut(s1), NULL));

    z_owned_session_t s2;
    const z_loaned_session_t *sub_session = z_loan(s1);
    if (!local_subscriber) {
        z_owned_config_t c2;
        z_config_default(&c2);

        assert_ok(z_open(&s2, z_config_move(&c2), NULL));

        assert_ok(zp_start_read_task(z_loan_mut(s2), NULL));
        assert_ok(zp_start_lease_task(z_loan_mut(s2), NULL));

        sub_session = z_loan(s2);
    }

    z_view_keyexpr_t ke;
    assert_ok(z_view_keyexpr_from_str(&ke, keyexpr));

    z_owned_closure_sample_t callback;
    z_owned_fifo_handler_sample_t handler;
    assert_ok(z_fifo_channel_sample_new(&callback, &handler, 1));
    z_owned_subscriber_t sub;
    assert_ok(z_declare_subscriber(sub_session, &sub, z_loan(ke), z_move(callback), NULL));
    z_sleep_s(1);

    z_owned_bytes_t payload;
    if (put) {
        assert_ok(z_bytes_copy_from_str(&payload, test_payload));
    }

    z_entity_global_id_t egid = _z_entity_global_id_null();
    uint32_t sn = z_random_u32();
    if (put && publisher) {
        z_owned_publisher_t pub;
        assert_ok(z_declare_publisher(z_loan(s1), &pub, z_loan(ke), NULL));
        z_sleep_s(1);

        z_publisher_put_options_t opt;
        z_publisher_put_options_default(&opt);
        egid = z_publisher_id(z_loan(pub));
        z_owned_source_info_t si;
        assert_ok(z_source_info_new(&si, &egid, sn));
        opt.source_info = z_move(si);

        z_publisher_put(z_loan(pub), z_move(payload), &opt);
        z_sleep_s(1);
        assert_ok(z_undeclare_publisher(z_move(pub)));
    } else if (put) {
        z_put_options_t opt;
        z_put_options_default(&opt);
        egid.zid = z_info_zid(z_loan(s1));
        egid.eid = z_random_u32();
        z_owned_source_info_t si;
        assert_ok(z_source_info_new(&si, &egid, sn));
        opt.source_info = z_move(si);

        assert_ok(z_put(z_loan(s1), z_loan(ke), z_move(payload), &opt));
    } else if (!put && publisher) {
        z_owned_publisher_t pub;
        assert_ok(z_declare_publisher(z_loan(s1), &pub, z_loan(ke), NULL));
        z_sleep_s(1);

        z_publisher_delete_options_t opt;
        z_publisher_delete_options_default(&opt);
        egid = z_publisher_id(z_loan(pub));
        z_owned_source_info_t si;
        assert_ok(z_source_info_new(&si, &egid, sn));
        opt.source_info = z_move(si);

        z_publisher_delete(z_loan(pub), &opt);
        z_sleep_s(1);
        assert_ok(z_undeclare_publisher(z_move(pub)));
    } else {
        z_delete_options_t opt;
        z_delete_options_default(&opt);
        egid.zid = z_info_zid(z_loan(s1));
        egid.eid = z_random_u32();
        z_owned_source_info_t si;
        assert_ok(z_source_info_new(&si, &egid, sn));
        opt.source_info = z_move(si);

        assert_ok(z_delete(z_loan(s1), z_loan(ke), &opt));
    }
    z_sleep_s(1);

    z_owned_sample_t sample;
    assert_ok(z_fifo_handler_sample_try_recv(z_fifo_handler_sample_loan(&handler), &sample));

    if (put) {
        z_owned_string_t value;
        assert_ok(z_bytes_to_string(z_sample_payload(z_loan(sample)), &value));
        assert(strlen(test_payload) == z_string_len(z_loan(value)));
        assert(memcmp(test_payload, z_string_data(z_loan(value)), z_string_len(z_loan(value))) == 0);
        z_drop(z_move(value));
    }

    const z_loaned_source_info_t *si = z_sample_source_info(z_loan(sample));
    z_entity_global_id_t gid = z_source_info_id(si);
    assert(gid.eid == egid.eid);
    assert(memcmp(gid.zid.id, egid.zid.id, (sizeof(gid.zid.id) / sizeof(gid.zid.id[0]))) == 0);
    assert(sn == z_source_info_sn(si));

    z_drop(z_move(sample));

    assert_ok(z_undeclare_subscriber(z_move(sub)));

    z_drop(z_move(handler));

    assert_ok(zp_stop_read_task(z_loan_mut(s1)));
    assert_ok(zp_stop_lease_task(z_loan_mut(s1)));

    z_session_drop(z_session_move(&s1));

    if (!local_subscriber) {
        assert_ok(zp_stop_read_task(z_loan_mut(s2)));
        assert_ok(zp_stop_lease_task(z_loan_mut(s2)));

        z_session_drop(z_session_move(&s2));
    }
}

void test_source_info_put(bool publisher, bool local_subscriber) {
    printf("test_source_info_put: publisher=%d, local_subscriber=%d\n", publisher, local_subscriber);
    test_source_info(true, publisher, local_subscriber);
}

void test_source_info_delete(bool publisher, bool local_subscriber) {
    printf("test_source_info_delete: publisher=%d, local_subscriber=%d\n", publisher, local_subscriber);
    test_source_info(false, publisher, local_subscriber);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_source_info_put(false, false);
    test_source_info_delete(false, false);
    test_source_info_put(true, false);
    test_source_info_delete(true, false);
#if Z_FEATURE_LOCAL_SUBSCRIBER == 1
    test_source_info_put(false, true);
    test_source_info_put(true, true);
#endif
}

#else
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
}
#endif
