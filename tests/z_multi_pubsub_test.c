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

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"

#if Z_FEATURE_PUBLICATION == 1 && Z_FEATURE_SUBSCRIPTION == 1

#define TEST_SLEEP_MS 2000
#define FIFO_CAPACITY 32
#define NUM_MSGS 8

static void pub_put_str(const z_loaned_publisher_t *pub, const char *s) {
    z_owned_bytes_t payload;
    ASSERT_OK(z_bytes_copy_from_str(&payload, s));
    ASSERT_OK(z_publisher_put(pub, z_move(payload), NULL));
}

static bool try_recv_pub_id(const z_loaned_fifo_handler_sample_t *handler, int *out_pub_id) {
    z_owned_sample_t sample;
    z_internal_sample_null(&sample);

    if (z_try_recv(handler, &sample) != Z_OK) {
        return false;
    }

    ASSERT_TRUE(z_sample_kind(z_loan(sample)) == Z_SAMPLE_KIND_PUT);

    // Parse payload as string: expected "m=%d p=%d"
    z_owned_string_t s;
    z_internal_string_null(&s);
    ASSERT_OK(z_bytes_to_string(z_sample_payload(z_loan(sample)), &s));

    const char *ptr = z_string_data(z_loan(s));
    int m = -1, p = -1;
    if (ptr != NULL && sscanf(ptr, "m=%d p=%d", &m, &p) == 2) {
        *out_pub_id = p;
    } else {
        // Ignore unexpected payloads (e.g., from other tests/processes)
        *out_pub_id = -1;
    }

    z_drop(z_move(s));
    z_drop(z_move(sample));
    return true;
}

static bool all_seen_publishers(const bool *seen, int num_pubs) {
    for (int i = 0; i < num_pubs; i++) {
        if (!seen[i]) {
            return false;
        }
    }
    return true;
}

static size_t collect_seen_publishers(const z_loaned_fifo_handler_sample_t *handler, unsigned total_wait_ms,
                                      unsigned step_ms, bool *seen, int num_pubs) {
    size_t got = 0;
    unsigned waited = 0;

    while (waited < total_wait_ms && !all_seen_publishers(seen, num_pubs)) {
        int pub_id = -1;
        while (try_recv_pub_id(handler, &pub_id)) {
            got++;
            if (pub_id >= 0 && pub_id < num_pubs) {
                seen[pub_id] = true;
            }
        }
        z_sleep_ms(step_ms);
        waited += step_ms;
    }

    // Final drain of what's currently available
    int pub_id = -1;
    while (try_recv_pub_id(handler, &pub_id)) {
        got++;
        if (pub_id >= 0 && pub_id < num_pubs) {
            seen[pub_id] = true;
        }
    }

    return got;
}

