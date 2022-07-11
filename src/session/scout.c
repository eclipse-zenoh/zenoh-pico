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

#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/utils/logging.h"

#if ZN_SCOUTING_UDP == 1 && ZN_LINK_UDP_UNICAST == 0
    #error "Scouting UDP requires UDP unicast links to be enabled (ZN_LINK_UDP_UNICAST = 1 in config.h)"
#endif

zn_hello_array_t _zn_scout_loop(
    const _z_wbuf_t *wbf,
    const z_str_t locator,
    clock_t period,
    int exit_on_first)
{
    // Define an empty array
    zn_hello_array_t ls;
    ls.len = 0;
    ls.val = NULL;

    _zn_endpoint_result_t ep_res = _zn_endpoint_from_str(locator);
    if (ep_res.tag == _z_res_t_ERR)
        goto ERR_1;
    _zn_endpoint_t endpoint = ep_res.value.endpoint;

#if ZN_SCOUTING_UDP == 1
    if (_z_str_eq(endpoint.locator.protocol, UDP_SCHEMA))
    {
        // Do nothing
    }
    else
#endif
        goto ERR_2;
    _zn_endpoint_clear(&endpoint);

    _zn_link_p_result_t r_scout = _zn_open_link(locator);
    if (r_scout.tag == _z_res_t_ERR)
        return ls;

    // Send the scout message
    int res = _zn_link_send_wbuf(r_scout.value.link, wbf);
    if (res < 0)
    {
        _Z_INFO("Unable to send scout message\n");
        return ls;
    }

    // The receiving buffer
    _z_zbuf_t zbf = _z_zbuf_make(ZN_BATCH_SIZE);

    z_time_t start = z_time_now();
    while (z_time_elapsed_ms(&start) < period)
    {
        // Eventually read hello messages
        _z_zbuf_reset(&zbf);

        // Read bytes from the socket
        size_t len = _zn_link_recv_zbuf(r_scout.value.link, &zbf, NULL);
        if (len == SIZE_MAX)
            continue;

        _zn_transport_message_result_t r_hm = _zn_transport_message_decode(&zbf);
        if (r_hm.tag == _z_res_t_ERR)
        {
            _Z_ERROR("Scouting loop received malformed message\n");
            continue;
        }

        _zn_transport_message_t t_msg = r_hm.value.transport_message;
        switch (_ZN_MID(t_msg.header))
        {
        case _ZN_MID_HELLO:
        {
            _Z_INFO("Received _ZN_HELLO message\n");
            // Allocate or expand the vector
            if (ls.val)
            {
                zn_hello_t *val = (zn_hello_t *)z_malloc((ls.len + 1) * sizeof(zn_hello_t));
                memcpy(val, ls.val, ls.len);
                z_free((zn_hello_t *)ls.val);
                ls.val = val;
            }
            else
            {
                ls.val = (zn_hello_t *)z_malloc(sizeof(zn_hello_t));
            }
            ls.len++;

            // Get current element to fill
            zn_hello_t *sc = (zn_hello_t *)&ls.val[ls.len - 1];

            if _ZN_HAS_FLAG (t_msg.header, _ZN_FLAG_T_I)
                _z_bytes_copy(&sc->pid, &t_msg.body.hello.pid);
            else
                _z_bytes_reset(&sc->pid);

            if _ZN_HAS_FLAG (t_msg.header, _ZN_FLAG_T_W)
                sc->whatami = t_msg.body.hello.whatami;
            else
                sc->whatami = ZN_ROUTER; // Default value is from a router

            if _ZN_HAS_FLAG (t_msg.header, _ZN_FLAG_T_L)
            {
                z_str_array_t la = _z_str_array_make(t_msg.body.hello.locators.len);
                for (size_t i = 0; i < la.len; i++)
                {
                    la.val[i] = _zn_locator_to_str(&t_msg.body.hello.locators.val[i]);
                }
                _z_str_array_move(&sc->locators, &la);
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
            _Z_ERROR("Scouting loop received unexpected message\n");
            break;
        }
        }

        _zn_t_msg_clear(&t_msg);

        if (ls.len > 0 && exit_on_first)
            break;
    }

    _zn_link_free(&r_scout.value.link);
    _z_zbuf_clear(&zbf);

    return ls;

ERR_2:
    _zn_endpoint_clear(&endpoint);
ERR_1:
    return ls;
}

zn_hello_array_t _zn_scout(const unsigned int what, const zn_properties_t *config, const unsigned long scout_period, const int exit_on_first)
{
    zn_hello_array_t locs;
    locs.len = 0;
    locs.val = NULL;

    // Create the buffer to serialize the scout message on
    _z_wbuf_t wbf = _z_wbuf_make(ZN_BATCH_SIZE, 0);

    // Create and encode the scout message
    int request_id = 1;
    _zn_transport_message_t scout = _zn_t_msg_make_scout((z_zint_t)what, request_id);

    _zn_transport_message_encode(&wbf, &scout);

    // Scout on multicast
    const z_str_t locator = zn_properties_get(config, ZN_CONFIG_MULTICAST_ADDRESS_KEY).val;
    locs = _zn_scout_loop(&wbf, locator, scout_period, exit_on_first);

    _z_wbuf_clear(&wbf);

    return locs;
}
