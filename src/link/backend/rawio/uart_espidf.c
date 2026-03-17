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

#if Z_FEATURE_LINK_SERIAL == 1 && defined(ZENOH_ESPIDF)

#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

#include "zenoh-pico/utils/logging.h"

static z_result_t _z_espidf_uart_map_dev(const char *dev, uart_port_t *serial, uint32_t *txpin, uint32_t *rxpin) {
    if (strcmp(dev, "UART_0") == 0) {
        *serial = UART_NUM_0;
        *txpin = 1;
        *rxpin = 3;
    } else if (strcmp(dev, "UART_1") == 0) {
        *serial = UART_NUM_1;
        *txpin = 10;
        *rxpin = 9;
    } else if (strcmp(dev, "UART_2") == 0) {
        *serial = UART_NUM_2;
        *txpin = 17;
        *rxpin = 16;
    } else {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static z_result_t _z_espidf_uart_map_pins(uint32_t txpin, uint32_t rxpin, uart_port_t *serial) {
    if (txpin == 1 && rxpin == 3) {
        *serial = UART_NUM_0;
    } else if (txpin == 10 && rxpin == 9) {
        *serial = UART_NUM_1;
    } else if (txpin == 17 && rxpin == 16) {
        *serial = UART_NUM_2;
    } else {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static z_result_t _z_espidf_uart_open_impl(_z_sys_net_socket_t *sock, uart_port_t serial, uint32_t txpin,
                                           uint32_t rxpin, uint32_t baudrate) {
    if (sock == NULL || baudrate == 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    sock->_serial = serial;

    const uart_config_t config = {
        .baud_rate = (int)baudrate,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    if (uart_param_config(sock->_serial, &config) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (uart_set_pin(sock->_serial, (int)txpin, (int)rxpin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    const int uart_buffer_size = 1024 * 2;
    QueueHandle_t uart_queue = NULL;
    if (uart_driver_install(sock->_serial, uart_buffer_size, 0, 100, &uart_queue, 0) != 0) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    uart_flush_input(sock->_serial);

    return _Z_RES_OK;
}

static z_result_t _z_espidf_uart_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                uint32_t baudrate) {
    uart_port_t serial = UART_NUM_0;
    _Z_RETURN_IF_ERR(_z_espidf_uart_map_pins(txpin, rxpin, &serial));
    return _z_espidf_uart_open_impl(sock, serial, txpin, rxpin, baudrate);
}

static z_result_t _z_espidf_uart_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    uart_port_t serial = UART_NUM_0;
    uint32_t txpin = 0;
    uint32_t rxpin = 0;
    _Z_RETURN_IF_ERR(_z_espidf_uart_map_dev(dev, &serial, &txpin, &rxpin));
    return _z_espidf_uart_open_impl(sock, serial, txpin, rxpin, baudrate);
}

static z_result_t _z_espidf_uart_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                  uint32_t baudrate) {
    return _z_espidf_uart_open_from_pins(sock, txpin, rxpin, baudrate);
}

static z_result_t _z_espidf_uart_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_espidf_uart_open_from_dev(sock, dev, baudrate);
}

static void _z_espidf_uart_close(_z_sys_net_socket_t *sock) {
    if (sock != NULL) {
        uart_wait_tx_done(sock->_serial, 1000 / portTICK_PERIOD_MS);
        uart_flush(sock->_serial);
        uart_driver_delete(sock->_serial);
    }
}

static size_t _z_espidf_uart_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    int rb = uart_read_bytes(sock._serial, ptr, len, 1000 / portTICK_PERIOD_MS);
    if (rb <= 0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_espidf_uart_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    int wb = uart_write_bytes(sock._serial, ptr, len);
    if (wb <= 0) {
        return SIZE_MAX;
    }
    return (size_t)wb;
}

const _z_rawio_ops_t _z_uart_espidf_rawio_ops = {
    .open_from_pins = _z_espidf_uart_open_from_pins,
    .open_from_dev = _z_espidf_uart_open_from_dev,
    .listen_from_pins = _z_espidf_uart_listen_from_pins,
    .listen_from_dev = _z_espidf_uart_listen_from_dev,
    .close = _z_espidf_uart_close,
    .read = _z_espidf_uart_read,
    .write = _z_espidf_uart_write,
};

#endif /* Z_FEATURE_LINK_SERIAL == 1 && defined(ZENOH_ESPIDF) */
