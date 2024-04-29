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

#include "zenoh-pico/protocol/definitions/declarations.h"

#include "zenoh-pico/protocol/keyexpr.h"

void _z_declaration_clear(_z_declaration_t *decl) {
    switch (decl->_tag) {
        case _Z_DECL_KEXPR: {
            _z_keyexpr_clear(&decl->_body._decl_kexpr._keyexpr);
            break;
        }
        case _Z_UNDECL_KEXPR: {
            break;
        }
        case _Z_DECL_SUBSCRIBER: {
            _z_keyexpr_clear(&decl->_body._decl_subscriber._keyexpr);
            break;
        }
        case _Z_UNDECL_SUBSCRIBER: {
            _z_keyexpr_clear(&decl->_body._undecl_subscriber._ext_keyexpr);
            break;
        }
        case _Z_DECL_QUERYABLE: {
            _z_keyexpr_clear(&decl->_body._decl_queryable._keyexpr);
            break;
        }
        case _Z_UNDECL_QUERYABLE: {
            _z_keyexpr_clear(&decl->_body._undecl_queryable._ext_keyexpr);
            break;
        }
        case _Z_DECL_TOKEN: {
            _z_keyexpr_clear(&decl->_body._decl_token._keyexpr);
            break;
        }
        case _Z_UNDECL_TOKEN: {
            _z_keyexpr_clear(&decl->_body._undecl_token._ext_keyexpr);
            break;
        }
        default:
        case _Z_DECL_FINAL: {
            break;
        }
    }
}
_z_declaration_t _z_make_decl_keyexpr(uint16_t id, _Z_MOVE(_z_keyexpr_t) key) {
    return (_z_declaration_t){._tag = _Z_DECL_KEXPR,
                              ._body = {._decl_kexpr = {._id = id, ._keyexpr = _z_keyexpr_steal(key)}}};
}
_z_declaration_t _z_make_undecl_keyexpr(uint16_t id) {
    return (_z_declaration_t){._tag = _Z_UNDECL_KEXPR, ._body = {._undecl_kexpr = {._id = id}}};
}
_z_declaration_t _z_make_decl_subscriber(_Z_MOVE(_z_keyexpr_t) key, uint32_t id, _Bool reliable) {
    return (_z_declaration_t){
        ._tag = _Z_DECL_SUBSCRIBER,
        ._body = {._decl_subscriber = {
                      ._id = id, ._keyexpr = _z_keyexpr_steal(key), ._ext_subinfo = {._reliable = reliable}}}};
}

_z_declaration_t _z_make_undecl_subscriber(uint32_t id, _Z_OPTIONAL const _z_keyexpr_t *key) {
    return (_z_declaration_t){
        ._tag = _Z_UNDECL_SUBSCRIBER,
        ._body = {._undecl_subscriber = {
                      ._id = id, ._ext_keyexpr = (key == NULL) ? _z_keyexpr_null() : _z_keyexpr_duplicate(*key)}}};
}

_z_declaration_t _z_make_decl_queryable(_Z_MOVE(_z_keyexpr_t) key, uint32_t id, uint16_t distance, _Bool complete) {
    return (_z_declaration_t){
        ._tag = _Z_DECL_QUERYABLE,
        ._body = {._decl_queryable = {._id = id,
                                      ._keyexpr = _z_keyexpr_steal(key),
                                      ._ext_queryable_info = {._complete = complete, ._distance = distance}}}};
}
_z_declaration_t _z_make_undecl_queryable(uint32_t id, _Z_OPTIONAL const _z_keyexpr_t *key) {
    return (_z_declaration_t){
        ._tag = _Z_UNDECL_QUERYABLE,
        ._body = {._undecl_queryable = {
                      ._id = id, ._ext_keyexpr = (key == NULL) ? _z_keyexpr_null() : _z_keyexpr_duplicate(*key)}}};
}
_z_declaration_t _z_make_decl_token(_Z_MOVE(_z_keyexpr_t) key, uint32_t id) {
    return (_z_declaration_t){._tag = _Z_DECL_TOKEN,
                              ._body = {._decl_token = {
                                            ._id = id,
                                            ._keyexpr = _z_keyexpr_steal(key),
                                        }}};
}
_z_declaration_t _z_make_undecl_token(uint32_t id, _Z_OPTIONAL const _z_keyexpr_t *key) {
    return (_z_declaration_t){
        ._tag = _Z_UNDECL_TOKEN,
        ._body = {._undecl_token = {._id = id,
                                    ._ext_keyexpr = (key == NULL) ? _z_keyexpr_null() : _z_keyexpr_duplicate(*key)}}};
}
_z_declaration_t _z_make_decl_final(void) {
    return (_z_declaration_t){._tag = _Z_DECL_FINAL, ._body = {._decl_final = {0}}};
}

_z_decl_kexpr_t _z_decl_kexpr_null(void) { return (_z_decl_kexpr_t){0}; }
_z_decl_subscriber_t _z_decl_subscriber_null(void) { return (_z_decl_subscriber_t){0}; }
_z_decl_queryable_t _z_decl_queryable_null(void) { return (_z_decl_queryable_t){0}; }
_z_decl_token_t _z_decl_token_null(void) { return (_z_decl_token_t){0}; }
_z_undecl_kexpr_t _z_undecl_kexpr_null(void) { return (_z_undecl_kexpr_t){0}; }
_z_undecl_subscriber_t _z_undecl_subscriber_null(void) { return (_z_undecl_subscriber_t){0}; }
_z_undecl_queryable_t _z_undecl_queryable_null(void) { return (_z_undecl_queryable_t){0}; }
_z_undecl_token_t _z_undecl_token_null(void) { return (_z_undecl_token_t){0}; }
_z_decl_final_t _z_decl_final_null(void) { return (_z_decl_final_t){0}; }

void _z_decl_fix_mapping(_z_declaration_t *msg, uint16_t mapping) {
    switch (msg->_tag) {
        case _Z_DECL_KEXPR: {
            _z_keyexpr_fix_mapping(&msg->_body._decl_kexpr._keyexpr, mapping);
        } break;
        case _Z_DECL_SUBSCRIBER: {
            _z_keyexpr_fix_mapping(&msg->_body._decl_subscriber._keyexpr, mapping);
        } break;
        case _Z_UNDECL_SUBSCRIBER: {
            _z_keyexpr_fix_mapping(&msg->_body._undecl_subscriber._ext_keyexpr, mapping);
        } break;
        case _Z_DECL_QUERYABLE: {
            _z_keyexpr_fix_mapping(&msg->_body._decl_queryable._keyexpr, mapping);
        } break;
        case _Z_UNDECL_QUERYABLE: {
            _z_keyexpr_fix_mapping(&msg->_body._undecl_queryable._ext_keyexpr, mapping);
        } break;
        case _Z_DECL_TOKEN: {
            _z_keyexpr_fix_mapping(&msg->_body._decl_token._keyexpr, mapping);
        } break;
        case _Z_UNDECL_TOKEN: {
            _z_keyexpr_fix_mapping(&msg->_body._undecl_token._ext_keyexpr, mapping);
        } break;
        default:
            break;
    }
}
