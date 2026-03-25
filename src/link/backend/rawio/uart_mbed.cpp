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

#include "zenoh-pico/config.h"

#if defined(ZENOH_MBED)

#include <mbed.h>

extern "C" {
#include <stddef.h>

#include "zenoh-pico/link/backend/rawio.h"
#include "zenoh-pico/utils/logging.h"

static z_result_t _z_uart_mbed_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                              uint32_t baudrate) {
    sock->_serial = new BufferedSerial(PinName(txpin), PinName(rxpin), baudrate);
    if (sock->_serial == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock->_serial->set_format(8, BufferedSerial::None, 1);
    sock->_serial->enable_input(true);
    sock->_serial->enable_output(true);
    return _Z_RES_OK;
}

static z_result_t _z_uart_mbed_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(dev);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static z_result_t _z_uart_mbed_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                uint32_t baudrate) {
    return _z_uart_mbed_open_from_pins(sock, txpin, rxpin, baudrate);
}

static z_result_t _z_uart_mbed_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(dev);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static void _z_uart_mbed_close(_z_sys_net_socket_t *sock) {
    if (sock != NULL && sock->_serial != NULL) {
        delete sock->_serial;
        sock->_serial = NULL;
    }
}

static size_t _z_uart_mbed_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    // flawfinder: ignore
    ssize_t rb = sock._serial->read(ptr, len);
    if (rb <= 0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_uart_mbed_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    ssize_t wb = sock._serial->write(ptr, len);
    if (wb <= 0) {
        return SIZE_MAX;
    }
    return (size_t)wb;
}

extern const _z_rawio_ops_t _z_uart_mbed_rawio_ops = {
    _z_uart_mbed_open_from_pins,
    _z_uart_mbed_open_from_dev,
    _z_uart_mbed_listen_from_pins,
    _z_uart_mbed_listen_from_dev,
    _z_uart_mbed_close,
    _z_uart_mbed_read,
    _z_uart_mbed_write,
};
}  // extern "C"

#endif /* defined(ZENOH_MBED) */
