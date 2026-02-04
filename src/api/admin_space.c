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
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/json_encoder.h"

#if Z_FEATURE_ADMIN_SPACE == 1

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_STARSTAR
static z_result_t _ze_admin_space_queryable_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    z_internal_keyexpr_null(ke);

    z_owned_string_t zid_str;
    _Z_RETURN_IF_ERR(z_id_to_string(zid, &zid_str));

    char buf[128];
    int n = snprintf(buf, sizeof(buf), "%s%s%.*s%s%s%s%s%s%s", _Z_KEYEXPR_AT, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&zid_str)), z_string_data(z_string_loan(&zid_str)),
                     _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_PICO, _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_SESSION,
                     _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_STARSTAR);

    z_string_drop(z_string_move(&zid_str));

    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return _Z_ERR_INVALID;
    }

    return z_keyexpr_from_str(ke, buf);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORT_*
static z_result_t _ze_admin_space_transport_ke(z_owned_keyexpr_t *ke, const z_id_t *zid, const char *transport) {
    z_internal_keyexpr_null(ke);

    z_owned_string_t zid_str;
    _Z_RETURN_IF_ERR(z_id_to_string(zid, &zid_str));

    char buf[128];
    int n = snprintf(buf, sizeof(buf), "%s%s%.*s%s%s%s%s%s%s", _Z_KEYEXPR_AT, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&zid_str)), z_string_data(z_string_loan(&zid_str)),
                     _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_PICO, _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_SESSION,
                     _Z_KEYEXPR_SEPARATOR, transport);

    z_string_drop(z_string_move(&zid_str));

    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return _Z_ERR_INVALID;
    }

    return z_keyexpr_from_str(ke, buf);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORT_* / _Z_KEYEXPR_LINK / LID
static z_result_t _ze_admin_space_transport_link_ke(z_owned_keyexpr_t *ke, const z_id_t *zid, const char *transport,
                                                    const z_id_t *lid) {
    z_internal_keyexpr_null(ke);

    z_owned_string_t zid_str;
    _Z_RETURN_IF_ERR(z_id_to_string(zid, &zid_str));
    z_owned_string_t lid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(lid, &lid_str), z_string_drop(z_string_move(&zid_str)));

    char buf[128];
    int n = snprintf(buf, sizeof(buf), "%s%s%.*s%s%s%s%s%s%s%s%s%s%.*s", _Z_KEYEXPR_AT, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&zid_str)), z_string_data(z_string_loan(&zid_str)),
                     _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_PICO, _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_SESSION,
                     _Z_KEYEXPR_SEPARATOR, transport, _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_LINK, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&lid_str)), z_string_data(z_string_loan(&lid_str)));

    z_string_drop(z_string_move(&zid_str));
    z_string_drop(z_string_move(&lid_str));

    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return _Z_ERR_INVALID;
    }

    return z_keyexpr_from_str(ke, buf);
}

static z_result_t _ze_admin_space_encode_whatami(_z_json_encoder_t *je, z_whatami_t mode) {
    switch (mode) {
        case Z_WHATAMI_ROUTER:
            return _z_json_encoder_write_string(je, "router");
        case Z_WHATAMI_PEER:
            return _z_json_encoder_write_string(je, "peer");
        case Z_WHATAMI_CLIENT:
            return _z_json_encoder_write_string(je, "client");
        default:
            return _Z_ERR_INVALID;
    }
}

static z_result_t _ze_admin_space_encode_str_intmap(_z_json_encoder_t *je, const _z_str_intmap_t *map) {
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));

    _z_str_intmap_iterator_t it = _z_str_intmap_iterator_make(map);
    while (_z_str_intmap_iterator_next(&it)) {
        size_t key = _z_str_intmap_iterator_key(&it);
        char key_buf[21];  // enough for 64-bit size_t
        int n = snprintf(key_buf, sizeof(key_buf), "%zu", key);
        if (n <= 0 || (size_t)n >= sizeof(key_buf)) {
            return _Z_ERR_INVALID;
        }
        _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, key_buf));

        char *value = _z_str_intmap_iterator_value(&it);
        _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, value));
    }
    return _z_json_encoder_end_object(je);
}

