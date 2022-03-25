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

int _z_sn_precedes(const _z_zint_t sn_resolution_half, const _z_zint_t sn_left, const _z_zint_t sn_right)
{
    if (sn_right > sn_left)
        return (sn_right - sn_left <= sn_resolution_half);
    else
        return (sn_left - sn_right > sn_resolution_half);
}

_z_zint_t _z_sn_increment(const _z_zint_t sn_resolution, const _z_zint_t sn)
{
    return (sn + 1) % sn_resolution;
}

_z_zint_t _z_sn_decrement(const _z_zint_t sn_resolution, const _z_zint_t sn)
{
    if (sn == 0)
        return sn_resolution - 1;
    else
        return sn - 1;
}

void _z_conduit_sn_list_copy(_z_conduit_sn_list_t *dst, const _z_conduit_sn_list_t *src)
{
    dst->is_qos = src->is_qos;
    if (dst->is_qos == 0)
    {
        dst->val.plain.best_effort = src->val.plain.best_effort;
        dst->val.plain.reliable = src->val.plain.reliable;
    }
    else
    {
        for (int i = 0; i < Z_PRIORITIES_NUM; i++)
        {
            dst->val.qos[i].best_effort = src->val.qos[i].best_effort;
            dst->val.qos[i].reliable = src->val.qos[i].reliable;
        }
    }
}

void _z_conduit_sn_list_decrement(const _z_zint_t sn_resolution, _z_conduit_sn_list_t *sns)
{
    if (sns->is_qos == 0)
    {
        sns->val.plain.best_effort = _z_sn_decrement(sn_resolution, sns->val.plain.best_effort);
        sns->val.plain.reliable = _z_sn_decrement(sn_resolution, sns->val.plain.reliable);
    }
    else
    {
        for (int i = 0; i < Z_PRIORITIES_NUM; i++)
        {
            sns->val.qos[i].best_effort = _z_sn_decrement(sn_resolution, sns->val.qos[i].best_effort);
            sns->val.qos[i].best_effort = _z_sn_decrement(sn_resolution, sns->val.qos[i].reliable);
        }
    }
}