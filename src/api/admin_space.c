//
// Copyright (c) 2025 ZettaScale Technology
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

#include "zenoh-pico/api/admin_space.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/net/primitives.h"

#if Z_FEATURE_ADMIN_SPACE == 1

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION
static z_result_t _ze_admin_space_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    z_internal_keyexpr_null(ke);
    _Z_RETURN_IF_ERR(_z_keyexpr_append_str(ke, _Z_KEYEXPR_AT));

    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(zid, &zid_str), z_keyexpr_drop(z_keyexpr_move(ke)));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_keyexpr_append_substr(ke, z_string_data(z_string_loan(&zid_str)), z_string_len(z_string_loan(&zid_str))),
        z_string_drop(z_string_move(&zid_str));
        z_keyexpr_drop(z_keyexpr_move(ke)));
    z_string_drop(z_string_move(&zid_str));

    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(ke, _Z_KEYEXPR_PICO), z_keyexpr_drop(z_keyexpr_move(ke)));
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(ke, _Z_KEYEXPR_SESSION), z_keyexpr_drop(z_keyexpr_move(ke)));
    return _Z_RES_OK;
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_STARSTAR
static z_result_t _ze_admin_space_queryable_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    _Z_RETURN_IF_ERR(_ze_admin_space_ke(ke, zid));
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(ke, _Z_KEYEXPR_STARSTAR), z_keyexpr_drop(z_keyexpr_move(ke)));
    return _Z_RES_OK;
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORT_*
static z_result_t _ze_admin_space_transport_ke(z_owned_keyexpr_t *ke, const z_id_t *zid, const char *transport) {
    _Z_RETURN_IF_ERR(_ze_admin_space_ke(ke, zid));
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(ke, transport), z_keyexpr_drop(z_keyexpr_move(ke)));
    return _Z_RES_OK;
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORT_* / _Z_KEYEXPR_LINK / LID
static z_result_t _ze_admin_space_transport_link_ke(z_owned_keyexpr_t *ke, const z_id_t *zid, const char *transport, const z_id_t *lid) {
    _Z_RETURN_IF_ERR(_ze_admin_space_transport_ke(ke, zid, transport));
    _Z_CLEAN_RETURN_IF_ERR(_z_keyexpr_append_str(ke, _Z_KEYEXPR_LINK), z_keyexpr_drop(z_keyexpr_move(ke)));

    z_owned_string_t lid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(lid, &lid_str), z_keyexpr_drop(z_keyexpr_move(ke)));
    _Z_CLEAN_RETURN_IF_ERR(
        _z_keyexpr_append_substr(ke, z_string_data(z_string_loan(&lid_str)), z_string_len(z_string_loan(&lid_str))),
        z_string_drop(z_string_move(&lid_str));
        z_keyexpr_drop(z_keyexpr_move(ke)));
    z_string_drop(z_string_move(&lid_str));

    return _Z_RES_OK;
}

static void _ze_admin_space_reply_unicast(z_loaned_query_t *query, const z_id_t *zid,
                                        _z_transport_unicast_t *unicast) {
    z_owned_keyexpr_t ke1;
    if (_ze_admin_space_transport_ke(&ke1, zid, _Z_KEYEXPR_TRANSPORT_UNICAST) != _Z_RES_OK) {
        _Z_ERROR("Failed to construct admin space unicast transport key expression - dropping query");
        return;
    }
    if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke1))) {
        // TODO: Reply unicast transport info
    }
    z_keyexpr_drop(z_keyexpr_move(&ke1));

    _z_transport_peer_unicast_slist_t *curr = unicast->_peers;
    while (curr != NULL) {
        _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(curr);
        z_owned_keyexpr_t ke2;
        if (_ze_admin_space_transport_link_ke(&ke2, zid, _Z_KEYEXPR_TRANSPORT_UNICAST, &peer->common._remote_zid) != _Z_RES_OK) {
            _Z_ERROR("Failed to construct admin space unicast link key expression - dropping query");
            return;
        }
        if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke2))) {
            // TODO: Reply unicast link info
        }
        z_keyexpr_drop(z_keyexpr_move(&ke2));

        curr = _z_transport_peer_unicast_slist_next(curr);
    }
}

static void _ze_admin_space_reply_multicast_common(z_loaned_query_t *query, const z_id_t *zid,
                                                _z_transport_multicast_t *multicast, const char *transport_keyexpr) {
    z_owned_keyexpr_t ke1;
    if (_ze_admin_space_transport_ke(&ke1, zid, transport_keyexpr) != _Z_RES_OK) {
        _Z_ERROR("Failed to construct admin space multicast transport key expression - dropping query");
        return;
    }
    if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke1))) {
        // TODO: Reply unicast transport info
    }
    z_keyexpr_drop(z_keyexpr_move(&ke1));

    _z_transport_peer_multicast_slist_t *curr = multicast->_peers;
    while (curr != NULL) {
        _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(curr);
        z_owned_keyexpr_t ke2;
        if (_ze_admin_space_transport_link_ke(&ke2, zid, transport_keyexpr, &peer->common._remote_zid) != _Z_RES_OK) {
            _Z_ERROR("Failed to construct admin space multicast link key expression - dropping query");
            return;
        }
        if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke2))) {
            // TODO: Reply multicast link info
        }
        z_keyexpr_drop(z_keyexpr_move(&ke2));

        curr = _z_transport_peer_multicast_slist_next(curr);
    }                                            
}

