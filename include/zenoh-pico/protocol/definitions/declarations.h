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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_DECLARATIONS_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_DECLARATIONS_H

#include <stdint.h>

#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"

typedef struct {
    uint16_t _id;
    _z_keyexpr_t _keyexpr;
} _z_decl_kexpr_t;
_z_decl_kexpr_t _z_decl_kexpr_null(void);
typedef struct {
    uint16_t _id;
} _z_undecl_kexpr_t;
_z_undecl_kexpr_t _z_undecl_kexpr_null(void);

typedef struct {
    _z_keyexpr_t _keyexpr;
    uint32_t _id;
    struct {
        _Bool _pull_mode;
        _Bool _reliable;
    } _ext_subinfo;
} _z_decl_subscriber_t;
_z_decl_subscriber_t _z_decl_subscriber_null(void);
typedef struct {
    uint32_t _id;
    _z_keyexpr_t _ext_keyexpr;
} _z_undecl_subscriber_t;
_z_undecl_subscriber_t _z_undecl_subscriber_null(void);

typedef struct {
    _z_keyexpr_t _keyexpr;
    uint32_t _id;
    struct {
        uint8_t _complete;
        uint32_t _distance;
    } _ext_queryable_info;
} _z_decl_queryable_t;
_z_decl_queryable_t _z_decl_queryable_null(void);
typedef struct {
    uint32_t _id;
    _z_keyexpr_t _ext_keyexpr;
} _z_undecl_queryable_t;
_z_undecl_queryable_t _z_undecl_queryable_null(void);

typedef struct {
    _z_keyexpr_t _keyexpr;
    uint32_t _id;
} _z_decl_token_t;
_z_decl_token_t _z_decl_token_null(void);
typedef struct {
    uint32_t _id;
    _z_keyexpr_t _ext_keyexpr;
} _z_undecl_token_t;
_z_undecl_token_t _z_undecl_token_null(void);

#define _Z_INTEREST_FLAG_KEYEXPRS (1)
#define _Z_INTEREST_FLAG_SUBSCRIBERS (1 << 1)
#define _Z_INTEREST_FLAG_QUERYABLES (1 << 2)
#define _Z_INTEREST_FLAG_TOKENS (1 << 3)
#define _Z_INTEREST_FLAG_CURRENT (1 << 5)
#define _Z_INTEREST_FLAG_FUTURE (1 << 6)
#define _Z_INTEREST_FLAG_AGGREGATE (1 << 7)
typedef struct {
    _z_keyexpr_t _keyexpr;
    uint32_t _id;
    uint8_t interest_flags;
} _z_decl_interest_t;
_z_decl_interest_t _z_decl_interest_null(void);
typedef struct {
    uint32_t _id;
} _z_final_interest_t;
_z_final_interest_t _z_final_interest_null(void);
typedef struct {
    uint32_t _id;
    _z_keyexpr_t _ext_keyexpr;
} _z_undecl_interest_t;
_z_undecl_interest_t _z_undecl_interest_null(void);

typedef struct {
    enum {
        _Z_DECL_KEXPR,
        _Z_UNDECL_KEXPR,
        _Z_DECL_SUBSCRIBER,
        _Z_UNDECL_SUBSCRIBER,
        _Z_DECL_QUERYABLE,
        _Z_UNDECL_QUERYABLE,
        _Z_DECL_TOKEN,
        _Z_UNDECL_TOKEN,
        _Z_DECL_INTEREST,
        _Z_FINAL_INTEREST,
        _Z_UNDECL_INTEREST,
    } _tag;
    union {
        _z_decl_kexpr_t _decl_kexpr;
        _z_undecl_kexpr_t _undecl_kexpr;
        _z_decl_subscriber_t _decl_subscriber;
        _z_undecl_subscriber_t _undecl_subscriber;
        _z_decl_queryable_t _decl_queryable;
        _z_undecl_queryable_t _undecl_queryable;
        _z_decl_token_t _decl_token;
        _z_undecl_token_t _undecl_token;
        _z_decl_interest_t _decl_interest;
        _z_final_interest_t _final_interest;
        _z_undecl_interest_t _undecl_interest;
    } _body;
} _z_declaration_t;
void _z_declaration_clear(_z_declaration_t* decl);
static inline void _z_decl_fix_mapping(_z_declaration_t* msg, uint16_t mapping) {
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
        case _Z_DECL_INTEREST: {
            _z_keyexpr_fix_mapping(&msg->_body._decl_interest._keyexpr, mapping);
        } break;
        case _Z_UNDECL_INTEREST: {
            _z_keyexpr_fix_mapping(&msg->_body._undecl_interest._ext_keyexpr, mapping);
        } break;
        default:
            break;
    }
}

_z_declaration_t _z_make_decl_keyexpr(uint16_t id, _Z_MOVE(_z_keyexpr_t) key);
_z_declaration_t _z_make_undecl_keyexpr(uint16_t id);

_z_declaration_t _z_make_decl_subscriber(_Z_MOVE(_z_keyexpr_t) key, uint32_t id, _Bool reliable, _Bool pull_mode);
_z_declaration_t _z_make_undecl_subscriber(uint32_t id, _Z_OPTIONAL _Z_MOVE(_z_keyexpr_t) key);

_z_declaration_t _z_make_decl_queryable(_Z_MOVE(_z_keyexpr_t) key, uint32_t id, uint32_t distance, uint8_t complete);
_z_declaration_t _z_make_undecl_queryable(uint32_t id, _Z_OPTIONAL _Z_MOVE(_z_keyexpr_t) key);

_z_declaration_t _z_make_decl_token(_Z_MOVE(_z_keyexpr_t) key, uint32_t id);
_z_declaration_t _z_make_undecl_token(uint32_t id, _Z_OPTIONAL _Z_MOVE(_z_keyexpr_t) key);

_z_declaration_t _z_make_decl_interest(_Z_MOVE(_z_keyexpr_t) key, uint32_t id);
_z_declaration_t _z_make_undecl_interest(uint32_t id, _Z_OPTIONAL _Z_MOVE(_z_keyexpr_t) key);
_z_declaration_t _z_make_final_decl(uint32_t id);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_DEFINITIONS_DECLARATIONS_H */
