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
#include "zenoh-pico/protocol/private/msgcodec.h"
#include "zenoh-pico/protocol/private/utils.h"
#include "zenoh-pico/transport/private/manager.h"
#include "zenoh-pico/system/common.h"
#include "zenoh-pico/utils/collections.h"
#include "zenoh-pico/utils/types.h"
#include "zenoh-pico/utils/private/logging.h"

zn_hello_array_t _zn_scout_loop(
    const _z_wbuf_t *wbf,
    z_str_t mcast,
    int port,
    clock_t period,
    int exit_on_first)
{
    // Define an empty array
    zn_hello_array_t ls;
    ls.len = 0;
    ls.val = NULL;

    _zn_link_t *scout_link = _zn_new_udp_link(mcast, port);
    scout_link->o_func(scout_link);

    // Send the scout message
    int res = _zn_send_wbuf(scout_link, wbf);
    if (res < 0)
    {
        _Z_DEBUG("Unable to send scout message\n");
        printf("Return here\n");
        return ls;
    }

    // The receiving buffer
    _z_zbuf_t zbf = _z_zbuf_make(ZN_READ_BUF_LEN);

    z_clock_t start = z_clock_now();
    while (z_clock_elapsed_ms(&start) < period)
    {
        // Eventually read hello messages
        _z_zbuf_clear(&zbf);

        // Read bytes from the socket
        int len = _zn_recv_zbuf(scout_link, &zbf);
        if (len == -1)
            continue;

        _zn_transport_message_p_result_t r_hm = _zn_transport_message_decode(&zbf);
        if (r_hm.tag == _z_res_t_ERR)
        {
            _Z_DEBUG("Scouting loop received malformed message\n");
            continue;
        }

        _zn_transport_message_t *t_msg = r_hm.value.transport_message;
        switch (_ZN_MID(t_msg->header))
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

            if _ZN_HAS_FLAG (t_msg->header, _ZN_FLAG_T_I)
                _z_bytes_copy(&sc->pid, &t_msg->body.hello.pid);
            else
                _z_bytes_reset(&sc->pid);

            if _ZN_HAS_FLAG (t_msg->header, _ZN_FLAG_T_W)
                sc->whatami = t_msg->body.hello.whatami;
            else
                sc->whatami = ZN_ROUTER; // Default value is from a router

            if _ZN_HAS_FLAG (t_msg->header, _ZN_FLAG_T_L)
            {
                _z_str_array_copy(&sc->locators, &t_msg->body.hello.locators);
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

        _zn_transport_message_free(t_msg);
        _zn_transport_message_p_result_free(&r_hm);

        if (ls.len > 0 && exit_on_first)
            break;
    }

    scout_link->c_func(scout_link);
    free(scout_link);
    _z_zbuf_free(&zbf);

    return ls;
}

zn_hello_array_t _zn_scout(unsigned int what, zn_properties_t *config, unsigned long scout_period, int exit_on_first)
{
    zn_hello_array_t locs;
    locs.len = 0;
    locs.val = NULL;

    // Create the buffer to serialize the scout message on
    _z_wbuf_t wbf = _z_wbuf_make(ZN_WRITE_BUF_LEN, 0);

    // Create and encode the scout message
    _zn_transport_message_t scout = _zn_transport_message_init(_ZN_MID_SCOUT);

    // Ask for peer ID to be put in the scout message
    _ZN_SET_FLAG(scout.header, _ZN_FLAG_T_I);
    scout.body.scout.what = (z_zint_t)what;
    if (what != ZN_ROUTER)
        _ZN_SET_FLAG(scout.header, _ZN_FLAG_T_W);

    _zn_transport_message_encode(&wbf, &scout);

    // Scout on multicast
    z_str_t locator = strdup(zn_properties_get(config, ZN_CONFIG_MULTICAST_ADDRESS_KEY).val);
    z_str_t maddr = strtok(locator, ":");
    int port = atoi(strtok(NULL, ":"));

    locs = _zn_scout_loop(&wbf, maddr, port, scout_period, exit_on_first);

    _z_wbuf_free(&wbf);

    return locs;
}
