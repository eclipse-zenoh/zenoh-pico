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

int8_t _z_open_serial_from_pins(_z_sys_net_socket_t* sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    return _Z_ERR_GENERIC;
}

int8_t _z_open_serial_from_dev(_z_sys_net_socket_t* sock, char* dev, uint32_t baudrate) {
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
    return _Z_RES_OK;
}

int8_t _z_listen_serial_from_pins(_z_sys_net_socket_t* sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    (void)(sock);
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    return _Z_ERR_GENERIC;
}

int8_t _z_listen_serial_from_dev(_z_sys_net_socket_t* sock, char* dev, uint32_t baudrate) {
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
    }
    _Z_DEBUG("Serial port closed");
}

size_t _z_read_serial(const _z_sys_net_socket_t sock, uint8_t* ptr, size_t len) {
    int8_t ret = _Z_RES_OK;

    uint8_t* before_cobs = (uint8_t*)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    size_t rb = 0;
    for (size_t i = 0; i < _Z_SERIAL_MAX_COBS_BUF_SIZE; i++) {
        size_t len = 0;
        len = furi_stream_buffer_receive(sock._rx_stream, &before_cobs[i], 1, FLIPPER_SERIAL_TIMEOUT_MS);
        if (!len) {
            z_free(before_cobs);
            return SIZE_MAX;
        }
        rb += 1;

        if (before_cobs[i] == (uint8_t)0x00) {
            break;
        }
    }

    uint8_t* after_cobs = (uint8_t*)z_malloc(_Z_SERIAL_MFS_SIZE);
    size_t trb = _z_cobs_decode(before_cobs, rb, after_cobs);

    size_t i = 0;
    uint16_t payload_len = 0;
    for (i = 0; i < sizeof(payload_len); i++) {
        payload_len |= (after_cobs[i] << ((uint8_t)i * (uint8_t)8));
    }

    if (trb == (size_t)(payload_len + (uint16_t)6)) {
        (void)memcpy(ptr, &after_cobs[i], payload_len);
        i += (size_t)payload_len;

        uint32_t crc = 0;
        for (uint8_t j = 0; j < sizeof(crc); j++) {
            crc |= (uint32_t)(after_cobs[i] << (j * (uint8_t)8));
            i += 1;
        }

        uint32_t c_crc = _z_crc32(ptr, payload_len);
        if (c_crc != crc) {
            ret = _Z_ERR_GENERIC;
        }
    } else {
        ret = _Z_ERR_GENERIC;
    }

    z_free(before_cobs);
    z_free(after_cobs);

    rb = payload_len;
    if (ret != _Z_RES_OK) {
        rb = SIZE_MAX;
    }

    return rb;
}

size_t _z_read_exact_serial(const _z_sys_net_socket_t sock, uint8_t* ptr, size_t len) {
    size_t n = len;

    do {
        size_t rb = _z_read_serial(sock, ptr, n);
        if (rb == SIZE_MAX) {
            return rb;
        }
        n -= rb;
        ptr += len - n;
    } while (n > 0);

    return len;
}

size_t _z_send_serial(const _z_sys_net_socket_t sock, const uint8_t* ptr, size_t len) {
    int8_t ret = _Z_RES_OK;

    uint8_t* before_cobs = (uint8_t*)z_malloc(_Z_SERIAL_MFS_SIZE);
    size_t i = 0;
    for (i = 0; i < sizeof(uint16_t); ++i) {
        before_cobs[i] = (len >> (i * (size_t)8)) & (size_t)0XFF;
    }

    (void)memcpy(&before_cobs[i], ptr, len);
    i += len;

    uint32_t crc = _z_crc32(ptr, len);
    for (uint8_t j = 0; j < sizeof(crc); j++) {
        before_cobs[i] = (crc >> (j * (uint8_t)8)) & (uint32_t)0XFF;
        i++;
    }

    uint8_t* after_cobs = (uint8_t*)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    ssize_t twb = _z_cobs_encode(before_cobs, i, after_cobs);
    after_cobs[twb] = 0x00;  // Manually add the COBS delimiter

    furi_hal_serial_tx(sock._serial, after_cobs, twb + (ssize_t)1);
    furi_hal_serial_tx_wait_complete(sock._serial);

    z_free(before_cobs);
    z_free(after_cobs);

    return len;
}
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported on Flipper port of Zenoh-Pico"
#endif
