/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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
#include "zenoh-pico/net.h"
#include "zenoh-pico/net/private/internal.h"
#include "zenoh-pico/net/private/msgcodec.h"
#include "zenoh-pico/net/private/system.h"
#include "zenoh-pico/private/collection.h"
#include "zenoh-pico/private/logging.h"

/*=============================*/
/*          Private            */
/*=============================*/
/*------------------ Init/Free/Close session ------------------*/
zn_session_t *_zn_session_init()
{
    zn_session_t *zn = (zn_session_t *)malloc(sizeof(zn_session_t));

    // Initialize the read and write buffers
    zn->wbuf = _z_wbuf_make(ZN_WRITE_BUF_LEN, 0);
    zn->rbuf = _z_rbuf_make(ZN_READ_BUF_LEN);

    // Initialize the defragmentation buffers
    zn->dbuf_reliable = _z_wbuf_make(0, 1);
    zn->dbuf_best_effort = _z_wbuf_make(0, 1);

    // Initialize the mutexes
    _z_mutex_init(&zn->mutex_rx);
    _z_mutex_init(&zn->mutex_tx);
    _z_mutex_init(&zn->mutex_inner);

    // The initial SN at RX side
    zn->lease = 0;
    zn->sn_resolution = 0;

    // The initial SN at RX side
    zn->sn_rx_reliable = 0;
    zn->sn_rx_best_effort = 0;

    // The initial SN at TX side
    zn->sn_tx_reliable = 0;
    zn->sn_tx_best_effort = 0;

    // Initialize the counters to 1
    zn->entity_id = 1;
    zn->resource_id = 1;
    zn->query_id = 1;
    zn->pull_id = 1;

    // Initialize the data structs
    zn->local_resources = _z_list_empty;
    zn->remote_resources = _z_list_empty;

    zn->local_subscriptions = _z_list_empty;
    zn->remote_subscriptions = _z_list_empty;
    zn->rem_res_loc_sub_map = _z_i_map_make(_Z_DEFAULT_I_MAP_CAPACITY);

    zn->local_queryables = _z_list_empty;
    zn->rem_res_loc_qle_map = _z_i_map_make(_Z_DEFAULT_I_MAP_CAPACITY);

    zn->pending_queries = _z_list_empty;

    zn->read_task_running = 0;
    zn->read_task = NULL;

    zn->received = 0;
    zn->transmitted = 0;
    zn->lease_task_running = 0;
    zn->lease_task = NULL;

    return zn;
}

void _zn_session_free(zn_session_t *zn)
{
    // Close the socket
    _zn_close_tx_session(zn->sock);

    // Clean up the entities
    _zn_flush_resources(zn);
    _zn_flush_subscriptions(zn);
    _zn_flush_queryables(zn);
    _zn_flush_pending_queries(zn);

    // Clean up the mutexes
    _z_mutex_free(&zn->mutex_inner);
    _z_mutex_free(&zn->mutex_tx);
    _z_mutex_free(&zn->mutex_rx);

    // Clean up the buffers
    _z_wbuf_free(&zn->wbuf);
    _z_rbuf_free(&zn->rbuf);

    _z_wbuf_free(&zn->dbuf_reliable);
    _z_wbuf_free(&zn->dbuf_best_effort);

    // Clean up the PIDs
    _z_bytes_free(&zn->local_pid);
    _z_bytes_free(&zn->remote_pid);

    // Clean up the locator
    free(zn->locator);

    // Clean up the tasks
    free(zn->read_task);
    free(zn->lease_task);

    free(zn);

    zn = NULL;
}

int _zn_send_close(zn_session_t *zn, uint8_t reason, int link_only)
{
    _zn_session_message_t cm = _zn_session_message_init(_ZN_MID_CLOSE);
    cm.body.close.pid = zn->local_pid;
    cm.body.close.reason = reason;
    _ZN_SET_FLAG(cm.header, _ZN_FLAG_S_I);
    if (link_only)
        _ZN_SET_FLAG(cm.header, _ZN_FLAG_S_K);

    int res = _zn_send_s_msg(zn, &cm);

    // Free the message
    _zn_session_message_free(&cm);

    return res;
}

int _zn_session_close(zn_session_t *zn, uint8_t reason)
{
    int res = _zn_send_close(zn, reason, 0);
    // Free the session
    _zn_session_free(zn);

    return res;
}

void _zn_default_on_disconnect(void *vz)
{
    zn_session_t *zn = (zn_session_t *)vz;
    for (int i = 0; i < 3; i++)
    {
        _z_sleep_s(3);
        // Try to reconnect -- eventually we should scout here.
        // We should also re-do declarations.
        _Z_DEBUG("Tring to reconnect...\n");
        _zn_socket_result_t r_sock = _zn_open_tx_session(zn->locator);
        if (r_sock.tag == _z_res_t_OK)
        {
            zn->sock = r_sock.value.socket;
            return;
        }
    }
}

