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

#include "zenoh-pico/protocol/codec/declarations.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/codec/ext.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/core.h"
#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/protocol/ext.h"
#include "zenoh-pico/protocol/iobuf.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/system/platform.h"

int8_t _z_decl_ext_keyexpr_encode(_z_wbuf_t *wbf, _z_keyexpr_t ke, _Bool has_next_ext) {
    uint8_t header = _Z_MSG_EXT_ENC_ZBUF | _Z_MSG_EXT_FLAG_M | 0x0f | (has_next_ext ? _Z_FLAG_Z_Z : 0);
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    uint32_t kelen = (uint32_t)(_z_keyexpr_has_suffix(ke) ? strlen(ke._suffix) : 0);
    header = (_z_keyexpr_is_local(&ke) ? 2 : 0) | (kelen != 0 ? 1 : 0);
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, 1 + kelen + _z_zint_len(ke._id)));
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, ke._id));
    if (kelen) {
        _Z_RETURN_IF_ERR(_z_wbuf_write_bytes(wbf, (const uint8_t *)ke._suffix, 0, kelen))
    }
    return _Z_RES_OK;
}

int8_t _z_decl_kexpr_encode(_z_wbuf_t *wbf, const _z_decl_kexpr_t *decl) {
    uint8_t header = _Z_DECL_KEXPR_MID;
    int has_kesuffix = _z_keyexpr_has_suffix(decl->_keyexpr);
    if (has_kesuffix) {
        header |= _Z_DECL_KEXPR_FLAG_N;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, decl->_id));
    _Z_RETURN_IF_ERR(_z_keyexpr_encode(wbf, has_kesuffix, &decl->_keyexpr))

    return _Z_RES_OK;
}

int8_t _z_decl_commons_encode(_z_wbuf_t *wbf, uint8_t header, _Bool has_extensions, uint32_t id, _z_keyexpr_t keyexpr) {
    _Bool has_kesuffix = _z_keyexpr_has_suffix(keyexpr);
    if (has_extensions) {
        header |= _Z_FLAG_Z_Z;
    }
    if (has_kesuffix) {
        header |= _Z_DECL_SUBSCRIBER_FLAG_N;
    }
    if (_z_keyexpr_is_local(&keyexpr)) {
        header |= _Z_DECL_SUBSCRIBER_FLAG_M;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, id));
    return _z_keyexpr_encode(wbf, has_kesuffix, &keyexpr);
}
int8_t _z_decl_subscriber_encode(_z_wbuf_t *wbf, const _z_decl_subscriber_t *decl) {
    uint8_t header = _Z_DECL_SUBSCRIBER_MID;
    _Bool has_submode_ext = decl->_ext_subinfo._reliable;
    _Z_RETURN_IF_ERR(_z_decl_commons_encode(wbf, header, has_submode_ext, decl->_id, decl->_keyexpr));
    if (has_submode_ext) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZINT | 0x01));
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, (decl->_ext_subinfo._reliable ? 1 : 0)));
    }

    return _Z_RES_OK;
}
int8_t _z_undecl_kexpr_encode(_z_wbuf_t *wbf, const _z_undecl_kexpr_t *decl) {
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_UNDECL_KEXPR));
    return _z_zsize_encode(wbf, decl->_id);
}
int8_t _z_undecl_encode(_z_wbuf_t *wbf, uint8_t header, _z_zint_t decl_id, _z_keyexpr_t ke) {
    _Bool has_keyexpr_ext = _z_keyexpr_check(&ke);
    if (has_keyexpr_ext) {
        header |= _Z_FLAG_Z_Z;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zsize_encode(wbf, decl_id));
    if (has_keyexpr_ext) {
        _Z_RETURN_IF_ERR(_z_decl_ext_keyexpr_encode(wbf, ke, false));
    }
    return _Z_RES_OK;
}
int8_t _z_undecl_subscriber_encode(_z_wbuf_t *wbf, const _z_undecl_subscriber_t *decl) {
    return _z_undecl_encode(wbf, _Z_UNDECL_SUBSCRIBER_MID, decl->_id, decl->_ext_keyexpr);
}
int8_t _z_decl_queryable_encode(_z_wbuf_t *wbf, const _z_decl_queryable_t *decl) {
    uint8_t header = _Z_DECL_QUERYABLE_MID;
    _Bool has_info_ext = decl->_ext_queryable_info._complete || (decl->_ext_queryable_info._distance != 0);
    _Z_RETURN_IF_ERR(_z_decl_commons_encode(wbf, header, has_info_ext, decl->_id, decl->_keyexpr));
    if (has_info_ext) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZINT | 0x01));
        uint8_t flags = 0;
        if (decl->_ext_queryable_info._complete) {
            flags |= 0x01;
        }
        uint64_t value = (uint64_t)flags | (uint64_t)decl->_ext_queryable_info._distance << 8;
        _Z_RETURN_IF_ERR(_z_zint64_encode(wbf, value));
    }
    return _Z_RES_OK;
}
int8_t _z_undecl_queryable_encode(_z_wbuf_t *wbf, const _z_undecl_queryable_t *decl) {
    return _z_undecl_encode(wbf, _Z_UNDECL_QUERYABLE_MID, decl->_id, decl->_ext_keyexpr);
}
int8_t _z_decl_token_encode(_z_wbuf_t *wbf, const _z_decl_token_t *decl) {
    uint8_t header = _Z_DECL_TOKEN_MID;
    _Z_RETURN_IF_ERR(_z_decl_commons_encode(wbf, header, false, decl->_id, decl->_keyexpr));
    return _Z_RES_OK;
}
int8_t _z_undecl_token_encode(_z_wbuf_t *wbf, const _z_undecl_token_t *decl) {
    return _z_undecl_encode(wbf, _Z_UNDECL_TOKEN_MID, decl->_id, decl->_ext_keyexpr);
}
int8_t _z_decl_final_encode(_z_wbuf_t *wbf) {
    uint8_t header = _Z_DECL_FINAL_MID;
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    return _Z_RES_OK;
}

