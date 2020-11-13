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
#include <sys/time.h>
#include "zenoh-pico/net.h"

#define N 100000

volatile unsigned long long int count = 0;
volatile struct timeval start;
volatile struct timeval stop;

void print_stats(volatile struct timeval *start, volatile struct timeval *stop)
{
    double t0 = start->tv_sec + ((double)start->tv_usec / 1000000.0);
    double t1 = stop->tv_sec + ((double)stop->tv_usec / 1000000.0);
    double thpt = N / (t1 - t0);
    printf("%f msgs/sec\n", thpt);
}

void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(sample); // Unused argument
    (void)(arg);    // Unused argument

    struct timeval tv;
    if (count == 0)
    {
        gettimeofday(&tv, 0);
        start = tv;
        count++;
    }
    else if (count < N)
    {
        count++;
    }
    else
    {
        gettimeofday(&tv, 0);
        stop = tv;
        print_stats(&start, &stop);
        count = 0;
    }
}

int main(int argc, char **argv)
{
    zn_properties_t *config = zn_config_default();
    if (argc > 1)
    {
        zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(argv[1]));
    }

    printf("Openning session...\n");
    zn_session_t *s = zn_open(config);
    if (s == 0)
    {
        printf("Unable to open session!\n");
        exit(-1);
    }

    // Start the read session session lease loops
    znp_start_read_task(s);
    znp_start_lease_task(s);

    zn_reskey_t rid = zn_rid(zn_declare_resource(s, zn_rname("/test/thr")));
    zn_subscriber_t *sub = zn_declare_subscriber(s, rid, zn_subinfo_default(), data_handler, NULL);
    if (sub == 0)
    {
        printf("Unable to declare subscriber.\n");
        exit(-1);
    }

    char c = 0;
    while (c != 'q')
    {
        c = fgetc(stdin);
    }

    zn_undeclare_subscriber(sub);
    zn_close(s);

    return 0;
}
