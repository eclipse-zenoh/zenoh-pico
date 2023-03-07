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

#include <stdint.h>

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/utils/config.h"
#include "zenoh-pico/utils/result.h"

#define _Z_EC(fn)          \
    if (fn != _Z_RES_OK) { \
        return -1;         \
    }

/*------------------ Internal Zenoh-net Macros ------------------*/
int8_t _z_encoding_prefix_encode(_z_wbuf_t *wbf, z_encoding_prefix_t en);
int8_t _z_encoding_prefix_decode(z_encoding_prefix_t *en, _z_zbuf_t *zbf);
int8_t _z_consolidation_mode_encode(_z_wbuf_t *wbf, z_consolidation_mode_t en);
int8_t _z_consolidation_mode_decode(z_consolidation_mode_t *en, _z_zbuf_t *zbf);
int8_t _z_query_target_encode(_z_wbuf_t *wbf, z_query_target_t en);
int8_t _z_query_target_decode(z_query_target_t *en, _z_zbuf_t *zbf);
int8_t _z_whatami_encode(_z_wbuf_t *wbf, z_whatami_t en);
int8_t _z_whatami_decode(z_whatami_t *en, _z_zbuf_t *zbf);

int8_t _z_uint_encode(_z_wbuf_t *buf, unsigned int v);
int8_t _z_uint_decode(unsigned int *u, _z_zbuf_t *buf);

int8_t _z_uint8_encode(_z_wbuf_t *buf, uint8_t v);
int8_t _z_uint8_decode(uint8_t *u8, _z_zbuf_t *buf);

int8_t _z_uint64_encode(_z_wbuf_t *buf, uint64_t v);
int8_t _z_uint64_decode(uint64_t *u8, _z_zbuf_t *buf);

int8_t _z_zint_encode(_z_wbuf_t *buf, _z_zint_t v);
int8_t _z_zint_decode(_z_zint_t *zint, _z_zbuf_t *buf);

int8_t _z_bytes_encode(_z_wbuf_t *buf, const _z_bytes_t *bs);
int8_t _z_bytes_decode(_z_bytes_t *bs, _z_zbuf_t *buf);
int8_t _z_bytes_decode_na(_z_bytes_t *bs, _z_zbuf_t *buf);

int8_t _z_str_encode(_z_wbuf_t *buf, const char *s);
int8_t _z_str_decode(char **str, _z_zbuf_t *buf);

int8_t _z_period_encode(_z_wbuf_t *wbf, const _z_period_t *m);
int8_t _z_period_decode(_z_period_t *p, _z_zbuf_t *zbf);
int8_t _z_period_decode_na(_z_period_t *p, _z_zbuf_t *zbf);

#endif /* ZENOH_PICO_PROTOCOL_CODEC_H */