int8_t _z_declaration_encode(_z_wbuf_t *wbf, const _z_declaration_t *decl) {
    int8_t ret = _Z_RES_OK;
    switch (decl->_tag) {
        case _Z_DECL_KEXPR: {
            ret = _z_decl_kexpr_encode(wbf, &decl->_body._decl_kexpr);
        } break;
        case _Z_UNDECL_KEXPR: {
            ret = _z_undecl_kexpr_encode(wbf, &decl->_body._undecl_kexpr);
        } break;
        case _Z_DECL_SUBSCRIBER: {
            ret = _z_decl_subscriber_encode(wbf, &decl->_body._decl_subscriber);
        } break;
        case _Z_UNDECL_SUBSCRIBER: {
            ret = _z_undecl_subscriber_encode(wbf, &decl->_body._undecl_subscriber);
        } break;
        case _Z_DECL_QUERYABLE: {
            ret = _z_decl_queryable_encode(wbf, &decl->_body._decl_queryable);
        } break;
        case _Z_UNDECL_QUERYABLE: {
            ret = _z_undecl_queryable_encode(wbf, &decl->_body._undecl_queryable);
        } break;
        case _Z_DECL_TOKEN: {
            ret = _z_decl_token_encode(wbf, &decl->_body._decl_token);
        } break;
        case _Z_UNDECL_TOKEN: {
            ret = _z_undecl_token_encode(wbf, &decl->_body._undecl_token);
        } break;
        case _Z_DECL_FINAL: {
            ret = _z_decl_final_encode(wbf);
        } break;
            ;
    }
    return ret;
}
int8_t _z_decl_kexpr_decode(_z_decl_kexpr_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    *decl = _z_decl_kexpr_null();
    _Z_RETURN_IF_ERR(_z_zint16_decode(&decl->_id, zbf));
    _Z_RETURN_IF_ERR(_z_keyexpr_decode(&decl->_keyexpr, zbf, _Z_HAS_FLAG(header, _Z_DECL_KEXPR_FLAG_N)));
    _z_keyexpr_set_mapping(&decl->_keyexpr, _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE);

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_skip_non_mandatories(zbf, 0x15));
    }
    return _Z_RES_OK;
}
int8_t _z_undecl_kexpr_decode(_z_undecl_kexpr_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    *decl = _z_undecl_kexpr_null();
    _Z_RETURN_IF_ERR(_z_zint16_decode(&decl->_id, zbf));

    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_skip_non_mandatories(zbf, 0x10));
    }
    return _Z_RES_OK;
}

