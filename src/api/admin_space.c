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

#include "zenoh-pico/api/encoding.h"
#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/net/primitives.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/utils/json_encoder.h"

#if Z_FEATURE_ADMIN_SPACE == 1

#define _ZE_ADMIN_SPACE_KE_BUF_LEN 128

typedef struct {
    const char *data;
    size_t len;
} _ze_admin_space_ke_segment_t;

static z_result_t _ze_admin_space_build_ke(z_owned_keyexpr_t *ke, const z_id_t *zid,
                                           const _ze_admin_space_ke_segment_t *segments, size_t segment_count) {
    z_internal_keyexpr_null(ke);

    z_owned_string_t zid_str;
    _Z_RETURN_IF_ERR(z_id_to_string(zid, &zid_str));

    const z_loaned_string_t *zid_loan = z_string_loan(&zid_str);
    const char *zid_data = z_string_data(zid_loan);
    size_t zid_len = z_string_len(zid_loan);

    char buf[_ZE_ADMIN_SPACE_KE_BUF_LEN];
    size_t off = 0;

    int n =
        snprintf(buf + off, sizeof(buf) - off, "%s%s%.*s", _Z_KEYEXPR_AT, _Z_KEYEXPR_SEPARATOR, (int)zid_len, zid_data);
    z_string_drop(z_string_move(&zid_str));
    if (n < 0 || (size_t)n >= sizeof(buf) - off) {
        return _Z_ERR_INVALID;
    }
    off += (size_t)n;

    for (size_t i = 0; i < segment_count; i++) {
        n = snprintf(buf + off, sizeof(buf) - off, "%s%.*s", _Z_KEYEXPR_SEPARATOR, (int)segments[i].len,
                     segments[i].data);
        if (n < 0 || (size_t)n >= sizeof(buf) - off) {
            return _Z_ERR_INVALID;
        }
        off += (size_t)n;
    }

    return z_keyexpr_from_substr(ke, buf, off);
}

// ke = _Z_KEYEXPR_AT / ZID / Z_KEYEXPR_STARSTAR
static z_result_t _ze_admin_space_queryable_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    static const _ze_admin_space_ke_segment_t segments[] = {
        {_Z_KEYEXPR_STARSTAR, _Z_KEYEXPR_STARSTAR_LEN},
    };
    return _ze_admin_space_build_ke(ke, zid, segments, 1);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO
static z_result_t _ze_admin_space_pico_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    static const _ze_admin_space_ke_segment_t segments[] = {
        {_Z_KEYEXPR_PICO, _Z_KEYEXPR_PICO_LEN},
    };
    return _ze_admin_space_build_ke(ke, zid, segments, 1);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION
static z_result_t _ze_admin_space_pico_session_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    static const _ze_admin_space_ke_segment_t segments[] = {
        {_Z_KEYEXPR_PICO, _Z_KEYEXPR_PICO_LEN},
        {_Z_KEYEXPR_SESSION, _Z_KEYEXPR_SESSION_LEN},
    };
    return _ze_admin_space_build_ke(ke, zid, segments, 2);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORTS
static z_result_t _ze_admin_space_pico_transports_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    static const _ze_admin_space_ke_segment_t segments[] = {
        {_Z_KEYEXPR_PICO, _Z_KEYEXPR_PICO_LEN},
        {_Z_KEYEXPR_SESSION, _Z_KEYEXPR_SESSION_LEN},
        {_Z_KEYEXPR_TRANSPORTS, _Z_KEYEXPR_TRANSPORTS_LEN},
    };
    return _ze_admin_space_build_ke(ke, zid, segments, 3);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORTS / 0
static z_result_t _ze_admin_space_pico_transports_0_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    static const _ze_admin_space_ke_segment_t segments[] = {
        {_Z_KEYEXPR_PICO, _Z_KEYEXPR_PICO_LEN},
        {_Z_KEYEXPR_SESSION, _Z_KEYEXPR_SESSION_LEN},
        {_Z_KEYEXPR_TRANSPORTS, _Z_KEYEXPR_TRANSPORTS_LEN},
        {"0", 1},
    };
    return _ze_admin_space_build_ke(ke, zid, segments, 4);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORTS / 0 / _Z_KEYEXPR_PEERS
