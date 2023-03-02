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

#include "zenoh-pico/utils/logging.h"

/*------------------ period ------------------*/
int8_t _z_period_encode(_z_wbuf_t *buf, const _z_period_t *tp) {
    _Z_EC(_z_uint_encode(buf, tp->origin))
    _Z_EC(_z_uint_encode(buf, tp->period))
    return _z_uint_encode(buf, tp->duration);
}

int8_t _z_period_decode_na(_z_period_t *p, _z_zbuf_t *buf) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint_decode(&p->origin, buf);
    if (ret == _Z_RES_OK) {
        ret |= _z_uint_decode(&p->period, buf);
    } else {
        p->origin = 0;
    }

    if (ret == _Z_RES_OK) {
        ret |= _z_uint_decode(&p->duration, buf);
    } else {
        p->period = 0;
    }

    if (ret != _Z_RES_OK) {
        p->duration = 0;
    }

    return ret;
}

int8_t _z_period_decode(_z_period_t *p, _z_zbuf_t *buf) { return _z_period_decode_na(p, buf); }

/*------------------ uint8 -------------------*/
int8_t _z_encoding_prefix_encode(_z_wbuf_t *wbf, z_encoding_prefix_t en) { return _z_zint_encode(wbf, en); }

int8_t _z_encoding_prefix_decode(z_encoding_prefix_t *en, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t tmp;
    ret |= _z_zint_decode(&tmp, zbf);
    *en = tmp;

    return ret;
}

int8_t _z_consolidation_mode_encode(_z_wbuf_t *wbf, z_consolidation_mode_t en) { return _z_zint_encode(wbf, en); }

int8_t _z_consolidation_mode_decode(z_consolidation_mode_t *en, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t tmp;
    ret |= _z_zint_decode(&tmp, zbf);
    *en = tmp;

    return ret;
}

int8_t _z_query_target_encode(_z_wbuf_t *wbf, z_query_target_t en) { return _z_zint_encode(wbf, en); }

int8_t _z_query_target_decode(z_query_target_t *en, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t tmp;
    ret |= _z_zint_decode(&tmp, zbf);
    *en = tmp;

    return ret;
}

int8_t _z_whatami_encode(_z_wbuf_t *wbf, z_whatami_t en) { return _z_zint_encode(wbf, en); }

int8_t _z_whatami_decode(z_whatami_t *en, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t tmp;
    ret |= _z_zint_decode(&tmp, zbf);
    *en = tmp;

    return ret;
}

int8_t _z_uint_encode(_z_wbuf_t *wbf, unsigned int uint) {
    unsigned int lv = uint;

    while (lv > (unsigned int)0x7f) {
        uint8_t c = (uint8_t)(lv & (unsigned int)0x7f) | (uint8_t)0x80;
        _Z_EC(_z_wbuf_write(wbf, c))
        lv = lv >> (unsigned int)7;
    }

    uint8_t c = (uint8_t)(lv & (unsigned int)0xff);
    return _z_wbuf_write(wbf, c);
}

