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
#include "zenoh-pico/utils/result.h"

#if Z_FEATURE_ADMIN_SPACE == 1
#if Z_FEATURE_CONNECTIVITY != 1
#error "Z_FEATURE_ADMIN_SPACE requires Z_FEATURE_CONNECTIVITY"
#endif

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

static z_result_t _ze_admin_space_encode_whatami(_z_json_encoder_t *je, z_whatami_t mode);

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORT_* / PEER_ZID
static z_result_t _ze_admin_space_peer_transport_ke(z_owned_keyexpr_t *ke, const z_id_t *zid, const char *transport,
                                                    const z_id_t *peer_zid) {
    z_internal_keyexpr_null(ke);

    z_owned_string_t zid_str;
    _Z_RETURN_IF_ERR(z_id_to_string(zid, &zid_str));
    z_owned_string_t peer_zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(peer_zid, &peer_zid_str), z_string_drop(z_string_move(&zid_str)));

    char buf[192];
    int n = snprintf(buf, sizeof(buf), "%s%s%.*s%s%s%s%s%s%s%s%.*s", _Z_KEYEXPR_AT, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&zid_str)), z_string_data(z_string_loan(&zid_str)),
                     _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_PICO, _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_SESSION,
                     _Z_KEYEXPR_SEPARATOR, transport, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&peer_zid_str)), z_string_data(z_string_loan(&peer_zid_str)));

    z_string_drop(z_string_move(&zid_str));
    z_string_drop(z_string_move(&peer_zid_str));

    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return _Z_ERR_INVALID;
    }

    return z_keyexpr_from_str(ke, buf);
}

// ke = _Z_KEYEXPR_AT / ZID / _Z_KEYEXPR_PICO / _Z_KEYEXPR_SESSION / _Z_KEYEXPR_TRANSPORT_* / PEER_ZID /
//      _Z_KEYEXPR_LINK / LINK_ID
static z_result_t _ze_admin_space_peer_link_ke(z_owned_keyexpr_t *ke, const z_id_t *zid, const char *transport,
                                               const z_id_t *peer_zid, const z_id_t *link_id) {
    z_internal_keyexpr_null(ke);

    z_owned_string_t zid_str;
    _Z_RETURN_IF_ERR(z_id_to_string(zid, &zid_str));
    z_owned_string_t peer_zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(peer_zid, &peer_zid_str), z_string_drop(z_string_move(&zid_str)));
    z_owned_string_t link_id_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(link_id, &link_id_str), z_string_drop(z_string_move(&zid_str));
                           z_string_drop(z_string_move(&peer_zid_str)));

    char buf[256];
    int n = snprintf(buf, sizeof(buf), "%s%s%.*s%s%s%s%s%s%s%s%.*s%s%s%s%.*s", _Z_KEYEXPR_AT, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&zid_str)), z_string_data(z_string_loan(&zid_str)),
                     _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_PICO, _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_SESSION,
                     _Z_KEYEXPR_SEPARATOR, transport, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&peer_zid_str)), z_string_data(z_string_loan(&peer_zid_str)),
                     _Z_KEYEXPR_SEPARATOR, _Z_KEYEXPR_LINK, _Z_KEYEXPR_SEPARATOR,
                     (int)z_string_len(z_string_loan(&link_id_str)), z_string_data(z_string_loan(&link_id_str)));

    z_string_drop(z_string_move(&zid_str));
    z_string_drop(z_string_move(&peer_zid_str));
    z_string_drop(z_string_move(&link_id_str));

    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return _Z_ERR_INVALID;
    }

    return z_keyexpr_from_str(ke, buf);
}

static inline const char *_ze_admin_space_transport_kind_from_session(const _z_session_t *session, bool is_multicast) {
    if (session != NULL && session->_tp._type == _Z_TRANSPORT_RAWETH_TYPE) {
        return _Z_KEYEXPR_TRANSPORT_RAWETH;
    }
    return is_multicast ? _Z_KEYEXPR_TRANSPORT_MULTICAST : _Z_KEYEXPR_TRANSPORT_UNICAST;
}

