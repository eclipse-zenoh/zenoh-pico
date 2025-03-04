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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/types.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/net/query.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/network.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"
#include "zenoh-pico/utils/string.h"

#if Z_FEATURE_QUERYABLE == 1

#define _Z_QLEINFOS_VEC_SIZE 4  // Arbitrary initial size

static inline _z_queryable_cache_data_t _z_queryable_cache_data_null(void) {
    _z_queryable_cache_data_t ret = {0};
    return ret;
}

void _z_queryable_cache_invalidate(_z_session_t *zn) {
#if Z_FEATURE_RX_CACHE == 1
    _z_queryable_lru_cache_clear(&zn->_queryable_cache);
#else
    _ZP_UNUSED(zn);
#endif
}

#if Z_FEATURE_RX_CACHE == 1
int _z_queryable_cache_data_compare(const void *first, const void *second) {
    _z_queryable_cache_data_t *first_data = (_z_queryable_cache_data_t *)first;
    _z_queryable_cache_data_t *second_data = (_z_queryable_cache_data_t *)second;
    return _z_keyexpr_compare(&first_data->ke_in, &second_data->ke_in);
}

void _z_queryable_cache_data_clear(_z_queryable_cache_data_t *val) {
    _z_queryable_infos_svec_clear(&val->infos);
    _z_keyexpr_clear(&val->ke_in);
    _z_keyexpr_clear(&val->ke_out);
}
#endif  // Z_FEATURE_RX_CACHE == 1

bool _z_session_queryable_eq(const _z_session_queryable_t *one, const _z_session_queryable_t *two) {
    return one->_id == two->_id;
}

void _z_session_queryable_clear(_z_session_queryable_t *qle) {
    if (qle->_dropper != NULL) {
        qle->_dropper(qle->_arg);
    }
    _z_keyexpr_clear(&qle->_key);
}

