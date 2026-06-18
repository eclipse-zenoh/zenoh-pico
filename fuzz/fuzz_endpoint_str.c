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

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/endpoint.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    _z_string_t input = _z_string_alias_substr((const char *)data, size);
    _z_endpoint_t endpoint = {0};

    if (_z_endpoint_from_string(&endpoint, &input) != _Z_RES_OK) {
        _z_endpoint_clear(&endpoint);
        return 0;
    }

    _z_string_t endpoint_str = _z_endpoint_to_string(&endpoint);
    if (_z_string_check(&endpoint_str)) {
        _z_endpoint_t reparsed = {0};

        (void)_z_endpoint_from_string(&reparsed, &endpoint_str);
        _z_endpoint_clear(&reparsed);
        _z_string_clear(&endpoint_str);
    }

    _z_endpoint_clear(&endpoint);
    return 0;
}
