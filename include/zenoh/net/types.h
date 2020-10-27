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
#include "zenoh/iobuf.h"
#include "zenoh/net/result.h"
#include "zenoh/net/collection.h"
#include "zenoh/net/config.h"
#include "zenoh/net/property.h"

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

// -- SubInfo is optionally included in Subscriber Declarations
// -- The reliabiliey
//
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// |P|X|X| SubMode | if S==1. Otherwise: SubMode=Push
// +---------------+
// ~    Period     ~ if P==1
// +---------------+
typedef struct
{
    int is_reliable;
    int is_periodic;
    zn_temporal_property_t period;
    uint8_t mode;
} zn_sub_info_t;
ZN_RESULT_DECLARE(zn_sub_info_t, sub_info)

// -- DataInfo is optionally included in Data Messages
//
//  7 6 5 4 3 2 1 0
// +-+-+-+---------+
// ~     flags     ~ -- encoded as z_zint_t
// +---------------+
// ~   source_id   ~ if ZN_DATA_INFO_SRC_ID==1
// +---------------+
// ~   source_sn   ~ if ZN_DATA_INFO_SRC_SN==1
// +---------------+
// ~first_router_id~ if ZN_DATA_INFO_RTR_ID==1
// +---------------+
// ~first_router_sn~ if ZN_DATA_INFO_RTR_SN==1
// +---------------+
// ~   timestamp   ~ if ZN_DATA_INFO_TSTAMP==1
// +---------------+
// ~      kind     ~ if ZN_DATA_INFO_KIND==1
// +---------------+
// ~   encoding    ~ if ZN_DATA_INFO_ENC==1
// +---------------+
//
typedef struct
{
    z_zint_t flags;
    z_uint8_array_t source_id;
    z_zint_t source_sn;
    z_uint8_array_t first_router_id;
    z_zint_t first_router_sn;
    z_timestamp_t tstamp;
    z_zint_t kind;
    z_zint_t encoding;
} zn_data_info_t;
ZN_RESULT_DECLARE(zn_data_info_t, data_info)

// -- ResKey is a field used in Declarations
//
//  7 6 5 4 3 2 1 0
// +-+-+-+-+-+-+-+-+
// ~      RID      ~
// +---------------+
// ~    Suffix     ~ if K==1
// +---------------+
typedef struct
{
    z_zint_t rid;
    char *rname;
} zn_res_key_t;
ZN_RESULT_DECLARE(zn_res_key_t, res_key)

typedef struct
{
    char kind;
    const unsigned char *srcid;
    size_t srcid_length;
    z_zint_t rsn;
    const char *rname;
    const unsigned char *data;
    size_t data_length;
    zn_data_info_t info;
} zn_reply_value_t;

typedef void (*zn_reply_handler_t)(const zn_reply_value_t *reply, void *arg);

typedef void (*zn_data_handler_t)(const zn_res_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg);

typedef struct
{
    const char *rname;
    const unsigned char *data;
    size_t length;
    unsigned short encoding;
    unsigned short kind;
    void *context;
} zn_resource_t;

ZN_ARRAY_P_DECLARE(resource)

typedef void (*zn_replies_sender_t)(void *query_handle, zn_resource_p_array_t replies);
typedef void (*zn_query_handler_t)(const char *rname, const char *predicate, zn_replies_sender_t send_replies, void *query_handle, void *arg);
typedef void (*zn_on_disconnect_t)(void *z);

typedef struct
{
    // Socket and internal buffers
    _zn_socket_t sock;
    _zn_mutex_t mutex_rx;
    _zn_mutex_t mutex_tx;

    z_wbuf_t wbuf;
    z_rbuf_t rbuf;
    z_wbuf_t dbuf;

    // Connection state
    z_uint8_array_t local_pid;
    z_uint8_array_t remote_pid;

    char *locator;

    z_zint_t lease;
    z_zint_t sn_resolution;

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
    z_list_t *local_resources;
    z_list_t *remote_resources;

    z_list_t *local_subscriptions;
    z_list_t *remote_subscriptions;

    z_i_map_t *rem_res_loc_sub_map;

    z_list_t *local_publishers;
    z_list_t *local_queryables;

    z_list_t *replywaiters;

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
    zn_res_key_t key;
} zn_res_t;

typedef struct
{
    zn_session_t *z;
    z_zint_t id;
    zn_res_key_t key;
} zn_pub_t;

typedef struct
{
    zn_session_t *z;
    zn_sub_info_t info;
    zn_res_key_t key;
    z_zint_t id;
} zn_sub_t;

typedef struct
{
    zn_session_t *z;
    z_zint_t rid;
    z_zint_t id;
} zn_qle_t;

ZN_P_RESULT_DECLARE(zn_session_t, session)
ZN_P_RESULT_DECLARE(zn_res_t, res)
ZN_P_RESULT_DECLARE(zn_sub_t, sub)
ZN_P_RESULT_DECLARE(zn_pub_t, pub)
ZN_P_RESULT_DECLARE(zn_qle_t, qle)

typedef struct
{
    uint8_t kind;
    uint8_t nb;
} zn_query_dest_t;

#endif /* ZENOH_C_NET_TYPES_H_ */