int8_t _z_undecl_decode_extensions(_z_msg_ext_t *extension, void *ctx) {
    _z_keyexpr_t *ke = (_z_keyexpr_t *)ctx;
    switch (extension->_header) {
        case _Z_MSG_EXT_ENC_ZBUF | _Z_MSG_EXT_FLAG_M | 0x0f: {
            _z_zbuf_t _zbf = _z_slice_as_zbuf(extension->_body._zbuf._val);
            _z_zbuf_t *zbf = &_zbf;
            uint8_t header;
            _Z_RETURN_IF_ERR(_z_uint8_decode(&header, zbf));
            uint16_t mapping = _Z_HAS_FLAG(header, 2) ? _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE : _Z_KEYEXPR_MAPPING_LOCAL;
            _Z_RETURN_IF_ERR(_z_zint16_decode(&ke->_id, zbf));
            if (_Z_HAS_FLAG(header, 1)) {
                size_t len = _z_zbuf_len(zbf);
                ke->_suffix = z_malloc(len + 1);
                if (!ke->_suffix) {
                    return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
                }
                ke->_mapping = _z_keyexpr_mapping(mapping, true);
                _z_zbuf_read_bytes(zbf, (uint8_t *)ke->_suffix, 0, len);
                ke->_suffix[len] = 0;
            } else {
                ke->_mapping = _z_keyexpr_mapping(mapping, false);
            }
        } break;
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                return _z_msg_ext_unknown_error(extension, 0x0e);
            }
    }
    return _Z_RES_OK;
}
int8_t _z_undecl_trivial_decode(_z_zbuf_t *zbf, _z_keyexpr_t *_ext_keyexpr, uint32_t *decl_id, uint8_t header) {
    _Z_RETURN_IF_ERR(_z_zint32_decode(decl_id, zbf));
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_undecl_decode_extensions, _ext_keyexpr));
    }
    return _Z_RES_OK;
}
int8_t _z_decl_commons_decode(_z_zbuf_t *zbf, uint8_t header, _Bool *has_extensions, uint32_t *id, _z_keyexpr_t *ke) {
    *has_extensions = _Z_HAS_FLAG(header, _Z_FLAG_Z_Z);
    uint16_t mapping =
        _Z_HAS_FLAG(header, _Z_DECL_SUBSCRIBER_FLAG_M) ? _Z_KEYEXPR_MAPPING_UNKNOWN_REMOTE : _Z_KEYEXPR_MAPPING_LOCAL;
    _Z_RETURN_IF_ERR(_z_zint32_decode(id, zbf));
    _Z_RETURN_IF_ERR(_z_zint16_decode(&ke->_id, zbf));
    if (_Z_HAS_FLAG(header, _Z_DECL_SUBSCRIBER_FLAG_N)) {
        _z_zint_t len;
        _Z_RETURN_IF_ERR(_z_zsize_decode(&len, zbf));
        if (_z_zbuf_len(zbf) < len) {
            return _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
        ke->_suffix = z_malloc(len + 1);
        if (ke->_suffix == NULL) {
            return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        }
        ke->_mapping = _z_keyexpr_mapping(mapping, true);
        _z_zbuf_read_bytes(zbf, (uint8_t *)ke->_suffix, 0, len);
        ke->_suffix[len] = 0;
    } else {
        ke->_suffix = NULL;
        ke->_mapping = _z_keyexpr_mapping(mapping, false);
    }
    return _Z_RES_OK;
}
int8_t _z_decl_subscriber_decode_extensions(_z_msg_ext_t *extension, void *ctx) {
    _z_decl_subscriber_t *decl = (_z_decl_subscriber_t *)ctx;
    switch (extension->_header) {
        case _Z_MSG_EXT_ENC_ZINT | 0x01: {
            decl->_ext_subinfo._reliable = _Z_HAS_FLAG(extension->_body._zint._val, 1);
        } break;
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                return _z_msg_ext_unknown_error(extension, 0x14);
            }
    }
    return _Z_RES_OK;
}