typedef struct {
    _z_session_weak_t _session;
} _ze_admin_space_connectivity_listener_ctx_t;

static void _ze_admin_space_connectivity_listener_ctx_drop(void *ctx) {
    _ze_admin_space_connectivity_listener_ctx_t *listener_ctx = (_ze_admin_space_connectivity_listener_ctx_t *)ctx;
    if (listener_ctx != NULL) {
        _z_session_weak_drop(&listener_ctx->_session);
        z_free(listener_ctx);
    }
}

static _ze_admin_space_connectivity_listener_ctx_t *_ze_admin_space_connectivity_listener_ctx_new(
    const z_loaned_session_t *zs) {
    _ze_admin_space_connectivity_listener_ctx_t *ctx =
        (_ze_admin_space_connectivity_listener_ctx_t *)z_malloc(sizeof(_ze_admin_space_connectivity_listener_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->_session = _z_session_rc_clone_as_weak(zs);
    if (_Z_RC_IS_NULL(&ctx->_session)) {
        z_free(ctx);
        return NULL;
    }

    return ctx;
}

static z_result_t _ze_admin_space_encode_transport_event_payload(z_owned_bytes_t *payload,
                                                                 const z_loaned_transport_t *transport) {
    z_internal_bytes_null(payload);

    _z_json_encoder_t je;
    _Z_RETURN_IF_ERR(_z_json_encoder_empty(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(&je), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "zid"), _z_json_encoder_clear(&je));
    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(&transport->_zid, &zid_str), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(&je, &zid_str._val), z_string_drop(z_string_move(&zid_str));
                           _z_json_encoder_clear(&je));
    z_string_drop(z_string_move(&zid_str));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "whatami"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_encode_whatami(&je, transport->_whatami), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_qos"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, transport->_is_qos), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_multicast"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, transport->_is_multicast), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_shm"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, transport->_is_shm), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(&je), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_finish(&je, payload), _z_json_encoder_clear(&je));
    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_encode_link_event_payload(z_owned_bytes_t *payload, const z_loaned_link_t *link) {
    z_internal_bytes_null(payload);

    _z_json_encoder_t je;
    _Z_RETURN_IF_ERR(_z_json_encoder_empty(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(&je), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "zid"), _z_json_encoder_clear(&je));
    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(&link->_zid, &zid_str), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(&je, &zid_str._val), z_string_drop(z_string_move(&zid_str));
                           _z_json_encoder_clear(&je));
    z_string_drop(z_string_move(&zid_str));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "src"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(&je, &link->_src), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "dst"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(&je, &link->_dst), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "mtu"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_u64(&je, (uint64_t)link->_mtu), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_streamed"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, link->_is_streamed), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_reliable"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, link->_is_reliable), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(&je), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_finish(&je, payload), _z_json_encoder_clear(&je));
    return _Z_RES_OK;
}

