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

#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/utils/logging.h"

#if Z_SCOUTING_UDP == 1 && Z_LINK_UDP_UNICAST == 0
    #error "Scouting UDP requires UDP unicast links to be enabled (Z_LINK_UDP_UNICAST = 1 in config.h)"
#endif

_z_hello_array_t _z_scout_loop(
    const _z_wbuf_t *wbf,
    const char *locator,
    clock_t period,
    int exit_on_first)
{
    // Define an empty array
    _z_hello_array_t ls = _z_hello_array_make(0);

    _z_endpoint_result_t ep_res = _z_endpoint_from_str(locator);
    if (ep_res._tag == _Z_RES_ERR)
        goto ERR_1;
    _z_endpoint_t endpoint = ep_res._value._endpoint;

#if Z_SCOUTING_UDP == 1
    if (_z_str_eq(endpoint._locator._protocol, UDP_SCHEMA))
    {
        // Do nothing
    }
    else
#endif
        goto ERR_2;
    _z_endpoint_clear(&endpoint);

    _z_link_p_result_t r_scout = _z_open_link(locator);
    if (r_scout._tag == _Z_RES_ERR)
        return ls;

    // Send the scout message
    int res = _z_link_send_wbuf(r_scout._value._link, wbf);
    if (res < 0)
    {
        _Z_INFO("Unable to send scout message\n");
        return ls;
    }

    // The receiving buffer
    _z_zbuf_t zbf = _z_zbuf_make(Z_BATCH_SIZE);

    _z_clock_t start = _z_clock_now();
    while (_z_clock_elapsed_ms(&start) < period)
    {
        // Eventually read hello messages
        _z_zbuf_reset(&zbf);

        // Read bytes from the socket
        size_t len = _z_link_recv_zbuf(r_scout._value._link, &zbf, NULL);
        if (len == SIZE_MAX)
            continue;

        _z_transport_message_result_t r_hm = _z_transport_message_decode(&zbf);
        if (r_hm._tag == _Z_RES_ERR)
        {
            _Z_ERROR("Scouting loop received malformed message\n");
            continue;
        }

        _z_transport_message_t t_msg = r_hm._value._transport_message;
        switch (_Z_MID(t_msg._header))
        {
        case _Z_MID_HELLO:
        {
            _Z_INFO("Received _Z_HELLO message\n");
            // FIXME: hide this behind the array implementation
            // Allocate or expand the vector
            if (ls._val)
            {
                _z_hello_t *val = (_z_hello_t *)malloc((ls._len + 1) * sizeof(_z_hello_t));
                memcpy(val, ls._val, ls._len);
                free(ls._val);
                ls._val = val;
            }
            else
            {
                ls._val = (_z_hello_t *)malloc(sizeof(_z_hello_t));
            }
            ls._len++;

            // Get current element to fill
            _z_hello_t *sc = (_z_hello_t *)&ls._val[ls._len - 1];

            if _Z_HAS_FLAG (t_msg._header, _Z_FLAG_T_I)
                _z_bytes_copy(&sc->pid, &t_msg._body._hello._pid);
            else
                _z_bytes_reset(&sc->pid);

            if _Z_HAS_FLAG (t_msg._header, _Z_FLAG_T_W)
                sc->whatami = t_msg._body._hello._whatami;
            else
                sc->whatami = Z_ROUTER; // Default value is from a router

            if _Z_HAS_FLAG (t_msg._header, _Z_FLAG_T_L)
            {
                sc->locators = _z_str_array_make(t_msg._body._hello._locators._len);
                for (size_t i = 0; i < sc->locators._len; i++)
                    sc->locators._val[i] = _z_locator_to_str(&t_msg._body._hello._locators._val[i]);
            }
            else
            {
                // @TODO: construct the locator departing from the sock address
                sc->locators._len = 0;
                sc->locators._val = NULL;
            }

            break;
        }
        default:
        {
            _Z_ERROR("Scouting loop received unexpected message\n");
            break;
        }
        }

        _z_t_msg_clear(&t_msg);

        if (ls._len > 0 && exit_on_first)
            break;
    }

    _z_link_free(&r_scout._value._link);
    _z_zbuf_clear(&zbf);

    return ls;

ERR_2:
    _z_endpoint_clear(&endpoint);
ERR_1:
    return ls;
}

_z_hello_array_t _z_scout_inner(const _z_zint_t what, const _z_config_t *config, const uint32_t scout_period, const int exit_on_first)
{
    _z_hello_array_t locs = _z_hello_array_make(0);

    // Create the buffer to serialize the scout message on
    _z_wbuf_t wbf = _z_wbuf_make(Z_BATCH_SIZE, 0);

    // Create and encode the scout message
    int request_id = 1;
    _z_transport_message_t scout = _z_t_msg_make_scout((_z_zint_t)what, request_id);

    _z_transport_message_encode(&wbf, &scout);

    // Scout on multicast
    const char *locator = _z_config_get(config, Z_CONFIG_MULTICAST_ADDRESS_KEY);
    locs = _z_scout_loop(&wbf, locator, scout_period, exit_on_first);

    _z_wbuf_clear(&wbf);

    return locs;
}
