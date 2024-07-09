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

#ifndef INCLUDE_ZENOH_PICO_CONFIG_H
#define INCLUDE_ZENOH_PICO_CONFIG_H

/*------------------ Runtime configuration properties ------------------*/
/**
 * The library mode.
 * Accepted values : `"client"`, `"peer"`.
 * Default value : `"client"`.
 */
#define Z_CONFIG_MODE_KEY 0x40
#define Z_CONFIG_MODE_CLIENT "client"
#define Z_CONFIG_MODE_PEER "peer"
#define Z_CONFIG_MODE_DEFAULT Z_CONFIG_MODE_CLIENT

/**
 * The locator of a peer to connect to.
 * Accepted values : `<locator>` (ex: `"tcp/10.10.10.10:7447"`).
 * Default value : None.
 * Multiple values are not accepted in zenoh-pico.
 */
#define Z_CONFIG_CONNECT_KEY 0x41

/**
 * A locator to listen on.
 * Accepted values : `<locator>` (ex: `"tcp/10.10.10.10:7447"`).
 * Default value : None.
 * Multiple values accepted.
 */
#define Z_CONFIG_LISTEN_KEY 0x42

/**
 * The user name to use for authentication.
 * Accepted values : `<string>`.
 * Default value : None.
 */
#define Z_CONFIG_USER_KEY 0x43

/**
 * The password to use for authentication.
 * Accepted values : `<string>`.
 * Default value : None.
 */
#define Z_CONFIG_PASSWORD_KEY 0x44

/**
 * Activates/Deactivates multicast scouting.
 * Accepted values : `false`, `true`.
 * Default value : `true`.
 */
#define Z_CONFIG_MULTICAST_SCOUTING_KEY 0x45
#define Z_CONFIG_MULTICAST_SCOUTING_DEFAULT "true"

/**
 * The multicast address and ports to use for multicast scouting.
 * Accepted values : `<ip address>:<port>`.
 * Default value : `"224.0.0.224:7446"`.
 */
#define Z_CONFIG_MULTICAST_LOCATOR_KEY 0x46
#define Z_CONFIG_MULTICAST_LOCATOR_DEFAULT "udp/224.0.0.224:7446"

/**
 * In client mode, the period dedicated to scouting a router before failing.
 * Accepted values : `<int in milliseconds>`.
 * Default value : `"1000"`.
 */
#define Z_CONFIG_SCOUTING_TIMEOUT_KEY 0x47
#define Z_CONFIG_SCOUTING_TIMEOUT_DEFAULT "1000"

/**
 * The entities to find in the multicast scouting, defined as a bitwise value.
 * Accepted values : [0-7]. Bitwise value are defined in :c:enum:`z_whatami_t`.
 * Default value : `3`.
 */
#define Z_CONFIG_SCOUTING_WHAT_KEY 0x48
#define Z_CONFIG_SCOUTING_WHAT_DEFAULT "3"

/**
 * A configurable and static Zenoh ID to be used on Zenoh Sessions.
 * Accepted values : `<UUDI 128-bit>`.
 */
#define Z_CONFIG_SESSION_ZID_KEY 0x49

/**
 * Indicates if data messages should be timestamped.
 * Accepted values : `false`, `true`.
 * Default value : `false`.
 */
#define Z_CONFIG_ADD_TIMESTAMP_KEY 0x4A
#define Z_CONFIG_ADD_TIMESTAMP_DEFAULT "false"

/*------------------ Compile-time feature configuration ------------------*/
// WARNING: Default values may always be overridden by CMake/make values

/**
 * Enable multi-thread support.
 */
#ifndef Z_FEATURE_MULTI_THREAD
#define Z_FEATURE_MULTI_THREAD 1
#endif

/**
 * Enable dynamic memory allocation.
 */
#ifndef Z_FEATURE_DYNAMIC_MEMORY_ALLOCATION
#define Z_FEATURE_DYNAMIC_MEMORY_ALLOCATION 0
#endif

/**
 * Enable queryables
 */
#ifndef Z_FEATURE_QUERYABLE
#define Z_FEATURE_QUERYABLE 1
#endif

/**
 * Enable queries
 */
#ifndef Z_FEATURE_QUERY
#define Z_FEATURE_QUERY 1
#endif

/**
 * Enable subscription on this node
 */
#ifndef Z_FEATURE_SUBSCRIPTION
#define Z_FEATURE_SUBSCRIPTION 1
#endif

/**
 * Enable publication
 */
#ifndef Z_FEATURE_PUBLICATION
#define Z_FEATURE_PUBLICATION 1
#endif

/**
 * Enable TCP links.
 */
#ifndef Z_FEATURE_LINK_TCP
#define Z_FEATURE_LINK_TCP 1
#endif

/**
 * Enable Bluetooth links.
 */
#ifndef Z_FEATURE_LINK_BLUETOOTH
#define Z_FEATURE_LINK_BLUETOOTH 0
#endif

/**
 * Enable WebSocket links.
 */
#ifndef Z_FEATURE_LINK_WS
#define Z_FEATURE_LINK_WS 0
#endif

/**
 * Enable Serial links.
 */
