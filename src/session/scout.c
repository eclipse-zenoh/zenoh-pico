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

_z_hello_list_t *__z_scout_loop(const _z_wbuf_t *wbf, const char *locator, unsigned long period, _Bool exit_on_first) {
    // Define an empty array
    _z_hello_list_t *ret = NULL;
    int8_t err = _Z_RES_OK;

    _z_endpoint_result_t ep_res = _z_endpoint_from_str(locator);
    if ((err == _Z_RES_OK) && (ep_res._tag != _Z_RES_OK)) {
        err = _Z_ERR_LOCATOR_INVALID;
    }

#if Z_SCOUTING_UDP == 1
    if ((err == _Z_RES_OK) && (_z_str_eq(ep_res._value._locator._protocol, UDP_SCHEMA) == true)) {
        _z_endpoint_clear(&ep_res._value);
    } else
#endif
        if (err == _Z_RES_OK) {
        _z_endpoint_clear(&ep_res._value);
        err = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    if (err == _Z_RES_OK) {
        _z_link_result_t r_scout = _z_open_link(locator);
        if (r_scout._tag == _Z_RES_OK) {
            // Send the scout message
            int8_t res = _z_link_send_wbuf(&r_scout._value, wbf);
            if (res == _Z_RES_OK) {
                // The receiving buffer
                _z_zbuf_t zbf = _z_zbuf_make(Z_BATCH_SIZE_RX);

                z_time_t start = z_time_now();
                while (z_time_elapsed_ms(&start) < period) {
                    // Eventually read hello messages
                    _z_zbuf_reset(&zbf);

                    // Read bytes from the socket
                    size_t len = _z_link_recv_zbuf(&r_scout._value, &zbf, NULL);
                    if (len == SIZE_MAX) {
                        continue;
                    }

                    _z_transport_message_result_t r_hm = _z_transport_message_decode(&zbf);
                    if (r_hm._tag != _Z_RES_OK) {
                        _Z_ERROR("Scouting loop received malformed message\n");
                        continue;
                    }

                    _z_transport_message_t t_msg = r_hm._value;
                    switch (_Z_MID(t_msg._header)) {
                        case _Z_MID_HELLO: {
                            _Z_INFO("Received _Z_HELLO message\n");
                            _z_hello_t *hello = (_z_hello_t *)z_malloc(sizeof(_z_hello_t));
                            if (hello != NULL) {
                                if (_Z_HAS_FLAG(t_msg._header, _Z_FLAG_T_I) == true) {
                                    _z_bytes_copy(&hello->pid, &t_msg._body._hello._pid);
                                } else {
                                    _z_bytes_reset(&hello->pid);
                                }

                                if (_Z_HAS_FLAG(t_msg._header, _Z_FLAG_T_W) == true) {
                                    hello->whatami = t_msg._body._hello._whatami;
                                } else {
                                    hello->whatami = Z_WHATAMI_ROUTER;  // Default value is from a router
                                }

                                if (_Z_HAS_FLAG(t_msg._header, _Z_FLAG_T_L) == true) {
                                    hello->locators = _z_str_array_make(t_msg._body._hello._locators._len);
                                    for (size_t i = 0; i < hello->locators._len; i++) {
                                        hello->locators._val[i] =
                                            _z_locator_to_str(&t_msg._body._hello._locators._val[i]);
                                    }
                                } else {
                                    // @TODO: construct the locator departing from the sock address
                                    hello->locators._len = 0;
                                    hello->locators._val = NULL;
                                }

                                ret = _z_hello_list_push(ret, hello);
                            }

                            break;
                        }
                        default: {
                            // err = _Z_ERR_MESSAGE_UNEXPECTED;
                            _Z_ERROR("Scouting loop received unexpected message\n");
                            break;
                        }
                    }
                    _z_t_msg_clear(&t_msg);

                    if ((_z_hello_list_len(ret) > 0) && (exit_on_first == true)) {
                        break;
                    }
                }

                _z_link_clear(&r_scout._value);
                _z_zbuf_clear(&zbf);
            } else {
                // err = _Z_ERR_TRANSPORT_TX_FAILED;
                _z_link_clear(&r_scout._value);
            }
        } else {
            // err = _Z_ERR_TRANSPORT_OPEN_FAILED;
        }
    }

    return ret;
}

_z_hello_list_t *_z_scout_inner(const z_whatami_t what, const char *locator, const uint32_t timeout,
                                const _Bool exit_on_first) {
    _z_hello_list_t *ret = NULL;

    // Create the buffer to serialize the scout message on
    _z_wbuf_t wbf = _z_wbuf_make(Z_BATCH_SIZE_TX, false);

    // Create and encode the scout message
    _Bool request_pid = true;
    _z_transport_message_t scout = _z_t_msg_make_scout(what, request_pid);

    _z_transport_message_encode(&wbf, &scout);

    // Scout on multicast
#if Z_MULTICAST_TRANSPORT == 1
    ret = __z_scout_loop(&wbf, locator, timeout, exit_on_first);
#endif  // Z_MULTICAST_TRANSPORT == 1

    _z_wbuf_clear(&wbf);

    return ret;
}
