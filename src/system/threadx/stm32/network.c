/* Ubiquity robotics
 * ======================================================================
 * Zenoh-pico stm32 threadx
 * Network implementation for serial device running in circular DMA mode.
 * ======================================================================
 */
#if defined(ZENOH_THREADX_STM32)
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "hal.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/codec/serial.h"
#include "zenoh-pico/system/common/serial.h"
#include "zenoh-pico/system/link/serial.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_LINK_TCP == 1
#error "Z_FEATURE_LINK_TCP is not supported"
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Z_FEATURE_LINK_BLUETOOTH is not supported"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Z_FEATURE_RAWETH_TRANSPORT is not supported"
#endif

#if Z_FEATURE_LINK_SERIAL == 1

#define RX_DMA_BUFFER_SIZE \
    (_Z_SERIAL_MAX_COBS_BUF_SIZE * 2 + 2)  // for 2 max sized COBS frames + something little extra
static uint8_t dma_buffer[RX_DMA_BUFFER_SIZE];
static uint16_t delimiter_offset = 0;
static TX_SEMAPHORE data_ready_semaphore, data_processing_semaphore;
extern UART_HandleTypeDef ZENOH_HUART;
/*------------------ Serial sockets ------------------*/
void _z_socket_close(_z_sys_net_socket_t *sock) { _ZP_UNUSED(sock); }

z_result_t _z_open_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    z_result_t ret = _Z_RES_OK;
    (void)(sock);
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    // Not implemented
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    ret = _Z_ERR_GENERIC;

    return ret;
}

z_result_t _z_open_serial_from_dev(_z_sys_net_socket_t *sock, char *dev, uint32_t baudrate) {
    (void)(dev);
    (void)(baudrate);

    tx_semaphore_create(&data_ready_semaphore, "Data Ready", 0);
    tx_semaphore_create(&data_processing_semaphore, "Data Processing",
                        1);  // get is 1 step signal ahead, 0-still processing, 1-ready to process new data
    HAL_UARTEx_ReceiveToIdle_DMA(&ZENOH_HUART, (uint8_t *)dma_buffer, RX_DMA_BUFFER_SIZE);
    return _z_connect_serial(*sock);
}

z_result_t _z_listen_serial_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin, uint32_t baudrate) {
    z_result_t ret = _Z_RES_OK;
    (void)(sock);
    (void)(txpin);
    (void)(rxpin);
    (void)(baudrate);

    // Not implemented
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    ret = _Z_ERR_GENERIC;

    return ret;
}

z_result_t _z_listen_serial_from_dev(_z_sys_net_socket_t *sock, char *dev, uint32_t baudrate) {
    z_result_t ret = _Z_RES_OK;
    (void)(sock);
    (void)(dev);
    (void)(baudrate);

    // Not implemented
    _Z_ERROR_LOG(_Z_ERR_GENERIC);
    ret = _Z_ERR_GENERIC;

    return ret;
}

void _z_close_serial(_z_sys_net_socket_t *sock) {
    // Nothing to do
}

