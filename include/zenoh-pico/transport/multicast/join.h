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

#ifndef ZENOH_MULTICAST_JOIN_H
#define ZENOH_MULTICAST_JOIN_H

#include "zenoh-pico/transport/transport.h"

int8_t _zp_multicast_send_join(_z_transport_multicast_t *ztm);

#endif /* ZENOH_MULTICAST_JOIN_H */
