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

#include "zenoh-pico/net/subscribe.h"

_z_subinfo_t _z_subinfo_push_default()
{
    _z_subinfo_t si;
    si.reliability = Z_RELIABILITY_RELIABLE;
    si.mode = Z_SUBMODE_PUSH;
    si.period = (_z_period_t){.origin = 0, .period = 0, .duration = 0};
    return si;
}

_z_subinfo_t _z_subinfo_pull_default()
{
    _z_subinfo_t si;
    si.reliability = Z_RELIABILITY_RELIABLE;
    si.mode = Z_SUBMODE_PULL;
    si.period = (_z_period_t){.origin = 0, .period = 0, .duration = 0};
    return si;
}
