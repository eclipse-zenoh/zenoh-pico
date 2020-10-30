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
    char *path = "/demo/example/write";
    char *locator = NULL;
    char *value = "Write from Zenoh-pico!";

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

    // Build the resource key
    zn_reskey_t rk = zn_rname(path);
    zn_write(z, &rk, (const unsigned char *)value, strlen(value));

    // Close the session
    zn_close(z);

    return 0;
}
