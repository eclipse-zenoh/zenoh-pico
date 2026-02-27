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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/assert_helpers.h"
#include "zenoh-pico.h"

#if Z_FEATURE_QUERY == 1 && Z_FEATURE_QUERYABLE == 1

#define TEST_SLEEP_MS 2000
#define BASE_EXPR "zenoh-pico/multi-queryable"

// Per-queryable state (kept alive for the lifetime of the queryable)
typedef struct {
    int qid;
    z_owned_keyexpr_t ke;  // concrete keyexpr for this queryable
} qstate_t;

static void on_query(z_loaned_query_t *query, void *ctx) {
    const qstate_t *st = (const qstate_t *)ctx;

    char msg[32];
    (void)snprintf(msg, sizeof(msg), "q=%d", st->qid);

    z_owned_bytes_t payload;
    ASSERT_OK(z_bytes_copy_from_str(&payload, msg));
    ASSERT_OK(z_query_reply(query, z_keyexpr_loan(&st->ke), z_move(payload), NULL));
}

static bool all_seen(const bool *seen, int n) {
    for (int i = 0; i < n; i++) {
        if (!seen[i]) return false;
    }
    return true;
}

static bool try_recv_qid(const z_loaned_fifo_handler_reply_t *handler, int *out_qid) {
    z_owned_reply_t r;
    z_internal_reply_null(&r);

    if (z_try_recv(handler, &r) != Z_OK) {
        return false;
    }

    if (!z_reply_is_ok(z_loan(r))) {
        z_drop(z_move(r));
        *out_qid = -1;
        return true;
    }

    const z_loaned_sample_t *sample = z_reply_ok(z_loan(r));
    ASSERT_NOT_NULL(sample);

    z_owned_string_t s;
    z_internal_string_null(&s);
    ASSERT_OK(z_bytes_to_string(z_sample_payload(sample), &s));

    const char *ptr = z_string_data(z_loan(s));
    int qid = -1;
    if (ptr != NULL && sscanf(ptr, "q=%d", &qid) == 1) {
        *out_qid = qid;
    } else {
        *out_qid = -1;
    }

    z_drop(z_move(s));
    z_drop(z_move(r));
    return true;
}

static size_t collect_seen_queriables(const z_loaned_fifo_handler_reply_t *handler, unsigned total_wait_ms,
                                      unsigned step_ms, bool *seen, int num_q) {
    size_t got = 0;
    unsigned waited = 0;

    while (waited < total_wait_ms && !all_seen(seen, num_q)) {
        int qid = -1;
        while (try_recv_qid(handler, &qid)) {
            got++;
            if (qid >= 0 && qid < num_q) {
                seen[qid] = true;
            }
        }
        z_sleep_ms(step_ms);
        waited += step_ms;
    }

    // Final drain
    int qid = -1;
    while (try_recv_qid(handler, &qid)) {
        got++;
        if (qid >= 0 && qid < num_q) {
            seen[qid] = true;
        }
    }

    return got;
}

