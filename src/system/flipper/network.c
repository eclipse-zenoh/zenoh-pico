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

#include "zenoh-pico/config.h"
#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_LINK_TCP == 1
#error "TCP is not supported on Flipper port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
#error "UDP is not supported on Flipper port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1

/*------------------ Serial sockets ------------------*/

static void _z_serial_received_byte_callback(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* context) {
    if (!context) {
        return;
    }
    FuriStreamBuffer* buf = context;

    if (event == FuriHalSerialRxEventData) {
        uint8_t data = furi_hal_serial_async_rx(handle);
        furi_stream_buffer_send(buf, (void*)&data, 1, FLIPPER_SERIAL_STREAM_TRIGGERED_LEVEL);
    }
}

z_result_t _z_open_serial_from_pins(_z_sys_net_socket_t* sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    return _Z_ERR_GENERIC;
}

z_result_t _z_open_serial_from_dev(_z_sys_net_socket_t* sock, char* dev, uint32_t baudrate) {
    if (furi_hal_serial_control_is_busy(FuriHalSerialIdUsart)) {
        _Z_ERROR("Serial port is busy");
        return _Z_ERR_TRANSPORT_OPEN_FAILED;
    }

    FuriHalSerialId sid;
    if (!strcmp(dev, "usart")) {
        sid = FuriHalSerialIdUsart;
    } else if (!strcmp(dev, "lpuart")) {
        sid = FuriHalSerialIdLpuart;
    } else {
        _Z_ERROR("Unknown serial port device: %s", dev);
        return _Z_ERR_TRANSPORT_OPEN_FAILED;
    }

    sock->_serial = furi_hal_serial_control_acquire(sid);
    if (!sock->_serial) {
        _Z_ERROR("Serial port control acquire failed");
        return _Z_ERR_TRANSPORT_OPEN_FAILED;
    };
    furi_hal_serial_init(sock->_serial, baudrate);

    sock->_rx_stream =
        furi_stream_buffer_alloc(FLIPPER_SERIAL_STREAM_BUFFER_SIZE, FLIPPER_SERIAL_STREAM_TRIGGERED_LEVEL);
    if (!sock->_rx_stream) {
        _Z_ERROR("Serial stream buffer allocation failed");
        return _Z_ERR_TRANSPORT_NO_SPACE;
    };
    furi_hal_serial_async_rx_start(sock->_serial, _z_serial_received_byte_callback, sock->_rx_stream, false);

    _Z_DEBUG("Serial port opened: %s (%li)", dev, baudrate);

    sock->tmp_buf = (uint8_t*)z_malloc(_Z_SERIAL_MFS_SIZE);
    sock->raw_buf = (uint8_t*)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);

    return _z_connect_serial(*sock);
}

z_result_t _z_listen_serial_from_pins(_z_sys_net_socket_t* sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    (void)(sock);
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    return _Z_ERR_GENERIC;
}

z_result_t _z_listen_serial_from_dev(_z_sys_net_socket_t* sock, char* dev, uint32_t baudrate) {
    (void)(sock);
    (void)(dev);
    (void)(baudrate);

    // @TODO: To be implemented

    return _Z_ERR_GENERIC;
}

void _z_close_serial(_z_sys_net_socket_t* sock) {
    if (sock->_serial) {
        furi_hal_serial_async_rx_stop(sock->_serial);
        furi_hal_serial_deinit(sock->_serial);
        furi_hal_serial_control_release(sock->_serial);

        // Wait until the serial read timeout ends
        z_sleep_ms(FLIPPER_SERIAL_TIMEOUT_MS * 2);

        furi_stream_buffer_free(sock->_rx_stream);

        sock->_serial = 0;
        sock->_rx_stream = 0;

        z_free(sock->tmp_buf);
        z_free(sock->raw_buf);
    }
    _Z_DEBUG("Serial port closed");
}

size_t _z_read_serial_internal(const _z_sys_net_socket_t sock, uint8_t* header, uint8_t* ptr, size_t len) {
    size_t rb = 0;
    for (size_t i = 0; i < _Z_SERIAL_MAX_COBS_BUF_SIZE; i++) {
        size_t len = 0;
        len = furi_stream_buffer_receive(sock._rx_stream, &sock.raw_buf[i], 1, FLIPPER_SERIAL_TIMEOUT_MS);
        if (!len) {
            return SIZE_MAX;
        }
        rb++;

        if (sock.raw_buf[i] == (uint8_t)0x00) {
            break;
        }
    }

    return _z_serial_msg_deserialize(sock.raw_buf, rb, ptr, len, header, sock.tmp_buf, _Z_SERIAL_MFS_SIZE);
}

size_t _z_send_serial_internal(const _z_sys_net_socket_t sock, uint8_t header, const uint8_t* ptr, size_t len) {
    size_t ret = _z_serial_msg_serialize(sock.raw_buf, _Z_SERIAL_MAX_COBS_BUF_SIZE, ptr, len, header, sock.tmp_buf,
                                         _Z_SERIAL_MFS_SIZE);

    if (ret == SIZE_MAX) {
        return ret;
    }

    furi_hal_serial_tx(sock._serial, sock.raw_buf, ret);
    furi_hal_serial_tx_wait_complete(sock._serial);

    return len;
}
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported on Flipper port of Zenoh-Pico"
#endif
