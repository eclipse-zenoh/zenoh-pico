/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http: *www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https: *www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#ifndef ZENOH_NET_PICO_CONFIG_H
#define ZENOH_NET_PICO_CONFIG_H

/*------------------ Configuration properties ------------------*/
/** 
 * The library mode.
 * String key : `"mode"`.
 * Accepted values : `"client"`.
 * Default value : `"client"`.
 */
#define ZN_CONFIG_MODE_KEY 0x40
#define ZN_CONFIG_MODE_DEFAULT "client"

/** 
 * The locator of a peer to connect to.
 * String key : `"peer"`.
 * Accepted values : `<locator>` (ex: `"tcp/10.10.10.10:7447"`).
 * Default value : None.
 * Multiple values are not accepted in zenoh-pico.
 */
#define ZN_CONFIG_PEER_KEY 0x41

/** 
 * The user name to use for authentication.
 * String key : `"user"`.
 * Accepted values : `<string>`.
 * Default value : None.
 */
#define ZN_CONFIG_USER_KEY 0x43

/** 
 * The password to use for authentication.
 * String key : `"password"`.
 * Accepted values : `<string>`.
 * Default value : None.
 */
#define ZN_CONFIG_PASSWORD_KEY 0x44

/** 
 * Activates/Desactivates multicast scouting.
 * String key : `"multicast_scouting"`.
 * Accepted values : `0`, `1`.
 * Default value : `1`.
 */
#define ZN_CONFIG_MULTICAST_SCOUTING_KEY 0x45
#define ZN_CONFIG_MULTICAST_SCOUTING_DEFAULT "true"

/** 
 * The network interface to use for multicast scouting.
 * String key : `"multicast_interface"`.
 * Accepted values : `"auto"`, `<ip address>`, `<interface name>`.
 * Default value : `"auto"`.
 */
#define ZN_CONFIG_MULTICAST_INTERFACE_KEY 0x46
#define ZN_CONFIG_MULTICAST_INTERFACE_DEFAULT "auto"

/** 
 * The multicast address and ports to use for multicast scouting.
 * String key : `"multicast_address"`.
 * Accepted values : `<ip address>:<port>`.
 * Default value : `"224.0.0.224:7447"`.
 */
#define ZN_CONFIG_MULTICAST_ADDRESS_KEY 0x47
#define ZN_CONFIG_MULTICAST_ADDRESS_DEFAULT "224.0.0.224:7447"

/** 
 * In client mode, the period dedicated to scouting a router before failing.
 * String key : `"scouting_timeout"`.
 * Accepted values : `<float in seconds>`.
 * Default value : `"3.0"`.
 */
#define ZN_CONFIG_SCOUTING_TIMEOUT_KEY 0x48
#define ZN_CONFIG_SCOUTING_TIMEOUT_DEFAULT "3.0"

/** 
 * Indicates if data messages should be timestamped.
 * String key : `"add_timestamp"`.
 * Accepted values : `0`, `1`.
 * Default value : `0`.
 */
#define ZN_CONFIG_ADD_TIMESTAMP_KEY 0x4A
#define ZN_CONFIG_ADD_TIMESTAMP_DEFAULT "false"

/*------------------ Configuration properties ------------------*/
#define ZN_ATTACHMENT_BUF_LEN 16384
#define ZN_PID_LENGTH 8
#define ZN_TSID_LENGTH 16
#define ZN_PROTO_VERSION 0x00
/** 
 * Default session lease in milliseconds: 10 seconds
 */
#define ZN_SESSION_LEASE 10000
#define ZN_KEEP_ALIVE_INTERVAL 1000

/** 
 * The default sequence number resolution takes 4 bytes on the wire.
 * Given the VLE encoding of ZInt, 4 bytes result in 28 useful bits.
 * 2^28 = 268_435_456 => Max Seq Num = 268_435_455
 */
#define ZN_SN_RESOLUTION_DEFAULT 268435455
#define ZN_SN_RESOLUTION ZN_SN_RESOLUTION_DEFAULT

#define ZN_CONGESTION_CONTROL_DEFAULT zn_congestion_control_t_DROP

#define ZN_TRANSPORT_TCP_IP 1
//#define ZN_TRANSPORT_BLE 1

#define ZN_FRAG_BUF_TX_CHUNK 128
#define ZN_FRAG_BUF_RX_LIMIT 10000000

#define ZN_BATCH_SIZE 16384
#ifdef ZN_TRANSPORT_TCP_IP
/** 
 * NOTE: 16 bits (2 bytes) may be prepended to the serialized message indicating the total length
 *       in bytes of the message, resulting in the maximum length of a message being 65_535 bytes.
 *       This is necessary in those stream-oriented transports (e.g., TCP) that do not preserve
 *       the boundary of the serialized messages. The length is encoded as little-endian.
 *       In any case, the length of a message must not exceed 65_535 bytes.
 */
#define ZN_READ_BUF_LEN 65535 + _ZN_MSG_LEN_ENC_SIZE
#define ZN_WRITE_BUF_LEN ZN_BATCH_SIZE + _ZN_MSG_LEN_ENC_SIZE
#else
#define ZN_READ_BUF_LEN 65535
#define ZN_WRITE_BUF_LEN ZN_BATCH_SIZE
#endif

#endif /* ZENOH_NET_PICO_CONFIG_H */
