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

// @TODO: remove the platform-specific include
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "zenoh/net/types.h"
#include "zenoh/net/private/msg.h"
#include "zenoh/net/private/result.h"

char *_zn_select_scout_iface(void);
_zn_socket_result_t _zn_open_tx_session(const char *locator);

struct sockaddr_in *_zn_make_socket_address(const char *addr, int port);
_zn_socket_result_t _zn_create_udp_socket(const char *addr, int port, int recv_timeout);

int _zn_send_dgram_to(_zn_socket_t sock, const _z_wbuf_t *wbf, const struct sockaddr *dest, socklen_t salen);
int _zn_recv_dgram_from(_zn_socket_t sock, _z_rbuf_t *rbf, struct sockaddr *from, socklen_t *salen);

int _zn_send_wbuf(_zn_socket_t sock, const _z_wbuf_t *wbf);
int _zn_recv_rbuf(_zn_socket_t sock, _z_rbuf_t *rbf);
int _zn_recv_bytes(_zn_socket_t sock, uint8_t *buf, size_t len);

#endif /* ZENOH_C_NET_H */
