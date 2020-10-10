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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include "zenoh/private/logging.h"
#include "zenoh/net/session.h"
#include "zenoh/net/property.h"
#include "zenoh/net/recv_loop.h"
#include "zenoh/net/private/net.h"
#include "zenoh/net/private/msgcodec.h"

// -- Some refactoring will be done to support multiple platforms / transports

void default_on_disconnect(void *vz)
{
    zn_session_t *z = (zn_session_t *)vz;
    for (int i = 0; i < 3; ++i)
    {
        sleep(3);
        // Try to reconnect -- eventually we should scout here.
        // We should also re-do declarations.
        _Z_DEBUG("Tring to reconnect...\n");
        _zn_socket_result_t r_sock = _zn_open_tx_session(strdup(z->locator));
        if (r_sock.tag == Z_OK_TAG)
        {
            z->sock = r_sock.value.socket;
            return;
        }
    }
}

z_vec_t _zn_scout_loop(_zn_socket_t socket, const z_iobuf_t *sbuf, const struct sockaddr *dest, socklen_t salen, size_t tries)
{
    struct sockaddr *from = (struct sockaddr *)malloc(2 * sizeof(struct sockaddr_in *));
    socklen_t flen = 0;
    z_iobuf_t hbuf = z_iobuf_make(ZENOH_NET_MAX_SCOUT_MSG_LEN);
    z_vec_t ls = z_vec_uninit();

    while (tries > 0)
    {
        tries--;

        // Send the scout message
        _zn_send_dgram_to(socket, sbuf, dest, salen);
        // Eventually read hello messages
        z_iobuf_clear(&hbuf);
        int len = _zn_recv_dgram_from(socket, &hbuf, from, &flen);

        // Retry if we haven't received anything
        if (len <= 0)
            continue;

        _zn_session_message_p_result_t r_hm = _zn_session_message_decode(&hbuf);
        if (r_hm.tag == Z_ERROR_TAG)
        {
            perror("Scouting loop received malformed message\n");
            continue;
        }

        _zn_session_message_t *s_msg = r_hm.value.session_message;
        switch (_ZN_MID(s_msg->header))
        {
        case _ZN_MID_HELLO:
        {
            if _ZN_HAS_FLAG (s_msg->header, _ZN_FLAG_S_L)
            {
                // Clone the locators vector
                ls = z_vec_clone(&s_msg->body.hello.locators);
                // Free the internal memory allocation of the vector without freeing the
                // element, i.e., the locators
                z_vec_free_inner(&s_msg->body.hello.locators);
            }
            else
            {
                // @TODO: construct the locator departing from the from sock address
            }

            break;
        }
        default:
            perror("Scouting loop received unexpected message\n");
            break;
        }

        _zn_session_message_free(s_msg);
        _zn_session_message_p_result_free(&r_hm);

        if (z_vec_is_init(&ls))
            break;
        else
            continue;
    }

    free(from);
    z_iobuf_free(&hbuf);

    return ls;
}

z_vec_t zn_scout(char *iface, size_t tries, size_t period)
{
    int is_auto = 0;
    char *addr = iface;
    if (!iface || (strcmp(iface, "auto") == 0))
    {
        addr = _zn_select_scout_iface();
        is_auto = 1;
    }

    z_iobuf_t sbuf = z_iobuf_make(ZENOH_NET_MAX_SCOUT_MSG_LEN);
    _zn_session_message_t scout;
    scout.header = _ZN_MID_SCOUT;
    // NOTE: when W flag is set to 0 in the header, it means implicitly scouting for Routers
    //       and the what value is not sent on the wire. Here we scout for Routers
    _zn_session_message_encode(&sbuf, &scout);

    _zn_socket_result_t r = _zn_create_udp_socket(addr, 0, period);
    ASSERT_RESULT(r, "Unable to create scouting socket\n");

    socklen_t salen = sizeof(struct sockaddr_in);
    // Scout first on localhost
    struct sockaddr_in *laddr = _zn_make_socket_address(addr, ZENOH_NET_SCOUT_PORT);
    z_vec_t locs = _zn_scout_loop(r.value.socket, &sbuf, (struct sockaddr *)laddr, salen, tries);
    free(laddr);

    if (z_vec_length(&locs) == 0)
    {
        // We did not find a router on localhost, hence scout on the LAN
        struct sockaddr_in *maddr = _zn_make_socket_address(ZENOH_NET_SCOUT_MCAST_ADDR, ZENOH_NET_SCOUT_PORT);
        locs = _zn_scout_loop(r.value.socket, &sbuf, (struct sockaddr *)maddr, salen, tries);
        free(maddr);
    }

    if (is_auto)
        free(addr);
    z_iobuf_free(&sbuf);

    return locs;
}