static void test_multi_pub_multi_sub(int num_pubs, int num_subs) {
    printf("test_multi_pub_multi_sub (pubs=%d subs=%d)\n", num_pubs, num_subs);

    const char *expr = "zenoh-pico/multi-pubsub";

    z_owned_session_t s_pub, s_sub;
    z_owned_config_t c_pub, c_sub;
    z_config_default(&c_pub);
    z_config_default(&c_sub);

    z_view_keyexpr_t k;
    ASSERT_OK(z_view_keyexpr_from_str(&k, expr));

    ASSERT_OK(z_open(&s_pub, z_config_move(&c_pub), NULL));
    ASSERT_OK(z_open(&s_sub, z_config_move(&c_sub), NULL));

    // Start tasks on both sessions
    ASSERT_OK(zp_start_read_task(z_loan_mut(s_pub), NULL));
    ASSERT_OK(zp_start_read_task(z_loan_mut(s_sub), NULL));
    ASSERT_OK(zp_start_lease_task(z_loan_mut(s_pub), NULL));
    ASSERT_OK(zp_start_lease_task(z_loan_mut(s_sub), NULL));

    // Declare N publishers on s_pub
    z_owned_publisher_t *pubs = (z_owned_publisher_t *)calloc((size_t)num_pubs, sizeof(z_owned_publisher_t));
    ASSERT_NOT_NULL(pubs);

    z_publisher_options_t pub_opts;
    z_publisher_options_default(&pub_opts);

    for (int i = 0; i < num_pubs; i++) {
        ASSERT_OK(z_declare_publisher(z_loan(s_pub), &pubs[i], z_loan(k), &pub_opts));
    }

    // Declare M subscribers on s_sub, each with its own fifo channel
    z_owned_subscriber_t *subs = (z_owned_subscriber_t *)calloc((size_t)num_subs, sizeof(z_owned_subscriber_t));
    z_owned_fifo_handler_sample_t *handlers =
        (z_owned_fifo_handler_sample_t *)calloc((size_t)num_subs, sizeof(z_owned_fifo_handler_sample_t));
    ASSERT_NOT_NULL(subs);
    ASSERT_NOT_NULL(handlers);

    z_subscriber_options_t sub_opts;
    z_subscriber_options_default(&sub_opts);

    for (int i = 0; i < num_subs; i++) {
        z_owned_closure_sample_t closure;
        ASSERT_OK(z_fifo_channel_sample_new(&closure, &handlers[i], FIFO_CAPACITY));
        ASSERT_OK(z_declare_subscriber(z_loan(s_sub), &subs[i], z_loan(k), z_move(closure), &sub_opts));
    }

    // Allow subscription declarations to propagate
    z_sleep_ms(TEST_SLEEP_MS);

    // Publish multiple messages (from all publishers) to reduce flakiness.
    char msg[64];
    for (int m = 0; m < NUM_MSGS; m++) {
        for (int p = 0; p < num_pubs; p++) {
            snprintf(msg, sizeof(msg), "m=%d p=%d", m, p);
            pub_put_str(z_loan(pubs[p]), msg);
        }
        // tiny pacing so we don't instantly overflow FIFOs at high fanout
        z_sleep_ms(5);
    }

    // Give time for delivery then assert each subscriber got >= 1 from each publisher
    for (int i = 0; i < num_subs; i++) {
        bool *seen = (bool *)calloc((size_t)num_pubs, sizeof(bool));
        ASSERT_NOT_NULL(seen);

        size_t got = collect_seen_publishers(z_loan(handlers[i]),
                                             /*total_wait_ms=*/TEST_SLEEP_MS,
                                             /*step_ms=*/50, seen, num_pubs);

        if (!all_seen_publishers(seen, num_pubs)) {
            fprintf(stderr, "Subscriber %d missing samples from publishers:", i);
            for (int p = 0; p < num_pubs; p++) {
                if (!seen[p]) {
                    fprintf(stderr, " %d", p);
                }
            }
            fprintf(stderr, " (total samples seen=%zu)\n", got);
            fflush(stderr);
            assert(false && "subscriber missing at least one publisher");
        }

        free(seen);
    }

    // Cleanup
    for (int i = 0; i < num_subs; i++) {
        z_subscriber_drop(z_subscriber_move(&subs[i]));
        z_fifo_handler_sample_drop(z_fifo_handler_sample_move(&handlers[i]));
    }
    for (int i = 0; i < num_pubs; i++) {
        z_publisher_drop(z_publisher_move(&pubs[i]));
    }

    free(handlers);
    free(subs);
    free(pubs);

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s_pub)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s_sub)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s_pub)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s_sub)));

    z_session_drop(z_session_move(&s_pub));
    z_session_drop(z_session_move(&s_sub));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    test_multi_pub_multi_sub(1, 1);
    test_multi_pub_multi_sub(1, 4);
    test_multi_pub_multi_sub(1, 5);
    test_multi_pub_multi_sub(10, 10);

    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf(
        "Missing config token to build this test. This test requires: Z_FEATURE_PUBLICATION "
        "and Z_FEATURE_SUBSCRIPTION\n");
    return 0;
}

#endif
