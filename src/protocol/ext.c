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

#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Transport Message ------------------*/
void _zn_t_ext_copy(_zn_transport_extension_t *clone, _zn_transport_extension_t *ext)
{
    clone->header = ext->header;

    uint8_t eid = _ZN_EID(ext->header);
    switch (eid)
    {
    default:
        _Z_ERROR("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
        break;
    }
}

void _zn_t_ext_clear(_zn_transport_extension_t *ext)
{
    uint8_t eid = _ZN_EID(ext->header);
    switch (eid)
    {
    default:
        _Z_ERROR("WARNING: Trying to free session message with unknown ID(%d)\n", mid);
        return;
    }
}

void _zn_t_ext_skip(_z_zbuf_t *zbf)
{
    uint8_t next_ext = 0;
    do
    {
        _z_uint8_result_t _void;

        // Decode flags and ID
        _z_uint8_result_t r_uint8 = _z_uint8_decode(zbf);
        _ASSURE_P_RESULT(r_uint8, (&_void), _z_err_t_PARSE_UINT8)
        next_ext = _ZN_HAS_EFLAG(r_uint8.value.uint8, _ZN_FLAG_EXT_Z);

        // Decode len
        _z_zint_result_t r_zint = _z_zint_decode(zbf);
        _ASSURE_P_RESULT(r_zint, (&_void), _z_err_t_PARSE_ZINT)

        _z_zbuf_set_rpos(zbf, _z_zbuf_get_rpos(zbf) + r_zint.value.zint);
    } while (next_ext);
}