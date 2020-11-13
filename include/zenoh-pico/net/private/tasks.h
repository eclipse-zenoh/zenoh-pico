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

#ifndef _ZENOH_NET_PICO_TASKS_H
#define _ZENOH_NET_PICO_TASKS_H

#include "zenoh-pico/net/types.h"

void *_znp_lease_task(zn_session_t *z);
void *_znp_read_task(zn_session_t *z);

#endif /* _ZENOH_NET_PICO_TASKS_H */
