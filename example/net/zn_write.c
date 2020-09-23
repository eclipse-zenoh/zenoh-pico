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

int main(int argc, char **argv) {
  char *selector = "/zenoh/examples/c/write/hello";
  char *value = "Zenitude writtem from zenoh-c!";
  char *locator = 0;

  if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
    printf("USAGE:\n\tzn_stream [<selector>=%s] [<value>=%s] [<locator>=auto]\n\n", selector, value);
    return 0;
  }
  if (argc > 1) {
    selector = argv[1];
  }
  
  if (argc > 2) {
    value = argv[2];
  }
  
  if (argc > 3) {
    locator = argv[3];
  }

  printf("Openning session...\n");
  zn_session_p_result_t r_z = zn_open(locator, 0, 0);
  ASSERT_RESULT(r_z, "Unable to open v.\n")
  zn_session_t *z = r_z.value.session;

  printf("Writing Data ('%s': '%s')...\n", selector, value);
  zn_write_data(z, selector, (const unsigned char *)value, strlen(value));

  zn_close(z);
  return 0;
}