/*------------------ Scout ------------------*/
zn_hello_array_t _zn_scout_loop(
    _zn_socket_t socket,
    const _z_wbuf_t *wbf,
    const struct sockaddr *dest,
    socklen_t salen,
    clock_t period,
    int exit_on_first)
{
    // Define an empty array
    zn_hello_array_t ls;
    ls.len = 0;
    ls.val = NULL;

    // Send the scout message
    int res = _zn_send_dgram_to(socket, wbf, dest, salen);
    if (res <= 0)
    {
        _Z_DEBUG("Unable to send scout message\n");
        return ls;
    }

    // @TODO: need to abstract the platform-specific data types
    struct sockaddr *from = (struct sockaddr *)malloc(2 * sizeof(struct sockaddr_in *));
    socklen_t flen = 0;

    // The receiving buffer
    _z_rbuf_t rbf = _z_rbuf_make(ZN_READ_BUF_LEN);

    _z_clock_t start = _z_clock_now();
    while (_z_clock_elapsed_ms(&start) < period)
    {
        // Eventually read hello messages
        _z_rbuf_clear(&rbf);
        int len = _zn_recv_dgram_from(socket, &rbf, from, &flen);

        // Retry if we haven't received anything
        if (len <= 0)
            continue;

        _zn_session_message_p_result_t r_hm = _zn_session_message_decode(&rbf);
        if (r_hm.tag == _z_res_t_ERR)
        {
            _Z_DEBUG("Scouting loop received malformed message\n");
            continue;
        }

        _zn_session_message_t *s_msg = r_hm.value.session_message;
        switch (_ZN_MID(s_msg->header))
        {
        case _ZN_MID_HELLO:
        {
            // Allocate or expand the vector
            if (ls.val)
            {
                zn_hello_t *val = (zn_hello_t *)malloc((ls.len + 1) * sizeof(zn_hello_t));
                memcpy(val, ls.val, ls.len);
                free((zn_hello_t *)ls.val);
                ls.val = val;
            }
            else
            {
                ls.val = (zn_hello_t *)malloc(sizeof(zn_hello_t));
            }
            ls.len++;

            // Get current element to fill
            zn_hello_t *sc = (zn_hello_t *)&ls.val[ls.len - 1];

            if _ZN_HAS_FLAG (s_msg->header, _ZN_FLAG_S_I)
                _z_bytes_copy(&sc->pid, &s_msg->body.hello.pid);
            else
                _z_bytes_reset(&sc->pid);

            if _ZN_HAS_FLAG (s_msg->header, _ZN_FLAG_S_W)
                sc->whatami = s_msg->body.hello.whatami;
            else
                sc->whatami = ZN_ROUTER; // Default value is from a router

            if _ZN_HAS_FLAG (s_msg->header, _ZN_FLAG_S_L)
            {
                _z_str_array_copy(&sc->locators, &s_msg->body.hello.locators);
            }
            else
            {
                // @TODO: construct the locator departing from the sock address
                sc->locators.len = 0;
                sc->locators.val = NULL;
            }

            break;
        }

        default:
        {
            _Z_DEBUG("Scouting loop received unexpected message\n");
            break;
        }
        }

        _zn_session_message_free(s_msg);
        _zn_session_message_p_result_free(&r_hm);

        if (ls.len > 0 && exit_on_first)
            break;
    }

    free(from);
    _z_rbuf_free(&rbf);

    return ls;
}

zn_hello_array_t _zn_scout(unsigned int what, zn_properties_t *config, unsigned long scout_period, int exit_on_first)
{
    zn_hello_array_t locs;
    locs.len = 0;
    locs.val = NULL;

    z_str_t addr = _zn_select_scout_iface();
    _zn_socket_result_t r = _zn_create_udp_socket(addr, 0, scout_period);
    if (r.tag == _z_res_t_ERR)
    {
        _Z_DEBUG("Unable to create scouting socket\n");
        free(addr);
        return locs;
    }

    // Create the buffer to serialize the scout message on
    _z_wbuf_t wbf = _z_wbuf_make(ZN_WRITE_BUF_LEN, 0);

    // Create and encode the scout message
    _zn_session_message_t scout = _zn_session_message_init(_ZN_MID_SCOUT);
    // Ask for peer ID to be put in the scout message
    _ZN_SET_FLAG(scout.header, _ZN_FLAG_S_I);
    scout.body.scout.what = what;
    if (what != ZN_ROUTER)
        _ZN_SET_FLAG(scout.header, _ZN_FLAG_S_W);

    _zn_session_message_encode(&wbf, &scout);

    // Scout on multicast
    z_str_t sock_addr = strdup(zn_properties_get(config, ZN_CONFIG_MULTICAST_ADDRESS_KEY).val);
    z_str_t ip_addr = strtok(sock_addr, ":");
    int port_num = atoi(strtok(NULL, ":"));

    struct sockaddr_in *maddr = _zn_make_socket_address(ip_addr, port_num);
    socklen_t salen = sizeof(struct sockaddr_in);
    locs = _zn_scout_loop(r.value.socket, &wbf, (struct sockaddr *)maddr, salen, scout_period, exit_on_first);
    free(maddr);

    free(sock_addr);
    free(addr);
    _z_wbuf_free(&wbf);

    return locs;
}

