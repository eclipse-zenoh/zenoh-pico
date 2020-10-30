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
#include <unistd.h>
#include "zenoh/net/lease_loop.h"
#include "zenoh/net/property.h"
#include "zenoh/net/recv_loop.h"
#include "zenoh/net/session.h"
#include "zenoh/net/private/internal.h"
#include "zenoh/net/private/msgcodec.h"
#include "zenoh/net/private/net.h"
#include "zenoh/net/private/sync.h"
#include "zenoh/private/internal.h"
#include "zenoh/private/logging.h"

/*=============================*/
/*          Private            */
/*=============================*/
/*------------------ Init/Free/Close session ------------------*/
zn_session_t *_zn_session_init()
{
    zn_session_t *z = (zn_session_t *)malloc(sizeof(zn_session_t));

    // Initialize the buffers
    z->wbuf = _z_wbuf_make(ZENOH_NET_WRITE_BUF_LEN, 0);
    z->rbuf = _z_rbuf_make(ZENOH_NET_READ_BUF_LEN);
    z->dbuf = _z_wbuf_make(0, 1);

    // Initialize the mutexes
    _zn_mutex_init(&z->mutex_rx);
    _zn_mutex_init(&z->mutex_tx);

    // The initial SN at RX side
    z->lease = 0;
    z->sn_resolution = 0;

    // The initial SN at RX side
    z->sn_rx_reliable = 0;
    z->sn_rx_best_effort = 0;

    // The initial SN at TX side
    z->sn_tx_reliable = 0;
    z->sn_tx_best_effort = 0;

    // Initialize the counters to 1
    z->entity_id = 1;
    z->resource_id = 1;
    z->query_id = 1;

    // Initialize the data structs
    z->local_resources = _z_list_empty;
    z->remote_resources = _z_list_empty;

    z->local_subscriptions = _z_list_empty;
    z->remote_subscriptions = _z_list_empty;
    z->rem_res_loc_sub_map = _z_i_map_make(DEFAULT_I_MAP_CAPACITY);

    z->local_publishers = _z_list_empty;
    z->local_queryables = _z_list_empty;

    z->replywaiters = _z_list_empty;

    z->recv_loop_running = 0;
    z->recv_loop_thread = NULL;

    z->received = 0;
    z->transmitted = 0;
    z->lease_loop_running = 0;
    z->lease_loop_thread = NULL;

    return z;
}

void _zn_session_free(zn_session_t *z)
{
    printf("SESSION FREE!!!\n");
    close(z->sock);

    _zn_mutex_free(&z->mutex_tx);
    _zn_mutex_free(&z->mutex_rx);

    _z_wbuf_free(&z->wbuf);
    _z_rbuf_free(&z->rbuf);

    free(z);
}

int _zn_send_close(zn_session_t *z, unsigned int reason, int link_only)
{
    _zn_session_message_t cm = _zn_session_message_init();
    cm.header = _ZN_MID_CLOSE;
    cm.body.close.pid = z->local_pid;
    cm.body.close.reason = reason;
    _ZN_SET_FLAG(cm.header, _ZN_FLAG_S_I);
    if (link_only)
        _ZN_SET_FLAG(cm.header, _ZN_FLAG_S_K);

    int res = _zn_send_s_msg(z, &cm);

    // Free the message
    _zn_session_message_free(&cm);

    return res;
}

int _zn_session_close(zn_session_t *z, unsigned int reason)
{
    int res = _zn_send_close(z, reason, 0);
    // Free the session
    _zn_session_free(z);

    return res;
}

void _zn_default_on_disconnect(void *vz)
{
    zn_session_t *z = (zn_session_t *)vz;
    for (int i = 0; i < 3; ++i)
    {
        _zn_sleep_s(3);
        // Try to reconnect -- eventually we should scout here.
        // We should also re-do declarations.
        _Z_DEBUG("Tring to reconnect...\n");
        _zn_socket_result_t r_sock = _zn_open_tx_session(strdup(z->locator));
        if (r_sock.tag == _z_res_t_OK)
        {
            z->sock = r_sock.value.socket;
            return;
        }
    }
}

