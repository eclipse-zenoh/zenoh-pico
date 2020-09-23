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

void data_handler(const zn_resource_key_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg) {
  Z_UNUSED_ARG_2(info, arg);
  char str[MAX_LEN];
  memcpy(&str, data, length < MAX_LEN ? length : MAX_LEN - 1);
  str[length < MAX_LEN ? length : MAX_LEN - 1] = 0;
  if (rkey->kind == ZN_INT_RES_KEY) 
    printf(">> [Subscription listener] Received (#%zu: '%s')\n", rkey->key.rid, str);
  else
    printf(">> [Subscription listener] Received ('%s': '%s')\n", rkey->key.rname, str);
}

int main(int argc, char **argv) {
  char *selector = "/zenoh/examples/**";
  char *locator = 0;
  
  if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
    printf("USAGE:\n\tzn_sub [<selector>=%s] [<locator>=auto]\n\n", selector);
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

  printf("Declaring Subscriber on '%s'...\n", selector);
  zn_sub_mode_t sm;
  sm.kind = ZN_PUSH_MODE;
  zn_sub_p_result_t r = zn_declare_subscriber(z, selector, &sm, data_handler, NULL);
  ASSERT_P_RESULT(r,"Unable to declare subscriber.\n");
  zn_sub_t *sub = r.value.sub;

  char c = 0;
  while (c != 'q') {
    c = fgetc(stdin);
  }

  zn_undeclare_subscriber(sub);
  zn_close(z);
  zn_stop_recv_loop(z);
  return 0;
}
