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

#include <stdint.h>

#include "zenoh-pico/net/session.h"
#include "zenoh-pico/protocol/core.h"
#include "zenoh-pico/protocol/definitions/message.h"
#include "zenoh-pico/protocol/definitions/network.h"

#ifndef ZENOH_PICO_SESSION_REPLY_H
#define ZENOH_PICO_SESSION_REPLY_H

int8_t _z_trigger_reply_partial(_z_session_t *zn, _z_zint_t id, _z_keyexpr_t key, _z_msg_reply_t *reply);

int8_t _z_trigger_reply_err(_z_session_t *zn, _z_zint_t id, _z_msg_err_t *error);

int8_t _z_trigger_reply_final(_z_session_t *zn, _z_n_msg_response_final_t *final);

#endif /* ZENOH_PICO_SESSION_REPLY_H */
