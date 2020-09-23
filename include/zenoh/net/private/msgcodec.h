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


#define _ZN_DECLARE_ENCODE(name) \
void _zn_ ##name ##_encode(z_iobuf_t* buf, const _zn_ ##name ##_t* m)

#define _ZN_DECLARE_DECODE(name) \
_zn_ ##name ##_result_t z_ ##name ## _decode(z_iobuf_t* buf, uint8_t header); \
void _zn_ ## name ## _decode_na(z_iobuf_t* buf, uint8_t header, _zn_ ##name ##_result_t *r)

#define _ZN_DECLARE_DECODE_NOH(name) \
_zn_ ##name ##_result_t z_ ##name ## _decode(z_iobuf_t* buf); \
void _zn_ ## name ## _decode_na(z_iobuf_t* buf, _zn_ ##name ##_result_t *r)


_ZN_DECLARE_ENCODE(scout);
_ZN_DECLARE_DECODE_NOH(scout);

_ZN_DECLARE_ENCODE(hello);
_ZN_DECLARE_DECODE_NOH(hello);


_ZN_DECLARE_ENCODE(open);
_ZN_DECLARE_DECODE_NOH(accept);

_ZN_DECLARE_ENCODE(close);
_ZN_DECLARE_DECODE_NOH(close);

_ZN_DECLARE_ENCODE(declare);
_ZN_DECLARE_DECODE_NOH(declare);

_ZN_DECLARE_ENCODE(compact_data);
_ZN_DECLARE_DECODE_NOH(compact_data);

_ZN_DECLARE_ENCODE(payload_header);
_ZN_DECLARE_DECODE_NOH(payload_header);

_ZN_DECLARE_ENCODE(stream_data);
_ZN_DECLARE_DECODE_NOH(stream_data);

_ZN_DECLARE_ENCODE(write_data);
_ZN_DECLARE_DECODE_NOH(write_data);

_ZN_DECLARE_ENCODE(query);
_ZN_DECLARE_DECODE_NOH(query);

void _zn_message_encode(z_iobuf_t* buf, const _zn_message_t* m);
_zn_message_p_result_t z_message_decode(z_iobuf_t* buf);
void _zn_message_decode_na(z_iobuf_t* buf, _zn_message_p_result_t *r);

#endif /* ZENOH_C_NET_MSGCODEC_H */
