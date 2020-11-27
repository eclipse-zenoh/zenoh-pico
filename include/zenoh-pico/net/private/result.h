/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#ifndef _ZENOH_NET_PICO_RESULT_H
#define _ZENOH_NET_PICO_RESULT_H

#include "zenoh-pico/net/types.h"
#include "zenoh-pico/private/result.h"

/*------------------ Internal Zenoh-net Errors ------------------*/
typedef enum
{
    _zn_err_t_IO_GENERIC,
    _zn_err_t_IOBUF_NO_SPACE,
    _zn_err_t_INVALID_LOCATOR,
    _zn_err_t_OPEN_SESSION_FAILED,
    _zn_err_t_PARSE_CONSOLIDATION,
    _zn_err_t_PARSE_DECLARATION,
    _zn_err_t_PARSE_PAYLOAD,
    _zn_err_t_PARSE_PERIOD,
    _zn_err_t_PARSE_PROPERTIES,
    _zn_err_t_PARSE_PROPERTY,
    _zn_err_t_PARSE_RESKEY,
    _zn_err_t_PARSE_SESSION_MESSAGE,
    _zn_err_t_PARSE_SUBMODE,
    _zn_err_t_PARSE_TIMESTAMP,
    _zn_err_t_PARSE_ZENOH_MESSAGE,
    _zn_err_t_RESOURCE_DECLARATION,
    _zn_err_t_TX_CONNECTION,
    _zn_err_t_UNEXPECTED_MESSAGE
} _zn_err_t;

/*------------------ Internal Zenoh-net Results ------------------*/
#define _ZN_RESULT_DECLARE(type, name) _RESULT_DECLARE(type, name, _zn)
#define _ZN_P_RESULT_DECLARE(type, name) _P_RESULT_DECLARE(type, name, _zn)

#endif /* _ZENOH_NET_PICO_RESULT_H */
