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
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <zenoh-pico.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("USAGE: ./z_keyexpr_canonizer <keyexpr_1> [keyexpr_2 .. keyexpr_N]\n");
        printf("  Arguments:\n");
        printf("    - Pass any number of key expressions as arguments to obtain their canon forms");
        return -1;
    }

    char *buffer = NULL;
    for (int i = 1; i < argc; i++) {
        size_t len = strlen(argv[i]);
        buffer = realloc(buffer, len + 1);
        strncpy(buffer, argv[i], len);
        buffer[len] = '\0';
        zp_keyexpr_canon_status_t status = zp_keyexpr_canonize_null_terminated(buffer);

        switch (status) {
            case Z_KEYEXPR_CANON_SUCCESS:
                printf("canon(%s) => %s\r\n", argv[i], buffer);
                break;

            case Z_KEYEXPR_CANON_EMPTY_CHUNK:
                printf(
                    "canon(%s) => Couldn't canonize `%s` because empty chunks are forbidden (as well as leading and "
                    "trailing slashes)\r\n",
                    argv[i], argv[i]);
                break;

            case Z_KEYEXPR_CANON_STARS_IN_CHUNK:
                printf(
                    "canon(%s) => Couldn't canonize `%s` because `*` is only legal when surrounded by `/` or preceded "
                    "by `$`\r\n",
                    argv[i], argv[i]);
                break;

            case Z_KEYEXPR_CANON_DOLLAR_AFTER_DOLLAR_OR_STAR:
                printf("Ccanon(%s) => ouldn't canonize `%s` because `*$` and `$$` are illegal patterns\r\n", argv[i],
                       argv[i]);
                break;

            case Z_KEYEXPR_CANON_CONTAINS_SHARP_OR_QMARK:
                printf("canon(%s) => Couldn't canonize `%s` because `#` and `?` are illegal characters\r\n", argv[i],
                       argv[i]);
                break;

            case Z_KEYEXPR_CANON_CONTAINS_UNBOUND_DOLLAR:
                printf("canon(%s) => Couldn't canonize `%s` because `$` may only be followed by `*`\r\n", argv[i],
                       argv[i]);
                break;

            default:
                assert(false);
                break;
        }
    }

    return 0;
}
