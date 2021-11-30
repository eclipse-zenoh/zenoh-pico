/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include "zenoh-pico/collections/element.h"
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Entity ------------------*/
z_zint_t _zn_get_entity_id(zn_session_t *zn)
{
    return zn->entity_id++;
}

/*------------------ Resource ------------------*/
/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_resource_t *__unsafe_zn_get_resource_by_id(zn_session_t *zn, int is_local, z_zint_t id)
{
    _z_list_t *decls = is_local ? zn->local_resources : zn->remote_resources;
    while (decls)
    {
        _zn_resource_t *decl = (_zn_resource_t *)_z_list_head(decls);

        if (decl->id == id)
            return decl;

        decls = _z_list_tail(decls);
    }

    return NULL;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_resource_t *__unsafe_zn_get_resource_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_list_t *decls = is_local ? zn->local_resources : zn->remote_resources;
    while (decls)
    {
        _zn_resource_t *decl = (_zn_resource_t *)_z_list_head(decls);

        if (decl->key.rid == reskey->rid && strcmp(decl->key.rname, reskey->rname) == 0)
            return decl;

        decls = _z_list_tail(decls);
    }

    return NULL;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
z_str_t __unsafe_zn_get_resource_name_from_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    z_str_t rname = NULL;

    // Case 2) -> string only reskey, clonelicate the rname
    if (reskey->rid == ZN_RESOURCE_ID_NONE)
    {
        rname = _z_str_clone(reskey->rname);
        return rname;
    }

    // Need to build the complete resource name
    _z_list_t *strs = NULL;
    size_t len = 0;

    if (reskey->rname)
    {
        // Case 3) -> numerical reskey with suffix, same as Case 1) but we first append the suffix
        len += strlen(reskey->rname);
        strs = _z_list_push(strs, (void *)reskey->rname);
    }

    // Case 1) -> numerical only reskey
    z_zint_t id = reskey->rid;
    do
    {
        _zn_resource_t *res = __unsafe_zn_get_resource_by_id(zn, is_local, id);
        if (res == NULL)
        {
            _z_list_free(&strs, _zn_noop_elem_free);
            return rname;
        }

        if (res->key.rname)
        {
            len += strlen(res->key.rname);
            strs = _z_list_push(strs, (void *)res->key.rname);
        }

        id = res->key.rid;
    } while (id != ZN_RESOURCE_ID_NONE);

    // Concatenate all the partial resource names
    rname = (z_str_t)malloc(len + 1);
    // Start with a zero-length string to concatenate upon
    rname[0] = '\0';
    while (strs)
    {
        z_str_t s = (z_str_t)_z_list_head(strs);
        strcat(rname, s);
        strs = _z_list_tail(strs);
    }

    return rname;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
_zn_resource_t *__unsafe_zn_get_resource_matching_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    _z_list_t *decls = is_local ? zn->local_resources : zn->remote_resources;

    z_str_t rname;
    if (reskey->rid == ZN_RESOURCE_ID_NONE)
        rname = reskey->rname;
    else
        rname = __unsafe_zn_get_resource_name_from_key(zn, is_local, reskey);

    while (decls)
    {
        _zn_resource_t *decl = (_zn_resource_t *)_z_list_head(decls);

        z_str_t lname;
        if (decl->key.rid == ZN_RESOURCE_ID_NONE)
            lname = decl->key.rname;
        else
            lname = __unsafe_zn_get_resource_name_from_key(zn, is_local, &decl->key);

        // Verify if it intersects
        int res = zn_rname_intersect(lname, rname);

        // Free the resource key if allocated
        if (decl->key.rid != ZN_RESOURCE_ID_NONE)
            free(lname);

        // Exit if it inersects
        if (res)
        {
            if (reskey->rid != ZN_RESOURCE_ID_NONE)
                free(rname);
            return decl;
        }

        decls = _z_list_tail(decls);
    }

    if (reskey->rid != ZN_RESOURCE_ID_NONE)
        free(rname);

    return NULL;
}

z_zint_t _zn_get_resource_id(zn_session_t *zn)
{
    return zn->resource_id++;
}

_zn_resource_t *_zn_get_resource_by_id(zn_session_t *zn, int is_local, z_zint_t rid)
{
    // Lock the resources data struct
    z_mutex_lock(&zn->mutex_inner);

    _zn_resource_t *res = __unsafe_zn_get_resource_by_id(zn, is_local, rid);

    // Release the lock
    z_mutex_unlock(&zn->mutex_inner);
    return res;
}