/*------------------ Scout ------------------*/
zn_hello_array_t _zn_scout_loop(_zn_socket_t socket, const _z_wbuf_t *wbf, const struct sockaddr *dest, socklen_t salen, size_t tries)
{
    // @TODO: need to abstract the platform-specific data types
    struct sockaddr *from = (struct sockaddr *)malloc(2 * sizeof(struct sockaddr_in *));
    socklen_t flen = 0;
    _z_rbuf_t rbf = _z_rbuf_make(ZENOH_NET_MAX_SCOUT_MSG_LEN);
    zn_hello_array_t ls;
    ls.len = 0;
    ls.val = NULL;

    while (tries > 0)
    {
        tries--;

        // Send the scout message
        _zn_send_dgram_to(socket, wbf, dest, salen);
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
            if (!ls.val)
            {
                ls.val = (zn_hello_t *)malloc(sizeof(zn_hello_t));
                ls.len = 1;
                memset((void *)&ls.val[0], 0, sizeof(zn_hello_t));
            }

            if _ZN_HAS_FLAG (s_msg->header, _ZN_FLAG_S_L)
            {
                zn_hello_t *sc = (zn_hello_t *)&ls.val[ls.len - 1];
                sc->whatami = s_msg->body.hello.whatami;
                _z_bytes_move(&sc->pid, &s_msg->body.hello.pid);
                _z_str_array_move(&sc->locators, &s_msg->body.hello.locators);
            }
            else
            {
                // @TODO: construct the locator departing from the sock address
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

        if (ls.val)
            break;
        else
            continue;
    }

    free(from);
    _z_rbuf_free(&rbf);

    return ls;
}

/*------------------ Handle message ------------------*/
int _zn_handle_zenoh_message(zn_session_t *z, _zn_zenoh_message_t *msg)
{
    switch (_ZN_MID(msg->header))
    {
    case _ZN_MID_DATA:
    {
        _Z_DEBUG_VA("Received _ZN_MID_DATA message %d\n", _Z_MID(msg->header));

        _z_list_t *subs = _zn_get_subscriptions_from_remote_key(z, &msg->body.data.key);
        _zn_subscriber_t *sub;
        while (subs)
        {
            sub = (_zn_subscriber_t *)_z_list_head(subs);

            // Reconstruct the whole key name
            // @TODO: store this in the subscription and do not
            //        compute it every time
            char *ks = _zn_get_resource_name_from_key(z, 1, &sub->key);
            z_string_t key;
            key.val = ks;
            key.len = strlen(ks);

            // Build the sample
            zn_sample_t sample;
            sample.key = key;
            sample.value = msg->body.data.payload;

            // Call the callback at API level
            sub->data_handler(&sample, sub->arg);

            // Drop the head element and continue with the tail
            subs = _z_list_drop_val(subs, 0);
        }

        return _z_res_t_OK;
    }

    case _ZN_MID_DECLARE:
    {
        _Z_DEBUG("Received _ZN_DECLARE message\n");
        for (size_t i = 0; i < msg->body.declare.declarations.len; ++i)
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
                _zn_register_resource(z, _ZN_IS_REMOTE, id, &key);

                // Check if there is a matching local subscription
                _zn_subscriber_t *sub = _zn_get_subscription_by_key(z, _ZN_IS_LOCAL, &key);
                if (sub)
                {
                    // Add the mapping between remote id and local subscription
                    _z_i_map_set(z->rem_res_loc_sub_map, id, sub);
                }

                break;
            }

            case _ZN_DECL_PUBLISHER:
            {
                // Check if there are matching local subscriptions
                _z_list_t *subs = _zn_get_subscriptions_from_remote_key(z, &msg->body.data.key);
                size_t len = _z_list_len(subs);
                if (len > 0)
                {
                    // Need to reply with a decl subscriber
                    _zn_zenoh_message_t ds = _zn_zenoh_message_init();
                    ds.header = _ZN_MID_DECLARE;
                    _ZN_ARRAY_S_DEFINE(declaration, declarations, len);

                    _zn_subscriber_t *sub;
                    while (subs)
                    {
                        len--;
                        sub = (_zn_subscriber_t *)_z_list_head(subs);

                        declarations.val[len].header = _ZN_DECL_SUBSCRIBER;
                        declarations.val[len].body.sub.key = sub->key;
                        declarations.val[len].body.sub.subinfo = sub->info;

                        // Drop the head element and continue with the tail
                        subs = _z_list_drop_val(subs, 0);
                    }

                    // Send the message
                    _zn_send_z_msg(z, &ds, 1);

                    // Free the declarations
                    _ARRAY_S_FREE(declarations);
                }
                break;
            }

            case _ZN_DECL_SUBSCRIBER:
            {
                _zn_subscriber_t *sub = _zn_get_subscription_by_key(z, _ZN_IS_REMOTE, &decl.body.sub.key);
                if (sub)
                {
                    _zn_subscriber_t rs;
                    rs.id = _zn_get_entity_id(z);
                    rs.key = decl.body.sub.key;
                    rs.info = decl.body.sub.subinfo;
                    rs.data_handler = NULL;
                    rs.arg = NULL;
                    _zn_register_subscription(z, _ZN_IS_REMOTE, &rs);
                }

                break;
            }
            case _ZN_DECL_QUERYABLE:
            {
                // @TODO
                break;
            }
            case _ZN_DECL_FORGET_RESOURCE:
            {
                _zn_res_decl_t *rd = _zn_get_resource_by_id(z, _ZN_IS_REMOTE, decl.body.forget_res.rid);
                if (rd)
                    _zn_unregister_resource(z, _ZN_IS_REMOTE, rd);

                break;
            }
            case _ZN_DECL_FORGET_PUBLISHER:
            {
                // @TODO
                break;
            }
            case _ZN_DECL_FORGET_SUBSCRIBER:
            {
                _zn_subscriber_t *sub = _zn_get_subscription_by_key(z, _ZN_IS_REMOTE, &decl.body.forget_sub.key);
                if (sub)
                    _zn_unregister_subscription(z, _ZN_IS_REMOTE, sub);

                break;
            }
            case _ZN_DECL_FORGET_QUERYABLE:
            {
                // @TODO
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
        _Z_DEBUG("Handling of Pull messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_QUERY:
    {
        _Z_DEBUG("Handling of Query messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_UNIT:
    {
        _Z_DEBUG("Handling of Unit messages not implemented");
        return _z_res_t_OK;
    }

    default:
    {
        _Z_DEBUG("Unknown zenoh message ID");
        return _z_res_t_ERR;
    }
    }
}

int _zn_handle_session_message(zn_session_t *z, _zn_session_message_t *msg)
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
        _Z_DEBUG("Handling of Hello messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_OPEN:
    {
        _Z_DEBUG("Handling of Open messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_ACCEPT:
    {
        _Z_DEBUG("Handling of Accept messages not implemented");
        return _z_res_t_OK;
    }

    case _ZN_MID_CLOSE:
    {
        _Z_DEBUG("Closing session as requested by the remote peer");
        _zn_session_free(z);
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
            if (_zn_sn_precedes(z->sn_resolution_half, z->sn_rx_reliable, msg->body.frame.sn))
            {
                z->sn_rx_reliable = msg->body.frame.sn;
            }
            else
            {
                _Z_DEBUG("Reliable message dropped because it is out of order");
                return _z_res_t_OK;
            }
        }
        else
        {
            if (_zn_sn_precedes(z->sn_resolution_half, z->sn_rx_best_effort, msg->body.frame.sn))
            {
                z->sn_rx_best_effort = msg->body.frame.sn;
            }
            else
            {
                _Z_DEBUG("Best effort message dropped because it is out of order");
                return _z_res_t_OK;
            }
        }

        if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_S_F))
        {
            int res = _z_res_t_OK;
            // Add the fragment to the defragmentation buffer
            _z_wbuf_add_iosli_from(&z->dbuf, msg->body.frame.payload.fragment.val, msg->body.frame.payload.fragment.len);

            // Check if this is the last fragment
            if (_ZN_HAS_FLAG(msg->header, _ZN_FLAG_S_E))
            {
                // Convert the defragmentation buffer into a decoding buffer
                _z_rbuf_t rbf = _z_wbuf_to_rbuf(&z->dbuf);

                // Decode the zenoh message
                _zn_zenoh_message_p_result_t r_zm = _zn_zenoh_message_decode(&rbf);
                if (r_zm.tag == _z_res_t_OK)
                {
                    _zn_zenoh_message_t *d_zm = r_zm.value.zenoh_message;
                    res = _zn_handle_zenoh_message(z, d_zm);
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
                _z_wbuf_reset(&z->dbuf);
            }

            return res;
        }
        else
        {
            // Handle all the zenoh message, one by one
            size_t len = _z_vec_len(&msg->body.frame.payload.messages);
            for (size_t i = 0; i < len; ++i)
            {
                int res = _zn_handle_zenoh_message(z, (_zn_zenoh_message_t *)_z_vec_get(&msg->body.frame.payload.messages, i));
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
// zn_hello_array_t zn_scout(char *iface, unsigned int tries, unsigned int period)
zn_hello_array_t zn_scout(unsigned int what, zn_properties_t *config, unsigned long scout_period)
{
    zn_hello_array_t locs;
    locs.len = 0;
    locs.val = NULL;

    char *addr = _zn_select_scout_iface();
    _zn_socket_result_t r = _zn_create_udp_socket(addr, 0, scout_period);
    if (r.tag == _z_res_t_ERR)
    {
        _Z_DEBUG("Unable to create scouting socket\n");
        free(addr);
        return locs;
    }

    _z_wbuf_t wbf = _z_wbuf_make(ZENOH_NET_MAX_SCOUT_MSG_LEN, 0);
    _zn_session_message_t scout = _zn_session_message_init();
    scout.header = _ZN_MID_SCOUT;
    // @TODO: check the
    scout.body.scout.what = what;
    // NOTE: when W flag is set to 0 in the header, it means implicitly scouting for Routers
    //       and the what value is not sent on the wire. Here we scout for Routers.
    _zn_session_message_encode(&wbf, &scout);

    socklen_t salen = sizeof(struct sockaddr_in);
    // Scout first on localhost
    struct sockaddr_in *laddr = _zn_make_socket_address(addr, ZENOH_NET_SCOUT_PORT);

    // @TODO: take the tries from config properties
    (void)(config);
    unsigned int tries = 30;

    locs = _zn_scout_loop(r.value.socket, &wbf, (struct sockaddr *)laddr, salen, tries);
    free(laddr);

    if (locs.len == 0)
    {
        // We did not find a router on localhost, hence scout on the LAN
        struct sockaddr_in *maddr = _zn_make_socket_address(ZENOH_NET_SCOUT_MCAST_ADDR, ZENOH_NET_SCOUT_PORT);
        locs = _zn_scout_loop(r.value.socket, &wbf, (struct sockaddr *)maddr, salen, tries);
        free(maddr);
    }

    free(addr);
    _z_wbuf_free(&wbf);

    return locs;
}

void zn_close(zn_session_t *z)
{
    _zn_session_close(z, _ZN_CLOSE_GENERIC);
    return;
}

zn_session_t *zn_open(zn_properties_t *config)
{
    _zn_session_p_result_t r;
    int locator_is_scouted = 0;
    if (!locator)
    {
        zn_hello_array_t locs = zn_scout("auto", ZENOH_NET_SCOUT_TRIES, ZENOH_NET_SCOUT_TIMEOUT);
        if (locs.len > 0)
        {
            locator = strdup((const char *)_z_vec_get(&locs, 0));
            // Mark that the locator has been scouted, need to be freed before returning
            locator_is_scouted = 1;
            // Free all the scouted locators
            _z_vec_free(&locs);
        }
        else
        {
            _Z_DEBUG("Unable to scout a zenoh router\n");
            _Z_ERROR("%sPlease make sure one is running on your network!\n", "");
            r.tag = _z_res_t_ERR;
            r.value.error = _zn_err_t_TX_CONNECTION;
            // Free all the scouted locators
            _z_vec_free(&locs);

            return r;
        }
    }

    // Initialize the session to null
    r.value.session = NULL;
    srand(clock());

    // Attempt to open a socket
    _zn_socket_result_t r_sock = _zn_open_tx_session(locator);
    if (r_sock.tag == _z_res_t_ERR)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = _zn_err_t_IO_GENERIC;

        if (locator_is_scouted)
            free(locator);

        return r;
    }

    r.tag = _z_res_t_OK;

    // Randomly generate a peer ID
    _ARRAY_S_DEFINE(uint8_t, bytes, z_, pid, ZENOH_NET_PID_LENGTH);
    for (int i = 0; i < ZENOH_NET_PID_LENGTH; ++i)
        pid.elem[i] = rand() % 255;

    // Build the open message
    _zn_session_message_t om = _zn_session_message_init();
    om.header = _ZN_MID_OPEN;
    // Add an attachement if properties have been provided
    _z_wbuf_t att_pld;
    if (ps)
    {
        att_pld = _z_wbuf_make(ZENOH_NET_ATTACHMENT_BUF_LEN, 0);
        zn_properties_encode(&att_pld, ps);

        om.attachment = (_zn_attachment_t *)malloc(sizeof(_zn_attachment_t));
        om.attachment->header = _ZN_MID_OPEN | _ZN_ATT_ENC_PROPERTIES;
        om.attachment->payload = _z_iosli_to_bytes(_z_wbuf_get_iosli(&att_pld, 0));
    }

    om.body.open.version = ZENOH_NET_PROTO_VERSION;
    om.body.open.whatami = ZN_WHATAMI_CLIENT;
    om.body.open.opid = pid;
    om.body.open.lease = ZENOH_NET_SESSION_LEASE;
    om.body.open.initial_sn = (z_zint_t)rand() % ZENOH_NET_SN_RESOLUTION;
    om.body.open.sn_resolution = ZENOH_NET_SN_RESOLUTION;

    if (ZENOH_NET_SN_RESOLUTION != ZENOH_NET_SN_RESOLUTION_DEFAULT)
        _ZN_SET_FLAG(om.header, _ZN_FLAG_S_S);
    // NOTE: optionally the open can include a list of locators the opener
    //       is reachable at. Since zenoh-pico acts as a client, this is not
    //       needed because a client is not expected to receive opens.

    // Initialize the session
    r.value.session = _zn_session_init();
    r.value.session->sock = r_sock.value.socket;

    _Z_DEBUG("Sending Open\n");
    // Encode and send the message
    _zn_send_s_msg(r.value.session, &om);

    _zn_session_message_p_result_t r_msg = _zn_recv_s_msg(r.value.session);
    if (r_msg.tag == _z_res_t_ERR)
    {
        r.tag = _z_res_t_ERR;
        r.value.error = ZN_FAILED_TO_OPEN_SESSION;

        // Free the pid
        ARRAY_S_FREE(pid);

        // Free the attachment payload;
        if (ps)
            _z_wbuf_free(&att_pld);

        // Free the locator
        if (locator_is_scouted)
            free(locator);

        // Free
        _zn_session_message_free(&om);
        _zn_session_free(r.value.session);

        return r;
    }

    _zn_session_message_t *p_am = r_msg.value.session_message;
    switch (_ZN_MID(p_am->header))
    {
    case _ZN_MID_ACCEPT:
    {
        r.tag = _z_res_t_OK;

        // The announced sn resolution
        r.value.session->sn_resolution = om.body.open.sn_resolution;
        r.value.session->sn_resolution_half = r.value.session->sn_resolution / 2;

        // The announced initial SN at TX side
        r.value.session->sn_tx_reliable = om.body.open.initial_sn;
        r.value.session->sn_tx_best_effort = om.body.open.initial_sn;

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
                    r.value.session->sn_tx_reliable = om.body.open.initial_sn % p_am->body.accept.sn_resolution;
                    r.value.session->sn_tx_best_effort = om.body.open.initial_sn % p_am->body.accept.sn_resolution;
                }
                r.value.session->sn_resolution = p_am->body.accept.sn_resolution;
                r.value.session->sn_resolution_half = r.value.session->sn_resolution / 2;
            }
            else
            {
                // Close the session
                _zn_session_close(r.value.session, _ZN_CLOSE_INVALID);

                r.tag = _z_res_t_ERR;
                r.value.error = ZN_FAILED_TO_OPEN_SESSION;

                break;
            }
        }

        if _ZN_HAS_FLAG (p_am->header, _ZN_FLAG_S_L)
        {
            // @TODO: we might want to connect or store the locators
        }

        // The session lease
        r.value.session->lease = p_am->body.accept.lease;

        // The initial SN at RX side. Initialize the session as we had already received
        // a message with a SN equal to initial_sn - 1.
        if (p_am->body.accept.initial_sn > 0)
        {
            r.value.session->sn_rx_reliable = p_am->body.accept.initial_sn - 1;
            r.value.session->sn_rx_best_effort = p_am->body.accept.initial_sn - 1;
        }
        else
        {
            r.value.session->sn_rx_reliable = r.value.session->sn_resolution - 1;
            r.value.session->sn_rx_best_effort = r.value.session->sn_resolution - 1;
        }

        // Initialize the Local and Remote Peer IDs
        ARRAY_S_COPY(uint8_t, r.value.session->local_pid, pid);
        ARRAY_S_COPY(uint8_t, r.value.session->remote_pid, r_msg.value.session_message->body.accept.apid);

        r.value.session->locator = strdup(locator);

        r.value.session->on_disconnect = on_disconnect ? on_disconnect : &_zn_default_on_disconnect;

        break;
    }

    default:
    {
        // Close the session
        _zn_session_close(r.value.session, _ZN_CLOSE_INVALID);

        r.tag = _z_res_t_ERR;
        r.value.error = ZN_FAILED_TO_OPEN_SESSION;

        break;
    }
    }

    // Free the pid
    ARRAY_S_FREE(pid);

    // Free the attachment payload;
    if (ps)
        _z_wbuf_free(&att_pld);

    // Free the locator
    if (locator_is_scouted)
        free(locator);

    // Free the messages and result
    _zn_session_message_free(&om);
    _zn_session_message_free(p_am);
    _zn_session_message_p_result_free(&r_msg);

    return r;
}

// _z_vec_t zn_info(zn_session_t *z)
// {
//     _z_vec_t res = _z_vec_make(3);
//     zn_property_t *pid = zn_property_make(ZN_INFO_PID_KEY, z->pid);
//     z_bytes_t locator;
//     locator.length = strlen(z->locator) + 1;
//     locator.elem = (uint8_t *)z->locator;
//     zn_property_t *peer = zn_property_make(ZN_INFO_PEER_KEY, locator);
//     zn_property_t *peer_pid = zn_property_make(ZN_INFO_PEER_PID_KEY, z->peer_pid);

//     _z_vec_set(&res, ZN_INFO_PID_KEY, pid);
//     _z_vec_set(&res, ZN_INFO_PEER_KEY, peer);
//     _z_vec_set(&res, ZN_INFO_PEER_PID_KEY, peer_pid);
//     res._length = 3;

//     return res;
// }

/*------------------ Resource Keys operations ------------------*/
zn_reskey_t zn_rname(const char *rname)
{
    zn_reskey_t rk;
    rk.rid = ZN_NO_RESOURCE_ID;
    rk.rname = strdup(rname);

    return rk;
}

zn_reskey_t zn_rid(const zn_resource_t *rd)
{
    zn_reskey_t rk;
    rk.rid = rd->id;
    rk.rname = NULL;

    return rk;
}

/*------------------ Resource Declaration ------------------*/
zn_res_p_result_t zn_declare_resource(zn_session_t *z, const zn_reskey_t *reskey)
{
    // Create the result
    zn_res_p_result_t r;
    r.tag = _z_res_t_OK;
    r.value.res = (zn_resource_t *)malloc(sizeof(zn_resource_t));
    r.value.res->z = z;
    r.value.res->id = _zn_get_resource_id(z, reskey);
    r.value.res->key.rid = reskey->rid;
    r.value.res->key.rname = reskey->rname;

    _zn_zenoh_message_t z_msg;
    _ZN_INIT_Z_MSG(z_msg);
    z_msg.header = _ZN_MID_DECLARE;

    // We need to declare the resource
    int decl_num = 1;
    _ZN_ARRAY_S_DEFINE(declaration, decl, decl_num)

    // Resource declaration
    decl.elem[0].header = _ZN_DECL_RESOURCE;
    decl.elem[0].body.res.id = r.value.res->id;
    decl.elem[0].body.res.key = r.value.res->key;

    z_msg.body.declare.declarations = decl;
    if (_zn_send_z_msg(z, &z_msg, 1) != 0)
    {
        _Z_DEBUG("Trying to reconnect...\n");
        z->on_disconnect(z);
        _zn_send_z_msg(z, &z_msg, 1);
    }
    ARRAY_S_FREE(decl);
    _zn_register_resource(z, 1, r.value.res->id, &r.value.res->key);

    return r;
}

int zn_undeclare_resource(zn_resource_t *res)
{
    _zn_res_decl_t *r = _zn_get_resource_by_id(res->z, _ZN_IS_LOCAL, res->id);
    if (r)
    {
        _zn_zenoh_message_t z_msg;
        _ZN_INIT_Z_MSG(z_msg)
        z_msg.header = _ZN_MID_DECLARE;

        // We need to undeclare the resource and the publisher
        int decl_num = 1;
        _ZN_ARRAY_S_DEFINE(declaration, decl, decl_num)

        // Resource declaration
        decl.elem[0].header = _ZN_DECL_FORGET_RESOURCE;
        decl.elem[0].body.forget_res.rid = res->id;

        z_msg.body.declare.declarations = decl;
        if (_zn_send_z_msg(res->z, &z_msg, 1) != 0)
        {
            _Z_DEBUG("Trying to reconnect...\n");
            res->z->on_disconnect(res->z);
            _zn_send_z_msg(res->z, &z_msg, 1);
        }
        ARRAY_S_FREE(decl);
        _zn_unregister_resource(res->z, 1, r);
    }

    return 0;
}

/*------------------  Publisher Declaration ------------------*/
zn_pub_p_result_t zn_declare_publisher(zn_session_t *z, const zn_reskey_t *reskey)
{
    zn_pub_p_result_t r;
    r.tag = _z_res_t_OK;
    r.value.pub = (zn_publisher_t *)malloc(sizeof(zn_publisher_t));
    r.value.pub->z = z;
    r.value.pub->key.rid = reskey->rid;
    if (reskey->rname)
        r.value.pub->key.rname = strdup(reskey->rname);
    else
        r.value.pub->key.rname = NULL;
    r.value.pub->id = _zn_get_entity_id(z);

    _zn_zenoh_message_t z_msg;
    _ZN_INIT_Z_MSG(z_msg)
    z_msg.header = _ZN_MID_DECLARE;

    // We need to declare the resource and the publisher
    int decl_num = 1;
    _ZN_ARRAY_S_DEFINE(declaration, decl, decl_num)

    // Publisher declaration
    decl.elem[0].header = _ZN_DECL_PUBLISHER;
    decl.elem[0].body.pub.key.rid = r.value.pub->key.rid;
    decl.elem[0].body.pub.key.rname = r.value.pub->key.rname;
    // Mark the key as numerical if the key has no resource name
    if (!decl.elem[0].body.pub.key.rname)
        _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_K);

    z_msg.body.declare.declarations = decl;
    if (_zn_send_z_msg(z, &z_msg, 1) != 0)
    {
        _Z_DEBUG("Trying to reconnect...\n");
        z->on_disconnect(z);
        _zn_send_z_msg(z, &z_msg, 1);
    }
    ARRAY_S_FREE(decl);

    return r;
}

int zn_undeclare_publisher(zn_publisher_t *pub)
{
    // @TODO: manage multi publishers

    _zn_zenoh_message_t z_msg;
    _ZN_INIT_Z_MSG(z_msg)
    z_msg.header = _ZN_MID_DECLARE;

    // We need to undeclare the publisher
    int dnum = 1;
    _ZN_ARRAY_S_DEFINE(declaration, decl, dnum)

    // Forget publisher declaration
    decl.elem[0].header = _ZN_DECL_FORGET_PUBLISHER;
    decl.elem[0].body.forget_pub.key.rid = pub->key.rid;
    decl.elem[0].body.forget_pub.key.rname = pub->key.rname;
    // Mark the key as numerical if the key has no resource name
    if (!decl.elem[0].body.forget_pub.key.rname)
        _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_K);

    z_msg.body.declare.declarations = decl;
    if (_zn_send_z_msg(pub->z, &z_msg, 1) != 0)
    {
        _Z_DEBUG("Trying to reconnect...\n");
        pub->z->on_disconnect(pub->z);
        _zn_send_z_msg(pub->z, &z_msg, 1);
    }
    ARRAY_S_FREE(decl);

    return 0;
}

/*------------------ Subscriber Declaration ------------------*/
zn_subscriber_t *zn_declare_subscriber(zn_session_t *z, zn_reskey_t *reskey, zn_subinfo_t sub_info, zn_data_handler_t zn_data_handler_t, void *arg)
{
    zn_subscriber_t *subscriber = (zn_subscriber_t *)malloc(sizeof(zn_subscriber_t));
    subscriber->z = z;
    subscriber->id = _zn_get_entity_id(z);
    subscriber->key.rid = reskey->rid;
    if (reskey->rname)
        subscriber->key.rname = strdup(reskey->rname);
    else
        subscriber->key.rname = NULL;
    memcpy(&subscriber->info, &sub_info, sizeof(zn_subinfo_t));

    _zn_zenoh_message_t z_msg;
    _ZN_INIT_Z_MSG(z_msg)
    z_msg.header = _ZN_MID_DECLARE;

    // We need to declare the subscriber
    int dnum = 1;
    _ZN_ARRAY_S_DEFINE(declaration, decl, dnum)

    // Subscriber declaration
    decl.elem[0].header = _ZN_DECL_SUBSCRIBER;
    if (!reskey->rname)
        _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_K);
    if (sub_info.mode != ZN_PUSH_MODE || sub_info.is_periodic)
        _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_S);
    if (sub_info.is_reliable)
        _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_R);

    decl.elem[0].body.sub.key = subscriber->key;

    // SubMode
    decl.elem[0].body.sub.subinfo.mode = sub_info.mode;
    decl.elem[0].body.sub.subinfo.is_reliable = sub_info->is_reliable;
    decl.elem[0].body.sub.subinfo.is_periodic = sub_info->is_periodic;
    decl.elem[0].body.sub.subinfo.period = sub_info.period;

    z_msg.body.declare.declarations = decl;
    if (_zn_send_z_msg(z, &z_msg, 1) != 0)
    {
        _Z_DEBUG("Trying to reconnect....\n");
        z->on_disconnect(z);
        _zn_send_z_msg(z, &z_msg, 1);
    }
    _ARRAY_S_FREE(decl);

    z_string_t s;
    s.val = _zn_get_resource_name_from_key();

    _zn_subscriber_t rs;
    rs.id = r.value.sub->id;
    rs.key = r.value.sub->key;
    rs.info = r.value.sub->info;
    rs.data_handler = data_handler;
    rs.arg = arg;
    _zn_register_subscription(z, 1, &rs);

    return r;
}

