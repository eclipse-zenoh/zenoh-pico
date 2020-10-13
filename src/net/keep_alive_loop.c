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

#include <unistd.h>
#include "zenoh/net/session.h"
#include "zenoh/net/types.h"

void *zn_keep_alive_loop(zn_session_t *z)
{
    while (z->running)
    {
        z->transmitted = 0;

        // @TODO: Need to abstract usleep for multiple platforms
        // The ZENOH_NET_KEEP_ALIVE_INTERVAL is expressed in milliseconds
        usleep(1000 * ZENOH_NET_KEEP_ALIVE_INTERVAL);

        if (z->transmitted == 0)
            zn_send_keep_alive(z);
    }

    return 0;
}
