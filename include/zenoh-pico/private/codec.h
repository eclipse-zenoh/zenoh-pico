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

#ifndef _ZENOH_PICO_CODEC_H
#define _ZENOH_PICO_CODEC_H

#include "zenoh-pico/types.h"
#include "zenoh-pico/private/iobuf.h"
#include "zenoh-pico/private/result.h"

#define _ZN_EC(fn) \
    if (fn != 0)   \
    {              \
        return -1; \
    }

/*------------------ Internal Zenoh-net Macros ------------------*/
_Z_RESULT_DECLARE(z_zint_t, zint)
int _z_zint_encode(_z_wbuf_t *buf, z_zint_t v);
_z_zint_result_t _z_zint_decode(_z_rbuf_t *buf);

_Z_RESULT_DECLARE(z_bytes_t, bytes)
int _z_bytes_encode(_z_wbuf_t *buf, const z_bytes_t *bs);
_z_bytes_result_t _z_bytes_decode(_z_rbuf_t *buf);
void _z_bytes_free(z_bytes_t *bs);

_Z_RESULT_DECLARE(z_str_t, str)
int _z_str_encode(_z_wbuf_t *buf, const z_str_t s);
_z_str_result_t _z_str_decode(_z_rbuf_t *buf);

#endif /* _ZENOH_PICO_CODEC_H */
