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

char *hexdump(z_uint8_array_t array) {
  char *res = malloc((array.length*2+1)*sizeof(char));
  for(unsigned int i = 0; i < array.length; ++i){
    sprintf(res + 2*i, "%02x", array.elem[i]);
  }
  res[array.length*2+1] = 0;
  return res;
}

int main(int argc, char **argv) {
  char *locator = 0;

  if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
    printf("USAGE:\n\tzn_info [<locator>=auto]\n\n");
    return 0;
  }

  if (argc > 1) {
    locator = argv[1];
  }

  z_vec_t ps = z_vec_make(2);
  z_vec_append(&ps, zn_property_make_from_str(ZN_USER_KEY, "user"));
  z_vec_append(&ps, zn_property_make_from_str(ZN_PASSWD_KEY, "password"));

  printf("Openning session...\n");
  zn_session_p_result_t r_z = zn_open(locator, 0, &ps);
  ASSERT_RESULT(r_z, "Unable to open session.\n")
  zn_session_t *z = r_z.value.session;

  z_vec_t info = zn_info(z);
  printf("LOCATOR :  %s\n", ((zn_property_t *)z_vec_get(&info, ZN_INFO_PEER_KEY))->value.elem);
  printf("PID :      %s\n", hexdump(((zn_property_t *)z_vec_get(&info, ZN_INFO_PID_KEY))->value));
  printf("PEER PID : %s\n", hexdump(((zn_property_t *)z_vec_get(&info, ZN_INFO_PEER_PID_KEY))->value));

  z_vec_free(&info);

  zn_close(z);
  return 0;
}
