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

#include "zenoh-pico/link/manager.h"

_zn_link_manager_t *_zn_link_manager_init()
{
    _zn_link_manager_t *zlm = (_zn_link_manager_t *)malloc(sizeof(_zn_link_manager_t));

    return zlm;
}

void _zn_link_manager_free(_zn_link_manager_t **zlm)
{
    _zn_link_manager_t *ptr = *zlm;
    free(ptr);
    *zlm = NULL;
}