zn_session_p_result_t zn_open(char *locator, zn_on_disconnect_t on_disconnect, const z_vec_t *ps)
{
    zn_session_p_result_t r;
    int locator_is_scouted = 0;
    if (!locator)
    {
        z_vec_t locs = zn_scout("auto", ZENOH_NET_SCOUT_TRIES, ZENOH_NET_SCOUT_TIMEOUT);
        if (z_vec_length(&locs) > 0)
        {
            locator = strdup((const char *)z_vec_get(&locs, 0));
            // Mark that the locator has been scouted, need to be freed before returning
            locator_is_scouted = 1;
            // Free all the scouted locators
            z_vec_free(&locs);
        }
        else
        {
            perror("Unable to scout a zenoh router ");
            _Z_ERROR("%sPlease make sure one is running on your network!\n", "");
            r.tag = Z_ERROR_TAG;
            r.value.error = ZN_TX_CONNECTION_ERROR;
            // Free all the scouted locators
            z_vec_free(&locs);

            return r;
        }
    }

    // Initialize the session to null
    r.value.session = NULL;
    srand(clock());

    // Attempt to open a socket
    _zn_socket_result_t r_sock = _zn_open_tx_session(locator);
    if (r_sock.tag == Z_ERROR_TAG)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = ZN_IO_ERROR;

        if (locator_is_scouted)
            free(locator);

        return r;
    }

    r.tag = Z_OK_TAG;

    // Randomly generate a peer ID
    ARRAY_S_DEFINE(uint8_t, uint8, z_, pid, ZENOH_NET_PID_LENGTH);
    for (int i = 0; i < ZENOH_NET_PID_LENGTH; ++i)
        pid.elem[i] = rand() % 255;

    // Build the open message
    _zn_session_message_t om;

    // Add an attachement if properties have been provided
    if (ps)
    {
        om.attachment = (_zn_attachment_t *)malloc(sizeof(_zn_attachment_t));
        om.attachment->header = _ZN_MID_OPEN | _ZN_ATT_ENC_PROPERTIES;
        om.attachment->payload = z_iobuf_make(ZENOH_NET_ATTACHMENT_BUF_LEN);
        zn_properties_encode(&om.attachment->payload, ps);
    }
    else
    {
        om.attachment = NULL;
    }

    om.header = _ZN_MID_OPEN;
    om.body.open.version = ZENOH_NET_PROTO_VERSION;
    om.body.open.whatami = ZN_WHATAMI_CLIENT;
    om.body.open.opid = pid;
    om.body.open.lease = ZENOH_NET_DEFAULT_LEASE;
    om.body.open.initial_sn = (z_zint_t)rand() % ZENOH_NET_SN_RESOLUTION;
    om.body.open.sn_resolution = ZENOH_NET_SN_RESOLUTION;
    om.body.open.options = 0;

    if (ZENOH_NET_SN_RESOLUTION != ZENOH_NET_SN_RESOLUTION_DEFAULT)
        _ZN_SET_FLAG(om.body.open.options, _ZN_FLAG_S_S);
    // NOTE: optionally the open can include a list of locators the opener
    //       is reachable at. Since zenoh-pico acts as a client, this is not
    //       needed because a client is not expected to receive opens.
    _ZN_SET_FLAG(om.header, om.body.open.options ? _ZN_FLAG_S_O : 0);

    _Z_DEBUG("Sending Open\n");
    // Create write buffer
    z_iobuf_t wbuf = z_iobuf_make(ZENOH_NET_WRITE_BUF_LEN);
    // Encode and send the message
    _zn_send_s_msg(r_sock.value.socket, &wbuf, &om);

    // Create read buffer
    z_iobuf_t rbuf = z_iobuf_make(ZENOH_NET_READ_BUF_LEN);

    // Read response message
    z_iobuf_clear(&rbuf);

    _zn_session_message_p_result_t r_msg = _zn_recv_s_msg(r_sock.value.socket, &rbuf);

    if (r_msg.tag == Z_ERROR_TAG)
    {
        r.tag = Z_ERROR_TAG;
        r.value.error = ZN_FAILED_TO_OPEN_SESSION;

        // Free the locator
        if (locator_is_scouted)
            free(locator);

        // Free read and write buffers
        z_iobuf_free(&wbuf);
        z_iobuf_free(&rbuf);

        // Free the message and result
        _zn_session_message_free(&om);

        return r;
    }

    _zn_session_message_t *p_am = r_msg.value.session_message;
    switch (_ZN_MID(p_am->header))
    {
    case _ZN_MID_ACCEPT:
        r.tag = Z_OK_TAG;

        r.value.session = (zn_session_t *)malloc(sizeof(zn_session_t));
        r.value.session->sock = r_sock.value.socket;

        // Set the default values sent in the open for: Lease, SN resolution, Initial SN
        r.value.session->lease = om.body.open.lease;
        r.value.session->sn_resolution = om.body.open.sn_resolution;
        r.value.session->sn_tx_reliable = om.body.open.initial_sn;
        r.value.session->sn_tx_best_effort = om.body.open.initial_sn;

        // If options are present, the above values may be overridden
        if _ZN_HAS_FLAG (p_am->header, _ZN_FLAG_S_O)
        {
            // Handle SN resolution option
            if _ZN_HAS_FLAG (p_am->body.accept.options, _ZN_FLAG_S_S)
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
                    if (om.body.open.initial_sn >= r.value.session->sn_resolution)
                    {
                        r.value.session->sn_tx_reliable = om.body.open.initial_sn % p_am->body.accept.sn_resolution;
                        r.value.session->sn_tx_best_effort = om.body.open.initial_sn % p_am->body.accept.sn_resolution;
                    }
                    r.value.session->sn_resolution = p_am->body.accept.sn_resolution;
                }
                else
                {
                    r.tag = Z_ERROR_TAG;
                    r.value.error = ZN_FAILED_TO_OPEN_SESSION;

                    // Free the locator
                    if (locator_is_scouted)
                        free(locator);

                    // Free read and write buffers
                    z_iobuf_free(&wbuf);
                    z_iobuf_free(&rbuf);

                    // Free the message and result
                    _zn_session_message_free(&om);
                    _zn_session_message_free(p_am);
                    _zn_session_message_p_result_free(&r_msg);

                    return r;
                }
            }
            // Handle Lease option
            if _ZN_HAS_FLAG (p_am->body.accept.options, _ZN_FLAG_S_D)
            {
                // The lease period in the ACCEPT must be less or equal than the lease period in the OPEN,
                // otherwise the ACCEPT message is considered invalid and it should be treated as a
                // CLOSE message with L==0 by the Opener Peer -- the recipient of the ACCEPT message.
                if (p_am->body.accept.lease <= om.body.open.lease)
                {
                    r.value.session->lease = p_am->body.accept.lease;
                }
                else
                {
                    r.tag = Z_ERROR_TAG;
                    r.value.error = ZN_FAILED_TO_OPEN_SESSION;

                    // Free the locator
                    if (locator_is_scouted)
                        free(locator);

                    // Free read and write buffers
                    z_iobuf_free(&wbuf);
                    z_iobuf_free(&rbuf);

                    // Free the message and result
                    _zn_session_message_free(&om);
                    _zn_session_message_free(p_am);
                    _zn_session_message_p_result_free(&r_msg);

                    return r;
                }
            }
            if _ZN_HAS_FLAG (p_am->body.accept.options, _ZN_FLAG_S_L)
            {
                // @TODO: we might want to connect or store the locators
            }
        }

        // The initial SN at RX side
        r.value.session->sn_rx_reliable = p_am->body.accept.initial_sn;
        r.value.session->sn_rx_best_effort = p_am->body.accept.initial_sn;

        // Initialize the counters to 1.
        r.value.session->entity_id = 1;
        r.value.session->resource_id = 1;
        r.value.session->query_id = 1;

        // Initialize the buffers
        r.value.session->rbuf = rbuf;
        r.value.session->wbuf = wbuf;

        // Initialize the Peer IDs
        r.value.session->local_pid = pid;
        ARRAY_S_COPY(uint8_t, r.value.session->remote_pid, r_msg.value.session_message->body.accept.apid);

        r.value.session->locator = strdup(locator);

        r.value.session->local_resources = z_list_empty;
        r.value.session->remote_resources = z_list_empty;

        r.value.session->local_subscriptions = z_list_empty;
        r.value.session->remote_subscriptions = z_list_empty;
        r.value.session->rem_loc_sub_map = z_i_map_make(DEFAULT_I_MAP_CAPACITY);

        r.value.session->local_queryables = z_list_empty;
        r.value.session->replywaiters = z_list_empty;

        r.value.session->on_disconnect = on_disconnect != 0 ? on_disconnect : &default_on_disconnect;
        r.value.session->running = 0;
        r.value.session->thread = 0;

        break;
    default:
        r.tag = Z_ERROR_TAG;
        r.value.error = ZN_FAILED_TO_OPEN_SESSION;

        // Free read and write buffers
        z_iobuf_free(&wbuf);
        z_iobuf_free(&rbuf);

        break;
    }

    // Free the locator
    if (locator_is_scouted)
        free(locator);

    // Free the messages and result
    _zn_session_message_free(&om);
    _zn_session_message_free(p_am);
    _zn_session_message_p_result_free(&r_msg);

    return r;
}

