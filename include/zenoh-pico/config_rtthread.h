//
// Copyright (c) 2024 ZettaScale Technology
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

#ifndef ZENOH_PICO_CONFIG_RTTHREAD_H
#define ZENOH_PICO_CONFIG_RTTHREAD_H

// Platform identification
#define ZENOH_RTTHREAD 1

// Multi-threading support
#ifndef Z_FEATURE_MULTI_THREAD
#define Z_FEATURE_MULTI_THREAD 1
#endif

// Publication support
#ifndef Z_FEATURE_PUBLICATION
#define Z_FEATURE_PUBLICATION 1
#endif

// Subscription support  
#ifndef Z_FEATURE_SUBSCRIPTION
#define Z_FEATURE_SUBSCRIPTION 1
#endif

// Query support
#ifndef Z_FEATURE_QUERY
#define Z_FEATURE_QUERY 1
#endif

// Queryable support
#ifndef Z_FEATURE_QUERYABLE
#define Z_FEATURE_QUERYABLE 1
#endif

// Transport protocols
#ifndef Z_FEATURE_LINK_TCP
#define Z_FEATURE_LINK_TCP 1
#endif

#ifndef Z_FEATURE_LINK_UDP_UNICAST
#define Z_FEATURE_LINK_UDP_UNICAST 1
#endif

#ifndef Z_FEATURE_LINK_UDP_MULTICAST
#define Z_FEATURE_LINK_UDP_MULTICAST 1
#endif

#ifndef Z_FEATURE_LINK_SERIAL
#define Z_FEATURE_LINK_SERIAL 0
#endif

// Memory management
#ifndef Z_FEATURE_DYNAMIC_MEMORY_ALLOCATION
#define Z_FEATURE_DYNAMIC_MEMORY_ALLOCATION 1
#endif

// Fragmentation support
#ifndef Z_FEATURE_FRAGMENTATION
#define Z_FEATURE_FRAGMENTATION 1
#endif

// Batching support
#ifndef Z_FEATURE_BATCHING
#define Z_FEATURE_BATCHING 1
#endif

// Compression support
#ifndef Z_FEATURE_COMPRESSION
#define Z_FEATURE_COMPRESSION 0
#endif

// Attachment support
#ifndef Z_FEATURE_ATTACHMENT
#define Z_FEATURE_ATTACHMENT 1
#endif

// Interest support
#ifndef Z_FEATURE_INTEREST
#define Z_FEATURE_INTEREST 1
#endif

// RX cache support
#ifndef Z_FEATURE_RX_CACHE
#define Z_FEATURE_RX_CACHE 1
#endif

// TX cache support
#ifndef Z_FEATURE_TX_CACHE
#define Z_FEATURE_TX_CACHE 1
#endif

// Unstable API support
#ifndef Z_FEATURE_UNSTABLE_API
#define Z_FEATURE_UNSTABLE_API 0
#endif

// Debug level (0=none, 1=error, 2=info, 3=debug)
#ifndef ZENOH_DEBUG
#define ZENOH_DEBUG 0
#endif

// Buffer sizes
#ifndef Z_BATCH_UNICAST_SIZE
#define Z_BATCH_UNICAST_SIZE 65535
#endif

#ifndef Z_BATCH_MULTICAST_SIZE
#define Z_BATCH_MULTICAST_SIZE 8192
#endif

#ifndef Z_FRAGMENT_MAX_SIZE
#define Z_FRAGMENT_MAX_SIZE 1024
#endif

// Timeouts (in milliseconds)
#ifndef Z_CONNECT_TIMEOUT
#define Z_CONNECT_TIMEOUT 10000
#endif

#ifndef Z_SCOUT_TIMEOUT
#define Z_SCOUT_TIMEOUT 3000
#endif

// Thread stack sizes
#ifndef Z_THREAD_STACK_SIZE_DEFAULT
#define Z_THREAD_STACK_SIZE_DEFAULT 4096
#endif

#ifndef Z_THREAD_PRIORITY_DEFAULT
#define Z_THREAD_PRIORITY_DEFAULT 10
#endif

// Network interface
#ifndef Z_LINK_MTU
#define Z_LINK_MTU 1500
#endif

// Session defaults
#ifndef Z_SESSION_LEASE_DEFAULT
#define Z_SESSION_LEASE_DEFAULT 10000
#endif

#ifndef Z_SESSION_KEEP_ALIVE_DEFAULT
#define Z_SESSION_KEEP_ALIVE_DEFAULT 4
#endif

// RT-Thread specific includes
#include <rtthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>

#endif /* ZENOH_PICO_CONFIG_RTTHREAD_H */