static z_result_t _ze_admin_space_pico_transports_0_peers_ke(z_owned_keyexpr_t *ke, const z_id_t *zid) {
    static const _ze_admin_space_ke_segment_t segments[] = {
        {_Z_KEYEXPR_PICO, _Z_KEYEXPR_PICO_LEN},
        {_Z_KEYEXPR_SESSION, _Z_KEYEXPR_SESSION_LEN},
        {_Z_KEYEXPR_TRANSPORTS, _Z_KEYEXPR_TRANSPORTS_LEN},
        {"0", 1},
        {_Z_KEYEXPR_PEERS, _Z_KEYEXPR_PEERS_LEN},
    };
    return _ze_admin_space_build_ke(ke, zid, segments, 5);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORTS / 0 / _Z_KEYEXPR_PEERS /
// REMOTE_ZID
static z_result_t _ze_admin_space_pico_transports_0_peers_peer_ke(z_owned_keyexpr_t *ke, const z_id_t *zid,
                                                                  const z_id_t *remote_zid) {
    z_owned_string_t remote_zid_str;
    _Z_RETURN_IF_ERR(z_id_to_string(remote_zid, &remote_zid_str));

    const z_loaned_string_t *remote_zid_loan = z_string_loan(&remote_zid_str);
    const _ze_admin_space_ke_segment_t segments[] = {
        {_Z_KEYEXPR_PICO, _Z_KEYEXPR_PICO_LEN},
        {_Z_KEYEXPR_SESSION, _Z_KEYEXPR_SESSION_LEN},
        {_Z_KEYEXPR_TRANSPORTS, _Z_KEYEXPR_TRANSPORTS_LEN},
        {"0", 1},
        {_Z_KEYEXPR_PEERS, _Z_KEYEXPR_PEERS_LEN},
        {z_string_data(remote_zid_loan), z_string_len(remote_zid_loan)},
    };

    z_result_t ret = _ze_admin_space_build_ke(ke, zid, segments, 6);
    z_string_drop(z_string_move(&remote_zid_str));
    return ret;
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

static z_result_t _ze_admin_space_encode_transport_type(_z_json_encoder_t *je, int type) {
    switch (type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            return _z_json_encoder_write_string(je, "unicast");
        case _Z_TRANSPORT_MULTICAST_TYPE:
            return _z_json_encoder_write_string(je, "multicast");
        case _Z_TRANSPORT_RAWETH_TYPE:
            return _z_json_encoder_write_string(je, "raweth");
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

static z_result_t _ze_admin_space_encode_transport_common(_z_json_encoder_t *je, const _z_transport_common_t *common) {
    return _ze_admin_space_encode_link(je, common->_link);
}

static z_result_t _ze_admin_space_encode_peer_common(_z_json_encoder_t *je, const _z_transport_peer_common_t *peer) {
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "zid"));
    z_owned_string_t id_string;
    _Z_RETURN_IF_ERR(z_id_to_string(&peer->_remote_zid, &id_string));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(je, z_string_loan(&id_string)),
                           z_string_drop(z_string_move(&id_string)));
    z_string_drop(z_string_move(&id_string));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "whatami"));
    return _ze_admin_space_encode_whatami(je, peer->_remote_whatami);
}

static z_result_t _ze_admin_space_encode_unicast_peer(_z_json_encoder_t *je, const _z_transport_peer_unicast_t *peer) {
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_peer_common(je, &peer->common));
    return _z_json_encoder_end_object(je);
}

static z_result_t _ze_admin_space_encode_unicast_peers(_z_json_encoder_t *je,
                                                       const _z_transport_peer_unicast_slist_t *peers) {
    _Z_RETURN_IF_ERR(_z_json_encoder_start_array(je));

    for (; peers != NULL; peers = _z_transport_peer_unicast_slist_next(peers)) {
        const _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(peers);
        _Z_RETURN_IF_ERR(_ze_admin_space_encode_unicast_peer(je, peer));
    }

    return _z_json_encoder_end_array(je);
}

static z_result_t _ze_admin_space_encode_multicast_peer(_z_json_encoder_t *je,
                                                        const _z_transport_peer_multicast_t *peer) {
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_peer_common(je, &peer->common));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "remote_addr"));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_z_slice(je, &peer->_remote_addr));
    return _z_json_encoder_end_object(je);
}