int zn_undeclare_subscriber(zn_subscriber_t *sub)
{
    _zn_subscriber_t *s = _zn_get_subscription_by_id(sub->z, _ZN_IS_LOCAL, sub->id);
    if (s)
    {
        _zn_zenoh_message_t z_msg;
        _ZN_INIT_Z_MSG(z_msg)
        z_msg.header = _ZN_MID_DECLARE;

        // We need to undeclare the subscriber
        int dnum = 1;
        _ZN_ARRAY_S_DEFINE(declaration, decl, dnum)

        // Forget Subscriber declaration
        decl.elem[0].header = _ZN_DECL_FORGET_SUBSCRIBER;
        if (!sub->key.rname)
            _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_K);

        decl.elem[0].body.forget_sub.key = sub->key;

        z_msg.body.declare.declarations = decl;
        if (_zn_send_z_msg(sub->z, &z_msg, 1) != 0)
        {
            _Z_DEBUG("Trying to reconnect....\n");
            sub->z->on_disconnect(sub->z);
            _zn_send_z_msg(sub->z, &z_msg, 1);
        }
        ARRAY_S_FREE(decl);

        _zn_unregister_subscription(sub->z, 1, s);
    }

    return 0;
}

/*------------------ Write ------------------*/
int zn_write_wo(zn_session_t *z, zn_reskey_t *resource, const unsigned char *payload, size_t length, uint8_t encoding, uint8_t kind, int is_droppable)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    _zn_zenoh_message_t z_msg;
    _ZN_INIT_Z_MSG(z_msg)
    // Set the header
    z_msg.header = _ZN_MID_DATA;
    // Eventually mark the message for congestion control
    _ZN_SET_FLAG(z_msg.header, is_droppable ? _ZN_FLAG_Z_D : 0);
    // Set the resource key
    z_msg.body.data.key.rid = resource->rid;
    z_msg.body.data.key.rname = resource->rname;
    _ZN_SET_FLAG(z_msg.header, resource->rname ? 0 : _ZN_FLAG_Z_K);

    // Set the data info
    _ZN_SET_FLAG(z_msg.header, _ZN_FLAG_Z_I);
    zn_data_info_t info;
    info.flags = 0;
    info.encoding = encoding;
    _ZN_SET_FLAG(info.flags, ZN_DATA_INFO_ENC);
    info.kind = kind;
    _ZN_SET_FLAG(info.flags, ZN_DATA_INFO_KIND);
    z_msg.body.data.info = info;

    // Set the payload
    z_msg.body.data.payload.length = length;
    z_msg.body.data.payload.elem = (uint8_t *)payload;

    return _zn_send_z_msg(z, &z_msg, 1);
}

