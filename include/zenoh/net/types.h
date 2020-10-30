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

#ifndef ZENOH_C_NET_TYPES_H_
#define ZENOH_C_NET_TYPES_H_

#include <stdint.h>
#include <string.h>
#include "zenoh/net/config.h"
#include "zenoh/private/collection.h"
#include "zenoh/private/iobuf.h"
#include "zenoh/types.h"

#if (ZENOH_LINUX == 1) || (ZENOH_MACOS == 1)
#include "zenoh/net/private/unix/types.h"
#elif (ZENOH_CONTIKI == 1)
#include "zenoh/net/private/contiki/types.h"
#endif

#define ZN_WHATAMI_ROUTER 0x01 // 1 << 0
#define ZN_WHATAMI_PEER 0x02   // 1 << 1
#define ZN_WHATAMI_CLIENT 0x04 // 1 << 2

#define ZN_PUSH_MODE 0x01
#define ZN_PULL_MODE 0x02

#define ZN_NO_RESOURCE_ID 0

#define ZN_DEST_STORAGES_KEY 0x10
#define ZN_DEST_EVALS_KEY 0x11

#define ZN_STORAGE_DATA 0
#define ZN_STORAGE_FINAL 1
#define ZN_EVAL_DATA 2
#define ZN_EVAL_FINAL 3
#define ZN_REPLY_FINAL 4

#define ZN_T_STAMP 0x10
#define ZN_KIND 0x20
#define ZN_ENCODING 0x40

#define ZN_QTGT_BEST_MATCH 0
#define ZN_QTGT_COMPLETE 1
#define ZN_QTGT_ALL 2
#define ZN_QTGT_NONE 3

#define ZN_QCON_NONE 0
#define ZN_QCON_LAST_HOP 1
#define ZN_QCON_INCREMENTAL 2

/*=============================*/
/*       DataInfo flags        */
/*=============================*/
#define ZN_DATA_INFO_SRC_ID 0x01 // 1 << 0
#define ZN_DATA_INFO_SRC_SN 0x02 // 1 << 1
#define ZN_DATA_INFO_RTR_ID 0x04 // 1 << 2
#define ZN_DATA_INFO_RTR_SN 0x08 // 1 << 3
#define ZN_DATA_INFO_TSTAMP 0x10 // 1 << 4
#define ZN_DATA_INFO_KIND 0x20   // 1 << 5
#define ZN_DATA_INFO_ENC 0x40    // 1 << 6

/**
 * A zenoh-net property.
 *
 * Members:
 *   size_t id: The property ID.
 *   z_string_t value: The property value as a string.
 *
 */
typedef struct
{
    size_t id;
    z_string_t value;
} zn_property_t;

/**
 * Zenoh-net properties are represented as int-string map.
 * 
 */
typedef _z_i_map_t zn_properties_t;

/**
 * A zenoh-net resource key.
 *
 * Members:
 *   z_zint_t: The resource ID.
 *   const char *val: A pointer to the string containing the resource name.
 *
 */
typedef struct
{
    z_zint_t rid;
    z_str_t rname;
} zn_reskey_t;

/**
 * A zenoh-net data sample.
 *
 * A sample is the value associated to a given resource at a given point in time.
 *
 * Members:
 *   zn_string_t key: The resource key of this data sample.
 *   zn_bytes_t value: The value of this data sample.
 */
typedef struct
{
    z_string_t key;
    z_bytes_t value;
} zn_sample_t;

/**
 * A hello message returned by a zenoh entity to a scout message sent with :c:func:`zn_scout`.
 *
 * Members:
 *   unsigned int whatami: The kind of zenoh entity.
 *   zn_bytes_t pid: The peer id of the scouted entity (empty if absent).
 *   zn_str_array_t locators: The locators of the scouted entity.
 *
 */
typedef struct zn_hello_t
{
    z_zint_t whatami;
    z_bytes_t pid;
    z_str_array_t locators;
} zn_hello_t;

/**
 * An array of :c:struct:`zn_hello_t` messages.
 *
 * Members:
 *   const zn_hello_t *val: A pointer to the array.
 *   unsigned int len: The length of the array.
 *
 */
typedef struct zn_hello_array_t
{
    size_t len;
    const zn_hello_t *val;
} zn_hello_array_t;

/**
 * The possible values of :c:member:`zn_target_t.tag`.
 *
 *     - **zn_target_t_BEST_MATCHING**: The nearest complete queryable if any else all matching queryables.
 *     - **zn_target_t_COMPLETE**: A set of complete queryables.
 *     - **zn_target_t_ALL**: All matching queryables.
 *     - **zn_target_t_NONE**: No queryables.
 */
typedef enum
{
    zn_target_t_BEST_MATCHING,
    zn_target_t_COMPLETE,
    zn_target_t_ALL,
    zn_target_t_NONE,
} zn_target_t_Tag;

typedef struct
{
    unsigned int n;
} zn_target_t_COMPLETE_Body;

typedef struct
{
    zn_target_t_Tag tag;
    union
    {
        zn_target_t_COMPLETE_Body complete;
    } type;
} zn_target_t;

/**
 * The zenoh-net queryables that should be target of a :c:func:`zn_query`.
 *
 * Members:
 *     unsigned int kind: A mask of queryable kinds.
 *     zn_target_t target: The query target.
 */
typedef struct zn_query_target_t
{
    unsigned int kind;
    zn_target_t target;
} zn_query_target_t;

/**
 * Information on the source of a reply.
 *
 * Members:
 *   unsigned int kind: The kind of source.
 *   zn_bytes_t id: The unique id of the source.
 */
typedef struct
{
    unsigned int kind;
    z_bytes_t id;
} zn_source_info_t;

