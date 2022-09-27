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

#ifndef ZENOH_PICO_PROTOCOL_KEYEXPR_H
#define ZENOH_PICO_PROTOCOL_KEYEXPR_H

#include <stdbool.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/protocol/core.h"

zp_keyexpr_canon_status_t _z_keyexpr_is_canon(const char *start, size_t len);
zp_keyexpr_canon_status_t _z_keyexpr_canonize(char *start, size_t *len);
_Bool _z_keyexpr_includes(const char *lstart, const size_t llen, const char *rstart, const size_t rlen);
_Bool _z_keyexpr_intersects(const char *lstart, const size_t llen, const char *rstart, const size_t rlen);

/*------------------ clone/Copy/Free helpers ------------------*/
void _z_keyexpr_copy(_z_keyexpr_t *dst, const _z_keyexpr_t *src);
_z_keyexpr_t _z_keyexpr_duplicate(const _z_keyexpr_t *src);
_z_timestamp_t _z_timestamp_duplicate(const _z_timestamp_t *tstamp);
void _z_timestamp_reset(_z_timestamp_t *tstamp);

#endif /* ZENOH_PICO_PROTOCOL_KEYEXPR_H */
