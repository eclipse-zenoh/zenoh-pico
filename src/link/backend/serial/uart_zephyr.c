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

#include "zenoh-pico/link/backend/serial.h"

#if Z_FEATURE_LINK_SERIAL == 1 && defined(ZENOH_ZEPHYR)

#include <version.h>

#if KERNEL_VERSION_MAJOR == 2
#include <drivers/uart.h>
#else
#include <zephyr/drivers/uart.h>
#endif

#include "zenoh-pico/utils/logging.h"

static z_result_t _z_zephyr_uart_open_impl(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    if (sock == NULL || dev == NULL || baudrate == 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    sock->_serial = device_get_binding(dev);
    if (sock->_serial == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    const struct uart_config config = {
        .baudrate = baudrate,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
    };
    if (uart_configure(sock->_serial, &config) != 0) {
        sock->_serial = NULL;
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static z_result_t _z_zephyr_uart_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(txpin);
    _ZP_UNUSED(rxpin);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static z_result_t _z_zephyr_uart_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_zephyr_uart_open_impl(sock, dev, baudrate);
}

static z_result_t _z_zephyr_uart_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                  uint32_t baudrate) {
    return _z_zephyr_uart_open_from_pins(sock, txpin, rxpin, baudrate);
}

static z_result_t _z_zephyr_uart_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_zephyr_uart_open_impl(sock, dev, baudrate);
}

static void _z_zephyr_uart_close(_z_sys_net_socket_t *sock) {
    if (sock != NULL) {
        sock->_serial = NULL;
    }
}

static size_t _z_zephyr_uart_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    for (size_t i = 0; i < len; i++) {
        int res = -1;
        while (res != 0) {
            res = uart_poll_in(sock._serial, &ptr[i]);
        }
    }

    return len;
}

static size_t _z_zephyr_uart_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uart_poll_out(sock._serial, ptr[i]);
    }

    return len;
}

z_result_t _z_serial_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    return _z_zephyr_uart_open_from_pins(sock, txpin, rxpin, baudrate);
}

z_result_t _z_serial_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_zephyr_uart_open_from_dev(sock, dev, baudrate);
}

z_result_t _z_serial_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    return _z_zephyr_uart_listen_from_pins(sock, txpin, rxpin, baudrate);
}

z_result_t _z_serial_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_zephyr_uart_listen_from_dev(sock, dev, baudrate);
}

void _z_serial_close(_z_sys_net_socket_t *sock) { _z_zephyr_uart_close(sock); }

size_t _z_serial_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_zephyr_uart_read(sock, ptr, len);
}

size_t _z_serial_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return _z_zephyr_uart_write(sock, ptr, len);
}

#endif /* Z_FEATURE_LINK_SERIAL == 1 && defined(ZENOH_ZEPHYR) */
