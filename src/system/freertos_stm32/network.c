//
// Copyright (c) 2022 ZettaScale Technology
// Copyright (c) 2023 Fictionlab sp. z o.o.
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
//   Błażej Sowa, <blazej@fictionlab.pl>

#include <stdlib.h>

#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/transport.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"
#include "zenoh-pico/utils/result.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "stream_buffer.h"
extern StreamBufferHandle_t xSerialRxStreamBuffer;
extern StreamBufferHandle_t xSerialTxStreamBuffer;

// STM32 includes
#include "usart.h"

#if Z_FEATURE_LINK_TCP == 1
#error "TCP transport not supported yet on FreeRTOS-STM32 port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1
#error "UDP transport not supported yet on FreeRTOS-STM32 port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_UDP_MULTICAST == 1
#error "UDP Multicast not supported yet on FreeRTOS-STM32 port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on FreeRTOS-STM32 port of Zenoh-Pico"
#endif

#if Z_FEATURE_LINK_SERIAL == 1

static uint8_t _r_after_cobs [_Z_SERIAL_MAX_COBS_BUF_SIZE];
static uint8_t _r_before_cobs[_Z_SERIAL_MFS_SIZE];
static uint8_t _w_after_cobs [_Z_SERIAL_MFS_SIZE];
static uint8_t _w_before_cobs[_Z_SERIAL_MAX_COBS_BUF_SIZE];

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) {
	return _Z_RES_OK;
}

z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out) {
    _ZP_UNUSED(sock_in);
    _ZP_UNUSED(sock_out);
    _Z_ERROR("Function not yet supported on this system");
	return _Z_ERR_GENERIC;
}

void _z_socket_close(_z_sys_net_socket_t *sock) { _ZP_UNUSED(sock); }

z_result_t _z_socket_wait_event(void *v_peers, _z_mutex_rec_t *mutex) {

	size_t avail;
	TickType_t startTime = xTaskGetTickCount();
	do {
		if ((avail = xStreamBufferBytesAvailable(xSerialRxStreamBuffer)) > 0) {
			break;
		}
		vTaskDelay(1);
	} while (xTaskGetTickCount() - startTime < Z_CONFIG_SOCKET_TIMEOUT);

	_z_transport_unicast_peer_list_t **peers = (_z_transport_unicast_peer_list_t **)v_peers;
    _z_mutex_rec_lock(mutex);
    _z_transport_unicast_peer_list_t *curr = *peers;
    while (curr != NULL) {
        _z_transport_unicast_peer_t *peer = _z_transport_unicast_peer_list_head(curr);
        if (avail > 0) {
            peer->_pending = true;
        }
        curr = _z_transport_unicast_peer_list_tail(curr);
    }
    _z_mutex_rec_unlock(mutex);
    return _Z_RES_OK;
}

int8_t _z_open_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;
    (void)(sock);
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

int8_t _z_open_serial_from_dev(_z_sys_net_socket_t *sock, char *dev, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;

    if (strcmp(dev, "ZUART") == 0) {
        sock->_serial = Z_UART;
    } else {
        return _Z_ERR_GENERIC;
    }

	if (sock->_serial->Init.BaudRate != baudrate) {
        return _Z_ERR_GENERIC;
	}

	while (xStreamBufferReceive(xSerialRxStreamBuffer, &_r_before_cobs[0], _Z_SERIAL_MFS_SIZE, 0) > 0);

    return ret;
}

int8_t _z_listen_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;
    (void)(sock);
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    // @TODO: To be implemented
    ret = _Z_ERR_GENERIC;

    return ret;
}

int8_t _z_listen_serial_from_dev(_z_sys_net_socket_t *sock, char *dev, uint32_t baudrate) {
    int8_t ret = _Z_RES_OK;
    (void)(sock);
    (void)(dev);
    (void)(baudrate);

    // @TODO: To be implemented
    // ret = _Z_ERR_GENERIC;
    ret = _z_open_serial_from_dev(sock, dev, baudrate);

    return ret;
}

void _z_close_serial(_z_sys_net_socket_t *sock) {

}

