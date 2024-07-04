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
#include <string.h>

#include "zenoh-pico/link/manager.h"
#include "zenoh-pico/protocol/codec/transport.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/transport/multicast.h"
#include "zenoh-pico/utils/logging.h"

#if Z_FEATURE_SCOUTING_UDP == 1 && Z_FEATURE_LINK_UDP_UNICAST == 0
#error "Scouting UDP requires UDP unicast links to be enabled (Z_FEATURE_LINK_UDP_UNICAST = 1 in config.h)"
#endif

_z_hello_list_t *__z_scout_loop(const _z_wbuf_t *wbf, const char *locator, unsigned long period, _Bool exit_on_first) {
    // Define an empty array
    _z_hello_list_t *ret = NULL;
    int8_t err = _Z_RES_OK;

    _z_endpoint_t ep;
    err = _z_endpoint_from_str(&ep, locator);

#if Z_FEATURE_SCOUTING_UDP == 1
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
        memset(&zl, 0, sizeof(_z_link_t));

        err = _z_open_link(&zl, locator);
        if (err == _Z_RES_OK) {
            // Send the scout message
            err = _z_link_send_wbuf(&zl, wbf);
            if (err == _Z_RES_OK) {
                // The receiving buffer
                _z_zbuf_t zbf = _z_zbuf_make(Z_BATCH_UNICAST_SIZE);

                z_clock_t start = z_clock_now();
                while (z_clock_elapsed_ms(&start) < period) {
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
                        _Z_ERROR("Scouting loop received malformed message");
                        continue;
                    }

                    switch (_Z_MID(s_msg._header)) {
                        case _Z_MID_HELLO: {
                            _Z_INFO("Received _Z_HELLO message");
                            _z_hello_t *hello = (_z_hello_t *)z_malloc(sizeof(_z_hello_t));
                            if (hello != NULL) {
                                hello->_version = s_msg._body._hello._version;
                                hello->_whatami = s_msg._body._hello._whatami;
                                memcpy(hello->_zid.id, s_msg._body._hello._zid.id, 16);

                                size_t n_loc = _z_locator_array_len(&s_msg._body._hello._locators);
                                if (n_loc > 0) {
                                    hello->_locators = _z_string_svec_make(n_loc);

                                    for (size_t i = 0; i < n_loc; i++) {
                                        _z_string_t s = _z_locator_to_string(&s_msg._body._hello._locators._val[i]);
                                        _z_string_svec_append(&hello->_locators, &s);
                                    }
                                } else {
                                    // @TODO: construct the locator departing from the sock address
                                    _z_string_svec_clear(&hello->_locators);
                                }

                                ret = _z_hello_list_push(ret, hello);
                            }

                            break;
                        }
                        default: {
                            err = _Z_ERR_MESSAGE_UNEXPECTED;
                            _Z_ERROR("Scouting loop received unexpected message");
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

_z_hello_list_t *_z_scout_inner(const z_what_t what, _z_id_t zid, const char *locator, const uint32_t timeout,
                                const _Bool exit_on_first) {
    _z_hello_list_t *ret = NULL;

    // Create the buffer to serialize the scout message on
    _z_wbuf_t wbf = _z_wbuf_make(Z_BATCH_UNICAST_SIZE, false);

    // Create and encode the scout message
    _z_scouting_message_t scout = _z_s_msg_make_scout(what, zid);

    _z_scouting_message_encode(&wbf, &scout);

    // Scout on multicast
    ret = __z_scout_loop(&wbf, locator, timeout, exit_on_first);

    _z_wbuf_clear(&wbf);

    return ret;
}
