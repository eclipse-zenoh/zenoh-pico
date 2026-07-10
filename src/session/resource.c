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

#include "zenoh-pico/session/resource.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "zenoh-pico/api/types.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/session/keyexpr.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

bool _z_resource_eq(const _z_resource_t *other, const _z_resource_t *this_) { return this_->_id == other->_id; }

void _z_resource_clear(_z_resource_t *res) { _z_keyexpr_clear(&res->_key); }

size_t _z_resource_size(_z_resource_t *p) {
    _ZP_UNUSED(p);
    return sizeof(_z_resource_t);
}

void _z_resource_copy(_z_resource_t *dst, const _z_resource_t *src) {
    _z_keyexpr_copy(&dst->_key, &src->_key);
    dst->_id = src->_id;
}

void _z_resource_free(_z_resource_t **res) {
    _z_resource_t *ptr = *res;

    if (ptr != NULL) {
        _z_resource_clear(ptr);

        z_free(ptr);
        *res = NULL;
    }
}

/*------------------ Entity ------------------*/
uint32_t _z_get_entity_id(_z_session_t *zn) { return zn->_entity_id++; }

uint16_t _z_get_resource_id(_z_session_t *zn) { return zn->_resource_id++; }

/*------------------ Resource ------------------*/
_z_resource_t *_z_get_resource_by_id_inner(_z_resource_slist_t *rl, const _z_zint_t id) {
    _z_resource_t *ret = NULL;

    _z_resource_slist_t *xs = rl;
    while (xs != NULL) {
        _z_resource_t *r = _z_resource_slist_value(xs);
        if (r->_id == id) {
            ret = r;
            break;
        }

        xs = _z_resource_slist_next(xs);
    }

    return ret;
}

_z_resource_t *_z_get_resource_by_key_inner(_z_resource_slist_t *rl, const _z_keyexpr_t *keyexpr) {
    _z_resource_t *ret = NULL;
    _z_resource_slist_t *xs = rl;
    while (xs != NULL) {
        _z_resource_t *r = _z_resource_slist_value(xs);
        if (_z_keyexpr_equals(&r->_key, keyexpr)) {
            ret = r;
            break;
        }

        xs = _z_resource_slist_next(xs);
    }

    return ret;
}

static z_result_t _z_get_keyexpr_from_wireexpr_inner(_z_resource_slist_t *xs, const _z_wireexpr_t *expr, char **buf,
                                                     size_t *buf_len) {
    _z_zint_t id = expr->_id;
    size_t prefix_len = 0;
    size_t suffix_len = 0;
    const _z_string_t *suffix = NULL;
    const _z_string_t *prefix = NULL;

    if (!_z_string_view_is_empty(&expr->_suffix)) {
        suffix = _z_string_view_deref(&expr->_suffix);
        suffix_len = _z_string_len(suffix);
    }

    if (id != Z_RESOURCE_ID_NONE) {
        _z_resource_t *res = _z_get_resource_by_id_inner(xs, id);
        if (res == NULL) {
            return _Z_ERR_KEYEXPR_UNKNOWN;
        }
        prefix = &res->_key._keyexpr;
        prefix_len = _z_string_len(prefix);
    }
    if (*buf == NULL) {
        *buf_len = prefix_len + suffix_len;
        _Z_RETURN_ERR_OOM_IF_TRUE((*buf = (char *)z_malloc(*buf_len)) == NULL);
    } else if (prefix_len + suffix_len > *buf_len) {
        *buf_len = prefix_len + suffix_len;
        return _Z_ERR_INVALID;
    } else {
        *buf_len = prefix_len + suffix_len;
    }
    if (prefix_len > 0) {
        // SAFETY: buf is guaranteed to be large enough by the caller.
        // Flawfinder: ignore [CWE-120]
        memcpy(*buf, _z_string_data(prefix), prefix_len);
    }
    if (suffix_len > 0) {
        // SAFETY: buf is guaranteed to be large enough by the caller.
        // Flawfinder: ignore [CWE-120]
        memcpy(*buf + prefix_len, _z_string_data(suffix), suffix_len);
    }
    return _Z_RES_OK;
}

