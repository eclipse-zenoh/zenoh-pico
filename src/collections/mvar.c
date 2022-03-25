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

#include <string.h>
#include "zenoh-pico/system/collections.h"

/*-------- mvar --------*/
_z_mvar_t *_z_mvar_empty()
{
    _z_mvar_t *mv = (_z_mvar_t *)malloc(sizeof(_z_mvar_t));
    memset(mv, 0, sizeof(_z_mvar_t));
    _z_mutex_init(&mv->mtx);
    _z_condvar_init(&mv->can_put);
    _z_condvar_init(&mv->can_get);
    return mv;
}

int _z_mvar_is_empty(_z_mvar_t *mv)
{
    return mv->full == 0;
}

_z_mvar_t *_z_mvar_of(void *e)
{
    _z_mvar_t *mv = _z_mvar_empty();
    mv->elem = e;
    mv->full = 1;
    return mv;
}

void *_z_mvar_get(_z_mvar_t *mv)
{
    int lock = 1;
    do
    {
        if (lock == 1)
            _z_mutex_lock(&mv->mtx);

        if (mv->full)
        {
            mv->full = 0;
            void *e = mv->elem;
            mv->elem = 0;
            _z_mutex_unlock(&mv->mtx);
            _z_condvar_signal(&mv->can_put);
            return e;
        }
        else
        {
            _z_condvar_wait(&mv->can_get, &mv->mtx);
            lock = 0;
        }
    } while (1);
}

void _z_mvar_put(_z_mvar_t *mv, void *e)
{
    int lock = 1;
    do
    {
        if (lock == 1)
            _z_mutex_lock(&mv->mtx);

        if (mv->full)
        {
            _z_condvar_wait(&mv->can_put, &mv->mtx);
            lock = 0;
        }
        else
        {
            mv->elem = e;
            mv->full = 1;
            _z_condvar_signal(&mv->can_get);
            _z_mutex_unlock(&mv->mtx);
            return;
        }
    } while (1);
}
