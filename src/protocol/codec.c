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

#include "zenoh-pico/protocol/codec.h"

#include <stdint.h>

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/utils/endianness.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

/*------------------ uint8 -------------------*/
int8_t _z_consolidation_mode_encode(_z_wbuf_t *wbf, z_consolidation_mode_t en) { return _z_zsize_encode(wbf, en); }

int8_t _z_consolidation_mode_decode(z_consolidation_mode_t *en, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t tmp;
    ret |= _z_zsize_decode(&tmp, zbf);
    *en = tmp;

    return ret;
}

int8_t _z_query_target_encode(_z_wbuf_t *wbf, z_query_target_t en) { return _z_zsize_encode(wbf, en); }

int8_t _z_query_target_decode(z_query_target_t *en, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t tmp;
    ret |= _z_zsize_decode(&tmp, zbf);
    *en = tmp;

    return ret;
}

int8_t _z_whatami_encode(_z_wbuf_t *wbf, z_whatami_t en) { return _z_zsize_encode(wbf, _z_whatami_to_uint8(en)); }

int8_t _z_whatami_decode(z_whatami_t *en, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t tmp;
    ret |= _z_zsize_decode(&tmp, zbf);
    *en = _z_whatami_from_uint8((uint8_t)tmp);

    return ret;
}

int8_t _z_uint8_encode(_z_wbuf_t *wbf, uint8_t u8) { return _z_wbuf_write(wbf, u8); }

int8_t _z_uint8_decode(uint8_t *u8, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    *u8 = 0;

    if (_z_zbuf_can_read(zbf) == true) {
        *u8 = _z_zbuf_read(zbf);
    } else {
        _Z_DEBUG("WARNING: Not enough bytes to read");
        ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }

    return ret;
}

int8_t _z_uint16_encode(_z_wbuf_t *wbf, uint16_t u16) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_wbuf_write(wbf, _z_get_u16_lsb(u16));
    ret |= _z_wbuf_write(wbf, _z_get_u16_msb(u16));

    return ret;
}

int8_t _z_uint16_decode(uint16_t *u16, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    uint8_t enc_u16[2];

    ret |= _z_uint8_decode(&enc_u16[0], zbf);
    ret |= _z_uint8_decode(&enc_u16[1], zbf);
    *u16 = _z_host_le_load16(enc_u16);

    return ret;
}

/*------------------ z_zint ------------------*/
// Zint is a variable int composed of up to 9 bytes.
// The msb of the 8 first bytes has meaning: (1: the zint continue, 0: end of the zint)
#define VLE_LEN 9
#define VLE_LEN1_MASK (UINT64_MAX << (7 * 1))
#define VLE_LEN2_MASK (UINT64_MAX << (7 * 2))
#define VLE_LEN3_MASK (UINT64_MAX << (7 * 3))
#define VLE_LEN4_MASK (UINT64_MAX << (7 * 4))
#define VLE_LEN5_MASK (UINT64_MAX << (7 * 5))
#define VLE_LEN6_MASK (UINT64_MAX << (7 * 6))
#define VLE_LEN7_MASK (UINT64_MAX << (7 * 7))
#define VLE_LEN8_MASK (UINT64_MAX << (7 * 8))

uint8_t _z_zint_len(uint64_t v) {
    if ((v & VLE_LEN1_MASK) == 0) {
        return 1;
    } else if ((v & VLE_LEN2_MASK) == 0) {
        return 2;
    } else if ((v & VLE_LEN3_MASK) == 0) {
        return 3;
    } else if ((v & VLE_LEN4_MASK) == 0) {
        return 4;
    } else if ((v & VLE_LEN5_MASK) == 0) {
        return 5;
    } else if ((v & VLE_LEN6_MASK) == 0) {
        return 6;
    } else if ((v & VLE_LEN7_MASK) == 0) {
        return 7;
    } else if ((v & VLE_LEN8_MASK) == 0) {
        return 8;
    } else {
        return 9;
    }
}

uint8_t _z_zint64_encode_buf(uint8_t *buf, uint64_t v) {
    uint64_t lv = v;
    uint8_t len = 0;
    size_t start = 0;
    while ((lv & VLE_LEN1_MASK) != 0) {
        uint8_t c = (lv & 0x7f) | 0x80;
        buf[start++] = c;
        len++;
        lv = lv >> (uint64_t)7;
    }
    if (len != VLE_LEN) {
        uint8_t c = (lv & 0xff);
        buf[start++] = c;
    }

    return (uint8_t)start;
}

int8_t _z_zint64_encode(_z_wbuf_t *wbf, uint64_t v) {
    uint8_t buf[VLE_LEN];
    size_t len = _z_zint64_encode_buf(buf, v);

    for (size_t i = 0; i < len; i++) {
        _Z_RETURN_IF_ERR(_z_wbuf_write(wbf, buf[i]));
    }
    return _Z_RES_OK;
}

int8_t _z_zint16_encode(_z_wbuf_t *wbf, uint16_t v) { return _z_zint64_encode(wbf, (uint64_t)v); }
int8_t _z_zint32_encode(_z_wbuf_t *wbf, uint32_t v) { return _z_zint64_encode(wbf, (uint64_t)v); }
int8_t _z_zsize_encode(_z_wbuf_t *wbf, _z_zint_t v) { return _z_zint64_encode(wbf, (uint64_t)v); }

