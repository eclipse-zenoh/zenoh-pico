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

#include "zenoh-pico.h"

#undef NDEBUG
#include <assert.h>

#define URL "demo/example"

void test_keyexpr(void) {
    z_owned_keyexpr_t keyexpr;
    z_keyexpr_from_str(&keyexpr, URL);
    assert(z_check(keyexpr));
    z_drop(z_move(keyexpr));
    assert(!z_check(keyexpr));
    z_drop(z_move(keyexpr));
    assert(!z_check(keyexpr));
}

void test_config(void) {
    z_owned_config_t config;
    z_config_default(&config);
    assert(z_check(config));
    z_drop(z_move(config));
    assert(!z_check(config));
    z_drop(z_move(config));
    assert(!z_check(config));
}

int main(void) {
    test_keyexpr();
    test_config();
    return 0;
}
