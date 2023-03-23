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

    _z_endpoint_t ep;
    err = _z_endpoint_from_str(&ep, locator);

#if Z_SCOUTING_UDP == 1
    if ((err == _Z_RES_OK) && (_z_str_eq(ep._locator._protocol, UDP_SCHEMA) == true)) {
        _z_endpoint_clear(&ep);
    } else
#endif
        if (err == _Z_RES_OK) {
        _z_endpoint_clear(&ep);
        err = _Z_ERR_TRANSPORT_NOT_AVAILABLE;
    }

    if (err == _Z_RES_OK) {
        _z_link_t zl;
        err = _z_open_link(&zl, locator);
        if (err == _Z_RES_OK) {
            // Send the scout message
            err = _z_link_send_wbuf(&zl, wbf);
            if (err == _Z_RES_OK) {
                // The receiving buffer
                _z_zbuf_t zbf = _z_zbuf_make(Z_BATCH_SIZE);

                z_time_t start = z_time_now();
                while (z_time_elapsed_ms(&start) < period) {
                    // Eventually read hello messages
                    _z_zbuf_reset(&zbf);

                    // Read bytes from the socket
                    size_t len = _z_link_recv_zbuf(&zl, &zbf, NULL);
                    if (len == SIZE_MAX) {
                        continue;
                    }

                    _z_scouting_message_t s_msg;
                    err = _z_scouting_message_decode(&s_msg, &zbf);
                    if (err != _Z_RES_OK) {
                        _Z_ERROR("Scouting loop received malformed message\n");
                        continue;
                    }

                    switch (_Z_MID(s_msg._header)) {
                        case _Z_MID_HELLO: {
                            _Z_INFO("Received _Z_HELLO message\n");
                            _z_hello_t *hello = (_z_hello_t *)z_malloc(sizeof(_z_hello_t));
                            if (hello != NULL) {
                                hello->version = s_msg._body._hello._version;
                                hello->whatami = s_msg._body._hello._whatami;
                                _z_bytes_copy(&hello->zid, &s_msg._body._hello._zid);

                                size_t n_loc = _z_locator_array_len(&s_msg._body._hello._locators);
                                if (n_loc > 0) {
                                    hello->locators = _z_str_array_make(n_loc);
                                    for (size_t i = 0; i < n_loc; i++) {
                                        hello->locators._val[i] =
                                            _z_locator_to_str(&s_msg._body._hello._locators._val[i]);
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
                            err = _Z_ERR_MESSAGE_UNEXPECTED;
                            _Z_ERROR("Scouting loop received unexpected message\n");
                            break;
                        }
                    }
                    _z_s_msg_clear(&s_msg);

                    if ((_z_hello_list_len(ret) > 0) && (exit_on_first == true)) {
                        break;
                    }
                }

                _z_link_clear(&zl);
                _z_zbuf_clear(&zbf);
            } else {
                err = _Z_ERR_TRANSPORT_TX_FAILED;
                _z_link_clear(&zl);
            }
        } else {
            err = _Z_ERR_TRANSPORT_OPEN_FAILED;
        }
    }

    (void)(err);
    return ret;
}

_z_hello_list_t *_z_scout_inner(const z_what_t what, const _z_bytes_t zid, const char *locator, const uint32_t timeout,
                                const _Bool exit_on_first) {
    _z_hello_list_t *ret = NULL;

    // Create the buffer to serialize the scout message on
    _z_wbuf_t wbf = _z_wbuf_make(Z_BATCH_SIZE, false);

    // Create and encode the scout message
    _z_scouting_message_t scout = _z_s_msg_make_scout(what, zid);

    _z_scouting_message_encode(&wbf, &scout);

    // Scout on multicast
#if Z_MULTICAST_TRANSPORT == 1
    ret = __z_scout_loop(&wbf, locator, timeout, exit_on_first);
#else
    (void)(locator);
    (void)(timeout);
    (void)(exit_on_first);
#endif  // Z_MULTICAST_TRANSPORT == 1

    _z_wbuf_clear(&wbf);

    return ret;
}