static void _ze_admin_space_reply_multicast(z_loaned_query_t *query, const z_id_t *zid,
                                        _z_transport_multicast_t *multicast) {
    _ze_admin_space_reply_multicast_common(query, zid, multicast, _Z_KEYEXPR_TRANSPORT_MULTICAST);
}

static void _ze_admin_space_reply_raweth(z_loaned_query_t *query, const z_id_t *zid,
                                        _z_transport_multicast_t *raweth) {
    _ze_admin_space_reply_multicast_common(query, zid, raweth, _Z_KEYEXPR_TRANSPORT_RAWETH);
}

static void _ze_admin_space_query_handler(z_loaned_query_t *query, void *ctx) {
    _z_session_weak_t *session_weak = (_z_session_weak_t *)ctx;
    
    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(session_weak);
    if (_Z_RC_IS_NULL(&session_rc)) {
        _Z_ERROR("Dropped admin space query - session closed");
        return;
    }

    z_id_t zid = z_info_zid(&session_rc);
    _z_transport_t *tp = &_Z_RC_IN_VAL(&session_rc)->_tp;

    switch(tp->_type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _ze_admin_space_reply_unicast(query, &zid, &tp->_transport._unicast);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            _ze_admin_space_reply_multicast(query, &zid, &tp->_transport._multicast);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            _ze_admin_space_reply_raweth(query, &zid, &tp->_transport._raweth);
            break;
        default:
            break;
    }
    _z_session_rc_drop(&session_rc);
}

static void _ze_admin_space_query_dropper(void *ctx) {
    _z_session_weak_t *session_weak = (_z_session_weak_t *)ctx;
    _z_session_weak_drop(session_weak);
    z_free(session_weak);
}

z_result_t ze_start_admin_space(z_loaned_session_t *zs) {
    if (_Z_RC_IN_VAL(zs)->_admin_space_queryable_id != 0) {
        // Already started
        return _Z_RES_OK;
    }

    _z_session_weak_t *session_weak = _z_session_rc_clone_as_weak_ptr(zs);
    if (_Z_RC_IS_NULL(session_weak)) {
        _Z_ERROR_RETURN(_Z_ERR_SYSTEM_OUT_OF_MEMORY);
    }
    z_id_t zid = z_info_zid(zs);

    z_owned_keyexpr_t ke;
    _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_queryable_ke(&ke, &zid), _z_session_weak_drop(session_weak); z_free(session_weak));

    z_owned_closure_query_t callback;
    _Z_CLEAN_RETURN_IF_ERR(
        z_closure_query(&callback, _ze_admin_space_query_handler, _ze_admin_space_query_dropper, session_weak),
        z_keyexpr_drop(z_keyexpr_move(&ke));
        _z_session_weak_drop(session_weak); z_free(session_weak));

    z_owned_queryable_t admin_space_queryable;
    _Z_CLEAN_RETURN_IF_ERR(
        z_declare_queryable(zs, &admin_space_queryable, z_keyexpr_loan(&ke), z_closure_query_move(&callback), NULL),
        z_keyexpr_drop(z_keyexpr_move(&ke)));
    z_keyexpr_drop(z_keyexpr_move(&ke));

    _Z_RC_IN_VAL(zs)->_admin_space_queryable_id = admin_space_queryable._val._entity_id;
    _z_queryable_clear(&admin_space_queryable._val);

    return _Z_RES_OK;
}

z_result_t ze_stop_admin_space(z_loaned_session_t *zs) {
    uint32_t admin_space_queryable_id = _Z_RC_IN_VAL(zs)->_admin_space_queryable_id;
    if (admin_space_queryable_id == 0) {
        // Already stopped
        return _Z_RES_OK;
    }

    _z_queryable_t admin_space_queryable = {
        ._entity_id = admin_space_queryable_id,
        ._zn = _z_session_rc_clone_as_weak(zs),
    };

    z_result_t ret = _z_undeclare_queryable(&admin_space_queryable);
    _z_queryable_clear(&admin_space_queryable);
    if (ret == _Z_RES_OK) {
        _Z_RC_IN_VAL(zs)->_admin_space_queryable_id = 0;
    }
    return ret;
}

#endif  // Z_FEATURE_ADMIN_SPACE == 1
