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
#include "zenoh-pico/utils/config.h"
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
    res = _z_tls_config_from_str(&config,
                                 "root_ca_certificate=ca.pem;enable_mtls=1;connect_private_key=client.key;"
                                 "connect_certificate=client.pem;verify_name_on_connect=0;"
                                 "root_ca_certificate_base64=BASE64_CA;connect_private_key_base64=BASE64_KEY;"
                                 "connect_certificate_base64=BASE64_CERT");
    assert(res == _Z_RES_OK);
    assert(_z_str_intmap_len(&config) == 8);

    ca_cert = _z_str_intmap_get(&config, TLS_CONFIG_ROOT_CA_CERTIFICATE_KEY);
    assert(ca_cert != NULL);
    assert(_z_str_eq(ca_cert, "ca.pem") == true);
    char *enable = _z_str_intmap_get(&config, TLS_CONFIG_ENABLE_MTLS_KEY);
    assert(enable != NULL);
    assert(_z_str_eq(enable, "1") == true);
    char *client_key = _z_str_intmap_get(&config, TLS_CONFIG_CONNECT_PRIVATE_KEY_KEY);
    assert(client_key != NULL);
    assert(_z_str_eq(client_key, "client.key") == true);
    char *client_cert = _z_str_intmap_get(&config, TLS_CONFIG_CONNECT_CERTIFICATE_KEY);
    assert(client_cert != NULL);
    assert(_z_str_eq(client_cert, "client.pem") == true);
    char *verify = _z_str_intmap_get(&config, TLS_CONFIG_VERIFY_NAME_ON_CONNECT_KEY);
    assert(verify != NULL);
    assert(_z_str_eq(verify, "0") == true);
    char *ca_base64 = _z_str_intmap_get(&config, TLS_CONFIG_ROOT_CA_CERTIFICATE_BASE64_KEY);
    assert(ca_base64 != NULL && _z_str_eq(ca_base64, "BASE64_CA") == true);
    char *client_key_inline = _z_str_intmap_get(&config, TLS_CONFIG_CONNECT_PRIVATE_KEY_BASE64_KEY);
    assert(client_key_inline != NULL && _z_str_eq(client_key_inline, "BASE64_KEY") == true);
    _z_str_intmap_clear(&config);

    _z_str_intmap_init(&config);
    res = _z_tls_config_from_str(&config, "");
    assert(res == _Z_RES_OK);
    assert(_z_str_intmap_is_empty(&config) == true);
    _z_str_intmap_clear(&config);

    _z_config_t session_cfg;
    _z_config_init(&session_cfg);
    assert(_zp_config_insert(&session_cfg, Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_KEY, "/session/ca.pem") == _Z_RES_OK);
    assert(_zp_config_insert(&session_cfg, Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_BASE64_KEY, "SESSION_CA") == _Z_RES_OK);
    assert(_zp_config_insert(&session_cfg, Z_CONFIG_TLS_ENABLE_MTLS_KEY, "true") == _Z_RES_OK);
    assert(_zp_config_insert(&session_cfg, Z_CONFIG_TLS_CONNECT_CERTIFICATE_BASE64_KEY, "SESSION_CERT") == _Z_RES_OK);
    assert(_z_str_eq(_z_config_get(&session_cfg, Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_KEY), "/session/ca.pem"));
    assert(_z_str_eq(_z_config_get(&session_cfg, Z_CONFIG_TLS_ENABLE_MTLS_KEY), "true"));
    _z_config_clear(&session_cfg);
#else
    printf("TLS feature not enabled, skipping tests\n");
#endif

    return 0;
}
