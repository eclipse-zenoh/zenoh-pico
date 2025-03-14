//
// Copyright (c) 2025 ZettaScale Technology
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

#ifndef ZENOH_PICO_UNICAST_ACCEPT_H
#define ZENOH_PICO_UNICAST_ACCEPT_H

#include "zenoh-pico/transport/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

z_result_t _zp_unicast_start_accept_task(_z_transport_unicast_t *ztu);
void _zp_unicast_stop_accept_task(_z_transport_common_t *ztc);

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_UNICAST_ACCEPT_H */
