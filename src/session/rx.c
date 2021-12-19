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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "zenoh-pico/collections/list.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/session/session.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Handle message ------------------*/
int _zn_handle_zenoh_message(zn_session_t *zn, _zn_zenoh_message_t *msg)
{
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_DATA:
    {
        _Z_DEBUG_VA("Received _ZN_MID_DATA message %d\n", msg->header);
        if (msg->reply_context) // This is some data from a query
            _zn_trigger_query_reply_partial(zn, msg->reply_context, msg->body.data.key, msg->body.data.payload, msg->body.data.info);
        else // This is pure data
            _zn_trigger_subscriptions(zn, msg->body.data.key, msg->body.data.payload);

        return _z_res_t_OK;
    }

    case _ZN_MID_DECLARE:
    {
        _Z_DEBUG("Received _ZN_DECLARE message\n");
        for (unsigned int i = 0; i < msg->body.declare.declarations.len; i++)
        {
            _zn_declaration_t decl = msg->body.declare.declarations.val[i];
            switch (_ZN_MID(decl.header))
            {
            case _ZN_DECL_RESOURCE:
            {
                _Z_DEBUG("Received declare-resource message\n");

                z_zint_t id = decl.body.res.id;
                zn_reskey_t key = decl.body.res.key;

                // Register remote resource declaration
                _zn_resource_t *r = (_zn_resource_t *)malloc(sizeof(_zn_resource_t));
                r->id = id;
                r->key.rid = key.rid;
                r->key.rname = _z_str_clone(key.rname);

                int res = _zn_register_resource(zn, _ZN_IS_REMOTE, r);
                if (res != 0)
                {
                    _zn_resource_clear(r);
                    free(r);
                }

                break;
            }

            case _ZN_DECL_PUBLISHER:
            {
                // TODO: not supported yet
                break;
            }

            case _ZN_DECL_SUBSCRIBER:
            {
                z_str_t rname = _zn_get_resource_name_from_key(zn, _ZN_IS_REMOTE, &decl.body.sub.key);

                _zn_subscriber_list_t *subs = _zn_get_subscriptions_by_name(zn, _ZN_IS_REMOTE, rname);
                if (subs != NULL)
                {
                    _z_str_clear(rname);
                    break;
                }

                _zn_subscriber_t *rs = (_zn_subscriber_t *)malloc(sizeof(_zn_subscriber_t));
                rs->id = _zn_get_entity_id(zn);
                rs->rname = rname;
                rs->key = _zn_reskey_duplicate(&decl.body.sub.key);
                rs->info = decl.body.sub.subinfo;
                rs->callback = NULL;
                rs->arg = NULL;
                _zn_register_subscription(zn, _ZN_IS_REMOTE, rs);

                _z_list_free(&subs, _zn_noop_free);
                break;
            }
            case _ZN_DECL_QUERYABLE:
            {
                // TODO: not supported yet
                break;
            }
            case _ZN_DECL_FORGET_RESOURCE:
            {
                _zn_resource_t *rd = _zn_get_resource_by_id(zn, _ZN_IS_REMOTE, decl.body.forget_res.rid);
                if (rd != NULL)
                    _zn_unregister_resource(zn, _ZN_IS_REMOTE, rd);

                break;
            }
            case _ZN_DECL_FORGET_PUBLISHER:
            {
                // TODO: not supported yet
                break;
            }
            case _ZN_DECL_FORGET_SUBSCRIBER:
            {
                _zn_subscriber_list_t *subs = _zn_get_subscription_by_key(zn, _ZN_IS_REMOTE, &decl.body.forget_sub.key);

                _zn_subscriber_list_t *xs = subs;
                while (xs != NULL)
                {
                    _zn_subscriber_t *sub = _zn_subscriber_list_head(xs);
                    _zn_unregister_subscription(zn, _ZN_IS_REMOTE, sub);
                    xs = _zn_subscriber_list_tail(xs);
                }
                _z_list_free(&subs, _zn_noop_free);
                break;
            }
            case _ZN_DECL_FORGET_QUERYABLE:
            {
                // TODO: not supported yet
                break;
            }
            default:
            {
                return _z_res_t_ERR;
            }
            }
        }

        return _z_res_t_OK;
    }

    case _ZN_MID_PULL:
    {
        // TODO: not supported yet
        return _z_res_t_OK;
    }

    case _ZN_MID_QUERY:
    {
        _zn_trigger_queryables(zn, &msg->body.query);
        return _z_res_t_OK;
    }

    case _ZN_MID_UNIT:
    {
        // This might be the final reply
        if (msg->reply_context)
            _zn_trigger_query_reply_final(zn, msg->reply_context);

        return _z_res_t_OK;
    }

    default:
    {
        _Z_DEBUG("Unknown zenoh message ID");
        return _z_res_t_ERR;
    }
    }
}
