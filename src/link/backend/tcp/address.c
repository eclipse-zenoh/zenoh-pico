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

#include <stdlib.h>

#include "zenoh-pico/link/backend/tcp.h"
#include "zenoh-pico/link/endpoint.h"

char *_z_tcp_address_parse_host(const _z_string_t *address) { return _z_endpoint_parse_host(address); }

z_result_t _z_tcp_address_valid(const _z_string_t *address) {
    char *host = _z_tcp_address_parse_host(address);
    char *port = _z_endpoint_parse_port(address);
    z_result_t ret = ((host != NULL) && (port != NULL)) ? _Z_RES_OK : _Z_ERR_CONFIG_LOCATOR_INVALID;

    z_free(host);
    z_free(port);
    return ret;
}

z_result_t _z_tcp_endpoint_init_from_address(const _z_tcp_ops_t *ops, _z_sys_net_endpoint_t *ep,
                                             const _z_string_t *address) {
    z_result_t ret = _Z_RES_OK;
    char *host = _z_tcp_address_parse_host(address);
    char *port = _z_endpoint_parse_port(address);

    if (ops == NULL) {
        ret = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    } else if ((host == NULL) || (port == NULL)) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    } else {
        ret = _z_tcp_endpoint_init(ops, ep, host, port);
    }

    z_free(host);
    z_free(port);
    return ret;
}