z_result_t _z_get_keyexpr_view_from_wireexpr(_z_session_t *zn, _z_keyexpr_view_t *out, const _z_wireexpr_t *expr,
                                             size_t peer_id, char *out_buf, size_t out_buf_len) {
    *out = _z_keyexpr_view_null();
    z_result_t ret = _Z_ERR_NULL;
    if (expr != NULL && _z_wireexpr_check(expr)) {
        if (expr->_id == Z_RESOURCE_ID_NONE) {
            *out = _z_keyexpr_view_from_string_view(&expr->_suffix);
            return _Z_RES_OK;
        }
        _z_session_mutex_lock(zn);
        _z_resource_slist_t *decls = _z_wireexpr_is_local(expr) ? zn->_local_resources : zn->_remote_resources[peer_id];
        ret = _z_get_keyexpr_from_wireexpr_inner(decls, expr, &out_buf, &out_buf_len);
        _z_session_mutex_unlock(zn);
        if (ret == _Z_RES_OK) {
            _z_string_view_t sv = _z_string_view_make(out_buf, out_buf_len);
            *out = _z_keyexpr_view_from_string_view(&sv);
        }
    }
    return ret;
}

z_result_t _z_get_keyexpr_from_wireexpr(_z_session_t *zn, _z_keyexpr_t *out, const _z_wireexpr_t *expr,
                                        size_t peer_id) {
    *out = _z_keyexpr_null();
    z_result_t ret = _Z_ERR_NULL;
    if (expr != NULL && _z_wireexpr_check(expr)) {
        _z_session_mutex_lock(zn);
        _z_resource_slist_t *decls = _z_wireexpr_is_local(expr) ? zn->_local_resources : zn->_remote_resources[peer_id];
        char *buf = NULL;
        size_t buf_len = 0;
        ret = _z_get_keyexpr_from_wireexpr_inner(decls, expr, &buf, &buf_len);
        _z_session_mutex_unlock(zn);
        if (ret == _Z_RES_OK) {
            _z_string_t s = _z_string_from_substr_custom_deleter(buf, buf_len, _z_delete_context_default());
            _z_keyexpr_from_string(out, &s);
        }
    }
    return ret;
}

z_result_t _z_register_local_resource_inner(_z_session_t *zn, const _z_string_t *key, uint16_t *out_id) {
    _z_keyexpr_view_t ke_view = _z_keyexpr_view_from_string(key);
    _z_resource_t *res = _z_get_resource_by_key_inner(zn->_local_resources, _z_keyexpr_view_deref(&ke_view));
    if (res != NULL) {  // declaration of already declared resource
        res->_refcount++;
        *out_id = res->_id;
        return _Z_RES_OK;
    } else {
        _z_keyexpr_t ke = _z_keyexpr_null();
        _Z_RETURN_IF_ERR(_z_keyexpr_copy(&ke, _z_keyexpr_view_deref(&ke_view)));
        zn->_local_resources = _z_resource_slist_push_empty(zn->_local_resources);
        res = _z_resource_slist_value(zn->_local_resources);
        res->_refcount = 1;
        res->_key = ke;
        res->_id = _z_get_resource_id(zn);
        *out_id = res->_id;
    }
    return _Z_RES_OK;
}

z_result_t _z_register_local_resource(_z_session_t *zn, const _z_string_t *expr, uint16_t *out_id) {
    _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(zn));
    z_result_t ret = _z_register_local_resource_inner(zn, expr, out_id);
    _z_session_mutex_unlock(zn);
    return ret;
}