static void _ze_admin_space_publish_transport_event(z_loaned_transport_event_t *event, void *ctx) {
    _ze_admin_space_connectivity_listener_ctx_t *listener_ctx = (_ze_admin_space_connectivity_listener_ctx_t *)ctx;
    if (event == NULL || listener_ctx == NULL) {
        return;
    }

    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(&listener_ctx->_session);
    if (_Z_RC_IS_NULL(&session_rc)) {
        return;
    }

    _z_session_t *session = _Z_RC_IN_VAL(&session_rc);
    const z_loaned_transport_t *transport = z_transport_event_transport(event);
    const char *transport_kind = _ze_admin_space_transport_kind_from_session(session, transport->_is_multicast);

    z_owned_keyexpr_t ke;
    z_result_t ret = _ze_admin_space_peer_transport_ke(&ke, &session->_local_zid, transport_kind, &transport->_zid);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("Failed to build admin-space keyexpr for transport event: %d", ret);
        _z_session_rc_drop(&session_rc);
        return;
    }

    if (z_transport_event_kind(event) == Z_SAMPLE_KIND_PUT) {
        z_owned_bytes_t payload;
        ret = _ze_admin_space_encode_transport_event_payload(&payload, transport);
        if (ret == _Z_RES_OK) {
            z_put_options_t opts;
            z_put_options_default(&opts);
            z_owned_encoding_t encoding;
            if (z_encoding_clone(&encoding, z_encoding_application_json()) == _Z_RES_OK) {
                opts.encoding = z_encoding_move(&encoding);
                ret = z_put(&session_rc, z_keyexpr_loan(&ke), z_bytes_move(&payload), &opts);
            } else {
                z_bytes_drop(z_bytes_move(&payload));
                ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }
        }
    } else if (z_transport_event_kind(event) == Z_SAMPLE_KIND_DELETE) {
        z_delete_options_t opts;
        z_delete_options_default(&opts);
        ret = z_delete(&session_rc, z_keyexpr_loan(&ke), &opts);
    } else {
        ret = _Z_RES_OK;
    }

    if (ret != _Z_RES_OK) {
        _Z_ERROR("Failed to publish admin-space transport event: %d", ret);
    }

    z_keyexpr_drop(z_keyexpr_move(&ke));
    _z_session_rc_drop(&session_rc);
}

static void _ze_admin_space_publish_link_event(z_loaned_link_event_t *event, void *ctx) {
    _ze_admin_space_connectivity_listener_ctx_t *listener_ctx = (_ze_admin_space_connectivity_listener_ctx_t *)ctx;
    if (event == NULL || listener_ctx == NULL) {
        return;
    }

    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(&listener_ctx->_session);
    if (_Z_RC_IS_NULL(&session_rc)) {
        return;
    }

    _z_session_t *session = _Z_RC_IN_VAL(&session_rc);
    const z_loaned_link_t *link = z_link_event_link(event);
    const bool is_multicast = session->_tp._type != _Z_TRANSPORT_UNICAST_TYPE;
    const char *transport_kind = _ze_admin_space_transport_kind_from_session(session, is_multicast);

    z_owned_keyexpr_t ke;
    z_result_t ret = _ze_admin_space_peer_link_ke(&ke, &session->_local_zid, transport_kind, &link->_zid, &link->_zid);
    if (ret != _Z_RES_OK) {
        _Z_ERROR("Failed to build admin-space keyexpr for link event: %d", ret);
        _z_session_rc_drop(&session_rc);
        return;
    }

    if (z_link_event_kind(event) == Z_SAMPLE_KIND_PUT) {
        z_owned_bytes_t payload;
        ret = _ze_admin_space_encode_link_event_payload(&payload, link);
        if (ret == _Z_RES_OK) {
            z_put_options_t opts;
            z_put_options_default(&opts);
            z_owned_encoding_t encoding;
            if (z_encoding_clone(&encoding, z_encoding_application_json()) == _Z_RES_OK) {
                opts.encoding = z_encoding_move(&encoding);
                ret = z_put(&session_rc, z_keyexpr_loan(&ke), z_bytes_move(&payload), &opts);
            } else {
                z_bytes_drop(z_bytes_move(&payload));
                ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
            }
        }
    } else if (z_link_event_kind(event) == Z_SAMPLE_KIND_DELETE) {
        z_delete_options_t opts;
        z_delete_options_default(&opts);
        ret = z_delete(&session_rc, z_keyexpr_loan(&ke), &opts);
    } else {
        ret = _Z_RES_OK;
    }

    if (ret != _Z_RES_OK) {
        _Z_ERROR("Failed to publish admin-space link event: %d", ret);
    }

    z_keyexpr_drop(z_keyexpr_move(&ke));
    _z_session_rc_drop(&session_rc);
}

