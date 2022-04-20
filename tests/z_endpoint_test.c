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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/link/config/udp.h"

int main(void)
{
#if ZN_LINK_UDP_UNICAST == 1
    char s[64];

    // Locator
    printf(">>> Testing locators...\n");

    _zn_locator_result_t lres;

    sprintf(s, "tcp/127.0.0.1:7447");
    printf("- %s\n", s);
    lres = _zn_locator_from_str(s);
    assert(lres.tag == _z_res_t_OK);
    assert(_z_str_eq(lres.value.locator.protocol, "tcp"));
    assert(_z_str_eq(lres.value.locator.address, "127.0.0.1:7447"));
    assert(_z_str_intmap_is_empty(&lres.value.locator.metadata));
    _zn_locator_clear(&lres.value.locator);

    s[0] = '\0'; // sprintf(s, "");
    printf("- %s\n", s);
    lres = _zn_locator_from_str(s);
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "/");
    printf("- %s\n", s);
    lres = _zn_locator_from_str("/");
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "tcp");
    printf("- %s\n", s);
    lres = _zn_locator_from_str(s);
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "tcp/");
    printf("- %s\n", s);
    lres = _zn_locator_from_str(s);
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "127.0.0.1:7447");
    printf("- %s\n", s);
    lres = _zn_locator_from_str(s);
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "tcp/127.0.0.1:7447?");
    printf("- %s\n", s);
    lres = _zn_locator_from_str(s);
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    // No metadata defined so far... but this is a valid syntax in principle
    sprintf(s, "tcp/127.0.0.1:7447?invalid=ctrl");
    printf("- %s\n", s);
    lres = _zn_locator_from_str(s);
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    // Endpoint
    printf(">>> Testing endpoints...\n");

    _zn_endpoint_result_t eres;

    sprintf(s, "tcp/127.0.0.1:7447");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_OK);
    assert(_z_str_eq(eres.value.endpoint.locator.protocol, "tcp"));
    assert(_z_str_eq(eres.value.endpoint.locator.address, "127.0.0.1:7447"));
    assert(_z_str_intmap_is_empty(&eres.value.endpoint.locator.metadata));
    assert(_z_str_intmap_is_empty(&eres.value.endpoint.config));
    _zn_endpoint_clear(&eres.value.endpoint);

    s[0] = '\0'; // sprintf(s, "");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "/");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "tcp");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "tcp");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "127.0.0.1:7447");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "tcp/127.0.0.1:7447?");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    // No metadata defined so far... but this is a valid syntax in principle
    sprintf(s, "tcp/127.0.0.1:7447?invalid=ctrl");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "udp/127.0.0.1:7447#%s=eth0", UDP_CONFIG_IFACE_STR);
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_OK);
    assert(_z_str_eq(eres.value.endpoint.locator.protocol, "udp"));
    assert(_z_str_eq(eres.value.endpoint.locator.address, "127.0.0.1:7447"));
    assert(_z_str_intmap_is_empty(&eres.value.endpoint.locator.metadata));
    assert(_z_str_intmap_len(&eres.value.endpoint.config) == 1);
    z_str_t p = _z_str_intmap_get(&eres.value.endpoint.config, UDP_CONFIG_IFACE_KEY);
    assert(_z_str_eq(p, "eth0"));
    (void)(p);
    _zn_endpoint_clear(&eres.value.endpoint);

    sprintf(s, "udp/127.0.0.1:7447#invalid=eth0");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "udp/127.0.0.1:7447?invalid=ctrl#%s=eth0", UDP_CONFIG_IFACE_STR);
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "udp/127.0.0.1:7447?invalid=ctrl#invalid=eth0");
    printf("- %s\n", s);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);
#endif

    return 0;
}
