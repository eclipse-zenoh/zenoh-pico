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

#include "zenoh-pico/transport/utils.h"

int _zn_sn_precedes(const z_zint_t sn_resolution_half, const z_zint_t sn_left, const z_zint_t sn_right)
{
    if (sn_right > sn_left)
        return (sn_right - sn_left <= sn_resolution_half);
    else
        return (sn_left - sn_right > sn_resolution_half);
}

z_zint_t _zn_sn_increment(const z_zint_t sn_resolution, const z_zint_t sn)
{
    return (sn + 1) % sn_resolution;
}

z_zint_t _zn_sn_decrement(const z_zint_t sn_resolution, const z_zint_t sn)
{
    if (sn == 0)
        return sn_resolution - 1;
    else
        return sn - 1;
}