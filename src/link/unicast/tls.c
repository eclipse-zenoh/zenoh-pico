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

#include "zenoh-pico/link/config/tls.h"

#include <stddef.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/common/socket_ops.h"
#include "zenoh-pico/link/driver.h"
#include "zenoh-pico/link/link.h"
#include "zenoh-pico/link/transport/tcp.h"
#include "zenoh-pico/link/transport/tls_stream.h"
#include "zenoh-pico/utils/config.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/string.h"

#if Z_FEATURE_LINK_TLS == 1

typedef struct {
    _z_link_t _base;
    _z_tls_socket_t _tls;
} _z_tls_link_t;

static size_t _z_link_peer_read_tls(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len);
static size_t _z_link_peer_write_tls(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr, size_t len);
z_result_t _z_new_peer_tls(_z_endpoint_t *endpoint, _z_sys_net_socket_t *socket, const _z_config_t *session_cfg);

static const _z_link_peer_ops_t _z_tls_peer_ops = {
    ._read_f = _z_link_peer_read_tls,
    ._write_f = _z_link_peer_write_tls,
    ._set_blocking_f = _z_link_socket_peer_set_blocking,
    ._get_endpoints_f = _z_link_socket_peer_get_endpoints,
    ._close_f = _z_link_socket_peer_close,
};

uint16_t _z_get_link_mtu_tls(void) { return 65535; }

z_result_t _z_endpoint_tls_valid(_z_endpoint_t *endpoint) {
    _z_string_t tls_str = _z_string_alias_str(TLS_SCHEMA);
    if (!_z_string_equals(&endpoint->_locator._protocol, &tls_str)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    return _z_tcp_address_valid(&endpoint->_locator._address);
}

static _z_config_t _z_tls_merge_config(_z_str_intmap_t *endpoint_cfg, const _z_config_t *session_cfg) {
    _z_config_t cfg;
    if (endpoint_cfg != NULL) {
        _z_str_intmap_move(&cfg, endpoint_cfg);
    } else {
        cfg = _z_str_intmap_make();
    }
    if (session_cfg == NULL) {
        return cfg;
    }
    static const struct {
        uint8_t locator_key;
        uint8_t session_key;
    } mapping[] = {{TLS_CONFIG_ROOT_CA_CERTIFICATE_KEY, Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_KEY},
                   {TLS_CONFIG_ROOT_CA_CERTIFICATE_BASE64_KEY, Z_CONFIG_TLS_ROOT_CA_CERTIFICATE_BASE64_KEY},
                   {TLS_CONFIG_LISTEN_PRIVATE_KEY_KEY, Z_CONFIG_TLS_LISTEN_PRIVATE_KEY_KEY},
                   {TLS_CONFIG_LISTEN_PRIVATE_KEY_BASE64_KEY, Z_CONFIG_TLS_LISTEN_PRIVATE_KEY_BASE64_KEY},
                   {TLS_CONFIG_LISTEN_CERTIFICATE_KEY, Z_CONFIG_TLS_LISTEN_CERTIFICATE_KEY},
                   {TLS_CONFIG_LISTEN_CERTIFICATE_BASE64_KEY, Z_CONFIG_TLS_LISTEN_CERTIFICATE_BASE64_KEY},
                   {TLS_CONFIG_ENABLE_MTLS_KEY, Z_CONFIG_TLS_ENABLE_MTLS_KEY},
                   {TLS_CONFIG_CONNECT_PRIVATE_KEY_KEY, Z_CONFIG_TLS_CONNECT_PRIVATE_KEY_KEY},
                   {TLS_CONFIG_CONNECT_PRIVATE_KEY_BASE64_KEY, Z_CONFIG_TLS_CONNECT_PRIVATE_KEY_BASE64_KEY},
                   {TLS_CONFIG_CONNECT_CERTIFICATE_KEY, Z_CONFIG_TLS_CONNECT_CERTIFICATE_KEY},
                   {TLS_CONFIG_CONNECT_CERTIFICATE_BASE64_KEY, Z_CONFIG_TLS_CONNECT_CERTIFICATE_BASE64_KEY},
                   {TLS_CONFIG_VERIFY_NAME_ON_CONNECT_KEY, Z_CONFIG_TLS_VERIFY_NAME_ON_CONNECT_KEY}};

    for (size_t i = 0; i < sizeof(mapping) / sizeof(mapping[0]); i++) {
        if (_z_config_get(&cfg, mapping[i].locator_key) != NULL) {
            continue;
        }
        const char *value = _z_config_get(session_cfg, mapping[i].session_key);
        if (value != NULL) {
            _zp_config_insert(&cfg, mapping[i].locator_key, value);
        }
    }
    return cfg;
}

static z_result_t _z_f_link_open_tls(_z_link_t *self) {
    _z_tls_link_t *link = (_z_tls_link_t *)self;
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    char *hostname = _z_tcp_address_parse_host(&self->_endpoint._locator._address);
    if (hostname == NULL) {
        _Z_ERROR("Failed to parse TLS endpoint address");
        z_free(hostname);
        return _Z_ERR_GENERIC;
    }

    _z_sys_net_endpoint_t rep = {0};
    _Z_CLEAN_RETURN_IF_ERR(_z_tcp_endpoint_init_from_address(&rep, &self->_endpoint._locator._address),
                           z_free(hostname));

    z_result_t ret = _z_open_tls(&link->_tls, &rep, hostname, &self->_endpoint._config, false);
    _z_tcp_endpoint_clear(&rep);
    z_free(hostname);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("TLS open failed");
        return ret;
    }

    // Link-owned TLS state is closed through _z_close_tls(); peer handles only provide link-peer ops.
    _Z_CLEAN_RETURN_IF_ERR(_z_link_socket_peer_from_socket(&self->_peer, link->_tls._sock, NULL, &_z_tls_peer_ops),
                           _z_close_tls(&link->_tls));
    return _Z_RES_OK;
}

