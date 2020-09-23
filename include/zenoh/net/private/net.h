/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_C_NET_H
#define ZENOH_C_NET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "zenoh/net/types.h"
#include "zenoh/net/private/msg.h"

_ZN_RESULT_DECLARE (_zn_socket_t, socket)

char * _zn_select_scout_iface();

_zn_socket_result_t _zn_open_tx_session(const char *locator);

struct sockaddr_in *
_zn_make_socket_address(const char* addr, int port);

_zn_socket_result_t
_zn_create_udp_socket(const char *addr, int port, int recv_timeout);

int _zn_recv_dgram_from(_zn_socket_t sock, z_iobuf_t* buf, struct sockaddr* from, socklen_t *salen);

int _zn_send_dgram_to(_zn_socket_t sock, const z_iobuf_t* buf, const struct sockaddr *dest,  socklen_t salen);

int _zn_send_buf(_zn_socket_t sock, const z_iobuf_t* buf);

int _zn_recv_buf(_zn_socket_t sock, z_iobuf_t *buf);

int _zn_recv_n(_zn_socket_t sock, uint8_t* buf, size_t len);

size_t _zn_send_msg(_zn_socket_t sock, z_iobuf_t* buf, _zn_message_t* m);

size_t _zn_send_large_msg(_zn_socket_t sock, z_iobuf_t* buf, _zn_message_t* m, unsigned int max_len);

z_vle_result_t _zn_recv_vle(_zn_socket_t sock);

_zn_message_p_result_t _zn_recv_msg(_zn_socket_t sock, z_iobuf_t* buf);
void _zn_recv_msg_na(_zn_socket_t sock, z_iobuf_t* buf, _zn_message_p_result_t *r);

#endif /* ZENOH_C_NET_H */
