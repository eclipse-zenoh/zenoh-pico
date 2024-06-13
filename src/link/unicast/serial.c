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

#include "zenoh-pico/link/config/serial.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_LINK_SERIAL == 1

#define SPP_MAXIMUM_PAYLOAD 128

int8_t _z_endpoint_serial_valid(_z_endpoint_t *endpoint) {
    int8_t ret = _Z_RES_OK;

    if (_z_str_eq(endpoint->_locator._protocol, SERIAL_SCHEMA) != true) {
        ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
    }

    if (ret == _Z_RES_OK) {
        char *p_dot = strchr(endpoint->_locator._address, '.');
        if (p_dot != NULL) {
            if ((endpoint->_locator._address == p_dot) ||
                (strlen(p_dot) == (size_t)1)) {  // If dot is the first or last character
                ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
            }
        } else {
            if (strlen(endpoint->_locator._address) == (size_t)0) {
                ret = _Z_ERR_CONFIG_LOCATOR_INVALID;
            }
        }
    }

    return ret;
}

int8_t _z_f_link_open_serial(_z_link_t *self) {
    int8_t ret = _Z_RES_OK;

    const char *baudrate_str = _z_str_intmap_get(&self->_endpoint._config, SERIAL_CONFIG_BAUDRATE_KEY);
    uint32_t baudrate = strtoul(baudrate_str, NULL, 10);

    char *p_dot = strchr(self->_endpoint._locator._address, '.');
    if (p_dot != NULL) {
        uint32_t txpin = strtoul(self->_endpoint._locator._address, &p_dot, 10);
        p_dot = _z_ptr_char_offset(p_dot, 1);
        uint32_t rxpin = strtoul(p_dot, NULL, 10);
        ret = _z_open_serial_from_pins(&self->_socket._serial._sock, txpin, rxpin, baudrate);
    } else {
        ret = _z_open_serial_from_dev(&self->_socket._serial._sock, self->_endpoint._locator._address, baudrate);
    }

    return ret;
}

int8_t _z_f_link_listen_serial(_z_link_t *self) {
    int8_t ret = _Z_RES_OK;

    const char *baudrate_str = _z_str_intmap_get(&self->_endpoint._config, SERIAL_CONFIG_BAUDRATE_KEY);
    uint32_t baudrate = strtoul(baudrate_str, NULL, 10);

    char *p_dot = strchr(self->_endpoint._locator._address, '.');
    if (p_dot != NULL) {
        uint32_t txpin = strtoul(self->_endpoint._locator._address, &p_dot, 10);
        p_dot = _z_ptr_char_offset(p_dot, 1);
        uint32_t rxpin = strtoul(p_dot, NULL, 10);
        ret = _z_listen_serial_from_pins(&self->_socket._serial._sock, txpin, rxpin, baudrate);
    } else {
        ret = _z_listen_serial_from_dev(&self->_socket._serial._sock, self->_endpoint._locator._address, baudrate);
    }

    return ret;
}

void _z_f_link_close_serial(_z_link_t *self) { _z_close_serial(&self->_socket._serial._sock); }

void _z_f_link_free_serial(_z_link_t *self) { (void)(self); }

size_t _z_f_link_write_serial(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_write_all_serial(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_read_serial(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    (void)(addr);
    return _z_read_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_read_exact_serial(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    (void)(addr);
    return _z_read_exact_serial(self->_socket._serial._sock, ptr, len);
}

uint16_t _z_get_link_mtu_serial(void) { return _Z_SERIAL_MTU_SIZE; }

int8_t _z_new_link_serial(_z_link_t *zl, _z_endpoint_t endpoint) {
    int8_t ret = _Z_RES_OK;

    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    zl->_cap._is_reliable = false;

    zl->_mtu = _z_get_link_mtu_serial();

    zl->_endpoint = endpoint;

    zl->_open_f = _z_f_link_open_serial;
    zl->_listen_f = _z_f_link_listen_serial;
    zl->_close_f = _z_f_link_close_serial;
    zl->_free_f = _z_f_link_free_serial;

    zl->_write_f = _z_f_link_write_serial;
    zl->_write_all_f = _z_f_link_write_all_serial;
    zl->_read_f = _z_f_link_read_serial;
    zl->_read_exact_f = _z_f_link_read_exact_serial;

    return ret;
}
#endif
