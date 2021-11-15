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

#include "zenoh-pico/link/config/tcp.h"
#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/utils/properties.h"

_zn_state_result_t _zn_tcp_config_from_str(const z_str_t s)
{
    _zn_state_result_t res;

    if (s != NULL)
        goto ERR;

    res.tag = _z_res_t_OK;
    res.value.state = _zn_state_make();

    return res;

ERR:
    res.tag = _z_res_t_ERR;
    res.value.error = _z_err_t_PARSE_STRING;
    return res;
}