static z_result_t _ze_admin_space_encode_multicast_peers(_z_json_encoder_t *je,
                                                         const _z_transport_peer_multicast_slist_t *peers) {
    _Z_RETURN_IF_ERR(_z_json_encoder_start_array(je));

    for (; peers != NULL; peers = _z_transport_peer_multicast_slist_next(peers)) {
        const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(peers);
        _Z_RETURN_IF_ERR(_ze_admin_space_encode_multicast_peer(je, peer));
    }

    return _z_json_encoder_end_array(je);
}

static z_result_t _ze_admin_space_encode_transport_unicast(_z_json_encoder_t *je, _z_transport_unicast_t *tp) {
    z_result_t ret = _Z_RES_OK;

    _z_transport_peer_mutex_lock(&tp->_common);

    ret = _ze_admin_space_encode_transport_common(je, &tp->_common);
    if (ret == _Z_RES_OK) {
        ret = _z_json_encoder_write_key(je, "peers");
    }
    if (ret == _Z_RES_OK) {
        ret = _ze_admin_space_encode_unicast_peers(je, tp->_peers);
    }

    _z_transport_peer_mutex_unlock(&tp->_common);
    return ret;
}

static z_result_t _ze_admin_space_encode_transport_multicast(_z_json_encoder_t *je, _z_transport_multicast_t *tp) {
    z_result_t ret = _Z_RES_OK;

    _z_transport_peer_mutex_lock(&tp->_common);

    ret = _ze_admin_space_encode_transport_common(je, &tp->_common);
    if (ret == _Z_RES_OK) {
        ret = _z_json_encoder_write_key(je, "peers");
    }
    if (ret == _Z_RES_OK) {
        ret = _ze_admin_space_encode_multicast_peers(je, tp->_peers);
    }

    _z_transport_peer_mutex_unlock(&tp->_common);
    return ret;
}

static z_result_t _ze_admin_space_encode_transport(_z_json_encoder_t *je, _z_transport_t *tp) {
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "type"));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_transport_type(je, tp->_type));

    switch (tp->_type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _Z_RETURN_IF_ERR(_ze_admin_space_encode_transport_unicast(je, &tp->_transport._unicast));
            break;
        case _Z_TRANSPORT_MULTICAST_TYPE:
            _Z_RETURN_IF_ERR(_ze_admin_space_encode_transport_multicast(je, &tp->_transport._multicast));
            break;
        case _Z_TRANSPORT_RAWETH_TYPE:
            _Z_RETURN_IF_ERR(_ze_admin_space_encode_transport_multicast(je, &tp->_transport._raweth));
            break;
        default:
            break;
    }

    return _z_json_encoder_end_object(je);
}

static z_result_t _ze_admin_space_encode_transport_peers(_z_json_encoder_t *je, _z_transport_t *tp) {
    z_result_t ret = _Z_RES_OK;

    switch (tp->_type) {
        case _Z_TRANSPORT_UNICAST_TYPE: {
            _z_transport_unicast_t *utp = &tp->_transport._unicast;
            _z_transport_peer_mutex_lock(&utp->_common);
            ret = _ze_admin_space_encode_unicast_peers(je, utp->_peers);
            _z_transport_peer_mutex_unlock(&utp->_common);
            break;
        }
        case _Z_TRANSPORT_MULTICAST_TYPE: {
            _z_transport_multicast_t *mtp = &tp->_transport._multicast;
            _z_transport_peer_mutex_lock(&mtp->_common);
            ret = _ze_admin_space_encode_multicast_peers(je, mtp->_peers);
            _z_transport_peer_mutex_unlock(&mtp->_common);
            break;
        }
        case _Z_TRANSPORT_RAWETH_TYPE: {
            _z_transport_multicast_t *rtp = &tp->_transport._raweth;
            _z_transport_peer_mutex_lock(&rtp->_common);
            ret = _ze_admin_space_encode_multicast_peers(je, rtp->_peers);
            _z_transport_peer_mutex_unlock(&rtp->_common);
            break;
        }
        default:
            break;
    }

    return ret;
}

static z_result_t _ze_admin_space_encode_transports(_z_json_encoder_t *je, _z_transport_t *tp) {
    _Z_RETURN_IF_ERR(_z_json_encoder_start_array(je));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_transport(je, tp));
    return _z_json_encoder_end_array(je);
}