static void test_multi_queryables(int num_q) {
    printf("test_multi_queryables (queriables=%d)\n", num_q);

    z_owned_session_t s_q, s_cli;
    z_owned_config_t c_q, c_cli;
    z_config_default(&c_q);
    z_config_default(&c_cli);

    ASSERT_OK(z_open(&s_q, z_config_move(&c_q), NULL));
    ASSERT_OK(z_open(&s_cli, z_config_move(&c_cli), NULL));

    ASSERT_OK(zp_start_read_task(z_loan_mut(s_q), NULL));
    ASSERT_OK(zp_start_read_task(z_loan_mut(s_cli), NULL));
    ASSERT_OK(zp_start_lease_task(z_loan_mut(s_q), NULL));
    ASSERT_OK(zp_start_lease_task(z_loan_mut(s_cli), NULL));

    // Declare N queryables, each on its own keyexpr: BASE_EXPR/<qid>
    z_owned_queryable_t *qs = (z_owned_queryable_t *)z_malloc((size_t)num_q * sizeof(z_owned_queryable_t));
    qstate_t *states = (qstate_t *)z_malloc((size_t)num_q * sizeof(qstate_t));
    ASSERT_NOT_NULL(qs);
    ASSERT_NOT_NULL(states);

    z_queryable_options_t qopts;
    z_queryable_options_default(&qopts);

    for (int i = 0; i < num_q; i++) {
        states[i].qid = i;
        z_internal_keyexpr_null(&states[i].ke);

        char expr[128];
        (void)snprintf(expr, sizeof(expr), "%s/%d", BASE_EXPR, i);

        ASSERT_OK(z_keyexpr_from_str(&states[i].ke, expr));

        z_owned_closure_query_t cb;
        ASSERT_OK(z_closure_query(&cb, on_query, NULL, &states[i]));

        ASSERT_OK(z_declare_queryable(z_loan(s_q), &qs[i], z_keyexpr_loan(&states[i].ke), z_move(cb), &qopts));
    }

    // Let declarations propagate
    z_sleep_ms(TEST_SLEEP_MS);

    // Query with wildcard to match them all
    char qexpr[128];
    (void)snprintf(qexpr, sizeof(qexpr), "%s/**", BASE_EXPR);

    z_view_keyexpr_t qke;
    ASSERT_OK(z_view_keyexpr_from_str(&qke, qexpr));

    z_owned_fifo_handler_reply_t rh;
    z_owned_closure_reply_t rcb;

    // Capacity: expect at least num_q replies
    ASSERT_OK(z_fifo_channel_reply_new(&rcb, &rh, (size_t)num_q * 2));

    z_get_options_t gopts;
    z_get_options_default(&gopts);
    gopts.timeout_ms = TEST_SLEEP_MS;
    gopts.target = Z_QUERY_TARGET_ALL;

    ASSERT_OK(z_get(z_loan(s_cli), z_loan(qke), "", z_closure_reply_move(&rcb), &gopts));

    bool *seen = (bool *)z_malloc((size_t)num_q * sizeof(bool));
    ASSERT_NOT_NULL(seen);
    (void)memset(seen, 0, (size_t)num_q * sizeof(bool));

    size_t got = collect_seen_queriables(z_loan(rh), TEST_SLEEP_MS, 50, seen, num_q);

    if (!all_seen(seen, num_q)) {
        fprintf(stderr, "Missing replies from queryables:");
        for (int i = 0; i < num_q; i++) {
            if (!seen[i]) fprintf(stderr, " %d", i);
        }
        fprintf(stderr, " (total replies seen=%zu)\n", got);
        fflush(stderr);
        assert(false && "missing at least one queryable reply");
    }

    z_free(seen);

    // Cleanup
    z_fifo_handler_reply_drop(z_fifo_handler_reply_move(&rh));
    for (int i = 0; i < num_q; i++) {
        z_queryable_drop(z_queryable_move(&qs[i]));
        z_keyexpr_drop(z_keyexpr_move(&states[i].ke));
    }
    z_free(states);
    z_free(qs);

    ASSERT_OK(zp_stop_read_task(z_loan_mut(s_q)));
    ASSERT_OK(zp_stop_read_task(z_loan_mut(s_cli)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s_q)));
    ASSERT_OK(zp_stop_lease_task(z_loan_mut(s_cli)));

    z_session_drop(z_session_move(&s_q));
    z_session_drop(z_session_move(&s_cli));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    test_multi_queryables(1);
    test_multi_queryables(4);
    test_multi_queryables(5);
    test_multi_queryables(10);

    return 0;
}

#else

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf(
        "Missing config token to build this test. This test requires: "
        "Z_FEATURE_QUERY and Z_FEATURE_QUERYABLE\n");
    return 0;
}

#endif