/*------------------ Handle message ------------------*/
int _zn_handle_zenoh_message(zn_session_t *zn, _zn_zenoh_message_t *msg)
{
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_DATA:
    {
        _Z_DEBUG_VA("Received _ZN_MID_DATA message %d\n", msg->header);
        if (msg->reply_context)
        {
            // This is some data from a query
            _zn_trigger_query_reply_partial(zn, msg->reply_context, msg->body.data.key, msg->body.data.payload, msg->body.data.info);
        }
        else
        {
            // This is pure data
            _zn_trigger_subscriptions(zn, msg->body.data.key, msg->body.data.payload);
        }
        return _z_res_t_OK;
    }

    case _ZN_MID_DECLARE:
    {
        _Z_DEBUG("Received _ZN_DECLARE message\n");
        for (unsigned int i = 0; i < msg->body.declare.declarations.len; ++i)
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
                r->key.rname = strdup(key.rname);

                int res = _zn_register_resource(zn, _ZN_IS_REMOTE, r);
                if (res != 0)
                {
                    _zn_reskey_free(&r->key);
                    free(r);
                }

                break;
            }

            case _ZN_DECL_PUBLISHER:
            {
                // Check if there are matching local subscriptions
                _z_list_t *subs = _zn_get_subscriptions_from_remote_key(zn, &msg->body.data.key);
                unsigned int len = _z_list_len(subs);
                if (len > 0)
                {
                    // Need to reply with a declare subscriber
                    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);
                    z_msg.body.declare.declarations.len = len;
                    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

                    while (subs)
                    {
                        _zn_subscriber_t *sub = (_zn_subscriber_t *)_z_list_head(subs);

                        z_msg.body.declare.declarations.val[len].header = _ZN_DECL_SUBSCRIBER;
                        z_msg.body.declare.declarations.val[len].body.sub.key = sub->key;
                        z_msg.body.declare.declarations.val[len].body.sub.subinfo = sub->info;

                        subs = _z_list_pop(subs);
                    }

                    // Send the message
                    _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);

                    // Free the message
                    _zn_zenoh_message_free(&z_msg);
                }
                break;
            }

            case _ZN_DECL_SUBSCRIBER:
            {
                _zn_subscriber_t *sub = _zn_get_subscription_by_key(zn, _ZN_IS_REMOTE, &decl.body.sub.key);
                if (sub)
                {
                    _zn_subscriber_t rs;
                    rs.id = _zn_get_entity_id(zn);
                    rs.key = decl.body.sub.key;
                    rs.info = decl.body.sub.subinfo;
                    rs.callback = NULL;
                    rs.arg = NULL;
                    _zn_register_subscription(zn, _ZN_IS_REMOTE, &rs);
                }

                break;
            }
            case _ZN_DECL_QUERYABLE:
            {
                // Do nothing, zenoh clients are not expected to handle remote queryable declarations
                break;
            }
            case _ZN_DECL_FORGET_RESOURCE:
            {
                _zn_resource_t *rd = _zn_get_resource_by_id(zn, _ZN_IS_REMOTE, decl.body.forget_res.rid);
                if (rd)
                    _zn_unregister_resource(zn, _ZN_IS_REMOTE, rd);

                break;
            }
            case _ZN_DECL_FORGET_PUBLISHER:
            {
                // Do nothing, remote publishers are not stored in the session
                break;
            }
            case _ZN_DECL_FORGET_SUBSCRIBER:
            {
                _zn_subscriber_t *sub = _zn_get_subscription_by_key(zn, _ZN_IS_REMOTE, &decl.body.forget_sub.key);
                if (sub)
                    _zn_unregister_subscription(zn, _ZN_IS_REMOTE, sub);

                break;
            }
            case _ZN_DECL_FORGET_QUERYABLE:
            {
                // Do nothing, zenoh clients are not expected to handle remote queryable declarations
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
        // Do nothing, zenoh clients are not expected to handle pull messages
        return _z_res_t_OK;
    }

    case _ZN_MID_QUERY:
    {
        _zn_trigger_queryables(zn, &msg->body.query);
        return _z_res_t_OK;
    }

    case _ZN_MID_UNIT:
    {
        if (msg->reply_context)
        {
            // This might be the final reply
            _zn_trigger_query_reply_final(zn, msg->reply_context);
        }
        return _z_res_t_OK;
    }

    default:
    {
        _Z_DEBUG("Unknown zenoh message ID");
        return _z_res_t_ERR;
    }
    }
}