z_result_t _z_register_remote_resource_inner(_z_session_t *zn, const _z_wireexpr_t *key, uint16_t id, size_t peer_id) {
    _z_resource_slist_t **resources = &zn->_remote_resources[peer_id];
    _z_resource_slist_t *parent_resources =
        key->_mapping == _Z_KEYEXPR_MAPPING_LOCAL ? zn->_local_resources : *resources;
    _z_string_t new_key = _z_string_null();
    if (key->_id != Z_RESOURCE_ID_NONE) {
        _z_resource_t *res = _z_get_resource_by_id_inner(parent_resources, key->_id);
        if (res == NULL) {
            _Z_ERROR("Unknown scope: %d, for mapping: %d", (unsigned int)key->_id, (int)key->_mapping);
            return _Z_ERR_ENTITY_DECLARATION_FAILED;
        }
        if (_z_wireexpr_has_suffix(key)) {
            _Z_RETURN_IF_ERR(
                _z_string_concat(&new_key, &res->_key._keyexpr, _z_string_view_deref(&key->_suffix), NULL, 0));
        } else {
            _Z_RETURN_IF_ERR(_z_string_copy(&new_key, &res->_key._keyexpr));
        }
    } else {
        _Z_RETURN_IF_ERR(_z_string_copy(&new_key, _z_string_view_deref(&key->_suffix)));
    }
    *resources = _z_resource_slist_push_empty(*resources);
    _z_resource_t *res = _z_resource_slist_value(*resources);
    res->_refcount = 1;
    _z_keyexpr_from_string(&res->_key, &new_key);
    res->_id = id;

    return _Z_RES_OK;
}

z_result_t _z_register_remote_resource(_z_session_t *zn, const _z_wireexpr_t *key, uint16_t id, size_t peer_id) {
    // No particular locking is needed here for now, since only rx thread can access remote resources list
    if (key->_mapping != _Z_KEYEXPR_MAPPING_REMOTE) {
        _Z_RETURN_IF_ERR(_z_session_mutex_lock_if_open(zn));
    }
    z_result_t ret = _z_register_remote_resource_inner(zn, key, id, peer_id);
    if (key->_mapping != _Z_KEYEXPR_MAPPING_REMOTE) {
        _z_session_mutex_unlock(zn);
    }
    return ret;
}

z_result_t _z_unregister_remote_resource(_z_session_t *zn, uint16_t id, size_t peer_id) {
    // No particular locking is needed here for now, since only rx thread can access remote resources list
    _Z_DEBUG("unregistering remote resource: id %d, peer_id: %zu", id, peer_id);
    // No particular locking is needed here, since this function is only called by rx thread
    _z_resource_t *res = _z_get_resource_by_id_inner(zn->_remote_resources[peer_id], id);
    if (res == NULL) {
        return _Z_ERR_KEYEXPR_UNKNOWN;
    } else {
        zn->_remote_resources[peer_id] =
            _z_resource_slist_drop_first_filter(zn->_remote_resources[peer_id], _z_resource_eq, res);
    }
    return _Z_RES_OK;
}

z_result_t _z_unregister_local_resource_inner(_z_session_t *zn, uint16_t id) {
    _z_resource_t *res = _z_get_resource_by_id_inner(zn->_local_resources, id);
    if (res == NULL) {
        return _Z_ERR_KEYEXPR_UNKNOWN;
    } else {
        res->_refcount--;
        if (res->_refcount == 0) {
            zn->_local_resources = _z_resource_slist_drop_first_filter(zn->_local_resources, _z_resource_eq, res);
        }
    }
    return _Z_RES_OK;
}

z_result_t _z_unregister_local_resource(_z_session_t *zn, uint16_t id) {
    _Z_DEBUG("unregistering local resource: id %d", id);
    _z_session_mutex_lock(zn);
    z_result_t ret = _z_unregister_local_resource_inner(zn, id);
    _z_session_mutex_unlock(zn);
    return ret;
}

void _z_flush_local_resources(_z_session_t *zn) {
    _z_session_mutex_lock(zn);
    _z_resource_slist_free(&zn->_local_resources);
    _z_session_mutex_unlock(zn);
}

void _z_flush_remote_resources(_z_session_t *zn) {
    // No particular locking is needed here, since this function is only called once runtime is stopped,
    // and thus no other thread should be accessing the remote resources at this point.
    for (size_t i = 0; i < Z_MAX_NUM_PEERS; i++) {
        _z_resource_slist_free(&zn->_remote_resources[i]);
    }
}

void _z_flush_remote_resources_for_peer(_z_session_t *zn, size_t peer_id) {
    // No particular locking is needed here, since this function is only called by rx thread when a peer is
    // disconnected.
    if (peer_id < Z_MAX_NUM_PEERS) {
        _z_resource_slist_free(&zn->_remote_resources[peer_id]);
    }
}