int8_t _z_zint64_decode_with_reader(uint64_t *zint, __z_single_byte_reader_t reader, void *context) {
    *zint = 0;

    uint8_t b = 0;
    _Z_RETURN_IF_ERR(reader(&b, context));

    uint8_t i = 0;
    while (((b & 0x80) != 0) && (i != 7 * (VLE_LEN - 1))) {
        *zint = *zint | ((uint64_t)(b & 0x7f)) << i;
        _Z_RETURN_IF_ERR(reader(&b, context));
        i = i + (uint8_t)7;
    }
    *zint = *zint | ((uint64_t)b << i);

    return _Z_RES_OK;
}

int8_t _z_zsize_decode_with_reader(_z_zint_t *zint, __z_single_byte_reader_t reader, void *context) {
    uint64_t i = 0;
    int8_t res = _z_zint64_decode_with_reader(&i, reader, context);
    if (res != _Z_RES_OK || i > SIZE_MAX) {
        res = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    } else {
        *zint = (_z_zint_t)i;
    }
    return res;
}

int8_t _z_uint8_decode_reader(uint8_t *zint, void *context) { return _z_uint8_decode(zint, (_z_zbuf_t *)context); }

int8_t _z_zint64_decode(uint64_t *zint, _z_zbuf_t *zbf) {
    return _z_zint64_decode_with_reader(zint, _z_uint8_decode_reader, (void *)zbf);
}

int8_t _z_zint16_decode(uint16_t *zint, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    uint64_t buf;
    _Z_RETURN_IF_ERR(_z_zint64_decode(&buf, zbf));
    if (buf <= UINT16_MAX) {
        *zint = (uint16_t)buf;
    } else {
        ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
    return ret;
}

int8_t _z_zint32_decode(uint32_t *zint, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    uint64_t buf;
    _Z_RETURN_IF_ERR(_z_zint64_decode(&buf, zbf));
    if (buf <= UINT32_MAX) {
        *zint = (uint32_t)buf;
    } else {
        ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
    return ret;
}

int8_t _z_zsize_decode(_z_zint_t *zint, _z_zbuf_t *zbf) {
    return _z_zsize_decode_with_reader(zint, _z_uint8_decode_reader, (void *)zbf);
}

/*------------------ uint8_array ------------------*/
int8_t _z_buf_encode(_z_wbuf_t *wbf, const uint8_t *buf, size_t len) {
    int8_t ret = _Z_RES_OK;

    if ((wbf->_expansion_step != 0) && (len > Z_TSID_LENGTH)) {
        ret |= _z_wbuf_wrap_bytes(wbf, buf, 0, len);
    } else {
        ret |= _z_wbuf_write_bytes(wbf, buf, 0, len);
    }

    return ret;
}

int8_t _z_slice_val_encode(_z_wbuf_t *wbf, const _z_slice_t *bs) { return _z_buf_encode(wbf, bs->start, bs->len); }

int8_t _z_slice_encode(_z_wbuf_t *wbf, const _z_slice_t *bs) {
    int8_t ret = _Z_RES_OK;

    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, bs->len))
    _Z_RETURN_IF_ERR(_z_slice_val_encode(wbf, bs))

    return ret;
}

int8_t _z_slice_val_decode_na(_z_slice_t *bs, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    if (ret == _Z_RES_OK) {
        if (_z_zbuf_len(zbf) >= bs->len) {                           // Check if we have enough bytes to read
            *bs = _z_slice_wrap(_z_zbuf_get_rptr(zbf), bs->len);     // Decode without allocating
            _z_zbuf_set_rpos(zbf, _z_zbuf_get_rpos(zbf) + bs->len);  // Move the read position
        } else {
            _Z_DEBUG("WARNING: Not enough bytes to read");
            bs->len = 0;
            bs->start = NULL;
            ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    } else {
        bs->len = 0;
        bs->start = NULL;
    }

    return ret;
}

int8_t _z_slice_decode_na(_z_slice_t *bs, _z_zbuf_t *zbf) {
    _Z_RETURN_IF_ERR(_z_zsize_decode(&bs->len, zbf));
    return _z_slice_val_decode_na(bs, zbf);
}

int8_t _z_slice_val_decode(_z_slice_t *bs, _z_zbuf_t *zbf) { return _z_slice_val_decode_na(bs, zbf); }

int8_t _z_slice_decode(_z_slice_t *bs, _z_zbuf_t *zbf) { return _z_slice_decode_na(bs, zbf); }

int8_t _z_bytes_decode(_z_bytes_t *bs, _z_zbuf_t *zbf) {
    _z_slice_t s;
    _Z_RETURN_IF_ERR(_z_slice_decode(&s, zbf));
    if (s._is_alloc) {
        return _z_bytes_from_slice(bs, s);
    } else {
        return _z_bytes_from_buf(bs, s.start, s.len);
    }
}

int8_t _z_bytes_encode_val(_z_wbuf_t *wbf, const _z_bytes_t *bs) {
    int8_t ret = _Z_RES_OK;
    for (size_t i = 0; i < _z_bytes_num_slices(bs); ++i) {
        const _z_arc_slice_t *arc_s = _z_bytes_get_slice(bs, i);
        _Z_RETURN_IF_ERR(_z_buf_encode(wbf, _z_arc_slice_data(arc_s), _z_arc_slice_len(arc_s)))
    }

    return ret;
}

int8_t _z_bytes_encode(_z_wbuf_t *wbf, const _z_bytes_t *bs) {
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, _z_bytes_len(bs)))
    return _z_bytes_encode_val(wbf, bs);
}

