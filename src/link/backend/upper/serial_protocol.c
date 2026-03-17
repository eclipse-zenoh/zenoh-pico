//
// Copyright (c) 2024 ZettaScale Technology
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

#include "zenoh-pico/link/backend/serial_protocol.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/link/backend/default_ops.h"
#include "zenoh-pico/link/backend/rawio.h"
#include "zenoh-pico/link/config/serial.h"
#include "zenoh-pico/protocol/codec/serial.h"
#include "zenoh-pico/protocol/definitions/serial.h"
#include "zenoh-pico/system/common/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_LINK_SERIAL == 1

#define SERIAL_CONNECT_THROTTLE_TIME_MS 250

typedef struct {
    bool _from_pins;
    uint32_t _baudrate;
    uint32_t _txpin;
    uint32_t _rxpin;
    char *_dev;
} _z_serial_endpoint_cfg_t;

static void _z_serial_endpoint_cfg_clear(_z_serial_endpoint_cfg_t *cfg) {
    z_free(cfg->_dev);
    cfg->_dev = NULL;
}

static z_result_t _z_serial_parse_u32(const char *str, uint32_t *value) {
    char *end = NULL;
    unsigned long parsed = 0;

    if (str == NULL || str[0] == '\0') {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    errno = 0;
    parsed = strtoul(str, &end, 10);
    if (errno != 0 || end == str || *end != '\0' || parsed == 0 || parsed > UINT32_MAX) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    *value = (uint32_t)parsed;
    return _Z_RES_OK;
}

static char *_z_serial_copy_address(const _z_string_t *address) {
    size_t len = _z_string_len(address);
    char *copy = (char *)z_malloc(len + 1);
    if (copy != NULL) {
        _z_str_n_copy(copy, _z_string_data(address), len + 1);
    }
    return copy;
}

static z_result_t _z_serial_endpoint_parse(_z_serial_endpoint_cfg_t *cfg, const _z_endpoint_t *endpoint) {
    z_result_t ret = _Z_RES_OK;
    _z_string_t ser_str = _z_string_alias_str(SERIAL_SCHEMA);
    char *address = NULL;
    char *dot = NULL;
    const char *baudrate_str = NULL;

    (void)memset(cfg, 0, sizeof(*cfg));

    if (!_z_string_equals(&endpoint->_locator._protocol, &ser_str)) {
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    baudrate_str = _z_str_intmap_get(&endpoint->_config, SERIAL_CONFIG_BAUDRATE_KEY);
    ret = _z_serial_parse_u32(baudrate_str, &cfg->_baudrate);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    address = _z_serial_copy_address(&endpoint->_locator._address);
    if (address == NULL || address[0] == '\0') {
        z_free(address);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    dot = strchr(address, '.');
    if (dot == NULL) {
        cfg->_dev = address;
        return _Z_RES_OK;
    }

    *dot = '\0';
    dot++;
    if (address[0] == '\0' || dot[0] == '\0') {
        z_free(address);
        return _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    ret = _z_serial_parse_u32(address, &cfg->_txpin);
    if (ret != _Z_RES_OK) {
        z_free(address);
        return ret;
    }

    ret = _z_serial_parse_u32(dot, &cfg->_rxpin);
    z_free(address);
    if (ret != _Z_RES_OK) {
        return ret;
    }

    cfg->_from_pins = true;
    return _Z_RES_OK;
}

#if defined(ZP_DEFAULT_RAWIO_OPS)
static const _z_rawio_ops_t *_z_serial_rawio_ops(void) { return _z_default_rawio_ops(); }

static size_t _z_serial_rawio_write_all(const _z_rawio_ops_t *ops, _z_sys_net_socket_t sock, const uint8_t *ptr,
                                        size_t len) {
    size_t total = 0;
    while (total != len) {
        size_t wb = _z_rawio_write(ops, sock, _z_ptr_u8_offset((uint8_t *)ptr, (ptrdiff_t)total), len - total);
        if (wb == SIZE_MAX || wb == 0) {
            return SIZE_MAX;
        }
        total += wb;
    }
    return total;
}

static size_t _z_read_serial_internal(const _z_sys_net_socket_t sock, uint8_t *header, uint8_t *ptr, size_t len) {
    const _z_rawio_ops_t *ops = _z_serial_rawio_ops();
    if (ops == NULL) {
        return SIZE_MAX;
    }

    uint8_t *raw_buf = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    if (raw_buf == NULL) {
        _Z_ERROR("Failed to allocate serial COBS buffer");
        return SIZE_MAX;
    }

    size_t rb = 0;
    while (rb < _Z_SERIAL_MAX_COBS_BUF_SIZE) {
        size_t chunk = _z_rawio_read(ops, sock, &raw_buf[rb], 1);
        if (chunk != 1) {
            z_free(raw_buf);
            return SIZE_MAX;
        }

        rb++;
        if (raw_buf[rb - 1] == (uint8_t)0x00) {
            break;
        }
    }

    uint8_t *tmp_buf = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    if (tmp_buf == NULL) {
        z_free(raw_buf);
        _Z_ERROR("Failed to allocate serial MFS buffer");
        return SIZE_MAX;
    }

    size_t ret = _z_serial_msg_deserialize(raw_buf, rb, ptr, len, header, tmp_buf, _Z_SERIAL_MFS_SIZE);
    z_free(raw_buf);
    z_free(tmp_buf);
    return ret;
}

static size_t _z_send_serial_internal(const _z_sys_net_socket_t sock, uint8_t header, const uint8_t *ptr, size_t len) {
    const _z_rawio_ops_t *ops = _z_serial_rawio_ops();
    if (ops == NULL) {
        return SIZE_MAX;
    }

    uint8_t *tmp_buf = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    uint8_t *raw_buf = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    if ((raw_buf == NULL) || (tmp_buf == NULL)) {
        z_free(raw_buf);
        z_free(tmp_buf);
        _Z_ERROR("Failed to allocate serial COBS and/or MFS buffer");
        return SIZE_MAX;
    }

    size_t raw_len =
        _z_serial_msg_serialize(raw_buf, _Z_SERIAL_MAX_COBS_BUF_SIZE, ptr, len, header, tmp_buf, _Z_SERIAL_MFS_SIZE);
    if (raw_len == SIZE_MAX) {
        z_free(raw_buf);
        z_free(tmp_buf);
        return SIZE_MAX;
    }

    size_t written = _z_serial_rawio_write_all(ops, sock, raw_buf, raw_len);
    z_free(raw_buf);
    z_free(tmp_buf);
    return (written == raw_len) ? len : SIZE_MAX;
}
#else
static size_t _z_read_serial_internal(const _z_sys_net_socket_t sock, uint8_t *header, uint8_t *ptr, size_t len) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(header);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    return SIZE_MAX;
}

static size_t _z_send_serial_internal(const _z_sys_net_socket_t sock, uint8_t header, const uint8_t *ptr, size_t len) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(header);
    _ZP_UNUSED(ptr);
    _ZP_UNUSED(len);
    return SIZE_MAX;
}
#endif

z_result_t _z_serial_endpoint_valid(const _z_endpoint_t *endpoint) {
    _z_serial_endpoint_cfg_t cfg;
    z_result_t ret = _z_serial_endpoint_parse(&cfg, endpoint);
    _z_serial_endpoint_cfg_clear(&cfg);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
    }
    return ret;
}

#if defined(ZP_DEFAULT_RAWIO_OPS)
static z_result_t _z_serial_open_impl(_z_serial_socket_t *sock, const _z_endpoint_t *endpoint, bool connect) {
    _z_serial_endpoint_cfg_t cfg;
    const _z_rawio_ops_t *ops = _z_serial_rawio_ops();
    z_result_t ret = _Z_RES_OK;

    if (ops == NULL) {
        return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    ret = _z_serial_endpoint_parse(&cfg, endpoint);
    if (ret != _Z_RES_OK) {
        _Z_ERROR_LOG(_Z_ERR_CONFIG_LOCATOR_INVALID);
        return ret;
    }

    if (cfg._from_pins) {
        ret = connect ? _z_rawio_open_from_pins(ops, &sock->_sock, cfg._txpin, cfg._rxpin, cfg._baudrate)
                      : _z_rawio_listen_from_pins(ops, &sock->_sock, cfg._txpin, cfg._rxpin, cfg._baudrate);
    } else {
        ret = connect ? _z_rawio_open_from_dev(ops, &sock->_sock, cfg._dev, cfg._baudrate)
                      : _z_rawio_listen_from_dev(ops, &sock->_sock, cfg._dev, cfg._baudrate);
    }

    if (ret != _Z_RES_OK || !connect) {
        _z_serial_endpoint_cfg_clear(&cfg);
        return ret;
    }

    ret = _z_connect_serial(sock->_sock);
    if (ret != _Z_RES_OK) {
        _z_rawio_close(ops, &sock->_sock);
    }

    _z_serial_endpoint_cfg_clear(&cfg);
    return ret;
}

z_result_t _z_serial_open(_z_serial_socket_t *sock, const _z_endpoint_t *endpoint) {
    return _z_serial_open_impl(sock, endpoint, true);
}

z_result_t _z_serial_listen(_z_serial_socket_t *sock, const _z_endpoint_t *endpoint) {
    return _z_serial_open_impl(sock, endpoint, false);
}

void _z_serial_close(_z_serial_socket_t *sock) {
    const _z_rawio_ops_t *ops = _z_serial_rawio_ops();
    if (ops != NULL) {
        _z_rawio_close(ops, &sock->_sock);
    }
}
#else
z_result_t _z_serial_open(_z_serial_socket_t *sock, const _z_endpoint_t *endpoint) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

z_result_t _z_serial_listen(_z_serial_socket_t *sock, const _z_endpoint_t *endpoint) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(endpoint);
    return _Z_ERR_TRANSPORT_NOT_AVAILABLE;
}

void _z_serial_close(_z_serial_socket_t *sock) { _ZP_UNUSED(sock); }
#endif

z_result_t _z_connect_serial(const _z_sys_net_socket_t sock) {
    while (true) {
        uint8_t header = _Z_FLAG_SERIAL_INIT;

        _z_send_serial_internal(sock, header, NULL, 0);
        uint8_t tmp;
        size_t ret = _z_read_serial_internal(sock, &header, &tmp, sizeof(tmp));
        if (ret == SIZE_MAX) {
            _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_RX_FAILED);
        }

        if (_Z_HAS_FLAG(header, _Z_FLAG_SERIAL_ACK) && _Z_HAS_FLAG(header, _Z_FLAG_SERIAL_INIT)) {
            _Z_DEBUG("connected");
            break;
        } else if (_Z_HAS_FLAG(header, _Z_FLAG_SERIAL_RESET)) {
            z_sleep_ms(SERIAL_CONNECT_THROTTLE_TIME_MS);
            _Z_DEBUG("reset");
            continue;
        } else {
            _Z_ERROR("unknown header received: %02X", header);
            _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_RX_FAILED);
        }
    }

    return _Z_RES_OK;
}

size_t _z_read_serial(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    uint8_t header;
    return _z_read_serial_internal(sock, &header, ptr, len);
}

size_t _z_send_serial(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return _z_send_serial_internal(sock, 0, ptr, len);
}

size_t _z_read_exact_serial(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t n = 0;

    do {
        size_t rb = _z_read_serial(sock, ptr, len - n);
        if (rb == SIZE_MAX) {
            n = rb;
            break;
        }

        n += rb;
    } while (n != len);

    return n;
}
#endif /* Z_FEATURE_LINK_SERIAL == 1 */
