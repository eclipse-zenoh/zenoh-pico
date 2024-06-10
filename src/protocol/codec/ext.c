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

#include "zenoh-pico/protocol/ext.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/codec/ext.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/result.h"

int8_t _z_msg_ext_encode_unit(_z_wbuf_t *wbf, const _z_msg_ext_unit_t *ext) {
    int8_t ret = _Z_RES_OK;
    (void)(wbf);
    (void)(ext);
    return ret;
}

int8_t _z_msg_ext_decode_unit(_z_msg_ext_unit_t *ext, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    (void)(zbf);
    (void)(ext);
    return ret;
}

int8_t _z_msg_ext_decode_unit_na(_z_msg_ext_unit_t *ext, _z_zbuf_t *zbf) { return _z_msg_ext_decode_unit(ext, zbf); }

int8_t _z_msg_ext_encode_zint(_z_wbuf_t *wbf, const _z_msg_ext_zint_t *ext) {
    int8_t ret = _Z_RES_OK;
    _Z_RETURN_IF_ERR(_z_zint64_encode(wbf, ext->_val))
    return ret;
}

int8_t _z_msg_ext_decode_zint(_z_msg_ext_zint_t *ext, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    ret |= _z_zint64_decode(&ext->_val, zbf);
    return ret;
}

int8_t _z_msg_ext_decode_zint_na(_z_msg_ext_zint_t *ext, _z_zbuf_t *zbf) { return _z_msg_ext_decode_zint(ext, zbf); }

int8_t _z_msg_ext_encode_zbuf(_z_wbuf_t *wbf, const _z_msg_ext_zbuf_t *ext) {
    int8_t ret = _Z_RES_OK;
    _Z_RETURN_IF_ERR(_z_slice_encode(wbf, &ext->_val))
    return ret;
}

int8_t _z_msg_ext_decode_zbuf(_z_msg_ext_zbuf_t *ext, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;
    ret |= _z_slice_decode(&ext->_val, zbf);
    return ret;
}

int8_t _z_msg_ext_decode_zbuf_na(_z_msg_ext_zbuf_t *ext, _z_zbuf_t *zbf) { return _z_msg_ext_decode_zbuf(ext, zbf); }

/*------------------ Message Extension ------------------*/
int8_t _z_msg_ext_encode(_z_wbuf_t *wbf, const _z_msg_ext_t *ext, _Bool has_next) {
    int8_t ret = _Z_RES_OK;

    _Z_RETURN_IF_ERR(_z_wbuf_write(wbf, _Z_EXT_FULL_ID(ext->_header) | (has_next << 7)))

    uint8_t enc = _Z_EXT_ENC(ext->_header);
    switch (enc) {
        case _Z_MSG_EXT_ENC_UNIT: {
            _z_msg_ext_encode_unit(wbf, &ext->_body._unit);
        } break;

        case _Z_MSG_EXT_ENC_ZINT: {
            _z_msg_ext_encode_zint(wbf, &ext->_body._zint);
        } break;

        case _Z_MSG_EXT_ENC_ZBUF: {
            _z_msg_ext_encode_zbuf(wbf, &ext->_body._zbuf);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to copy message extension with unknown encoding(%d)", enc);
        } break;
    }

    return ret;
}

int8_t _z_msg_ext_unknown_body_decode(_z_msg_ext_body_t *body, uint8_t enc, _z_zbuf_t *zbf) {
    int8_t ret = _Z_RES_OK;

    switch (enc) {
        case _Z_MSG_EXT_ENC_UNIT: {
            ret |= _z_msg_ext_decode_unit(&body->_unit, zbf);
        } break;

        case _Z_MSG_EXT_ENC_ZINT: {
            ret |= _z_msg_ext_decode_zint(&body->_zint, zbf);
        } break;

        case _Z_MSG_EXT_ENC_ZBUF: {
            ret |= _z_msg_ext_decode_zbuf(&body->_zbuf, zbf);
        } break;

        default: {
            _Z_DEBUG("WARNING: Trying to copy message extension with unknown encoding(%d)", enc);
        } break;
    }

    return ret;
}

int8_t _z_msg_ext_decode(_z_msg_ext_t *ext, _z_zbuf_t *zbf, _Bool *has_next) {
    int8_t ret = _Z_RES_OK;

    ret |= _z_uint8_decode(&ext->_header, zbf);  // Decode the header
    if (ret == _Z_RES_OK) {
        // TODO: define behaviour on decode failure, regarding zbuf allocation
        ret |= _z_msg_ext_unknown_body_decode(&ext->_body, _Z_EXT_ENC(ext->_header), zbf);
    }
    *has_next = (ext->_header & _Z_MSG_EXT_FLAG_Z) != 0;
    ext->_header &= _Z_EXT_FULL_ID_MASK;

    return ret;
}

