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

#ifndef ZENOH_C_NET_CODEC_H
#define ZENOH_C_NET_CODEC_H

#include "zenoh/codec.h"
#include "zenoh/net/result.h"
#include "zenoh/net/types.h"
#include "zenoh/net/property.h"

#define ZN_DECLARE_ENCODE(name) \
void zn_ ##name ##_encode(z_iobuf_t* buf, const zn_ ##name ##_t* m)

#define ZN_DECLARE_DECODE(name) \
zn_ ##name ##_result_t zn_ ##name ## _decode(z_iobuf_t* buf, uint8_t header); \
void zn_ ## name ## _decode_na(z_iobuf_t* buf, uint8_t header, _zn_ ##name ##_result_t *r)

#define ZN_DECLARE_DECODE_NOH(name) \
zn_ ##name ##_result_t zn_ ##name ## _decode(z_iobuf_t* buf); \
void zn_ ## name ## _decode_na(z_iobuf_t* buf, zn_ ##name ##_result_t *r)

ZN_DECLARE_ENCODE(property);
ZN_DECLARE_DECODE_NOH(property);

ZN_DECLARE_ENCODE(temporal_property);
ZN_DECLARE_DECODE_NOH(temporal_property);

void zn_properties_encode(z_iobuf_t *buf, const z_vec_t *ps);

ZN_DECLARE_ENCODE(sub_mode);
ZN_DECLARE_DECODE_NOH(sub_mode);

#endif /* ZENOH_C_NET_CODEC_H */
