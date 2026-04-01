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

#if Z_FEATURE_LINK_SERIAL == 1 && (defined(ZENOH_LINUX) || defined(ZENOH_MACOS) || defined(ZENOH_BSD))

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <termios.h>
#include <unistd.h>

#include "zenoh-pico/utils/logging.h"

static speed_t _z_posix_tty_baudrate_to_speed(uint32_t baudrate) {
    switch (baudrate) {
        case 1200:
            return B1200;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
#ifdef B57600
        case 57600:
            return B57600;
#endif
#ifdef B115200
        case 115200:
            return B115200;
#endif
#ifdef B230400
        case 230400:
            return B230400;
#endif
        default:
            return (speed_t)0;
    }
}

static z_result_t _z_posix_tty_open_impl(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    if (sock == NULL || dev == NULL || baudrate == 0) {
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    speed_t speed = _z_posix_tty_baudrate_to_speed(baudrate);
    if (speed == (speed_t)0) {
        _Z_ERROR("Unsupported serial baudrate: %u", (unsigned)baudrate);
        _Z_ERROR_RETURN(_Z_ERR_INVALID);
    }

    // flawfinder: ignore
    int fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        _Z_ERROR("Failed to open serial device %s: errno=%d", dev, errno);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    cfmakeraw(&tty);
    tty.c_cflag |= (tcflag_t)(CLOCAL | CREAD);
#ifdef CRTSCTS
    tty.c_cflag &= (tcflag_t)~CRTSCTS;
#endif
    tty.c_cflag &= (tcflag_t)~CSTOPB;
    tty.c_cflag &= (tcflag_t)~PARENB;
    tty.c_cflag &= (tcflag_t)~CSIZE;
    tty.c_cflag |= (tcflag_t)CS8;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;

    if (cfsetispeed(&tty, speed) != 0 || cfsetospeed(&tty, speed) != 0) {
        close(fd);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }
    if (tcflush(fd, TCIOFLUSH) != 0) {
        close(fd);
        _Z_ERROR_RETURN(_Z_ERR_GENERIC);
    }

    sock->_fd = fd;
    return _Z_RES_OK;
}

static z_result_t _z_posix_tty_open_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                              uint32_t baudrate) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(txpin);
    _ZP_UNUSED(rxpin);
    _ZP_UNUSED(baudrate);
    _Z_ERROR_RETURN(_Z_ERR_INVALID);
}

static z_result_t _z_posix_tty_open_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_posix_tty_open_impl(sock, dev, baudrate);
}

static z_result_t _z_posix_tty_listen_from_pins(_z_sys_net_socket_t *sock, uint32_t txpin, uint32_t rxpin,
                                                uint32_t baudrate) {
    return _z_posix_tty_open_from_pins(sock, txpin, rxpin, baudrate);
}

static z_result_t _z_posix_tty_listen_from_dev(_z_sys_net_socket_t *sock, const char *dev, uint32_t baudrate) {
    return _z_posix_tty_open_impl(sock, dev, baudrate);
}

static void _z_posix_tty_close(_z_sys_net_socket_t *sock) {
    if (sock != NULL && sock->_fd >= 0) {
        close(sock->_fd);
        sock->_fd = -1;
    }
}

static size_t _z_posix_tty_read(_z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {
    size_t count = len > (size_t)SSIZE_MAX ? (size_t)SSIZE_MAX : len;
    // flawfinder: ignore
    ssize_t rb = read(sock._fd, ptr, count);
    if (rb <= 0) {
        return SIZE_MAX;
    }
    return (size_t)rb;
}

static size_t _z_posix_tty_write(_z_sys_net_socket_t sock, const uint8_t *ptr, size_t len) {
    size_t count = len > (size_t)SSIZE_MAX ? (size_t)SSIZE_MAX : len;
    ssize_t wb = write(sock._fd, ptr, count);
    if (wb <= 0) {
        return SIZE_MAX;
    }
    return (size_t)wb;
}

const _z_serial_ops_t _z_tty_posix_serial_ops = {
    .open_from_pins = _z_posix_tty_open_from_pins,
    .open_from_dev = _z_posix_tty_open_from_dev,
    .listen_from_pins = _z_posix_tty_listen_from_pins,
    .listen_from_dev = _z_posix_tty_listen_from_dev,
    .close = _z_posix_tty_close,
    .read = _z_posix_tty_read,
    .write = _z_posix_tty_write,
};

#endif /* Z_FEATURE_LINK_SERIAL == 1 && POSIX */
