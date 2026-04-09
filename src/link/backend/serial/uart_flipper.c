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

#if defined(ZENOH_FLIPPER) && (Z_FEATURE_LINK_SERIAL == 1)

#include <string.h>

#include "zenoh-pico/utils/logging.h"

static void _z_uart_flipper_received_byte_callback(FuriHalSerialHandle *handle, FuriHalSerialRxEvent event,
                                                   void *context) {
    if (context == NULL) {
        return;
    }

    if (event == FuriHalSerialRxEventData) {
        uint8_t data = furi_hal_serial_async_rx(handle);
        furi_stream_buffer_send((FuriStreamBuffer *)context, &data, 1, FLIPPER_SERIAL_STREAM_TRIGGERED_LEVEL);
    }
}

static z_result_t _z_uart_flipper_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                 uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(txpin);
    _ZP_UNUSED(rxpin);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static z_result_t _z_uart_flipper_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    if (furi_hal_serial_control_is_busy(FuriHalSerialIdUsart)) {
        _Z_ERROR("Serial port is busy");
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_OPEN_FAILED);
    }

    FuriHalSerialId sid;
    if (!strcmp(dev, "usart")) {
        sid = FuriHalSerialIdUsart;
    } else if (!strcmp(dev, "lpuart")) {
        sid = FuriHalSerialIdLpuart;
    } else {
        _Z_ERROR("Unknown serial port device: %s", dev);
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_OPEN_FAILED);
    }

    sock->_serial = furi_hal_serial_control_acquire(sid);
    if (sock->_serial == NULL) {
        _Z_ERROR("Serial port control acquire failed");
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_OPEN_FAILED);
    }

    furi_hal_serial_init(sock->_serial, baudrate);
    sock->_rx_stream =
        furi_stream_buffer_alloc(FLIPPER_SERIAL_STREAM_BUFFER_SIZE, FLIPPER_SERIAL_STREAM_TRIGGERED_LEVEL);
    if (sock->_rx_stream == NULL) {
        _Z_ERROR("Serial stream buffer allocation failed");
        _Z_ERROR_RETURN(_Z_ERR_TRANSPORT_NO_SPACE);
    }

    furi_hal_serial_async_rx_start(sock->_serial, _z_uart_flipper_received_byte_callback, sock->_rx_stream, false);
    _Z_DEBUG("Serial port opened: %s (%li)", dev, baudrate);
    return _Z_RES_OK;
}

static z_result_t _z_uart_flipper_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                   uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(txpin);
    _ZP_UNUSED(rxpin);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static z_result_t _z_uart_flipper_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(dev);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static void _z_uart_flipper_close(_z_sys_net_socket_t *sock) {
    if (sock->_serial != NULL) {
        furi_hal_serial_async_rx_stop(sock->_serial);
        furi_hal_serial_deinit(sock->_serial);
        furi_hal_serial_control_release(sock->_serial);
        z_sleep_ms(FLIPPER_SERIAL_TIMEOUT_MS * 2);
        furi_stream_buffer_free(sock->_rx_stream);
        sock->_serial = NULL;
        sock->_rx_stream = NULL;
    }
    _Z_DEBUG("Serial port closed");
}

static size_t _z_uart_flipper_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    for (size_t i = 0; i < len; i++) {
        size_t received = furi_stream_buffer_receive(sock._rx_stream, &ptr[i], 1, FLIPPER_SERIAL_TIMEOUT_MS);
        if (received != 1) {
            return SIZE_MAX;
        }
    }
    return len;
}

static size_t _z_uart_flipper_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    furi_hal_serial_tx(sock._serial, ptr, len);
    furi_hal_serial_tx_wait_complete(sock._serial);
    return len;
}

z_result_t _z_serial_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    return _z_uart_flipper_open_from_pins(sock, txpin, rxpin, baudrate);
}

z_result_t _z_serial_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_uart_flipper_open_from_dev(sock, dev, baudrate);
}

z_result_t _z_serial_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    return _z_uart_flipper_listen_from_pins(sock, txpin, rxpin, baudrate);
}

z_result_t _z_serial_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_uart_flipper_listen_from_dev(sock, dev, baudrate);
}

void _z_serial_close(_z_sys_net_socket_t *sock) { _z_uart_flipper_close(sock); }

size_t _z_serial_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_uart_flipper_read(sock, ptr, len);
}

size_t _z_serial_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return _z_uart_flipper_write(sock, ptr, len);
}

#endif /* defined(ZENOH_FLIPPER) && (Z_FEATURE_LINK_SERIAL == 1) */
