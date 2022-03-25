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

#ifndef ZENOH_PICO_SYSTEM_COLLECTIONS_H
#define ZENOH_PICO_SYSTEM_COLLECTIONS_H

#include "zenoh-pico/system/platform.h"

/*-------- Mvar --------*/
typedef struct
{
    void *elem;
    int full;
    _z_mutex_t mtx;
    _z_condvar_t can_put;
    _z_condvar_t can_get;
} _z_mvar_t;

_z_mvar_t *_z_mvar_empty(void);
int _z_mvar_is_empty(_z_mvar_t *mv);

_z_mvar_t *_z_mvar_of(void *e);
void *_z_mvar_get(_z_mvar_t *mv);
void _z_mvar_put(_z_mvar_t *mv, void *e);

#endif /* ZENOH_PICO_SYSTEM_COLLECTIONS_H */
