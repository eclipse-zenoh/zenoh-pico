
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
#include "zenoh-pico/net.h"

void fprintpid(FILE *stream, z_bytes_t pid)
{
    if (pid.val == NULL)
    {
        fprintf(stream, "None");
    }
    else
    {
        fprintf(stream, "Some(");
        for (unsigned int i = 0; i < pid.len; i++)
        {
            fprintf(stream, "%02X", pid.val[i]);
        }
        fprintf(stream, ")");
    }
}

void fprintwhatami(FILE *stream, unsigned int whatami)
{
    if (whatami == ZN_ROUTER)
    {
        fprintf(stream, "\"Router\"");
    }
    else if (whatami == ZN_PEER)
    {
        fprintf(stream, "\"Peer\"");
    }
    else
    {
        fprintf(stream, "\"Other\"");
    }
}

void fprintlocators(FILE *stream, z_str_array_t locs)
{
    fprintf(stream, "[");
    for (unsigned int i = 0; i < locs.len; i++)
    {
        fprintf(stream, "\"");
        fprintf(stream, "%s", locs.val[i]);
        fprintf(stream, "\"");
        if (i < locs.len - 1)
        {
            fprintf(stream, ", ");
        }
    }
    fprintf(stream, "]");
}

void fprinthello(FILE *stream, zn_hello_t hello)
{
    fprintf(stream, "Hello { pid: ");
    fprintpid(stream, hello.pid);
    fprintf(stream, ", whatami: ");
    fprintwhatami(stream, hello.whatami);
    fprintf(stream, ", locators: ");
    fprintlocators(stream, hello.locators);
    fprintf(stream, " }");
}

int main(void)
{
    zn_properties_t *config = zn_config_default();

    printf("Scouting...\n");
    zn_hello_array_t hellos = zn_scout(ZN_ROUTER, config, 1000);
    if (hellos.len > 0)
    {
        for (size_t i = 0; i < hellos.len; ++i)
        {
            fprinthello(stdout, hellos.val[i]);
            fprintf(stdout, "\n");
        }

        zn_hello_array_free(hellos);
    }
    else
    {
        printf("Did not find any zenoh process.\n");
    }

    return 0;
}