#ifndef Z_FEATURE_LINK_SERIAL
#define Z_FEATURE_LINK_SERIAL 0
#endif

/**
 * Enable UDP Scouting.
 */
#ifndef Z_FEATURE_SCOUTING_UDP
#define Z_FEATURE_SCOUTING_UDP 1
#endif

/**
 * Enable UDP Multicast links.
 */
#ifndef Z_FEATURE_LINK_UDP_MULTICAST
#define Z_FEATURE_LINK_UDP_MULTICAST 1
#endif

/**
 * Enable UDP Unicast links.
 */
#ifndef Z_FEATURE_LINK_UDP_UNICAST
#define Z_FEATURE_LINK_UDP_UNICAST 1
#endif

/**
 * Enable Multicast Transport.
 */
#ifndef Z_FEATURE_MULTICAST_TRANSPORT
#if Z_FEATURE_SCOUTING_UDP == 0 && Z_FEATURE_LINK_BLUETOOTH == 0 && Z_FEATURE_LINK_UDP_MULTICAST == 0
#define Z_FEATURE_MULTICAST_TRANSPORT 0
#else
#define Z_FEATURE_MULTICAST_TRANSPORT 1
#endif
#endif

/**
 * Enable Unicast Transport.
 */
#ifndef Z_FEATURE_UNICAST_TRANSPORT
#if Z_FEATURE_LINK_TCP == 0 && Z_FEATURE_LINK_UDP_UNICAST == 0 && Z_FEATURE_LINK_SERIAL == 0 && Z_FEATURE_LINK_WS == 0
#define Z_FEATURE_UNICAST_TRANSPORT 0
#else
#define Z_FEATURE_UNICAST_TRANSPORT 1
#endif
#endif

/**
 * Enable raweth transport/link.
 */
#ifndef Z_FEATURE_RAWETH_TRANSPORT
#define Z_FEATURE_RAWETH_TRANSPORT 0
#endif

/**
 * Enable message fragmentation.
 */
#ifndef Z_FEATURE_FRAGMENTATION
#define Z_FEATURE_FRAGMENTATION 1
#endif

/**
 * Enable interests.
 */
#ifndef Z_FEATURE_INTEREST
#define Z_FEATURE_INTEREST 0
#endif

/**
 * Enable encoding values.
 */
#ifndef Z_FEATURE_ENCODING_VALUES
#define Z_FEATURE_ENCODING_VALUES 1
#endif

/*------------------ Compile-time configuration properties ------------------*/
/**
 * Default length for Zenoh ID. Maximum size is 16 bytes.
 * This configuration will only be applied to Zenoh IDs generated by Zenoh-Pico.
 */
#ifndef Z_ZID_LENGTH
#define Z_ZID_LENGTH 16
#endif

#ifndef Z_TSID_LENGTH
#define Z_TSID_LENGTH 16
#endif

/**
 * Protocol version identifier.
 * Do not change this value.
 */
#ifndef Z_PROTO_VERSION
#define Z_PROTO_VERSION 0x09
#endif

/**
 * Default session lease in milliseconds.
 */
#ifndef Z_TRANSPORT_LEASE
#define Z_TRANSPORT_LEASE 10000
#endif

/**
 * Default session lease expire factor.
 */
#ifndef Z_TRANSPORT_LEASE_EXPIRE_FACTOR
#define Z_TRANSPORT_LEASE_EXPIRE_FACTOR 3
#endif

/**
 * Default multicast session join interval in milliseconds.
 */
#ifndef Z_JOIN_INTERVAL
#define Z_JOIN_INTERVAL 2500
#endif

/**
 * Default socket timeout in milliseconds.
 */
#ifndef Z_CONFIG_SOCKET_TIMEOUT
#define Z_CONFIG_SOCKET_TIMEOUT 100
#endif

#ifndef Z_SN_RESOLUTION
#define Z_SN_RESOLUTION 0x02
#endif

#ifndef Z_REQ_RESOLUTION
#define Z_REQ_RESOLUTION 0x02
#endif

/**
 * Default size for an IO slice.
 */
#ifndef Z_IOSLICE_SIZE
#define Z_IOSLICE_SIZE 128
#endif

/**
 * Default maximum batch size possible to be received or sent.
 */
#ifndef Z_BATCH_UNICAST_SIZE
#define Z_BATCH_UNICAST_SIZE 65535
#endif
/**
 * Default maximum batch size possible to be received or sent.
 */
#ifndef Z_BATCH_MULTICAST_SIZE
#define Z_BATCH_MULTICAST_SIZE 8192
#endif

/**
 * Default maximum size for fragmented messages.
 */
#ifndef Z_FRAG_MAX_SIZE
#define Z_FRAG_MAX_SIZE 300000
#endif

/**
 * Default "nop" instruction
 */
#ifndef ZP_ASM_NOP
#define ZP_ASM_NOP __asm__("nop")
#endif

/**
 * Default get timeout in milliseconds.
 */
#ifndef Z_GET_TIMEOUT_DEFAULT
#define Z_GET_TIMEOUT_DEFAULT 10000
#endif

#endif /* INCLUDE_ZENOH_PICO_CONFIG_H */
