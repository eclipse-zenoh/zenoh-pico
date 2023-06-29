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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_DECLARATIONS_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_DECLARATIONS_H

#include "zenoh-pico/protocol/definitions/declarations.h"
#include "zenoh-pico/protocol/iobuf.h"
#define _Z_DECL_KEXPR_MID 0
#define _Z_DECL_KEXPR_FLAG_N 0x20
#define _Z_UNDECL_KEXPR_MID 1
#define _Z_DECL_SUBSCRIBER_MID 2
#define _Z_DECL_SUBSCRIBER_FLAG_N 0x20
#define _Z_DECL_SUBSCRIBER_FLAG_M 0x40
#define _Z_UNDECL_SUBSCRIBER_MID 3
#define _Z_DECL_QUERYABLE_MID 4
#define _Z_UNDECL_QUERYABLE_MID 5
#define _Z_DECL_TOKEN_MID 6
#define _Z_UNDECL_TOKEN_MID 7
#define _Z_DECL_INTEREST_MID 8
#define _Z_FINAL_INTEREST_MID 9
#define _Z_UNDECL_INTEREST_MID 10
int8_t _z_declaration_encode(_z_wbuf_t *wbf, const _z_declaration_t *decl);
int8_t _z_declaration_decode(_z_declaration_t *decl, _z_zbuf_t *zbf, uint8_t header);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_DECLARATIONS_H */
