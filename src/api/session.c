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

zn_session_t *_zn_open(z_str_t locator, int mode)
{
    zn_session_t *zn = _zn_session_init();

    _zn_transport_p_result_t res = _zn_new_transport(zn->tp_manager, locator, mode);
    if (res.tag == _z_res_t_ERR)
        return NULL;

    // Attach session and transport to one another
    zn->tp = res.value.transport;
    if (zn->tp->type == _ZN_TRANSPORT_UNICAST_TYPE)
        zn->tp->transport.unicast.session = zn;
    else if (zn->tp->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        ; // TODO: to be implemented

    return zn;
}

zn_session_t *zn_open(zn_properties_t *config)
{
    if (config == NULL)
        return NULL;

    z_str_t locator = NULL;
    // Scout if peer is not configured
    if (zn_properties_get(config, ZN_CONFIG_PEER_KEY).val == NULL)
    {
        // ZN_CONFIG_SCOUTING_TIMEOUT_KEY is expressed in seconds as a float
        // while the scout loop timeout uses milliseconds granularity
        z_str_t tout = zn_properties_get(config, ZN_CONFIG_SCOUTING_TIMEOUT_KEY).val;
        if (tout == NULL)
            tout = ZN_CONFIG_SCOUTING_TIMEOUT_DEFAULT;
        unsigned long timeout = 1000 * strtof(tout, NULL); // FIXME: make it clock_t

        // Scout and return upon the first result
        zn_hello_array_t locs = _zn_scout(ZN_ROUTER, config, timeout, 1);
        if (locs.len > 0)
        {
            if (locs.val[0].locators.len > 0)
                locator = _z_str_clone(locs.val[0].locators.val[0]);
            else
            {
                zn_hello_array_free(locs);
                return NULL;
            }

            zn_hello_array_free(locs);
        }
        else
        {
            _Z_DEBUG("Unable to scout a zenoh router\n");
            _Z_ERROR("%sPlease make sure at least one router is running on your network!\n", "");

            return NULL;
        }
    }
    else
        locator = _z_str_clone(zn_properties_get(config, ZN_CONFIG_PEER_KEY).val);

    // TODO: check invalid configurations
    // For example, client mode in multicast links

    // Check operation mode
    z_str_t s_mode = zn_properties_get(config, ZN_CONFIG_MODE_KEY).val;
    int mode = 0; // By default, zenoh-pico will operate as a client
    if (_z_str_eq(s_mode, ZN_CONFIG_MODE_CLIENT))
        mode = 0;
    else if (_z_str_eq(s_mode, ZN_CONFIG_MODE_PEER))
        mode = 1;

    return _zn_open(locator, mode);
}

void zn_close(zn_session_t *zn)
{
    _zn_session_close(zn, _ZN_CLOSE_GENERIC);
}

zn_properties_t *zn_info(zn_session_t *zn)
{
    zn_properties_t *ps = (zn_properties_t *)malloc(sizeof(zn_properties_t));
    zn_properties_init(ps);
    zn_properties_insert(ps, ZN_INFO_PID_KEY, _z_string_from_bytes(&zn->tp_manager->local_pid));
    // zn_properties_insert(ps, ZN_INFO_ROUTER_PID_KEY, _z_string_from_bytes(&zn->remote_pid));
    return ps;
}

int znp_read(zn_session_t *zn)
{
    return _znp_read(zn->tp);
}

int znp_send_keep_alive(zn_session_t *zn)
{
    return _znp_send_keep_alive(zn->tp);
}

int znp_start_read_task(zn_session_t *zn)
{
    z_task_t *task = (z_task_t *)malloc(sizeof(z_task_t));
    memset(task, 0, sizeof(pthread_t));

    if (zn->tp->type == _ZN_TRANSPORT_UNICAST_TYPE)
    {
        zn->tp->transport.unicast.read_task = task;
        if (z_task_init(task, NULL, _znp_unicast_read_task, &zn->tp->transport.unicast) != 0)
            return -1;
    }
    else if (zn->tp->type == _ZN_TRANSPORT_MULTICAST_TYPE)
    {
        // TODO: to be implemented
    }
    else
        return -1;

    return 0;
}

int znp_stop_read_task(zn_session_t *zn)
{
    if (zn->tp->type == _ZN_TRANSPORT_UNICAST_TYPE)
        zn->tp->transport.unicast.read_task_running = 0;
    else if (zn->tp->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        ; // TODO: to be implemented

    return 0;
}

int znp_start_lease_task(zn_session_t *zn)
{
    z_task_t *task = (z_task_t *)malloc(sizeof(z_task_t));
    memset(task, 0, sizeof(pthread_t));

    if (zn->tp->type == _ZN_TRANSPORT_UNICAST_TYPE)
    {
        zn->tp->transport.unicast.lease_task = task;
        if (z_task_init(task, NULL, _znp_unicast_lease_task, &zn->tp->transport.unicast) != 0)
            return -1;
    }
    else if (zn->tp->type == _ZN_TRANSPORT_MULTICAST_TYPE)
    {
        // TODO: to be implemented
    }
    else
        return -1;

    return 0;
}

int znp_stop_lease_task(zn_session_t *zn)
{
    if (zn->tp->type == _ZN_TRANSPORT_UNICAST_TYPE)
        zn->tp->transport.unicast.lease_task_running = 0;
    else if (zn->tp->type == _ZN_TRANSPORT_MULTICAST_TYPE)
        ; // TODO: to be implemented

    return 0;
}