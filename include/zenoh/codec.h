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

#ifndef ZENOH_C_CODEC_H
#define ZENOH_C_CODEC_H

#include "zenoh/iobuf.h"
#include "zenoh/types.h"
#include "zenoh/net/types.h"
#include "zenoh/net/property.h"

void z_vle_encode(z_iobuf_t* buf, z_vle_t v);
z_vle_result_t z_vle_decode(z_iobuf_t* buf);

void z_uint8_array_encode(z_iobuf_t* buf, const z_uint8_array_t* bs);
z_uint8_array_result_t z_uint8_array_decode(z_iobuf_t* buf);

void z_string_encode(z_iobuf_t* buf, const char* s);
z_string_result_t z_string_decode(z_iobuf_t* buf);

#endif /* ZENOH_C_CODEC_H */

