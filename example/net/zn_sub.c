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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "zenoh.h"

#define MAX_LEN 256

void data_handler(const zn_reskey_t *rkey, const unsigned char *data, size_t length, const zn_data_info_t *info, void *arg)
{
    Z_UNUSED_ARG_2(info, arg);
    char str[MAX_LEN];
    memcpy(&str, data, length < MAX_LEN ? length : MAX_LEN - 1);
    str[length < MAX_LEN ? length : MAX_LEN - 1] = 0;

    printf(">> [Subscription listener] Received ('%s'): '%s')...\n", rkey->rname, str);
}

int main(int argc, char **argv)
{
    char *path = "/demo/example/**";
    char *locator = NULL;

    if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
    {
        printf("USAGE:\n\tzn_sub [<path>=%s] [<locator>=auto]\n\n", path);
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

    // Open a session
    zn_session_p_result_t rz = zn_open(locator, 0, 0);
    ASSERT_RESULT(rz, "Unable to open a session.\n")
    zn_session_t *z = rz.value.session;
    zn_start_recv_loop(z);
    zn_start_lease_loop(z);

    // Build the resource key
    zn_reskey_t rk = zn_rname(path);

    // Declare a resource
    zn_res_p_result_t rr = zn_declare_resource(z, &rk);
    ASSERT_P_RESULT(rr, "Unable to declare resource.\n");
    zn_resource_t *res = rr.value.res;

    zn_subinfo_t si;
    si.mode = ZN_PUSH_MODE;
    si.is_reliable = 1;
    si.is_periodic = 0;
    zn_sub_p_result_t r = zn_declare_subscriber(z, &res->key, &si, data_handler, NULL);
    ASSERT_P_RESULT(r, "Unable to declare subscriber.\n");
    zn_subscriber_t *sub = r.value.sub;

    char c = 0;
    while (c != 'q')
    {
        c = fgetc(stdin);
    }

    zn_undeclare_subscriber(sub);
    zn_undeclare_resource(res);
    zn_close(z);
    zn_stop_lease_loop(z);
    zn_stop_recv_loop(z);

    return 0;
}
