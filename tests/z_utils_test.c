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

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/query_params.h"
#include "zenoh-pico/utils/time_range.h"

#undef NDEBUG
#include <assert.h>

static void test_query_params(void) {
#define TEST_PARAMS(str, expected, n)                                 \
    {                                                                 \
        _z_str_se_t params = _z_bstrnew(str);                         \
                                                                      \
        for (int i = 0; i <= n; i++) {                                \
            _z_query_param_t param = _z_query_params_next(&params);   \
            if (i < n) {                                              \
                assert(param.key.start == expected[i].key.start);     \
                assert(param.key.end == expected[i].key.end);         \
                assert(param.value.start == expected[i].value.start); \
                assert(param.value.end == expected[i].value.end);     \
            } else {                                                  \
                assert(param.key.start == NULL);                      \
                assert(param.key.end == NULL);                        \
                assert(param.value.start == NULL);                    \
                assert(param.value.end == NULL);                      \
            }                                                         \
        }                                                             \
        assert(params.start == NULL);                                 \
        assert(params.end == NULL);                                   \
    }

    const char *params1 = "";
    const _z_query_param_t *params1_expected = NULL;
    TEST_PARAMS(params1, params1_expected, 0);

    const char *params2 = "a=1";
    const _z_query_param_t params2_expected[] = {{
        .key.start = params2,
        .key.end = _z_cptr_char_offset(params2, 1),
        .value.start = _z_cptr_char_offset(params2, 2),
        .value.end = _z_cptr_char_offset(params2, 3),
    }};
    TEST_PARAMS(params2, params2_expected, 1);

    const char *params3 = "a=1;bee=string";
    const _z_query_param_t params3_expected[] = {{
                                                     .key.start = params3,
                                                     .key.end = _z_cptr_char_offset(params3, 1),
                                                     .value.start = _z_cptr_char_offset(params3, 2),
                                                     .value.end = _z_cptr_char_offset(params3, 3),
                                                 },
                                                 {
                                                     .key.start = _z_cptr_char_offset(params3, 4),
                                                     .key.end = _z_cptr_char_offset(params3, 7),
                                                     .value.start = _z_cptr_char_offset(params3, 8),
                                                     .value.end = _z_cptr_char_offset(params3, 14),
                                                 }};
    TEST_PARAMS(params3, params3_expected, 2);

    const char *params4 = ";";
    const _z_query_param_t params4_expected[] = {{
        .key.start = NULL,
        .key.end = NULL,
        .value.start = NULL,
        .value.end = NULL,
    }};
    TEST_PARAMS(params4, params4_expected, 1);

    const char *params5 = "a";
    const _z_query_param_t params5_expected[] = {{
        .key.start = params5,
        .key.end = _z_cptr_char_offset(params5, 1),
        .value.start = NULL,
        .value.end = NULL,
    }};
    TEST_PARAMS(params5, params5_expected, 1);

    const char *params6 = "a=";
    const _z_query_param_t params6_expected[] = {{
        .key.start = params6,
        .key.end = _z_cptr_char_offset(params6, 1),
        .value.start = NULL,
        .value.end = NULL,
    }};
    TEST_PARAMS(params6, params6_expected, 1);
}

static bool compare_double_result(const double expected, const double result) {
    static const double EPSILON = 1e-6;
    return fabs(result - expected) < EPSILON;
}

static bool compare_time_range(const _z_time_range_t *a, const _z_time_range_t *b) {
    return a->start.bound == b->start.bound && compare_double_result(a->start.now_offset, b->start.now_offset) &&
           a->end.bound == b->end.bound && compare_double_result(a->end.now_offset, b->end.now_offset);
}

static void test_time_range_roundtrip(const char *input) {
    _z_time_range_t parsed1 = {0}, parsed2 = {0};
    char buf[128];

    // SAFETY: input should be a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(input, strlen(input), &parsed1));
    assert(_z_time_range_to_str(&parsed1, buf, sizeof(buf)));
    // SAFETY: _z_time_range_to_str() creates a null-terminated string if successful.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(buf, strlen(buf), &parsed2));

    printf("Round-trip: input='%s' → output='%s'\n", input, buf);

    assert(compare_time_range(&parsed1, &parsed2));
}

