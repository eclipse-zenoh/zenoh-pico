/*
 * Copyright (c) 2017, 2020 ADLINK Technology Inc.
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

#ifndef ZENOH_C_NET_MSGCODEC_H
#define ZENOH_C_NET_MSGCODEC_H

#include "zenoh/net/codec.h"
#include "zenoh/net/result.h"
#include "zenoh/net/types.h"
#include "zenoh/net/property.h"
#include "zenoh/net/private/msg.h"

/*------------------ Session Message ------------------*/
void _zn_session_message_encode(z_iobuf_t *buf, const _zn_session_message_t *m);
_zn_session_message_p_result_t z_session_message_decode(z_iobuf_t *buf);
void _zn_session_message_decode_na(z_iobuf_t *buf, _zn_session_message_p_result_t *r);

/*------------------ Zenoh Message ------------------*/
void _zn_zenoh_message_encode(z_iobuf_t *buf, const _zn_zenoh_message_t *m);
_zn_zenoh_message_p_result_t z_zenoh_message_decode(z_iobuf_t *buf);
void _zn_zenoh_message_decode_na(z_iobuf_t *buf, _zn_zenoh_message_p_result_t *r);

#endif /* ZENOH_C_NET_MSGCODEC_H */