static z_result_t _ze_admin_space_undeclare_transport_listener(z_loaned_session_t *zs, size_t listener_id) {
    if (listener_id == 0) {
        return _Z_RES_OK;
    }

    z_owned_transport_events_listener_t listener;
    listener._val = (_z_transport_events_listener_t){0};
    listener._val._id = listener_id;
    listener._val._session = _z_session_rc_clone_as_weak(zs);
    if (_Z_RC_IS_NULL(&listener._val._session)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return z_undeclare_transport_events_listener(z_transport_events_listener_move(&listener));
}

static z_result_t _ze_admin_space_undeclare_link_listener(z_loaned_session_t *zs, size_t listener_id) {
    if (listener_id == 0) {
        return _Z_RES_OK;
    }

    z_owned_link_events_listener_t listener;
    listener._val = (_z_link_events_listener_t){0};
    listener._val._id = listener_id;
    listener._val._session = _z_session_rc_clone_as_weak(zs);
    if (_Z_RC_IS_NULL(&listener._val._session)) {
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    return z_undeclare_link_events_listener(z_link_events_listener_move(&listener));
}

static inline void _ze_admin_space_transport_listener_handle_clear(z_owned_transport_events_listener_t *listener) {
    _z_session_weak_drop(&listener->_val._session);
    listener->_val = (_z_transport_events_listener_t){0};
}

static inline void _ze_admin_space_link_listener_handle_clear(z_owned_link_events_listener_t *listener) {
    _z_session_weak_drop(&listener->_val._session);
    listener->_val = (_z_link_events_listener_t){0};
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

static void _ze_admin_space_reply_null(_ze_admin_space_reply_t *reply) {
    z_internal_keyexpr_null(&reply->ke);
    z_internal_bytes_null(&reply->payload);
}

void _ze_admin_space_reply_clear(_ze_admin_space_reply_t *reply) {
    z_keyexpr_drop(z_keyexpr_move(&reply->ke));
    z_bytes_drop(z_bytes_move(&reply->payload));
    _ze_admin_space_reply_null(reply);
}

static z_result_t _ze_admin_space_add_reply_payload(const z_loaned_keyexpr_t *ke, z_owned_bytes_t *payload,
                                                    _ze_admin_space_reply_list_t **replies) {
    _ze_admin_space_reply_t *reply = z_malloc(sizeof(_ze_admin_space_reply_t));
    if (reply == NULL) {
        z_bytes_drop(z_bytes_move(payload));
        return _Z_ERR_SYSTEM_OUT_OF_MEMORY;
    }
    _ze_admin_space_reply_null(reply);

    _Z_CLEAN_RETURN_IF_ERR(z_keyexpr_clone(&reply->ke, ke), z_bytes_drop(z_bytes_move(payload)); z_free(reply));
    reply->payload._val = payload->_val;
    z_internal_bytes_null(payload);

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

static z_result_t _ze_admin_space_encode_transport_payload_from_peer(z_owned_bytes_t *payload,
                                                                     const _z_transport_peer_common_t *peer,
                                                                     bool is_multicast) {
    z_internal_bytes_null(payload);

    _z_json_encoder_t je;
    _Z_RETURN_IF_ERR(_z_json_encoder_empty(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(&je), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "zid"), _z_json_encoder_clear(&je));
    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(&peer->_remote_zid, &zid_str), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(&je, &zid_str._val), z_string_drop(z_string_move(&zid_str));
                           _z_json_encoder_clear(&je));
    z_string_drop(z_string_move(&zid_str));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "whatami"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_ze_admin_space_encode_whatami(&je, peer->_remote_whatami), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_qos"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, false), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_multicast"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, is_multicast), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_shm"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, false), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(&je), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_finish(&je, payload), _z_json_encoder_clear(&je));
    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_encode_link_payload_from_peer(z_owned_bytes_t *payload,
                                                                const _z_transport_peer_common_t *peer, uint16_t mtu,
                                                                bool is_streamed, bool is_reliable) {
    z_internal_bytes_null(payload);

    _z_json_encoder_t je;
    _Z_RETURN_IF_ERR(_z_json_encoder_empty(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_start_object(&je), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "zid"), _z_json_encoder_clear(&je));
    z_owned_string_t zid_str;
    _Z_CLEAN_RETURN_IF_ERR(z_id_to_string(&peer->_remote_zid, &zid_str), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(&je, &zid_str._val), z_string_drop(z_string_move(&zid_str));
                           _z_json_encoder_clear(&je));
    z_string_drop(z_string_move(&zid_str));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "src"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(&je, &peer->_link_src), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "dst"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_z_string(&je, &peer->_link_dst), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "mtu"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_u64(&je, (uint64_t)mtu), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_streamed"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, is_streamed), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_key(&je, "is_reliable"), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_write_boolean(&je, is_reliable), _z_json_encoder_clear(&je));

    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_end_object(&je), _z_json_encoder_clear(&je));
    _Z_CLEAN_RETURN_IF_ERR(_z_json_encoder_finish(&je, payload), _z_json_encoder_clear(&je));
    return _Z_RES_OK;
}

static z_result_t _ze_admin_space_add_transport_reply_for_peer(const z_loaned_query_t *query, const z_id_t *zid,
                                                               const char *transport_keyexpr,
                                                               const _z_transport_peer_common_t *peer,
                                                               bool is_multicast,
                                                               _ze_admin_space_reply_list_t **replies) {
    z_owned_keyexpr_t ke;
    _Z_RETURN_IF_ERR(_ze_admin_space_peer_transport_ke(&ke, zid, transport_keyexpr, &peer->_remote_zid));

    z_result_t ret = _Z_RES_OK;
    if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke))) {
        z_owned_bytes_t payload;
        ret = _ze_admin_space_encode_transport_payload_from_peer(&payload, peer, is_multicast);
        if (ret == _Z_RES_OK) {
            ret = _ze_admin_space_add_reply_payload(z_keyexpr_loan(&ke), &payload, replies);
        }
    }

    z_keyexpr_drop(z_keyexpr_move(&ke));
    return ret;
}