int8_t _z_decl_subscriber_decode(_z_decl_subscriber_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    _Bool has_ext;
    *decl = _z_decl_subscriber_null();
    _Z_RETURN_IF_ERR(_z_decl_commons_decode(zbf, header, &has_ext, &decl->_id, &decl->_keyexpr));
    if (has_ext) {
        _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_decl_subscriber_decode_extensions, decl));
    }
    return _Z_RES_OK;
}
int8_t _z_undecl_subscriber_decode(_z_undecl_subscriber_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    *decl = _z_undecl_subscriber_null();
    return _z_undecl_trivial_decode(zbf, &decl->_ext_keyexpr, &decl->_id, header);
}
int8_t _z_decl_queryable_decode_extensions(_z_msg_ext_t *extension, void *ctx) {
    _z_decl_queryable_t *decl = (_z_decl_queryable_t *)ctx;
    switch (extension->_header) {
        case _Z_MSG_EXT_ENC_ZINT | 0x01: {
            uint64_t val = extension->_body._zint._val;
            decl->_ext_queryable_info._complete = _Z_HAS_FLAG(val, 0x01);
            decl->_ext_queryable_info._distance = (uint16_t)(val >> 8);
        } break;
        default:
            if (_Z_HAS_FLAG(extension->_header, _Z_MSG_EXT_FLAG_M)) {
                return _z_msg_ext_unknown_error(extension, 0x11);
            }
    }
    return _Z_RES_OK;
}
int8_t _z_decl_queryable_decode(_z_decl_queryable_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    _Bool has_ext;
    *decl = _z_decl_queryable_null();
    _Z_RETURN_IF_ERR(_z_decl_commons_decode(zbf, header, &has_ext, &decl->_id, &decl->_keyexpr));
    if (has_ext) {
        _Z_RETURN_IF_ERR(_z_msg_ext_decode_iter(zbf, _z_decl_queryable_decode_extensions, decl));
    }
    return _Z_RES_OK;
}
int8_t _z_undecl_queryable_decode(_z_undecl_queryable_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    *decl = _z_undecl_queryable_null();
    return _z_undecl_trivial_decode(zbf, &decl->_ext_keyexpr, &decl->_id, header);
}
int8_t _z_decl_token_decode(_z_decl_token_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    _Bool has_ext;
    *decl = _z_decl_token_null();
    _Z_RETURN_IF_ERR(_z_decl_commons_decode(zbf, header, &has_ext, &decl->_id, &decl->_keyexpr));
    if (has_ext) {
        _Z_RETURN_IF_ERR(_z_msg_ext_skip_non_mandatories(zbf, 0x12));
    }
    return _Z_RES_OK;
}
int8_t _z_undecl_token_decode(_z_undecl_token_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    return _z_undecl_trivial_decode(zbf, &decl->_ext_keyexpr, &decl->_id, header);
}

int8_t _z_decl_final_decode(_z_decl_final_t *decl, _z_zbuf_t *zbf, uint8_t header) {
    // Nothing to do
    _ZP_UNUSED(decl);
    if (_Z_HAS_FLAG(header, _Z_FLAG_Z_Z)) {
        _Z_RETURN_IF_ERR(_z_msg_ext_skip_non_mandatories(zbf, 0x13));
    }
    return _Z_RES_OK;
}

int8_t _z_declaration_decode(_z_declaration_t *decl, _z_zbuf_t *zbf) {
    uint8_t header;
    _Z_RETURN_IF_ERR(_z_uint8_decode(&header, zbf));
    int8_t ret;
    switch (_Z_MID(header)) {
        case _Z_DECL_KEXPR_MID: {
            decl->_tag = _Z_DECL_KEXPR;
            ret = _z_decl_kexpr_decode(&decl->_body._decl_kexpr, zbf, header);
        } break;
        case _Z_UNDECL_KEXPR_MID: {
            decl->_tag = _Z_UNDECL_KEXPR;
            ret = _z_undecl_kexpr_decode(&decl->_body._undecl_kexpr, zbf, header);
        } break;
        case _Z_DECL_SUBSCRIBER_MID: {
            decl->_tag = _Z_DECL_SUBSCRIBER;
            ret = _z_decl_subscriber_decode(&decl->_body._decl_subscriber, zbf, header);
        } break;
        case _Z_UNDECL_SUBSCRIBER_MID: {
            decl->_tag = _Z_UNDECL_SUBSCRIBER;
            ret = _z_undecl_subscriber_decode(&decl->_body._undecl_subscriber, zbf, header);
        } break;
        case _Z_DECL_QUERYABLE_MID: {
            decl->_tag = _Z_DECL_QUERYABLE;
            ret = _z_decl_queryable_decode(&decl->_body._decl_queryable, zbf, header);
        } break;
        case _Z_UNDECL_QUERYABLE_MID: {
            decl->_tag = _Z_UNDECL_QUERYABLE;
            ret = _z_undecl_queryable_decode(&decl->_body._undecl_queryable, zbf, header);
        } break;
        case _Z_DECL_TOKEN_MID: {
            decl->_tag = _Z_DECL_TOKEN;
            ret = _z_decl_token_decode(&decl->_body._decl_token, zbf, header);
        } break;
        case _Z_UNDECL_TOKEN_MID: {
            decl->_tag = _Z_UNDECL_TOKEN;
            ret = _z_undecl_token_decode(&decl->_body._undecl_token, zbf, header);
        } break;
        case _Z_DECL_FINAL_MID: {
            decl->_tag = _Z_DECL_FINAL;
            ret = _z_decl_final_decode(&decl->_body._decl_final, zbf, header);
        } break;
        default: {
            ret = _Z_ERR_MESSAGE_DESERIALIZATION_FAILED;
        }
    }
    return ret;
}