int _zn_handle_session_message(zn_session_t *zn, _zn_session_message_t *msg)
{
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_SCOUT:
    {
        _Z_DEBUG("Handling of Scout messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_HELLO:
    {
        // Do nothing, zenoh-pico clients are not expected to handle hello messages
        return _z_res_t_OK;
    }

    case _ZN_MID_OPEN:
    {
        // Do nothing, zenoh clients are not expected to handle open messages
        return _z_res_t_OK;
    }

    case _ZN_MID_ACCEPT:
    {
        // Do nothing, zenoh clients are not expected to handle accept messages on established sessions
        return _z_res_t_OK;
    }

    case _ZN_MID_CLOSE:
    {
        _Z_DEBUG("Closing session as requested by the remote peer");
        _zn_session_free(zn);
        return _z_res_t_ERR;
    }

    case _ZN_MID_SYNC:
    {
        _Z_DEBUG("Handling of Sync messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_ACK_NACK:
    {
        _Z_DEBUG("Handling of AckNack messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_KEEP_ALIVE:
    {
        return _z_res_t_OK;
    }

    case _ZN_MID_PING_PONG:
    {
        _Z_DEBUG("Handling of PingPong messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_FRAME:
    {
        // Check if the SN is correct
        if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_S_R))
        {
            // @TODO: amend once reliability is in place. For the time being only
            //        monothonic SNs are ensured
            if (_zn_sn_precedes(zn->sn_resolution_half, zn->sn_rx_reliable, msg->body.frame.sn))
            {
                zn->sn_rx_reliable = msg->body.frame.sn;
            }
            else
            {
                _z_wbuf_reset(&zn->dbuf_reliable);
                _Z_DEBUG("Reliable message dropped because it is out of order");
                return _z_res_t_OK;
            }
        }
        else
        {
            if (_zn_sn_precedes(zn->sn_resolution_half, zn->sn_rx_best_effort, msg->body.frame.sn))
            {
                zn->sn_rx_best_effort = msg->body.frame.sn;
            }
            else
            {
                _z_wbuf_reset(&zn->dbuf_best_effort);
                _Z_DEBUG("Best effort message dropped because it is out of order");
                return _z_res_t_OK;
            }
        }

        if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_S_F))
        {
            int res = _z_res_t_OK;

            // Select the right defragmentation buffer
            _z_wbuf_t *dbuf = _ZN_HAS_FLAG(msg->header, _ZN_FLAG_S_R) ? &zn->dbuf_reliable : &zn->dbuf_best_effort;
            // Add the fragment to the defragmentation buffer
            _z_wbuf_add_iosli_from(dbuf, msg->body.frame.payload.fragment.val, msg->body.frame.payload.fragment.len);

            // Check if this is the last fragment
            if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_S_E))
            {
                // Convert the defragmentation buffer into a decoding buffer
                _z_rbuf_t rbf = _z_wbuf_to_rbuf(dbuf);

                // Decode the zenoh message
                _zn_zenoh_message_p_result_t r_zm = _zn_zenoh_message_decode(&rbf);
                if (r_zm.tag == _z_res_t_OK)
                {
                    _zn_zenoh_message_t *d_zm = r_zm.value.zenoh_message;
                    res = _zn_handle_zenoh_message(zn, d_zm);
                    // Free the decoded message
                    _zn_zenoh_message_free(d_zm);
                }
                else
                {
                    res = _z_res_t_ERR;
                }

                // Free the result
                _zn_zenoh_message_p_result_free(&r_zm);
                // Free the decoding buffer
                _z_rbuf_free(&rbf);
                // Reset the defragmentation buffer
                _z_wbuf_reset(dbuf);
            }

            return res;
        }
        else
        {
            // Handle all the zenoh message, one by one
            unsigned int len = _z_vec_len(&msg->body.frame.payload.messages);
            for (unsigned int i = 0; i < len; ++i)
            {
                int res = _zn_handle_zenoh_message(zn, (_zn_zenoh_message_t *)_z_vec_get(&msg->body.frame.payload.messages, i));
                if (res != _z_res_t_OK)
                    return res;
            }
            return _z_res_t_OK;
        }
    }

    default:
    {
        _Z_DEBUG("Unknown session message ID");
        return _z_res_t_ERR;
    }
    }
}

/*=============================*/
/*           Public            */
/*=============================*/
void z_init_logger()
{
    // @TODO
}

/*------------------ Init/Config ------------------*/
zn_properties_t *zn_config_empty()
{
    return zn_properties_make();
}

zn_properties_t *zn_config_client(const char *locator)
{
    zn_properties_t *ps = zn_config_empty();
    zn_properties_insert(ps, ZN_CONFIG_MODE_KEY, z_string_make("client"));
    if (locator)
    {
        // Connect only to the provided locator
        zn_properties_insert(ps, ZN_CONFIG_PEER_KEY, z_string_make(locator));
    }
    else
    {
        // The locator is not provided, we should perform scouting
        zn_properties_insert(ps, ZN_CONFIG_MULTICAST_SCOUTING_KEY, z_string_make(ZN_CONFIG_MULTICAST_SCOUTING_DEFAULT));
        zn_properties_insert(ps, ZN_CONFIG_MULTICAST_ADDRESS_KEY, z_string_make(ZN_CONFIG_MULTICAST_ADDRESS_DEFAULT));
        zn_properties_insert(ps, ZN_CONFIG_MULTICAST_INTERFACE_KEY, z_string_make(ZN_CONFIG_MULTICAST_INTERFACE_DEFAULT));
        zn_properties_insert(ps, ZN_CONFIG_SCOUTING_TIMEOUT_KEY, z_string_make(ZN_CONFIG_SCOUTING_TIMEOUT_DEFAULT));
    }
    return ps;
}

zn_properties_t *zn_config_default()
{
    return zn_config_client(NULL);
}

/*------------------ Scout/Open/Close ------------------*/
void zn_hello_array_free(zn_hello_array_t hellos)
{
    zn_hello_t *h = (zn_hello_t *)hellos.val;
    if (h)
    {
        for (unsigned int i = 0; i < hellos.len; i++)
        {
            if (h[i].pid.len > 0)
                _z_bytes_free(&h[i].pid);
            if (h[i].locators.len > 0)
                _z_str_array_free(&h[i].locators);
        }

        free(h);
    }
}

zn_hello_array_t zn_scout(unsigned int what, zn_properties_t *config, unsigned long timeout)
{
    return _zn_scout(what, config, timeout, 0);
}

void zn_close(zn_session_t *zn)
{
    _zn_session_close(zn, _ZN_CLOSE_GENERIC);
    return;
}

zn_session_t *zn_open(zn_properties_t *config)
{
    zn_session_t *zn = NULL;

    int locator_is_scouted = 0;
    const char *locator = zn_properties_get(config, ZN_CONFIG_PEER_KEY).val;

    if (locator == NULL)
    {
        // Scout for routers
        unsigned int what = ZN_ROUTER;
        const char *mode = zn_properties_get(config, ZN_CONFIG_MODE_KEY).val;
        if (mode == NULL)
        {
            return zn;
        }

        const char *to = zn_properties_get(config, ZN_CONFIG_SCOUTING_TIMEOUT_KEY).val;
        if (to == NULL)
        {
            to = ZN_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
        }
        // The ZN_CONFIG_SCOUTING_TIMEOUT_KEY is expressed in seconds as a float while the
        // scout loop timeout uses milliseconds granularity
        clock_t timeout = (clock_t)1000 * strtof(to, NULL);
        // Scout and return upon the first result
        zn_hello_array_t locs = _zn_scout(what, config, timeout, 1);
        if (locs.len > 0)
        {
            if (locs.val[0].locators.len > 0)
            {
                locator = strdup(locs.val[0].locators.val[0]);
                // Mark that the locator has been scouted, need to be freed before returning
                locator_is_scouted = 1;
            }
            // Free all the scouted locators
            zn_hello_array_free(locs);
        }
        else
        {
            _Z_DEBUG("Unable to scout a zenoh router\n");
            _Z_ERROR("%sPlease make sure one is running on your network!\n", "");
            // Free all the scouted locators
            zn_hello_array_free(locs);

            return zn;
        }
    }

    // Initialize the PRNG
    srand(clock());

    // Attempt to open a socket
    _zn_socket_result_t r_sock = _zn_open_tx_session(locator);
    if (r_sock.tag == _z_res_t_ERR)
    {
        if (locator_is_scouted)
            free((char *)locator);

        return zn;
    }

    // Randomly generate a peer ID
    z_bytes_t pid = _z_bytes_make(ZN_PID_LENGTH);
    for (unsigned int i = 0; i < pid.len; i++)
        ((uint8_t *)pid.val)[i] = rand() % 255;

    // Build the open message
    _zn_session_message_t om = _zn_session_message_init(_ZN_MID_OPEN);

    om.body.open.version = ZN_PROTO_VERSION;
    om.body.open.whatami = ZN_CLIENT;
    om.body.open.opid = pid;
    om.body.open.lease = ZN_SESSION_LEASE;
    om.body.open.initial_sn = (z_zint_t)rand() % ZN_SN_RESOLUTION;
    om.body.open.sn_resolution = ZN_SN_RESOLUTION;

    if (ZN_SN_RESOLUTION != ZN_SN_RESOLUTION_DEFAULT)
        _ZN_SET_FLAG(om.header, _ZN_FLAG_S_S);
    // NOTE: optionally the open can include a list of locators the opener
    //       is reachable at. Since zenoh-pico acts as a client, this is not
    //       needed because a client is not expected to receive opens.

    // Initialize the session
    zn = _zn_session_init();
    zn->sock = r_sock.value.socket;

    _Z_DEBUG("Sending Open\n");
    // Encode and send the message
    _zn_send_s_msg(zn, &om);

    _zn_session_message_p_result_t r_msg = _zn_recv_s_msg(zn);
    if (r_msg.tag == _z_res_t_ERR)
    {
        // Free the pid
        _z_bytes_free(&pid);

        // Free the locator
        if (locator_is_scouted)
            free((char *)locator);

        // Free
        _zn_session_message_free(&om);
        _zn_session_free(zn);

        return zn;
    }

    _zn_session_message_t *p_am = r_msg.value.session_message;
    switch (_ZN_MID(p_am->header))
    {
    case _ZN_MID_ACCEPT:
    {
        // The announced sn resolution
        zn->sn_resolution = om.body.open.sn_resolution;
        zn->sn_resolution_half = zn->sn_resolution / 2;

        // The announced initial SN at TX side
        zn->sn_tx_reliable = om.body.open.initial_sn;
        zn->sn_tx_best_effort = om.body.open.initial_sn;

        // Handle SN resolution option if present
        if _ZN_HAS_FLAG (p_am->header, _ZN_FLAG_S_S)
        {
            // The resolution in the ACCEPT must be less or equal than the resolution in the OPEN,
            // otherwise the ACCEPT message is considered invalid and it should be treated as a
            // CLOSE message with L==0 by the Opener Peer -- the recipient of the ACCEPT message.
            if (p_am->body.accept.sn_resolution <= om.body.open.sn_resolution)
            {
                // In case of the SN Resolution proposed in this ACCEPT message is smaller than the SN Resolution
                // proposed in the OPEN message AND the Initial SN contained in the OPEN messages results to be
                // out-of-bound, the new Agreed Initial SN for the Opener Peer is calculated according to the
                // following modulo operation:
                //     Agreed Initial SN := (Initial SN_Open) mod (SN Resolution_Accept)
                if (om.body.open.initial_sn >= p_am->body.accept.sn_resolution)
                {
                    zn->sn_tx_reliable = om.body.open.initial_sn % p_am->body.accept.sn_resolution;
                    zn->sn_tx_best_effort = om.body.open.initial_sn % p_am->body.accept.sn_resolution;
                }
                zn->sn_resolution = p_am->body.accept.sn_resolution;
                zn->sn_resolution_half = zn->sn_resolution / 2;
            }
            else
            {
                // Close the session
                _zn_session_close(zn, _ZN_CLOSE_INVALID);
                break;
            }
        }

        if _ZN_HAS_FLAG (p_am->header, _ZN_FLAG_S_L)
        {
            // @TODO: we might want to connect or store the locators
        }

        // The session lease
        zn->lease = p_am->body.accept.lease;

        // The initial SN at RX side. Initialize the session as we had already received
        // a message with a SN equal to initial_sn - 1.
        if (p_am->body.accept.initial_sn > 0)
        {
            zn->sn_rx_reliable = p_am->body.accept.initial_sn - 1;
            zn->sn_rx_best_effort = p_am->body.accept.initial_sn - 1;
        }
        else
        {
            zn->sn_rx_reliable = zn->sn_resolution - 1;
            zn->sn_rx_best_effort = zn->sn_resolution - 1;
        }

        // Initialize the Local and Remote Peer IDs
        _z_bytes_move(&zn->local_pid, &pid);
        _z_bytes_copy(&zn->remote_pid, &r_msg.value.session_message->body.accept.apid);

        if (locator_is_scouted)
            zn->locator = (char *)locator;
        else
            zn->locator = strdup(locator);

        zn->on_disconnect = &_zn_default_on_disconnect;

        break;
    }

    default:
    {
        // Close the session
        _zn_session_close(zn, _ZN_CLOSE_INVALID);
        break;
    }
    }

    // Free the messages and result
    _zn_session_message_free(&om);
    _zn_session_message_free(p_am);
    _zn_session_message_p_result_free(&r_msg);

    return zn;
}

zn_properties_t *zn_info(zn_session_t *zn)
{
    zn_properties_t *ps = zn_properties_make();
    zn_properties_insert(ps, ZN_INFO_PID_KEY, _z_string_from_bytes(&zn->local_pid));
    zn_properties_insert(ps, ZN_INFO_ROUTER_PID_KEY, _z_string_from_bytes(&zn->remote_pid));
    return ps;
}

void zn_sample_free(zn_sample_t sample)
{
    if (sample.key.val)
        _z_string_free(&sample.key);
    if (sample.value.val)
        _z_bytes_free(&sample.value);
}

/*------------------ Resource Keys operations ------------------*/
zn_reskey_t zn_rname(const char *rname)
{
    zn_reskey_t rk;
    rk.rid = ZN_RESOURCE_ID_NONE;
    rk.rname = strdup(rname);
    return rk;
}

zn_reskey_t zn_rid(unsigned long rid)
{
    zn_reskey_t rk;
    rk.rid = rid;
    rk.rname = NULL;
    return rk;
}

zn_reskey_t zn_rid_with_suffix(unsigned long id, const char *suffix)
{
    zn_reskey_t rk;
    rk.rid = id;
    rk.rname = strdup(suffix);
    return rk;
}

/*------------------ Resource Declaration ------------------*/
z_zint_t zn_declare_resource(zn_session_t *zn, zn_reskey_t reskey)
{
    _zn_resource_t *r = (_zn_resource_t *)malloc(sizeof(_zn_resource_t));
    r->id = _zn_get_resource_id(zn);
    r->key = reskey;

    int res = _zn_register_resource(zn, _ZN_IS_LOCAL, r);
    if (res != 0)
    {
        free(r);
        return ZN_RESOURCE_ID_NONE;
    }

    // Build the declare message to send on the wire
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to declare the resource
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Resource declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_RESOURCE;
    z_msg.body.declare.declarations.val[0].body.res.id = r->id;
    z_msg.body.declare.declarations.val[0].body.res.key = _zn_reskey_clone(&r->key);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        _Z_DEBUG("Trying to reconnect...\n");
        zn->on_disconnect(zn);
        _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    }

    _zn_zenoh_message_free(&z_msg);

    return r->id;
}