int8_t _z_msg_ext_decode_na(_z_msg_ext_t *ext, _z_zbuf_t *zbf, _Bool *has_next) {
    return _z_msg_ext_decode(ext, zbf, has_next);
}

int8_t _z_msg_ext_vec_encode(_z_wbuf_t *wbf, const _z_msg_ext_vec_t *extensions) {
    int8_t ret = _Z_RES_OK;
    size_t len = _z_msg_ext_vec_len(extensions);
    if (len > 0) {
        size_t i;
        for (i = 0; ret == _Z_RES_OK && i < len - 1; i++) {
            ret |= _z_msg_ext_encode(wbf, _z_msg_ext_vec_get(extensions, i), true);
        }
        if (ret == _Z_RES_OK) {
            ret |= _z_msg_ext_encode(wbf, _z_msg_ext_vec_get(extensions, i), true);
        }
    }
    return ret;
}
int8_t _z_msg_ext_vec_push_callback(_z_msg_ext_t *extension, _z_msg_ext_vec_t *extensions) {
    _z_msg_ext_t *ext = (_z_msg_ext_t *)z_malloc(sizeof(_z_msg_ext_t));
    *ext = *extension;
    *extension = _z_msg_ext_make_unit(0);
    _z_msg_ext_vec_append(extensions, extension);
    return 0;
}
int8_t _z_msg_ext_vec_decode(_z_msg_ext_vec_t *extensions, _z_zbuf_t *zbf) {
    _z_msg_ext_vec_reset(extensions);
    return _z_msg_ext_decode_iter(zbf, (int8_t(*)(_z_msg_ext_t *, void *))_z_msg_ext_vec_push_callback,
                                  (void *)extensions);
}

int8_t _z_msg_ext_unknown_error(_z_msg_ext_t *extension, uint8_t trace_id) {
#if (ZENOH_DEBUG >= 1)
    uint8_t ext_id = _Z_EXT_ID(extension->_header);
    switch (_Z_EXT_ENC(extension->_header)) {
        case _Z_MSG_EXT_ENC_UNIT: {
            _Z_ERROR("Unknown mandatory extension found (extension_id: %02x, trace_id: %02x), UNIT", ext_id, trace_id);
            break;
        }
        case _Z_MSG_EXT_ENC_ZINT: {
            _Z_ERROR("Unknown mandatory extension found (extension_id: %02x, trace_id: %02x), ZINT(%02jx)", ext_id,
                     trace_id, (uintmax_t)extension->_body._zint._val);
            break;
        }
        case _Z_MSG_EXT_ENC_ZBUF: {
            _z_slice_t buf = extension->_body._zbuf._val;
            char *hex = z_malloc(buf.len * 2 + 1);
            for (size_t i = 0; i < buf.len; ++i) {
                snprintf(hex + 2 * i, 3, "%02x", buf.start[i]);
            }
            _Z_ERROR("Unknown mandatory extension found (extension_id: %02x, trace_id: %02x), ZBUF(%.*s)", ext_id,
                     trace_id, (int)buf.len * 2, hex);
            z_free(hex);
            break;
        }
        default: {
            _Z_ERROR("Unknown mandatory extension found (extension_id: %02x, trace_id: %02x), UNKOWN_ENCODING", ext_id,
                     trace_id);
        }
    }
#else
    _ZP_UNUSED(extension);
    _ZP_UNUSED(trace_id);
#endif
    return _Z_ERR_MESSAGE_EXTENSION_MANDATORY_AND_UNKNOWN;
}

int8_t _z_msg_ext_skip_non_mandatory(_z_msg_ext_t *extension, void *ctx) {
    int8_t ret = _Z_RES_OK;
    if ((extension->_header & _Z_MSG_EXT_FLAG_M) != 0) {
        uint8_t trace_id = *(uint8_t *)ctx;
        ret = _z_msg_ext_unknown_error(extension, trace_id);
    }

    return ret;
}
int8_t _z_msg_ext_decode_iter(_z_zbuf_t *zbf, int8_t (*callback)(_z_msg_ext_t *, void *), void *context) {
    int8_t ret = _Z_RES_OK;
    _Bool has_next = true;
    while (has_next && ret == _Z_RES_OK) {
        _z_msg_ext_t ext = _z_msg_ext_make_unit(0);
        ret |= _z_msg_ext_decode(&ext, zbf, &has_next);
        if (ret == _Z_RES_OK) {
            ret |= callback(&ext, context);
            _z_msg_ext_clear(&ext);
        }
    }
    return ret;
}

int8_t _z_msg_ext_skip_non_mandatories(_z_zbuf_t *zbf, uint8_t trace_id) {
    return _z_msg_ext_decode_iter(zbf, _z_msg_ext_skip_non_mandatory, &trace_id);
}
