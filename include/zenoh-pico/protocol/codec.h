//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#ifndef ZENOH_PICO_PROTOCOL_CODEC_H
#define ZENOH_PICO_PROTOCOL_CODEC_H

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/utils/config.h"
#include "zenoh-pico/utils/result.h"

#define _Z_EC(fn)          \
    if (fn != _Z_RES_OK) { \
        return -1;         \
    }

/*------------------ Internal Zenoh-net Macros ------------------*/
_Z_RESULT_DECLARE(uint8_t, uint8)
int8_t _z_uint8_encode(_z_wbuf_t *buf, uint8_t v);
_z_uint8_result_t _z_uint8_decode(_z_zbuf_t *buf);

_Z_RESULT_DECLARE(_z_zint_t, zint)
int8_t _z_zint_encode(_z_wbuf_t *buf, _z_zint_t v);
_z_zint_result_t _z_zint_decode(_z_zbuf_t *buf);

_Z_RESULT_DECLARE(_z_bytes_t, bytes)
int8_t _z_bytes_encode(_z_wbuf_t *buf, const _z_bytes_t *bs);
_z_bytes_result_t _z_bytes_decode(_z_zbuf_t *buf);

_Z_RESULT_DECLARE(char *, str)
int8_t _z_str_encode(_z_wbuf_t *buf, const char *s);
_z_str_result_t _z_str_decode(_z_zbuf_t *buf);

_Z_RESULT_DECLARE(_z_period_t, period)
int8_t _z_period_encode(_z_wbuf_t *wbf, const _z_period_t *m);
_z_period_result_t _z_period_decode(_z_zbuf_t *zbf);
void _z_period_decode_na(_z_zbuf_t *zbf, _z_period_result_t *r);

#endif /* ZENOH_PICO_PROTOCOL_CODEC_H */
