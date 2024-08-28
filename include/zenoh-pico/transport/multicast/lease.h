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

#ifndef ZENOH_PICO_MULTICAST_LEASE_H
#define ZENOH_PICO_MULTICAST_LEASE_H

#include "zenoh-pico/transport/transport.h"

int8_t _zp_multicast_send_join(_z_transport_multicast_t *ztm);
int8_t _zp_multicast_send_keep_alive(_z_transport_multicast_t *ztm);
int8_t _zp_multicast_stop_lease_task(_z_transport_multicast_t *ztm);
void *_zp_multicast_lease_task(void *ztm_arg);  // The argument is void* to avoid incompatible pointer types in tasks

#if Z_FEATURE_MULTI_THREAD == 1 && (Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1)
int8_t _zp_multicast_start_lease_task(_z_transport_multicast_t *ztm, z_task_attr_t *attr, _z_task_t *task);
#else
int8_t _zp_multicast_start_lease_task(_z_transport_multicast_t *ztm, void *attr, void *task);
#endif /* Z_FEATURE_MULTI_THREAD == 1 &&  (Z_FEATURE_MULTICAST_TRANSPORT == 1 || Z_FEATURE_RAWETH_TRANSPORT == 1) */

#endif /* ZENOH_PICO_MULTICAST_LEASE_H */