void zn_undeclare_resource(zn_session_t *zn, z_zint_t rid)
{
    _zn_resource_t *r = _zn_get_resource_by_id(zn, _ZN_IS_LOCAL, rid);
    if (r)
    {
        _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

        // We need to undeclare the resource and the publisher
        unsigned int len = 1;
        z_msg.body.declare.declarations.len = len;
        z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

        // Resource declaration
        z_msg.body.declare.declarations.val[0].header = _ZN_DECL_FORGET_RESOURCE;
        z_msg.body.declare.declarations.val[0].body.forget_res.rid = rid;

        if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            _Z_DEBUG("Trying to reconnect...\n");
            zn->on_disconnect(zn);
            _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
        }

        _zn_zenoh_message_free(&z_msg);

        _zn_unregister_resource(zn, _ZN_IS_LOCAL, r);
    }
}

/*------------------  Publisher Declaration ------------------*/
zn_publisher_t *zn_declare_publisher(zn_session_t *zn, zn_reskey_t reskey)
{
    zn_publisher_t *pub = (zn_publisher_t *)malloc(sizeof(zn_publisher_t));
    pub->zn = zn;
    pub->key = reskey;
    pub->id = _zn_get_entity_id(zn);

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to declare the resource and the publisher
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Publisher declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_PUBLISHER;
    z_msg.body.declare.declarations.val[0].body.pub.key = _zn_reskey_clone(&reskey);
    ;
    // Mark the key as numerical if the key has no resource name
    if (!pub->key.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        _Z_DEBUG("Trying to reconnect...\n");
        zn->on_disconnect(zn);
        _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    }

    _zn_zenoh_message_free(&z_msg);

    return pub;
}

