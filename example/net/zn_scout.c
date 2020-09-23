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

int main(int argc, char* argv[]) {    
  if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
    printf("USAGE:\n\tzn_scout\n\n");
    return 0;
  }

  printf("Scouting...\n");
  z_vec_t locs = zn_scout("auto", 10, 500000);  
  if (z_vec_length(&locs) > 0) {
    for (unsigned int i = 0; i < z_vec_length(&locs); ++i) {
      printf("Locator: %s\n", (char*)z_vec_get(&locs, i));
    }
  } else {
    printf("Did not find any zenoh router.\n");
  }
  return 0;
}