// z_vec_t zn_info(zn_session_t *z)
// {
//     z_vec_t res = z_vec_make(3);
//     zn_property_t *pid = zn_property_make(ZN_INFO_PID_KEY, z->pid);
//     z_uint8_array_t locator;
//     locator.length = strlen(z->locator) + 1;
//     locator.elem = (uint8_t *)z->locator;
//     zn_property_t *peer = zn_property_make(ZN_INFO_PEER_KEY, locator);
//     zn_property_t *peer_pid = zn_property_make(ZN_INFO_PEER_PID_KEY, z->peer_pid);

//     z_vec_set(&res, ZN_INFO_PID_KEY, pid);
//     z_vec_set(&res, ZN_INFO_PEER_KEY, peer);
//     z_vec_set(&res, ZN_INFO_PEER_PID_KEY, peer_pid);
//     res.length_ = 3;

//     return res;
// }

int zn_close(zn_session_t *z)
{
    _zn_session_message_t c;
    c.attachment = NULL;
    c.header = _ZN_MID_CLOSE;
    c.body.close.pid = z->local_pid;
    c.body.close.reason = _ZN_CLOSE_GENERIC;
    _ZN_SET_FLAG(c.header, _ZN_FLAG_S_I);
    // NOTE: we are closing the whole zenoh session.
    //       So, the K flag in the close message is set to 0
    int rv = _zn_send_s_msg(z->sock, &z->wbuf, &c);
    close(z->sock);

    return rv;
}