static z_result_t _ze_admin_space_encode_session(_z_json_encoder_t *je, _z_session_t *session) {
    _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(session));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(je), _z_session_mutex_unlock(session));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(je, "zid"), _z_session_mutex_unlock(session));
    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(&session->_local_zid, &zid_str), _z_session_mutex_unlock(session));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(je, z_string_loan(&zid_str)),
                           z_string_drop(z_string_move(&zid_str));
                           _z_session_mutex_unlock(session));
    z_string_drop(z_string_move(&zid_str));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(je, "whatami"), _z_session_mutex_unlock(session));
    _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_encode_whatami(je, session->_mode), _z_session_mutex_unlock(session));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(je, "transports"), _z_session_mutex_unlock(session));
    _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_encode_transports(je, &session->_tp), _z_session_mutex_unlock(session));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(je), _z_session_mutex_unlock(session));

    _z_session_mutex_unlock(session);
    return _Z_RES_OK;
}

typedef z_result_t (*_ze_admin_space_encode_fn_t)(_z_json_encoder_t *je, void *ctx);

static z_result_t _ze_admin_space_reply_if_intersects(const z_loaned_query_t *query, const z_loaned_keyexpr_t *ke,
                                                      void *ctx, _ze_admin_space_reply_list_t **replies,
                                                      _ze_admin_space_encode_fn_t encode) {
    if (z_keyexpr_intersects(z_query_keyexpr(query), ke)) {
        _z_json_encoder_t je;
        _Z_RETURN_IF_ERR(_z_json_encoder_empty(&je));
        _Z_CLEAN_RETURN_IF_ERR(encode(&je, ctx), _z_json_encoder_clear(&je));
        _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_add_reply(&je, ke, replies), _z_json_encoder_clear(&je));
    }
    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_encode_pico(_z_json_encoder_t *je, void *ctx) {
    _z_session_t *session = (_z_session_t *)ctx;
    _Z_RETURN_IF_ERR(_z_json_encoder_start_object(je));
    _Z_RETURN_IF_ERR(_z_json_encoder_write_key(je, "session"));
    _Z_RETURN_IF_ERR(_ze_admin_space_encode_session(je, session));
    return _z_json_encoder_end_object(je);
}

static z_result_t _ze_admin_space_encode_pico_session(_z_json_encoder_t *je, void *ctx) {
    _z_session_t *session = (_z_session_t *)ctx;
    return _ze_admin_space_encode_session(je, session);
}

static z_result_t _ze_admin_space_encode_pico_transports(_z_json_encoder_t *je, void *ctx) {
    _z_session_t *session = (_z_session_t *)ctx;
    _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(session));
    z_result_t ret = _ze_admin_space_encode_transports(je, &session->_tp);
    _z_session_mutex_unlock(session);
    return ret;
}

static z_result_t _ze_admin_space_encode_pico_transport_0(_z_json_encoder_t *je, void *ctx) {
    _z_session_t *session = (_z_session_t *)ctx;
    _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(session));
    z_result_t ret = _ze_admin_space_encode_transport(je, &session->_tp);
    _z_session_mutex_unlock(session);
    return ret;
}

static z_result_t _ze_admin_space_encode_pico_transport_0_peers(_z_json_encoder_t *je, void *ctx) {
    _z_session_t *session = (_z_session_t *)ctx;
    _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(session));
    z_result_t ret = _ze_admin_space_encode_transport_peers(je, &session->_tp);
    _z_session_mutex_unlock(session);
    return ret;
}

typedef struct {
    const _z_transport_peer_unicast_t *peer;
} _ze_admin_space_unicast_peer_ctx_t;

typedef struct {
    const _z_transport_peer_multicast_t *peer;
} _ze_admin_space_multicast_peer_ctx_t;

static z_result_t _ze_admin_space_encode_pico_transport_0_unicast_peer(_z_json_encoder_t *je, void *ctx) {
    const _ze_admin_space_unicast_peer_ctx_t *pctx = (const _ze_admin_space_unicast_peer_ctx_t *)ctx;
    return _ze_admin_space_encode_unicast_peer(je, pctx->peer);
}

static z_result_t _ze_admin_space_encode_pico_transport_0_multicast_peer(_z_json_encoder_t *je, void *ctx) {
    const _ze_admin_space_multicast_peer_ctx_t *pctx = (const _ze_admin_space_multicast_peer_ctx_t *)ctx;
    return _ze_admin_space_encode_multicast_peer(je, pctx->peer);
}

