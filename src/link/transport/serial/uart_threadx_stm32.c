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

#include "zenoh-pico/link/transport/serial.h"

#if Z_FEATURE_LINK_SERIAL == 1 && defined(ZENOH_THREADX_STM32)

#include <stdbool.h>
#include <string.h>

#include "hal.h"
#include "zenoh-pico/link/transport/serial_protocol.h"
#include "zenoh-pico/utils/logging.h"

#define RX_DMA_BUFFER_SIZE (_Z_SERIAL_MAX_COBS_BUF_SIZE * 2 + 2)

static uint8_t _z_threadx_stm32_dma_buffer[RX_DMA_BUFFER_SIZE];
static uint16_t _z_threadx_stm32_delimiter_offset = 0;
static TX_SEMAPHORE _z_threadx_stm32_data_ready_semaphore;
static TX_SEMAPHORE _z_threadx_stm32_data_processing_semaphore;
static uint8_t _z_threadx_stm32_frame_buffer[_Z_SERIAL_MAX_COBS_BUF_SIZE];
static size_t _z_threadx_stm32_frame_len = 0;
static size_t _z_threadx_stm32_frame_offset = 0;
static bool _z_threadx_stm32_initialized = false;
static uint16_t _z_threadx_stm32_last_dma_offset = 0;
extern UART_HandleTypeDef ZENOH_HUART;

static z_result_t _z_threadx_stm32_uart_open_impl(void) {
    if (!_z_threadx_stm32_initialized) {
        tx_semaphore_create(&_z_threadx_stm32_data_ready_semaphore, "Data Ready", 0);
        tx_semaphore_create(&_z_threadx_stm32_data_processing_semaphore, "Data Processing", 1);
        _z_threadx_stm32_initialized = true;
    }

    _z_threadx_stm32_delimiter_offset = 0;
    _z_threadx_stm32_last_dma_offset = 0;
    _z_threadx_stm32_frame_len = 0;
    _z_threadx_stm32_frame_offset = 0;
    memset(_z_threadx_stm32_dma_buffer, 0, sizeof(_z_threadx_stm32_dma_buffer));

    if (HAL_UARTEx_ReceiveToIdle_DMA(&ZENOH_HUART, _z_threadx_stm32_dma_buffer, RX_DMA_BUFFER_SIZE) != HAL_OK) {
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    return _Z_RES_OK;
}

static z_result_t _z_threadx_stm32_uart_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                       uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(txpin);
    _ZP_UNUSED(rxpin);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

static z_result_t _z_threadx_stm32_uart_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(dev);
    _ZP_UNUSED(baudrate);
    return _z_threadx_stm32_uart_open_impl();
}

static z_result_t _z_threadx_stm32_uart_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                         uint32_t baudrate) {
    return _z_threadx_stm32_uart_open_from_pins(sock, txpin, rxpin, baudrate);
}

static z_result_t _z_threadx_stm32_uart_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(dev);
    _ZP_UNUSED(baudrate);
    return _z_threadx_stm32_uart_open_impl();
}

static void _z_threadx_stm32_uart_close(_z_sys_net_socket_t *sock) { _ZP_UNUSED(sock); }

static size_t _z_threadx_stm32_uart_fill_frame(void) {
    size_t rb = 0;

    if (tx_semaphore_get(&_z_threadx_stm32_data_ready_semaphore, TX_TIMER_TICKS_PER_SECOND) != TX_SUCCESS) {
        return SIZE_MAX;
    }

    if (_z_threadx_stm32_delimiter_offset < _z_threadx_stm32_last_dma_offset) {
        rb = (RX_DMA_BUFFER_SIZE - _z_threadx_stm32_last_dma_offset) + _z_threadx_stm32_delimiter_offset;
    } else {
        rb = _z_threadx_stm32_delimiter_offset - _z_threadx_stm32_last_dma_offset;
    }
    if (rb == 0 || rb > sizeof(_z_threadx_stm32_frame_buffer)) {
        return SIZE_MAX;
    }

    if (_z_threadx_stm32_delimiter_offset < _z_threadx_stm32_last_dma_offset) {
        size_t second_part = RX_DMA_BUFFER_SIZE - _z_threadx_stm32_last_dma_offset;
        // flawfinder: ignore
        memcpy(_z_threadx_stm32_frame_buffer, _z_threadx_stm32_dma_buffer + _z_threadx_stm32_last_dma_offset,
               second_part);
        // flawfinder: ignore
        memcpy(_z_threadx_stm32_frame_buffer + second_part, _z_threadx_stm32_dma_buffer,
               _z_threadx_stm32_delimiter_offset);
    } else {
        // flawfinder: ignore
        memcpy(_z_threadx_stm32_frame_buffer, _z_threadx_stm32_dma_buffer + _z_threadx_stm32_last_dma_offset, rb);
    }
    _z_threadx_stm32_last_dma_offset = _z_threadx_stm32_delimiter_offset;

    tx_semaphore_put(&_z_threadx_stm32_data_processing_semaphore);

    _z_threadx_stm32_frame_len = rb;
    _z_threadx_stm32_frame_offset = 0;
    return rb;
}

