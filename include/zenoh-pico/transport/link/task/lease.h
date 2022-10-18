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

#ifndef ZENOH_PICO_TRANSPORT_LINK_TASK_LEASE_H
#define ZENOH_PICO_TRANSPORT_LINK_TASK_LEASE_H

#include "zenoh-pico/transport/transport.h"

int8_t _z_send_keep_alive(_z_transport_t *zt);
int8_t _zp_unicast_send_keep_alive(_z_transport_unicast_t *ztu);
int8_t _zp_multicast_send_keep_alive(_z_transport_multicast_t *ztm);

void *_zp_lease_task(void *zt_arg);             // The argument is void* to avoid incompatible pointer types in tasks
void *_zp_unicast_lease_task(void *ztu_arg);    // The argument is void* to avoid incompatible pointer types in tasks
void *_zp_multicast_lease_task(void *ztm_arg);  // The argument is void* to avoid incompatible pointer types in tasks

#endif /* ZENOH_PICO_TRANSPORT_LINK_TASK_LEASE_H */
