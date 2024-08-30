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

#ifndef ZENOH_PICO_MULTICAST_READ_H
#define ZENOH_PICO_MULTICAST_READ_H

#include "zenoh-pico/transport/transport.h"

int8_t _zp_multicast_read(_z_transport_multicast_t *ztm);
int8_t _zp_multicast_stop_read_task(_z_transport_t *zt);
void *_zp_multicast_read_task(void *ztm_arg);  // The argument is void* to avoid incompatible pointer types in tasks

#if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_MULTICAST_TRANSPORT == 1
int8_t _zp_multicast_start_read_task(_z_transport_t *zt, z_task_attr_t *attr, _z_task_t *task);
#else
int8_t _zp_multicast_start_read_task(_z_transport_t *zt, void *attr, void *task);
#endif /* #if Z_FEATURE_MULTI_THREAD == 1 && Z_FEATURE_MULTICAST_TRANSPORT == 1 */

#endif /* ZENOH_PICO_MULTICAST_READ_H */
