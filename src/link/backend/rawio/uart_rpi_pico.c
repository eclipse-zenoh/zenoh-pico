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

#include "zenoh-pico/link/backend/rawio.h"

#if defined(ZENOH_RPI_PICO) && (Z_FEATURE_LINK_SERIAL == 1)

#include <string.h>

#include "zenoh-pico/utils/logging.h"

typedef struct {
    uint32_t tx_pin;
    uint32_t rx_pin;
    uart_inst_t *uart;
    const char *alias;
} _z_uart_rpi_pico_pins_t;

static const _z_uart_rpi_pico_pins_t _z_uart_rpi_pico_allowed_pins[] = {{0, 1, uart0, "uart0_0"},
                                                                        {4, 5, uart1, "uart1_0"},
                                                                        {8, 9, uart1, "uart1_1"},
                                                                        {12, 13, uart0, "uart0_1"},
                                                                        {16, 17, uart0, "uart0_2"}};

#define _Z_UART_RPI_PICO_NUM_PIN_COMBINATIONS \
    (sizeof(_z_uart_rpi_pico_allowed_pins) / sizeof(_z_uart_rpi_pico_allowed_pins[0]))

static void _z_uart_rpi_pico_open_impl(uart_inst_t *uart, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    uart_init(uart, baudrate);
    gpio_set_function(txpin, UART_FUNCSEL_NUM(uart, txpin));
    gpio_set_function(rxpin, UART_FUNCSEL_NUM(uart, rxpin));
    uart_set_format(uart, 8, 1, UART_PARITY_NONE);
}

static z_result_t _z_uart_rpi_pico_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                  uint32_t baudrate) {
    if (sock == NULL || baudrate == 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    sock->_serial = NULL;
    for (size_t i = 0; i < _Z_UART_RPI_PICO_NUM_PIN_COMBINATIONS; i++) {
        if (_z_uart_rpi_pico_allowed_pins[i].tx_pin == txpin && _z_uart_rpi_pico_allowed_pins[i].rx_pin == rxpin) {
            sock->_serial = _z_uart_rpi_pico_allowed_pins[i].uart;
            break;
        }
    }

    if (sock->_serial == NULL) {
        _Z_ERROR("invalid pin combination");
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _z_uart_rpi_pico_open_impl(sock->_serial, txpin, rxpin, baudrate);
    return _Z_RES_OK;
}

static z_result_t _z_uart_rpi_pico_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    sock->_serial = NULL;

#if Z_FEATURE_LINK_SERIAL_USB == 1
    if (strcmp("usb", dev) == 0) {
        _z_usb_uart_init();
        return _Z_RES_OK;
    }
#endif

    uint32_t txpin = 0;
    uint32_t rxpin = 0;
    for (size_t i = 0; i < _Z_UART_RPI_PICO_NUM_PIN_COMBINATIONS; i++) {
        if (strcmp(_z_uart_rpi_pico_allowed_pins[i].alias, dev) == 0) {
            sock->_serial = _z_uart_rpi_pico_allowed_pins[i].uart;
            txpin = _z_uart_rpi_pico_allowed_pins[i].tx_pin;
            rxpin = _z_uart_rpi_pico_allowed_pins[i].rx_pin;
            break;
        }
    }

    if (sock->_serial == NULL) {
        _Z_ERROR("invalid device name");
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    _z_uart_rpi_pico_open_impl(sock->_serial, txpin, rxpin, baudrate);
    return _Z_RES_OK;
}

static z_result_t _z_uart_rpi_pico_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                    uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(txpin);
    _ZP_UNUSED(rxpin);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

static z_result_t _z_uart_rpi_pico_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(dev);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    return _Z_ERR_GENERIC;
}

static void _z_uart_rpi_pico_close(_z_sys_net_socket_t *sock) {
    if (sock->_serial != NULL) {
        uart_deinit(sock->_serial);
    } else {
#if Z_FEATURE_LINK_SERIAL_USB == 1
        _z_usb_uart_deinit();
#endif
    }
}

static size_t _z_uart_rpi_pico_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    for (size_t i = 0; i < len; i++) {
#if Z_FEATURE_LINK_SERIAL_USB == 1
        ptr[i] = (sock._serial == NULL) ? _z_usb_uart_getc() : uart_getc(sock._serial);
#else
        ptr[i] = uart_getc(sock._serial);
#endif
    }
    return len;
}

static size_t _z_uart_rpi_pico_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    if (sock._serial == NULL) {
#if Z_FEATURE_LINK_SERIAL_USB == 1
        _z_usb_uart_write(ptr, (int)len);
#endif
    } else {
        uart_write_blocking(sock._serial, ptr, len);
    }
    return len;
}

const _z_rawio_ops_t _z_uart_rpi_pico_rawio_ops = {
    .open_from_pins = _z_uart_rpi_pico_open_from_pins,
    .open_from_dev = _z_uart_rpi_pico_open_from_dev,
    .listen_from_pins = _z_uart_rpi_pico_listen_from_pins,
    .listen_from_dev = _z_uart_rpi_pico_listen_from_dev,
    .close = _z_uart_rpi_pico_close,
    .read = _z_uart_rpi_pico_read,
    .write = _z_uart_rpi_pico_write,
};

#endif /* defined(ZENOH_RPI_PICO) && (Z_FEATURE_LINK_SERIAL == 1) */
