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

#include <string.h>
#include "zenoh/iobuf.h"
#include "zenoh/mvar.h"
#include "zenoh/net/result.h"
#include "zenoh/net/collection.h"
#include "zenoh/net/config.h"
#include "zenoh/net/property.h"

#if (ZENOH_LINUX ==1) || (ZENOH_MACOS == 1)
#include "zenoh/net/private/unix/types.h"
#elif (ZENOH_CONTIKI == 1)
#include "zenoh/net/private/contiki/types.h"
#endif

#define ZN_PUSH_MODE 0x01
#define ZN_PULL_MODE 0x02
#define ZN_PERIODIC_PUSH_MODE 0x03
#define ZN_PERIODIC_PULL_MODE 0x04

#define ZN_INT_RES_KEY 0
#define ZN_STR_RES_KEY 1

#define ZN_DEST_STORAGES_KEY 0x10
#define ZN_DEST_EVALS_KEY 0x11

#define ZN_STORAGE_DATA 0
#define ZN_STORAGE_FINAL 1
#define ZN_EVAL_DATA 2
#define ZN_EVAL_FINAL 3
#define ZN_REPLY_FINAL 4

#define ZN_BEST_MATCH 0
#define ZN_COMPLETE 1
#define ZN_ALL 2
#define ZN_NONE 3

#define ZN_T_STAMP 0x10
#define ZN_KIND 0x20
#define ZN_ENCODING 0x40

typedef struct {
  uint8_t kind;
  zn_temporal_property_t tprop;
} zn_sub_mode_t;
ZN_RESULT_DECLARE (zn_sub_mode_t, sub_mode)

typedef struct {
  unsigned int flags;
  z_timestamp_t tstamp;
  uint8_t encoding;
  unsigned short kind;
} zn_data_info_t;

typedef struct {
  char kind;
  const unsigned char *srcid;
  size_t srcid_length;
  z_vle_t rsn;
  const char* rname;
  const unsigned char *data;
  size_t data_length;
  zn_data_info_t info;
} zn_reply_value_t;

typedef union {
  z_vle_t rid;
  char *rname;
} zn_res_key_t;

typedef struct {
  int kind;
  zn_res_key_t key;
} zn_resource_key_t;

typedef void (*zn_reply_handler_t)(const zn_reply_value_t *reply, void *arg);

typedef void (*zn_data_handler_t)(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg);

typedef struct {
  const char* rname;
  const unsigned char *data;
  size_t length;
  unsigned short encoding;
  unsigned short kind;
  void *context;
} zn_resource_t;

ZN_ARRAY_P_DECLARE(resource)

typedef void (*zn_replies_sender_t)(void* query_handle, zn_resource_p_array_t replies);
typedef void (*zn_query_handler_t)(const char *rname, const char *predicate, zn_replies_sender_t send_replies, void *query_handle, void *arg);
typedef void (*zn_on_disconnect_t)(void *z);

typedef struct {
  _zn_socket_t sock;
  z_vle_t sn;
  z_vle_t cid;
  z_vle_t rid;
  z_vle_t eid;
  z_iobuf_t wbuf;
  z_iobuf_t rbuf;
  z_uint8_array_t pid;
  z_uint8_array_t peer_pid;
  z_vle_t qid;
  char *locator;
  zn_on_disconnect_t on_disconnect;
  z_list_t *declarations;
  z_list_t *subscriptions;
  z_list_t *storages;
  z_list_t *evals;
  z_list_t *replywaiters;
  z_i_map_t *remote_subs;
  z_mvar_t *reply_msg_mvar;
  volatile int running;
  void *thread;
} zn_session_t;


typedef struct {
  zn_session_t *z;
  z_vle_t rid;
  z_vle_t id;
} zn_sub_t;

typedef struct {
  zn_session_t *z;
  z_vle_t rid;
  z_vle_t id;
} zn_sto_t;

typedef struct {
  zn_session_t *z;
  z_vle_t rid;
  z_vle_t id;
} zn_pub_t;

typedef struct {
  zn_session_t *z;
  z_vle_t rid;
  z_vle_t id;
} zn_eva_t;

ZN_P_RESULT_DECLARE(zn_session_t, session)
ZN_P_RESULT_DECLARE(zn_sub_t, sub)
ZN_P_RESULT_DECLARE(zn_sto_t, sto)
ZN_P_RESULT_DECLARE(zn_pub_t, pub)
ZN_P_RESULT_DECLARE(zn_eva_t, eval)

typedef struct {
  uint8_t kind;
  uint8_t nb;
} zn_query_dest_t;

#endif /* ZENOH_C_NET_TYPES_H_ */
