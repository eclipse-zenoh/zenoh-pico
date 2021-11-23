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

#include "zenoh-pico/transport/link/task/lease.h"
#include "zenoh-pico/transport/link/rx.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/utils/logging.h"

int _znp_multicast_read(_zn_transport_multicast_t *ztm)
{
    // TODO: to be implemented
    return -1;
}

void *_znp_multicast_read_task(void *arg)
{
    // TODO: to be implemented

    return 0;
}