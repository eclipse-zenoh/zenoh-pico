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

#include "zenoh-pico/api/session.h"
#include "zenoh-pico/api/logger.h"
#include "zenoh-pico/api/memory.h"
#include "zenoh-pico/session/utils.h"
#include "zenoh-pico/protocol/utils.h"
#include "zenoh-pico/transport/utils.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/transport/link/task/lease.h"
#include "zenoh-pico/transport/link/task/read.h"

zn_session_t *zn_open(zn_properties_t *config)
{
    zn_session_t *zn = NULL;

    int locator_is_scouted = 0;
    z_str_t locator = zn_properties_get(config, ZN_CONFIG_PEER_KEY).val;

    if (locator == NULL)
    {
        // Scout for routers
        unsigned int what = ZN_ROUTER;
        z_str_t mode = zn_properties_get(config, ZN_CONFIG_MODE_KEY).val;
        if (mode == NULL)
        {
            return zn;
        }

        // The ZN_CONFIG_SCOUTING_TIMEOUT_KEY is expressed in seconds as a float while the
        // scout loop timeout uses milliseconds granularity
        z_str_t to = zn_properties_get(config, ZN_CONFIG_SCOUTING_TIMEOUT_KEY).val;
        if (to == NULL)
        {
            to = ZN_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
        }
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
    srand(time(NULL));

    // Attempt to configure the link
    _zn_link_p_result_t r_link = _zn_open_link(locator, 0);
    if (r_link.tag == _z_res_t_ERR)
    {
        if (locator_is_scouted)
            free((z_str_t)locator);

        return zn;
    }

    // Randomly generate a peer ID
    z_bytes_t pid = _z_bytes_make(ZN_PID_LENGTH);
    for (unsigned int i = 0; i < pid.len; i++)
        ((uint8_t *)pid.val)[i] = rand() % 255;

    // Build the open message
    _zn_transport_message_t ism = _zn_transport_message_init(_ZN_MID_INIT);

    ism.body.init.options = 0;
    ism.body.init.version = ZN_PROTO_VERSION;
    ism.body.init.whatami = ZN_CLIENT;
    ism.body.init.pid = pid;
    ism.body.init.sn_resolution = ZN_SN_RESOLUTION;

    if (ZN_SN_RESOLUTION != ZN_SN_RESOLUTION_DEFAULT)
        _ZN_SET_FLAG(ism.header, _ZN_FLAG_T_S);

    // Initialize the session
    zn = _zn_session_init();
    zn->link = r_link.value.link;

    _Z_DEBUG("Sending InitSyn\n");
    // Encode and send the message
    int res = _zn_send_t_msg(zn, &ism);
    if (res != 0)
    {
        // Free the pid
        _z_bytes_free(&pid);

        // Free the locator
        if (locator_is_scouted)
            free((z_str_t)locator);

        // Free
        _zn_transport_message_free(&ism);
        _zn_session_free(&zn);

        return zn;
    }

    _zn_transport_message_p_result_t r_msg = _zn_recv_t_msg(zn);
    if (r_msg.tag == _z_res_t_ERR)
    {
        // Free the pid
        _z_bytes_free(&pid);

        // Free the locator
        if (locator_is_scouted)
            free((z_str_t)locator);

        // Free
        _zn_transport_message_free(&ism);
        _zn_session_free(&zn);

        return zn;
    }

    _zn_transport_message_t *p_iam = r_msg.value.transport_message;
    switch (_ZN_MID(p_iam->header))
    {
    case _ZN_MID_INIT:
    {
        if _ZN_HAS_FLAG (p_iam->header, _ZN_FLAG_T_A)
        {
            // The announced sn resolution
            zn->sn_resolution = ism.body.init.sn_resolution;
            zn->sn_resolution_half = zn->sn_resolution / 2;

            // Handle SN resolution option if present
            if _ZN_HAS_FLAG (p_iam->header, _ZN_FLAG_T_S)
            {
                // The resolution in the InitAck must be less or equal than the resolution in the InitSyn,
                // otherwise the InitAck message is considered invalid and it should be treated as a
                // CLOSE message with L==0 by the Initiating Peer -- the recipient of the InitAck message.
                if (p_iam->body.init.sn_resolution <= ism.body.init.sn_resolution)
                {
                    zn->sn_resolution = p_iam->body.init.sn_resolution;
                    zn->sn_resolution_half = zn->sn_resolution / 2;
                }
                else
                {
                    // Close the session
                    _zn_session_close(zn, _ZN_CLOSE_INVALID);
                    break;
                }
            }

            // The initial SN at TX side
            z_zint_t initial_sn = (z_zint_t)rand() % zn->sn_resolution;
            zn->sn_tx_reliable = initial_sn;
            zn->sn_tx_best_effort = initial_sn;

            // Create the OpenSyn message
            _zn_transport_message_t osm = _zn_transport_message_init(_ZN_MID_OPEN);
            osm.body.open.lease = ZN_TRANSPORT_LEASE;
            if (ZN_TRANSPORT_LEASE % 1000 == 0)
                _ZN_SET_FLAG(osm.header, _ZN_FLAG_T_T2);
            osm.body.open.initial_sn = initial_sn;
            osm.body.open.cookie = p_iam->body.init.cookie;

            _Z_DEBUG("Sending OpenSyn\n");
            // Encode and send the message
            int res = _zn_send_t_msg(zn, &osm);
            _zn_transport_message_free(&osm);
            if (res != 0)
            {
                _z_bytes_free(&pid);
                if (locator_is_scouted)
                    free((z_str_t)locator);
                _zn_session_free(&zn);
                break;
            }

            // Initialize the Local and Remote Peer IDs
            _z_bytes_move(&zn->local_pid, &pid);
            _z_bytes_copy(&zn->remote_pid, &p_iam->body.init.pid);

            if (locator_is_scouted)
                zn->locator = (z_str_t)locator;
            else
                zn->locator = strdup(locator);

            break;
        }
        else
        {
            // Close the session
            _zn_session_close(zn, _ZN_CLOSE_INVALID);
            break;
        }
    }

    default:
    {
        // Close the session
        _zn_session_close(zn, _ZN_CLOSE_INVALID);
        break;
    }
    }

    // Free the messages and result
    _zn_transport_message_free(&ism);
    _zn_transport_message_free(p_iam);
    _zn_transport_message_p_result_free(&r_msg);

    return zn;
}

void zn_close(zn_session_t *zn)
{
    _zn_session_close(zn, _ZN_CLOSE_GENERIC);
    return;
}

zn_properties_t *zn_info(zn_session_t *zn)
{
    zn_properties_t *ps = (zn_properties_t *)malloc(sizeof(zn_properties_t));
    zn_properties_init(ps);
    zn_properties_insert(ps, ZN_INFO_PID_KEY, _z_string_from_bytes(&zn->local_pid));
    zn_properties_insert(ps, ZN_INFO_ROUTER_PID_KEY, _z_string_from_bytes(&zn->remote_pid));
    return ps;
}

int znp_read(zn_session_t *zn)
{
    return _znp_read(zn);
}

int znp_send_keep_alive(zn_session_t *zn)
{
    return _znp_send_keep_alive(zn);
}

int znp_start_read_task(zn_session_t *z)
{
    z_task_t *task = (z_task_t *)malloc(sizeof(z_task_t));
    memset(task, 0, sizeof(pthread_t));
    z->read_task = task;
    if (z_task_init(task, NULL, _znp_read_task, z) != 0)
    {
        return -1;
    }
    return 0;
}

int znp_stop_read_task(zn_session_t *z)
{
    z->read_task_running = 0;
    return 0;
}

int znp_start_lease_task(zn_session_t *zn)
{
    z_task_t *task = (z_task_t *)malloc(sizeof(z_task_t));
    memset(task, 0, sizeof(pthread_t));
    zn->lease_task = task;
    if (z_task_init(task, NULL, _znp_lease_task, zn) != 0)
    {
        return -1;
    }
    return 0;
}

int znp_stop_lease_task(zn_session_t *zn)
{
    zn->lease_task_running = 0;
    return 0;
}
