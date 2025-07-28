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

#include "zenoh-pico/api/types.h"
#include "zenoh-pico/config.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/keyexpr.h"
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
    dst->_refcount = src->_refcount;
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
_z_resource_t *__z_get_resource_by_id(_z_resource_slist_t *rl, uintptr_t mapping, const _z_zint_t id) {
    _z_resource_t *ret = NULL;

    _z_resource_slist_t *xs = rl;
    while (xs != NULL) {
        _z_resource_t *r = _z_resource_slist_value(xs);
        if (r->_id == id && r->_key._mapping == mapping) {
            ret = r;
            break;
        }

        xs = _z_resource_slist_next(xs);
    }

    return ret;
}

_z_resource_t *__z_get_resource_by_key(_z_resource_slist_t *rl, const _z_keyexpr_t *keyexpr) {
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

static _z_keyexpr_t __z_get_expanded_key_from_key(_z_resource_slist_t *xs, const _z_keyexpr_t *keyexpr,
                                                  bool force_alias) {
    _z_zint_t id = keyexpr->_id;

    // Check if ke is already expanded
    if (id == Z_RESOURCE_ID_NONE) {
        if (!_z_keyexpr_has_suffix(keyexpr)) {
            return _z_keyexpr_null();
        }
        // Keyexpr can be aliased from a rx buffer
        if (force_alias) {
            return _z_keyexpr_alias(keyexpr);
        } else {
            return _z_keyexpr_duplicate(keyexpr);
        }
    }

    // Need to build the complete resource name, by recursively look at RIDs
    // Resource names are looked up from right to left
    _z_keyexpr_t ret = _z_keyexpr_null();
    _z_string_list_t *strs = NULL;
    size_t len = 0;

    // Append suffix as the right-most segment
    if (_z_keyexpr_has_suffix(keyexpr)) {
        len = len + _z_string_len(&keyexpr->_suffix);
        strs = _z_string_list_push(strs, (_z_string_t *)&keyexpr->_suffix);
    }
    // Recursively go through all the RIDs
    while (id != Z_RESOURCE_ID_NONE) {
        _z_resource_t *res = __z_get_resource_by_id(xs, keyexpr->_mapping, id);
        if (res == NULL) {
            len = 0;
            break;
        }
        if (_z_keyexpr_has_suffix(&res->_key)) {
            len = len + _z_string_len(&res->_key._suffix);
            strs = _z_string_list_push(strs, &res->_key._suffix);
        }
        id = res->_key._id;
    }
    // Copy the data
    if (len != (size_t)0) {
        ret._suffix = _z_string_preallocate(len);
        if (_z_keyexpr_has_suffix(&ret)) {
            char *curr_ptr = (char *)_z_string_data(&ret._suffix);

            // Concatenate all the partial resource names
            _z_string_list_t *xstr = strs;
            while (xstr != NULL) {
                _z_string_t *s = _z_string_list_value(xstr);
                memcpy(curr_ptr, _z_string_data(s), _z_string_len(s));
                curr_ptr = _z_ptr_char_offset(curr_ptr, (ptrdiff_t)_z_string_len(s));
                xstr = _z_string_list_next(xstr);
            }
        }
    }
    // Warning: list must be released with _z_list_free(&strs, _z_noop_free) or will release the suffix as well
    _z_list_free(&strs, _z_noop_free);
    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
static _z_resource_t *__unsafe_z_get_resource_by_id(_z_session_t *zn, _z_zint_t id, _z_transport_peer_common_t *peer) {
    _z_resource_slist_t *decls = zn->_local_resources;
    uintptr_t mapping = _Z_KEYEXPR_MAPPING_LOCAL;
    if (peer != NULL) {
        decls = peer->_remote_resources;
        mapping = (uintptr_t)peer;
    }
    return __z_get_resource_by_id(decls, mapping, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_resource_t *__unsafe_z_get_resource_by_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr,
                                              _z_transport_peer_common_t *peer) {
    _z_resource_slist_t *decls = _z_keyexpr_is_local(keyexpr) ? zn->_local_resources : peer->_remote_resources;
    return __z_get_resource_by_key(decls, keyexpr);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_keyexpr_t __unsafe_z_get_expanded_key_from_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr, bool force_alias,
                                                  _z_transport_peer_common_t *peer) {
    _z_resource_slist_t *decls =
        (_z_keyexpr_is_local(keyexpr) || (peer == NULL)) ? zn->_local_resources : peer->_remote_resources;
    return __z_get_expanded_key_from_key(decls, keyexpr, force_alias);
}

_z_resource_t *_z_get_resource_by_id(_z_session_t *zn, _z_zint_t rid, _z_transport_peer_common_t *peer) {
    _z_session_mutex_lock(zn);
    _z_resource_t *res = __unsafe_z_get_resource_by_id(zn, rid, peer);
    _z_session_mutex_unlock(zn);
    return res;
}

_z_resource_t *_z_get_resource_by_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr, _z_transport_peer_common_t *peer) {
    if (!_z_keyexpr_has_suffix(keyexpr)) {
        return _z_get_resource_by_id(zn, keyexpr->_id, peer);
    }
    _z_session_mutex_lock(zn);

    _z_resource_t *res = __unsafe_z_get_resource_by_key(zn, keyexpr, peer);

    _z_session_mutex_unlock(zn);

    return res;
}

_z_keyexpr_t _z_get_expanded_key_from_key(_z_session_t *zn, const _z_keyexpr_t *keyexpr,
                                          _z_transport_peer_common_t *peer) {
    _z_session_mutex_lock(zn);
    _z_keyexpr_t res = __unsafe_z_get_expanded_key_from_key(zn, keyexpr, false, peer);

    _z_session_mutex_unlock(zn);

    return res;
}

/// Returns the ID of the registered keyexpr. Returns 0 if registration failed.
uint16_t _z_register_resource(_z_session_t *zn, const _z_keyexpr_t *key, uint16_t id,
                              _z_transport_peer_common_t *peer) {
    uint16_t ret = Z_RESOURCE_ID_NONE;
    uintptr_t mapping = (peer == NULL) ? _Z_KEYEXPR_MAPPING_LOCAL : (uintptr_t)peer;
    uintptr_t parent_mapping = key->_mapping;
    _z_keyexpr_t full_ke = _z_keyexpr_alias(key);
    _Z_DEBUG("registering: key: %.*s id: %d, mapping: %d", (int)_z_string_len(&key->_suffix),
             _z_string_data(&key->_suffix), id, (unsigned int)mapping);

    _z_session_mutex_lock(zn);
    if (key->_id != Z_RESOURCE_ID_NONE) {
        if (parent_mapping == mapping) {
            _z_resource_t *parent = __unsafe_z_get_resource_by_id(zn, key->_id, peer);
            parent->_refcount++;
        } else {
            full_ke = __unsafe_z_get_expanded_key_from_key(zn, key, false, peer);
        }
    }
    ret = full_ke._id;
    if (_z_keyexpr_has_suffix(&full_ke)) {
        _z_resource_t *res = NULL;
        // Register the resource
        if ((peer != NULL) && (mapping != _Z_KEYEXPR_MAPPING_LOCAL)) {
            peer->_remote_resources = _z_resource_slist_push_empty(peer->_remote_resources);
            res = _z_resource_slist_value(peer->_remote_resources);
        } else {
            zn->_local_resources = _z_resource_slist_push_empty(zn->_local_resources);
            res = _z_resource_slist_value(zn->_local_resources);
        }
        res->_refcount = 1;
        _z_keyexpr_copy(&res->_key, &full_ke);
        ret = (id == Z_RESOURCE_ID_NONE) ? _z_get_resource_id(zn) : id;
        res->_id = ret;
    }
    _z_session_mutex_unlock(zn);

    return ret;
}

void _z_unregister_resource(_z_session_t *zn, uint16_t id, _z_transport_peer_common_t *peer) {
    bool is_local = true;
    uintptr_t mapping = _Z_KEYEXPR_MAPPING_LOCAL;
    if (peer != NULL) {
        is_local = false;
        mapping = (uintptr_t)peer;
    }
    _Z_DEBUG("unregistering: id %d, mapping: %d", id, (unsigned int)mapping);
    _z_session_mutex_lock(zn);
    _z_resource_slist_t **resources = is_local ? &zn->_local_resources : &peer->_remote_resources;
    while (id != 0) {
        _z_resource_slist_t *prev = NULL;
        _z_resource_slist_t *head = *resources;
        while (head != NULL) {
            _z_resource_t *value = _z_resource_slist_value(head);

            if ((value != NULL) && (value->_id == id) && (value->_key._mapping == mapping)) {
                value->_refcount--;
                if (value->_refcount == 0) {
                    id = value->_key._id;
                    mapping = value->_key._mapping;
                    *resources = _z_resource_slist_drop_element(*resources, prev);
                } else {
                    id = 0;
                }
                break;
            }
            prev = head;
            head = _z_resource_slist_next(head);
        }
        if (head == NULL) {
            id = 0;
        }
    }
    _z_session_mutex_unlock(zn);
}

void _z_flush_local_resources(_z_session_t *zn) {
    _z_session_mutex_lock(zn);
    _z_resource_slist_free(&zn->_local_resources);
    _z_session_mutex_unlock(zn);
}