size_t _z_read_serial_internal(const _z_sys_net_socket_t sock, uint8_t *header, uint8_t *ptr, size_t len) {
	int8_t ret = _Z_RES_OK;

    size_t rb = 0;
    while (rb < _Z_SERIAL_MAX_COBS_BUF_SIZE) {
    	int r = xStreamBufferReceive(xSerialRxStreamBuffer, &_r_before_cobs[rb], 1, pdMS_TO_TICKS(1000));
        if (r == 0) {
            _Z_DEBUG("Timeout reading from serial");
            if (rb == 0) {
                ret = _Z_ERR_GENERIC;
            }
            break;
        } else if (r == 1) {
            rb = rb + (size_t)1;
            if (_r_before_cobs[rb - 1] == (uint8_t)0x00) {
                break;
            }
        } else {
            _Z_ERROR("Error reading from serial");
            ret = _Z_ERR_GENERIC;
        }
    }
    uint16_t payload_len = 0;

    if (ret == _Z_RES_OK) {
        _Z_DEBUG("Read %u bytes from serial", rb);
        size_t trb = _z_cobs_decode(_r_before_cobs, rb, _r_after_cobs);
        _Z_DEBUG("Decoded %u bytes from serial", trb);
        size_t i = 0;
        for (i = 0; i < sizeof(payload_len); i++) {
            payload_len |= (_r_after_cobs[i] << ((uint8_t)i * (uint8_t)8));
        }
        _Z_DEBUG("payload_len = %u <= %X %X", payload_len, sock.after_cobs[1], sock.after_cobs[0]);

        if (trb == (size_t)(payload_len + (uint16_t)6)) {
            (void)memcpy(ptr, &_r_after_cobs[i], payload_len);
            i = i + (size_t)payload_len;

            uint32_t crc = 0;
            for (uint8_t j = 0; j < sizeof(crc); j++) {
                crc |= (uint32_t)(_r_after_cobs[i] << (j * (uint8_t)8));
                i = i + (size_t)1;
            }

            uint32_t c_crc = _z_crc32(ptr, payload_len);
            if (c_crc != crc) {
                _Z_ERROR("CRC mismatch: %d != %d ", c_crc, crc);
                ret = _Z_ERR_GENERIC;
            }
        } else {
            _Z_ERROR("length mismatch => %d <> %d ", trb, payload_len + (uint16_t)6);
            ret = _Z_ERR_GENERIC;
        }
    }

    rb = payload_len;
    if (ret != _Z_RES_OK) {
        rb = SIZE_MAX;
    }
    _Z_DEBUG("return _z_read_serial() = %d ", rb);
    return rb;
}

size_t _z_send_serial_internal(const _z_sys_net_socket_t sock, uint8_t header, const uint8_t *ptr, size_t len) {
    int8_t ret = _Z_RES_OK;
    
    size_t i = 0;
    for (i = 0; i < sizeof(uint16_t); ++i) {
        _w_before_cobs[i] = (len >> (i * (size_t)8)) & (size_t)0XFF;
    }

    (void)memcpy(&_w_before_cobs[i], ptr, len);
    i = i + len;

    uint32_t crc = _z_crc32(ptr, len);
    for (uint8_t j = 0; j < sizeof(crc); j++) {
    	_w_before_cobs[i] = (crc >> (j * (uint8_t)8)) & (uint32_t)0XFF;
        i = i + (size_t)1;
    }

    size_t twb = _z_cobs_encode(_w_before_cobs, i, _w_after_cobs);
    _w_after_cobs[twb] = 0x00;  // Manually add the COBS delimiter
    size_t wb = xStreamBufferSend(xSerialTxStreamBuffer, _w_after_cobs, twb + (size_t)1, pdMS_TO_TICKS(1000));
    if (wb != (twb + (size_t)1)) {
        ret = _Z_ERR_GENERIC;
    }

    size_t sb = len;
    if (ret != _Z_RES_OK) {
        sb = SIZE_MAX;
    }
    
    return sb;
}
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on FreeRTOS-STM32 port of Zenoh-Pico"
#endif
