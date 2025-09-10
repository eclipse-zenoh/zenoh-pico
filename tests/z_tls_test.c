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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/config/tls.h"
#include "zenoh-pico/system/link/tls.h"
#include "zenoh-pico/utils/result.h"

#undef NDEBUG
#include <assert.h>

int main(void) {
#if Z_FEATURE_LINK_TLS == 1
    printf(">>> Testing TLS config parsing...\n");

    _z_str_intmap_t config;
    _z_str_intmap_init(&config);

    z_result_t res = _z_tls_config_from_str(&config, "root_ca_certificate=/etc/ssl/ca.pem");
    assert(res == _Z_RES_OK);
    assert(_z_str_intmap_len(&config) == 1);

    char *ca_cert = _z_str_intmap_get(&config, TLS_CONFIG_ROOT_CA_CERTIFICATE_KEY);
    assert(ca_cert != NULL);
    assert(_z_str_eq(ca_cert, "/etc/ssl/ca.pem") == true);
    _z_str_intmap_clear(&config);

    _z_str_intmap_init(&config);
    res = _z_tls_config_from_str(&config, "root_ca_certificate=ca.pem");
    assert(res == _Z_RES_OK);
    assert(_z_str_intmap_len(&config) == 1);

    ca_cert = _z_str_intmap_get(&config, TLS_CONFIG_ROOT_CA_CERTIFICATE_KEY);
    assert(ca_cert != NULL);
    assert(_z_str_eq(ca_cert, "ca.pem") == true);
    _z_str_intmap_clear(&config);

    _z_str_intmap_init(&config);
    res = _z_tls_config_from_str(&config, "");
    assert(res == _Z_RES_OK);
    assert(_z_str_intmap_is_empty(&config) == true);
    _z_str_intmap_clear(&config);
#else
    printf("TLS feature not enabled, skipping tests\n");
#endif

    return 0;
}
