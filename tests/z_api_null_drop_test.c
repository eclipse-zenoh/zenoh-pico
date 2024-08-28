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
#include <string.h>

#undef NDEBUG
#include <assert.h>

#include "zenoh-pico.h"

// fill v with invalid values
// set v to null
// check if v it is null
// make sure that drop on null does not crash
// make sure that double drop on null does not crash
// fill v with invalid values again
//
// set v1 to null
// move v to v1
// make sure that v is null now
#define TEST(name)                    \
    {                                 \
        z_owned_##name##_t v;         \
        memset(&v, -1, sizeof(v));    \
        z_internal_null(&v);          \
        assert(!z_internal_check(v)); \
        z_drop(z_move(v));            \
        z_drop(z_move(v));            \
        z_owned_##name##_t v1;        \
        z_internal_null(&v1);         \
        memset(&v, -1, sizeof(v));    \
        z_take(&v1, z_move(v));       \
        assert(!z_internal_check(v)); \
    }

int main(void) {
    TEST(session)
    TEST(keyexpr)
    TEST(config)
    TEST(hello)
    TEST(closure_sample)
    TEST(closure_query)
    TEST(closure_reply)
    TEST(closure_hello)
    TEST(closure_zid)
    TEST(string)
    TEST(string_array)
    TEST(sample)
    TEST(slice)
    TEST(bytes)
    TEST(encoding)
#if Z_FEATURE_PUBLICATION == 1
    TEST(publisher)
#endif
#if Z_FEATURE_SUBSCRIPTION == 1
    TEST(subscriber)
#endif
#if Z_FEATURE_QUERYABLE == 1
    TEST(query)
    TEST(queryable)
#endif
#if Z_FEATURE_QUERY == 1
    TEST(reply)
#endif
    // Double drop not supported for these types
    // TEST(task)
    // TEST(mutex)
    // TEST(condvar)

    return 0;
}
