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

#ifndef ZENOH_C_NET_INTERNAL_H
#define ZENOH_C_NET_INTERNAL_H

#include "zenoh/net/types.h"

typedef struct {
  z_vle_t rid;
  char* r_name;
} _zn_res_decl_t;

z_vle_t _zn_get_entity_id(zn_session_t *z);
z_vle_t _zn_get_resource_id(zn_session_t *z, const char *rname);
int _zn_register_res_decl(zn_session_t *z, z_vle_t rid, const char *rname);
_zn_res_decl_t *_zn_get_res_decl_by_rid(zn_session_t *z, z_vle_t rid);
_zn_res_decl_t *_zn_get_res_decl_by_rname(zn_session_t *z, const char *rname);

void _zn_register_subscription(zn_session_t *z, z_vle_t rid, z_vle_t id, zn_data_handler_t data_handler, void *arg);
const char * _zn_get_resource_name(zn_session_t *z, z_vle_t rid);
z_list_t * _zn_get_subscriptions_by_rid(zn_session_t *z, z_vle_t rid);
z_list_t * _zn_get_subscriptions_by_rname(zn_session_t *z, const char *rname);
void _zn_unregister_subscription(zn_sub_t *s) ;

void _zn_register_storage(zn_session_t *z, z_vle_t rid, z_vle_t id, zn_data_handler_t data_handler, zn_query_handler_t query_handler, void *arg);
z_list_t * _zn_get_storages_by_rid(zn_session_t *z, z_vle_t rid);
z_list_t * _zn_get_storages_by_rname(zn_session_t *z, const char *rname);
void _zn_unregister_storage(zn_sto_t *s) ;

void _zn_register_eval(zn_session_t *z, z_vle_t rid, z_vle_t id, zn_query_handler_t query_handler, void *arg);
z_list_t * _zn_get_evals_by_rid(zn_session_t *z, z_vle_t rid);
z_list_t * _zn_get_evals_by_rname(zn_session_t *z, const char *rname);
void _zn_unregister_eval(zn_eva_t *s) ;

int _zn_matching_remote_sub(zn_session_t *z, z_vle_t rid);

typedef struct {
  z_vle_t qid;
  zn_reply_handler_t reply_handler;
  void *arg;
} _zn_replywaiter_t;

void _zn_register_query(zn_session_t *z, z_vle_t qid, zn_reply_handler_t reply_handler, void *arg);
_zn_replywaiter_t *_zn_get_query(zn_session_t *z, z_vle_t qid);

typedef struct {
  char *rname;
  z_vle_t rid;
  z_vle_t id;
  zn_data_handler_t data_handler;
  void *arg;
}  _zn_sub_t;

typedef struct {
  char *rname;
  z_vle_t rid;
  z_vle_t id;
  zn_data_handler_t data_handler;
  zn_query_handler_t query_handler;
  void *arg;
}  _zn_sto_t;

typedef struct {
  char *rname;
  z_vle_t rid;
  z_vle_t id;
  zn_query_handler_t query_handler;
  void *arg;
}  _zn_eva_t;

#endif /* ZENOH_C_NET_INTERNAL_H */
