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
#include <stddef.h>
#include <stdio.h>

#include "zenoh-pico/link/driver_registry.h"

int main(void) {
    printf(">>> Testing link driver registry smoke invariants...\n");

    const _z_link_driver_t *const *drivers = _z_link_driver_registry();
    size_t driver_count = 0;
    for (size_t i = 0; drivers[i] != NULL; i++) {
        const _z_link_driver_t *driver = drivers[i];
        assert(driver != NULL);
        assert(driver->_validate_f != NULL);
        assert(driver->_create_f != NULL);
        assert((driver->_open_f != NULL) || (driver->_listen_f != NULL));
        driver_count++;
    }

    assert(driver_count > 0);
    return 0;
}
