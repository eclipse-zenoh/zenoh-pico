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

#ifndef ZENOH_PICO_TRANSPORT_READ_H
#define ZENOH_PICO_TRANSPORT_READ_H

#include "zenoh-pico/transport/transport.h"

int8_t _z_read(_z_transport_t *zt);
void *_zp_read_task(void *zt_arg);  // The argument is void* to avoid incompatible pointer types in tasks

#endif /* ZENOH_PICO_TRANSPORT_READ_H */
