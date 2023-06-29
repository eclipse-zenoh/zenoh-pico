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

void _z_declaration_clear(_z_declaration_t* decl) {
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
        case _Z_DECL_INTEREST: {
            _z_keyexpr_clear(&decl->_body._decl_interest._keyexpr);
            break;
        }
        case _Z_FINAL_INTEREST: {
            break;
        }
        case _Z_UNDECL_INTEREST: {
            _z_keyexpr_clear(&decl->_body._undecl_interest._ext_keyexpr);
            break;
        }
    }
}