static z_result_t _ze_admin_space_encode_link(_z_json_encoder_t *je, const _z_link_t *link) {
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "link"));
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "type"));
    switch (link->_type) {
        case _Z_LINK_TYPE_TCP:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "tcp"));
            break;
        case _Z_LINK_TYPE_UDP:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "udp"));
            break;
        case _Z_LINK_TYPE_BT:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "bt"));
            break;
        case _Z_LINK_TYPE_SERIAL:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "serial"));
            break;
        case _Z_LINK_TYPE_WS:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "ws"));
            break;
        case _Z_LINK_TYPE_TLS:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "tls"));
            break;
        case _Z_LINK_TYPE_RAWETH:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "raweth"));
            break;
        default:
            return _Z_ERR_INVALID;
    }

    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "endpoint"));
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "locator"));
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "metadata"));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_str_intmap(je, &link->_endpoint._locator._metadata));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "protocol"));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_z_string(je, &link->_endpoint._locator._protocol));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "address"));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_z_string(je, &link->_endpoint._locator._address));
    _Z_RETURN_IF_ERR(_z_json_encoder_end_object(je));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "config"));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_str_intmap(je, &link->_endpoint._config));
    _Z_RETURN_IF_ERR(_z_json_encoder_end_object(je));

    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "capabilities"));
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "transport"));
    switch (link->_cap._transport) {
        case Z_LINK_CAP_TRANSPORT_UNICAST:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "unicast"));
            break;
        case Z_LINK_CAP_TRANSPORT_MULTICAST:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "multicast"));
            break;
        case Z_LINK_CAP_TRANSPORT_RAWETH:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "raweth"));
            break;
        default:
            return _Z_ERR_INVALID;
    }
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "flow"));
    switch (link->_cap._flow) {
        case Z_LINK_CAP_FLOW_DATAGRAM:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "datagram"));
            break;
        case Z_LINK_CAP_FLOW_STREAM:
            _Z_RETURN_IF_ERR(_z_json_encoder_write_string(je, "stream"));
            break;
        default:
            return _Z_ERR_INVALID;
    }
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "is_reliable"));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_boolean(je, link->_cap._is_reliable != 0));
    _Z_RETURN_IF_ERR(_z_json_encoder_end_object(je));
    return _z_json_encoder_end_object(je);
}

static z_result_t _ze_admin_space_encode_transport_common(_z_json_encoder_t *je, const _z_transport_common_t *common,
                                                          z_whatami_t mode) {
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "mode"));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_whatami(je, mode));
    return _ze_admin_space_encode_link(je, common->_link);
}

static z_result_t _ze_admin_space_encode_peer_common(_z_json_encoder_t *je, const _z_transport_peer_common_t *peer) {
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "zid"));
    z_owned_string_t id_string;
    _Z_RETURN_IF_ERR(z_id_to_string(&peer->_remote_zid, &id_string));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(je, &id_string._val),
                           z_string_drop(z_string_move(&id_string)));
    z_string_drop(z_string_move(&id_string));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "whatami"));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_whatami(je, peer->_remote_whatami));
    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_encode_peer_multicast(_z_json_encoder_t *je,
                                                        const _z_transport_peer_multicast_t *peer) {
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_peer_common(je, &peer->common));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "remote_addr"));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_z_slice(je, &peer->_remote_addr));
    return _Z_RES_OK;
}

static void _ze_admin_space_reply_null(_ze_admin_space_reply_t *reply) {
    z_internal_keyexpr_null(&reply->ke);
    z_internal_bytes_null(&reply->payload);
}

void _ze_admin_space_reply_clear(_ze_admin_space_reply_t *reply) {
    z_keyexpr_drop(z_keyexpr_move(&reply->ke));
    z_bytes_drop(z_bytes_move(&reply->payload));
    _ze_admin_space_reply_null(reply);
}

