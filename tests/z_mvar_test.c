//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "zenoh-pico/system/collections.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/collections/string.h"

#define RUN 1000000
#define TIMEOUT 60

char msg[256];

volatile unsigned int produced = 0;
void *produce(void *m)
{
    z_mvar_t *mv = (z_mvar_t *)m;
    for (produced = 0; produced < RUN; produced++)
    {
        // z_sleep_us(250000);
        printf(">> Producing (%u/%u)\n", produced + 1, RUN);
        sprintf(msg, "My message #%d", produced);
        z_mvar_put(mv, msg);
        printf(">> Produced (%u/%u): %s\n", produced + 1, RUN, msg);
    }
    return 0;
}

volatile int consumed = 0;
void *consume(void *m)
{
    z_mvar_t *mv = (z_mvar_t *)m;
    for (consumed = 0; consumed < RUN; consumed++)
    {
        printf("<< Consuming (%u/%u)\n", consumed + 1, RUN);
        // z_sleep_us(250000);
        z_str_t m = (z_str_t)z_mvar_get(mv);
        printf("<< Consumed (%u:%u): %s\n", consumed + 1, RUN, m);
    }
    return 0;
}

int main(void)
{
    z_mutex_t m1;
    z_mutex_init(&m1);

    z_mutex_t m2;
    z_mutex_init(&m2);

    z_mvar_t *mv = z_mvar_empty();

    z_task_t producer;
    z_task_t consumer;
    z_task_init(&producer, NULL, produce, mv);
    z_task_init(&consumer, NULL, consume, mv);

    // Wait to receive all the data
    z_clock_t now = z_clock_now();
    while (produced < RUN && consumed < RUN)
    {
        assert(z_clock_elapsed_s(&now) < TIMEOUT);
        (void)(now);
        z_sleep_s(1);
    }

    return 0;
}
