/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/link/endpoint.h"
#include "zenoh-pico/link/config/udp.h"

int main(void)
{

    // Locator
    _zn_locator_result_t lres;

    lres = _zn_locator_from_str("tcp/127.0.0.1:7447");
    assert(lres.tag == _z_res_t_OK);
    assert(strcmp(lres.value.locator.protocol, "tcp") == 0);
    assert(strcmp(lres.value.locator.address, "127.0.0.1:7447") == 0);
    assert(lres.value.locator.metadata.len == 0);
    _zn_locator_clear(&lres.value.locator);

    lres = _zn_locator_from_str("");
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    lres = _zn_locator_from_str("/");
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    lres = _zn_locator_from_str("tcp");
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    lres = _zn_locator_from_str("tcp/");
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    lres = _zn_locator_from_str("127.0.0.1:7447");
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    lres = _zn_locator_from_str("tcp/127.0.0.1:7447?");
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    // No metadata defined so far... but this is a valid syntax in principle
    lres = _zn_locator_from_str("tcp/127.0.0.1:7447?invalid=ctrl");
    assert(lres.tag == _z_res_t_ERR);
    assert(lres.value.error == _z_err_t_PARSE_STRING);

    // Endpoint
    _zn_endpoint_result_t eres;

    eres = _zn_endpoint_from_str("tcp/127.0.0.1:7447");
    assert(eres.tag == _z_res_t_OK);
    assert(strcmp(eres.value.endpoint.locator.protocol, "tcp") == 0);
    assert(strcmp(eres.value.endpoint.locator.address, "127.0.0.1:7447") == 0);
    assert(zn_properties_is_empty(&eres.value.endpoint.locator.metadata));
    assert(zn_properties_is_empty(&eres.value.endpoint.config));
    _zn_endpoint_clear(&eres.value.endpoint);

    eres = _zn_endpoint_from_str("");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    eres = _zn_endpoint_from_str("/");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    eres = _zn_endpoint_from_str("tcp");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    eres = _zn_endpoint_from_str("tcp/");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    eres = _zn_endpoint_from_str("127.0.0.1:7447");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    eres = _zn_endpoint_from_str("tcp/127.0.0.1:7447?");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    // No metadata defined so far... but this is a valid syntax in principle
    eres = _zn_endpoint_from_str("tcp/127.0.0.1:7447?invalid=ctrl");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    char s[64];
    sprintf(s, "udp/127.0.0.1:7447#%s=eth0", UDP_CONFIG_MULTICAST_IFACE_STR);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_OK);
    assert(strcmp(eres.value.endpoint.locator.protocol, "udp") == 0);
    assert(strcmp(eres.value.endpoint.locator.address, "127.0.0.1:7447") == 0);
    assert(zn_properties_is_empty(&eres.value.endpoint.locator.metadata));
    z_string_t p = zn_properties_get(&eres.value.endpoint.config, UDP_CONFIG_MULTICAST_IFACE_KEY);
    assert(strncmp(p.val, "eth0", p.len) == 0);
    _zn_endpoint_clear(&eres.value.endpoint);

    eres = _zn_endpoint_from_str("udp/127.0.0.1:7447#invalid=eth0");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    sprintf(s, "udp/127.0.0.1:7447?invalid=ctrl#%s=eth0", UDP_CONFIG_MULTICAST_IFACE_STR);
    eres = _zn_endpoint_from_str(s);
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    eres = _zn_endpoint_from_str("udp/127.0.0.1:7447?invalid=ctrl#invalid=eth0");
    assert(eres.tag == _z_res_t_ERR);
    assert(eres.value.error == _z_err_t_PARSE_STRING);

    return 0;
}