static void _ze_admin_space_query_handle_pico_transport_0_unicast_peers(const z_loaned_query_t *query,
                                                                        const z_id_t *local_zid,
                                                                        _z_transport_unicast_t *tp,
                                                                        _ze_admin_space_reply_list_t **replies) {
    _z_transport_peer_mutex_lock(&tp->_common);

    for (_z_transport_peer_unicast_slist_t *peers = tp->_peers; peers != NULL;
         peers = _z_transport_peer_unicast_slist_next(peers)) {
        const _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(peers);

        z_owned_keyexpr_t ke;
        z_result_t ret;

        ret = _ze_admin_space_pico_transports_0_peers_peer_ke(&ke, local_zid, &peer->common._remote_zid);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to build key expression for pico/session/transports/0/peers peer query: %d", ret);
            continue;
        }

        _ze_admin_space_unicast_peer_ctx_t ctx = {
            .peer = peer,
        };

        ret = _ze_admin_space_reply_if_intersects(query, z_keyexpr_loan(&ke), &ctx, replies,
                                                  _ze_admin_space_encode_pico_transport_0_unicast_peer);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to handle admin space query for pico/session/transports/0/peers peer endpoint: %d", ret);
        }

        z_keyexpr_drop(z_keyexpr_move(&ke));
    }

    _z_transport_peer_mutex_unlock(&tp->_common);
}

static void _ze_admin_space_query_handle_pico_transport_0_multicast_peers(const z_loaned_query_t *query,
                                                                          const z_id_t *local_zid,
                                                                          _z_transport_multicast_t *tp,
                                                                          _ze_admin_space_reply_list_t **replies) {
    _z_transport_peer_mutex_lock(&tp->_common);

    for (_z_transport_peer_multicast_slist_t *peers = tp->_peers; peers != NULL;
         peers = _z_transport_peer_multicast_slist_next(peers)) {
        const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(peers);

        z_owned_keyexpr_t ke;
        z_result_t ret;

        ret = _ze_admin_space_pico_transports_0_peers_peer_ke(&ke, local_zid, &peer->common._remote_zid);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to build key expression for pico/session/transports/0/peers peer query: %d", ret);
            continue;
        }

        _ze_admin_space_multicast_peer_ctx_t ctx = {
            .peer = peer,
        };

        ret = _ze_admin_space_reply_if_intersects(query, z_keyexpr_loan(&ke), &ctx, replies,
                                                  _ze_admin_space_encode_pico_transport_0_multicast_peer);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to handle admin space query for pico/session/transports/0/peers peer endpoint: %d", ret);
        }

        z_keyexpr_drop(z_keyexpr_move(&ke));
    }

    _z_transport_peer_mutex_unlock(&tp->_common);
}

static void _ze_admin_space_query_handle_pico_transport_0_peers(const z_loaned_query_t *query, _z_session_t *session,
                                                                _ze_admin_space_reply_list_t **replies) {
    switch (session->_tp._type) {
        case _Z_TRANSPORT_UNICAST_TYPE:
            _ze_admin_space_query_handle_pico_transport_0_unicast_peers(query, &session->_local_zid,
                                                                        &session->_tp._transport._unicast, replies);
            break;

        case _Z_TRANSPORT_MULTICAST_TYPE:
            _ze_admin_space_query_handle_pico_transport_0_multicast_peers(query, &session->_local_zid,
                                                                          &session->_tp._transport._multicast, replies);
            break;

        case _Z_TRANSPORT_RAWETH_TYPE:
            _ze_admin_space_query_handle_pico_transport_0_multicast_peers(query, &session->_local_zid,
                                                                          &session->_tp._transport._raweth, replies);
            break;

        default:
            break;
    }
}