static z_result_t _ze_admin_space_add_link_reply_for_peer(const z_loaned_query_t *query, const z_id_t *zid,
                                                          const char *transport_keyexpr,
                                                          const _z_transport_peer_common_t *peer,
                                                          const _z_transport_common_t *transport_common,
                                                          _ze_admin_space_reply_list_t **replies) {
    z_owned_keyexpr_t ke;
    _Z_RETURN_IF_ERR(_ze_admin_space_peer_link_ke(&ke, zid, transport_keyexpr, &peer->_remote_zid, &peer->_remote_zid));

    z_result_t ret = _Z_RES_OK;
    if (z_keyexpr_intersects(z_query_keyexpr(query), z_keyexpr_loan(&ke))) {
        uint16_t mtu;
        bool is_streamed;
        bool is_reliable;
        _z_transport_link_properties_from_transport(transport_common, &mtu, &is_streamed, &is_reliable);

        z_owned_bytes_t payload;
        ret = _ze_admin_space_encode_link_payload_from_peer(&payload, peer, mtu, is_streamed, is_reliable);
        if (ret == _Z_RES_OK) {
            ret = _ze_admin_space_add_reply_payload(z_keyexpr_loan(&ke), &payload, replies);
        }
    }

    z_keyexpr_drop(z_keyexpr_move(&ke));
    return ret;
}

static void _ze_admin_space_query_handle_unicast(const z_loaned_query_t *query, _z_session_t *session,
                                                 _ze_admin_space_reply_list_t **replies) {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_transport_peer_mutex_lock(&session->_tp._transport._unicast._common);
#endif

    const _z_transport_common_t *transport_common = &session->_tp._transport._unicast._common;
    const _z_transport_peer_unicast_slist_t *peers = session->_tp._transport._unicast._peers;
    while (peers != NULL) {
        const _z_transport_peer_unicast_t *peer = _z_transport_peer_unicast_slist_value(peers);
        z_result_t ret = _ze_admin_space_add_transport_reply_for_peer(
            query, &session->_local_zid, _Z_KEYEXPR_TRANSPORT_UNICAST, &peer->common, false, replies);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Failed to reply to admin space %s transport query: %d", _Z_KEYEXPR_TRANSPORT_UNICAST, ret);
        }

        ret = _ze_admin_space_add_link_reply_for_peer(query, &session->_local_zid, _Z_KEYEXPR_TRANSPORT_UNICAST,
                                                      &peer->common, transport_common, replies);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Failed to reply to admin space %s link query: %d", _Z_KEYEXPR_TRANSPORT_UNICAST, ret);
        }

        peers = _z_transport_peer_unicast_slist_next(peers);
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_transport_peer_mutex_unlock(&session->_tp._transport._unicast._common);
#endif
}

