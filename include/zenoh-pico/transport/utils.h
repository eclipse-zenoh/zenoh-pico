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

#ifndef ZENOH_PICO_TRANSPORT_UTILS_H
#define ZENOH_PICO_TRANSPORT_UTILS_H

#include "zenoh-pico/session/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"

/*------------------ SN helpers ------------------*/
int _zn_sn_precedes(const z_zint_t sn_resolution_half, const z_zint_t sn_left, const z_zint_t sn_right);
z_zint_t _zn_sn_increment(const z_zint_t sn_resolution, const z_zint_t sn);
z_zint_t _zn_sn_decrement(const z_zint_t sn_resolution, const z_zint_t sn);
void _zn_conduit_sn_list_copy(_zn_conduit_sn_list_t *dst, const _zn_conduit_sn_list_t *src);
void _zn_conduit_sn_list_decrement(const z_zint_t sn_resolution, _zn_conduit_sn_list_t *sns);

#endif /* ZENOH_PICO_TRANSPORT_UTILS_H */