static z_result_t _z_f_link_listen_tls(_z_link_t *self) {
    _z_tls_link_t *link = (_z_tls_link_t *)self;
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    z_result_t ret = _Z_RES_OK;

    char *host = _z_tcp_address_parse_host(&self->_endpoint._locator._address);
    if (host == NULL) {
        _Z_ERROR("Invalid TLS endpoint");
        z_free(host);
        return _Z_ERR_GENERIC;
    }

    _z_sys_net_endpoint_t rep = {0};
    _Z_CLEAN_RETURN_IF_ERR(_z_tcp_endpoint_init_from_address(&rep, &self->_endpoint._locator._address), z_free(host));

    ret = _z_listen_tls(&link->_tls, &rep, &self->_endpoint._config);
    _z_tcp_endpoint_clear(&rep);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("TLS listen failed");
    } else {
        // Link-owned TLS state is closed through _z_close_tls(); peer handles only provide link-peer ops.
        ret = _z_link_socket_peer_from_socket(&self->_peer, link->_tls._sock, NULL, &_z_tls_peer_ops);
        if (ret != _Z_RES_OK) {
            _z_close_tls(&link->_tls);
        }
    }

    z_free(host);
    return ret;
}

static void _z_f_link_close_tls(_z_link_t *self) {
    _z_tls_link_t *link = (_z_tls_link_t *)self;
    if (link != NULL) {
        _z_close_tls(&link->_tls);
    }
}

static size_t _z_f_link_write_tls(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    if ((socket == NULL) || (socket->_tls_sock == NULL)) {
        _Z_ERROR("TLS context not found in socket");
        return SIZE_MAX;
    }
    return _z_write_tls((_z_tls_socket_t *)socket->_tls_sock, ptr, len);
}

static size_t _z_f_link_write_all_tls(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    if ((socket == NULL) || (socket->_tls_sock == NULL)) {
        _Z_ERROR("TLS context not found in socket");
        return SIZE_MAX;
    }
    return _z_write_all_tls((_z_tls_socket_t *)socket->_tls_sock, ptr, len);
}

