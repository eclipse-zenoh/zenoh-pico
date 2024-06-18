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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zenoh-pico.h>

void fprintzid(FILE *stream, z_id_t zid) {
    unsigned int zidlen = _z_id_len(zid);
    if (zidlen == 0) {
        fprintf(stream, "None");
    } else {
        fprintf(stream, "Some(");
        for (unsigned int i = 0; i < zidlen; i++) {
            fprintf(stream, "%02X", (int)zid.id[i]);
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

void fprintlocators(FILE *stream, const z_loaned_string_array_t *locs) {
    fprintf(stream, "[");
    for (unsigned int i = 0; i < z_string_array_len(locs); i++) {
        fprintf(stream, "\"");
        const z_loaned_string_t *str = z_string_array_get(locs, i);
        fprintf(stream, "%.*s", (int)str->len, str->val);
        fprintf(stream, "\"");
        if (i < z_string_array_len(locs) - 1) {
            fprintf(stream, ", ");
        }
    }
    fprintf(stream, "]");
}

void fprinthello(FILE *stream, const z_loaned_hello_t *hello) {
    fprintf(stream, "Hello { zid: ");
    fprintzid(stream, hello->zid);
    fprintf(stream, ", whatami: ");
    fprintwhatami(stream, hello->whatami);
    fprintf(stream, ", locators: ");
    fprintlocators(stream, &hello->locators);
    fprintf(stream, " }");
}

void callback(const z_loaned_hello_t *hello, void *context) {
    fprinthello(stdout, hello);
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

int main(int argc, char **argv) {
    (void)(argc);
    (void)(argv);

    int *context = (int *)malloc(sizeof(int));
    *context = 0;
    z_owned_scouting_config_t config;
    z_scouting_config_default(&config);
    z_owned_closure_hello_t closure;
    z_closure_hello(&closure, callback, drop, context);
    printf("Scouting...\n");
    z_scout(z_scouting_config_move(&config), z_closure_hello_move(&closure));
    sleep(1);
    return 0;
}
