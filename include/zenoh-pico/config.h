/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
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

#ifndef ZENOH_PICO_CONFIG_H
#define ZENOH_PICO_CONFIG_H

/*------------------ Configuration properties ------------------*/
/**
 * The library mode.
 * String key : `"mode"`.
 * Accepted values : `"client"`, `"peer"`.
 * Default value : `"client"`.
 */
#define ZN_CONFIG_MODE_KEY 0x40
#define ZN_CONFIG_MODE_CLIENT "client"
#define ZN_CONFIG_MODE_PEER "peer"
#define ZN_CONFIG_MODE_DEFAULT ZN_CONFIG_MODE_CLIENT

/**
 * The locator of a peer to connect to.
 * String key : `"peer"`.
 * Accepted values : `<locator>` (ex: `"tcp/10.10.10.10:7447"`).
 * Default value : None.
 * Multiple values are not accepted in zenoh-pico.
 */
#define ZN_CONFIG_PEER_KEY 0x41

/**
 * A locator to listen on.
 * String key : `"listener"`.
 * Accepted values : `<locator>` (ex: `"tcp/10.10.10.10:7447"`).
 * Default value : None.
 * Multiple values accepted.
 */
#define ZN_CONFIG_LISTENER_KEY 0x42

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
#define ZN_CONFIG_MULTICAST_ADDRESS_DEFAULT "udp/224.0.0.224:7447"

/**
 * In client mode, the period dedicated to scouting a router before failing.
 * String key : `"scouting_timeout"`.
 * Accepted values : `<int in seconds>`.
 * Default value : `"3"`.
 */
#define ZN_CONFIG_SCOUTING_TIMEOUT_KEY 0x48
#define ZN_CONFIG_SCOUTING_TIMEOUT_DEFAULT "3"

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
#define ZN_TRANSPORT_LEASE 10000
#define ZN_TRANSPORT_LEASE_EXPIRE_FACTOR 3.5

/**
 * Default multicast session join interval in milliseconds: 2.5 seconds
 */
#define ZN_JOIN_INTERVAL 2500

/**
 * Default socket timeout: 2 seconds
 */
#define ZN_CONFIG_SOCKET_TIMEOUT_DEFAULT 2

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

#define ZN_BATCH_SIZE 65535

#endif /* ZENOH_PICO_CONFIG_H */
