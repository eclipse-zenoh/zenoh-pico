//
// Copyright (c) 2022 ZettaScale Technology
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

#include "zenoh-pico/config.h"

#if defined(ZENOH_MBED)

#include <NetworkInterface.h>
#include <USBSerial.h>
#include <mbed.h>

extern "C" {
#include <netdb.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

static uintptr_t _z_socket_id_impl(const _z_sys_net_socket_t *sock) {
#if Z_FEATURE_LINK_TCP == 1
    return (uintptr_t)sock->_tcp;
#elif Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
    return (uintptr_t)sock->_udp;
#else
    _ZP_UNUSED(sock);
    return 0;
#endif
}

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(blocking);
    _Z_ERROR("Function not yet supported on this system");
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock) {
    _ZP_UNUSED(sock);
    _Z_ERROR("Function not yet supported on this system");
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

void _z_socket_close(_z_sys_net_socket_t *sock) { _ZP_UNUSED(sock); }

static z_result_t _z_socket_wait_readable_impl(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                               uint32_t timeout_ms) {
    _ZP_UNUSED(sockets);
    _ZP_UNUSED(count);
    _ZP_UNUSED(ready);
    _ZP_UNUSED(timeout_ms);
    _Z_ERROR("Function not yet supported on this system");
    _Z_ERROR_RETURN(_Z_ERR_GENERIC);
}

uintptr_t _z_socket_id(const _z_sys_net_socket_t *sock) { return _z_socket_id_impl(sock); }

z_result_t _z_socket_wait_readable(const _z_sys_net_socket_t *sockets, size_t count, uint8_t *ready,
                                   uint32_t timeout_ms) {
    return _z_socket_wait_readable_impl(sockets, count, ready, timeout_ms);
}

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Bluetooth not supported yet on MBED port of Zenoh-Pico"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Raw ethernet transport not supported yet on MBED port of Zenoh-Pico"
#endif

}  // extern "C"

#endif /* defined(ZENOH_MBED) */
