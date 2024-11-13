//
// Copyright (c) 2024 ZettaScale Technology
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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_INTEREST_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_INTEREST_H

#include "zenoh-pico/protocol/definitions/interest.h"
#include "zenoh-pico/protocol/iobuf.h"

#ifdef __cplusplus
extern "C" {
#endif

z_result_t _z_interest_encode(_z_wbuf_t *wbf, const _z_interest_t *interest, bool is_final);
z_result_t _z_interest_decode(_z_interest_t *decl, _z_zbuf_t *zbf, bool is_final, bool has_ext);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_CODEC_DECLARATIONS_H */
