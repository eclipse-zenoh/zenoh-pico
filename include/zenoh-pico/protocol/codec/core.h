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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_CORE_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/utils/config.h"
#include "zenoh-pico/utils/result.h"

#define _Z_RETURN_IF_ERR(expr)    \
    {                             \
        int8_t __res = expr;      \
        if (__res != _Z_RES_OK) { \
            return __res;         \
        }                         \
    }

#define _Z_CLEAN_RETURN_IF_ERR(base_expr, clean_expr) \
    {                                                 \
        int8_t __res = base_expr;                     \
        if (__res != _Z_RES_OK) {                     \
            clean_expr;                               \
            return __res;                             \
        }                                             \
    }

/*------------------ Internal Zenoh-net Macros ------------------*/
int8_t _z_consolidation_mode_encode(_z_wbuf_t *wbf, z_consolidation_mode_t en);
int8_t _z_consolidation_mode_decode(z_consolidation_mode_t *en, _z_zbuf_t *zbf);
int8_t _z_query_target_encode(_z_wbuf_t *wbf, z_query_target_t en);
int8_t _z_query_target_decode(z_query_target_t *en, _z_zbuf_t *zbf);
int8_t _z_whatami_encode(_z_wbuf_t *wbf, z_whatami_t en);
int8_t _z_whatami_decode(z_whatami_t *en, _z_zbuf_t *zbf);

int8_t _z_uint8_encode(_z_wbuf_t *buf, uint8_t v);
int8_t _z_uint8_decode(uint8_t *u8, _z_zbuf_t *buf);

int8_t _z_uint16_encode(_z_wbuf_t *buf, uint16_t v);
int8_t _z_uint16_decode(uint16_t *u16, _z_zbuf_t *buf);

uint8_t _z_zint_len(uint64_t v);
int8_t _z_zsize_encode(_z_wbuf_t *buf, _z_zint_t v);
int8_t _z_zint64_encode(_z_wbuf_t *buf, uint64_t v);
int8_t _z_zint16_decode(uint16_t *zint, _z_zbuf_t *buf);
int8_t _z_zint32_decode(uint32_t *zint, _z_zbuf_t *buf);
int8_t _z_zint64_decode(uint64_t *zint, _z_zbuf_t *buf);
int8_t _z_zsize_decode(_z_zint_t *zint, _z_zbuf_t *buf);

int8_t _z_bytes_val_encode(_z_wbuf_t *buf, const _z_bytes_t *bs);
int8_t _z_bytes_val_decode(_z_bytes_t *bs, _z_zbuf_t *buf);
int8_t _z_bytes_val_decode_na(_z_bytes_t *bs, _z_zbuf_t *zbf);

int8_t _z_bytes_encode(_z_wbuf_t *buf, const _z_bytes_t *bs);
size_t _z_bytes_encode_len(const _z_bytes_t *bs);
int8_t _z_bytes_decode(_z_bytes_t *bs, _z_zbuf_t *buf);
int8_t _z_zbuf_read_exact(_z_zbuf_t *zbf, uint8_t *dest, size_t length);

int8_t _z_str_encode(_z_wbuf_t *buf, const char *s);
int8_t _z_zstr_encode(_z_wbuf_t *wbf, const _z_string_t *s);
int8_t _z_str_decode(char **str, _z_zbuf_t *buf);

size_t _z_encoding_len(const _z_encoding_t *en);
int8_t _z_encoding_encode(_z_wbuf_t *wbf, const _z_encoding_t *en);
int8_t _z_encoding_decode(_z_encoding_t *en, _z_zbuf_t *zbf);

int8_t _z_keyexpr_encode(_z_wbuf_t *buf, _Bool has_suffix, const _z_keyexpr_t *ke);
int8_t _z_keyexpr_decode(_z_keyexpr_t *ke, _z_zbuf_t *buf, _Bool has_suffix);

int8_t _z_timestamp_encode(_z_wbuf_t *buf, const _z_timestamp_t *ts);
int8_t _z_timestamp_encode_ext(_z_wbuf_t *buf, const _z_timestamp_t *ts);
int8_t _z_timestamp_decode(_z_timestamp_t *ts, _z_zbuf_t *buf);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_CORE_H */
