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
#include "zenoh/net/recv_loop.h"
#include "zenoh/mvar.h"

typedef struct {
   char *name;
   unsigned char *data;
   size_t length;
} resource;

resource sub_last_res;
z_mvar_t *sub_mvar = 0;

resource sto_last_res;
z_mvar_t *sto_mvar = 0;

resource rep_last_res;
z_mvar_t *rep_mvar = 0;

void sub_listener(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {
  Z_UNUSED_ARG_2(info, arg);
  sub_last_res.name = strdup(rkey->key.rname);
  sub_last_res.data = malloc(length);
  memcpy(sub_last_res.data, data, length);
  sub_last_res.length = length;
  z_mvar_put(sub_mvar, &sub_last_res);
}

void sto_listener(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {
  Z_UNUSED_ARG_2(info, arg);
  sto_last_res.name = strdup(rkey->key.rname);
  sto_last_res.data = malloc(length);
  memcpy(sto_last_res.data, data, length);
  sto_last_res.length = length;
  z_mvar_put(sto_mvar, &sto_last_res);
}

void sto_handler(const char *rname, const char *predicate, zn_replies_sender_t send_replies, void *query_handle, void *arg) {
  Z_UNUSED_ARG_3(rname, predicate, arg);
  zn_resource_t resource;
  resource.rname = sto_last_res.name;
  resource.data = sto_last_res.data;
  resource.length = sto_last_res.length;;
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
  switch (reply->kind) {
    case ZN_STORAGE_DATA:
      rep_last_res.name = strdup(reply->rname);
      rep_last_res.data = malloc(reply->data_length);
      memcpy(rep_last_res.data, reply->data, reply->data_length);
      rep_last_res.length = reply->data_length;
      break;
    case ZN_STORAGE_FINAL:
      break;
    case ZN_REPLY_FINAL:
      z_mvar_put(rep_mvar, &rep_last_res);
      break;
  }
}

int main(int argc, char **argv) {
  Z_UNUSED_ARG_2(argc, argv);

  sub_mvar = z_mvar_empty();
  sto_mvar = z_mvar_empty();
  rep_mvar = z_mvar_empty();

  char *locator = strdup("tcp/127.0.0.1:7447");

  zn_session_p_result_t z1_r = zn_open(locator, 0, 0);
  ASSERT_RESULT(z1_r, "Unable to open session with broker")
  zn_session_t *z1 = z1_r.value.session;
  zn_start_recv_loop(z1);

  zn_pub_p_result_t z1_pub1_r = zn_declare_publisher(z1, "/test/large_data/big");
  ASSERT_P_RESULT(z1_pub1_r, "Unable to declare publisher\n");
  zn_pub_t *z1_pub1 = z1_pub1_r.value.pub;

  zn_session_p_result_t z2_r = zn_open(locator, 0, 0);
  ASSERT_RESULT(z2_r, "Unable to open session with broker")
  zn_session_t *z2 = z2_r.value.session;
  zn_start_recv_loop(z2);

  zn_sub_mode_t sm;
  sm.kind = ZN_PUSH_MODE;
  zn_sub_p_result_t z2_sub1_r = zn_declare_subscriber(z2, "/test/large_data/big", &sm, sub_listener, NULL);
  ASSERT_P_RESULT(z2_sub1_r,"Unable to declare subscriber\n");
  zn_sub_t *z2_sub1 = z2_sub1_r.value.sub;

  zn_sto_p_result_t z2_sto1_r = zn_declare_storage(z2, "/test/large_data/big", sto_listener, sto_handler, NULL);
  ASSERT_P_RESULT(z2_sto1_r,"Unable to declare storage\n");
  zn_sto_t *z2_sto1 = z2_sto1_r.value.sto;

  sleep(1);

  resource  sent_res;
  resource *rcvd_res;

  sent_res.name = "/test/large_data/big";
  sent_res.data = malloc(1000000);
  sent_res.length = 1000000;

  zn_stream_data(z1_pub1, sent_res.data, sent_res.length);
  rcvd_res = z_mvar_get(sub_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.length == rcvd_res->length);
  assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));
  rcvd_res = z_mvar_get(sto_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.length == rcvd_res->length);
  assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));

  zn_query(z1, "/test/large_data/big", "", reply_handler, NULL);
  rcvd_res = z_mvar_get(rep_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.length == rcvd_res->length);
  assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));

  zn_write_data(z1, sent_res.name, sent_res.data, sent_res.length);
  rcvd_res = z_mvar_get(sub_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.length == rcvd_res->length);
  assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));
  rcvd_res = z_mvar_get(sto_mvar);
  assert(0 == strcmp(sent_res.name, rcvd_res->name));
  assert(sent_res.length == rcvd_res->length);
  assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));

  int i;
  for(i = (ZENOH_NET_WRITE_BUF_LEN - 500); i <= (ZENOH_NET_WRITE_BUF_LEN + 500); ++i) {
    sent_res.length = i;
    zn_write_data(z1, sent_res.name, sent_res.data, sent_res.length);
    rcvd_res = z_mvar_get(sub_mvar);
    assert(0 == strcmp(sent_res.name, rcvd_res->name));
    assert(sent_res.length == rcvd_res->length);
    assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));
    rcvd_res = z_mvar_get(sto_mvar);
    assert(0 == strcmp(sent_res.name, rcvd_res->name));
    assert(sent_res.length == rcvd_res->length);
    assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));
  }
  for(i = (ZENOH_NET_WRITE_BUF_LEN + 500); i >= (ZENOH_NET_WRITE_BUF_LEN - 500); --i) {
    sent_res.length = i;
    zn_write_data(z1, sent_res.name, sent_res.data, sent_res.length);
    rcvd_res = z_mvar_get(sub_mvar);
    assert(0 == strcmp(sent_res.name, rcvd_res->name));
    assert(sent_res.length == rcvd_res->length);
    assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));
    rcvd_res = z_mvar_get(sto_mvar);
    assert(0 == strcmp(sent_res.name, rcvd_res->name));
    assert(sent_res.length == rcvd_res->length);
    assert(0 == memcmp(sent_res.data, rcvd_res->data, sent_res.length));
  }

  zn_undeclare_publisher(z1_pub1);
  zn_undeclare_subscriber(z2_sub1);
  zn_undeclare_storage(z2_sto1);

  zn_close(z1);
  zn_close(z2);

  zn_stop_recv_loop(z1);
  zn_stop_recv_loop(z2);
}
