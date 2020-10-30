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

int main(int argc, char **argv)
{
    char *path = "/demo/example/**";
    char *locator = NULL;
    char *value = "Pub from Zenoh-pico!";

    if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
    {
        printf("USAGE:\n\tzn_pub [<path>=%s] [<locator>=auto] [<value>='%s']\n\n", path, value);
        return 0;
    }
    if (argc > 1)
    {
        path = argv[1];
    }
    if (argc > 2)
    {
        if (strcmp(argv[2], "auto") != 0)
            locator = argv[2];
    }
    if (argc > 3)
    {
        value = argv[3];
    }

    // Open a session
    zn_session_p_result_t rz = zn_open(locator, 0, 0);
    ASSERT_P_RESULT(rz, "Unable to open a session.\n");
    zn_session_t *z = rz.value.session;
    zn_start_recv_loop(z);
    zn_start_lease_loop(z);

    // Build the resource key
    zn_reskey_t rk = zn_rname(path);

    // Declare a resource
    zn_res_p_result_t rr = zn_declare_resource(z, &rk);
    ASSERT_P_RESULT(rr, "Unable to declare resource.\n");
    zn_resource_t *res = rr.value.res;

    // Declare a publisher
    zn_pub_p_result_t rp = zn_declare_publisher(z, &res->key);
    ASSERT_P_RESULT(rp, "Unable to declare publisher.\n");

    // Create random data
    char data[MAX_LEN];

    // Loop endessly and write data
    zn_reskey_t ri = zn_rid(res);
    unsigned int count = 0;
    while (1)
    {
        sleep(1);
        snprintf(data, MAX_LEN, "[%4d] %s", count, value);
        printf("Writing Data ('%zu': '%s')...\n", ri.rid, data);
        zn_write(z, &ri, (const unsigned char *)data, strlen(data));
        count++;
    }

    return 0;
}