static size_t _z_threadx_stm32_uart_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    _ZP_UNUSED(sock);

    if (_z_threadx_stm32_frame_offset == _z_threadx_stm32_frame_len) {
        if (_z_threadx_stm32_uart_fill_frame() == SIZE_MAX) {
            return SIZE_MAX;
        }
    }

    size_t available = _z_threadx_stm32_frame_len - _z_threadx_stm32_frame_offset;
    size_t chunk = len < available ? len : available;
    // flawfinder: ignore
    memcpy(ptr, &_z_threadx_stm32_frame_buffer[_z_threadx_stm32_frame_offset], chunk);
    _z_threadx_stm32_frame_offset += chunk;

    if (_z_threadx_stm32_frame_offset == _z_threadx_stm32_frame_len) {
        _z_threadx_stm32_frame_len = 0;
        _z_threadx_stm32_frame_offset = 0;
    }

    return chunk;
}

static size_t _z_threadx_stm32_uart_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    _ZP_UNUSED(sock);

    if (HAL_UART_Transmit(&ZENOH_HUART, (uint8_t *)ptr, len, 2000) != HAL_OK) {
        _Z_ERROR("Could not send to serial device!");
        return SIZE_MAX;
    }

    return len;
}

z_result_t _z_serial_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    return _z_threadx_stm32_uart_open_from_pins(sock, txpin, rxpin, baudrate);
}

z_result_t _z_serial_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_threadx_stm32_uart_open_from_dev(sock, dev, baudrate);
}

z_result_t _z_serial_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    return _z_threadx_stm32_uart_listen_from_pins(sock, txpin, rxpin, baudrate);
}

z_result_t _z_serial_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_threadx_stm32_uart_listen_from_dev(sock, dev, baudrate);
}

void _z_serial_close(_z_sys_net_socket_t *sock) { _z_threadx_stm32_uart_close(sock); }

size_t _z_serial_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    return _z_threadx_stm32_uart_read(sock, ptr, len);
}

size_t _z_serial_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    return _z_threadx_stm32_uart_write(sock, ptr, len);
}

void zptxstm32_rx_event_cb(UART_HandleTypeDef *huart, uint16_t offset) {
    static uint16_t last_offset = 0;
    if (huart != &ZENOH_HUART) {
        return;
    }
    if (offset == last_offset) {
        return;
    }
    if (offset < last_offset) {
        last_offset = 0;
    }

    while (last_offset < offset) {
        if (_z_threadx_stm32_dma_buffer[last_offset] == (uint8_t)0x00) {
            tx_semaphore_get(&_z_threadx_stm32_data_processing_semaphore, TX_WAIT_FOREVER);
            _z_threadx_stm32_delimiter_offset = last_offset + 1;
            tx_semaphore_put(&_z_threadx_stm32_data_ready_semaphore);
        }
        ++last_offset;
    }
}

void zptxstm32_error_event_cb(UART_HandleTypeDef *huart) {
    if (huart != &ZENOH_HUART) {
        return;
    }

    _Z_ERROR("UART error!");
    HAL_UARTEx_ReceiveToIdle_DMA(&ZENOH_HUART, _z_threadx_stm32_dma_buffer, RX_DMA_BUFFER_SIZE);
}

#if ZENOH_THREADX_STM32_GEN_IRQ == 1
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t offset) { zptxstm32_rx_event_cb(huart, offset); }

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    HAL_UART_DMAStop(huart);
    HAL_UART_Abort(huart);
    __HAL_UART_CLEAR_IDLEFLAG(huart);
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    zptxstm32_error_event_cb(huart);
}
#endif

#endif /* Z_FEATURE_LINK_SERIAL == 1 && defined(ZENOH_THREADX_STM32) */
