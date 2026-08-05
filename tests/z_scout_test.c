//
// Copyright (c) 2026 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License 2.0 which is
// available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//

#include <assert.h>

#include "zenoh-pico/session/utils.h"

int main(void) {
    _z_hello_t hello = {0};
    assert(!_z_scout_should_exit_on_first(true, &hello));

    _z_string_t locator = _z_string_alias_str("tcp/127.0.0.1:7447");
    hello._locators = _z_string_svec_make(1);
    assert(_z_string_svec_append(&hello._locators, &locator, true) == _Z_RES_OK);
    assert(_z_scout_should_exit_on_first(true, &hello));
    assert(!_z_scout_should_exit_on_first(false, &hello));

    _z_string_svec_clear(&hello._locators);
    return 0;
}
