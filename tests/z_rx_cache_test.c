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

#undef NDEBUG
#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zenoh-pico.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/session/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"

#if Z_FEATURE_SUBSCRIPTION == 1

typedef struct {
    atomic_uint deliveries;
    const uint8_t *expected_payload;
    size_t expected_payload_len;
} rx_cache_test_ctx_t;

static void assert_payload(z_loaned_sample_t *sample, const uint8_t *expected, size_t expected_len) {
    z_owned_string_t value;
    z_internal_string_null(&value);

    assert(z_bytes_to_string(z_sample_payload(sample), &value) == Z_OK);

    const char *ptr = z_string_data(z_loan(value));
    size_t len = z_string_len(z_loan(value));

    assert(len == expected_len);
    assert(memcmp(ptr, expected, len) == 0);

    z_drop(z_move(value));
}

static void sample_callback(_z_sample_t *sample, void *arg) {
    rx_cache_test_ctx_t *ctx = (rx_cache_test_ctx_t *)arg;

    assert_payload((z_loaned_sample_t *)sample, ctx->expected_payload, ctx->expected_payload_len);
    atomic_fetch_add_explicit(&ctx->deliveries, 1, memory_order_relaxed);
}

static void trigger_put(_z_session_t *session, const char *expr, const uint8_t *payload_data, size_t payload_len) {
    _z_keyexpr_t keyexpr = _z_keyexpr_alias_from_str(expr);
    _z_wireexpr_t wireexpr = _z_keyexpr_alias_to_wire(&keyexpr);
    _z_bytes_t payload = _z_bytes_null();
    _z_encoding_t encoding = _z_encoding_null();
    _z_timestamp_t timestamp = _z_timestamp_null();
    _z_bytes_t attachment = _z_bytes_null();
    _z_source_info_t source_info = _z_source_info_null();
    _z_n_qos_t qos = _z_n_qos_make(false, false, Z_PRIORITY_DEFAULT);

    assert(_z_bytes_from_buf(&payload, payload_data, payload_len) == _Z_RES_OK);
    assert(_z_trigger_subscriptions_put(session, &wireexpr, &payload, &encoding, &timestamp, qos, &attachment,
                                        Z_RELIABILITY_RELIABLE, &source_info, NULL) == _Z_RES_OK);
}

static void test_subscription_registration_invalidates_empty_cache_entry(void) {
    printf("test_subscription_registration_invalidates_empty_cache_entry\n");

    const char *expr = "zenoh-pico/rx-cache/subscription-registration";
    static const uint8_t warmup_payload[] = "before-subscribe";
    static const uint8_t matched_payload[] = "after-subscribe";

    _z_id_t zid;
    _z_session_generate_zid(&zid, Z_ZID_LENGTH);

    _z_session_t session;
    assert(_z_session_init(&session, &zid) == _Z_RES_OK);

    trigger_put(&session, expr, warmup_payload, sizeof(warmup_payload) - 1);

    rx_cache_test_ctx_t ctx = {
        .expected_payload = matched_payload,
        .expected_payload_len = sizeof(matched_payload) - 1,
    };
    atomic_init(&ctx.deliveries, 0);

    _z_subscription_t sub = _z_subscription_null();
    sub._id = _z_get_entity_id(&session);
    sub._key = _z_declared_keyexpr_alias_from_str(expr);
    sub._callback = sample_callback;
    sub._arg = &ctx;
    sub._allowed_origin = Z_LOCALITY_SESSION_LOCAL;

    _z_subscription_rc_t sub_rc = _z_register_subscription(&session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, &sub);
    assert(!_Z_RC_IS_NULL(&sub_rc));

    trigger_put(&session, expr, matched_payload, sizeof(matched_payload) - 1);
    assert(atomic_load_explicit(&ctx.deliveries, memory_order_relaxed) == 1);

    _z_unregister_subscription(&session, _Z_SUBSCRIBER_KIND_SUBSCRIBER, &sub_rc);
    _z_session_clear(&session);
}

int main(void) {
    test_subscription_registration_invalidates_empty_cache_entry();
    return 0;
}

#else

int main(void) {
    printf("This test requires: Z_FEATURE_SUBSCRIPTION.\n");
    return 0;
}

#endif