static z_result_t _ze_admin_space_add_reply(_z_json_encoder_t *je, const z_loaned_keyexpr_t *ke,
                                            _ze_admin_space_reply_list_t **replies) {
    _ze_admin_space_reply_t *reply = z_malloc(sizeof(_ze_admin_space_reply_t));
    if (reply == NULL) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _ze_admin_space_reply_null(reply);

    _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_clone(&reply->ke, ke), z_free(reply));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_finish(je, &reply->payload), _ze_admin_space_reply_clear(reply);
                           z_free(reply));

    _ze_admin_space_reply_list_t *old = *replies;
    _ze_admin_space_reply_list_t *tmp = _ze_admin_space_reply_list_push(*replies, reply);
    if (tmp == old) {
        _ze_admin_space_reply_clear(reply);
        z_free(reply);
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    *replies = tmp;

    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_encode_unicast(const z_loaned_query_t *query, const _z_transport_unicast_t *unicast,
                                                 const z_id_t *zid, z_whatami_t mode,
                                                 _ze_admin_space_reply_list_t **replies) {
    z_owned_keyexpr_t ke;
    _Z_RETURN_IF_ERR(_ze_admin_space_transport_ke(&ke, zid, _Z_KEYEXPR_TRANSPORT_UNICAST));
    if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke))) {
        _z_json_encoder_t je;
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_empty(&je), z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(&je), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_encode_transport_common(&je, &unicast->_common, mode),
                               _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(&je), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_add_reply(&je, z_keyexpr_loan(&ke), replies), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
    }
    z_keyexpr_drop(z_keyexpr_move(&ke));
    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_encode_unicast_peer(const z_loaned_query_t *query,
                                                      const _z_transport_peer_unicast_t *peer, const z_id_t *zid,
                                                      _ze_admin_space_reply_list_t **replies) {
    z_owned_keyexpr_t ke;
    _Z_RETURN_IF_ERR(
        _ze_admin_space_transport_link_ke(&ke, zid, _Z_KEYEXPR_TRANSPORT_UNICAST, &peer->common._remote_zid));
    if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke))) {
        _z_json_encoder_t je;
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_empty(&je), z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(&je), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_encode_peer_common(&je, &peer->common), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(&je), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_add_reply(&je, z_keyexpr_loan(&ke), replies), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
    }
    z_keyexpr_drop(z_keyexpr_move(&ke));

    return _Z_RES_OK;
}

