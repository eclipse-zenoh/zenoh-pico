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

#if Z_FEATURE_LINK_SERIAL == 1 && defined(ZENOH_ZEPHYR)

#include <version.h>

#if KERNEL_VERSION_MAJOR == 2
#include <drivers/uart.h>
#include <kernel.h>
#else
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
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

    // @TODO: To be implemented
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

/* Bound the wait, yield between polls, and report a receive error.
 *
 * The previous loop spun on `uart_poll_in` with no timeout, no yield and no
 * error check, and returned `len` whatever happened. Three consequences, all
 * observed on an MR-CANHUBK344 (S32K344) carrying zenoh over a 115200 line:
 *
 *   * No timeout. A peer that stops mid-frame parks this thread forever. The
 *     other ports do not do this: `tty_posix` returns `SIZE_MAX` when the read
 *     fails, and callers such as `_z_read_exact_serial` already handle it.
 *
 *   * No yield. On a cooperatively scheduled or equal-priority system this
 *     starves every other thread, including the one that would have refilled
 *     the line. `k_yield()` and not `z_sleep_ms(1)`: the shortest sleep is a
 *     whole tick, and at 115200 a byte arrives every 87 us, so sleeping loses
 *     bytes that yielding does not.
 *
 *   * No `uart_err_check`. `uart_poll_in` reports "no character available"; it
 *     has no way to say "a character was destroyed before you asked". A UART
 *     overrun is therefore SILENT, and a receiver that only reads the return
 *     value cannot tell a quiet link from one it is failing to keep up with.
 *     That indistinguishability cost a long investigation that ended in a bug
 *     report against the peer, which turned out to be innocent: the overrun
 *     flag was set on exactly the frame that was truncated and nowhere else.
 *
 * The return value now follows the contract the posix port already uses:
 * `SIZE_MAX` on failure, otherwise the number of bytes actually read.
 */
#ifndef Z_ZEPHYR_SERIAL_READ_TIMEOUT_MS
#define Z_ZEPHYR_SERIAL_READ_TIMEOUT_MS 1000
#endif

static size_t _z_zephyr_uart_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    for (size_t i = 0; i < len; i++) {
        int res = -1;
        int64_t deadline = k_uptime_get() + (int64_t)Z_ZEPHYR_SERIAL_READ_TIMEOUT_MS;
        while (res != 0) {
            res = uart_poll_in(sock._serial, &ptr[i]);
            if (res != 0) {
                if (k_uptime_get() > deadline) {
                    return SIZE_MAX;
                }
                k_yield();
            }
        }

        /* Checked per byte rather than per read: the flag is sticky until it is
         * read, so taking it here attributes the loss to the byte that was
         * actually damaged. */
        int err = uart_err_check(sock._serial);
        if (err > 0 && (err & UART_ERROR_OVERRUN) != 0) {
            _Z_ERROR("serial RX overrun after %zu byte(s) -- frame will be dropped", i);
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
