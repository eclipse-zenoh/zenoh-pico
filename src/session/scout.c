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

#include <stddef.h>

#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/utils/logging.h"

#if Z_SCOUTING_UDP == 1 && Z_LINK_UDP_UNICAST == 0
#error "Scouting UDP requires UDP unicast links to be enabled (Z_LINK_UDP_UNICAST = 1 in config.h)"
#endif

_z_hello_list_t *__z_scout_loop(const _z_wbuf_t *wbf, const char *locator, unsigned long period, int exit_on_first) {
    // Define an empty array
    _z_hello_list_t *hellos = NULL;

    _z_endpoint_result_t ep_res = _z_endpoint_from_str(locator);
    if (ep_res._tag == _Z_RES_ERR) {
        goto ERR_1;
    }
    _z_endpoint_t endpoint = ep_res._value._endpoint;

#if Z_SCOUTING_UDP == 1
    if (_z_str_eq(endpoint._locator._protocol, UDP_SCHEMA)) {
        // Do nothing
    } else
#endif
    {
        goto ERR_2;
    }
    _z_endpoint_clear(&endpoint);

    _z_link_p_result_t r_scout = _z_open_link(locator);
    if (r_scout._tag == _Z_RES_ERR) {
        return hellos;
    }

    // Send the scout message
    int res = _z_link_send_wbuf(r_scout._value._link, wbf);
    if (res < 0) {
        _Z_INFO("Unable to send scout message\n");
        return hellos;
    }

    // The receiving buffer
    _z_zbuf_t zbf = _z_zbuf_make(Z_BATCH_SIZE_RX);

    z_time_t start = z_time_now();
    while (z_time_elapsed_ms(&start) < period) {
        // Eventually read hello messages
        _z_zbuf_reset(&zbf);

        // Read bytes from the socket
        size_t len = _z_link_recv_zbuf(r_scout._value._link, &zbf, NULL);
        if (len == SIZE_MAX) {
            continue;
        }

        _z_transport_message_result_t r_hm = _z_transport_message_decode(&zbf);
        if (r_hm._tag == _Z_RES_ERR) {
            _Z_ERROR("Scouting loop received malformed message\n");
            continue;
        }

        _z_transport_message_t t_msg = r_hm._value._transport_message;
        switch (_Z_MID(t_msg._header)) {
            case _Z_MID_HELLO: {
                _Z_INFO("Received _Z_HELLO message\n");
                // Get current element to fill
                _z_hello_t *hello = (_z_hello_t *)z_malloc(sizeof(_z_hello_t));

                if (_Z_HAS_FLAG(t_msg._header, _Z_FLAG_T_I) != 0) {
                    _z_bytes_copy(&hello->pid, &t_msg._body._hello._pid);
                } else {
                    _z_bytes_reset(&hello->pid);
                }

                if (_Z_HAS_FLAG(t_msg._header, _Z_FLAG_T_W) != 0) {
                    hello->whatami = t_msg._body._hello._whatami;
                } else {
                    hello->whatami = Z_WHATAMI_ROUTER;  // Default value is from a router
                }

                if (_Z_HAS_FLAG(t_msg._header, _Z_FLAG_T_L) != 0) {
                    hello->locators = _z_str_array_make(t_msg._body._hello._locators._len);
                    for (size_t i = 0; i < hello->locators._len; i++) {
                        hello->locators._val[i] = _z_locator_to_str(&t_msg._body._hello._locators._val[i]);
                    }
                } else {
                    // @TODO: construct the locator departing from the sock address
                    hello->locators._len = 0;
                    hello->locators._val = NULL;
                }

                hellos = _z_hello_list_push(hellos, hello);

                break;
            }
            default: {
                _Z_ERROR("Scouting loop received unexpected message\n");
                break;
            }
        }

        _z_t_msg_clear(&t_msg);

        if ((_z_hello_list_len(hellos) > 0) && exit_on_first) {
            break;
        }
    }

    _z_link_free(&r_scout._value._link);
    _z_zbuf_clear(&zbf);

    return hellos;

ERR_2:
    _z_endpoint_clear(&endpoint);
ERR_1:
    return hellos;
}

_z_hello_list_t *_z_scout_inner(const uint8_t what, const char *locator, const uint32_t timeout,
                                const int exit_on_first) {
    _z_hello_list_t *hellos = NULL;

    // Create the buffer to serialize the scout message on
    _z_wbuf_t wbf = _z_wbuf_make(Z_BATCH_SIZE_TX, 0);

    // Create and encode the scout message
    int request_id = 1;
    _z_transport_message_t scout = _z_t_msg_make_scout((_z_zint_t)what, request_id);

    _z_transport_message_encode(&wbf, &scout);

    // Scout on multicast
#if Z_MULTICAST_TRANSPORT == 1
    hellos = __z_scout_loop(&wbf, locator, timeout, exit_on_first);
#endif  // Z_MULTICAST_TRANSPORT == 1

    _z_wbuf_clear(&wbf);

    return hellos;
}
