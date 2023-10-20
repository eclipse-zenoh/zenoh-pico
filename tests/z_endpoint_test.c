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
    char *s = (char *)malloc(64);

    // Locator
    printf(">>> Testing locators...\n");

    int8_t ret = _Z_RES_OK;
    _z_locator_t lc;

    snprintf(s, 64, "tcp/127.0.0.1:7447");
    printf("- %s\n", s);
    ret = _z_locator_from_str(&lc, s);
    assert(ret == _Z_RES_OK);
    assert(_z_str_eq(lc._protocol, "tcp") == true);
    assert(_z_str_eq(lc._address, "127.0.0.1:7447") == true);
    assert(_z_str_intmap_is_empty(&lc._metadata) == true);
    _z_locator_clear(&lc);

    s[0] = '\0';  // snprintf(s, "");
    printf("- %s\n", s);
    ret = _z_locator_from_str(&lc, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "/");
    printf("- %s\n", s);
    ret = _z_locator_from_str(&lc, "/");
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "tcp");
    printf("- %s\n", s);
    ret = _z_locator_from_str(&lc, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "tcp/");
    printf("- %s\n", s);
    ret = _z_locator_from_str(&lc, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "127.0.0.1:7447");
    printf("- %s\n", s);
    ret = _z_locator_from_str(&lc, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "tcp/127.0.0.1:7447?");
    printf("- %s\n", s);
    ret = _z_locator_from_str(&lc, s);
    assert(ret == _Z_RES_OK);

    // No metadata defined so far... but this is a valid syntax in principle
    snprintf(s, 64, "tcp/127.0.0.1:7447?invalid=ctrl");
    printf("- %s\n", s);
    ret = _z_locator_from_str(&lc, s);
    assert(ret == _Z_RES_OK);

    // Endpoint
    printf(">>> Testing endpoints...\n");

    _z_endpoint_t ep;

    snprintf(s, 64, "tcp/127.0.0.1:7447");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_RES_OK);
    assert(_z_str_eq(ep._locator._protocol, "tcp") == true);
    assert(_z_str_eq(ep._locator._address, "127.0.0.1:7447") == true);
    assert(_z_str_intmap_is_empty(&ep._locator._metadata) == true);
    assert(_z_str_intmap_is_empty(&ep._config) == true);
    _z_endpoint_clear(&ep);

    s[0] = '\0';  // snprintf(s, "");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "/");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "tcp");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "tcp");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "127.0.0.1:7447");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED);

    snprintf(s, 64, "tcp/127.0.0.1:7447?");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_RES_OK);

    // No metadata defined so far... but this is a valid syntax in principle
    snprintf(s, 64, "tcp/127.0.0.1:7447?invalid=ctrl");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_RES_OK);

    snprintf(s, 64, "udp/127.0.0.1:7447#%s=eth0", UDP_CONFIG_IFACE_STR);
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_RES_OK);
    assert(_z_str_eq(ep._locator._protocol, "udp") == true);
    assert(_z_str_eq(ep._locator._address, "127.0.0.1:7447") == true);
    assert(_z_str_intmap_is_empty(&ep._locator._metadata) == true);
    assert(_z_str_intmap_len(&ep._config) == 1);
    char *p = _z_str_intmap_get(&ep._config, UDP_CONFIG_IFACE_KEY);
    assert(_z_str_eq(p, "eth0") == true);
    (void)(p);
    _z_endpoint_clear(&ep);

    snprintf(s, 64, "udp/127.0.0.1:7447#invalid=eth0");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_RES_OK);

    snprintf(s, 64, "udp/127.0.0.1:7447?invalid=ctrl#%s=eth0", UDP_CONFIG_IFACE_STR);
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_RES_OK);

    snprintf(s, 64, "udp/127.0.0.1:7447?invalid=ctrl#invalid=eth0");
    printf("- %s\n", s);
    ret = _z_endpoint_from_str(&ep, s);
    assert(ret == _Z_RES_OK);

    return 0;
}
