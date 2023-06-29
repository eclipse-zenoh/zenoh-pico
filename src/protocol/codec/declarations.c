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

#include <stdint.h>
#include <string.h>

#include "zenoh-pico/protocol/codec.h"
#include "zenoh-pico/protocol/codec/core.h"
#include "zenoh-pico/protocol/definitions/message.h"

int8_t _z_decl_kexpr_encode(_z_wbuf_t *wbf, const _z_decl_kexpr_t *decl) {
    uint8_t header = _Z_DECL_KEXPR_MID;
    int has_kesuffix = _z_keyexpr_has_suffix(decl->_keyexpr);
    if (has_kesuffix) {
        header |= _Z_DECL_KEXPR_FLAG_N;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, decl->_id));
    _Z_RETURN_IF_ERR(_z_keyexpr_encode(wbf, has_kesuffix, &decl->_keyexpr))

    return _Z_RES_OK;
}
int8_t _z_undecl_kexpr_encode(_z_wbuf_t *wbf, const _z_undecl_kexpr_t *decl) {
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_UNDECL_KEXPR));
    return _z_zint_encode(wbf, decl->_id);
}
int8_t _z_decl_subscriber_encode(_z_wbuf_t *wbf, const _z_decl_subscriber_t *decl) {
    uint8_t header = _Z_DECL_SUBSCRIBER_MID;
    _Bool has_submode_ext = decl->_ext_subscriber_info._pull_mode || decl->_ext_subscriber_info._reliable;
    int has_kesuffix = _z_keyexpr_has_suffix(decl->_keyexpr);
    if (has_submode_ext) {
        header |= _Z_FLAG_Z_Z;
    }
    if (has_kesuffix) {
        header |= _Z_DECL_SUBSCRIBER_FLAG_N;
    }
    if (decl->_keyexpr.uses_remote_mapping) {
        header |= _Z_DECL_SUBSCRIBER_FLAG_M;
    }
    _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, header));
    _Z_RETURN_IF_ERR(_z_zint_encode(wbf, decl->_id));
    _Z_RETURN_IF_ERR(_z_keyexpr_encode(wbf, has_kesuffix, &decl->_keyexpr));
    if (has_submode_ext) {
        _Z_RETURN_IF_ERR(_z_uint8_encode(wbf, _Z_MSG_EXT_ENC_ZINT | 0x01));
        _Z_RETURN_IF_ERR(_z_uint8_encode(
            wbf, (decl->_ext_subscriber_info._pull_mode ? 2 : 0) | (decl->_ext_subscriber_info._reliable ? 1 : 0)));
    }

    return _Z_RES_OK;
}
int8_t _z_undecl_subscriber_encode(_z_wbuf_t *wbf, const _z_undecl_subscriber_t *decl) {
    uint8_t header = _Z_UNDECL_SUBSCRIBER_MID;
    if (_z_keyexpr_check(decl->_ext_keyexpr)) {
        header |= _Z_FLAG_Z_Z;
    }

    return _Z_RES_OK;
}
int8_t _z_decl_queryable_encode(_z_wbuf_t *wbf, const _z_decl_queryable_t *decl) {}
int8_t _z_undecl_queryable_encode(_z_wbuf_t *wbf, const _z_undecl_queryable_t *decl) {}
int8_t _z_decl_token_encode(_z_wbuf_t *wbf, const _z_decl_token_t *decl) {}
int8_t _z_undecl_token_encode(_z_wbuf_t *wbf, const _z_undecl_token_t *decl) {}
int8_t _z_decl_interest_encode(_z_wbuf_t *wbf, const _z_decl_interest_t *decl) {}
int8_t _z_final_interest_encode(_z_wbuf_t *wbf, const _z_final_interest_t *decl) {}
int8_t _z_undecl_interest_encode(_z_wbuf_t *wbf, const _z_undecl_interest_t *decl) {}
int8_t _z_declaration_encode(_z_wbuf_t *wbf, const _z_declaration_t *decl) {
    int8_t ret;
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
        case _Z_DECL_INTEREST: {
            ret = _z_decl_interest_encode(wbf, &decl->_body._decl_interest);
        } break;
        case _Z_FINAL_INTEREST: {
            ret = _z_final_interest_encode(wbf, &decl->_body._final_interest);
        } break;
        case _Z_UNDECL_INTEREST: {
            ret = _z_undecl_interest_encode(wbf, &decl->_body._undecl_interest);
        } break;
    }
    return ret;
}