/*------------------ string with null terminator ------------------*/
int8_t _z_str_encode(_z_wbuf_t *wbf, const char *s) {
    size_t len = strlen(s);
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, len))
    // Note that this does not put the string terminator on the wire.
    return _z_wbuf_write_bytes(wbf, (const uint8_t *)s, 0, len);
}

int8_t _z_str_decode(char **str, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t len = 0;
    ret |= _z_zsize_decode(&len, zbf);
    if (ret == _Z_RES_OK) {
        if (_z_zbuf_len(zbf) >= len) {                      // Check if we have enough bytes to read
            char *tmp = (char *)z_malloc(len + (size_t)1);  // Allocate space for the string terminator
            if (tmp != NULL) {
                tmp[len] = '\0';
                _z_zbuf_read_bytes(zbf, (uint8_t *)tmp, 0, len);
            } else {
                ret |= _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }
            *str = tmp;
        } else {
            _Z_DEBUG("WARNING: Not enough bytes to read");
            *str = NULL;
            ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    } else {
        *str = NULL;
        ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
    return ret;
}

int8_t _z_string_encode(_z_wbuf_t *wbf, const _z_string_t *s) {
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, s->len))
    // Note that this does not put the string terminator on the wire.
    return _z_wbuf_write_bytes(wbf, (const uint8_t *)s->val, 0, s->len);
}

int8_t _z_string_decode(_z_string_t *str, _z_zbuf_t *zbf) {
    *str = _z_string_null();
    _z_zint_t len = 0;
    // Decode string length
    _Z_RETURN_IF_ERR(_z_zsize_decode(&len, zbf));
    // Check if we have enough bytes to read
    if (_z_zbuf_len(zbf) < len) {
        _Z_DEBUG("WARNING: Not enough bytes to read");
        return _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
    // Allocate space for the string terminator
    str->val = (char *)z_malloc(len + (size_t)1);
    if (str->val == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    str->len = len;
    // Add null terminator
    str->val[len] = '\0';
    // Read bytes
    _z_zbuf_read_bytes(zbf, (uint8_t *)str->val, 0, len);
    return _Z_RES_OK;
}

/*------------------ encoding ------------------*/
#define _Z_ENCODING_FLAG_S 0x01

size_t _z_encoding_len(const _z_encoding_t *en) {
    size_t en_len = _z_zint_len((uint32_t)(en->id) << 1);
    if (_z_string_check(&en->schema)) {
        en_len += _z_zint_len(en->schema.len) + en->schema.len;
    }
    return en_len;
}

int8_t _z_encoding_encode(_z_wbuf_t *wbf, const _z_encoding_t *en) {
    _Bool has_schema = _z_string_check(&en->schema);
    uint32_t id = (uint32_t)(en->id) << 1;
    if (has_schema) {
        id |= _Z_ENCODING_FLAG_S;
    }
    _Z_RETURN_IF_ERR(_z_zint32_encode(wbf, id));
    if (has_schema) {
        _Z_RETURN_IF_ERR(_z_string_encode(wbf, &en->schema));
    }
    return _Z_RES_OK;
}

int8_t _z_encoding_decode(_z_encoding_t *en, _z_zbuf_t *zbf) {
    uint32_t id = 0;
    _Bool has_schema = false;
    _Z_RETURN_IF_ERR(_z_zint32_decode(&id, zbf));
    if ((id & _Z_ENCODING_FLAG_S) != 0) {
        has_schema = true;
    }
    en->id = (uint16_t)(id >> 1);
    if (has_schema) {
        _Z_RETURN_IF_ERR(_z_string_decode(&en->schema, zbf));
    }
    return _Z_RES_OK;
}

int8_t _z_value_encode(_z_wbuf_t *wbf, const _z_value_t *value) {
    size_t total_len = _z_encoding_len(&value->encoding) + _z_bytes_len(&value->payload);
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, total_len));
    _Z_RETURN_IF_ERR(_z_encoding_encode(wbf, &value->encoding));
    return _z_bytes_encode_val(wbf, &value->payload);
}

int8_t _z_value_decode(_z_value_t *value, _z_zbuf_t *zbf) {
    _Z_RETURN_IF_ERR(_z_encoding_decode(&value->encoding, zbf));
    _Z_RETURN_IF_ERR(_z_bytes_from_buf(&value->payload, (uint8_t *)_z_zbuf_start(zbf), _z_zbuf_len(zbf)));
    return _Z_RES_OK;
}
