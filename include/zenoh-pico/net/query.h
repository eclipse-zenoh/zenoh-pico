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

#ifndef ZENOH_PICO_QUERY_NETAPI_H
#define ZENOH_PICO_QUERY_NETAPI_H

#include <stdint.h>

#include "zenoh-pico/api/constants.h"
#include "zenoh-pico/collections/bytes.h"
#include "zenoh-pico/net/filtering.h"
#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/keyexpr.h"
#include "zenoh-pico/utils/query_params.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * The query to be answered by a queryable.
 */
typedef struct _z_query_owned_t {
    // TODO: This should be a plain _z_keyexpr_t, but we need type compatibility with user's declared key expression,
    // via z_declare_keyexpr. So this keyexpr is always undeclared.
    _z_declared_keyexpr_t _key;
    _z_value_t _value;
    _z_bytes_t _attachment;
    _z_string_t _parameters;
    _z_source_info_t _source_info;
    _z_n_qos_t _qos;
    bool _anyke;
    _z_session_weak_t _zn;
    _z_query_id_t _id;
} _z_query_owned_t;

// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_query_owned_t _z_query_owned_null(void) { return (_z_query_owned_t){0}; }
void _z_query_owned_move(_z_query_owned_t *dst, _z_query_owned_t *src);
z_result_t _z_query_owned_copy(_z_query_owned_t *dst, const _z_query_owned_t *src);
void _z_query_owned_clear(_z_query_owned_t *q);

// A non-owning view of fields of query
typedef struct _z_query_view_t {
    _z_query_owned_t _target;
} _z_query_view_t;

static inline const _z_query_owned_t *_z_query_view_deref(const _z_query_view_t *view) { return &view->_target; }

#define _ZP_VARIANT_TEMPLATE_NAME _z_query
#define _ZP_VARIANT_TEMPLATE_1_TYPE _z_query_owned_t
#define _ZP_VARIANT_TEMPLATE_1_NAME owned
#define _ZP_VARIANT_TEMPLATE_1_DESTROY_FN _z_query_owned_clear
#define _ZP_VARIANT_TEMPLATE_1_MOVE_FN _z_query_owned_move
#define _ZP_VARIANT_TEMPLATE_2_TYPE _z_query_view_t
#define _ZP_VARIANT_TEMPLATE_2_NAME view
#define _ZP_VARIANT_TEMPLATE_NO_MOVE_FN
#include "zenoh-pico/collections/variant_template.h"

// move if src is owned, copy if view
z_result_t _z_query_move_or_copy(_z_query_t *dst, _z_query_t *src);

void _z_query_create_view_from_data(_z_query_t *dst, const _z_keyexpr_t *key, const _z_value_t *opt_value,
                                    const _z_slice_t *opt_parameters, const _z_session_weak_t *zn,
                                    const _z_query_id_t *query_id, const _z_bytes_t *opt_attachment,
                                    bool implicit_anyke, _z_n_qos_t qos, const _z_source_info_t *opt_source_info);

z_result_t _z_query_copy(_z_query_t *dst, const _z_query_t *src);
static inline void _z_query_clear(_z_query_t *s) { _z_query_destroy(s); }

static inline const _z_query_owned_t *_z_query_get_ref(const _z_query_t *s) {
    _ZP_VARIANT_CONST_VISIT(_z_query, s,
        (owned, return _),
        (view, return _z_query_view_deref(_)),
        (none, return NULL)
    );
    return NULL;  // Unreachable, but avoids compiler warning.
}

static inline _z_query_t _z_query_null(void) { return _z_query_none(); }
static inline bool _z_query_check(const _z_query_t *s) { return !_z_query_is_none(s); }

z_result_t _z_received_query_count_increase(_z_session_t *zn, const _z_query_owned_t *query);
z_result_t _z_received_query_count_decrease(_z_session_t *zn, const _z_query_owned_t *query);
z_result_t _z_session_send_reply_final(_z_session_t *session, const _z_query_id_t *query_id);
/**
 * Return type when declaring a querier.
 */
typedef struct _z_querier_t {
    _z_declared_keyexpr_t _key;
    uint32_t _id;
    _z_session_weak_t _zn;
    _z_encoding_t _encoding;
    z_consolidation_mode_t _consolidation_mode;
    z_query_target_t _target;
    z_congestion_control_t _congestion_control;
    z_priority_t _priority;
    z_reliability_t reliability;
    bool _is_express;
    z_reply_keyexpr_t _accept_replies;
    uint64_t _timeout_ms;
    z_locality_t _allowed_destination;
    _z_write_filter_t _filter;
} _z_querier_t;

#if Z_FEATURE_QUERY == 1
// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_querier_t _z_querier_null(void) { return (_z_querier_t){0}; }
static inline bool _z_querier_check(const _z_querier_t *querier) { return !_Z_RC_IS_NULL(&querier->_zn); }
#endif

/**
 * Return type when declaring a queryable.
 */
typedef struct {
    uint32_t _entity_id;
    _z_session_weak_t _zn;
    _z_sync_group_t _callback_drop_sync_group;
} _z_queryable_t;

#if Z_FEATURE_QUERYABLE == 1
// Warning: None of the sub-types require a non-0 initialization. Add a init function if it changes.
static inline _z_queryable_t _z_queryable_null(void) { return (_z_queryable_t){0}; }
static inline bool _z_queryable_check(const _z_queryable_t *queryable) { return !_Z_RC_IS_NULL(&queryable->_zn); }
void _z_queryable_clear(_z_queryable_t *qbl);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZENOH_PICO_QUERY_NETAPI_H */