static void _ze_admin_space_query_handle_multicast_common(const z_loaned_query_t *query, _z_session_t *session,
                                                          _z_transport_multicast_t *transport,
                                                          const char *transport_keyexpr,
                                                          _ze_admin_space_reply_list_t **replies) {
#if Z_FEATURE_MULTI_THREAD == 1
    _z_transport_peer_mutex_lock(&transport->_common);
#endif

    const _z_transport_common_t *transport_common = &transport->_common;
    const _z_transport_peer_multicast_slist_t *peers = transport->_peers;
    while (peers != NULL) {
        const _z_transport_peer_multicast_t *peer = _z_transport_peer_multicast_slist_value(peers);
        z_result_t ret = _ze_admin_space_add_transport_reply_for_peer(query, &session->_local_zid, transport_keyexpr,
                                                                      &peer->common, true, replies);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Failed to reply to admin space %s transport query: %d", transport_keyexpr, ret);
        }

        ret = _ze_admin_space_add_link_reply_for_peer(query, &session->_local_zid, transport_keyexpr, &peer->common,
                                                      transport_common, replies);
        if (ret != _Z_RES_OK) {
            _Z_ERROR("Failed to reply to admin space %s link query: %d", transport_keyexpr, ret);
        }

        peers = _z_transport_peer_multicast_slist_next(peers);
    }

#if Z_FEATURE_MULTI_THREAD == 1
    _z_transport_peer_mutex_unlock(&transport->_common);
#endif
}

static void _ze_admin_space_query_handler(z_loaned_query_t *query, void *ctx) {
    _z_session_weak_t *session_weak = (_z_session_weak_t *)ctx;

    _z_session_rc_t session_rc = _z_session_weak_upgrade_if_open(session_weak);
    if (_Z_RC_IS_NULL(&session_rc)) {
        _Z_ERROR("Dropped admin space query - session closed");
        return;
    }

    _ze_admin_space_reply_list_t *replies = _ze_admin_space_reply_list_new();

    _z_session_t *session = _Z_RC_IN_VAL(&session_rc);
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

    _z_session_t *session = _Z_RC_IN_VAL(zs);
    session->_admin_space_transport_listener_id = 0;
    session->_admin_space_link_listener_id = 0;
    z_result_t ret = _Z_ERR_GENERIC;

    _ze_admin_space_connectivity_listener_ctx_t *transport_ctx = _ze_admin_space_connectivity_listener_ctx_new(zs);
    if (transport_ctx == NULL) {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        goto err_queryable;
    }

    z_owned_closure_transport_event_t transport_callback;
    ret = z_closure_transport_event(&transport_callback, _ze_admin_space_publish_transport_event,
                                    _ze_admin_space_connectivity_listener_ctx_drop, transport_ctx);
    if (ret != _Z_RES_OK) {
        _ze_admin_space_connectivity_listener_ctx_drop(transport_ctx);
        goto err_queryable;
    }

    z_owned_transport_events_listener_t transport_listener;
    z_transport_events_listener_options_t transport_opts;
    z_transport_events_listener_options_default(&transport_opts);
    ret = z_declare_transport_events_listener(zs, &transport_listener,
                                              z_closure_transport_event_move(&transport_callback), &transport_opts);
    if (ret != _Z_RES_OK) {
        goto err_queryable;
    }
    session->_admin_space_transport_listener_id = transport_listener._val._id;
    _ze_admin_space_transport_listener_handle_clear(&transport_listener);

    _ze_admin_space_connectivity_listener_ctx_t *link_ctx = _ze_admin_space_connectivity_listener_ctx_new(zs);
    if (link_ctx == NULL) {
        ret = _Z_ERR_SYSTEM_OUT_OF_MEMORY;
        goto err_transport_listener;
    }

    z_owned_closure_link_event_t link_callback;
    ret = z_closure_link_event(&link_callback, _ze_admin_space_publish_link_event,
                               _ze_admin_space_connectivity_listener_ctx_drop, link_ctx);
    if (ret != _Z_RES_OK) {
        _ze_admin_space_connectivity_listener_ctx_drop(link_ctx);
        goto err_transport_listener;
    }

    z_owned_link_events_listener_t link_listener;
    z_link_events_listener_options_t link_opts;
    z_link_events_listener_options_default(&link_opts);
    ret = z_declare_link_events_listener(zs, &link_listener, z_closure_link_event_move(&link_callback), &link_opts);
    if (ret != _Z_RES_OK) {
        goto err_transport_listener;
    }
    session->_admin_space_link_listener_id = link_listener._val._id;
    _ze_admin_space_link_listener_handle_clear(&link_listener);

    return _Z_RES_OK;

err_transport_listener: {
    z_result_t undeclare_transport_listener_ret =
        _ze_admin_space_undeclare_transport_listener(zs, session->_admin_space_transport_listener_id);
    if (undeclare_transport_listener_ret == _Z_RES_OK) {
        session->_admin_space_transport_listener_id = 0;
    } else if (ret == _Z_RES_OK) {
        ret = undeclare_transport_listener_ret;
    }
}

err_queryable: {
    _z_queryable_t queryable = {
        ._entity_id = session->_admin_space_queryable_id,
        ._zn = _z_session_rc_clone_as_weak(zs),
    };
    z_result_t undeclare_queryable_ret = _z_undeclare_queryable(&queryable);
    _z_queryable_clear(&queryable);
    if (undeclare_queryable_ret == _Z_RES_OK) {
        session->_admin_space_queryable_id = 0;
    } else if (ret == _Z_RES_OK) {
        ret = undeclare_queryable_ret;
    }
}
    return ret;
}

