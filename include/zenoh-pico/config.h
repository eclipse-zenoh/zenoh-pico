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

#ifndef ZENOH_PICO_CONFIG_H
#define ZENOH_PICO_CONFIG_H

/*------------------ Configuration properties ------------------*/
/**
 * The library mode.
 * String key : `"mode"`.
 * Accepted values : `"client"`, `"peer"`.
 * Default value : `"client"`.
 */
#define Z_CONFIG_MODE_KEY 0x40
#define Z_CONFIG_MODE_CLIENT "client"
#define Z_CONFIG_MODE_PEER "peer"
#define Z_CONFIG_MODE_DEFAULT Z_CONFIG_MODE_CLIENT

/**
 * The locator of a peer to connect to.
 * String key : `"peer"`.
 * Accepted values : `<locator>` (ex: `"tcp/10.10.10.10:7447"`).
 * Default value : None.
 * Multiple values are not accepted in zenoh-pico.
 */
#define Z_CONFIG_PEER_KEY 0x41

/**
 * A locator to listen on.
 * String key : `"listener"`.
 * Accepted values : `<locator>` (ex: `"tcp/10.10.10.10:7447"`).
 * Default value : None.
 * Multiple values accepted.
 */
#define Z_CONFIG_LISTENER_KEY 0x42

/**
 * The user name to use for authentication.
 * String key : `"user"`.
 * Accepted values : `<string>`.
 * Default value : None.
 */
#define Z_CONFIG_USER_KEY 0x43

/**
 * The password to use for authentication.
 * String key : `"password"`.
 * Accepted values : `<string>`.
 * Default value : None.
 */
#define Z_CONFIG_PASSWORD_KEY 0x44

/**
 * Activates/Desactivates multicast scouting.
 * String key : `"multicast_scouting"`.
 * Accepted values : `0`, `1`.
 * Default value : `1`.
 */
#define Z_CONFIG_MULTICAST_SCOUTING_KEY 0x45
#define Z_CONFIG_MULTICAST_SCOUTING_DEFAULT "true"

/**
 * The network interface to use for multicast scouting.
 * String key : `"multicast_interface"`.
 * Accepted values : `"auto"`, `<ip address>`, `<interface name>`.
 * Default value : `"auto"`.
 */
#define Z_CONFIG_MULTICAST_INTERFACE_KEY 0x46
#define Z_CONFIG_MULTICAST_INTERFACE_DEFAULT "auto"

/**
 * The multicast address and ports to use for multicast scouting.
 * String key : `"multicast_address"`.
 * Accepted values : `<ip address>:<port>`.
 * Default value : `"224.0.0.224:7447"`.
 */
#define Z_CONFIG_MULTICAST_ADDRESS_KEY 0x47
#define Z_CONFIG_MULTICAST_ADDRESS_DEFAULT "udp/224.0.0.224:7447"

/**
 * In client mode, the period dedicated to scouting a router before failing.
 * String key : `"scouting_timeout"`.
 * Accepted values : `<int in milliseconds>`.
 * Default value : `"3000"`.
 */
#define Z_CONFIG_SCOUTING_TIMEOUT_KEY 0x48
#define Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT "3000"

/**
 * Indicates if data messages should be timestamped.
 * String key : `"add_timestamp"`.
 * Accepted values : `0`, `1`.
 * Default value : `0`.
 */
#define Z_CONFIG_ADD_TIMESTAMP_KEY 0x4A
#define Z_CONFIG_ADD_TIMESTAMP_DEFAULT "false"

/*------------------ Configuration properties ------------------*/
#define Z_PID_LENGTH 8
#define Z_TSID_LENGTH 16
#define Z_PROTO_VERSION 0x06

/**
 * Default session lease in milliseconds.
 */
#define Z_TRANSPORT_LEASE 10000

/**
 * Default session lease expire factor.
 */
#define Z_TRANSPORT_LEASE_EXPIRE_FACTOR 3.5

/**
 * Default multicast session join interval in milliseconds.
 */
#define Z_JOIN_INTERVAL 2500

/**
 * Default socket timeout in milliseconds.
 */
#define Z_CONFIG_SOCKET_TIMEOUT 2000

/**
 * The default sequence number resolution takes 4 bytes on the wire.
 * Given the VLE encoding of ZInt, 4 bytes result in 28 useful bits.
 * 2^28 = 268_435_456 => Max Seq Num = 268_435_455
 */
#define Z_SN_RESOLUTION_DEFAULT 268435455
#define Z_SN_RESOLUTION Z_SN_RESOLUTION_DEFAULT

#define Z_CONGESTION_CONTROL_DEFAULT Z_CONGESTION_CONTROL_DROP

#define Z_LINK_TCP 1
#define Z_LINK_UDP_MULTICAST 1
#define Z_LINK_UDP_UNICAST 1
#define Z_LINK_BLUETOOTH 0

#define Z_SCOUTING_UDP 1

#define Z_IOSLICE_SIZE 128
#define Z_BATCH_SIZE_RX 65535 // Warning: changing this value can break the communication
                              //          with zenohd in the current protocol version.
                              //          In the future, it will be possible to negotiate such value.
                              // Change it at your own risk.
#define Z_BATCH_SIZE_TX 65535
#define Z_FRAG_MAX_SIZE 300000
#define Z_DYNAMIC_MEMORY_ALLOCATION 0

#endif /* ZENOH_PICO_CONFIG_H */
