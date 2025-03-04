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

#include "zenoh-pico/session/query.h"

#include <stddef.h>

#include "zenoh-pico/config.h"
#include "zenoh-pico/net/reply.h"
#include "zenoh-pico/net/sample.h"
#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_QUERY == 1
void _z_pending_query_clear(_z_pending_query_t *pen_qry) {
    if (pen_qry->_dropper != NULL) {
        pen_qry->_dropper(pen_qry->_arg);
    }
    _z_keyexpr_clear(&pen_qry->_key);
    _z_pending_reply_list_free(&pen_qry->_pending_replies);
}

bool _z_pending_query_eq(const _z_pending_query_t *one, const _z_pending_query_t *two) { return one->_id == two->_id; }

static bool _z_pending_query_timeout(const _z_pending_query_t *foo, const _z_pending_query_t *pq) {
    _ZP_UNUSED(foo);
    bool result = z_clock_elapsed_ms((z_clock_t *)&pq->_start_time) >= pq->_timeout;
    if (result) {
        _Z_INFO("Dropping query because of timeout");
    }
    return result;
}

void _z_pending_query_process_timeout(_z_session_t *zn) {
    // Lock session
    _z_session_mutex_lock(zn);
    // Drop all queries with timeout elapsed
    zn->_pending_queries = _z_pending_query_list_drop_filter(zn->_pending_queries, _z_pending_query_timeout, NULL);
    _z_session_mutex_unlock(zn);
}

/*------------------ Query ------------------*/
_z_zint_t _z_get_query_id(_z_session_t *zn) { return zn->_query_id++; }

_z_pending_query_t *__z_get_pending_query_by_id(_z_pending_query_list_t *pqls, const _z_zint_t id) {
    _z_pending_query_t *ret = NULL;

    _z_pending_query_list_t *xs = pqls;
    while (xs != NULL) {
        _z_pending_query_t *pql = _z_pending_query_list_head(xs);
        if (pql->_id == id) {
            ret = pql;
            break;
        }

        xs = _z_pending_query_list_tail(xs);
    }

    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_pending_query_t *__unsafe__z_get_pending_query_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_pending_query_list_t *pqls = zn->_pending_queries;
    return __z_get_pending_query_by_id(pqls, id);
}

_z_pending_query_t *_z_get_pending_query_by_id(_z_session_t *zn, const _z_zint_t id) {
    _z_session_mutex_lock(zn);

    _z_pending_query_t *pql = __unsafe__z_get_pending_query_by_id(zn, id);

    _z_session_mutex_unlock(zn);
    return pql;
}

z_result_t _z_register_pending_query(_z_session_t *zn, _z_pending_query_t *pen_qry) {
    z_result_t ret = _Z_RES_OK;

    _Z_DEBUG(">>> Allocating query for (%ju:%.*s)", (uintmax_t)pen_qry->_key._id,
             (int)_z_string_len(&pen_qry->_key._suffix), _z_string_data(&pen_qry->_key._suffix));

    _z_session_mutex_lock(zn);

    _z_pending_query_t *pql = __unsafe__z_get_pending_query_by_id(zn, pen_qry->_id);
    if (pql == NULL) {  // Register query only if a pending one with the same ID does not exist
        zn->_pending_queries = _z_pending_query_list_push(zn->_pending_queries, pen_qry);
    } else {
        ret = _Z_ERR_ENTITY_DECLARATION_FAILED;
    }

    _z_session_mutex_unlock(zn);

    return ret;
}

static z_result_t _z_trigger_query_reply_partial_inner(_z_session_t *zn, const _z_zint_t id,
                                                       const _z_keyexpr_t *keyexpr, _z_msg_put_t *msg,
                                                       z_sample_kind_t kind) {
    _z_session_mutex_lock(zn);

    // Get query infos
    _z_pending_query_t *pen_qry = __unsafe__z_get_pending_query_by_id(zn, id);
    if (pen_qry == NULL) {
        _z_session_mutex_unlock(zn);
        return _Z_ERR_ENTITY_UNKNOWN;
    }
    _z_keyexpr_t expanded_ke = __unsafe_z_get_expanded_key_from_key(zn, keyexpr, true);
    if (!pen_qry->_anykey && !_z_keyexpr_suffix_intersects(&pen_qry->_key, keyexpr)) {
        _z_session_mutex_unlock(zn);
        return _Z_ERR_QUERY_NOT_MATCH;
    }
    // Build the reply
    _z_reply_t reply = _z_reply_steal_data(&expanded_ke, zn->_local_zid, &msg->_payload, &msg->_commons._timestamp,
                                           &msg->_encoding, kind, &msg->_attachment, &msg->_commons._source_info);
    // Process monotonic & latest consolidation mode
    if ((pen_qry->_consolidation == Z_CONSOLIDATION_MODE_LATEST) ||
        (pen_qry->_consolidation == Z_CONSOLIDATION_MODE_MONOTONIC)) {
        bool drop = false;
        _z_pending_reply_list_t *pen_rps = pen_qry->_pending_replies;
        _z_pending_reply_t *pen_rep = NULL;

        // Verify if this is a newer reply, free the old one in case it is
        while (pen_rps != NULL) {
            pen_rep = _z_pending_reply_list_head(pen_rps);
            // Check if this is the same resource key
            if (_z_string_equals(&pen_rep->_reply.data._result.sample.keyexpr._suffix,
                                 &reply.data._result.sample.keyexpr._suffix)) {
                if (msg->_commons._timestamp.time <= pen_rep->_tstamp.time) {
                    drop = true;
                } else {
                    pen_qry->_pending_replies =
                        _z_pending_reply_list_drop_filter(pen_qry->_pending_replies, _z_pending_reply_eq, pen_rep);
                }
                break;
            }
            pen_rps = _z_pending_reply_list_tail(pen_rps);
        }
        if (!drop) {
            // Cache most recent reply
            pen_rep = (_z_pending_reply_t *)z_malloc(sizeof(_z_pending_reply_t));
            if (pen_rep == NULL) {
                _z_reply_clear(&reply);
                _z_session_mutex_unlock(zn);
                return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }
            if (pen_qry->_consolidation == Z_CONSOLIDATION_MODE_MONOTONIC) {
                // No need to store the whole reply in the monotonic mode.
                pen_rep->_reply = _z_reply_null();
                pen_rep->_reply.data._tag = _Z_REPLY_TAG_DATA;
                _Z_CLEAN_RETURN_IF_ERR(
                    _z_keyexpr_move(&pen_rep->_reply.data._result.sample.keyexpr, &reply.data._result.sample.keyexpr),
                    _z_reply_clear(&reply);
                    _z_session_mutex_unlock(zn));
            } else {
                // Copy the reply to store it out of context
                _Z_CLEAN_RETURN_IF_ERR(_z_reply_move(&pen_rep->_reply, &reply), _z_reply_clear(&reply);
                                       _z_session_mutex_unlock(zn));
            }
            pen_rep->_tstamp = _z_timestamp_duplicate(&msg->_commons._timestamp);
            pen_qry->_pending_replies = _z_pending_reply_list_push(pen_qry->_pending_replies, pen_rep);
        }
    }
    _z_session_mutex_unlock(zn);

    // Trigger callback if applicable
    if (pen_qry->_consolidation != Z_CONSOLIDATION_MODE_LATEST) {
        pen_qry->_callback(&reply, pen_qry->_arg);
    }
    _z_reply_clear(&reply);
    return _Z_RES_OK;
}