static size_t _z_f_link_read_tls(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    if ((socket == NULL) || (socket->_tls_sock == NULL)) {
        _Z_ERROR("TLS context not found in socket");
        return SIZE_MAX;
    }
    return _z_read_tls((_z_tls_socket_t *)socket->_tls_sock, ptr, len);
}

static size_t _z_f_link_read_exact_tls(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(&self->_peer);
    if ((socket == NULL) || (socket->_tls_sock == NULL)) {
        _Z_ERROR("TLS context not found in socket");
        return SIZE_MAX;
    }

    size_t n = (size_t)0;
    do {
        size_t rb = _z_read_tls((_z_tls_socket_t *)socket->_tls_sock, &ptr[n], len - n);

        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }
        n += rb;
    } while (n != len);

    return n;
}

static size_t _z_link_peer_read_tls(const _z_link_t *link, const _z_link_peer_t *peer, uint8_t *ptr, size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    if ((socket == NULL) || (socket->_tls_sock == NULL)) {
        _Z_ERROR("TLS context not found in socket");
        return SIZE_MAX;
    }
    return _z_read_tls((_z_tls_socket_t *)socket->_tls_sock, ptr, len);
}

static size_t _z_link_peer_write_tls(const _z_link_t *link, const _z_link_peer_t *peer, const uint8_t *ptr,
                                     size_t len) {
    _ZP_UNUSED(link);
    const _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket_const(peer);
    if ((socket == NULL) || (socket->_tls_sock == NULL)) {
        _Z_ERROR("TLS context not found in socket");
        return SIZE_MAX;
    }
    return _z_write_tls((_z_tls_socket_t *)socket->_tls_sock, ptr, len);
}

static void _z_link_peer_close_tls_socket(_z_sys_net_socket_t *socket) {
    if (socket == NULL) {
        return;
    }
    if (socket->_tls_sock != NULL) {
        _z_close_tls_socket(socket);
    } else {
        _z_tcp_close(socket);
    }
}

static z_result_t _z_f_link_open_peer_tls(const _z_link_t *link, _z_link_peer_t *peer, const _z_string_t *locator,
                                          const _z_config_t *session_cfg) {
    _ZP_UNUSED(link);

    _z_endpoint_t ep;
    z_result_t ret = _z_endpoint_from_string(&ep, locator);
    if (ret != _Z_RES_OK) {
        _z_endpoint_clear(&ep);
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    _z_sys_net_socket_t socket = {0};
    if (_z_endpoint_tls_valid(&ep) == _Z_RES_OK) {
        ret = _z_new_peer_tls(&ep, &socket, session_cfg);
    } else {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN);
        ret = _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN;
    }
    _z_endpoint_clear(&ep);

    if (ret != _Z_RES_OK) {
        return ret;
    }
    _Z_CLEAN_RETURN_IF_ERR(
        _z_link_socket_peer_from_socket(peer, socket, _z_link_peer_close_tls_socket, &_z_tls_peer_ops),
        _z_link_peer_close_tls_socket(&socket));
    return _Z_RES_OK;
}

