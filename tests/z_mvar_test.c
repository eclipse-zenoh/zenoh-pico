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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "zenoh-pico/private/collection.h"
#include "zenoh-pico/types.h"

#define RUN 1000000
#define TIMEOUT 60

char msg[256];

volatile unsigned int produced = 0;
void *produce(void *m)
{
    _z_mvar_t *mv = (_z_mvar_t *)m;
    for (produced = 0; produced < RUN; produced++)
    {
        // _z_sleep_us(250000);
        printf(">> Producing (%u/%u)\n", produced + 1, RUN);
        sprintf(msg, "My message #%d", produced);
        _z_mvar_put(mv, msg);
        printf(">> Produced (%u/%u): %s\n", produced + 1, RUN, msg);
    }
    return 0;
}

volatile int consumed = 0;
void *consume(void *m)
{
    _z_mvar_t *mv = (_z_mvar_t *)m;
    for (consumed = 0; consumed < RUN; consumed++)
    {
        printf("<< Consuming (%u/%u)\n", consumed + 1, RUN);
        // _z_sleep_us(250000);
        char *m = (char *)_z_mvar_get(mv);
        printf("<< Consumed (%u:%u): %s\n", consumed + 1, RUN, m);
    }
    return 0;
}

int main(void)
{
    _z_mutex_t m1;
    _z_mutex_init(&m1);

    _z_mutex_t m2;
    _z_mutex_init(&m2);

    _z_mvar_t *mv = _z_mvar_empty();

    _z_task_t producer;
    _z_task_t consumer;
    _z_task_init(&producer, NULL, produce, mv);
    _z_task_init(&consumer, NULL, consume, mv);

    // Wait to receive all the data
    _z_clock_t now = _z_clock_now();
    while (produced < RUN && consumed < RUN)
    {
        assert(_z_clock_elapsed_s(&now) < TIMEOUT);
        _z_sleep_s(1);
    }

    return 0;
}