_zn_resource_t *_zn_get_resource_by_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_resource_t *res = __unsafe_zn_get_resource_by_key(zn, is_local, reskey);
    z_mutex_unlock(&zn->mutex_inner);
    return res;
}

z_str_t _zn_get_resource_name_from_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    z_mutex_lock(&zn->mutex_inner);
    z_str_t res = __unsafe_zn_get_resource_name_from_key(zn, is_local, reskey);
    z_mutex_unlock(&zn->mutex_inner);
    return res;
}

_zn_resource_t *_zn_get_resource_matching_key(zn_session_t *zn, int is_local, const zn_reskey_t *reskey)
{
    z_mutex_lock(&zn->mutex_inner);
    _zn_resource_t *res = __unsafe_zn_get_resource_matching_key(zn, is_local, reskey);
    z_mutex_unlock(&zn->mutex_inner);
    return res;
}

int _zn_register_resource(zn_session_t *zn, int is_local, _zn_resource_t *res)
{
    _Z_DEBUG_VA(">>> Allocating res decl for (%zu,%lu,%s)\n", res->id, res->key.rid, res->key.rname);

    // Lock the resources data struct
    z_mutex_lock(&zn->mutex_inner);

    _zn_resource_t *rd_rid = __unsafe_zn_get_resource_by_id(zn, is_local, res->id);

    int r;
    if (rd_rid)
    {
        // Inconsistent declarations have been found, return an error
        r = -1;
    }
    else
    {
        // No resource declaration has been found, add the new one
        if (is_local)
        {
            zn->local_resources = _z_list_push(zn->local_resources, res);
        }
        else
        {
            __unsafe_zn_add_rem_res_to_loc_sub_map(zn, res->id, &res->key);
            __unsafe_zn_add_rem_res_to_loc_qle_map(zn, res->id, &res->key);
            zn->remote_resources = _z_list_push(zn->remote_resources, res);
        }

        r = 0;
    }

    // Release the lock
    z_mutex_unlock(&zn->mutex_inner);

    return r;
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
void __unsafe_zn_free_resource(_zn_resource_t *res)
{
    _zn_reskey_clear(&res->key);
}

/**
 * This function is unsafe because it operates in potentially concurrent data.
 * Make sure that the following mutexes are locked before calling this function:
 *  - zn->mutex_inner
 */
int __unsafe_zn_resource_predicate(const void *other, const void *this)
{
    _zn_resource_t *o = (_zn_resource_t *)other;
    _zn_resource_t *t = (_zn_resource_t *)this;
    if (t->id == o->id)
    {
        __unsafe_zn_free_resource(t);
        return 1;
    }
    else
    {
        return 0;
    }
}

void _zn_unregister_resource(zn_session_t *zn, int is_local, _zn_resource_t *res)
{
    // Lock the resources data struct
    z_mutex_lock(&zn->mutex_inner);

    if (is_local)
        zn->local_resources = _z_list_drop_filter(zn->local_resources, _zn_noop_elem_free, __unsafe_zn_resource_predicate, res);
    else
        zn->remote_resources = _z_list_drop_filter(zn->remote_resources, _zn_noop_elem_free, __unsafe_zn_resource_predicate, res);
    free(res);

    // Release the lock
    z_mutex_unlock(&zn->mutex_inner);
}

void _zn_flush_resources(zn_session_t *zn)
{
    // Lock the resources data struct
    z_mutex_lock(&zn->mutex_inner);

    while (zn->local_resources)
    {
        _zn_resource_t *res = (_zn_resource_t *)_z_list_head(zn->local_resources);
        __unsafe_zn_free_resource(res);
        free(res);
        zn->local_resources = _z_list_pop(zn->local_resources, _zn_noop_elem_free);
    }

    while (zn->remote_resources)
    {
        _zn_resource_t *res = (_zn_resource_t *)_z_list_head(zn->remote_resources);
        __unsafe_zn_free_resource(res);
        free(res);
        zn->remote_resources = _z_list_pop(zn->remote_resources, _zn_noop_elem_free);
    }

    // Release the lock
    z_mutex_unlock(&zn->mutex_inner);
}
