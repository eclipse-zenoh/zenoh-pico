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

#ifndef ZENOH_PICO_TRANSPORT_MANAGER_H
#define ZENOH_PICO_TRANSPORT_MANAGER_H

#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/transport/transport.h"

int8_t _z_new_transport(_z_transport_t *zt, _z_bytes_t *bs, char *locator, z_whatami_t mode);
void _z_free_transport(_z_transport_t **zt);

#endif /* ZENOH_PICO_TRANSPORT_MANAGER_H */
