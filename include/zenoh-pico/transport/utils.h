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

#ifndef ZENOH_PICO_TRANSPORT_UTILS_H
#define ZENOH_PICO_TRANSPORT_UTILS_H

#include <stdbool.h>

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/transport.h"

/*------------------ SN helpers ------------------*/
_z_zint_t _z_sn_max(uint8_t bits);
_z_zint_t _z_sn_half(_z_zint_t sn);
_z_zint_t _z_sn_modulo_mask(uint8_t bits);
_Bool _z_sn_precedes(const _z_zint_t sn_resolution, const _z_zint_t sn_left, const _z_zint_t sn_right);
_z_zint_t _z_sn_increment(const _z_zint_t sn_resolution, const _z_zint_t sn);
_z_zint_t _z_sn_decrement(const _z_zint_t sn_resolution, const _z_zint_t sn);

void _z_conduit_sn_list_copy(_z_conduit_sn_list_t *dst, const _z_conduit_sn_list_t *src);
void _z_conduit_sn_list_decrement(const _z_zint_t sn_resolution, _z_conduit_sn_list_t *sns);

#endif /* ZENOH_PICO_TRANSPORT_UTILS_H */