static void _ze_admin_space_query_handle_pico(const z_loaned_query_t *query, _z_session_t *session,
                                              _ze_admin_space_reply_list_t **replies) {
    z_owned_keyexpr_t ke;
    z_result_t ret;

    ret = _ze_admin_space_pico_ke(&ke, &session->_local_zid);
    if (ret == _Z_RES_OK) {
        ret = _ze_admin_space_reply_if_intersects(query, z_keyexpr_loan(&ke), session, replies,
                                                  _ze_admin_space_encode_pico);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to handle admin space query for pico endpoint: %d", ret);
        }
        z_keyexpr_drop(z_keyexpr_move(&ke));
    } else {
        _Z_WARN("Failed to build key expression for pico query: %d", ret);
    }

    ret = _ze_admin_space_pico_session_ke(&ke, &session->_local_zid);
    if (ret == _Z_RES_OK) {
        ret = _ze_admin_space_reply_if_intersects(query, z_keyexpr_loan(&ke), session, replies,
                                                  _ze_admin_space_encode_pico_session);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to handle admin space query for pico/session endpoint: %d", ret);
        }
        z_keyexpr_drop(z_keyexpr_move(&ke));
    } else {
        _Z_WARN("Failed to build key expression for pico/session query: %d", ret);
    }

    ret = _ze_admin_space_pico_transports_ke(&ke, &session->_local_zid);
    if (ret == _Z_RES_OK) {
        ret = _ze_admin_space_reply_if_intersects(query, z_keyexpr_loan(&ke), session, replies,
                                                  _ze_admin_space_encode_pico_transports);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to handle admin space query for pico/session/transports endpoint: %d", ret);
        }
        z_keyexpr_drop(z_keyexpr_move(&ke));
    } else {
        _Z_WARN("Failed to build key expression for pico/session/transports query: %d", ret);
    }

    ret = _ze_admin_space_pico_transports_0_ke(&ke, &session->_local_zid);
    if (ret == _Z_RES_OK) {
        ret = _ze_admin_space_reply_if_intersects(query, z_keyexpr_loan(&ke), session, replies,
                                                  _ze_admin_space_encode_pico_transport_0);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to handle admin space query for pico/session/transports/0 endpoint: %d", ret);
        }
        z_keyexpr_drop(z_keyexpr_move(&ke));
    } else {
        _Z_WARN("Failed to build key expression for pico/session/transports/0 query: %d", ret);
    }

    ret = _ze_admin_space_pico_transports_0_peers_ke(&ke, &session->_local_zid);
    if (ret == _Z_RES_OK) {
        ret = _ze_admin_space_reply_if_intersects(query, z_keyexpr_loan(&ke), session, replies,
                                                  _ze_admin_space_encode_pico_transport_0_peers);
        if (ret != _Z_RES_OK) {
            _Z_WARN("Failed to handle admin space query for pico/session/transports/0/peers endpoint: %d", ret);
        }
        z_keyexpr_drop(z_keyexpr_move(&ke));
    } else {
        _Z_WARN("Failed to build key expression for pico/session/transports/0/peers query: %d", ret);
    }

    _ze_admin_space_query_handle_pico_transport_0_peers(query, session, replies);
}

static void _ze_admin_space_query_handler(z_loaned_query_t *query, void *ctx) {
    _z_session_weak_t *session_weak = (_z_session_weak_t *)ctx;

    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(session_weak);
    if (_Z_RC_IS_NULL(&session_rc)) {
        _Z_ERROR("Dropped admin space query - session closed");
        return;
    }

    _z_session_t *session = _Z_RC_IN_VAL(&session_rc);
    _ze_admin_space_reply_list_t *replies = _ze_admin_space_reply_list_new();

    _ze_admin_space_query_handle_pico(query, session, &replies);

    _ze_admin_space_reply_list_t *next = replies;
    while (next != NULL) {
        _ze_admin_space_reply_t *reply = _ze_admin_space_reply_list_value(next);
        z_query_reply_options_t opt;
        z_query_reply_options_default(&opt);
        z_owned_encoding_t encoding;
        if (z_encoding_clone(&encoding, z_encoding_application_json()) == _Z_RES_OK) {
            opt.encoding = z_encoding_move(&encoding);
            z_result_t res = z_query_reply(query, z_keyexpr_loan(&reply->ke), z_bytes_move(&reply->payload), &opt);
            if (res != _Z_RES_OK) {
                z_view_string_t keystr;
                if (z_keyexpr_as_view_string(z_keyexpr_loan(&reply->ke), &keystr) == _Z_RES_OK) {
                    _Z_ERROR("Failed to reply to admin space query on key expression: %.*s",
                             (int)z_string_len(z_view_string_loan(&keystr)),
                             z_string_data(z_view_string_loan(&keystr)));
                } else {
                    _Z_ERROR("Failed to reply to admin space query");
                }
            }
        } else {
            _Z_ERROR("Failed to clone JSON encoding for admin space query reply");
            break;
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
