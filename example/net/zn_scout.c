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

// @TODO: align the example with zenoh-c

int main(int argc, char *argv[])
{
    if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
    {
        printf("USAGE:\n\tzn_scout\n\n");
        return 0;
    }

    printf("Scouting...\n");
    _z_vec_t locs = zn_scout("auto", 10, 100000);
    if (_z_vec_len(&locs) > 0)
    {
        for (size_t i = 0; i < _z_vec_len(&locs); ++i)
        {
            printf("Locator: %s\n", (char *)_z_vec_get(&locs, i));
        }
    }
    else
    {
        printf("Did not find any zenoh router.\n");
    }

    return 0;
}