void zn_undeclare_publisher(zn_publisher_t *pub)
{
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to undeclare the publisher
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Forget publisher declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_FORGET_PUBLISHER;
    z_msg.body.declare.declarations.val[0].body.forget_pub.key = _zn_reskey_clone(&pub->key);
    // Mark the key as numerical if the key has no resource name
    if (!pub->key.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

    if (_zn_send_z_msg(pub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        _Z_DEBUG("Trying to reconnect...\n");
        pub->zn->on_disconnect(pub->zn);
        _zn_send_z_msg(pub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    }

    _zn_zenoh_message_free(&z_msg);

    free(pub);
}

/*------------------ Subscriber Declaration ------------------*/
zn_subinfo_t zn_subinfo_default()
{
    zn_subinfo_t si;
    si.reliability = zn_reliability_t_RELIABLE;
    si.mode = zn_submode_t_PUSH;
    si.period = NULL;
    return si;
}

zn_subscriber_t *zn_declare_subscriber(zn_session_t *zn, zn_reskey_t reskey, zn_subinfo_t sub_info, zn_data_handler_t callback, void *arg)
{
    _zn_subscriber_t *rs = (_zn_subscriber_t *)malloc(sizeof(_zn_subscriber_t));
    rs->id = _zn_get_entity_id(zn);
    rs->key = reskey;
    rs->info = sub_info;
    rs->callback = callback;
    rs->arg = arg;

    int res = _zn_register_subscription(zn, _ZN_IS_LOCAL, rs);
    if (res != 0)
    {
        free(rs);
        return NULL;
    }

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to declare the subscriber
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Subscriber declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_SUBSCRIBER;
    if (!reskey.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);
    if (sub_info.mode != zn_submode_t_PUSH || sub_info.period)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_S);
    if (sub_info.reliability == zn_reliability_t_RELIABLE)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_R);

    z_msg.body.declare.declarations.val[0].body.sub.key = _zn_reskey_clone(&reskey);

    // SubMode
    z_msg.body.declare.declarations.val[0].body.sub.subinfo.mode = sub_info.mode;
    z_msg.body.declare.declarations.val[0].body.sub.subinfo.reliability = sub_info.reliability;
    z_msg.body.declare.declarations.val[0].body.sub.subinfo.period = sub_info.period;

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        _Z_DEBUG("Trying to reconnect....\n");
        zn->on_disconnect(zn);
        _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    }

    _zn_zenoh_message_free(&z_msg);

    zn_subscriber_t *subscriber = (zn_subscriber_t *)malloc(sizeof(zn_subscriber_t));
    subscriber->zn = zn;
    subscriber->id = rs->id;

    return subscriber;
}

void zn_undeclare_subscriber(zn_subscriber_t *sub)
{
    _zn_subscriber_t *s = _zn_get_subscription_by_id(sub->zn, _ZN_IS_LOCAL, sub->id);
    if (s)
    {
        _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

        // We need to undeclare the subscriber
        unsigned int len = 1;
        z_msg.body.declare.declarations.len = len;
        z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

        // Forget Subscriber declaration
        z_msg.body.declare.declarations.val[0].header = _ZN_DECL_FORGET_SUBSCRIBER;
        if (!s->key.rname)
            _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

        z_msg.body.declare.declarations.val[0].body.forget_sub.key = _zn_reskey_clone(&s->key);

        if (_zn_send_z_msg(sub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            _Z_DEBUG("Trying to reconnect....\n");
            sub->zn->on_disconnect(sub->zn);
            _zn_send_z_msg(sub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
        }

        _zn_zenoh_message_free(&z_msg);

        _zn_unregister_subscription(sub->zn, _ZN_IS_LOCAL, s);
    }

    free(sub);
}

/*------------------ Write ------------------*/
int zn_write_ext(zn_session_t *zn, zn_reskey_t reskey, const unsigned char *payload, size_t length, uint8_t encoding, uint8_t kind, zn_congestion_control_t cong_ctrl)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DATA);
    // Eventually mark the message for congestion control
    if (cong_ctrl == zn_congestion_control_t_DROP)
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_D);
    // Set the resource key
    z_msg.body.data.key = reskey;
    _ZN_SET_FLAG(z_msg.header, reskey.rname ? 0 : _ZN_FLAG_Z_K);

    // Set the data info
    _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_I);
    _zn_data_info_t info;
    info.flags = 0;
    info.encoding = encoding;
    _ZN_SET_FLAG(info.flags, _ZN_DATA_INFO_ENC);
    info.kind = kind;
    _ZN_SET_FLAG(info.flags, _ZN_DATA_INFO_KIND);
    z_msg.body.data.info = info;

    // Set the payload
    z_msg.body.data.payload.len = length;
    z_msg.body.data.payload.val = (uint8_t *)payload;

    return _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, cong_ctrl);
}