z_result_t _z_trigger_query_reply_partial(_z_session_t *zn, const _z_zint_t id, _z_keyexpr_t *keyexpr,
                                          _z_msg_put_t *msg, z_sample_kind_t kind) {
    z_result_t ret = _z_trigger_query_reply_partial_inner(zn, id, keyexpr, msg, kind);
    // Clean up
    _z_keyexpr_clear(keyexpr);
    _z_bytes_drop(&msg->_payload);
    _z_bytes_drop(&msg->_attachment);
    _z_encoding_clear(&msg->_encoding);
    return ret;
}

z_result_t _z_trigger_query_reply_err(_z_session_t *zn, _z_zint_t id, _z_msg_err_t *msg) {
    z_result_t ret = _Z_RES_OK;

    // Retrieve query
    _z_session_mutex_lock(zn);
    _z_pending_query_t *pen_qry = __unsafe__z_get_pending_query_by_id(zn, id);
    if (pen_qry == NULL) {
        ret = _Z_ERR_ENTITY_UNKNOWN;
    }
    _z_session_mutex_unlock(zn);

    // Trigger the user callback
    if (ret == _Z_RES_OK) {
        _z_reply_t reply = _z_reply_err_steal_data(&msg->_payload, &msg->_encoding);
        pen_qry->_callback(&reply, pen_qry->_arg);
        _z_reply_clear(&reply);
    } else {
        _z_bytes_drop(&msg->_payload);
        _z_encoding_clear(&msg->_encoding);
    }
    return ret;
}

z_result_t _z_trigger_query_reply_final(_z_session_t *zn, _z_zint_t id) {
    z_result_t ret = _Z_RES_OK;

    // Retrieve query
    _z_session_mutex_lock(zn);
    _z_pending_query_t *pen_qry = __unsafe__z_get_pending_query_by_id(zn, id);
    if (pen_qry == NULL) {
        ret = _Z_ERR_ENTITY_UNKNOWN;
    }
    // The reply is the final one, apply consolidation if needed
    if ((ret == _Z_RES_OK) && (pen_qry->_consolidation == Z_CONSOLIDATION_MODE_LATEST)) {
        while (pen_qry->_pending_replies != NULL) {
            _z_pending_reply_t *pen_rep = _z_pending_reply_list_head(pen_qry->_pending_replies);

            // Trigger the query handler
            pen_qry->_callback(&pen_rep->_reply, pen_qry->_arg);
            pen_qry->_pending_replies = _z_pending_reply_list_pop(pen_qry->_pending_replies, NULL);
        }
    }
    if (ret == _Z_RES_OK) {
        // Dropping a pending query triggers the dropper callback that is now the equivalent to a reply with the FINAL
        zn->_pending_queries = _z_pending_query_list_drop_filter(zn->_pending_queries, _z_pending_query_eq, pen_qry);
    }
    _z_session_mutex_unlock(zn);
    return ret;
}

void _z_unregister_pending_query(_z_session_t *zn, _z_pending_query_t *pen_qry) {
    _z_session_mutex_lock(zn);

    zn->_pending_queries = _z_pending_query_list_drop_filter(zn->_pending_queries, _z_pending_query_eq, pen_qry);

    _z_session_mutex_unlock(zn);
}

void _z_flush_pending_queries(_z_session_t *zn) {
    _z_session_mutex_lock(zn);

    _z_pending_query_list_free(&zn->_pending_queries);

    _z_session_mutex_unlock(zn);
}
#else

void _z_pending_query_process_timeout(_z_session_t *zn) {
    _ZP_UNUSED(zn);
    return;
}

#endif
