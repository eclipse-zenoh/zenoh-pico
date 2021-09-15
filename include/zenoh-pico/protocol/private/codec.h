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

#ifndef _ZENOH_PICO_PROTOCOL_PRIVATE_CODEC_H
#define _ZENOH_PICO_PROTOCOL_PRIVATE_CODEC_H

#include "zenoh-pico/protocol/private/iobuf.h"
#include "zenoh-pico/utils/property.h"
#include "zenoh-pico/utils/private/result.h"
#include "zenoh-pico/utils/types.h"

#define _ZN_EC(fn) \
    if (fn != 0)   \
    {              \
        return -1; \
    }

/*------------------ Internal Zenoh-net Macros ------------------*/
_Z_RESULT_DECLARE(uint8_t, uint8)
int _z_uint8_encode(_z_wbuf_t *buf, uint8_t v);
_z_uint8_result_t _z_uint8_decode(_z_zbuf_t *buf);

_Z_RESULT_DECLARE(z_zint_t, zint)
int _z_zint_encode(_z_wbuf_t *buf, z_zint_t v);
_z_zint_result_t _z_zint_decode(_z_zbuf_t *buf);

_Z_RESULT_DECLARE(z_bytes_t, bytes)
int _z_bytes_encode(_z_wbuf_t *buf, const z_bytes_t *bs);
_z_bytes_result_t _z_bytes_decode(_z_zbuf_t *buf);
void _z_bytes_free(z_bytes_t *bs);

_Z_RESULT_DECLARE(z_str_t, str)
int _z_str_encode(_z_wbuf_t *buf, const z_str_t s);
_z_str_result_t _z_str_decode(_z_zbuf_t *buf);

/*------------------ Internal Zenoh-net Encoding/Decoding ------------------*/
_ZN_RESULT_DECLARE(zn_property_t, property)
int _zn_property_encode(_z_wbuf_t *wbf, const zn_property_t *m);
_zn_property_result_t _zn_property_decode(_z_zbuf_t *zbf);
void _zn_property_decode_na(_z_zbuf_t *zbf, _zn_property_result_t *r);

_ZN_RESULT_DECLARE(zn_properties_t, properties)
int _zn_properties_encode(_z_wbuf_t *wbf, const zn_properties_t *m);
_zn_properties_result_t _zn_properties_decode(_z_zbuf_t *zbf);
void _zn_properties_decode_na(_z_zbuf_t *zbf, _zn_properties_result_t *r);

_ZN_RESULT_DECLARE(zn_period_t, period)
int _zn_period_encode(_z_wbuf_t *wbf, const zn_period_t *m);
_zn_period_result_t _zn_period_decode(_z_zbuf_t *zbf);
void _zn_period_decode_na(_z_zbuf_t *zbf, _zn_period_result_t *r);

#endif /* _ZENOH_PICO_PROTOCOL_PRIVATE_CODEC_H */
