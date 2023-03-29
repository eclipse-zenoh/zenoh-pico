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

#include "zenoh-pico/protocol/keyexpr.h"
#include "zenoh-pico/protocol/msgcodec.h"
#include "zenoh-pico/session/query.h"
#include "zenoh-pico/session/queryable.h"
#include "zenoh-pico/session/resource.h"
#include "zenoh-pico/session/subscription.h"
#include "zenoh-pico/utils/logging.h"

/*------------------ Handle message ------------------*/
int8_t _z_handle_zenoh_message(_z_session_t *zn, _z_zenoh_message_t *msg) {
    int8_t ret = _Z_RES_OK;

    switch (_Z_MID(msg->_header)) {
        case _Z_MID_Z_DATA: {
            _Z_INFO("Received _Z_MID_Z_DATA message %d\n", msg->_header);
            if (msg->_reply_context != NULL) {  // This is some data from a query
                ret = _z_trigger_query_reply_partial(zn, msg->_reply_context, msg->_body._data._key,
                                                     msg->_body._data._payload, msg->_body._data._info._encoding,
                                                     msg->_body._data._info._kind, msg->_body._data._info._tstamp);
            } else {  // This is pure data
                ret = _z_trigger_subscriptions(zn, msg->_body._data._key, msg->_body._data._payload,
                                               msg->_body._data._info._encoding, msg->_body._data._info._kind,
                                               msg->_body._data._info._tstamp);
            }
        } break;

        case _Z_MID_Z_PULL: {
            _Z_INFO("Received _Z_PULL message\n");
            // TODO: not supported yet
        } break;

        case _Z_MID_Z_QUERY: {
            _Z_INFO("Received _Z_QUERY message\n");
            _z_trigger_queryables(zn, &msg->_body._query);
        } break;

        case _Z_MID_Z_UNIT: {
            _Z_INFO("Received _Z_UNIT message\n");
            // This might be the final reply
            if (msg->_reply_context != NULL) {
                _z_trigger_query_reply_final(zn, msg->_reply_context);
            }
        } break;

        default: {
            _Z_ERROR("Unknown zenoh message ID\n");
            ret = _Z_ERR_MESSAGE_ZENOH_UNKNOWN;
        }
    }

    return ret;
}