static void _ze_admin_space_query_handle_unicast(const z_loaned_query_t *query, _z_session_t *session,
                                                 _ze_admin_space_reply_list_t **replies) {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_transport_peer_mutex_lock(&session->_tp._transport._unicast._common);
#endif

    z_result_t res = _ze_admin_space_encode_unicast(query, &session->_tp._transport._unicast, &session->_local_zid,
                                                    session->_mode, replies);
    if (res != _Z_RES_OK) {
        _Z_ERROR("Failed to reply to admin space %s query: %d", _Z_KEYEXPR_TRANSPORT_UNICAST, res);
    }

    const _z_transport_peer_unicast_slist_t *peers = session->_tp._transport._unicast._peers;
    while (peers != NULL) {
        const _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(peers);
        res = _ze_admin_space_encode_unicast_peer(query, peer, &session->_local_zid, replies);

        if (res != _Z_RES_OK) {
            z_owned_string_t zid_str;
            if (z_id_to_string(&peer->common._remote_zid, &zid_str) == _Z_RES_OK) {
                _Z_ERROR("Failed to reply to admin space %s link query (peer zid: %.*s): %d",
                         _Z_KEYEXPR_TRANSPORT_UNICAST, (int)z_string_len(z_string_loan(&zid_str)),
                         z_string_data(z_string_loan(&zid_str)), res);
                z_string_drop(z_string_move(&zid_str));
            } else {
                _Z_ERROR("Failed to reply to admin space %s link query: %d", _Z_KEYEXPR_TRANSPORT_UNICAST, res);
            }
        }

        peers = _z_transport_peer_unicast_slist_next(peers);
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_transport_peer_mutex_unlock(&session->_tp._transport._unicast._common);
#endif
}

static z_result_t _ze_admin_space_encode_multicast_common(const z_loaned_query_t *query,
                                                          const _z_transport_multicast_t *multicast, const z_id_t *zid,
                                                          z_whatami_t mode, const char *transport_keyexpr,
                                                          _ze_admin_space_reply_list_t **replies) {
    z_owned_keyexpr_t ke;
    _Z_RETURN_IF_ERR(_ze_admin_space_transport_ke(&ke, zid, transport_keyexpr));
    if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke))) {
        _z_json_encoder_t je;
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_empty(&je), z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(&je), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_encode_transport_common(&je, &multicast->_common, mode),
                               _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(&je), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_add_reply(&je, z_keyexpr_loan(&ke), replies), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
    }
    z_keyexpr_drop(z_keyexpr_move(&ke));
    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_encode_multicast_peer(const z_loaned_query_t *query,
                                                        const _z_transport_peer_multicast_t *peer, const z_id_t *zid,
                                                        const char *transport_keyexpr,
                                                        _ze_admin_space_reply_list_t **replies) {
    z_owned_keyexpr_t ke;
    _Z_RETURN_IF_ERR(_ze_admin_space_transport_link_ke(&ke, zid, transport_keyexpr, &peer->common._remote_zid));
    if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke))) {
        _z_json_encoder_t je;
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_empty(&je), z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(&je), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_encode_peer_multicast(&je, peer), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(&je), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_add_reply(&je, z_keyexpr_loan(&ke), replies), _z_json_encoder_clear(&je);
                               z_keyexpr_drop(z_keyexpr_move(&ke)));
    }
    z_keyexpr_drop(z_keyexpr_move(&ke));

    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_query_handle_multicast_common(const z_loaned_query_t *query, _z_session_t *session,
                                                                _z_transport_multicast_t *transport,
                                                                const char *transport_keyexpr,
                                                                _ze_admin_space_reply_list_t **replies) {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_transport_peer_mutex_lock(&transport->_common);
#endif

    z_result_t res = _ze_admin_space_encode_multicast_common(query, transport, &session->_local_zid, session->_mode,
                                                             transport_keyexpr, replies);
    if (res != _Z_RES_OK) {
        _Z_ERROR("Failed to reply to admin space %s query: %d", transport_keyexpr, res);
    }

    const _z_transport_peer_multicast_slist_t *peers = transport->_peers;
    while (peers != NULL) {
        const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(peers);
        res = _ze_admin_space_encode_multicast_peer(query, peer, &session->_local_zid, transport_keyexpr, replies);

        if (res != _Z_RES_OK) {
            z_owned_string_t zid_str;
            if (z_id_to_string(&peer->common._remote_zid, &zid_str) == _Z_RES_OK) {
                _Z_ERROR("Failed to reply to admin space %s link query (peer zid: %.*s): %d", transport_keyexpr,
                         (int)z_string_len(z_string_loan(&zid_str)), z_string_data(z_string_loan(&zid_str)), res);
                z_string_drop(z_string_move(&zid_str));
            } else {
                _Z_ERROR("Failed to reply to admin space %s link query: %d", transport_keyexpr, res);
            }
        }

        peers = _z_transport_peer_multicast_slist_next(peers);
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_transport_peer_mutex_unlock(&transport->_common);
#endif
    return _Z_RES_OK;
}

static void _ze_admin_space_query_handler(z_loaned_query_t *query, void *ctx) {
    _z_session_weak_t *session_weak = (_z_session_weak_t *)ctx;

    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(session_weak);
    if (_Z_RC_IS_NULL(&session_rc)) {
        _Z_ERROR("Dropped admin space query - session closed");
        return;
    }

    _z_session_t *session = _Z_RC_IN_VAL(&session_rc);

#if Z_FEATURE_MULTI_THREAD == 1
    _z_session_mutex_lock(session);
#endif

    _ze_admin_space_reply_list_t *replies = _ze_admin_space_reply_list_new();

    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _ze_admin_space_query_handle_unicast(query, session, &replies);
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            _ze_admin_space_query_handle_multicast_common(query, session, &session->_tp._transport._multicast,
                                                          _Z_KEYEXPR_TRANSPORT_MULTICAST, &replies);
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            _ze_admin_space_query_handle_multicast_common(query, session, &session->_tp._transport._raweth,
                                                          _Z_KEYEXPR_TRANSPORT_RAWETH, &replies);
            break;
        default:
            break;
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_session_mutex_unlock(session);
#endif

    _ze_admin_space_reply_list_t *next = replies;
    while (next != NULL) {
        _ze_admin_space_reply_t *reply = _ze_admin_space_reply_list_value(next);
        z_result_t res = z_query_reply(query, z_keyexpr_loan(&reply->ke), z_bytes_move(&reply->payload), NULL);
        if (res != _Z_RES_OK) {
            z_view_string_t keystr;
            if (z_keyexpr_as_view_string(z_keyexpr_loan(&reply->ke), &keystr) == _Z_RES_OK) {
                _Z_ERROR("Failed to reply to admin space query on key expression: %.*s",
                         (int)z_string_len(z_view_string_loan(&keystr)), z_string_data(z_view_string_loan(&keystr)));
            } else {
                _Z_ERROR("Failed to reply to admin space query");
            }
        }
        next = _ze_admin_space_reply_list_next(next);
    }
    _ze_admin_space_reply_list_free(&replies);
    _z_session_rc_drop(&session_rc);
}

static void _ze_admin_space_query_dropper(void *ctx) {
    _z_session_weak_t *session_weak = (_z_session_weak_t *)ctx;
    _z_session_weak_drop(session_weak);
    z_free(session_weak);
}

z_result_t zp_start_admin_space(z_loaned_session_t *zs) {
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
    _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_queryable_ke(&ke, &zid), _z_session_weak_drop(session_weak);
                           z_free(session_weak));

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

z_result_t zp_stop_admin_space(z_loaned_session_t *zs) {
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
