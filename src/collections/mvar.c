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
 *     ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include "zenoh-pico/system/common.h"

/*-------- mvar --------*/
z_mvar_t *z_mvar_empty()
{
    z_mvar_t *mv = (z_mvar_t *)malloc(sizeof(z_mvar_t));
    memset(mv, 0, sizeof(z_mvar_t));
    z_mutex_init(&mv->mtx);
    z_condvar_init(&mv->can_put);
    z_condvar_init(&mv->can_get);
    return mv;
}

int z_mvar_is_empty(z_mvar_t *mv)
{
    return mv->full == 0;
}

z_mvar_t *z_mvar_of(void *e)
{
    z_mvar_t *mv = z_mvar_empty();
    mv->elem = e;
    mv->full = 1;
    return mv;
}

void *z_mvar_get(z_mvar_t *mv)
{
    int lock = 1;
    do
    {
        if (lock == 1)
            z_mutex_lock(&mv->mtx);

        if (mv->full)
        {
            mv->full = 0;
            void *e = mv->elem;
            mv->elem = 0;
            z_mutex_unlock(&mv->mtx);
            z_condvar_signal(&mv->can_put);
            return e;
        }
        else
        {
            z_condvar_wait(&mv->can_get, &mv->mtx);
            lock = 0;
        }
    } while (1);
}

void z_mvar_put(z_mvar_t *mv, void *e)
{
    int lock = 1;
    do
    {
        if (lock == 1)
            z_mutex_lock(&mv->mtx);

        if (mv->full)
        {
            z_condvar_wait(&mv->can_put, &mv->mtx);
            lock = 0;
        }
        else
        {
            mv->elem = e;
            mv->full = 1;
            z_condvar_signal(&mv->can_get);
            z_mutex_unlock(&mv->mtx);
            return;
        }
    } while (1);
}
