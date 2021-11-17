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

#include "zenoh-pico/protocol/msg.h"
#include "zenoh-pico/protocol/msgcodec.h"

/*------------------ Message helpers ------------------*/
_zn_transport_message_t _zn_transport_message_init(uint8_t header)
{
    _zn_transport_message_t sm;
    memset(&sm, 0, sizeof(_zn_transport_message_t));
    sm.header = header;
    return sm;
}

_zn_zenoh_message_t _zn_zenoh_message_init(uint8_t header)
{
    _zn_zenoh_message_t zm;
    memset(&zm, 0, sizeof(_zn_zenoh_message_t));
    zm.header = header;
    return zm;
}

_zn_reply_context_t *_zn_reply_context_init(void)
{
    _zn_reply_context_t *rc = (_zn_reply_context_t *)malloc(sizeof(_zn_reply_context_t));
    memset(rc, 0, sizeof(_zn_reply_context_t));
    rc->header = _ZN_MID_REPLY_CONTEXT;
    return rc;
}

_zn_attachment_t *_zn_attachment_init(void)
{
    _zn_attachment_t *att = (_zn_attachment_t *)malloc(sizeof(_zn_attachment_t));
    memset(att, 0, sizeof(_zn_attachment_t));
    att->header = _ZN_MID_ATTACHMENT;
    return att;
}
