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

#ifndef ZENOH_PICO_REPLY_NETAPI_H
#define ZENOH_PICO_REPLY_NETAPI_H

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/collections/refcount.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/net/encoding.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/session.h"

/**
 * An reply to a :c:func:`z_query`.
 *
 * Members:
 *   _z_sample_t data: a :c:type:`_z_sample_t` containing the key and value of the reply.
 *   _z_bytes_t replier_id: The id of the replier that sent this reply.
 *
 */
typedef struct _z_reply_data_t {
    _z_sample_rc_t sample;
    _z_id_t replier_id;
} _z_reply_data_t;

void _z_reply_data_clear(_z_reply_data_t *rd);
void _z_reply_data_copy(_z_reply_data_t *dst, _z_reply_data_t *src);
_z_reply_t _z_reply_move(_z_reply_t *src_reply);

_Z_ELEM_DEFINE(_z_reply_data, _z_reply_data_t, _z_noop_size, _z_reply_data_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_reply_data, _z_reply_data_t)

/**
 * An reply to a :c:func:`z_query`.
 *
 * Members:
 *   _z_reply_t_Tag tag: Indicates if the reply contains data or if it's a FINAL reply.
 *   _z_reply_data_t data: The reply data if :c:member:`_z_reply_t.tag` equals
 * :c:member:`_z_reply_t_Tag.Z_REPLY_TAG_DATA`.
 *
 */
typedef struct _z_reply_t {
    _z_reply_data_t data;
    z_reply_tag_t _tag;
} _z_reply_t;

_z_reply_t _z_reply_null(void);
void _z_reply_clear(_z_reply_t *src);
void _z_reply_free(_z_reply_t **hello);
void _z_reply_copy(_z_reply_t *dst, _z_reply_t *src);
_z_reply_t _z_reply_create(_z_keyexpr_t keyexpr, z_reply_tag_t tag, _z_id_t id, const _z_bytes_t *payload,
                           const _z_timestamp_t *timestamp, _z_encoding_t encoding, z_sample_kind_t kind,
                           z_attachment_t att);

_Z_REFCOUNT_DEFINE(_z_reply, _z_reply)

typedef struct _z_pending_reply_t {
    _z_reply_t _reply;
    _z_timestamp_t _tstamp;
} _z_pending_reply_t;

_Bool _z_pending_reply_eq(const _z_pending_reply_t *one, const _z_pending_reply_t *two);
void _z_pending_reply_clear(_z_pending_reply_t *res);

_Z_ELEM_DEFINE(_z_pending_reply, _z_pending_reply_t, _z_noop_size, _z_pending_reply_clear, _z_noop_copy)
_Z_LIST_DEFINE(_z_pending_reply, _z_pending_reply_t)

#endif /* ZENOH_PICO_REPLY_NETAPI_H */
