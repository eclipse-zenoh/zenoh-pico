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
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/config/udp.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/utils/result.h"

#undef NDEBUG
#include <assert.h>

int main(void) {
    // Locator
    printf(">>> Testing locators...\n");

    _z_locator_t lc;

    _z_string_t str = _z_string_alias_str("tcp/127.0.0.1:7447");
    assert(_z_locator_from_string(&lc, &str) == _Z_RES_OK);

    str = _z_string_alias_str("tcp");
    assert(_z_string_equals(&lc._protocol, &str) == true);
    str = _z_string_alias_str("127.0.0.1:7447");
    assert(_z_string_equals(&lc._address, &str) == true);
    assert(_z_str_intmap_is_empty(&lc._metadata) == true);
    _z_locator_clear(&lc);

    str = _z_string_alias_str("");
    assert(_z_locator_from_string(&lc, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_locator_clear(&lc);

    str = _z_string_alias_str("/");
    assert(_z_locator_from_string(&lc, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_locator_clear(&lc);

    str = _z_string_alias_str("tcp");
    assert(_z_locator_from_string(&lc, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_locator_clear(&lc);

    str = _z_string_alias_str("tcp/");
    assert(_z_locator_from_string(&lc, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_locator_clear(&lc);

    str = _z_string_alias_str("127.0.0.1:7447");
    assert(_z_locator_from_string(&lc, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_locator_clear(&lc);

    str = _z_string_alias_str("tcp/127.0.0.1:7447?");
    assert(_z_locator_from_string(&lc, &str) == _Z_RES_OK);
    _z_locator_clear(&lc);

    // No metadata defined so far... but this is a valid syntax in principle
    str = _z_string_alias_str("tcp/127.0.0.1:7447?invalid=ctrl");
    assert(_z_locator_from_string(&lc, &str) == _Z_RES_OK);
    _z_locator_clear(&lc);

    // Endpoint
    printf(">>> Testing endpoints...\n");

    _z_endpoint_t ep;

    str = _z_string_alias_str("tcp/127.0.0.1:7447");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_RES_OK);

    str = _z_string_alias_str("tcp");
    assert(_z_string_equals(&ep._locator._protocol, &str) == true);
    str = _z_string_alias_str("127.0.0.1:7447");
    assert(_z_string_equals(&ep._locator._address, &str) == true);
    assert(_z_str_intmap_is_empty(&ep._locator._metadata) == true);
    assert(_z_str_intmap_is_empty(&ep._config) == true);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("/");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("tcp");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("tcp/");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("127.0.0.1:7447");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_ERR_CONFIG_LOCATOR_INVALID);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("tcp/127.0.0.1:7447?");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_RES_OK);
    _z_endpoint_clear(&ep);

    // No metadata defined so far... but this is a valid syntax in principle
    str = _z_string_alias_str("tcp/127.0.0.1:7447?invalid=ctrl");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_RES_OK);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("udp/127.0.0.1:7447#iface=eth0");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_RES_OK);

    str = _z_string_alias_str("udp");
    assert(_z_string_equals(&ep._locator._protocol, &str) == true);
    str = _z_string_alias_str("127.0.0.1:7447");
    assert(_z_string_equals(&ep._locator._address, &str) == true);
    assert(_z_str_intmap_is_empty(&ep._locator._metadata) == true);
    assert(_z_str_intmap_len(&ep._config) == 1);
    char *p = _z_str_intmap_get(&ep._config, UDP_CONFIG_IFACE_KEY);
    assert(_z_str_eq(p, "eth0") == true);
    (void)(p);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("udp/127.0.0.1:7447#invalid=eth0");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_RES_OK);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("udp/127.0.0.1:7447?invalid=ctrl#iface=eth0");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_RES_OK);
    _z_endpoint_clear(&ep);

    str = _z_string_alias_str("udp/127.0.0.1:7447?invalid=ctrl#invalid=eth0");
    assert(_z_endpoint_from_string(&ep, &str) == _Z_RES_OK);
    _z_endpoint_clear(&ep);

    return 0;
}
