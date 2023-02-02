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

#include "zenoh-pico/config.h"
#include "zenoh-pico/utils/logging.h"

_Bool _z_resource_eq(const _z_resource_t *other, const _z_resource_t *this) { return this->_id == other->_id; }

void _z_resource_clear(_z_resource_t *res) { _z_keyexpr_clear(&res->_key); }

void _z_resource_free(_z_resource_t **res) {
    _z_resource_t *ptr = *res;

    if (ptr != NULL) {
        _z_resource_clear(ptr);

        z_free(ptr);
        *res = NULL;
    }
}

/*------------------ Entity ------------------*/
_z_zint_t _z_get_entity_id(_z_session_t *zn) { return zn->_entity_id++; }

_z_zint_t _z_get_resource_id(_z_session_t *zn) { return zn->_resource_id++; }

/*------------------ Resource ------------------*/
_z_resource_t *__z_get_resource_by_id(_z_resource_list_t *rl, const _z_zint_t id) {
    _z_resource_t *ret = NULL;

    _z_resource_list_t *xs = rl;
    while (xs != NULL) {
        _z_resource_t *r = _z_resource_list_head(xs);
        if (r->_id == id) {
            ret = r;
            break;
        }

        xs = _z_resource_list_tail(xs);
    }

    return ret;
}

_z_resource_t *__z_get_resource_by_key(_z_resource_list_t *rl, const _z_keyexpr_t *keyexpr) {
    _z_resource_t *ret = NULL;

    _z_resource_list_t *xs = rl;
    while (xs != NULL) {
        _z_resource_t *r = _z_resource_list_head(xs);
        if ((r->_key._id == keyexpr->_id) && (_z_str_eq(r->_key._suffix, keyexpr->_suffix) == true)) {
            ret = r;
            break;
        }

        xs = _z_resource_list_tail(xs);
    }

    return ret;
}

_z_keyexpr_t __z_get_expanded_key_from_key(_z_resource_list_t *xs, const _z_keyexpr_t *keyexpr) {
    _z_keyexpr_t ret = {._id = Z_RESOURCE_ID_NONE, ._suffix = NULL};

    // Need to build the complete resource name, by recursively look at RIDs
    // Resource names are looked up from right to left
    _z_str_list_t *strs = NULL;
    size_t len = 1;  // Start with space for the null-terminator

    // Append suffix as the right-most segment
    if (keyexpr->_suffix != NULL) {
        len = len + strlen(keyexpr->_suffix);
        strs = _z_str_list_push(strs, (char *)keyexpr->_suffix);  // Warning: list must be release with
                                                                  //   _z_list_free(&strs, _z_noop_free);
                                                                  //   or will release the suffix as well
    }

    // Recursevely go through all the RIDs
    _z_zint_t id = keyexpr->_id;
    while (id != Z_RESOURCE_ID_NONE) {
        _z_resource_t *res = __z_get_resource_by_id(xs, id);
        if (res == NULL) {
            len = 0;
            break;
        }

        if (res->_key._suffix != NULL) {
            len = len + strlen(res->_key._suffix);
            strs = _z_str_list_push(strs, (char *)res->_key._suffix);  // Warning: list must be release with
                                                                       //   _z_list_free(&strs, _z_noop_free);
                                                                       //   or will release the suffix as well
        }
        id = res->_key._id;
    }

    if (len != (size_t)0) {
        char *rname = NULL;
        rname = (char *)z_malloc(len);
        if (rname != NULL) {
            rname[0] = '\0';  // NULL terminator must be set (required to strcat)
            len = len - (size_t)1;

            // Concatenate all the partial resource names
            _z_str_list_t *xstr = strs;
            while (xstr != NULL) {
                char *s = _z_str_list_head(xstr);
                if (len > (size_t)0) {
                    (void)strncat(rname, s, len);
                    len = len - strlen(s);
                }
                xstr = _z_str_list_tail(xstr);
            }
            ret._suffix = rname;
        }
    }

    _z_list_free(&strs, _z_noop_free);

    return ret;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_resource_t *__unsafe_z_get_resource_by_id(_z_session_t *zn, uint8_t is_local, _z_zint_t id) {
    _z_resource_list_t *decls = (is_local == _Z_RESOURCE_IS_LOCAL) ? zn->_local_resources : zn->_remote_resources;
    return __z_get_resource_by_id(decls, id);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_resource_t *__unsafe_z_get_resource_by_key(_z_session_t *zn, uint8_t is_local, const _z_keyexpr_t *keyexpr) {
    _z_resource_list_t *decls = (is_local == _Z_RESOURCE_IS_LOCAL) ? zn->_local_resources : zn->_remote_resources;
    return __z_get_resource_by_key(decls, keyexpr);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->_mutex_inner
 */
_z_keyexpr_t __unsafe_z_get_expanded_key_from_key(_z_session_t *zn, uint8_t is_local, const _z_keyexpr_t *keyexpr) {
    _z_resource_list_t *decls = (is_local == _Z_RESOURCE_IS_LOCAL) ? zn->_local_resources : zn->_remote_resources;
    return __z_get_expanded_key_from_key(decls, keyexpr);
}

_z_resource_t *_z_get_resource_by_id(_z_session_t *zn, uint8_t is_local, _z_zint_t rid) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_resource_t *res = __unsafe_z_get_resource_by_id(zn, is_local, rid);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return res;
}

_z_resource_t *_z_get_resource_by_key(_z_session_t *zn, uint8_t is_local, const _z_keyexpr_t *keyexpr) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_resource_t *res = __unsafe_z_get_resource_by_key(zn, is_local, keyexpr);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return res;
}

_z_keyexpr_t _z_get_expanded_key_from_key(_z_session_t *zn, uint8_t is_local, const _z_keyexpr_t *keyexpr) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_keyexpr_t res = __unsafe_z_get_expanded_key_from_key(zn, is_local, keyexpr);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return res;
}

int8_t _z_register_resource(_z_session_t *zn, uint8_t is_local, _z_resource_t *res) {
    int8_t ret = _Z_RES_OK;

    _Z_DEBUG(">>> Allocating res decl for (%zu,%zu:%s)\n", res->_id, res->_key._id, res->_key._suffix);

#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    // FIXME: check by keyexpr instead
    _z_resource_t *r = __unsafe_z_get_resource_by_id(zn, is_local, res->_id);
    if (r == NULL) {
        // Register the resource
        if (is_local == _Z_RESOURCE_IS_LOCAL) {
            zn->_local_resources = _z_resource_list_push(zn->_local_resources, res);
        } else {
            zn->_remote_resources = _z_resource_list_push(zn->_remote_resources, res);
        }
    } else {
        ret = _Z_ERR_ENTITY_DECLARATION_FAILED;
    }

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    return ret;
}

void _z_unregister_resource(_z_session_t *zn, uint8_t is_local, _z_resource_t *res) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    if (is_local == _Z_RESOURCE_IS_LOCAL) {
        zn->_local_resources = _z_resource_list_drop_filter(zn->_local_resources, _z_resource_eq, res);
    } else {
        zn->_remote_resources = _z_resource_list_drop_filter(zn->_remote_resources, _z_resource_eq, res);
    }

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
}

void _z_flush_resources(_z_session_t *zn) {
#if Z_MULTI_THREAD == 1
    _z_mutex_lock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1

    _z_resource_list_free(&zn->_local_resources);
    _z_resource_list_free(&zn->_remote_resources);

#if Z_MULTI_THREAD == 1
    _z_mutex_unlock(&zn->_mutex_inner);
#endif  // Z_MULTI_THREAD == 1
}