/*------------------  Resource Declaration ------------------*/
zn_res_p_result_t zn_declare_resource(zn_session_t *z, const char *resource)
{
    // Create the resource key to declare
    zn_res_key_t res_key;
    res_key.rid = ZN_NO_RESOURCE_ID;
    res_key.rname = (char *)resource;

    // Create the result
    zn_res_p_result_t r;
    r.tag = Z_OK_TAG;
    r.value.res = (zn_res_t *)malloc(sizeof(zn_res_t));
    r.value.res->z = z;
    r.value.res->id = _zn_get_resource_id(z, &res_key);
    r.value.res->key = res_key;

    _zn_zenoh_message_t z_msg;
    z_msg.attachment = NULL;
    z_msg.reply_context = NULL;
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

int zn_undeclare_resource(zn_res_t *res)
{
    _zn_res_decl_t *r = _zn_get_resource_by_id(res->z->local_resources, res->id);
    if (r)
    {
        _zn_zenoh_message_t z_msg;
        z_msg.attachment = NULL;
        z_msg.reply_context = NULL;
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
zn_pub_p_result_t zn_declare_publisher(zn_session_t *z, const zn_res_key_t *res_key)
{
    zn_pub_p_result_t r;
    r.tag = Z_OK_TAG;
    r.value.pub = (zn_pub_t *)malloc(sizeof(zn_pub_t));
    r.value.pub->z = z;
    r.value.pub->key.rid = res_key->rid;
    if (res_key->rname)
        r.value.pub->key.rname = strdup(res_key->rname);
    else
        r.value.pub->key.rname = NULL;
    r.value.pub->id = _zn_get_entity_id(z);

    _zn_zenoh_message_t z_msg;
    z_msg.attachment = NULL;
    z_msg.reply_context = NULL;
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

int zn_undeclare_publisher(zn_pub_t *pub)
{
    // @TODO: manage multi publishers

    _zn_zenoh_message_t z_msg;
    z_msg.attachment = NULL;
    z_msg.reply_context = NULL;
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
zn_sub_p_result_t zn_declare_subscriber(zn_session_t *z, const zn_res_key_t *res_key, const zn_sub_info_t *si, zn_data_handler_t data_handler, void *arg)
{
    zn_sub_p_result_t r;
    r.tag = Z_OK_TAG;
    r.value.sub = (zn_sub_t *)malloc(sizeof(zn_sub_t));
    r.value.sub->z = z;
    r.value.sub->id = _zn_get_entity_id(z);
    r.value.sub->key.rid = res_key->rid;
    if (res_key->rname)
        r.value.sub->key.rname = strdup(res_key->rname);
    else
        r.value.sub->key.rname = NULL;

    _zn_zenoh_message_t z_msg;
    z_msg.attachment = NULL;
    z_msg.reply_context = NULL;
    z_msg.header = _ZN_MID_DECLARE;

    // We need to declare the subscriber
    int dnum = 1;
    _ZN_ARRAY_S_DEFINE(declaration, decl, dnum)

    // Subscriber declaration
    decl.elem[0].header = _ZN_DECL_SUBSCRIBER;
    if (!res_key->rname)
        _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_K);
    if (si->mode != ZN_PUSH_MODE || si->is_periodic)
        _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_S);
    if (si->is_reliable)
        _ZN_SET_FLAG(decl.elem[0].header, _ZN_FLAG_Z_R);

    decl.elem[0].body.sub.key = r.value.sub->key;

    // SubMode
    decl.elem[0].body.sub.sub_info.mode = si->mode;
    decl.elem[0].body.sub.sub_info.is_reliable = si->is_reliable;
    decl.elem[0].body.sub.sub_info.is_periodic = si->is_periodic;
    decl.elem[0].body.sub.sub_info.period = si->period;

    z_msg.body.declare.declarations = decl;
    if (_zn_send_z_msg(z, &z_msg, 1) != 0)
    {
        _Z_DEBUG("Trying to reconnect....\n");
        z->on_disconnect(z);
        _zn_send_z_msg(z, &z_msg, 1);
    }
    ARRAY_S_FREE(decl);
    _zn_register_subscription(z, 1, r.value.sub->id, res_key, data_handler, arg);

    return r;
}

int zn_undeclare_subscriber(zn_sub_t *sub)
{
    _zn_sub_t *s = _zn_get_subscription_by_id(sub->z->local_subscriptions, sub->id);
    if (s)
    {
        _zn_zenoh_message_t z_msg;
        z_msg.attachment = NULL;
        z_msg.reply_context = NULL;
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

        _zn_unregister_subscription(sub);
    }

    return 0;
}

int zn_write_wo(zn_session_t *z, zn_res_key_t *resource, const unsigned char *payload, size_t length, uint8_t encoding, uint8_t kind, int is_droppable)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    _zn_zenoh_message_t z_msg;
    // Set the message decorators
    z_msg.attachment = NULL;
    z_msg.reply_context = NULL;
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
    z_msg.body.data.payload = z_iobuf_wrap_wo((unsigned char *)payload, length, 0, length);

    return _zn_send_z_msg(z, &z_msg, 1);
}

int zn_write(zn_session_t *z, zn_res_key_t *resource, const unsigned char *payload, size_t length)
{
    // @TODO: Need to verify that I have declared a publisher with the same resource key.
    //        Then, need to verify there are active subscriptions matching the publisher.
    // @TODO: Need to check subscriptions to determine the right reliability value.

    _zn_zenoh_message_t z_msg;
    // Set the message decorators
    z_msg.attachment = NULL;
    z_msg.reply_context = NULL;
    // Set the header
    z_msg.header = _ZN_MID_DATA;
    // Eventually mark the message for congestion control
    _ZN_SET_FLAG(z_msg.header, ZENOH_NET_CONGESTION_CONTROL_DEFAULT ? _ZN_FLAG_Z_D : 0);
    // Set the resource key
    z_msg.body.data.key.rid = resource->rid;
    z_msg.body.data.key.rname = resource->rname;
    _ZN_SET_FLAG(z_msg.header, resource->rname ? 0 : _ZN_FLAG_Z_K);

    // Set the payload
    z_msg.body.data.payload = z_iobuf_wrap_wo((unsigned char *)payload, length, 0, length);

    return _zn_send_z_msg(z, &z_msg, 1);
}

// int zn_pull(zn_sub_t *sub)
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
//             z_vec_t ps = z_vec_make(2);
//             if (handle->dest_storages.kind != ZN_BEST_MATCH)
//             {
//                 zn_property_t dest_storages_prop = {ZN_DEST_STORAGES_KEY, {1, (uint8_t *)&handle->dest_storages.kind}};
//                 z_vec_append(&ps, &dest_storages_prop);
//             }
//             if (handle->dest_evals.kind != ZN_BEST_MATCH)
//             {
//                 zn_property_t dest_evals_prop = {ZN_DEST_EVALS_KEY, {1, (uint8_t *)&handle->dest_evals.kind}};
//                 z_vec_append(&ps, &dest_evals_prop);
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
//     z_list_t *stos = dest_storages.kind == ZN_NONE ? 0 : _zn_get_storages_by_rname(z, resource);
//     z_list_t *evals = dest_evals.kind == ZN_NONE ? 0 : _zn_get_evals_by_rname(z, resource);
//     if (stos != 0 || evals != 0)
//     {
//         _zn_sto_t *sto;
//         _zn_eva_t *eval;
//         z_list_t *lit;

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
//             nb_qhandlers += z_list_len(stos);
//         }
//         if (evals != 0)
//         {
//             nb_qhandlers += z_list_len(evals);
//         }
//         atomic_init(&handle->nb_qhandlers, nb_qhandlers);
//         atomic_flag_clear(&handle->sent_final);

//         if (stos != 0)
//         {
//             lit = stos;
//             while (lit != 0)
//             {
//                 sto = z_list_head(lit);
//                 sto->query_handler(resource, predicate, send_local_storage_replies, handle, sto->arg);
//                 lit = z_list_tail(lit);
//             }
//             z_list_free(&stos);
//         }

//         if (evals != 0)
//         {
//             lit = evals;
//             while (lit != 0)
//             {
//                 eval = z_list_head(lit);
//                 eval->query_handler(resource, predicate, send_local_eval_replies, handle, eval->arg);
//                 lit = z_list_tail(lit);
//             }
//             z_list_free(&evals);
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
//             z_vec_t ps = z_vec_make(2);
//             if (dest_storages.kind != ZN_BEST_MATCH)
//             {
//                 zn_property_t dest_storages_prop = {ZN_DEST_STORAGES_KEY, {1, (uint8_t *)&dest_storages.kind}};
//                 z_vec_append(&ps, &dest_storages_prop);
//             }
//             if (dest_evals.kind != ZN_BEST_MATCH)
//             {
//                 zn_property_t dest_evals_prop = {ZN_DEST_EVALS_KEY, {1, (uint8_t *)&dest_evals.kind}};
//                 z_vec_append(&ps, &dest_evals_prop);
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
