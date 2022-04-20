
/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
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

#include "zenoh-pico.h"

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
            fprintf(stream, "%02X", (int)pid.val[i]);
        }
        fprintf(stream, ")");
    }
}

void fprintwhatami(FILE *stream, unsigned int whatami)
{
    if (whatami == Z_ROUTER)
    {
        fprintf(stream, "\"Router\"");
    }
    else if (whatami == Z_PEER)
    {
        fprintf(stream, "\"Peer\"");
    }
    else
    {
        fprintf(stream, "\"Other\"");
    }
}

void fprintlocators(FILE *stream, const z_str_array_t *locs)
{
    fprintf(stream, "[");
    for (unsigned int i = 0; i < locs->len; i++)
    {
        fprintf(stream, "\"");
        fprintf(stream, "%s", locs->val[i]);
        fprintf(stream, "\"");
        if (i < locs->len - 1)
        {
            fprintf(stream, ", ");
        }
    }
    fprintf(stream, "]");
}

void fprinthello(FILE *stream, const z_hello_t *hello)
{
    fprintf(stream, "Hello { pid: ");
    fprintpid(stream, hello->pid);
    fprintf(stream, ", whatami: ");
    fprintwhatami(stream, hello->whatami);
    fprintf(stream, ", locators: ");
    fprintlocators(stream, &hello->locators);
    fprintf(stream, " }");
}

int main(int argc, char **argv)
{
    (void) (argc);
    (void) (argv);

    z_init_logger();

    z_owned_config_t config = z_config_default();

    printf("Scouting...\n");
    z_owned_hello_array_t hellos = z_scout(Z_ROUTER | Z_PEER, z_move(&config), 1000);
    if (z_loan(&hellos)->len > 0)
    {
        for (unsigned int i = 0; i < z_loan(&hellos)->len; ++i)
        {
            fprinthello(stdout, &(z_loan(&hellos)->val[i]));
            fprintf(stdout, "\n");
        }
    }
    else
    {
        printf("Did not find any zenoh process.\n");
    }
    z_hello_array_clear(z_move(&hellos));
    return 0;
}
