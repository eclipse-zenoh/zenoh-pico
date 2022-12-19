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

#include <stdio.h>
#include <zenoh-pico.h>

void fprintzid(FILE *stream, z_bytes_t zid) {
    if (zid.start == NULL) {
        fprintf(stream, "None");
    } else {
        fprintf(stream, "Some(");
        for (unsigned int i = 0; i < zid.len; i++) {
            fprintf(stream, "%02X", (int)zid.start[i]);
        }
        fprintf(stream, ")");
    }
}

void fprintwhatami(FILE *stream, unsigned int whatami) {
    if (whatami == Z_WHATAMI_ROUTER) {
        fprintf(stream, "\"Router\"");
    } else if (whatami == Z_WHATAMI_PEER) {
        fprintf(stream, "\"Peer\"");
    } else {
        fprintf(stream, "\"Other\"");
    }
}

void fprintlocators(FILE *stream, const z_str_array_t *locs) {
    fprintf(stream, "[");
    for (unsigned int i = 0; i < z_str_array_len(locs); i++) {
        fprintf(stream, "\"");
        fprintf(stream, "%s", *z_str_array_get(locs, i));
        fprintf(stream, "\"");
        if (i < z_str_array_len(locs) - 1) {
            fprintf(stream, ", ");
        }
    }
    fprintf(stream, "]");
}

void fprinthello(FILE *stream, const z_hello_t hello) {
    fprintf(stream, "Hello { zid: ");
    fprintzid(stream, hello.zid);
    fprintf(stream, ", whatami: ");
    fprintwhatami(stream, hello.whatami);
    fprintf(stream, ", locators: ");
    fprintlocators(stream, &hello.locators);
    fprintf(stream, " }");
}

void callback(z_owned_hello_t *hello, void *context) {
    fprinthello(stdout, z_hello_loan(hello));
    fprintf(stdout, "\n");
    (*(int *)context)++;
}

void drop(void *context) {
    int count = *(int *)context;
    free(context);
    if (!count) {
        printf("Did not find any zenoh process.\n");
    } else {
        printf("Dropping scout results.\n");
    }
}

int main(void) {
    sleep(5);

    int *context = (int *)malloc(sizeof(int));
    *context = 0;
    z_owned_scouting_config_t config = z_scouting_config_default();
    z_owned_closure_hello_t closure = z_closure_hello(callback, drop, context);
    printf("Scouting...\n");
    z_scout(z_scouting_config_move(&config), z_closure_hello_move(&closure));

    return 0;
}