z_result_t zp_stop_admin_space(z_loaned_session_t *zs) {
    _z_session_t *session = _Z_RC_IN_VAL(zs);
    uint32_t admin_space_queryable_id = session->_admin_space_queryable_id;
    size_t admin_space_transport_listener_id = session->_admin_space_transport_listener_id;
    size_t admin_space_link_listener_id = session->_admin_space_link_listener_id;

    if (admin_space_queryable_id == 0 && admin_space_transport_listener_id == 0 && admin_space_link_listener_id == 0) {
        // Already stopped
        return _Z_RES_OK;
    }

    z_result_t ret = _Z_RES_OK;

    if (admin_space_transport_listener_id != 0) {
        z_result_t listener_ret = _ze_admin_space_undeclare_transport_listener(zs, admin_space_transport_listener_id);
        if (listener_ret == _Z_RES_OK) {
            session->_admin_space_transport_listener_id = 0;
        } else if (ret == _Z_RES_OK) {
            ret = listener_ret;
        }
    }

    if (admin_space_link_listener_id != 0) {
        z_result_t listener_ret = _ze_admin_space_undeclare_link_listener(zs, admin_space_link_listener_id);
        if (listener_ret == _Z_RES_OK) {
            session->_admin_space_link_listener_id = 0;
        } else if (ret == _Z_RES_OK) {
            ret = listener_ret;
        }
    }

    if (admin_space_queryable_id != 0) {
        _z_queryable_t admin_space_queryable = {
            ._entity_id = admin_space_queryable_id,
            ._zn = _z_session_rc_clone_as_weak(zs),
        };

        z_result_t queryable_ret = _z_undeclare_queryable(&admin_space_queryable);
        _z_queryable_clear(&admin_space_queryable);
        if (queryable_ret == _Z_RES_OK) {
            session->_admin_space_queryable_id = 0;
        } else if (ret == _Z_RES_OK) {
            ret = queryable_ret;
        }
    }

    return ret;
}

#endif  // Z_FEATURE_ADMIN_SPACE == 1