int zn_write(zn_session_t *zn, zn_reskey_t reskey, const uint8_t *payload, size_t length)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DATA);
    // Eventually mark the message for congestion control
    if (ZN_CONGESTION_CONTROL_DEFAULT == zn_congestion_control_t_DROP)
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_D);
    // Set the resource key
    z_msg.body.data.key = reskey;
    _ZN_SET_FLAG(z_msg.header, reskey.rname ? 0 : _ZN_FLAG_Z_K);

    // Set the payload
    z_msg.body.data.payload.len = length;
    z_msg.body.data.payload.val = (uint8_t *)payload;

    return _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, ZN_CONGESTION_CONTROL_DEFAULT);
}

/*------------------ Query/Queryable ------------------*/
zn_query_consolidation_t zn_query_consolidation_default(void)
{
    zn_query_consolidation_t qc;
    qc.first_routers = zn_consolidation_mode_t_LAZY;
    qc.last_router = zn_consolidation_mode_t_LAZY;
    qc.reception = zn_consolidation_mode_t_FULL;
    return qc;
}

z_string_t zn_query_predicate(zn_query_t *query)
{
    z_string_t s;
    s.len = strlen(query->predicate);
    s.val = query->predicate;
    return s;
}

z_string_t zn_query_res_name(zn_query_t *query)
{
    z_string_t s;
    s.len = strlen(query->rname);
    s.val = query->rname;
    return s;
}

zn_target_t zn_target_default(void)
{
    zn_target_t t;
    t.tag = zn_target_t_BEST_MATCHING;
    return t;
}

zn_query_target_t zn_query_target_default(void)
{
    zn_query_target_t qt;
    qt.kind = ZN_QUERYABLE_ALL_KINDS;
    qt.target = zn_target_default();
    return qt;
}

void zn_query(zn_session_t *zn, zn_reskey_t reskey, const char *predicate, zn_query_target_t target, zn_query_consolidation_t consolidation, zn_query_handler_t callback, void *arg)
{
    // Create the pending query object
    _zn_pending_query_t *pq = (_zn_pending_query_t *)malloc(sizeof(_zn_pending_query_t));
    pq->id = _zn_get_query_id(zn);
    pq->key = reskey;
    pq->predicate = strdup(predicate);
    pq->target = target;
    pq->consolidation = consolidation;
    pq->callback = callback;
    pq->pending_replies = _z_list_empty;
    pq->arg = arg;

    // Add the pending query to the current session
    _zn_register_pending_query(zn, pq);

    // Send the query
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_QUERY);
    z_msg.body.query.qid = pq->id;
    z_msg.body.query.key = reskey;
    _ZN_SET_FLAG(z_msg.header, reskey.rname ? 0 : _ZN_FLAG_Z_K);
    z_msg.body.query.predicate = (z_str_t)predicate;
    z_msg.body.query.target = target;
    z_msg.body.query.consolidation = consolidation;

    int res = _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    if (res != 0)
        _zn_unregister_pending_query(zn, pq);
}

void reply_collect_handler(const zn_reply_t reply, const void *arg)
{
    _zn_pending_query_collect_t *pqc = (_zn_pending_query_collect_t *)arg;
    if (reply.tag == zn_reply_t_Tag_DATA)
    {
        zn_reply_data_t *rd = (zn_reply_data_t *)malloc(sizeof(zn_reply_data_t));
        rd->source_kind = reply.data.source_kind;
        _z_bytes_copy(&rd->replier_id, &reply.data.replier_id);
        _z_string_copy(&rd->data.key, &reply.data.data.key);
        _z_bytes_copy(&rd->data.value, &reply.data.data.value);

        _z_vec_append(&pqc->replies, rd);
    }
    else
    {
        // Signal that we have received all the replies
        _z_condvar_signal(&pqc->cond_var);
    }
}

zn_reply_data_array_t zn_query_collect(zn_session_t *zn,
                                       zn_reskey_t reskey,
                                       const char *predicate,
                                       zn_query_target_t target,
                                       zn_query_consolidation_t consolidation)
{
    // Create the synchronization variables
    _zn_pending_query_collect_t pqc;
    _z_mutex_init(&pqc.mutex);
    _z_condvar_init(&pqc.cond_var);
    pqc.replies = _z_vec_make(1);

    // Issue the query
    zn_query(zn, reskey, predicate, target, consolidation, reply_collect_handler, &pqc);
    // Wait to be notified
    _z_condvar_wait(&pqc.cond_var, &pqc.mutex);

    zn_reply_data_array_t rda;
    rda.len = _z_vec_len(&pqc.replies);
    zn_reply_data_t *replies = (zn_reply_data_t *)malloc(rda.len * sizeof(zn_reply_data_t));
    for (unsigned int i = 0; i < rda.len; i++)
    {
        zn_reply_data_t *reply = (zn_reply_data_t *)_z_vec_get(&pqc.replies, i);
        replies[i].source_kind = reply->source_kind;
        _z_bytes_move(&replies[i].replier_id, &reply->replier_id);
        _z_string_move(&replies[i].data.key, &reply->data.key);
        _z_bytes_move(&replies[i].data.value, &reply->data.value);
    }
    rda.val = replies;

    _z_vec_free(&pqc.replies);
    _z_condvar_free(&pqc.cond_var);
    _z_mutex_free(&pqc.mutex);

    return rda;
}

void zn_reply_data_array_free(zn_reply_data_array_t replies)
{
    for (unsigned int i = 0; i < replies.len; i++)
    {
        if (replies.val[i].replier_id.val)
            _z_bytes_free((z_bytes_t *)&replies.val[i].replier_id);
        if (replies.val[i].data.value.val)
            _z_bytes_free((z_bytes_t *)&replies.val[i].data.value);
        if (replies.val[i].data.key.val)
            _z_string_free((z_string_t *)&replies.val[i].data.key);
    }
    free((zn_reply_data_t *)replies.val);
}