/*------------------ Queryable ------------------*/
static _z_session_queryable_rc_t *__z_get_session_queryable_by_id(_z_session_queryable_rc_list_t *qles,
                                                                  const _z_zint_t id) {
    _z_session_queryable_rc_t *ret = NULL;

    _z_session_queryable_rc_list_t *xs = qles;
    while (xs != NULL) {
        _z_session_queryable_rc_t *qle = _z_session_queryable_rc_list_head(xs);
        if (id == _Z_RC_IN_VAL(qle)->_id) {
            ret = qle;
            break;
        }

        xs = _z_session_queryable_rc_list_tail(xs);
    }

    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static _z_session_queryable_rc_t *__unsafe_z_get_session_queryable_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_queryable_rc_list_t *qles = zn->_local_queryable;
    return __z_get_session_queryable_by_id(qles, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static z_result_t __unsafe_z_get_session_queryable_by_key(_z_session_t *zn, const _z_keyexpr_t *key,
                                                          _z_queryable_infos_svec_t *qle_infos) {
    _z_session_queryable_rc_list_t *qles = zn->_local_queryable;

    *qle_infos = _z_queryable_infos_svec_make(_Z_QLEINFOS_VEC_SIZE);
    _z_session_queryable_rc_list_t *xs = qles;
    while (xs != NULL) {
        // Parse queryable list
        _z_session_queryable_rc_t *qle = _z_session_queryable_rc_list_head(xs);
        if (_z_keyexpr_suffix_intersects(&_Z_RC_IN_VAL(qle)->_key, key)) {
            _z_queryable_infos_t new_qle_info = {.arg = _Z_RC_IN_VAL(qle)->_arg,
                                                 .callback = _Z_RC_IN_VAL(qle)->_callback};
            _Z_RETURN_IF_ERR(_z_queryable_infos_svec_append(qle_infos, &new_qle_info, false));
        }
        xs = _z_session_queryable_rc_list_tail(xs);
    }
    return _Z_RES_OK;
}

_z_session_queryable_rc_t *_z_get_session_queryable_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_mutex_lock(zn);

    _z_session_queryable_rc_t *qle = __unsafe_z_get_session_queryable_by_id(zn, id);

    _z_session_mutex_unlock(zn);

    return qle;
}

_z_session_queryable_rc_t *_z_register_session_queryable(_z_session_t *zn, _z_session_queryable_t *q) {
    _Z_DEBUG(">>> Allocating queryable for (%ju:%.*s)", (uintmax_t)q->_key._id, (int)_z_string_len(&q->_key._suffix),
             _z_string_data(&q->_key._suffix));
    _z_session_queryable_rc_t *ret = NULL;

    _z_session_mutex_lock(zn);

    ret = (_z_session_queryable_rc_t *)z_malloc(sizeof(_z_session_queryable_rc_t));
    if (ret != NULL) {
        *ret = _z_session_queryable_rc_new_from_val(q);
        zn->_local_queryable = _z_session_queryable_rc_list_push(zn->_local_queryable, ret);
    }

    _z_session_mutex_unlock(zn);

    return ret;
}

static z_result_t _z_session_queryable_get_infos(_z_session_t *zn, _z_queryable_cache_data_t *infos) {
    _z_queryable_cache_data_t *cache_entry = NULL;
#if Z_FEATURE_RX_CACHE == 1
    cache_entry = _z_queryable_lru_cache_get(&zn->_queryable_cache, infos);
#endif
    // Check cache
    if (cache_entry != NULL) {
        // Note cache entry
        infos->ke_out = _z_keyexpr_alias(&cache_entry->ke_out);
        infos->infos = _z_queryable_infos_svec_alias(&cache_entry->infos);
        infos->qle_nb = cache_entry->qle_nb;
    } else {
        // Build queryable data
        _Z_DEBUG("Resolving %d - %.*s on mapping 0x%x", infos->ke_in._id, (int)_z_string_len(&infos->ke_in._suffix),
                 _z_string_data(&infos->ke_in._suffix), _z_keyexpr_mapping_id(&infos->ke_in));
        _z_session_mutex_lock(zn);
        infos->ke_out = __unsafe_z_get_expanded_key_from_key(zn, &infos->ke_in, true);

        if (!_z_keyexpr_has_suffix(&infos->ke_out)) {
            _z_session_mutex_unlock(zn);
            return _Z_ERR_KEYEXPR_UNKNOWN;
        }
        // Get queryable list
        z_result_t ret = __unsafe_z_get_session_queryable_by_key(zn, &infos->ke_out, &infos->infos);
        _z_session_mutex_unlock(zn);
        if (ret != _Z_RES_OK) {
            return ret;
        }
        infos->qle_nb = _z_queryable_infos_svec_len(&infos->infos);
#if Z_FEATURE_RX_CACHE == 1
        // Update cache
        _z_queryable_cache_data_t cache_storage = {
            .infos = _z_queryable_infos_svec_transfer(&infos->infos),
            .ke_in = _z_keyexpr_duplicate(&infos->ke_in),
            .ke_out = _z_keyexpr_duplicate(&infos->ke_out),
            .qle_nb = infos->qle_nb,
        };
        _z_queryable_lru_cache_insert(&zn->_queryable_cache, &cache_storage);
#endif
    }
    return _Z_RES_OK;
}

static inline void _z_queryable_query_steal_data(_z_query_t *query, _z_session_rc_t *zsrc, _z_msg_query_t *msgq,
                                                 _z_keyexpr_t *key, uint32_t qid, bool anyke) {
    // Steal received data in query
    *query = _z_query_steal_data(&msgq->_ext_value, key, &msgq->_parameters, zsrc, qid, &msgq->_ext_attachment, anyke);
}

z_result_t _z_trigger_queryables(_z_session_rc_t *zsrc, _z_msg_query_t *msgq, _z_keyexpr_t *q_key, uint32_t qid) {
    _z_session_t *zn = _Z_RC_IN_VAL(zsrc);
    _z_queryable_cache_data_t qle_infos = _z_queryable_cache_data_null();
    qle_infos.ke_in = _z_keyexpr_steal(q_key);
    // Retrieve sub infos
    _Z_CLEAN_RETURN_IF_ERR(_z_session_queryable_get_infos(zn, &qle_infos), _z_keyexpr_clear(&qle_infos.ke_in);
                           _z_value_clear(&msgq->_ext_value); _z_bytes_drop(&msgq->_ext_attachment);
                           _z_slice_clear(&msgq->_parameters););
    // Check if there are queryables
    _Z_DEBUG("Triggering %ju queryables for key %d - %.*s", (uintmax_t)qle_infos.qle_nb, qle_infos.ke_out._id,
             (int)_z_string_len(&qle_infos.ke_out._suffix), _z_string_data(&qle_infos.ke_out._suffix));
    if (qle_infos.qle_nb == 0) {
        _z_keyexpr_clear(&qle_infos.ke_in);
        _z_value_clear(&msgq->_ext_value);
        _z_bytes_drop(&msgq->_ext_attachment);
        _z_slice_clear(&msgq->_parameters);
        _z_keyexpr_clear(&qle_infos.ke_out);
#if Z_FEATURE_RX_CACHE == 0
        _z_queryable_infos_svec_release(&qle_infos.infos);  // Otherwise it's released with cache
#endif
        return _Z_RES_OK;
    }
    // Check anyke
    char *slice_end = _z_ptr_char_offset((char *)msgq->_parameters.start, (ptrdiff_t)msgq->_parameters.len);
    bool anyke = false;
    if (_z_slice_check(&msgq->_parameters)) {
        if (_z_strstr((char *)msgq->_parameters.start, slice_end, Z_SELECTOR_QUERY_MATCH) != NULL) {
            anyke = true;
        }
    }
    // Build the z_query
    _z_query_t *q = z_malloc(sizeof(_z_query_t));
    if (q == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _z_query_rc_t query = _z_query_rc_new(q);
    if (_Z_RC_IS_NULL(&query)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _z_queryable_query_steal_data(q, zsrc, msgq, &qle_infos.ke_out, qid, anyke);

    z_result_t ret = _Z_RES_OK;
    // Parse session_queryable svec
    for (size_t i = 0; i < qle_infos.qle_nb; i++) {
        _z_queryable_infos_t *qle_info = _z_queryable_infos_svec_get(&qle_infos.infos, i);
        if (i + 1 == qle_infos.qle_nb) {
            qle_info->callback(&query, qle_info->arg);
        } else {
            _z_query_rc_t query_copy;
            ret = _z_query_rc_copy(&query_copy, &query);
            if (ret != _Z_RES_OK) {
                break;
            }
            qle_info->callback(&query_copy, qle_info->arg);
            _z_query_rc_drop(&query_copy);
        }
    }
    _z_query_rc_drop(&query);
#if Z_FEATURE_RX_CACHE == 0
    _z_queryable_infos_svec_release(&qle_infos.infos);  // Otherwise it's released with cache
#endif
    _z_keyexpr_clear(&qle_infos.ke_in);
    return ret;
}

void _z_unregister_session_queryable(_z_session_t *zn, _z_session_queryable_rc_t *qle) {
    _z_session_mutex_lock(zn);

    zn->_local_queryable =
        _z_session_queryable_rc_list_drop_filter(zn->_local_queryable, _z_session_queryable_rc_eq, qle);

    _z_session_mutex_unlock(zn);
}

void _z_flush_session_queryable(_z_session_t *zn) {
    _z_session_mutex_lock(zn);

    _z_session_queryable_rc_list_free(&zn->_local_queryable);

    _z_session_mutex_unlock(zn);
}
#else  //  Z_FEATURE_QUERYABLE == 0

void _z_queryable_cache_invalidate(_z_session_t *zn) { _ZP_UNUSED(zn); }

#endif  // Z_FEATURE_QUERYABLE == 1
