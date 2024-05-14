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

#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/protocol/core.h"

void _z_hello_clear(_z_hello_t *hello) {
    if (hello->locators.len > 0) {
        _z_str_array_clear(&hello->locators);
    }
}

void _z_hello_free(_z_hello_t **hello) {
    _z_hello_t *ptr = *hello;

    if (ptr != NULL) {
        _z_hello_clear(ptr);

        z_free(ptr);
        *hello = NULL;
    }
}

void _z_value_clear(_z_value_t *value) {
    _z_bytes_clear(&value->encoding.schema);
    _z_bytes_clear(&value->payload);
}

void _z_value_free(_z_value_t **value) {
    _z_value_t *ptr = *value;

    if (ptr != NULL) {
        _z_value_clear(ptr);

        z_free(ptr);
        *value = NULL;
    }
}
