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

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "zenoh.h"
#include "zenoh/mvar.h"
#include "zenoh/codec.h"
#include "zenoh/net/recv_loop.h"

typedef struct {
   char *name;
   unsigned char data;
} resource;

resource z1_sub1_last_res;
z_mvar_t *z1_sub1_mvar = 0;

resource z2_sub1_last_res;
z_mvar_t *z2_sub1_mvar = 0;

resource z3_sub1_last_res;
z_mvar_t *z3_sub1_mvar = 0;

resource z1_sto1_last_res;
z_mvar_t *z1_sto1_mvar = 0;

resource z2_sto1_last_res;
z_mvar_t *z2_sto1_mvar = 0;

z_list_t *storage_replies = 0;
z_mvar_t *storage_replies_mvar = 0;

z_list_t *eval_replies = 0;
z_mvar_t *eval_replies_mvar = 0;

void z1_sub1_listener(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {
  Z_UNUSED_ARG_3(length, info, arg);
  z1_sub1_last_res.name = strdup(rkey->key.rname);
  z1_sub1_last_res.data = *data;
  z_mvar_put(z1_sub1_mvar, &z1_sub1_last_res);
}

void z2_sub1_listener(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {
  Z_UNUSED_ARG_3(length, info, arg);
  z2_sub1_last_res.name = strdup(rkey->key.rname);
  z2_sub1_last_res.data = *data;
  z_mvar_put(z2_sub1_mvar, &z2_sub1_last_res);
}

void z3_sub1_listener(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {
  Z_UNUSED_ARG_3(length, info, arg);
  z3_sub1_last_res.name = strdup(rkey->key.rname);
  z3_sub1_last_res.data = *data;
  z_mvar_put(z3_sub1_mvar, &z3_sub1_last_res);
}

void z1_sto1_listener(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {
  Z_UNUSED_ARG_3(length, info, arg);
  z1_sto1_last_res.name = strdup(rkey->key.rname);
  z1_sto1_last_res.data = *data;
  z_mvar_put(z1_sto1_mvar, &z1_sto1_last_res);
}

void z2_sto1_listener(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {
  Z_UNUSED_ARG_3(length, info, arg);
  z2_sto1_last_res.name = strdup(rkey->key.rname);
  z2_sto1_last_res.data = *data;
  z_mvar_put(z2_sto1_mvar, &z2_sto1_last_res);
}

void z1_sto1_handler(const char *rname, const char *predicate, zn_replies_sender_t send_replies, void *query_handle, void *arg) {
  Z_UNUSED_ARG_3(rname, predicate, arg);
  zn_resource_t resource;
  resource.rname = z1_sto1_last_res.name;
  resource.data = &z1_sto1_last_res.data;
  resource.length = 1;
  resource.encoding = 0;
  resource.kind = 0;

  zn_resource_t *p_resource = &resource;

  zn_resource_p_array_t replies;
  replies.length = 1;
  replies.elem = &p_resource;

  send_replies(query_handle, replies);
}

void z2_sto1_handler(const char *rname, const char *predicate, zn_replies_sender_t send_replies, void *query_handle, void *arg) {
  Z_UNUSED_ARG_3(rname, predicate, arg);
  zn_resource_t resource;
  resource.rname = z2_sto1_last_res.name;
  resource.data = &z2_sto1_last_res.data;
  resource.length = 1;
  resource.encoding = 0;
  resource.kind = 0;

  zn_resource_t *p_resource = &resource;

  zn_resource_p_array_t replies;
  replies.length = 1;
  replies.elem = &p_resource;

  send_replies(query_handle, replies);
}

void z1_eval1_handler(const char *rname, const char *predicate, zn_replies_sender_t send_replies, void *query_handle, void *arg) {
  Z_UNUSED_ARG_3(rname, predicate, arg);
  zn_resource_t resource;
  resource.rname = "/test/client/z1_eval1";
  uint8_t data = 33;
  resource.data = &data;
  resource.length = 1;
  resource.encoding = 0;
  resource.kind = 0;

  zn_resource_t *p_resource = &resource;

  zn_resource_p_array_t replies;
  replies.length = 1;
  replies.elem = &p_resource;

  send_replies(query_handle, replies);
}

void z2_eval1_handler(const char *rname, const char *predicate, zn_replies_sender_t send_replies, void *query_handle, void *arg) {
  Z_UNUSED_ARG_3(rname, predicate, arg);
  zn_resource_t resource;
  resource.rname = "/test/client/z2_eval1";
  uint8_t data = 66;
  resource.data = &data;
  resource.length = 1;
  resource.encoding = 0;
  resource.kind = 0;

  zn_resource_t *p_resource = &resource;

  zn_resource_p_array_t replies;
  replies.length = 1;
  replies.elem = &p_resource;

  send_replies(query_handle, replies);
}

void reply_handler(const zn_reply_value_t *reply, void *arg) {
  Z_UNUSED_ARG(arg);
  resource *res;
  switch (reply->kind) {
    case ZN_STORAGE_DATA:
      res = (resource*)malloc(sizeof(resource));
      res->name = strdup(reply->rname);
      res->data = *reply->data;
      storage_replies = z_list_cons(storage_replies, res);
      break;
    case ZN_EVAL_DATA:
      res = (resource*)malloc(sizeof(resource));
      res->name = strdup(reply->rname);
      res->data = *reply->data;
      eval_replies = z_list_cons(eval_replies, res);
      break;
    case ZN_STORAGE_FINAL:
    case ZN_EVAL_FINAL:
      break;
    case ZN_REPLY_FINAL:
      z_mvar_put(storage_replies_mvar, storage_replies);
      z_mvar_put(eval_replies_mvar, eval_replies);
      break;
  }
}

int main(int argc, char **argv) {
  Z_UNUSED_ARG_2(argc, argv);

  z1_sub1_mvar = z_mvar_empty();
  z2_sub1_mvar = z_mvar_empty();
  z3_sub1_mvar = z_mvar_empty();
  z1_sto1_mvar = z_mvar_empty();
  z2_sto1_mvar = z_mvar_empty();
  storage_replies_mvar = z_mvar_empty();
  eval_replies_mvar = z_mvar_empty();

  char *locator = strdup("tcp/127.0.0.1:7447");

  zn_sub_mode_t push_mode = {ZN_PUSH_MODE, {0, 0, 0}};
  zn_sub_mode_t pull_mode = {ZN_PULL_MODE, {0, 0, 0}};
  zn_query_dest_t best_match = {ZN_BEST_MATCH, 0};
  zn_query_dest_t none = {ZN_NONE, 0};

  zn_session_p_result_t z1_r = zn_open(locator, 0, 0);
  ASSERT_RESULT(z1_r, "Unable to open session with broker")
  zn_session_t *z1 = z1_r.value.session;
  zn_start_recv_loop(z1);

  z_vec_t z1_info = zn_info(z1);
  assert(0 == strcmp(locator, (const char *)((zn_property_t *)z_vec_get(&z1_info, ZN_INFO_PEER_KEY))->value.elem));
  z_vec_free(&z1_info);

  zn_sub_p_result_t z1_sub1_r = zn_declare_subscriber(z1, "/test/client/**", &push_mode, z1_sub1_listener, NULL);
  ASSERT_P_RESULT(z1_sub1_r,"Unable to declare subscriber\n");
  zn_sub_t *z1_sub1 = z1_sub1_r.value.sub;

  zn_sto_p_result_t z1_sto1_r = zn_declare_storage(z1, "/test/client/**", z1_sto1_listener, z1_sto1_handler, NULL);
  ASSERT_P_RESULT(z1_sto1_r, "Unable to declare storage\n");
  zn_sto_t *z1_sto1 = z1_sto1_r.value.sto;

  zn_eval_p_result_t z1_eval1_r = zn_declare_eval(z1, "/test/client/z1_eval1", z1_eval1_handler, NULL);
  ASSERT_P_RESULT(z1_eval1_r, "Unable to declare eval\n");
  zn_eva_t *z1_eval1 = z1_eval1_r.value.eval;

  zn_pub_p_result_t z1_pub1_r = zn_declare_publisher(z1, "/test/client/z1_pub1");
  ASSERT_P_RESULT(z1_pub1_r, "Unable to declare publisher\n");
  zn_pub_t *z1_pub1 = z1_pub1_r.value.pub;

  zn_session_p_result_t z2_r = zn_open(locator, 0, 0);
  ASSERT_RESULT(z2_r, "Unable to open session with broker")
  zn_session_t *z2 = z2_r.value.session;
  zn_start_recv_loop(z2);

  z_vec_t z2_info = zn_info(z2);
  assert(0 == strcmp(locator, (const char *)((zn_property_t *)z_vec_get(&z2_info, ZN_INFO_PEER_KEY))->value.elem));
  z_vec_free(&z2_info);

  zn_sub_p_result_t z2_sub1_r = zn_declare_subscriber(z2, "/test/client/**", &push_mode, z2_sub1_listener, NULL);
  ASSERT_P_RESULT(z2_sub1_r,"Unable to declare subscriber\n");
  zn_sub_t *z2_sub1 = z2_sub1_r.value.sub;

  zn_sto_p_result_t z2_sto1_r = zn_declare_storage(z2, "/test/client/**", z2_sto1_listener, z2_sto1_handler, NULL);
  ASSERT_P_RESULT(z2_sto1_r, "Unable to declare storage\n");
  zn_sto_t *z2_sto1 = z2_sto1_r.value.sto;

  zn_eval_p_result_t z2_eval1_r = zn_declare_eval(z2, "/test/client/z2_eval1", z2_eval1_handler, NULL);
  ASSERT_P_RESULT(z2_eval1_r, "Unable to declare eval\n");
  zn_eva_t *z2_eval1 = z2_eval1_r.value.eval;

  zn_pub_p_result_t z2_pub1_r = zn_declare_publisher(z2, "/test/client/z2_pub1");
  ASSERT_P_RESULT(z2_pub1_r, "Unable to declare publisher\n");
  zn_pub_t *z2_pub1 = z2_pub1_r.value.pub;

  zn_session_p_result_t z3_r = zn_open(locator, 0, 0);
  ASSERT_RESULT(z3_r, "Unable to open session with broker")
  zn_session_t *z3 = z3_r.value.session;
  zn_start_recv_loop(z3);

  z_vec_t z3_info = zn_info(z3);
  assert(0 == strcmp(locator, (const char *)((zn_property_t *)z_vec_get(&z3_info, ZN_INFO_PEER_KEY))->value.elem));
  z_vec_free(&z3_info);

  zn_sub_p_result_t z3_sub1_r = zn_declare_subscriber(z3, "/test/client/**", &pull_mode, z3_sub1_listener, NULL);
  ASSERT_P_RESULT(z3_sub1_r,"Unable to declare subscriber\n");
  zn_sub_t *z3_sub1 = z3_sub1_r.value.sub;

  sleep(1);

  assert(1 == zn_running(z1));
  assert(1 == zn_running(z2));
  assert(1 == zn_running(z3));

  resource  sent_res;
  resource *rcvd_res;

  sent_res.name = "/test/client/z1_wr1";
  sent_res.data = 12;
  zn_write_data(z1, sent_res.name, &sent_res.data, 1);
  rcvd_res = z_mvar_get(z1_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z2_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z1_sto1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z2_sto1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  assert(z_mvar_is_empty(z3_sub1_mvar));
  zn_pull(z3_sub1);
  rcvd_res = z_mvar_get(z3_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z3_sub1_mvar = z_mvar_empty();
  assert(z_mvar_is_empty(z3_sub1_mvar));

  zn_query(z1, "/test/client/**", "", reply_handler, NULL);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  assert(2 == z_list_len(eval_replies));
  z_list_free(&eval_replies);

  zn_query_wo(z1, "/test/client/**", "", reply_handler, NULL, best_match, none);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  // assert(0 == z_list_len(eval_replies));
  // This may not be true for now as :
  //  - zenoh-c does not check received query properties
  //  - zenohd does not filter out replies
  z_list_free(&eval_replies);

  zn_query_wo(z1, "/test/client/**", "", reply_handler, NULL, none, best_match);
  z_mvar_get(storage_replies_mvar);
  // assert(0 == z_list_len(storage_replies));
  // This may not be true for now as :
  //  - zenoh-c does not check received query properties
  //  - zenohd does not filter out replies
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  assert(2 == z_list_len(eval_replies));
  z_list_free(&eval_replies);

  zn_query(z2, "/test/client/**", "", reply_handler, NULL);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  assert(2 == z_list_len(eval_replies));
  z_list_free(&eval_replies);

  zn_query_wo(z2, "/test/client/**", "", reply_handler, NULL, best_match, none);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  // assert(0 == z_list_len(eval_replies));
  // This may not be true for now as :
  //  - zenoh-c does not check received query properties
  //  - zenohd does not filter out replies
  z_list_free(&eval_replies);

  zn_query_wo(z2, "/test/client/**", "", reply_handler, NULL, none, best_match);
  z_mvar_get(storage_replies_mvar);
  // assert(0 == z_list_len(storage_replies));
  // This may not be true for now as :
  //  - zenoh-c does not check received query properties
  //  - zenohd does not filter out replies
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  assert(2 == z_list_len(eval_replies));
  z_list_free(&eval_replies);


  sent_res.name = "/test/client/**";
  sent_res.data = 5;
  zn_write_data(z2, sent_res.name, &sent_res.data, 1);
  rcvd_res = z_mvar_get(z1_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z2_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z1_sto1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z2_sto1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  assert(z_mvar_is_empty(z3_sub1_mvar));
  zn_pull(z3_sub1);
  rcvd_res = z_mvar_get(z3_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z3_sub1_mvar = z_mvar_empty();
  assert(z_mvar_is_empty(z3_sub1_mvar));

  zn_query(z1, "/test/client/**", "", reply_handler, NULL);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  assert(2 == z_list_len(eval_replies));
  z_list_free(&eval_replies);

  zn_query(z2, "/test/client/**", "", reply_handler, NULL);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  assert(2 == z_list_len(eval_replies));
  z_list_free(&eval_replies);


  sent_res.name = "/test/client/z1_pub1";
  sent_res.data = 23;
  zn_stream_data(z1_pub1, &sent_res.data, 1);
  rcvd_res = z_mvar_get(z1_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z2_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z1_sto1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z2_sto1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  assert(z_mvar_is_empty(z3_sub1_mvar));
  zn_pull(z3_sub1);
  rcvd_res = z_mvar_get(z3_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z3_sub1_mvar = z_mvar_empty();
  assert(z_mvar_is_empty(z3_sub1_mvar));

  zn_query(z1, "/test/client/**", "", reply_handler, NULL);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  assert(2 == z_list_len(eval_replies));
  z_list_free(&eval_replies);

  zn_query(z2, "/test/client/**", "", reply_handler, NULL);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);
  z_mvar_get(eval_replies_mvar);
  assert(2 == z_list_len(eval_replies));
  z_list_free(&eval_replies);


  sent_res.name = "/test/client/z2_pub1";
  sent_res.data = 54;
  zn_stream_data(z2_pub1, &sent_res.data, 1);
  rcvd_res = z_mvar_get(z1_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z2_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z1_sto1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  rcvd_res = z_mvar_get(z2_sto1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  assert(z_mvar_is_empty(z3_sub1_mvar));
  zn_pull(z3_sub1);
  rcvd_res = z_mvar_get(z3_sub1_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z3_sub1_mvar = z_mvar_empty();
  assert(z_mvar_is_empty(z3_sub1_mvar));

  zn_query(z1, "/test/client/**", "", reply_handler, NULL);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);

  zn_query(z2, "/test/client/**", "", reply_handler, NULL);
  z_mvar_get(storage_replies_mvar);
  assert(2 == z_list_len(storage_replies));
  rcvd_res = z_list_head(storage_replies);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.data == rcvd_res->data);
  z_list_free(&storage_replies);

  zn_undeclare_subscriber(z1_sub1);
  zn_undeclare_subscriber(z2_sub1);
  zn_undeclare_subscriber(z3_sub1);
  zn_undeclare_storage(z1_sto1);
  zn_undeclare_storage(z2_sto1);
  zn_undeclare_eval(z1_eval1);
  zn_undeclare_eval(z2_eval1);
  zn_undeclare_publisher(z1_pub1);
  zn_undeclare_publisher(z2_pub1);

  zn_close(z1);
  zn_close(z2);
  zn_close(z3);

  sleep(1); // let time for close msg to comme back from router

  zn_stop_recv_loop(z1);
  zn_stop_recv_loop(z2);
  zn_stop_recv_loop(z3);

  assert(0 == zn_running(z1));
  assert(0 == zn_running(z2));
  assert(0 == zn_running(z3));
}
