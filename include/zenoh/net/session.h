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

z_vec_t
zn_scout(char* iface, size_t tries, size_t period);

zn_session_p_result_t
zn_open(char* locator, zn_on_disconnect_t on_disconnect, const z_vec_t *ps);

z_vec_t
zn_info(zn_session_t *z);

zn_sub_p_result_t
zn_declare_subscriber(zn_session_t *z, const char* resource, const zn_sub_mode_t *sm, zn_data_handler_t data_handler, void *arg);

zn_pub_p_result_t
zn_declare_publisher(zn_session_t *z, const char *resource);

zn_sto_p_result_t
zn_declare_storage(zn_session_t *z, const char* resource, zn_data_handler_t data_handler, zn_query_handler_t query_handler, void *arg);

zn_eval_p_result_t
zn_declare_eval(zn_session_t *z, const char* resource, zn_query_handler_t query_handler, void *arg);

int zn_stream_compact_data(zn_pub_t *pub, const unsigned char *payload, size_t len);
int zn_stream_data(zn_pub_t *pub, const unsigned char *payload, size_t len);
int zn_write_data(zn_session_t *z, const char* resource, const unsigned char *payload, size_t len);

int zn_stream_data_wo(zn_pub_t *pub, const unsigned char *payload, size_t len, uint8_t encoding, uint8_t kind);
int zn_write_data_wo(zn_session_t *z, const char* resource, const unsigned char *payload, size_t len, uint8_t encoding, uint8_t kind);

int zn_pull(zn_sub_t *sub);

int zn_query(zn_session_t *z, const char* resource, const char* predicate, zn_reply_handler_t reply_handler, void *arg);
int zn_query_wo(zn_session_t *z, const char* resource, const char* predicate, zn_reply_handler_t reply_handler, void *arg, zn_query_dest_t dest_storages, zn_query_dest_t dest_evals);

int zn_undeclare_subscriber(zn_sub_t *z);
int zn_undeclare_publisher(zn_pub_t *z);
int zn_undeclare_storage(zn_sto_t *z);
int zn_undeclare_eval(zn_eva_t *z);

int zn_close(zn_session_t *z);

#endif /* ZENOH_C_NET_SESSION_H */