static z_result_t _z_f_link_accept_tls(const _z_link_t *link, _z_link_peer_t *peer) {
    if ((link == NULL) || (peer == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    const _z_sys_net_socket_t *listen_socket = _z_link_socket_peer_get_socket_const(&link->_peer);
    if (listen_socket == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    _z_sys_net_socket_t con_socket = {0};
    _Z_RETURN_IF_ERR(_z_tcp_accept(listen_socket, &con_socket));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_link_socket_peer_from_socket(peer, con_socket, _z_link_peer_close_tls_socket, &_z_tls_peer_ops),
        _z_tcp_close(&con_socket));

    _Z_CLEAN_RETURN_IF_ERR(_z_link_socket_peer_set_blocking(peer, true), _z_link_peer_clear(peer));

    return _Z_RES_OK;
}

static z_result_t _z_f_link_accept_peer_complete_tls(const _z_link_t *link, _z_link_peer_t *peer) {
    if ((link == NULL) || (peer == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    const _z_sys_net_socket_t *listen_socket = _z_link_socket_peer_get_socket_const(&link->_peer);
    _z_sys_net_socket_t *socket = _z_link_socket_peer_get_socket(peer);
    if ((listen_socket == NULL) || (socket == NULL)) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    return _z_tls_accept(socket, listen_socket);
}

z_result_t _z_new_link_tls(_z_link_t **zl, _z_endpoint_t *endpoint, const _z_config_t *session_cfg) {
    if (zl == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }
    *zl = NULL;

    _z_tls_link_t *link = (_z_tls_link_t *)z_malloc(sizeof(_z_tls_link_t));
    if (link == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    memset(link, 0, sizeof(_z_tls_link_t));

    _z_link_t *base = &link->_base;
    base->_endpoint = *endpoint;
    *endpoint = (_z_endpoint_t){0};
    base->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    base->_cap._flow = Z_LINK_CAP_FLOW_STREAM;
    base->_cap._is_reliable = true;

    base->_mtu = _z_get_link_mtu_tls();

    _z_config_t cfg = _z_tls_merge_config(&base->_endpoint._config, session_cfg);
    base->_endpoint._config = cfg;
    _Z_DEBUG("TLS locator: '%.*s'", (int)_z_string_len(&base->_endpoint._locator._address),
             _z_string_data(&base->_endpoint._locator._address));

    base->_close_f = _z_f_link_close_tls;
    base->_write_f = _z_f_link_write_tls;
    base->_write_all_f = _z_f_link_write_all_tls;
    base->_read_f = _z_f_link_read_tls;
    base->_read_exact_f = _z_f_link_read_exact_tls;
    base->_wait_peers_readable_f = _z_link_socket_wait_peers_readable;
    base->_open_peer_f = _z_f_link_open_peer_tls;
    base->_peer_from_link_f = _z_link_peer_from_default;
    base->_accept_peer_f = _z_f_link_accept_tls;
    base->_accept_peer_complete_f = _z_f_link_accept_peer_complete_tls;

    *zl = base;

    return _Z_RES_OK;
}

static z_result_t _z_link_driver_tls_create(_z_link_t **link, _z_endpoint_t *endpoint, const _z_config_t *session_cfg) {
    return _z_new_link_tls(link, endpoint, session_cfg);
}

const _z_link_driver_t _z_link_driver_tls = {
    ._validate_f = _z_endpoint_tls_valid,
    ._create_f = _z_link_driver_tls_create,
    ._open_f = _z_f_link_open_tls,
    ._listen_f = _z_f_link_listen_tls,
};

z_result_t _z_new_peer_tls(_z_endpoint_t *endpoint, _z_sys_net_socket_t *socket, const _z_config_t *session_cfg) {
    _z_sys_net_endpoint_t sys_endpoint = {0};
    char *hostname = _z_tcp_address_parse_host(&endpoint->_locator._address);
    z_result_t ret = _Z_RES_OK;
    if (hostname == NULL) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
        goto cleanup;
    }

    ret = _z_tcp_endpoint_init_from_address(&sys_endpoint, &endpoint->_locator._address);
    if (ret != _Z_RES_OK) {
        goto cleanup;
    }

    socket->_tls_sock = z_malloc(sizeof(_z_tls_socket_t));
    if (socket->_tls_sock == NULL) {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        goto cleanup;
    }

    _z_config_t cfg = _z_tls_merge_config(&endpoint->_config, session_cfg);
    ret = _z_open_tls((_z_tls_socket_t *)socket->_tls_sock, &sys_endpoint, hostname, &cfg, true);
    if (ret != _Z_RES_OK) {
        z_free(socket->_tls_sock);
        socket->_tls_sock = NULL;
        _z_config_clear(&cfg);
        _z_str_intmap_clear(&endpoint->_config);
        goto cleanup;
    }

    socket->_fd = ((_z_tls_socket_t *)socket->_tls_sock)->_sock._fd;
    _z_config_clear(&cfg);
    _z_str_intmap_clear(&endpoint->_config);

cleanup:
    z_free(hostname);
    _z_tcp_endpoint_clear(&sys_endpoint);
    return ret;
}

#endif  // Z_FEATURE_LINK_TLS == 1