int8_t _z_uint_decode(unsigned int *uint, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    *uint = 0;

    uint8_t i = 0;
    uint8_t u8 = 0;
    do {
        if (_z_uint8_decode(&u8, zbf) == _Z_RES_OK) {
            *uint = *uint | (((unsigned int)u8 & (unsigned int)0x7f) << (unsigned int)i);
            i = i + (uint8_t)7;
        } else {
            ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    } while (u8 > (uint8_t)0x7f);

    return ret;
}

int8_t _z_uint8_encode(_z_wbuf_t *wbf, uint8_t u8) { return _z_wbuf_write(wbf, u8); }

int8_t _z_uint8_decode(uint8_t *u8, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    *u8 = 0;

    if (_z_zbuf_can_read(zbf) == true) {
        *u8 = _z_zbuf_read(zbf);
    } else {
        _Z_DEBUG("WARNING: Not enough bytes to read\n");
        ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }

    return ret;
}

int8_t _z_uint64_encode(_z_wbuf_t *wbf, uint64_t u64) {
    uint64_t lv = u64;

    while (lv > (uint64_t)0x7f) {
        uint8_t c = (uint8_t)(lv & (uint64_t)0x7f) | (uint8_t)0x80;
        _Z_EC(_z_wbuf_write(wbf, c))
        lv = lv >> (_z_zint_t)7;
    }

    uint8_t c = lv & (uint64_t)0xff;
    return _z_wbuf_write(wbf, c);
}

int8_t _z_uint64_decode(uint64_t *u64, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    *u64 = 0;

    uint8_t i = 0;
    uint8_t u8 = 0;
    do {
        if (_z_uint8_decode(&u8, zbf) == _Z_RES_OK) {
            *u64 = *u64 | (((_z_zint_t)u8 & 0x7f) << i);
            i = i + (uint8_t)7;
        } else {
            ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    } while (u8 > (uint8_t)0x7f);

    return ret;
}

/*------------------ z_zint ------------------*/
int8_t _z_zint_encode(_z_wbuf_t *wbf, _z_zint_t v) {
    _z_zint_t lv = v;

    while (lv > 0x7f) {
        uint8_t c = (lv & 0x7f) | 0x80;
        _Z_EC(_z_wbuf_write(wbf, c))
        lv = lv >> (_z_zint_t)7;
    }

    uint8_t c = lv & 0xff;
    return _z_wbuf_write(wbf, c);
}

int8_t _z_zint_decode(_z_zint_t *zint, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    *zint = 0;

    uint8_t i = 0;
    uint8_t u8 = 0;
    do {
        if (_z_uint8_decode(&u8, zbf) == _Z_RES_OK) {
            *zint = *zint | (((_z_zint_t)u8 & 0x7f) << i);
            i = i + (uint8_t)7;
        } else {
            ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    } while (u8 > (uint8_t)0x7f);

    return ret;
}

/*------------------ uint8_array ------------------*/
int8_t _z_bytes_encode(_z_wbuf_t *wbf, const _z_bytes_t *bs) {
    int8_t ret = _Z_RES_OK;

    _Z_EC(_z_zint_encode(wbf, bs->len))
    if ((wbf->_is_expandable == true) && (bs->len > Z_TSID_LENGTH)) {
        ret |= _z_wbuf_wrap_bytes(wbf, bs->start, 0, bs->len);
    } else {
        ret |= _z_wbuf_write_bytes(wbf, bs->start, 0, bs->len);
    }

    return ret;
}

int8_t _z_bytes_decode_na(_z_bytes_t *bs, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_zint_decode(&bs->len, zbf);
    if (ret == _Z_RES_OK) {
        if (_z_zbuf_len(zbf) >= bs->len) {                           // Check if we have enought bytes to read
            *bs = _z_bytes_wrap(_z_zbuf_get_rptr(zbf), bs->len);     // Decode without allocating
            _z_zbuf_set_rpos(zbf, _z_zbuf_get_rpos(zbf) + bs->len);  // Move the read position
        } else {
            _Z_DEBUG("WARNING: Not enough bytes to read\n");
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

int8_t _z_bytes_decode(_z_bytes_t *bs, _z_zbuf_t *zbf) { return _z_bytes_decode_na(bs, zbf); }

/*------------------ string with null terminator ------------------*/
int8_t _z_str_encode(_z_wbuf_t *wbf, const char *s) {
    size_t len = strlen(s);
    _Z_EC(_z_zint_encode(wbf, len))
    // Note that this does not put the string terminator on the wire.
    return _z_wbuf_write_bytes(wbf, (const uint8_t *)s, 0, len);
}

int8_t _z_str_decode(char **str, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    _z_zint_t len = 0;
    ret |= _z_zint_decode(&len, zbf);
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
            _Z_DEBUG("WARNING: Not enough bytes to read\n");
            *str = NULL;
            ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    } else {
        *str = NULL;
        ret |= _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
    }

    return ret;
}
