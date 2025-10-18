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
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zenoh-pico.h"
#include "zenoh-pico/api/handlers.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/collections/bytes.h"

#if Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_SUBSCRIPTION == 1 && defined(Z_FEATURE_UNSTABLE_API)

#undef NDEBUG
#include <assert.h>

const char *keyexpr = "zenoh-pico/source_info/test";
const char test_payload[] = "source-info-test";

#define SAMPLE_BUF_CAP 128

#define assert_ok(x)                            \
    {                                           \
        int ret = (int)x;                       \
        if (ret != Z_OK) {                      \
            printf("%s failed: %d\n", #x, ret); \
            assert(false);                      \
        }                                       \
    }

typedef struct sample_capture_t {
    atomic_bool ready;
    bool payload_ok;
    z_entity_global_id_t source;
    uint32_t sn;
    z_sample_kind_t kind;
} sample_capture_t;

static void sample_capture_reset(sample_capture_t *capture) {
    atomic_store_explicit(&capture->ready, false, memory_order_relaxed);
    capture->payload_ok = false;
    capture->source = (z_entity_global_id_t){0};
    capture->sn = 0;
    capture->kind = Z_SAMPLE_KIND_PUT;
}

static bool wait_for_capture(sample_capture_t *capture) {
    for (int attempt = 0; attempt < 50; ++attempt) {
        if (atomic_load_explicit(&capture->ready, memory_order_acquire)) {
            return true;
        }
        z_sleep_ms(100);
    }
    return false;
}

static void sample_capture_callback(_z_sample_t *sample, void *arg) {
    sample_capture_t *capture = (sample_capture_t *)arg;
    if ((sample == NULL) || (capture == NULL)) {
        return;
    }
    capture->kind = sample->kind;
    if (sample->kind == Z_SAMPLE_KIND_PUT) {
        size_t len = _z_bytes_len(&sample->payload);
        if ((len == sizeof(test_payload) - 1) && (len < SAMPLE_BUF_CAP)) {
            char buf[SAMPLE_BUF_CAP];
            size_t copied = _z_bytes_to_buf(&sample->payload, (uint8_t *)buf, len);
            capture->payload_ok = (copied == len) && (memcmp(buf, test_payload, sizeof(test_payload) - 1) == 0);
        } else {
            capture->payload_ok = false;
        }
    } else {
        capture->payload_ok = true;
    }

    capture->source = sample->source_info._source_id;
    capture->sn = sample->source_info._source_sn;
    atomic_store_explicit(&capture->ready, true, memory_order_release);
}

static void assert_source_info_equal(const z_entity_global_id_t *expected, uint32_t expected_sn,
                                     const z_entity_global_id_t *actual, uint32_t actual_sn) {
    assert(expected->eid == actual->eid);
    assert(memcmp(expected->zid.id, actual->zid.id, Z_ZID_LENGTH) == 0);
    assert(expected_sn == actual_sn);
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
    sample_capture_t capture;
    if (local_subscriber) {
        sample_capture_reset(&capture);
        assert_ok(z_closure_sample(&callback, sample_capture_callback, NULL, &capture));
    } else {
        assert_ok(z_fifo_channel_sample_new(&callback, &handler, 1));
    }
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

    if (local_subscriber) {
        assert(wait_for_capture(&capture));
        assert(capture.kind == (put ? Z_SAMPLE_KIND_PUT : Z_SAMPLE_KIND_DELETE));
        if (put) {
            assert(capture.payload_ok);
        }
        assert_source_info_equal(&egid, sn, &capture.source, capture.sn);
    } else {
        z_owned_sample_t sample;
        z_result_t r = Z_CHANNEL_NODATA;
        for (int attempt = 0; attempt < 50; ++attempt) {
            r = z_fifo_handler_sample_try_recv(z_fifo_handler_sample_loan(&handler), &sample);
            if (r == Z_OK) {
                break;
            }
            assert(r == Z_CHANNEL_NODATA);
            z_sleep_ms(100);
        }
        assert(r == Z_OK);

        if (put) {
            z_owned_string_t value;
            assert_ok(z_bytes_to_string(z_sample_payload(z_loan(sample)), &value));
            assert((sizeof(test_payload) - 1) == z_string_len(z_loan(value)));
            assert(memcmp(test_payload, z_string_data(z_loan(value)), sizeof(test_payload) - 1) == 0);
            z_drop(z_move(value));
        }

        const z_loaned_source_info_t *si = z_sample_source_info(z_loan(sample));
        z_entity_global_id_t gid = z_source_info_id(si);
        assert_source_info_equal(&egid, sn, &gid, z_source_info_sn(si));

        z_drop(z_move(sample));
    }

    assert_ok(z_undeclare_subscriber(z_move(sub)));

    if (!local_subscriber) {
        z_drop(z_move(handler));
    }

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
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
}
#endif