/**
 * The kind of consolidation that should be applied on replies to a :c:func:`zn_query`.
 *
 *     - **zn_consolidation_mode_t_FULL**: Guaranties unicity of replies. Optimizes bandwidth.
 *     - **zn_consolidation_mode_t_LAZY**: Does not garanty unicity. Optimizes latency.
 *     - **zn_consolidation_mode_t_NONE**: No consolidation.
 */
typedef enum
{
    zn_consolidation_mode_t_FULL,
    zn_consolidation_mode_t_LAZY,
    zn_consolidation_mode_t_NONE,
} zn_consolidation_mode_t;

/**
 * The kind of consolidation that should be applied on replies to a :c:func:`zn_query`
 * at the different stages of the reply process.
 *
 * Members:
 *   zn_consolidation_mode_t first_routers: The consolidation mode to apply on first routers of the replies routing path.
 *   zn_consolidation_mode_t last_router: The consolidation mode to apply on last router of the replies routing path.
 *   zn_consolidation_mode_t reception: The consolidation mode to apply at reception of the replies.
 */
typedef struct zn_query_consolidation_t
{
    zn_consolidation_mode_t first_routers;
    zn_consolidation_mode_t last_router;
    zn_consolidation_mode_t reception;
} zn_query_consolidation_t;

/**
 * The subscription reliability.
 *
 *     - **zn_reliability_t_BEST_EFFORT**
 *     - **zn_reliability_t_RELIABLE**
 */
typedef enum
{
    zn_reliability_t_BEST_EFFORT,
    zn_reliability_t_RELIABLE,
} zn_reliability_t;

/**
 * The congestion control.
 *
 *     - **zn_congestion_control_t_BLOCK**
 *     - **zn_congestion_control_t_DROP**
 */
typedef enum
{
    zn_congestion_control_t_BLOCK,
    zn_congestion_control_t_DROP,
} zn_congestion_control_t;

/**
 * The subscription period.
 *
 * Members:
 *     unsigned int origin:
 *     unsigned int period:
 *     unsigned int duration:
 */
typedef struct
{
    unsigned int origin;
    unsigned int period;
    unsigned int duration;
} zn_period_t;

/**
 * The subscription mode.
 *
 *     - **zn_submode_t_PUSH**
 *     - **zn_submode_t_PULL**
 */
typedef enum
{
    zn_submode_t_PUSH,
    zn_submode_t_PULL,
} zn_submode_t;

/**
 * Informations to be passed to :c:func:`zn_declare_subscriber` to configure the created :c:type:`zn_subscriber_t`.
 *
 * Members:
 *     zn_reliability_t reliability: The subscription reliability.
 *     zn_submode_t mode: The subscription mode.
 *     zn_period_t *period: The subscription period.
 */
typedef struct
{
    zn_reliability_t reliability;
    zn_submode_t mode;
    zn_period_t *period;
} zn_subinfo_t;

// typedef struct
// {
//     char kind;
//     const unsigned char *srcid;
//     size_t srcid_length;
//     z_zint_t rsn;
//     const char *rname;
//     const unsigned char *data;
//     size_t data_length;
//     zn_data_info_t info;
// } zn_reply_value_t;

// @TODO: define zn_query_t
typedef void zn_query_t;

typedef void (*zn_data_handler_t)(const zn_sample_t *, const void *arg);
typedef void (*zn_query_handler_t)(zn_query_t *, const void *arg);

// typedef void (*zn_reply_handler_t)(const zn_reply_value_t *reply, void *arg);
// typedef void (*zn_replies_sender_t)(void *query_handle, zn_resource_p_array_t replies);
typedef void (*zn_on_disconnect_t)(void *z);

/**
 * A zenoh-net session.
 * 
 */
typedef struct
{
    // Socket and internal buffers
    _zn_socket_t sock;
    _zn_mutex_t mutex_rx;
    _zn_mutex_t mutex_tx;

    _z_wbuf_t wbuf;
    _z_rbuf_t rbuf;
    _z_wbuf_t dbuf;

    // Connection state
    z_bytes_t local_pid;
    z_bytes_t remote_pid;

    char *locator;

    z_zint_t lease;
    z_zint_t sn_resolution;
    z_zint_t sn_resolution_half;

    // SN numbers
    z_zint_t sn_tx_reliable;
    z_zint_t sn_tx_best_effort;
    z_zint_t sn_rx_reliable;
    z_zint_t sn_rx_best_effort;

    // Counters
    z_zint_t resource_id;
    z_zint_t entity_id;
    z_zint_t query_id;

    // Declarations
    _z_list_t *local_resources;
    _z_list_t *remote_resources;

    _z_list_t *local_subscriptions;
    _z_list_t *remote_subscriptions;

    _z_i_map_t *rem_res_loc_sub_map;

    _z_list_t *local_publishers;
    _z_list_t *local_queryables;

    _z_list_t *replywaiters;

    // Runtime
    zn_on_disconnect_t on_disconnect;

    volatile int recv_loop_running;
    void *recv_loop_thread;

    volatile int lease_loop_running;
    volatile int received;
    volatile int transmitted;
    void *lease_loop_thread;
} zn_session_t;

typedef struct
{
    zn_session_t *z;
    z_zint_t id;
    zn_reskey_t key;
} zn_resource_t;

typedef struct
{
    zn_session_t *z;
    z_zint_t id;
    zn_reskey_t key;
} zn_publisher_t;

typedef struct
{
    zn_session_t *z;
    zn_subinfo_t info;
    zn_reskey_t key;
    z_zint_t id;
} zn_subscriber_t;

typedef struct
{
    zn_session_t *z;
    z_zint_t rid;
    z_zint_t id;
} zn_queryable_t;

#endif /* ZENOH_C_NET_TYPES_H_ */