size_t _z_read_serial_internal(const _z_sys_net_socket_t sock, uint8_t *header, uint8_t *ptr, size_t len) {
    size_t rb = 0;

    static uint16_t last_offset = 0;

    // Block on data ready for one second only - TODO: check if time is ok
    /* We used to block forever but like this zenoh-init never resends initial
     * handshake to router because this function never returns.
     * This is a problem if router starts later than this node.
     */
    if (tx_semaphore_get(&data_ready_semaphore, TX_TIMER_TICKS_PER_SECOND) != TX_SUCCESS) {
        return 0;
    }

    // COMUNICATION
    if (delimiter_offset < last_offset) {
        // wrap around
        rb = (RX_DMA_BUFFER_SIZE - last_offset) + delimiter_offset;
    } else {
        rb = delimiter_offset - last_offset;
    }
    if (rb >= _Z_SERIAL_MAX_COBS_BUF_SIZE) {
        _Z_ERROR("Received COBS message exceeds _Z_SERIAL_MAX_COBS_BUF_SIZE!");
        return SIZE_MAX;
    }

    uint8_t *raw_buf = z_malloc(rb);
    if (raw_buf == NULL) {
        return 0;
    }
    if (delimiter_offset < last_offset) {
        // copy in two parts if dma wrapped around
        size_t s2 = RX_DMA_BUFFER_SIZE - last_offset;
        memcpy(raw_buf, dma_buffer + last_offset, s2);
        memcpy(raw_buf + s2, dma_buffer, delimiter_offset);

    } else {
        memcpy(raw_buf, dma_buffer + last_offset, rb);
    }
    last_offset = delimiter_offset;

    // resume recieving data
    tx_semaphore_put(&data_processing_semaphore);

    uint8_t *tmp_buf = z_malloc(_Z_SERIAL_MFS_SIZE);
    if (tmp_buf == NULL) {
        z_free(raw_buf);
        return 0;
    }
    size_t ret = _z_serial_msg_deserialize(raw_buf, rb, ptr, len, header, tmp_buf, _Z_SERIAL_MFS_SIZE);

    z_free(raw_buf);
    z_free(tmp_buf);

    return ret;
}

size_t _z_send_serial_internal(const _z_sys_net_socket_t sock, uint8_t header, const uint8_t *ptr, size_t len) {
    uint8_t *tmp_buf = (uint8_t *)z_malloc(_Z_SERIAL_MFS_SIZE);
    if (tmp_buf == NULL) {
        return SIZE_MAX;
    }
    uint8_t *raw_buf = (uint8_t *)z_malloc(_Z_SERIAL_MAX_COBS_BUF_SIZE);
    if (raw_buf == NULL) {
        z_free(tmp_buf);
        return SIZE_MAX;
    }
    size_t ret =
        _z_serial_msg_serialize(raw_buf, _Z_SERIAL_MAX_COBS_BUF_SIZE, ptr, len, header, tmp_buf, _Z_SERIAL_MFS_SIZE);

    if (ret == SIZE_MAX) {
        z_free(raw_buf);
        z_free(tmp_buf);
        return ret;
    }

    if (HAL_UART_Transmit(&ZENOH_HUART, raw_buf, ret, 2000) != HAL_OK) {
        _Z_ERROR("Could not send to serial device!");
        ret = SIZE_MAX;
    }

    z_free(raw_buf);
    z_free(tmp_buf);

    return len;
}

void zptxstm32_rx_event_cb(UART_HandleTypeDef *huart, uint16_t offset) {
    static uint16_t last_offset = 0;
    if (huart != &ZENOH_HUART) {
        // not correct uart
        return;
    }
    if (offset == last_offset)
        // ignore if called twice (this happens on every half buffer)
        return;

    if (offset < last_offset)
        // if wrap around reset last_size
        last_offset = 0;

    while (last_offset < offset) {
        if (dma_buffer[last_offset] == (uint8_t)0x00) {
            tx_semaphore_get(&data_processing_semaphore, TX_WAIT_FOREVER);  // Block if data isnt processed yet
            delimiter_offset = last_offset + 1;
            tx_semaphore_put(&data_ready_semaphore);  // Notify waiting task
        }
        ++last_offset;
    }
}

void zptxstm32_error_event_cb(UART_HandleTypeDef *huart) {
    if (huart != &ZENOH_HUART) {
        // not correct uart
        return;
    }
    _Z_ERROR("UART error!");
    // restart UART DMA after error
    HAL_UARTEx_ReceiveToIdle_DMA(&ZENOH_HUART, (uint8_t *)dma_buffer, RX_DMA_BUFFER_SIZE);
}

#if ZENOH_THREADX_STM32_GEN_IRQ == 1
// This callback triggers when there is certain amount of "idling" on the UART line
// ("idling" == 1.5 stop bits; it also triggers when the buffer is half full)
// Tremendously decreases number of interrupts needed for communication
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

#endif  // Z_FEATURE_LINK_SERIAL
#endif  // ZENOH_THREADX_STM32
