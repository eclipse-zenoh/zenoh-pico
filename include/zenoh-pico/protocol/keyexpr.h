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

#ifndef INCLUDE_ZENOH_PICO_PROTOCOL_KEYEXPR_H
#define INCLUDE_ZENOH_PICO_PROTOCOL_KEYEXPR_H

#include <stdbool.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/protocol/core.h"

zp_keyexpr_canon_status_t _z_keyexpr_is_canon(const char *start, size_t len);
zp_keyexpr_canon_status_t _z_keyexpr_canonize(char *start, size_t *len);
_Bool _z_keyexpr_includes(const char *lstart, const size_t llen, const char *rstart, const size_t rlen);
_Bool _z_keyexpr_intersects(const char *lstart, const size_t llen, const char *rstart, const size_t rlen);

/*------------------ clone/Copy/Free helpers ------------------*/
int8_t _z_keyexpr_copy(_z_keyexpr_t *dst, const _z_keyexpr_t *src);
_z_keyexpr_t _z_keyexpr_duplicate(_z_keyexpr_t src);
_z_keyexpr_t _z_keyexpr_to_owned(_z_keyexpr_t src);
_z_keyexpr_t _z_keyexpr_alias(_z_keyexpr_t src);
/// Returns either keyexpr defined by id + mapping with null suffix if try_declared is true and id is non-zero,
/// or keyexpr defined by its suffix only, with 0 id and no mapping. This is to be used only when forwarding
/// keyexpr in user api to properly separate declard keyexpr from its suffix.
_z_keyexpr_t _z_keyexpr_alias_from_user_defined(_z_keyexpr_t src, _Bool try_declared);
_z_keyexpr_t _z_keyexpr_steal(_Z_MOVE(_z_keyexpr_t) src);
static inline _z_keyexpr_t _z_keyexpr_null(void) {
    _z_keyexpr_t keyexpr = {0, {0}, NULL};
    return keyexpr;
}
void _z_keyexpr_clear(_z_keyexpr_t *rk);
void _z_keyexpr_free(_z_keyexpr_t **rk);

#endif /* INCLUDE_ZENOH_PICO_PROTOCOL_KEYEXPR_H */
