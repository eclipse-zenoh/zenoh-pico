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

#ifndef ZENOH_C_NET_CONFIG_H
#define ZENOH_C_NET_CONFIG_H

#define ZENOH_NET_ATTACHMENT_BUF_LEN 16384
#define ZENOH_NET_PID_LENGTH 8
#define ZENOH_NET_PROTO_VERSION 0x00
// #define ZENOH_NET_DEFAULT_LEASE 10000 // @TODO: Restore
#define ZENOH_NET_DEFAULT_LEASE 3600000

// The default sequence number resolution takes 4 bytes on the wire.
// Given the VLE encoding of ZInt, 4 bytes result in 28 useful bits.
// 2^28 = 268_435_456 => Max Seq Num = 268_435_455
#define ZENOH_NET_SN_RESOLUTION_DEFAULT 268435455
#define ZENOH_NET_SN_RESOLUTION ZENOH_NET_SN_RESOLUTION_DEFAULT

#define ZENOH_NET_SCOUT_MCAST_ADDR "224.0.0.224"
#define ZENOH_NET_LOCAL_HOST "127.0.0.1"
#define ZENOH_NET_MAX_SCOUT_MSG_LEN 1024
#define ZENOH_NET_SCOUT_PORT 7447
#define ZENOH_NET_SCOUT_TRIES 3
#define ZENOH_NET_SCOUT_TIMEOUT 100000

#define ZENOH_NET_CONGESTION_CONTROL_DEFAULT 1

#define ZENOH_NET_TRANSPORT_TCP_IP 1
// #define ZENOH_NET_TRANSPORT_BLE 1

#ifdef ZENOH_NET_TRANSPORT_TCP_IP
#define ZENOH_NET_READ_BUF_LEN 65536 + _ZN_MSG_LEN_ENC_SIZE
#define ZENOH_NET_WRITE_BUF_LEN 65536 + _ZN_MSG_LEN_ENC_SIZE
#else
#define ZENOH_NET_READ_BUF_LEN 65536
#define ZENOH_NET_WRITE_BUF_LEN 65536
#endif

#endif /* ZENOH_C_NET_CONFIG_H */