zn_queryable_t *zn_declare_queryable(zn_session_t *zn, zn_reskey_t reskey, unsigned int kind, zn_queryable_handler_t callback, void *arg)
{
    _zn_queryable_t *rq = (_zn_queryable_t *)malloc(sizeof(_zn_queryable_t));
    rq->id = _zn_get_entity_id(zn);
    rq->key = reskey;
    rq->kind = kind;
    rq->callback = callback;
    rq->arg = arg;

    int res = _zn_register_queryable(zn, rq);
    if (res != 0)
    {
        free(rq);
        return NULL;
    }

    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

    // We need to declare the queryable
    unsigned int len = 1;
    z_msg.body.declare.declarations.len = len;
    z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

    // Queryable declaration
    z_msg.body.declare.declarations.val[0].header = _ZN_DECL_QUERYABLE;
    if (!reskey.rname)
        _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

    z_msg.body.declare.declarations.val[0].body.qle.key = _zn_reskey_clone(&reskey);

    if (_zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        _Z_DEBUG("Trying to reconnect....\n");
        zn->on_disconnect(zn);
        _zn_send_z_msg(zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    }

    _zn_zenoh_message_free(&z_msg);

    zn_queryable_t *queryable = (zn_queryable_t *)malloc(sizeof(zn_queryable_t));
    queryable->zn = zn;
    queryable->id = rq->id;

    return queryable;
}

void zn_undeclare_queryable(zn_queryable_t *qle)
{
    _zn_queryable_t *q = _zn_get_queryable_by_id(qle->zn, qle->id);
    if (q)
    {
        _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DECLARE);

        // We need to undeclare the subscriber
        unsigned int len = 1;
        z_msg.body.declare.declarations.len = len;
        z_msg.body.declare.declarations.val = (_zn_declaration_t *)malloc(len * sizeof(_zn_declaration_t));

        // Forget Subscriber declaration
        z_msg.body.declare.declarations.val[0].header = _ZN_DECL_FORGET_QUERYABLE;
        if (!q->key.rname)
            _ZN_SET_FLAG(z_msg.body.declare.declarations.val[0].header, _ZN_FLAG_Z_K);

        z_msg.body.declare.declarations.val[0].body.forget_sub.key = _zn_reskey_clone(&q->key);

        if (_zn_send_z_msg(qle->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            _Z_DEBUG("Trying to reconnect....\n");
            qle->zn->on_disconnect(qle->zn);
            _zn_send_z_msg(qle->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
        }

        _zn_zenoh_message_free(&z_msg);

        _zn_unregister_queryable(qle->zn, q);
    }

    free(qle);
}

void zn_send_reply(zn_query_t *query, const char *key, const uint8_t *payload, size_t len)
{
    _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_DATA);

    // Build the reply context decorator. This is NOT the final reply.
    z_msg.reply_context = _zn_reply_context_init();
    z_msg.reply_context->qid = query->qid;
    z_msg.reply_context->source_kind = query->kind;
    z_msg.reply_context->replier_id = query->zn->local_pid;

    // Build the data payload
    z_msg.body.data.payload.val = payload;
    z_msg.body.data.payload.len = len;
    // @TODO: use numerical resources if possible
    z_msg.body.data.key.rid = ZN_RESOURCE_ID_NONE;
    z_msg.body.data.key.rname = (z_str_t)key;
    if (!z_msg.body.data.key.rname)
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_K);
    // Do not set any data_info

    if (_zn_send_z_msg(query->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
    {
        _Z_DEBUG("Trying to reconnect....\n");
        query->zn->on_disconnect(query->zn);
        _zn_send_z_msg(query->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
    }

    free(z_msg.reply_context);
}

/*------------------ Pull ------------------*/
int zn_pull(zn_subscriber_t *sub)
{
    _zn_subscriber_t *s = _zn_get_subscription_by_id(sub->zn, _ZN_IS_LOCAL, sub->id);
    if (s)
    {
        _zn_zenoh_message_t z_msg = _zn_zenoh_message_init(_ZN_MID_PULL);
        z_msg.body.pull.key = _zn_reskey_clone(&s->key);
        _ZN_SET_FLAG(z_msg.header, s->key.rname ? 0 : _ZN_FLAG_Z_K);
        _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_F);

        z_msg.body.pull.pull_id = _zn_get_pull_id(sub->zn);
        // @TODO: get the correct value for max_sample
        z_msg.body.pull.max_samples = 0;
        // _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_N);

        if (_zn_send_z_msg(sub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK) != 0)
        {
            _Z_DEBUG("Trying to reconnect....\n");
            sub->zn->on_disconnect(sub->zn);
            _zn_send_z_msg(sub->zn, &z_msg, zn_reliability_t_RELIABLE, zn_congestion_control_t_BLOCK);
        }

        _zn_zenoh_message_free(&z_msg);

        return 0;
    }
    else
    {
        return -1;
    }
}

/*-----------------------------------------------------------*/
/*------------------ Zenoh-pico operations ------------------*/
/*-----------------------------------------------------------*/
/*------------------ Read ------------------*/
int znp_read(zn_session_t *zn)
{
    _zn_session_message_p_result_t r_s = _zn_recv_s_msg(zn);
    if (r_s.tag == _z_res_t_OK)
    {
        int res = _zn_handle_session_message(zn, r_s.value.session_message);
        _zn_session_message_free(r_s.value.session_message);
        _zn_session_message_p_result_free(&r_s);
        return res;
    }
    else
    {
        _zn_session_message_p_result_free(&r_s);
        return _z_res_t_ERR;
    }
}

/*------------------ Keep Alive ------------------*/
int znp_send_keep_alive(zn_session_t *zn)
{
    _zn_session_message_t s_msg = _zn_session_message_init(_ZN_MID_KEEP_ALIVE);

    return _zn_send_s_msg(zn, &s_msg);
}
