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

#ifndef ZENOH_PICO_TRANSPORT_LINK_TASK_READ_H
#define ZENOH_PICO_TRANSPORT_LINK_TASK_READ_H

#include "zenoh-pico/transport/transport.h"

int _znp_read(_zn_transport_t *zt);
int _znp_unicast_read(_zn_transport_unicast_t *ztu);
int _znp_multicast_read(_zn_transport_multicast_t *ztm);

void *_znp_read_task(void *arg);
void *_znp_unicast_read_task(void *arg);
void *_znp_multicast_read_task(void *arg);

#endif /* ZENOH_PICO_TRANSPORT_LINK_TASK_READ_H */