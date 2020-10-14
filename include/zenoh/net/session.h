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

#ifndef ZENOH_C_NET_SESSION_H
#define ZENOH_C_NET_SESSION_H

#include "zenoh/net/types.h"

#define ZN_SN_KEY 0x01
#define ZN_COMPACT_CLIENT_KEY 0x02
#define ZN_USER_KEY 0x50
#define ZN_PASSWD_KEY 0x51

#define ZN_INFO_PID_KEY 0x00
#define ZN_INFO_PEER_KEY 0x01
#define ZN_INFO_PEER_PID_KEY 0x02

/*------------------ Scout/Open/Close ------------------*/
z_vec_t zn_scout(char *iface, size_t tries, size_t period);
zn_session_p_result_t zn_open(char *locator, zn_on_disconnect_t on_disconnect, const z_vec_t *ps);
int zn_close(zn_session_t *z);

z_vec_t zn_info(zn_session_t *z);

/*------------------ Declarations ------------------*/
zn_res_p_result_t zn_declare_resource(zn_session_t *z, const zn_res_key_t *res_key);
int zn_undeclare_resource(zn_res_t *r);

zn_pub_p_result_t zn_declare_publisher(zn_session_t *z, const zn_res_key_t *res_key);
int zn_undeclare_publisher(zn_pub_t *p);

zn_sub_p_result_t zn_declare_subscriber(zn_session_t *z, const zn_res_key_t *res_key, const zn_sub_info_t *si, zn_data_handler_t data_handler, void *arg);
int zn_undeclare_subscriber(zn_sub_t *s);

zn_qle_p_result_t zn_declare_queryable(zn_session_t *z, const char *resource, zn_query_handler_t query_handler, void *arg);
int zn_undeclare_queryable(zn_qle_t *q);

/*------------------ Operations ------------------*/
zn_res_key_t zn_rid(const zn_res_t *rd);
zn_res_key_t zn_rname(const char *rname);

int zn_send_keep_alive(zn_session_t *z);

int zn_write(zn_session_t *z, zn_res_key_t *resource, const unsigned char *payload, size_t len);
int zn_write_wo(zn_session_t *z, zn_res_key_t *resource, const unsigned char *payload, size_t len, uint8_t encoding, uint8_t kind, int is_droppable);

int zn_read(zn_session_t *z);

int zn_pull(zn_sub_t *sub);

int zn_query(zn_session_t *z, zn_res_key_t *resource, const char *predicate, zn_reply_handler_t reply_handler, void *arg);
int zn_query_wo(zn_session_t *z, zn_res_key_t *resource, const char *predicate, zn_reply_handler_t reply_handler, void *arg, zn_query_dest_t dest_storages, zn_query_dest_t dest_evals);

#endif /* ZENOH_C_NET_SESSION_H */
