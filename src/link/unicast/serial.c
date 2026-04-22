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
//

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/link/transport/serial_protocol.h"

#if Z_FEATURE_LINK_SERIAL == 1

z_result_t _z_endpoint_serial_valid(_z_endpoint_t *endpoint) { return _z_serial_endpoint_valid(endpoint); }

z_result_t _z_f_link_open_serial(_z_link_t *self) {
    return _z_serial_protocol_open(&self->_socket._serial, &self->_endpoint);
}

z_result_t _z_f_link_listen_serial(_z_link_t *self) {
    return _z_serial_protocol_listen(&self->_socket._serial, &self->_endpoint);
}

void _z_f_link_close_serial(_z_link_t *self) { _z_serial_protocol_close(&self->_socket._serial); }

void _z_f_link_free_serial(_z_link_t *self) { (void)(self); }

size_t _z_f_link_write_serial(const _z_link_t *self, const uint8_t *ptr, size_t len, _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(socket);
    return _z_send_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_write_all_serial(const _z_link_t *self, const uint8_t *ptr, size_t len) {
    return _z_send_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_read_serial(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr) {
    _ZP_UNUSED(addr);
    return _z_read_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_read_exact_serial(const _z_link_t *self, uint8_t *ptr, size_t len, _z_slice_t *addr,
                                   _z_sys_net_socket_t *socket) {
    _ZP_UNUSED(addr);
    _ZP_UNUSED(socket);
    return _z_read_exact_serial(self->_socket._serial._sock, ptr, len);
}

size_t _z_f_link_read_socket_serial(const _z_sys_net_socket_t socket, uint8_t *ptr, size_t len) {
    return _z_read_serial(socket, ptr, len);
}

uint16_t _z_get_link_mtu_serial(void) { return _Z_SERIAL_MTU_SIZE; }

z_result_t _z_new_link_serial(_z_link_t *zl, _z_endpoint_t endpoint) {
    z_result_t ret = _Z_RES_OK;
    zl->_type = _Z_LINK_TYPE_SERIAL;
    zl->_cap._transport = Z_LINK_CAP_TRANSPORT_UNICAST;
    zl->_cap._flow = Z_LINK_CAP_FLOW_DATAGRAM;
    zl->_cap._is_reliable = false;

    zl->_mtu = _z_get_link_mtu_serial();

    zl->_endpoint = endpoint;

    zl->_open_f = _z_f_link_open_serial;
    zl->_listen_f = _z_f_link_listen_serial;
    zl->_close_f = _z_f_link_close_serial;
    zl->_free_f = _z_f_link_free_serial;

    zl->_write_f = _z_f_link_write_serial;
    zl->_write_all_f = _z_f_link_write_all_serial;
    zl->_read_f = _z_f_link_read_serial;
    zl->_read_exact_f = _z_f_link_read_exact_serial;
    zl->_read_socket_f = _z_f_link_read_socket_serial;

    return ret;
}
#endif
