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

int8_t _z_whatami_encode(_z_wbuf_t *wbf, z_whatami_t en) { return _z_zsize_encode(wbf, en); }

int8_t _z_whatami_decode(z_whatami_t *en, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t tmp;
    ret |= _z_zsize_decode(&tmp, zbf);
    *en = tmp;

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

    ret |= _z_wbuf_write(wbf, (u16 & 0xFF));
    ret |= _z_wbuf_write(wbf, ((u16 >> 8) & 0xFF));

    return ret;
}

int8_t _z_uint16_decode(uint16_t *u16, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    *u16 = 0;

    uint8_t u8;
    ret |= _z_uint8_decode(&u8, zbf);
    *u16 |= u8;
    ret |= _z_uint8_decode(&u8, zbf);
    *u16 |= u8 << 8;

    return ret;
}

/*------------------ z_zint ------------------*/
#define VLE_LEN 9

uint8_t _z_zint_len(uint64_t v) {
    if ((v & (UINT64_MAX << (7 * 1))) == 0) {
        return 1;
    } else if ((v & (UINT64_MAX << (7 * 2))) == 0) {
        return 2;
    } else if ((v & (UINT64_MAX << (7 * 3))) == 0) {
        return 3;
    } else if ((v & (UINT64_MAX << (7 * 4))) == 0) {
        return 4;
    } else if ((v & (UINT64_MAX << (7 * 5))) == 0) {
        return 5;
    } else if ((v & (UINT64_MAX << (7 * 6))) == 0) {
        return 6;
    } else if ((v & (UINT64_MAX << (7 * 7))) == 0) {
        return 7;
    } else if ((v & (UINT64_MAX << (7 * 8))) == 0) {
        return 8;
    } else {
        return 9;
    }
}

int8_t _z_zint64_encode(_z_wbuf_t *wbf, uint64_t v) {
    uint64_t lv = v;
    uint8_t len = 0;
    while ((lv & (uint64_t)~0x7f) != 0) {
        uint8_t c = (lv & 0x7f) | 0x80;
        _Z_RETURN_IF_ERR(_z_wbuf_write(wbf, c))
        len++;
        lv = lv >> (uint64_t)7;
    }
    if (len != VLE_LEN) {
        uint8_t c = (lv & 0xff);
        _Z_RETURN_IF_ERR(_z_wbuf_write(wbf, c))
    }

    return _Z_RES_OK;
}

int8_t _z_zint16_encode(_z_wbuf_t *wbf, uint16_t v) { return _z_zint64_encode(wbf, (uint64_t)v); }
int8_t _z_zint32_encode(_z_wbuf_t *wbf, uint32_t v) { return _z_zint64_encode(wbf, (uint64_t)v); }
int8_t _z_zsize_encode(_z_wbuf_t *wbf, _z_zint_t v) { return _z_zint64_encode(wbf, (uint64_t)v); }

int8_t _z_zint64_decode(uint64_t *zint, _z_zbuf_t *zbf) {
    *zint = 0;

    uint8_t b = 0;
    _Z_RETURN_IF_ERR(_z_uint8_decode(&b, zbf));

    uint8_t i = 0;
    while (((b & 0x80) != 0) && (i != 7 * (VLE_LEN - 1))) {
        *zint = *zint | ((uint64_t)(b & 0x7f)) << i;
        _Z_RETURN_IF_ERR(_z_uint8_decode(&b, zbf));
        i = i + (uint8_t)7;
    }
    *zint = *zint | ((uint64_t)b << i);

    return _Z_RES_OK;
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
    int8_t ret = _Z_RES_OK;
    uint64_t buf;
    _Z_RETURN_IF_ERR(_z_zint64_decode(&buf, zbf));
    if (buf <= SIZE_MAX) {
        *zint = (_z_zint_t)buf;
    } else {
        ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }
    return ret;
}

/*------------------ uint8_array ------------------*/
int8_t _z_bytes_val_encode(_z_wbuf_t *wbf, const _z_bytes_t *bs) {
    int8_t ret = _Z_RES_OK;

    if ((wbf->_expansion_step != 0) && (bs->len > Z_TSID_LENGTH)) {
        ret |= _z_wbuf_wrap_bytes(wbf, bs->start, 0, bs->len);
    } else {
        ret |= _z_wbuf_write_bytes(wbf, bs->start, 0, bs->len);
    }

    return ret;
}

int8_t _z_bytes_encode(_z_wbuf_t *wbf, const _z_bytes_t *bs) {
    int8_t ret = _Z_RES_OK;

    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, bs->len))
    _Z_RETURN_IF_ERR(_z_bytes_val_encode(wbf, bs))

    return ret;
}
size_t _z_bytes_encode_len(const _z_bytes_t *bs) { return _z_zint_len(bs->len) + bs->len; }

int8_t _z_bytes_val_decode_na(_z_bytes_t *bs, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    if (ret == _Z_RES_OK) {
        if (_z_zbuf_len(zbf) >= bs->len) {                           // Check if we have enough bytes to read
            *bs = _z_bytes_wrap(_z_zbuf_get_rptr(zbf), bs->len);     // Decode without allocating
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

int8_t _z_bytes_decode_na(_z_bytes_t *bs, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_zsize_decode(&bs->len, zbf);
    ret |= _z_bytes_val_decode_na(bs, zbf);

    return ret;
}

int8_t _z_bytes_val_decode(_z_bytes_t *bs, _z_zbuf_t *zbf) { return _z_bytes_val_decode_na(bs, zbf); }

int8_t _z_bytes_decode(_z_bytes_t *bs, _z_zbuf_t *zbf) { return _z_bytes_decode_na(bs, zbf); }

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

/*------------------ encoding ------------------*/
#define _Z_ENCODING_FLAG_S 0x01

size_t _z_encoding_len(const _z_encoding_t *en) {
    size_t en_len = _z_zint_len((uint32_t)(en->prefix) << 1);
    if (_z_bytes_check(en->suffix)) {
        en_len += _z_bytes_encode_len(&en->suffix);
    }
    return en_len;
}

int8_t _z_encoding_encode(_z_wbuf_t *wbf, const _z_encoding_t *en) {
    _Bool has_suffix = _z_bytes_check(en->suffix);
    uint32_t id = (uint32_t)(en->prefix) << 1;
    if (has_suffix) {
        id |= _Z_ENCODING_FLAG_S;
    }
    _Z_RETURN_IF_ERR(_z_zint32_encode(wbf, id));
    if (has_suffix) {
        _Z_RETURN_IF_ERR(_z_bytes_encode(wbf, &en->suffix));
    }
    return _Z_RES_OK;
}

int8_t _z_encoding_decode(_z_encoding_t *en, _z_zbuf_t *zbf) {
    uint32_t id = 0;
    _Bool has_suffix = false;
    _Z_RETURN_IF_ERR(_z_zint32_decode(&id, zbf));
    if ((id & _Z_ENCODING_FLAG_S) != 0) {
        has_suffix = true;
    }
    en->prefix = (z_encoding_prefix_t)(id >> 1);
    if (has_suffix) {
        _Z_RETURN_IF_ERR(_z_bytes_decode(&en->suffix, zbf));
    }
    return _Z_RES_OK;
}