static void test_time_range(void) {
    _z_time_range_t result;
    const char *range = "";

    // Range tests
    range = "[..]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_UNBOUNDED);
    assert(result.end.bound == _Z_TIME_BOUND_UNBOUNDED);
    test_time_range_roundtrip(range);

    range = "[now()..now(5)]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(5.0, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now(-999.9u)..now(100.5ms)]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(-0.0009999, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.1005, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "]now(-87.6s)..now(1.5m)[";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_EXCLUSIVE);
    assert(compare_double_result(-87.6, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_EXCLUSIVE);
    assert(compare_double_result(90.0, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now(-24.5h)..now(6.75d)]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(-88200.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(583200.0, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now(-1.75w)..now()]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(-1058400.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.end.now_offset));
    test_time_range_roundtrip(range);

    // Duration tests
    range = "[now();7.3]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(7.3, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now();97.4u]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0000974, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now();568.4ms]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.5684, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now();9.4s]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(9.4, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now();6.89m]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(413.4, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now();1.567h]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(5641.2, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now();2.7894d]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(241004.16, result.end.now_offset));
    test_time_range_roundtrip(range);

    range = "[now();5.9457w]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == true);
    assert(result.start.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(0.0, result.start.now_offset));
    assert(result.end.bound == _Z_TIME_BOUND_INCLUSIVE);
    assert(compare_double_result(3595959.36, result.end.now_offset));
    test_time_range_roundtrip(range);

    // Error cases
    range = "";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == false);

    range = "[;]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == false);

    range = "[now();]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == false);

    range = "[now()..5.6]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == false);

    range = "[now();s]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == false);

    range = "[now();one]";
    // SAFETY: range is a null-terminated string.
    // Flawfinder: ignore [CWE-126]
    assert(_z_time_range_from_str(range, strlen(range), &result) == false);
}

#define STUB_SECS 1000u /*   16-Aug-1970 02:46:40 UTC in Unix time  */
#define STUB_NANOS 0u

static void test_time_range_contains_null_pointer(void) {
    _z_ntp64_t t = _z_timestamp_ntp64_from_time(STUB_SECS, STUB_NANOS);
    assert(_z_time_range_contains_at_time(NULL, 12345u, t) == false);
}

static void test_time_range_contains_unbounded_range(void) {
    _z_time_range_t r = {.start = {.bound = _Z_TIME_BOUND_UNBOUNDED, .now_offset = 0},
                         .end = {.bound = _Z_TIME_BOUND_UNBOUNDED, .now_offset = 0}};

    _z_ntp64_t now = _z_timestamp_ntp64_from_time(STUB_SECS, STUB_NANOS);
    _z_ntp64_t before = now - 42;
    _z_ntp64_t after = now + 42;

    assert(_z_time_range_contains_at_time(&r, before, now) == true);
    assert(_z_time_range_contains_at_time(&r, now, now) == true);
    assert(_z_time_range_contains_at_time(&r, after, now) == true);
}

static void test_time_range_contains_lower_bound_inclusive_exclusive(void) {
    _z_time_range_t r = {.start = {.now_offset = 0}, .end = {.bound = _Z_TIME_BOUND_UNBOUNDED, .now_offset = 0}};

    _z_ntp64_t now = _z_timestamp_ntp64_from_time(STUB_SECS, STUB_NANOS);
    _z_ntp64_t before = now - 1;
    _z_ntp64_t after = now + 1;

    // Inclusive  [now .. +∞)
    r.start.bound = _Z_TIME_BOUND_INCLUSIVE;
    assert(_z_time_range_contains_at_time(&r, now, now) == true);
    assert(_z_time_range_contains_at_time(&r, before, now) == false);
    assert(_z_time_range_contains_at_time(&r, after, now) == true);

    // Exclusive  ]now .. +∞)
    r.start.bound = _Z_TIME_BOUND_EXCLUSIVE;
    assert(_z_time_range_contains_at_time(&r, now, now) == false);
    assert(_z_time_range_contains_at_time(&r, before, now) == false);
    assert(_z_time_range_contains_at_time(&r, after, now) == true);
}

static void test_time_range_contains_upper_bound_inclusive_exclusive(void) {
    _z_time_range_t r = {.start = {.bound = _Z_TIME_BOUND_UNBOUNDED, .now_offset = 0}, .end = {.now_offset = 0}};

    _z_ntp64_t now = _z_timestamp_ntp64_from_time(STUB_SECS, STUB_NANOS);
    _z_ntp64_t before = now - 1;
    _z_ntp64_t after = now + 1;

    // Inclusive  (-∞ .. now]
    r.end.bound = _Z_TIME_BOUND_INCLUSIVE;
    assert(_z_time_range_contains_at_time(&r, before, now) == true);
    assert(_z_time_range_contains_at_time(&r, now, now) == true);
    assert(_z_time_range_contains_at_time(&r, after, now) == false);

    // Exclusive  (-∞ .. now[
    r.end.bound = _Z_TIME_BOUND_EXCLUSIVE;
    assert(_z_time_range_contains_at_time(&r, before, now) == true);
    assert(_z_time_range_contains_at_time(&r, now, now) == false);
    assert(_z_time_range_contains_at_time(&r, after, now) == false);
}

static void test_time_range_contains_fully_bounded_mixed(void) {
    // [now-5  ..  now+5[   (inclusive start, exclusive end)
    _z_time_range_t r = {.start = {.bound = _Z_TIME_BOUND_INCLUSIVE, .now_offset = -5},
                         .end = {.bound = _Z_TIME_BOUND_EXCLUSIVE, .now_offset = 5}};

    _z_ntp64_t now = _z_timestamp_ntp64_from_time(STUB_SECS, STUB_NANOS);

    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, -6.0), now) == false);
    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, -5.0), now) == true);  // Inclusive
    assert(_z_time_range_contains_at_time(&r, now, now) == true);
    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, 4.0), now) == true);
    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, 5.0), now) == false);  // Exclusive
    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, 6.0), now) == false);

    // Swap bound types
    r.start.bound = _Z_TIME_BOUND_EXCLUSIVE;
    r.end.bound = _Z_TIME_BOUND_INCLUSIVE;

    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, -6.0), now) == false);
    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, -5.0), now) == false);  // Exclusive
    assert(_z_time_range_contains_at_time(&r, now, now) == true);
    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, 4.0), now) == true);
    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, 5.0), now) == true);  // Inclusive
    assert(_z_time_range_contains_at_time(&r, _z_time_range_resolve_offset(now, 6.0), now) == false);
}

int main(void) {
    test_query_params();
    test_time_range();
    test_time_range_contains_null_pointer();
    test_time_range_contains_unbounded_range();
    test_time_range_contains_lower_bound_inclusive_exclusive();
    test_time_range_contains_upper_bound_inclusive_exclusive();
    test_time_range_contains_fully_bounded_mixed();
    return 0;
}
