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
#include "zenoh.h"

#define MAX_LEN 256

void reply_handler(const zn_reply_value_t *reply, void *arg) {
  Z_UNUSED_ARG(arg);
  char str[MAX_LEN];
  switch (reply->kind) {
    case ZN_STORAGE_DATA: 
    case ZN_EVAL_DATA: 
      memcpy(&str, reply->data, reply->data_length < MAX_LEN ? reply->data_length : MAX_LEN - 1);
      str[reply->data_length < MAX_LEN ? reply->data_length : MAX_LEN - 1] = 0;
      switch (reply->kind) {
        case ZN_STORAGE_DATA: printf(">> [Reply handler] received -Storage Data- ('%s': '%s')\n", reply->rname, str);break;
        case ZN_EVAL_DATA:    printf(">> [Reply handler] received -Eval Data-    ('%s': '%s')\n", reply->rname, str);break;
      }
      break;
    case ZN_STORAGE_FINAL:
      printf(">> [Reply handler] received -Storage Final-\n");
      break;
    case ZN_EVAL_FINAL:
      printf(">> [Reply handler] received -Eval Final-\n");
      break;
    case ZN_REPLY_FINAL:
      printf(">> [Reply handler] received -Reply Final-\n");
      break;
  }
}

int main(int argc, char **argv) {
  char *selector = "/zenoh/examples/**";
  char *locator = 0;

  if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
    printf("USAGE:\n\tzn_query [<selector>=%s] [<locator>=auto]\n\n", selector);
    return 0;
  }
  if (argc > 1) {
    selector = argv[1];
  }
  if (argc > 2) {
    locator = argv[2];
  }

  printf("Openning session...\n");
  zn_session_p_result_t r_z = zn_open(locator, 0, 0);
  ASSERT_RESULT(r_z, "Unable to open session.\n")
  zn_session_t *z = r_z.value.session;
  zn_start_recv_loop(z);

  printf("Sending Query '%s'...\n", selector);
  zn_query_dest_t dest_all = {ZN_ALL, 0};
  if (zn_query_wo(z, selector, "", reply_handler, NULL, dest_all, dest_all) != 0) {
    printf("Unable to query.\n");
    return -1;
  }

  sleep(1);

  zn_close(z);
  zn_stop_recv_loop(z);
  return 0;
}
