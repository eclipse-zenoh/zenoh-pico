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

#include "zenoh-pico/protocol/extcodec.h"
#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Transport Extensions ------------------*/
int _zn_transport_extension_encode(_z_wbuf_t *wbf, const _zn_transport_extension_t *ext)
{
    // Encode the header
    _ZN_EC(_z_wbuf_write(wbf, ext->header))

    // Encode the body
    uint8_t eid = _ZN_EID(ext->header);
    switch (eid)
    {
    default:
        _Z_ERROR("WARNING: Trying to encode message with unknown ID(%d)\n", eid);
        return -1;
    }
}

void _zn_transport_extension_decode_na(_z_zbuf_t *zbf, _zn_transport_extension_result_t *r)
{
    r->tag = _z_res_t_OK;

    do
    {
        _z_uint8_result_t r_uint8 = _z_uint8_decode(zbf);
        _ASSURE_P_RESULT(r_uint8, r, _zn_err_t_PARSE_TRANSPORT_EXTENSION)
        r->value.transport_extension.header = r_uint8.value.uint8;

        uint8_t eid = _ZN_EID(r->value.transport_extension.header);
        switch (eid)
        {
        default:
        {
            _Z_ERROR("WARNING: Trying to decode zenoh message with unknown ID(%d)\n", eid);
            r->tag = _z_res_t_ERR;
            r->value.error = _zn_err_t_PARSE_TRANSPORT_EXTENSION;
            return;
        }
        }
    } while (1);
}

_zn_transport_extension_result_t _zn_transport_extension_decode(_z_zbuf_t *zbf)
{
    _zn_transport_extension_result_t r;
    _zn_transport_extension_decode_na(zbf, &r);
    return r;
}