int zn_write(zn_session_t *z, zn_reskey_t *resource, const unsigned char *payload, size_t length)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    _zn_zenoh_message_t z_msg;
    _ZN_INIT_Z_MSG(z_msg)
    // Set the header
    z_msg.header = _ZN_MID_DATA;
    // Eventually mark the message for congestion control
    _ZN_SET_FLAG(z_msg.header, ZENOH_NET_CONGESTION_CONTROL_DEFAULT ? _ZN_FLAG_Z_D : 0);
    // Set the resource key
    z_msg.body.data.key.rid = resource->rid;
    z_msg.body.data.key.rname = resource->rname;
    _ZN_SET_FLAG(z_msg.header, resource->rname ? 0 : _ZN_FLAG_Z_K);

    // Set the payload
    z_msg.body.data.payload.length = length;
    z_msg.body.data.payload.elem = (uint8_t *)payload;

    return _zn_send_z_msg(z, &z_msg, 1);
}

/*------------------ Read ------------------*/

int zn_read(zn_session_t *z)
{
    _zn_session_message_p_result_t r_s = _zn_recv_s_msg(z);
    if (r_s.tag == _z_res_t_OK)
    {
        int res = _zn_handle_session_message(z, r_s.value.session_message);
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
int zn_send_keep_alive(zn_session_t *z)
{
    _zn_session_message_t s_msg = _zn_session_message_init();
    s_msg.header = _ZN_MID_KEEP_ALIVE;

    return _zn_send_s_msg(z, &s_msg);
}

// int zn_pull(zn_subscriber_t *sub)
// {
//     _zn_message_t msg;
//     msg.header = _ZN_PULL | _ZN_F_FLAG | _ZN_S_FLAG;
//     msg.payload.pull.sn = sub->z->sn++;
//     msg.payload.pull.id = sub->rid;
//     if (_zn_send_msg(sub->z->sock, &sub->z->wbuf, &msg) != 0)
//     {
//         _Z_DEBUG("Trying to reconnect....\n");
//         sub->z->on_disconnect(sub->z);
//         return _zn_send_msg(sub->z->sock, &sub->z->wbuf, &msg);
//     }
//     return 0;
// }

// typedef struct
// {
//     zn_session_t *z;
//     char *resource;
//     char *predicate;
//     zn_reply_handler_t reply_handler;
//     void *arg;
//     zn_query_dest_t dest_storages;
//     zn_query_dest_t dest_evals;
//     atomic_int nb_qhandlers;
//     atomic_flag sent_final;
// } local_query_handle_t;

// void send_local_replies(void *query_handle, zn_resource_p_array_t replies, char eval)
// {
//     unsigned int i;
//     zn_reply_value_t rep;
//     local_query_handle_t *handle = (local_query_handle_t *)query_handle;
//     for (i = 0; i < replies.length; ++i)
//     {
//         if (eval)
//         {
//             rep.kind = ZN_EVAL_DATA;
//         }
//         else
//         {
//             rep.kind = ZN_STORAGE_DATA;
//         }
//         rep.srcid = handle->z->pid.elem;
//         rep.srcid_length = handle->z->pid.length;
//         rep.rsn = i;
//         rep.rname = replies.elem[i]->rname;
//         rep.info.flags = _ZN_ENCODING | _ZN_KIND;
//         rep.info.encoding = replies.elem[i]->encoding;
//         rep.info.kind = replies.elem[i]->kind;
//         rep.data = replies.elem[i]->data;
//         rep.data_length = replies.elem[i]->length;
//         handle->reply_handler(&rep, handle->arg);
//     }
//     memset(&rep, 0, sizeof(zn_reply_value_t));
//     if (eval)
//     {
//         rep.kind = ZN_EVAL_FINAL;
//     }
//     else
//     {
//         rep.kind = ZN_STORAGE_FINAL;
//     }
//     rep.srcid = handle->z->pid.elem;
//     rep.srcid_length = handle->z->pid.length;
//     rep.rsn = i;
//     handle->reply_handler(&rep, handle->arg);

//     atomic_fetch_sub(&handle->nb_qhandlers, 1);
//     if (handle->nb_qhandlers <= 0 && !atomic_flag_test_and_set(&handle->sent_final))
//     {
//         _zn_message_t msg;
//         msg.header = _ZN_QUERY;
//         msg.payload.query.pid = handle->z->pid;
//         msg.payload.query.qid = handle->z->qid++;
//         msg.payload.query.rname = handle->resource;
//         msg.payload.query.predicate = handle->predicate;

//         if (handle->dest_storages.kind != ZN_BEST_MATCH || handle->dest_evals.kind != ZN_BEST_MATCH)
//         {
//             msg.header = _ZN_QUERY | _ZN_P_FLAG;
//             _z_vec_t ps = _z_vec_make(2);
//             if (handle->dest_storages.kind != ZN_BEST_MATCH)
//             {
//                 zn_property_t dest_storages_prop = {ZN_DEST_STORAGES_KEY, {1, (uint8_t *)&handle->dest_storages.kind}};
//                 _z_vec_append(&ps, &dest_storages_prop);
//             }
//             if (handle->dest_evals.kind != ZN_BEST_MATCH)
//             {
//                 zn_property_t dest_evals_prop = {ZN_DEST_EVALS_KEY, {1, (uint8_t *)&handle->dest_evals.kind}};
//                 _z_vec_append(&ps, &dest_evals_prop);
//             }
//             msg.properties = &ps;
//         }

//         _zn_register_query(handle->z, msg.payload.query.qid, handle->reply_handler, handle->arg);
//         if (_zn_send_msg(handle->z->sock, &handle->z->wbuf, &msg) != 0)
//         {
//             _Z_DEBUG("Trying to reconnect....\n");
//             handle->z->on_disconnect(handle->z);
//             _zn_send_msg(handle->z->sock, &handle->z->wbuf, &msg);
//         }
//         free(handle);
//     }
// }

// void send_local_storage_replies(void *query_handle, zn_resource_p_array_t replies)
// {
//     send_local_replies(query_handle, replies, 0);
// }

// void send_local_eval_replies(void *query_handle, zn_resource_p_array_t replies)
// {
//     send_local_replies(query_handle, replies, 1);
// }

// int zn_query_wo(zn_session_t *z, const char *resource, const char *predicate, zn_reply_handler_t reply_handler, void *arg, zn_query_dest_t dest_storages, zn_query_dest_t dest_evals)
// {
//     _z_list_t *stos = dest_storages.kind == ZN_NONE ? 0 : _zn_get_storages_by_rname(z, resource);
//     _z_list_t *evals = dest_evals.kind == ZN_NONE ? 0 : _zn_get_evals_by_rname(z, resource);
//     if (stos != 0 || evals != 0)
//     {
//         _zn_sto_t *sto;
//         _zn_eva_t *eval;
//         _z_list_t *lit;

//         local_query_handle_t *handle = malloc(sizeof(local_query_handle_t));
//         handle->z = z;
//         handle->resource = (char *)resource;
//         handle->predicate = (char *)predicate;
//         handle->reply_handler = reply_handler;
//         handle->arg = arg;
//         handle->dest_storages = dest_storages;
//         handle->dest_evals = dest_evals;

//         int nb_qhandlers = 0;
//         if (stos != 0)
//         {
//             nb_qhandlers += _z_list_len(stos);
//         }
//         if (evals != 0)
//         {
//             nb_qhandlers += _z_list_len(evals);
//         }
//         atomic_init(&handle->nb_qhandlers, nb_qhandlers);
//         atomic_flag_clear(&handle->sent_final);

//         if (stos != 0)
//         {
//             lit = stos;
//             while (lit != 0)
//             {
//                 sto = _z_list_head(lit);
//                 sto->query_handler(resource, predicate, send_local_storage_replies, handle, sto->arg);
//                 lit = _z_list_tail(lit);
//             }
//             _z_list_free(&stos);
//         }

//         if (evals != 0)
//         {
//             lit = evals;
//             while (lit != 0)
//             {
//                 eval = _z_list_head(lit);
//                 eval->query_handler(resource, predicate, send_local_eval_replies, handle, eval->arg);
//                 lit = _z_list_tail(lit);
//             }
//             _z_list_free(&evals);
//         }
//     }
//     else
//     {
//         _zn_message_t msg;
//         msg.header = _ZN_QUERY;
//         msg.payload.query.pid = z->pid;
//         msg.payload.query.qid = z->qid++;
//         msg.payload.query.rname = (char *)resource;
//         msg.payload.query.predicate = (char *)predicate;

//         if (dest_storages.kind != ZN_BEST_MATCH || dest_evals.kind != ZN_BEST_MATCH)
//         {
//             msg.header = _ZN_QUERY | _ZN_P_FLAG;
//             _z_vec_t ps = _z_vec_make(2);
//             if (dest_storages.kind != ZN_BEST_MATCH)
//             {
//                 zn_property_t dest_storages_prop = {ZN_DEST_STORAGES_KEY, {1, (uint8_t *)&dest_storages.kind}};
//                 _z_vec_append(&ps, &dest_storages_prop);
//             }
//             if (dest_evals.kind != ZN_BEST_MATCH)
//             {
//                 zn_property_t dest_evals_prop = {ZN_DEST_EVALS_KEY, {1, (uint8_t *)&dest_evals.kind}};
//                 _z_vec_append(&ps, &dest_evals_prop);
//             }
//             msg.properties = &ps;
//         }

//         _zn_register_query(z, msg.payload.query.qid, reply_handler, arg);
//         if (_zn_send_msg(z->sock, &z->wbuf, &msg) != 0)
//         {
//             _Z_DEBUG("Trying to reconnect....\n");
//             z->on_disconnect(z);
//             _zn_send_msg(z->sock, &z->wbuf, &msg);
//         }
//     }
//     return 0;
// }

// int zn_query(zn_session_t *z, const char *resource, const char *predicate, zn_reply_handler_t reply_handler, void *arg)
// {
//     zn_query_dest_t best_match = {ZN_BEST_MATCH, 0};
//     return zn_query_wo(z, resource, predicate, reply_handler, arg, best_match, best_match);
// }
