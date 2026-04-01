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

#if defined(ZENOH_ARDUINO_ESP32)

#include <Arduino.h>
#include <string.h>

extern "C" {
#include "zenoh-pico/link/backend/serial.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_LINK_SERIAL == 1

static z_result_t _z_uart_arduino_esp32_map_pins(uint32_t txpin, uint32_t rxpin, uint8_t *uart) {
    if (txpin == 1 && rxpin == 3) {
        *uart = 0;
    } else if (txpin == 10 && rxpin == 9) {
        *uart = 1;
    } else if (txpin == 17 && rxpin == 16) {
        *uart = 2;
    } else {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static z_result_t _z_uart_arduino_esp32_map_dev(const char *dev, uint8_t *uart, uint32_t *txpin, uint32_t *rxpin) {
    if (strcmp(dev, "UART_0") == 0) {
        *uart = 0;
        *txpin = 1;
        *rxpin = 3;
    } else if (strcmp(dev, "UART_1") == 0) {
        *uart = 1;
        *txpin = 10;
        *rxpin = 9;
    } else if (strcmp(dev, "UART_2") == 0) {
        *uart = 2;
        *txpin = 17;
        *rxpin = 16;
    } else {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static z_result_t _z_uart_arduino_esp32_open_impl(_z_sys_net_socket_t *sock, uint8_t uart, uint32_t txpin,
                                                  uint32_t rxpin, uint32_t baudrate) {
    if (sock == NULL || baudrate == 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    // Keep the lines high before the UART is initialized to reduce the initial glitch.
    pinMode((int)rxpin, INPUT_PULLUP);
    pinMode((int)txpin, OUTPUT);
    digitalWrite((int)txpin, HIGH);

    sock->_serial = new HardwareSerial(uart);
    if (sock->_serial == NULL) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock->_serial->begin(baudrate);
    sock->_serial->flush();
    return _Z_RES_OK;
}

static z_result_t _z_uart_arduino_esp32_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                       uint32_t baudrate) {
    uint8_t uart = 0;
    _Z_RETURN_IF_ERR(_z_uart_arduino_esp32_map_pins(txpin, rxpin, &uart));
    return _z_uart_arduino_esp32_open_impl(sock, uart, txpin, rxpin, baudrate);
}

static z_result_t _z_uart_arduino_esp32_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    uint8_t uart = 0;
    uint32_t txpin = 0;
    uint32_t rxpin = 0;
    _Z_RETURN_IF_ERR(_z_uart_arduino_esp32_map_dev(dev, &uart, &txpin, &rxpin));
    return _z_uart_arduino_esp32_open_impl(sock, uart, txpin, rxpin, baudrate);
}

static z_result_t _z_uart_arduino_esp32_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                         uint32_t baudrate) {
    return _z_uart_arduino_esp32_open_from_pins(sock, txpin, rxpin, baudrate);
}

static z_result_t _z_uart_arduino_esp32_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_uart_arduino_esp32_open_from_dev(sock, dev, baudrate);
}

static void _z_uart_arduino_esp32_close(_z_sys_net_socket_t *sock) {
    if (sock != NULL && sock->_serial != NULL) {
        sock->_serial->end();
        delete sock->_serial;
        sock->_serial = NULL;
    }
}

static size_t _z_uart_arduino_esp32_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t rb = 0;
    while (rb < len) {
        while (sock._serial->available() < 1) {
            z_sleep_ms(1);
        }
        // flawfinder: ignore
        int byte = sock._serial->read();
        if (byte < 0) {
            return SIZE_MAX;
        }
        ptr[rb++] = (uint8_t)byte;
    }

    return rb;
}

static size_t _z_uart_arduino_esp32_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    size_t wb = sock._serial->write(ptr, len);
    if (wb == 0) {
        return SIZE_MAX;
    }
    return wb;
}

extern const _z_serial_ops_t _z_uart_arduino_esp32_serial_ops = {
    _z_uart_arduino_esp32_open_from_pins,
    _z_uart_arduino_esp32_open_from_dev,
    _z_uart_arduino_esp32_listen_from_pins,
    _z_uart_arduino_esp32_listen_from_dev,
    _z_uart_arduino_esp32_close,
    _z_uart_arduino_esp32_read,
    _z_uart_arduino_esp32_write,
};

#endif /* Z_FEATURE_LINK_SERIAL == 1 */
}  // extern "C"

#endif /* defined(ZENOH_ARDUINO_ESP32) */
