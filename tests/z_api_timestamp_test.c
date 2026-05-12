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

#include <stddef.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/utils.h"

#undef NDEBUG
#include <assert.h>

typedef struct {
    _z_id_t zid;
    _z_session_t session;
    _z_session_rc_t session_rc;
    _z_time_since_epoch clock_value;
} timestamp_test_fixture_t;

static void setup_session(timestamp_test_fixture_t *fixture) {
    *fixture = (timestamp_test_fixture_t){0};
    _z_session_generate_zid(&fixture->zid, Z_ZID_LENGTH);
    assert(_z_session_init(&fixture->session, &fixture->zid) == _Z_RES_OK);
    if (_Z_RC_IS_NULL(&fixture->session_rc)) {
        fixture->session_rc = _z_session_rc_new(&fixture->session);
        assert(!_Z_RC_IS_NULL(&fixture->session_rc));
    } else {
        fixture->session_rc._val = &fixture->session;
    }
}

static z_result_t fake_time_since_epoch(_z_time_since_epoch *t, void *arg) {
    timestamp_test_fixture_t *fixture = (timestamp_test_fixture_t *)arg;
    *t = fixture->clock_value;
    return _Z_RES_OK;
}

static void setup_session_with_fake_clock(timestamp_test_fixture_t *fixture, uint32_t secs, uint32_t nanos) {
    setup_session(fixture);
    fixture->clock_value.secs = secs;
    fixture->clock_value.nanos = nanos;
    _z_timestamp_set_time_since_epoch_override(fake_time_since_epoch, fixture);
}

static void cleanup_session(timestamp_test_fixture_t *fixture) {
    _z_timestamp_set_time_since_epoch_override(NULL, NULL);
    assert(_z_session_rc_decr(&fixture->session_rc));
    fixture->session_rc = _z_session_rc_null();
    _z_session_clear(&fixture->session);
}

static void test_timestamp_new_with_real_clock(void) {
    timestamp_test_fixture_t fixture;
    setup_session(&fixture);

    z_timestamp_t prev;
    z_timestamp_t next;
    assert(z_timestamp_new(&prev, &fixture.session_rc) == _Z_RES_OK);
    assert(_z_timestamp_check(&prev));
    assert(z_timestamp_ntp64_time(&prev) > 0);
    assert(_z_id_eq(&prev.id, &fixture.zid));

    for (size_t i = 0; i < 10000; i++) {
        assert(z_timestamp_new(&next, &fixture.session_rc) == _Z_RES_OK);
        assert(_z_timestamp_cmp(&prev, &next) < 0);
        prev = next;
    }

    cleanup_session(&fixture);
}

static void test_timestamp_new_with_repeated_time(void) {
    timestamp_test_fixture_t fixture;
    setup_session_with_fake_clock(&fixture, 42, 100);

    z_timestamp_t first;
    z_timestamp_t second;
    z_timestamp_t third;
    _z_ntp64_t timestamp = _z_timestamp_ntp64_from_time(42, 100);
    assert(z_timestamp_new(&first, &fixture.session_rc) == _Z_RES_OK);
    assert(z_timestamp_new(&second, &fixture.session_rc) == _Z_RES_OK);
    fixture.clock_value.nanos = 99;
    assert(z_timestamp_new(&third, &fixture.session_rc) == _Z_RES_OK);

    assert(z_timestamp_ntp64_time(&first) == timestamp);
    assert(z_timestamp_ntp64_time(&second) == timestamp + 1);
    assert(z_timestamp_ntp64_time(&third) == timestamp + 2);
    assert(_z_timestamp_cmp(&first, &second) < 0);
    assert(_z_timestamp_cmp(&second, &third) < 0);
    assert(_z_id_eq(&first.id, &fixture.zid));
    assert(_z_id_eq(&second.id, &fixture.zid));
    assert(_z_id_eq(&third.id, &fixture.zid));

    cleanup_session(&fixture);
}

int main(void) {
    test_timestamp_new_with_real_clock();
    test_timestamp_new_with_repeated_time();
    return 0;
}
