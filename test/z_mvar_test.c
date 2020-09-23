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

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "zenoh/types.h"
#include "zenoh/mvar.h"


char msg[256];

void *sleep_and_fill(void *m) {

  z_mvar_t *mv = (z_mvar_t*)m;
  for (int i = 0; i < 10; ++i) {
    usleep(250000);
    printf("Producing round #%d\n", i);
    sprintf(msg, "Goodie #%d", i);
    z_mvar_put(mv, msg);
  }
  return 0;
}

void *sleep_and_consume(void *m) {
  z_mvar_t *mv = (z_mvar_t*)m;
  for (int i = 0; i < 10; ++i) {
    printf("Consuming round #%d\n", i);
    usleep(250000);
    char *msg = (char *)z_mvar_get(mv);
    printf("consumed: MVar content: %s\n", msg);
  }
  return 0;
}
int main(int argc, char **argv) {
  Z_UNUSED_ARG_2(argc, argv);
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  z_mvar_t *mv = z_mvar_empty();
  pthread_t producer;
  pthread_t consumer;
  pthread_create(&producer, 0, sleep_and_fill, mv);
  pthread_create(&consumer, 0, sleep_and_consume, mv);
  pthread_join(consumer, NULL);
  pthread_join(producer, NULL);


  return 0;
}

