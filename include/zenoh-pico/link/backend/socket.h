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

#ifndef ZENOH_PICO_LINK_BACKEND_SOCKET_H
#define ZENOH_PICO_LINK_BACKEND_SOCKET_H

#include "zenoh-pico/system/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

z_result_t _z_socket_set_non_blocking(const _z_sys_net_socket_t *sock);
z_result_t _z_socket_accept(const _z_sys_net_socket_t *sock_in, _z_sys_net_socket_t *sock_out);
z_result_t _z_ip_port_to_endpoint(const uint8_t *address, size_t address_len, uint16_t port, char *dst, size_t dst_len);
z_result_t _z_socket_get_endpoints(const _z_sys_net_socket_t *sock, char *local, size_t local_len, char *remote,
                                   size_t remote_len);
void _z_socket_close(_z_sys_net_socket_t *sock);
z_result_t _z_socket_wait_event(void *peers, _z_mutex_rec_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_LINK_BACKEND_SOCKET_H */
